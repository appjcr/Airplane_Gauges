#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_ADS7830.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "hardware_config.h"
#include "sensors.h"
#include "sensor_utils.h"
#include "ui_utils.h"
#include "app_state.h"
#include "AXS15231B_touch.h"

#include "fuel_gauge.h"
#include "flaps_gauge.h"
#include "trim_gauge.h"
#include "flow_gauge.h"
#include "fuel_setup.h"

// Global instances
Adafruit_ADS7830 ad7830;
SoftwareSerial serial_fuel_left(FuelSensors::PIN_LEFT, -1);
SoftwareSerial serial_fuel_right(FuelSensors::PIN_RIGHT, -1);
AXS15231B_Touch touch(Touch::SCL, Touch::SDA, Touch::INT, Touch::ADDR, Display::ROTATION);
Arduino_DataBus *bus = new Arduino_ESP32QSPI(SPII::CS, SPII::SCK, SPII::SDA0, SPII::SDA1, SPII::SDA2, SPII::SDA3);
Arduino_GFX *g = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED, 0, false, Display::WIDTH, Display::HEIGHT);
Arduino_Canvas *gfx = new Arduino_Canvas(Display::WIDTH, Display::HEIGHT, g, 0, 0, Display::ROTATION);

// Fuel lookup tables
static const CapacityEntry fuel_L_table[] = {
    {1000, 100}, {1010, 98},  {1011, 95},  {1012, 93},  {1013, 91},  {1014, 89},  {1015, 87},  {1016, 85},
    {1017, 83},  {1030, 81},  {1044, 79},  {1057, 76},  {1072, 74},  {1082, 72},  {1092, 70},  {1102, 68},
    {1112, 66},  {1119, 64},  {1127, 62},  {1134, 60},  {1142, 58},  {1157, 56},  {1172, 54},  {1187, 52},
    {1202, 50},  {1208, 48},  {1213, 46},  {1219, 44},  {1225, 42},  {1228, 39},  {1232, 37},  {1236, 35},
    {1239, 33},  {1248, 30},  {1257, 28},  {1266, 26},  {1276, 24},  {1290, 22},  {1304, 20},  {1318, 18},
    {1332, 16},  {1349, 14},  {1366, 12},  {1383, 10},  {1400, 8},   {1418, 6},   {1437, 4},   {1457, 2},
};

static const CapacityEntry fuel_R_table[] = {
    {900, 100},  {905, 97},   {911, 95},   {916, 93},   {921, 91},   {932, 89},   {943, 87},   {952, 85},
    {965, 83},   {975, 81},   {985, 79},   {995, 76},   {1005, 74},  {1013, 72},  {1022, 70},  {1031, 68},
    {1040, 66},  {1048, 64},  {1056, 62},  {1064, 60},  {1073, 58},  {1082, 56},  {1090, 54},  {1099, 52},
    {1107, 50},  {1115, 48},  {1122, 46},  {1130, 44},  {1137, 42},  {1145, 39},  {1154, 37},  {1163, 35},
    {1172, 33},  {1178, 30},  {1185, 28},  {1191, 26},  {1198, 24},  {1209, 22},  {1219, 20},  {1229, 18},
    {1239, 16},  {1248, 14},  {1258, 12},  {1268, 10},  {1278, 8},   {1290, 6},   {1300, 4},   {1320, 2},
};

static constexpr int FUEL_L_TABLE_SIZE = sizeof(fuel_L_table) / sizeof(fuel_L_table[0]);
static constexpr int FUEL_R_TABLE_SIZE = sizeof(fuel_R_table) / sizeof(fuel_R_table[0]);

#if LV_USE_LOG != 0
static void lvgl_log(lv_log_level_t level, const char *buf) {
    (void)level;
    Serial.println(buf);
    Serial.flush();
}
#endif

static uint32_t millis_cb(void) {
    return millis();
}

static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
    lv_disp_flush_ready(disp);
}

static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
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

static void read_tank_sensor(SoftwareSerial &serial,
                            SerialBuffer &buffer,
                            int32_t &fuel_value,
                            SmoothingBuffer *smoother,
                            int16_t full_cap,
                            int16_t empty_cap,
                            int16_t empty_threshold,
                            const CapacityEntry *table,
                            int table_count,
                            const char *side) {
    if (serial.available() < 2) return;

    buffer.counter++;
    memset(buffer.data, 0, 10);
    buffer.bytes_read = serial.readBytesUntil('\n', buffer.data, 10);
    if (buffer.bytes_read < 2) return;

    fuel_value = (buffer.data[0] | (buffer.data[1] << 8));
    if (fuel_value < full_cap) fuel_value = full_cap;
    if (fuel_value > empty_cap) fuel_value = empty_cap;

    Serial.printf("%s counter: %" PRId32 " bytesRead: %" PRId32 " buffer: ", side, buffer.counter, buffer.bytes_read);
    for (int i = 0; i < 10 && buffer.data[i] != 0; i++) {
        Serial.print(buffer.data[i], HEX);
        Serial.print(" ");
    }
    Serial.printf(" Raw Bytes: %02X %02X Decoded: %" PRId32, buffer.data[0], buffer.data[1], fuel_value);

    int32_t avg = smoother->add_reading(fuel_value);
    Serial.printf("  Raw smoothed %s: %" PRId32, side, avg);

    fuel_value = SensorUtils::capacity_to_percentage(avg, empty_threshold, table, table_count);
    Serial.printf(" Converted: %" PRId32 "\n", fuel_value);

    while (serial.available() > 0) serial.read();
}

