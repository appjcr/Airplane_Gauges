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

Preferences prefs;
lv_obj_t *roller_left, *roller_right;
const char* fuel_options = "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12";
const int MAX_FUEL = 12;
int left_val = 0;
int right_val = 0;

bool first_time = true;
bool startup = true;
bool reverse_flag = false;
int16_t startup_int = 0;
static uint32_t last_run = 0;
const uint32_t interval = 200;

#define ADS_SDA 17
#define ADS_SCL 18

SoftwareSerial mySerial_L(5, -1);
SoftwareSerial mySerial_R(6, -1);

const int sensorPin = 7;
const float PULSES_PER_LITER = 4380.0;
const float LITERS_PER_GALLON = 3.78541;
const float K_FACTOR_GALLONS = PULSES_PER_LITER * LITERS_PER_GALLON;
volatile uint32_t pulse_count = 0;
uint32_t last_pulse_count = 0;
float total_gallons = 0.0;
float current_gph = 0.0;

int32_t L_counter = 0;
int32_t bytesRead_L = 0;
uint8_t buffer_L[10];
int32_t R_counter = 0;
int32_t bytesRead_R = 0;
uint8_t buffer_R[10];

int16_t Empty_fuel_L_capacitance_value = 1590;
int16_t Full_fuel_L_capacitance_value  = 805;
int16_t Empty_fuel_R_capacitance_value = 1590;
int16_t Full_fuel_R_capacitance_value  = 805;
int16_t No_flaps_resistance_value      = 0;
int16_t Full_flaps_resistance_value    = 255;
int16_t Down_elev_resistance_value     = 0;
int16_t Up_elev_resistance_value       = 255;
int16_t Down_ailer_resistance_value    = 0;
int16_t Up_ailer_resistance_value      = 255;

// --- Rolling-average smoother ---
struct SmoothBuffer {
    static const int SIZE = 50;
    int readings[SIZE];
    int readIndex;
    long total;
    SmoothBuffer() : readIndex(0), total(0) { memset(readings, 0, sizeof(readings)); }
};

static int32_t smooth_reading(SmoothBuffer &buf, int32_t new_val) {
    buf.total -= buf.readings[buf.readIndex];
    buf.readings[buf.readIndex] = new_val;
    buf.total += new_val;
    buf.readIndex = (buf.readIndex + 1) % SmoothBuffer::SIZE;
    return buf.total / SmoothBuffer::SIZE;
}

static SmoothBuffer smooth_L;
static SmoothBuffer smooth_R;

// --- Capacitance-to-percent lookup tables (ascending threshold, early-exit) ---
struct CapEntry { int16_t threshold; int8_t pct; };

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const CapEntry fuel_L_table[] = {
    {1000,100},{1010, 98},{1011, 95},{1012, 93},{1013, 91},{1014, 89},{1015, 87},{1016, 85},
    {1017, 83},{1030, 81},{1044, 79},{1057, 76},{1072, 74},{1082, 72},{1092, 70},{1102, 68},
    {1112, 66},{1119, 64},{1127, 62},{1134, 60},{1142, 58},{1157, 56},{1172, 54},{1187, 52},
    {1202, 50},{1208, 48},{1213, 46},{1219, 44},{1225, 42},{1228, 39},{1232, 37},{1236, 35},
    {1239, 33},{1248, 30},{1257, 28},{1266, 26},{1276, 24},{1290, 22},{1304, 20},{1318, 18},
    {1332, 16},{1349, 14},{1366, 12},{1383, 10},{1400,  8},{1418,  6},{1437,  4},{1457,  2},
};

