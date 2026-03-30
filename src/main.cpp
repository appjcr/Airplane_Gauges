/* Using LVGL with Arduino requires some extra steps:
   Be sure to read the docs here: https://docs.lvgl.io/master/details/integration/framework/arduino.html
   To use the built-in examples and demos of LVGL uncomment the includes below respectively.
   You also need to copy 'lvgl/examples' to 'lvgl/src/examples'. Similarly for the demos 'lvgl/demos' to 'lvgl/src/demos'.
*/
#include <Arduino.h>
#include <lvgl.h>
#include "pincfg.h"
#include "dispcfg.h"
#include "AXS15231B_touch.h"
#include <Arduino_GFX_Library.h>

#include <Wire.h>
#include <Adafruit_ADS7830.h>
Adafruit_ADS7830 ad7830;

#include <fuel_gauge.h>
#include <flaps_gauge.h>
#include <trim_gauge.h>
#include <flow_gauge.h>

// Define I2C pins
#define I2C_SDA 18
#define I2C_SCL 17

const int sensorPin = 2; // Pin with Interrupt
volatile long pulseCount = 0;
float flowRate = 0.0;
unsigned int flowMilliLitres = 0;
unsigned long totalMilliLitres = 0;
unsigned long oldTime = 0;

uint8_t value[7];

const int numReadings_L = 50; // Number of samples for average
int readings_L[numReadings_L];  // Array to store readings
int readIndex_L = 0;          // Current index in array
long total_L = 0;             // Running total
int averageFuel_L = 0;        // Final average

const int numReadings_R = 50; // Number of samples for average
int readings_R[numReadings_R];  // Array to store readings
int readIndex_R = 0;          // Current index in array
long total_R = 0;             // Running total
int averageFuel_R = 0;        // Final average


Arduino_DataBus *bus = new Arduino_ESP32QSPI(TFT_CS, TFT_SCK, TFT_SDA0, TFT_SDA1, TFT_SDA2, TFT_SDA3);
Arduino_GFX *g = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED, 0, false, TFT_res_W, TFT_res_H);
Arduino_Canvas *gfx = new Arduino_Canvas(TFT_res_W, TFT_res_H, g, 0, 0, TFT_rot);
AXS15231B_Touch touch(Touch_SCL, Touch_SDA, Touch_INT, Touch_ADDR, TFT_rot);

static int32_t hold_value=99;
static int32_t fuel_l_hold_value=99;
static int32_t fuel_r_hold_value=99;
static int32_t flaps_hold_value=99;
static int32_t elev_hold_value=99;
static int32_t ailer_hold_value=99;
static int32_t flow_hold_value=99;

#if LV_USE_LOG != 0
// Log to serial console
void my_print(lv_log_level_t level, const char *buf) {
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

// Callback so LVGL know the elapsed time
uint32_t millis_cb(void) {
    return millis();
}

// LVGL calls it when a rendered image needs to copied to the display
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);

    // Call it to tell LVGL everthing is ready
    lv_disp_flush_ready(disp);
}