static void fuel_sensors_timer_cb(lv_timer_t *) {
    AppState &state = AppState::instance();
    read_tank_sensor(serial_fuel_left, state.serial_left, Fuel_L_value, state.fuel.smooth_left,
                    FuelSensors::LEFT_CAP_FULL, FuelSensors::LEFT_CAP_EMPTY, FuelSensors::LEFT_EMPTY_THRESH,
                    fuel_L_table, FUEL_L_TABLE_SIZE, "Left");
    read_tank_sensor(serial_fuel_right, state.serial_right, Fuel_R_value, state.fuel.smooth_right,
                    FuelSensors::RIGHT_CAP_FULL, FuelSensors::RIGHT_CAP_EMPTY, FuelSensors::RIGHT_EMPTY_THRESH,
                    fuel_R_table, FUEL_R_TABLE_SIZE, "Right");
}

static void trim_flap_sensors_timer_cb(lv_timer_t *) {
    AppState &state = AppState::instance();

    Flaps_position_value = ad7830.readADCsingle(ADC::CH_FLAPS);
    Flaps_position_value = SensorUtils::read_and_clamp_adc(Flaps_position_value, ADC::FLAPS_LO, ADC::FLAPS_HI, ADC::FLAPS_SCALE);
    Serial.printf("Flaps_position_value: %" PRId32 "\n", Flaps_position_value);

    ailer_trim_value = SensorUtils::read_and_clamp_adc(ad7830.readADCsingle(ADC::CH_AILERON),
                                                       ADC::TRIM_LO, ADC::TRIM_HI, ADC::TRIM_SCALE);
    Serial.printf("ailer_trim_value: %" PRId32 "\n", ailer_trim_value);

    elev_trim_value = SensorUtils::read_and_clamp_adc(ad7830.readADCsingle(ADC::CH_ELEVATOR),
                                                      ADC::TRIM_LO, ADC::TRIM_HI, ADC::TRIM_SCALE);
    Serial.printf("elev_trim_value: %" PRId32 "\n", elev_trim_value);
}

static void IRAM_ATTR pulse_isr() {
    AppState &state = AppState::instance();
    state.flow.pulse_count++;
    state.flow.last_pulse_time_ms = millis();
}

static void flow_sensor_timer_cb(lv_timer_t *) {
    AppState &state = AppState::instance();
    uint32_t current_pulses = state.flow.pulse_count;
    uint32_t pulses_in_interval = current_pulses - state.flow.last_pulse_count;
    state.flow.last_pulse_count = current_pulses;

    float k_factor = FlowSensor::PULSES_PER_LITER * FlowSensor::LITERS_PER_GALLON;
    float raw_gph = ((float)pulses_in_interval / k_factor) * 3600.0f;

    bool pulse_fresh = (millis() - state.flow.last_pulse_time_ms) < FlowSensor::STALE_TIMEOUT_MS;

    if (pulse_fresh && raw_gph >= FlowSensor::MIN_GPH) {
        state.flow.total_gallons_used += (float)pulses_in_interval / k_factor;

        int32_t smoothed_scaled = state.flow.smooth_flow->add_reading((int32_t)(raw_gph * 100.0f));
        state.flow.current_gph = (float)smoothed_scaled / 100.0f;
        flow_value = state.flow.current_gph;

        state.flow.avg_gph_sample_count++;
        avg_gph_value += (state.flow.current_gph - avg_gph_value) / (float)state.flow.avg_gph_sample_count;

        float initial_fuel = (float)(state.fuel.left_user_setting + state.fuel.right_user_setting);
        flow_used_value = state.flow.total_gallons_used;
        remain_value = initial_fuel - flow_used_value;
        if (remain_value < 0.0f) remain_value = 0.0f;

        if (state.flow.current_gph > 0.0f) {
            float hours = remain_value / state.flow.current_gph;
            time_to_empty_hours_value = (int32_t)hours;
            time_to_empty_minutes_value = (int32_t)((hours - (float)time_to_empty_hours_value) * 60.0f);
        }
    } else {
        state.flow.current_gph = 0.0f;
        flow_value = 0.0f;
        time_to_empty_hours_value = 0;
        time_to_empty_minutes_value = 0;
    }

    static uint32_t save_ticks = 0;
    if (++save_ticks >= 60) {
        save_ticks = 0;
        save_flow_totals();
    }

    Serial.printf("Flow: %.2f GPH  Used: %.2f gal  Rem: %.2f gal  TTE: %02d:%02d\n",
                 flow_value, flow_used_value, remain_value,
                 (int)time_to_empty_hours_value, (int)time_to_empty_minutes_value);
}

