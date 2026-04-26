/* Using LVGL with Arduino requires some extra steps:
   Be sure to read the docs here: https://docs.lvgl.io/master/details/integration/framework/arduino.html
   To use the built-in examples and demos of LVGL uncomment the includes below respectively.
   You also need to copy 'lvgl/examples' to 'lvgl/src/examples'. Similarly for the demos 'lvgl/demos' to 'lvgl/src/demos'.
*/
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_ADS7830.h>
Adafruit_ADS7830 ad7830;

#include <lvgl.h>
#include "pincfg.h"
#include "dispcfg.h"
#include "AXS15231B_touch.h"
#include <Arduino_GFX_Library.h>
#include <Preferences.h>

#include <fuel_gauge.h>
#include <flaps_gauge.h>
#include <trim_gauge.h>
#include <flow_gauge.h>

lv_obj_t * screen_setup;
lv_obj_t * screen_gauges;

// Persistent storage instance
Preferences prefs;
lv_obj_t *roller_left, *roller_right;
// Constants
const char* fuel_options = "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12"; // Options for fuel levels (12 to 0 gallons)
const int MAX_FUEL = 12;
int left_val = 0;
int right_val = 0;


bool first_time = true;
bool startup = true;
bool reverse_flag = false;
int16_t startup_int = 0;
static uint32_t last_run = 0; 
const uint32_t interval = 200; // in ms

// Define I2C for ADS7830 pins used
#define ADS_SDA 17
#define ADS_SCL 18

// Pin 5 as RX Left tank, Pin 6 as RX Right tank, Pin -1 as TX (not used)
SoftwareSerial mySerial_L(5, -1); 
SoftwareSerial mySerial_R(6, -1); 

const int sensorPin = 7; // Pin with Interrupt for flow meter
// Constants for GFS402T
const float PULSES_PER_LITER = 4380.0;
const float LITERS_PER_GALLON = 3.78541;
const float K_FACTOR_GALLONS = PULSES_PER_LITER * LITERS_PER_GALLON; // ~16580 pulses/gal
volatile uint32_t pulse_count = 0;
uint32_t last_pulse_count = 0;
float total_gallons = 0.0;
float current_gph = 0.0;

int32_t L_counter = 0;
int32_t bytesRead_L = 0;
uint8_t buffer_L[10] = {0,0,0,0,0,0,0,0,0,0}; // Adjust size based on expected line length
int32_t R_counter = 0;
int32_t bytesRead_R = 0;
uint8_t buffer_R[10] = {0,0,0,0,0,0,0,0,0,0}; // Adjust size based on expected line length
uint8_t value[7];

int16_t Empty_fuel_L_capacitance_value = 1590;
int16_t Full_fuel_L_capacitance_value = 805;
int16_t Empty_fuel_R_capacitance_value = 1590;
int16_t Full_fuel_R_capacitance_value = 805;
int16_t No_flaps_resistance_value = 0;
int16_t Full_flaps_resistance_value = 255;
int16_t Down_elev_resistance_value = 0;
int16_t Up_elev_resistance_value = 255;
int16_t Down_ailer_resistance_value = 0;
int16_t Up_ailer_resistance_value = 255;

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

// Retrieve initial values from permanent memory
static void get_fuel() {
    prefs.begin("fuel_data", true); // Open in Read-Only
    left_val = prefs.getInt("left", 0); // Default to 0 if not found
    right_val = prefs.getInt("right", 0);
    prefs.end();
}

// (Saves to Permanent Memory)
static void save_fuel() {
    prefs.begin("fuel_data", false); // Open namespace in R/W mode
    prefs.putInt("left", left_val);
    prefs.putInt("right", right_val);
    prefs.end();
    Serial.printf("Saved: Left %d, Right %d\n", left_val, right_val);
}

// Event handler for "Full Fuel" button and save to permanent memory
static void full_fuel_event_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_roller_set_selected(roller_left, MAX_FUEL, LV_ANIM_ON);
        lv_roller_set_selected(roller_right, MAX_FUEL, LV_ANIM_ON);
        left_val = MAX_FUEL;
        right_val = MAX_FUEL;
        save_fuel(); // Save to permanent memory
        lv_screen_load(screen_gauges);
    }
}

// Event handler for "Update" button (Saves to Permanent Memory)
static void update_event_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        left_val = lv_roller_get_selected(roller_left);
        right_val = lv_roller_get_selected(roller_right);
        save_fuel(); // Save to permanent memory
        lv_screen_load(screen_gauges);
    }
}