static const CapEntry fuel_R_table[] = {
    { 900,100},{ 905, 97},{ 911, 95},{ 916, 93},{ 921, 91},{ 932, 89},{ 943, 87},{ 952, 85},
    { 965, 83},{ 975, 81},{ 985, 79},{ 995, 76},{1005, 74},{1013, 72},{1022, 70},{1031, 68},
    {1040, 66},{1048, 64},{1056, 62},{1064, 60},{1073, 58},{1082, 56},{1090, 54},{1099, 52},
    {1107, 50},{1115, 48},{1122, 46},{1130, 44},{1137, 42},{1145, 39},{1154, 37},{1163, 35},
    {1172, 33},{1178, 30},{1185, 28},{1191, 26},{1198, 24},{1209, 22},{1219, 20},{1229, 18},
    {1239, 16},{1248, 14},{1258, 12},{1268, 10},{1278,  8},{1290,  6},{1300,  4},{1320,  2},
};

static int32_t cap_to_pct(int32_t avg, int16_t empty_threshold,
                           const CapEntry *table, int count) {
    if (avg >= empty_threshold) return 0;
    for (int i = 0; i < count; i++) {
        if (avg < table[i].threshold) return table[i].pct;
    }
    return 0;
}

Arduino_DataBus *bus = new Arduino_ESP32QSPI(TFT_CS, TFT_SCK, TFT_SDA0, TFT_SDA1, TFT_SDA2, TFT_SDA3);
Arduino_GFX *g = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED, 0, false, TFT_res_W, TFT_res_H);
Arduino_Canvas *gfx = new Arduino_Canvas(TFT_res_W, TFT_res_H, g, 0, 0, TFT_rot);
AXS15231B_Touch touch(Touch_SCL, Touch_SDA, Touch_INT, Touch_ADDR, TFT_rot);

#if LV_USE_LOG != 0
void my_print(lv_log_level_t level, const char *buf) {
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

uint32_t millis_cb(void) {
    return millis();
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t x, y;
    if (touch.touched()) {
        touch.readData(&x, &y);
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void get_fuel() {
    prefs.begin("fuel_data", true);
    left_val = prefs.getInt("left", 0);
    right_val = prefs.getInt("right", 0);
    prefs.end();
}

static void save_fuel() {
    prefs.begin("fuel_data", false);
    prefs.putInt("left", left_val);
    prefs.putInt("right", right_val);
    prefs.end();
    Serial.printf("Saved: Left %d, Right %d\n", left_val, right_val);
}

static void full_fuel_event_cb(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_roller_set_selected(roller_left, MAX_FUEL, LV_ANIM_ON);
        lv_roller_set_selected(roller_right, MAX_FUEL, LV_ANIM_ON);
        left_val = MAX_FUEL;
        right_val = MAX_FUEL;
        save_fuel();
        lv_screen_load(screen_gauges);
    }
}

static void update_event_cb(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        left_val = lv_roller_get_selected(roller_left);
        right_val = lv_roller_get_selected(roller_right);
        save_fuel();
        lv_screen_load(screen_gauges);
    }
}

static void switch_screen_to_setup_event_cb(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        get_fuel();
        lv_roller_set_selected(roller_left, left_val, LV_ANIM_OFF);
        lv_roller_set_selected(roller_right, right_val, LV_ANIM_OFF);
        lv_screen_load(screen_setup);
    }
}