void setup() {
    delay(Startup::SERIAL_WAIT_MS);
#ifdef ARDUINO_USB_CDC_ON_BOOT
    delay(Startup::USB_CDC_WAIT_MS);
#endif

    Serial.begin(Startup::SERIAL_BAUD);
    Serial.println("Arduino_GFX LVGL ");
    String lvgl_version = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch() + " example";
    Serial.println(lvgl_version);

    if (!gfx->begin(Display::SPI_SPEED)) {
        Serial.println("Failed to initialize display!");
        return;
    }
    gfx->fillScreen(BLACK);

    pinMode(TFT::BL_PIN, OUTPUT);
    digitalWrite(TFT::BL_PIN, HIGH);

    if (!touch.begin()) {
        Serial.println("Failed to initialize touch module!");
        return;
    }
    touch.enOffsetCorrection(true);
    touch.setOffsets(Touch::X_MIN, Touch::X_MAX, Display::WIDTH - 1, Touch::Y_MIN, Touch::Y_MAX, Display::HEIGHT - 1);


    lv_init();
    lv_tick_set_cb(millis_cb);

#if LV_USE_LOG != 0
    lv_log_register_print_cb(lvgl_log);
#endif

    uint32_t screenWidth = gfx->width();
    uint32_t screenHeight = gfx->height();
    uint32_t bufSize = screenWidth * screenHeight / Startup::LVGL_BUFFER_DIVISOR;
    lv_color_t *disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) {
        Serial.println("LVGL failed to allocate display buffer!");
        return;
    }

    lv_display_t *disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, disp_draw_buf, nullptr, bufSize * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    AppState &state = AppState::instance();

    state.ui.screen_gauges = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(state.ui.screen_gauges, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(state.ui.screen_gauges, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *btn_setup = UIUtils::create_button_with_label(state.ui.screen_gauges, "Setup");
    lv_obj_align(btn_setup, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_add_event_cb(btn_setup, switch_to_setup_event_cb, LV_EVENT_CLICKED, state.ui.screen_gauges);

    Serial.println("Start receiving TTL to serial feeds\n");
    serial_fuel_left.begin(FuelSensors::BAUD);
    serial_fuel_right.begin(FuelSensors::BAUD);

    Wire1.begin(ADC::SDA, ADC::SCL, ADC::I2C_FREQ);
    Serial.println("Adafruit ADS7830 start\n");
    if (!ad7830.begin(ADC::I2C_ADDR, &Wire1)) {
        Serial.println("Failed to initialize ADS7830!\n");
        while (1);
    }

    state.fuel.smooth_left = new SmoothingBuffer(FuelSensors::SMOOTH_BUFFER_SIZE);
    state.fuel.smooth_right = new SmoothingBuffer(FuelSensors::SMOOTH_BUFFER_SIZE);
    state.flow.smooth_flow = new SmoothingBuffer(FlowSensor::SMOOTH_BUFFER_SIZE);

    pinMode(FlowSensor::PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(FlowSensor::PIN), pulse_isr, FALLING);

    load_flow_totals();

    fuel_gaugeL(Timers::GAUGE_FUEL_MS);
    fuel_gaugeR(Timers::GAUGE_FUEL_MS);
    flaps_gauge(Timers::GAUGE_FLAPS_MS);
    trim_gauge(Timers::GAUGE_TRIM_MS);
    flow_gauge(Timers::GAUGE_FLOW_MS);
    
    setup_fuel_gui();
    
    lv_screen_load(state.ui.screen_gauges);
}

void loop() {
    AppState &state = AppState::instance();

    if (state.startup.active) {
        delay(200);
        if (lv_tick_get() - state.startup.last_run >= Timers::STARTUP_ANIM_INTERVAL_MS) {
            state.startup.last_run += Timers::STARTUP_ANIM_INTERVAL_MS;
            Serial.println(state.startup.value);
            Fuel_L_value = state.startup.value * 5;
            Fuel_R_value = state.startup.value * 5;
            Flaps_position_value = state.startup.value;
            ailer_trim_value = state.startup.value * 5;
            elev_trim_value = state.startup.value * 5;
            if (!state.startup.reverse && state.startup.value <= 20) {
                state.startup.value++;
            } else {
                state.startup.reverse = true;
                state.startup.value--;
            }
            if (state.startup.value < 0) state.startup.active = false;
        }
    }

    if (state.ui.first_initialization && !state.startup.active) {
        state.ui.first_initialization = false;
        lv_timer_create(fuel_sensors_timer_cb, Timers::FUEL_SENSOR_MS, nullptr);
        lv_timer_create(trim_flap_sensors_timer_cb, Timers::TRIM_FLAP_SENSOR_MS, nullptr);
        lv_timer_create(flow_sensor_timer_cb, Timers::FLOW_SENSOR_MS, nullptr);
    }

    lv_timer_handler_run_in_period(Timers::LVGL_HANDLER_PERIOD_MS);
    gfx->flush();
}