// Read the touchpad
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t x, y;
    if (touch.touched()) {
        // Read touched point from touch module
        touch.readData(&x, &y);

        // Set the coordinates
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

int32_t smooth_fuel_readings_L (int32_t read_fuel_value_L ) {
    total_L = total_L - readings_L[readIndex_L];
    readings_L[readIndex_L] = read_fuel_value_L;
    total_L = total_L + readings_L[readIndex_L];
    readIndex_L = readIndex_L + 1;
    if (readIndex_L >= numReadings_L) readIndex_L = 0;
    averageFuel_L = total_L / numReadings_L;
    Serial.print("Smoothed Level left: ");
    Serial.println(averageFuel_L);
    return(averageFuel_L);
}

int32_t smooth_fuel_readings_R (int32_t read_fuel_value_R ) {
    total_R = total_R - readings_R[readIndex_R];
    readings_R[readIndex_R] = read_fuel_value_R;
    total_R = total_R + readings_R[readIndex_R];
    readIndex_R = readIndex_R + 1;
    if (readIndex_R >= numReadings_R) readIndex_R = 0;
    averageFuel_R = total_R / numReadings_R;
    Serial.print("Smoothed Level right: ");
    Serial.println(averageFuel_R);
    return(averageFuel_R);
}

static void sensors_timer_cb(lv_timer_t * timer1)
{
    LV_UNUSED(timer1);
    // read sensors
    Fuel_L_value = smooth_fuel_readings_L(ad7830.readADCsingle(0));
    Serial.printf("Fuel_L_value: %" PRId32 "\n", Fuel_L_value);

    Fuel_R_value = smooth_fuel_readings_R(ad7830.readADCsingle(1));
    Serial.printf("Fuel_R_value: %" PRId32 "\n", Fuel_R_value);

    elev_trim_value = ad7830.readADCsingle(2);
    Serial.printf("elev_trim_value: %" PRId32 "\n", elev_trim_value);

    ailer_trim_value = ad7830.readADCsingle(3);
    Serial.printf("ailer_trim_value: %" PRId32 "\n", ailer_trim_value);

    Flaps_position_value = ad7830.readADCsingle(4);
    Serial.printf("Flaps_position_value: %" PRId32 "\n", Flaps_position_value);

}


void pulseCounter() {
  pulseCount++;
}

static void flow_sensor_timer_cb(lv_timer_t * timer1) {
    detachInterrupt(digitalPinToInterrupt(sensorPin));
    
    // Formula: Freq / 7.5 = L/min
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / 7.5;
    oldTime = millis();
    
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;
    
    Serial.print("Flow Rate: ");
    Serial.print(flowRate);
    Serial.print(" L/min");
    Serial.print("\tTotal: ");
    Serial.print(totalMilliLitres);
    Serial.println(" mL");

    flow_value= flowRate;
    
    pulseCount = 0;
    attachInterrupt(digitalPinToInterrupt(sensorPin), pulseCounter, FALLING);
}

void setup() {
//    delay(10000);
    #ifdef ARDUINO_USB_CDC_ON_BOOT
    delay(2000);
    #endif

    Serial.begin(115200);
    Serial.println("Arduino_GFX LVGL ");
    String LVGL_Arduino = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch() + " example";
    Serial.println(LVGL_Arduino);

    // Display setup
    if(!gfx->begin(40000000UL)) {
        Serial.println("Failed to initialize display!");
        return;
    }
    gfx->fillScreen(BLACK);

    // Switch backlight on
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // Touch setup
    if(!touch.begin()) {
        Serial.println("Failed to initialize touch module!");
        return;
    }
    touch.enOffsetCorrection(true);
    touch.setOffsets(Touch_X_min, Touch_X_max, TFT_res_W-1, Touch_Y_min, Touch_Y_max, TFT_res_H-1);

    // Init LVGL
    lv_init();

    // Set a tick source so that LVGL will know how much time elapsed
    lv_tick_set_cb(millis_cb);

    // Register print function for debugging
    #if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print);
    #endif

    // Initialize the display buffer
    uint32_t screenWidth = gfx->width();
    uint32_t screenHeight = gfx->height();
    uint32_t bufSize = screenWidth * screenHeight / 10;
    lv_color_t *disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) {
        Serial.println("LVGL failed to allocate display buffer!");
        return;
    }

    // Initialize the display driver
    lv_display_t *disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, disp_draw_buf, NULL, bufSize * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Initialize the input device driver
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    // Set background color of main screen
    lv_obj_t * scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_palette_main(LV_PALETTE_NONE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.println("Adafruit ADS7830 start\n");
    if (!ad7830.begin()) {
        Serial.println("Failed to initialize ADS7830!\n");
        while (1);
    }
    // Create timer to run the sensors for fuel, flap, trim
    lv_timer_create(sensors_timer_cb, 100, NULL);

    pinMode(sensorPin, INPUT);
    // Trigger pulseCounter function on falling edge
    attachInterrupt(digitalPinToInterrupt(sensorPin), pulseCounter, FALLING);
    // Create the timer to run the flow sensor
    lv_timer_create(flow_sensor_timer_cb, 1000, NULL);

    // Initialize all Fuel readings to 0
    for (int i = 0; i < numReadings_L; i++) readings_L[i] = 0;
    for (int i = 0; i < numReadings_R; i++) readings_R[i] = 0;

    // Update display Fuel left gauge every 500ms
    fuel_gaugeL(500);
    // Update display Fuel right gauge every 500ms
    fuel_gaugeR(500);
    // Update display Flaps gauge every 250ms
    flaps_gauge(250);
    // Update display Trim every 500ms
    trim_gauge(500);
    // Update display Flow gauge every 1000ms
    flow_gauge(1000);


}

void loop() {
    //if(hold_value!=Flaps_position_value) {
    //    hold_value=Flaps_position_value;
    //    Serial.printf("Flaps position is: %d\n", Flaps_position_value);
    //}
    //if(hold_value!=Flaps_position_value) {
    //    hold_value=Flaps_position_value;
    //    Serial.printf("Flaps position is: %d\n", Flaps_position_value);
    //}
    //if(elev_hold_value!=elev_trim_value) {
    //    elev_hold_value=elev_trim_value;
    //    Serial.printf("Elev trim position is: %d\n", elev_trim_value);
    //}
    //if(ailer_hold_value!=ailer_trim_value) {
    //    ailer_hold_value=ailer_trim_value;
    //    Serial.printf("Ailer trim position is: %d\n", ailer_trim_value);
    //}
    //if(flow_hold_value!=flow_value) {
    //    flow_hold_value=flow_value;
    //    Serial.printf("Flow value is: %d\n", flow_value);
    //}


    // Automatically calls lv_timer_handler() every 5ms
    lv_timer_handler_run_in_period(5);
    gfx->flush();

}