// Event handler for "Setup" button
static void switch_screen_to_setup_event_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        get_fuel(); // Retrieve from permanent memory
        lv_roller_set_selected(roller_left, left_val, LV_ANIM_OFF);
        lv_roller_set_selected(roller_right, right_val, LV_ANIM_OFF);
        lv_screen_load(screen_setup);
    }
}

void setup_fuel_gui() {
    get_fuel(); // Retrieve from permanent memory last saved fuel levels for left and right tanks

    screen_setup = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_setup, lv_palette_main(LV_PALETTE_NONE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_setup, LV_OPA_COVER, LV_PART_MAIN);

    // 2. Create a main container
    lv_obj_t * fuel_gui_cont = lv_obj_create(screen_setup);
    lv_obj_set_size(fuel_gui_cont, 480, 320);
    lv_obj_center(fuel_gui_cont);
    lv_obj_set_flex_flow(fuel_gui_cont, LV_FLEX_FLOW_ROW_WRAP); // Layout widgets
    lv_obj_set_flex_align(fuel_gui_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 3. Create Fuel Rollers (0 to 12 gallons)
    lv_obj_t * label_l = lv_label_create(fuel_gui_cont);
    lv_label_set_text(label_l, "Fuel Left:");
    
    roller_left = lv_roller_create(fuel_gui_cont);
    lv_roller_set_options(roller_left, fuel_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(roller_left, left_val, LV_ANIM_OFF);
    lv_roller_set_visible_row_count(roller_left, 3);

    lv_obj_t * label_r = lv_label_create(fuel_gui_cont);
    lv_label_set_text(label_r, "Fuel Right:");

    roller_right = lv_roller_create(fuel_gui_cont);
    lv_roller_set_options(roller_right, fuel_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(roller_right, right_val, LV_ANIM_OFF);
    lv_roller_set_visible_row_count(roller_right, 3);

    // 4. Create "Full Fuel" Button
    lv_obj_t * btn_full = lv_btn_create(fuel_gui_cont);
    lv_obj_t * label_full = lv_label_create(btn_full);
    lv_label_set_text(label_full, "Full Fuel");
    lv_obj_add_event_cb(btn_full, full_fuel_event_cb, LV_EVENT_CLICKED, NULL);

    // 5. Create "Update" Button
    lv_obj_t * btn_update = lv_btn_create(fuel_gui_cont);
    lv_obj_t * label_update = lv_label_create(btn_update);
    lv_label_set_text(label_update, "Update");
    lv_obj_add_event_cb(btn_update, update_event_cb, LV_EVENT_CLICKED, NULL);
}

int32_t smooth_fuel_readings_L (int32_t read_fuel_value_L ) {
    total_L = total_L - readings_L[readIndex_L];
    readings_L[readIndex_L] = read_fuel_value_L;
    total_L = total_L + readings_L[readIndex_L];
    readIndex_L = readIndex_L + 1;
    if (readIndex_L >= numReadings_L) readIndex_L = 0;
    averageFuel_L = total_L / numReadings_L;
    return(averageFuel_L);
}

int32_t smooth_fuel_readings_R (int32_t read_fuel_value_R ) {
    total_R = total_R - readings_R[readIndex_R];
    readings_R[readIndex_R] = read_fuel_value_R;
    total_R = total_R + readings_R[readIndex_R];
    readIndex_R = readIndex_R + 1;
    if (readIndex_R >= numReadings_R) readIndex_R = 0;
    averageFuel_R = total_R / numReadings_R;
    return(averageFuel_R);
}

static void fuel_sensors_timer_cb(lv_timer_t * timer1)
{
    LV_UNUSED(timer1);
    // read ttl to serial sensors Left tank
    if (mySerial_L.available() >= 2) {
        L_counter++;
        memset(buffer_L, 0, sizeof(buffer_L));
        // Read the line until newline character or buffer filled
        bytesRead_L = mySerial_L.readBytesUntil('\n', buffer_L, sizeof(buffer_L));
        if (bytesRead_L >= 2) {
            // (Little Endian)
            Fuel_L_value = (buffer_L[0] | buffer_L[1] << 8);

            if(Fuel_L_value < Full_fuel_L_capacitance_value) Fuel_L_value = Full_fuel_L_capacitance_value;
            if(Fuel_L_value > Empty_fuel_L_capacitance_value) Fuel_L_value = Empty_fuel_L_capacitance_value;
            Serial.print("Left counter: ");
            Serial.print(L_counter);
            Serial.print(" bytesRead: ");
            Serial.print(bytesRead_L);
            Serial.print(" buffer: ");
            int i = 0;
            // Loop as long as the current character is not 0 AND we are within array bounds
            while (i < 10 && buffer_L[i] != 0) {
                Serial.print(buffer_L[i], HEX); // Print the character
                Serial.print(" ");
                i++;
            }
            Serial.print(" Raw Bytes: ");
            Serial.print(buffer_L[0], HEX);
            Serial.print(" ");
            Serial.print(buffer_L[1], HEX);
            Serial.print(" Decoded 16-bit value: ");
            Serial.print(Fuel_L_value);

            averageFuel_L = smooth_fuel_readings_L(Fuel_L_value);
            Serial.printf("  Raw smoothed sensor L value: %" PRId32, averageFuel_L);
            // Greater than or equal to 1475 is considered empty, so set to 0% 
            if (averageFuel_L >= 1475) Fuel_L_value = 0;
            if (averageFuel_L < 1457) Fuel_L_value = 2; 
            if (averageFuel_L < 1437) Fuel_L_value = 4; 
            if (averageFuel_L < 1418) Fuel_L_value = 6; 
            if (averageFuel_L < 1400) Fuel_L_value = 8; 
            if (averageFuel_L < 1383) Fuel_L_value = 10; 
            if (averageFuel_L < 1366) Fuel_L_value = 12; 
            if (averageFuel_L < 1349) Fuel_L_value = 14; 
            if (averageFuel_L < 1332) Fuel_L_value = 16; 
            if (averageFuel_L < 1318) Fuel_L_value = 18; 
            if (averageFuel_L < 1304) Fuel_L_value = 20; 
            if (averageFuel_L < 1290) Fuel_L_value = 22; 
            if (averageFuel_L < 1276) Fuel_L_value = 24; 
            if (averageFuel_L < 1266) Fuel_L_value = 26; 
            if (averageFuel_L < 1257) Fuel_L_value = 28; 
            if (averageFuel_L < 1248) Fuel_L_value = 30; 
            if (averageFuel_L < 1239) Fuel_L_value = 33; 
            if (averageFuel_L < 1236) Fuel_L_value = 35; 
            if (averageFuel_L < 1232) Fuel_L_value = 37; 
            if (averageFuel_L < 1228) Fuel_L_value = 39; 
            if (averageFuel_L < 1225) Fuel_L_value = 42; 
            if (averageFuel_L < 1219) Fuel_L_value = 44; 
            if (averageFuel_L < 1213) Fuel_L_value = 46; 
            if (averageFuel_L < 1208) Fuel_L_value = 48; 
            if (averageFuel_L < 1202) Fuel_L_value = 50; 
            if (averageFuel_L < 1187) Fuel_L_value = 52;
            if (averageFuel_L < 1172) Fuel_L_value = 54; 
            if (averageFuel_L < 1157) Fuel_L_value = 56; 
            if (averageFuel_L < 1142) Fuel_L_value = 58; 
            if (averageFuel_L < 1134) Fuel_L_value = 60; 
            if (averageFuel_L < 1127) Fuel_L_value = 62; 
            if (averageFuel_L < 1119) Fuel_L_value = 64; 
            if (averageFuel_L < 1112) Fuel_L_value = 66; 
            if (averageFuel_L < 1102) Fuel_L_value = 68; 
            if (averageFuel_L < 1092) Fuel_L_value = 70; 
            if (averageFuel_L < 1082) Fuel_L_value = 72; 
            if (averageFuel_L < 1072) Fuel_L_value = 74; 
            if (averageFuel_L < 1057) Fuel_L_value = 76; 
            if (averageFuel_L < 1044) Fuel_L_value = 79; 
            if (averageFuel_L < 1030) Fuel_L_value = 81; 
            if (averageFuel_L < 1017) Fuel_L_value = 83; 
            if (averageFuel_L < 1016) Fuel_L_value = 85; 
            if (averageFuel_L < 1015) Fuel_L_value = 87; 
            if (averageFuel_L < 1014) Fuel_L_value = 89; 
            if (averageFuel_L < 1013) Fuel_L_value = 91; 
            if (averageFuel_L < 1012) Fuel_L_value = 93; 
            if (averageFuel_L < 1011) Fuel_L_value = 95; 
            if (averageFuel_L < 1010) Fuel_L_value = 98; 
            if (averageFuel_L < 1000) Fuel_L_value = 100; 
            Serial.printf(" Converted Fuel_L_value: %" PRId32 "\n", Fuel_L_value);

            while (mySerial_L.available() > 0) {
                mySerial_L.read();
            }
        }
    }
    
    // read ttl to serial sensors Right tank
    if (mySerial_R.available() >= 2) {
        R_counter++;
        memset(buffer_R, 0, sizeof(buffer_R));
        // Read the line until newline character or buffer filled
        bytesRead_R = mySerial_R.readBytesUntil('\n', buffer_R, sizeof(buffer_R));
        if (bytesRead_R >= 2) {
            // (Little Endian)
            Fuel_R_value = (buffer_R[0] | buffer_R[1] << 8);

            if(Fuel_R_value < Full_fuel_R_capacitance_value) Fuel_R_value = Full_fuel_R_capacitance_value;
            if(Fuel_R_value > Empty_fuel_R_capacitance_value) Fuel_R_value = Empty_fuel_R_capacitance_value;
            Serial.print("Right counter: ");
            Serial.print(R_counter);
            Serial.print(" bytesRead: ");
            Serial.print(bytesRead_R);
            Serial.print(" buffer: ");
            int i = 0;
            // Loop as long as the current character is not 0 AND we are within array bounds
            while (i < 10 && buffer_R[i] != 0) {
                Serial.print(buffer_R[i], HEX); // Print the character
                Serial.print(" ");
                i++;
            }
            Serial.print(" Raw Bytes: ");
            Serial.print(buffer_R[0], HEX);
            Serial.print(" ");
            Serial.print(buffer_R[1], HEX);
            Serial.print(" Decoded 16-bit Counter: ");
            Serial.print(Fuel_R_value);

            averageFuel_R = smooth_fuel_readings_R(Fuel_R_value);
            Serial.printf("  Raw smoothed sensor R value: %" PRId32, averageFuel_R);
            // Greater than or equal to 1320 is considered empty, so set to 0% 
            if (averageFuel_R >= 1320) Fuel_R_value = 0;
            if (averageFuel_R < 1320) Fuel_R_value = 2; 
            if (averageFuel_R < 1300) Fuel_R_value = 4; 
            if (averageFuel_R < 1290) Fuel_R_value = 6; 
            if (averageFuel_R < 1278) Fuel_R_value = 8; 
            if (averageFuel_R < 1268) Fuel_R_value = 10; 
            if (averageFuel_R < 1258) Fuel_R_value = 12; 
            if (averageFuel_R < 1248) Fuel_R_value = 14; 
            if (averageFuel_R < 1239) Fuel_R_value = 16; 
            if (averageFuel_R < 1229) Fuel_R_value = 18; 
            if (averageFuel_R < 1219) Fuel_R_value = 20; 
            if (averageFuel_R < 1209) Fuel_R_value = 22; 
            if (averageFuel_R < 1198) Fuel_R_value = 24; 
            if (averageFuel_R < 1191) Fuel_R_value = 26; 
            if (averageFuel_R < 1185) Fuel_R_value = 28; 
            if (averageFuel_R < 1178) Fuel_R_value = 30;
            if (averageFuel_R < 1172) Fuel_R_value = 33; 
            if (averageFuel_R < 1163) Fuel_R_value = 35; 
            if (averageFuel_R < 1154) Fuel_R_value = 37; 
            if (averageFuel_R < 1145) Fuel_R_value = 39; 
            if (averageFuel_R < 1137) Fuel_R_value = 42; 
            if (averageFuel_R < 1130) Fuel_R_value = 44; 
            if (averageFuel_R < 1122) Fuel_R_value = 46; 
            if (averageFuel_R < 1115) Fuel_R_value = 48; 
            if (averageFuel_R < 1107) Fuel_R_value = 50; 
            if (averageFuel_R < 1099) Fuel_R_value = 52; 
            if (averageFuel_R < 1090) Fuel_R_value = 54; 
            if (averageFuel_R < 1082) Fuel_R_value = 56; 
            if (averageFuel_R < 1073) Fuel_R_value = 58; 
            if (averageFuel_R < 1064) Fuel_R_value = 60; 
            if (averageFuel_R < 1056) Fuel_R_value = 62; 
            if (averageFuel_R < 1048) Fuel_R_value = 64; 
            if (averageFuel_R < 1040) Fuel_R_value = 66; 
            if (averageFuel_R < 1031) Fuel_R_value = 68; 
            if (averageFuel_R < 1022) Fuel_R_value = 70; 
            if (averageFuel_R < 1013) Fuel_R_value = 72; 
            if (averageFuel_R < 1005) Fuel_R_value = 74; 
            if (averageFuel_R < 995) Fuel_R_value = 76;
            if (averageFuel_R < 985) Fuel_R_value = 79; 
            if (averageFuel_R < 975) Fuel_R_value = 81; 
            if (averageFuel_R < 965) Fuel_R_value = 83; 
            if (averageFuel_R < 952) Fuel_R_value = 85; 
            if (averageFuel_R < 943) Fuel_R_value = 87; 
            if (averageFuel_R < 932) Fuel_R_value = 89; 
            if (averageFuel_R < 921) Fuel_R_value = 91;
            if (averageFuel_R < 916) Fuel_R_value = 93; 
            if (averageFuel_R < 911) Fuel_R_value = 95; 
            if (averageFuel_R < 905) Fuel_R_value = 97; 
            if (averageFuel_R < 900) Fuel_R_value = 100; 
            Serial.printf(" Converted Fuel_R_value: %" PRId32 "\n", Fuel_R_value);

            while (mySerial_R.available() > 0) {
                mySerial_R.read();
            }
        }
    }

/*
    //// Old code for resistive fuel senders, using the ADS7830 ADC directly, without the microcontroller in the fuel tank
    //Fuel_L_value = smooth_fuel_readings_L(ad7830.readADCsingle(0));
    //Serial.printf("Raw smoothed sensor L value: %" PRId32 "\n", Fuel_L_value);
    //if(Fuel_L_value < Full_fuel_capacitance_value) Fuel_L_value = Full_fuel_capacitance_value;
    //if(Fuel_L_value > Empty_fuel_capacitance_value) Fuel_L_value = Empty_fuel_capacitance_value;
    //Fuel_L_value = (int32_t)round((Fuel_L_value-13)*1.851);  // normalize the range 0-100
    //Fuel_L_value = 100 - Fuel_L_value;  // Reverse the range
    //Serial.printf("Fuel_L_value: %" PRId32 "\n", Fuel_L_value);

    //Fuel_R_value = smooth_fuel_readings_R(ad7830.readADCsingle(1));
    //Serial.printf("Raw smoothed sensor R value: %" PRId32 "\n", Fuel_R_value);
    //if(Fuel_R_value < Full_fuel_capacitance_value) Fuel_R_value = Full_fuel_capacitance_value;
    //if(Fuel_R_value > Empty_fuel_capacitance_value) Fuel_R_value = Empty_fuel_capacitance_value;
    //Fuel_R_value = (int32_t)round((Fuel_R_value-13)*1.851);  // normalize the range 0-100
    //Fuel_R_value = 100 - Fuel_R_value;  // Reverse the range
    //Serial.printf("Fuel_R_value: %" PRId32 "\n", Fuel_R_value);
*/
}

static void trim_flap_sensors_timer_cb(lv_timer_t * timer2)
{
    LV_UNUSED(timer2);
    // read sensors
    Flaps_position_value = ad7830.readADCsingle(0);
    if(Flaps_position_value < Full_flaps_resistance_value) Flaps_position_value = Full_flaps_resistance_value;
    if(Flaps_position_value > No_flaps_resistance_value) Flaps_position_value = No_flaps_resistance_value;
    Flaps_position_value = (int32_t)round((Flaps_position_value)*0.784);  // normalize the range 0-20
    Serial.printf("Flaps_position_value: %" PRId32 "\n", Flaps_position_value);

    ailer_trim_value = ad7830.readADCsingle(1);
    if(ailer_trim_value > Up_ailer_resistance_value) ailer_trim_value = Up_ailer_resistance_value;
    if(ailer_trim_value < Down_ailer_resistance_value) ailer_trim_value = Down_ailer_resistance_value;
    ailer_trim_value = (int32_t)round((ailer_trim_value)*0.392);  // normalize the range 0-100
    Serial.printf("ailer_trim_value: %" PRId32 "\n", ailer_trim_value);

    elev_trim_value = ad7830.readADCsingle(2);
    if(elev_trim_value > Up_elev_resistance_value) elev_trim_value = Up_elev_resistance_value;
    if(elev_trim_value < Down_elev_resistance_value) elev_trim_value = Down_elev_resistance_value;
    elev_trim_value = (int32_t)round((elev_trim_value)*0.392);  // normalize the range 0-100
    Serial.printf("elev_trim_value: %" PRId32 "\n", elev_trim_value);

}

// ISR to count pulses
void IRAM_ATTR pulse_isr() {
    pulse_count++;
}

void flow_sensor_timer_cb(lv_timer_t * timer3) {

    LV_UNUSED(timer3);

    // 1. Calculate pulses since last check
    uint32_t current_pulses = pulse_count;
    uint32_t pulses_in_interval = current_pulses - last_pulse_count;
    last_pulse_count = current_pulses;

    // 2. Calculate Flow Rate (GPH)
    // Formula: (pulses_per_sec / pulses_per_gal) * 3600
    current_gph = ( (float)pulses_in_interval / K_FACTOR_GALLONS ) * 3600.0;

    // 3. Update Total Gallons
    total_gallons += (float)current_pulses / K_FACTOR_GALLONS;

    flow_value= current_gph;

    Serial.print("Flow Rate: ");
    Serial.print(current_gph);
    Serial.print(" GPH, Pulses in Interval: ");
    Serial.print(pulses_in_interval);
    Serial.print("\tTotal: ");
    Serial.print(total_gallons);
    Serial.println(" Gallons");
}


void setup() {
    delay(5000);
    #ifdef ARDUINO_USB_CDC_ON_BOOT
    delay(2000);
    #endif

    Serial.begin(115200);

    // Left and Right fuel tank Configure 1200 baud, 8N1 
    Serial.println("Start receiving TTL to serial feeds\n");
    mySerial_L.begin(1200); 
    mySerial_R.begin(1200); 

    // Initialize ADS7830 on pins 17 and 18
    Wire1.begin(ADS_SDA, ADS_SCL, 100000); // 100kHz
    Serial.println("Adafruit ADS7830 start\n");
    if (!ad7830.begin(0x48, &Wire1)) { // Pass the custom bus to the library
        Serial.println("Failed to initialize ADS7830!\n");
        while(1);   
    }

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

    // Create a screen and background color 
    screen_gauges = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_gauges, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_gauges, LV_OPA_COVER, LV_PART_MAIN);

    // Button on Screen 2 to go back to Screen 1
    lv_obj_t * btn2 = lv_button_create(screen_gauges);
    lv_obj_align(btn2, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_t * label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "Setup");
    lv_obj_add_event_cb(btn2, switch_screen_to_setup_event_cb, LV_EVENT_CLICKED, screen_gauges);



    pinMode(sensorPin, INPUT);
    // Trigger pulseCounter function on rising edge
    attachInterrupt(digitalPinToInterrupt(sensorPin), pulse_isr, FALLING);

    // Initialize all Fuel readings to 0
    for (int i = 0; i < numReadings_L; i++) readings_L[i] = 0;
    for (int i = 0; i < numReadings_R; i++) readings_R[i] = 0;

    // Update display Fuel left gauge every 200ms
    fuel_gaugeL(200);
    // Update display Fuel right gauge every 200ms
    fuel_gaugeR(200);
    // Update display Flaps gauge every 250ms
    flaps_gauge(250);
    // Update display Trim every 250ms
    trim_gauge(250);
    // Update display Flow gauge every 250ms
    flow_gauge(250);

    setup_fuel_gui();

    lv_screen_load(screen_gauges);


}

void loop() {

    if(startup==true) {
        delay(200);
        if (lv_tick_get() - last_run >= interval) {
            last_run += interval; // Update time for next check
            Serial.println(startup_int);
            Fuel_L_value = startup_int*5; // 0 to 100
            Fuel_R_value = startup_int*5; // 0 to 100
            Flaps_position_value = startup_int; // 0 to 20
            ailer_trim_value = startup_int*5; // 0 to 100
            elev_trim_value = startup_int*5; // 0 to 100
            if(reverse_flag==false && startup_int <= 20) {
                startup_int++;
            } else {
                reverse_flag = true;
                startup_int--;
            }
            if (startup_int < 0) {
                startup = false;
            }
        }
    }   
    if (first_time==true && startup==false) {
        first_time=false;
        lv_timer_create(fuel_sensors_timer_cb, 100, NULL);
        lv_timer_create(trim_flap_sensors_timer_cb, 100, NULL);
        lv_timer_create(flow_sensor_timer_cb, 1000, NULL);
//        lv_screen_load(screen_setup);
    }


    //if(flow_hold_value!=flow_value) {
    //    flow_hold_value=flow_value;
    //    Serial.printf("Flow value is: %d\n", flow_value);
    //}

    // Automatically calls lv_timer_handler() every 5ms
    lv_timer_handler_run_in_period(5);
    gfx->flush();

}