void setup_fuel_gui() {
    get_fuel();

    screen_setup = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_setup, lv_palette_main(LV_PALETTE_NONE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_setup, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t * fuel_gui_cont = lv_obj_create(screen_setup);
    lv_obj_set_size(fuel_gui_cont, 480, 320);
    lv_obj_center(fuel_gui_cont);
    lv_obj_set_flex_flow(fuel_gui_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(fuel_gui_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

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

    lv_obj_t * btn_full = lv_btn_create(fuel_gui_cont);
    lv_obj_t * label_full = lv_label_create(btn_full);
    lv_label_set_text(label_full, "Full Fuel");
    lv_obj_add_event_cb(btn_full, full_fuel_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_update = lv_btn_create(fuel_gui_cont);
    lv_obj_t * label_update = lv_label_create(btn_update);
    lv_label_set_text(label_update, "Update");
    lv_obj_add_event_cb(btn_update, update_event_cb, LV_EVENT_CLICKED, NULL);
}

static void read_tank_sensor(SoftwareSerial &serial, uint8_t *buf,
                              int32_t &counter, int32_t &bytes_read,
                              int32_t &fuel_value, SmoothBuffer &smoother,
                              int16_t full_cap, int16_t empty_cap,
                              int16_t empty_threshold,
                              const CapEntry *table, int table_count,
                              const char *side) {
    if (serial.available() < 2) return;
    counter++;
    memset(buf, 0, 10);
    bytes_read = serial.readBytesUntil('\n', buf, 10);
    if (bytes_read < 2) return;

    fuel_value = (buf[0] | buf[1] << 8);
    if (fuel_value < full_cap)  fuel_value = full_cap;
    if (fuel_value > empty_cap) fuel_value = empty_cap;

    Serial.printf("%s counter: %" PRId32 " bytesRead: %" PRId32 " buffer: ",
                  side, counter, bytes_read);
    for (int i = 0; i < 10 && buf[i] != 0; i++) {
        Serial.print(buf[i], HEX); Serial.print(" ");
    }
    Serial.printf(" Raw Bytes: %02X %02X Decoded: %" PRId32, buf[0], buf[1], fuel_value);

    int32_t avg = smooth_reading(smoother, fuel_value);
    Serial.printf("  Raw smoothed %s: %" PRId32, side, avg);

    fuel_value = cap_to_pct(avg, empty_threshold, table, table_count);
    Serial.printf(" Converted: %" PRId32 "\n", fuel_value);

    while (serial.available() > 0) serial.read();
}

static void fuel_sensors_timer_cb(lv_timer_t * timer1) {
    LV_UNUSED(timer1);
    read_tank_sensor(mySerial_L, buffer_L, L_counter, bytesRead_L, Fuel_L_value, smooth_L,
                     Full_fuel_L_capacitance_value, Empty_fuel_L_capacitance_value, 1475,
                     fuel_L_table, ARRAY_SIZE(fuel_L_table), "Left");
    read_tank_sensor(mySerial_R, buffer_R, R_counter, bytesRead_R, Fuel_R_value, smooth_R,
                     Full_fuel_R_capacitance_value, Empty_fuel_R_capacitance_value, 1320,
                     fuel_R_table, ARRAY_SIZE(fuel_R_table), "Right");
}

static int32_t read_clamp_adc(int channel, int16_t lo, int16_t hi, float scale) {
    int32_t val = ad7830.readADCsingle(channel);
    if (val < lo) val = lo;
    if (val > hi) val = hi;
    return (int32_t)round(val * scale);
}

static void trim_flap_sensors_timer_cb(lv_timer_t * timer2) {
    LV_UNUSED(timer2);

    Flaps_position_value = ad7830.readADCsingle(0);
    if (Flaps_position_value < Full_flaps_resistance_value) Flaps_position_value = Full_flaps_resistance_value;
    if (Flaps_position_value > No_flaps_resistance_value)   Flaps_position_value = No_flaps_resistance_value;
    Flaps_position_value = (int32_t)round(Flaps_position_value * 0.784);
    Serial.printf("Flaps_position_value: %" PRId32 "\n", Flaps_position_value);

    ailer_trim_value = read_clamp_adc(1, Down_ailer_resistance_value, Up_ailer_resistance_value, 0.392);
    Serial.printf("ailer_trim_value: %" PRId32 "\n", ailer_trim_value);

    elev_trim_value = read_clamp_adc(2, Down_elev_resistance_value, Up_elev_resistance_value, 0.392);
    Serial.printf("elev_trim_value: %" PRId32 "\n", elev_trim_value);
}

void IRAM_ATTR pulse_isr() {
    pulse_count++;
}

void flow_sensor_timer_cb(lv_timer_t * timer3) {
    LV_UNUSED(timer3);

    uint32_t current_pulses = pulse_count;
    uint32_t pulses_in_interval = current_pulses - last_pulse_count;
    last_pulse_count = current_pulses;

    current_gph = ((float)pulses_in_interval / K_FACTOR_GALLONS) * 3600.0;
    total_gallons += (float)pulses_in_interval / K_FACTOR_GALLONS;

    flow_value = current_gph;

    Serial.print("Flow Rate: "); Serial.print(current_gph);
    Serial.print(" GPH, Pulses in Interval: "); Serial.print(pulses_in_interval);
    Serial.print("\tTotal: "); Serial.print(total_gallons); Serial.println(" Gallons");
}

void setup() {
    delay(5000);
#ifdef ARDUINO_USB_CDC_ON_BOOT
    delay(2000);
#endif

    Serial.begin(115200);
    Serial.println("Start receiving TTL to serial feeds\n");
    mySerial_L.begin(1200);
    mySerial_R.begin(1200);

    Wire1.begin(ADS_SDA, ADS_SCL, 100000);
    Serial.println("Adafruit ADS7830 start\n");
    if (!ad7830.begin(0x48, &Wire1)) {
        Serial.println("Failed to initialize ADS7830!\n");
        while (1);
    }

    Serial.println("Arduino_GFX LVGL ");
    String LVGL_Arduino = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch() + " example";
    Serial.println(LVGL_Arduino);

    if (!gfx->begin(40000000UL)) {
        Serial.println("Failed to initialize display!");
        return;
    }
    gfx->fillScreen(BLACK);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    if (!touch.begin()) {
        Serial.println("Failed to initialize touch module!");
        return;
    }
    touch.enOffsetCorrection(true);
    touch.setOffsets(Touch_X_min, Touch_X_max, TFT_res_W-1, Touch_Y_min, Touch_Y_max, TFT_res_H-1);

    lv_init();
    lv_tick_set_cb(millis_cb);

#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print);
#endif

    uint32_t screenWidth  = gfx->width();
    uint32_t screenHeight = gfx->height();
    uint32_t bufSize = screenWidth * screenHeight / 10;
    lv_color_t *disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) {
        Serial.println("LVGL failed to allocate display buffer!");
        return;
    }

    lv_display_t *disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, disp_draw_buf, NULL, bufSize * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    screen_gauges = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_gauges, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen_gauges, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t * btn2 = lv_button_create(screen_gauges);
    lv_obj_align(btn2, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_t * label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "Setup");
    lv_obj_add_event_cb(btn2, switch_screen_to_setup_event_cb, LV_EVENT_CLICKED, screen_gauges);

    pinMode(sensorPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(sensorPin), pulse_isr, FALLING);

    fuel_gaugeL(200);
    fuel_gaugeR(200);
    flaps_gauge(250);
    trim_gauge(250);
    flow_gauge(250);

    setup_fuel_gui();
    lv_screen_load(screen_gauges);
}

void loop() {
    if (startup) {
        delay(200);
        if (lv_tick_get() - last_run >= interval) {
            last_run += interval;
            Serial.println(startup_int);
            Fuel_L_value         = startup_int * 5;
            Fuel_R_value         = startup_int * 5;
            Flaps_position_value = startup_int;
            ailer_trim_value     = startup_int * 5;
            elev_trim_value      = startup_int * 5;
            if (!reverse_flag && startup_int <= 20) {
                startup_int++;
            } else {
                reverse_flag = true;
                startup_int--;
            }
            if (startup_int < 0) startup = false;
        }
    }

    if (first_time && !startup) {
        first_time = false;
        lv_timer_create(fuel_sensors_timer_cb,       100,  NULL);
        lv_timer_create(trim_flap_sensors_timer_cb,  100,  NULL);
        lv_timer_create(flow_sensor_timer_cb,        1000, NULL);
    }

    lv_timer_handler_run_in_period(5);
    gfx->flush();
}
