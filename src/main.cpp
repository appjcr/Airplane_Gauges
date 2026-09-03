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
#include "kfactor_setup.h"

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

// Set whenever anything writes the canvas, cleared when it is pushed to the panel.
// Only ever touched from loop() context (LVGL flush callbacks run inside
// lv_timer_handler), so no interrupt guard is needed.
static bool canvas_dirty = false;

static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
    canvas_dirty = true;
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
    memset(buffer.data, 0, sizeof(buffer.data));
    buffer.bytes_read = serial.readBytesUntil('\n', buffer.data, sizeof(buffer.data));
    if (buffer.bytes_read < 2) {
        // Drop the remainder as well. Leaving a partial frame queued would leave
        // every subsequent read byte-shifted.
        while (serial.available() > 0) serial.read();
        return;
    }

    int32_t raw = (buffer.data[0] | (buffer.data[1] << 8));

    // Landing mid-frame decodes to nonsense. Drop it rather than clamping — a clamp
    // would hand the smoother a rail value as though it were a genuine reading.
    if (raw < FuelSensors::RAW_PLAUSIBLE_MIN || raw > FuelSensors::RAW_PLAUSIBLE_MAX) {
        Serial.printf("%s: implausible frame, raw %" PRId32 " discarded\n", side, raw);
        while (serial.available() > 0) serial.read();
        return;
    }

    fuel_value = raw;
    if (fuel_value < full_cap) fuel_value = full_cap;
    if (fuel_value > empty_cap) fuel_value = empty_cap;

    //Serial.printf("%s counter: %" PRId32 " bytesRead: %" PRId32 " buffer: ", side, buffer.counter, buffer.bytes_read);
    //for (int i = 0; i < 10 && buffer.data[i] != 0; i++) {
    //    Serial.print(buffer.data[i], HEX);
    //    Serial.print(" ");
    //}
    //Serial.printf(" Raw Bytes: %02X %02X Decoded: %" PRId32, buffer.data[0], buffer.data[1], fuel_value);

    int32_t avg = smoother->add_reading(fuel_value);
    //Serial.printf("  Raw smoothed %s: %" PRId32, side, avg);

    fuel_value = SensorUtils::capacity_to_percentage(avg, empty_threshold, table, table_count);
    //Serial.printf(" Converted: %" PRId32 "\n", fuel_value);

    while (serial.available() > 0) serial.read();
}

static void fuel_sensors_timer_cb(lv_timer_t *) {
    AppState &state = AppState::instance();
    read_tank_sensor(serial_fuel_left, state.serial_left, state.fuel.left_percentage, state.fuel.smooth_left,
                    FuelSensors::LEFT_CAP_FULL, FuelSensors::LEFT_CAP_EMPTY, FuelSensors::LEFT_EMPTY_THRESH,
                    fuel_L_table, FUEL_L_TABLE_SIZE, "Left");
    read_tank_sensor(serial_fuel_right, state.serial_right, state.fuel.right_percentage, state.fuel.smooth_right,
                    FuelSensors::RIGHT_CAP_FULL, FuelSensors::RIGHT_CAP_EMPTY, FuelSensors::RIGHT_EMPTY_THRESH,
                    fuel_R_table, FUEL_R_TABLE_SIZE, "Right");
}

// readADCsingle() returns -1 when the I2C transfer fails. Clamping that would park
// every channel on its low rail - a dead bus would indicate full flaps and zeroed
// trim - so a failed read holds the last good value and is logged on state change.
static void trim_flap_sensors_timer_cb(lv_timer_t *) {
    AppState &state = AppState::instance();
    int16_t flaps_raw = ad7830.readADCsingle(ADC::CH_FLAPS);
    int16_t ailer_raw = ad7830.readADCsingle(ADC::CH_AILERON);
    int16_t elev_raw  = ad7830.readADCsingle(ADC::CH_ELEVATOR);

    static bool read_failed = false;
    bool failed_now = (flaps_raw < 0) || (ailer_raw < 0) || (elev_raw < 0);
    if (failed_now != read_failed) {
        read_failed = failed_now;
        Serial.println(failed_now ? "ADS7830 read failed - holding last values"
                                  : "ADS7830 reads recovered");
    }

    // Flaps: FLAPS_LO..FLAPS_HI raw ADC to a value between 0 and 11, offset by
    // FLAPS_LO first so the low end zeroes out.
    if (flaps_raw >= 0) {
        state.adc.flaps_raw = flaps_raw;
        state.adc.flaps_position = SensorUtils::read_and_clamp_adc(flaps_raw - ADC::FLAPS_LO,
                                        0, ADC::FLAPS_HI - ADC::FLAPS_LO, ADC::FLAPS_SCALE);
    }

    // Aileron trim: 0-255 raw ADC to a value between 0 and 100
    if (ailer_raw >= 0) {
        state.adc.aileron_trim = SensorUtils::read_and_clamp_adc(ailer_raw,
                                        ADC::TRIM_LO, ADC::TRIM_HI, ADC::TRIM_SCALE);
    }

    // Elevator trim: 0-255 raw ADC to a value between 0 and 100
    if (elev_raw >= 0) {
        state.adc.elevator_trim = SensorUtils::read_and_clamp_adc(elev_raw,
                                        ADC::TRIM_LO, ADC::TRIM_HI, ADC::TRIM_SCALE);
    }
}

// Declared in sensors.h. Nothing but pulse_isr() may write these.
volatile uint32_t isr_pulse_count = 0;
volatile uint32_t isr_last_pulse_ms = 0;

// Touches no flash-resident code: two DRAM counters and millis(), which is IRAM-safe.
// Calling AppState::instance() from here would reach into flash, which is unavailable
// whenever the cache is off during an NVS write.
static void IRAM_ATTR pulse_isr() {
    isr_pulse_count++;
    isr_last_pulse_ms = millis();
}

static void flow_sensor_timer_cb(lv_timer_t *) {
    AppState &state = AppState::instance();

    uint32_t now = millis();
    uint32_t elapsed_ms = now - state.flow.last_calc_time_ms;
    if (elapsed_ms == 0) return;  // same-millisecond re-entry: nothing to rate yet
    state.flow.last_calc_time_ms = now;

    // Both counters are written by pulse_isr(), so snapshot them together with
    // interrupts held off — otherwise a pulse landing between the two reads gives
    // a count and a timestamp that disagree.
    noInterrupts();
    uint32_t current_pulses = isr_pulse_count;
    uint32_t last_pulse_time = isr_last_pulse_ms;
    interrupts();

    uint32_t pulses_in_interval = current_pulses - state.flow.last_pulse_count;
    state.flow.last_pulse_count = current_pulses;

    // The pulses cover however long it actually was since the last calc, not the
    // nominal timer period — LVGL timers run late, and this callback itself writes
    // NVS every other tick. Rating against a fixed 400 ms would read high.
    float k_factor = (float)state.flow.k_factor_thousands * 1000.0f;
    float gallons_in_interval = (float)pulses_in_interval / k_factor;
    float raw_gph = gallons_in_interval * 3600000.0f / (float)elapsed_ms;

    bool pulse_fresh = (now - last_pulse_time) < FlowSensor::STALE_TIMEOUT_MS;

    // ── Totalizer ─────────────────────────────────────
    // Every pulse is fuel that went through the transducer, so it counts even when
    // the rate is too slow to display. The one thing rejected is an implausibly high
    // rate: that's electrical noise, and folding a noise burst into a total that gets
    // persisted to NVS would corrupt it permanently.
    bool plausible = (raw_gph <= FlowSensor::MAX_GPH);
    if (plausible) {
        state.flow.total_gallons_used += gallons_in_interval;
    }

    float initial_fuel = (float)(state.fuel.left_user_setting + state.fuel.right_user_setting);
    state.flow.remaining_gallons = initial_fuel - state.flow.total_gallons_used;
    if (state.flow.remaining_gallons < 0.0f) state.flow.remaining_gallons = 0.0f;

    // ── Displayed rate ────────────────────────────────
    // Needs a recent pulse and a reading inside the FT-60's rated range — below
    // MIN_GPH the transducer isn't linear, so show nothing rather than a bad number.
    // The gate is hysteretic: once a rate is being shown the bar drops by
    // MIN_GPH_HYSTERESIS, so a reading parked near the threshold doesn't blink on and
    // off as the pulse count per interval alternates between two integers.
    static bool rate_shown = false;
    const float gate = rate_shown
        ? (FlowSensor::MIN_GPH - FlowSensor::MIN_GPH_HYSTERESIS)
        : FlowSensor::MIN_GPH;
    rate_shown = plausible && pulse_fresh && (raw_gph >= gate);

    if (rate_shown) {
        int32_t smoothed_scaled = state.flow.smooth_flow->add_reading((int32_t)(raw_gph * 100.0f));
        state.flow.current_gph = (float)smoothed_scaled / 100.0f;

        state.flow.add_average_sample(raw_gph);

        if (state.flow.current_gph > 0.0f) {
            float hours = state.flow.remaining_gallons / state.flow.current_gph;
            state.flow.time_to_empty_hours = (int32_t)hours;
            state.flow.time_to_empty_minutes = (int32_t)((hours - (float)state.flow.time_to_empty_hours) * 60.0f);
        }
    } else {
        state.flow.current_gph = 0.0f;
        state.flow.time_to_empty_hours = 0;
        state.flow.time_to_empty_minutes = 0;
        // Drop the damping window too, or the first reading after flow resumes is
        // the rate from before it stopped, decaying over the next few ticks.
        state.flow.smooth_flow->reset();
    }

    // Gate on the total actually having moved: parked with the engine off this
    // would otherwise rewrite the same three NVS keys every 30 s indefinitely.
    static uint32_t last_save_ms = 0;
    static float last_saved_total = -1.0f;
    bool saved = (now - last_save_ms) >= Timers::FLOW_SAVE_INTERVAL_MS &&
                 state.flow.total_gallons_used != last_saved_total;
    if (saved) {
        last_save_ms = now;
        last_saved_total = state.flow.total_gallons_used;
        save_flow_totals();
    }

    static uint32_t last_log_ms = 0;
    if (saved || (now - last_log_ms) >= Timers::FLOW_LOG_INTERVAL_MS) {
        last_log_ms = now;
        Serial.printf("Saved: %d  Flow: %.2f GPH  Used: %.2f gal  Rem: %.2f gal  TTE: %02d:%02d  Avg flow: %.2f  Pulses: %u (+%u in %u ms)\n",
                     (int)saved, state.flow.current_gph, state.flow.total_gallons_used, state.flow.remaining_gallons,
                     (int)state.flow.time_to_empty_hours, (int)state.flow.time_to_empty_minutes, state.flow.avg_gph,
                     current_pulses, pulses_in_interval, elapsed_ms);
    }
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
    canvas_dirty = true;  // written straight to the canvas, still needs a push

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
    lv_obj_align(btn_setup, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_add_event_cb(btn_setup, switch_to_setup_event_cb, LV_EVENT_CLICKED, nullptr);

    Serial.println("Start receiving TTL to serial feeds\n");
    serial_fuel_left.begin(FuelSensors::BAUD);
    serial_fuel_right.begin(FuelSensors::BAUD);
    serial_fuel_left.setTimeout(FuelSensors::READ_TIMEOUT_MS);
    serial_fuel_right.setTimeout(FuelSensors::READ_TIMEOUT_MS);

    Wire1.begin(ADC::SDA, ADC::SCL, ADC::I2C_FREQ);
    Serial.println("Adafruit ADS7830 start\n");
    if (!ad7830.begin(ADC::I2C_ADDR, &Wire1)) {
        Serial.println("Failed to initialize ADS7830!\n");
        return;
    }

    state.fuel.smooth_left = new SmoothingBuffer(FuelSensors::SMOOTH_BUFFER_SIZE);
    state.fuel.smooth_right = new SmoothingBuffer(FuelSensors::SMOOTH_BUFFER_SIZE);
    state.flow.smooth_flow = new SmoothingBuffer(FlowSensor::SMOOTH_BUFFER_SIZE);

    // FT-60 output is open-collector; a 2k pull-up to 3.3V is wired on the signal line.
    pinMode(FlowSensor::PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(FlowSensor::PIN), pulse_isr, RISING);

    load_k_factor();   // before the flow timer starts rating any pulses
    load_flow_totals();
    noInterrupts();
    state.flow.last_pulse_count = isr_pulse_count;
    interrupts();
    state.flow.last_calc_time_ms = millis();

    fuel_gaugeL(Timers::GAUGE_FUEL_MS);
    fuel_gaugeR(Timers::GAUGE_FUEL_MS);
    flaps_gauge(Timers::GAUGE_FLAPS_MS);
    trim_gauge(Timers::GAUGE_TRIM_MS);
    flow_gauge(Timers::GAUGE_FLOW_MS);
    
    setup_fuel_gui();
    setup_kfactor_gui();

    lv_screen_load(state.ui.screen_gauges);

    state.startup.last_run = lv_tick_get();
    state.ui.init_complete = true;   // every early return above leaves this false
}

void loop() {
    AppState &state = AppState::instance();

    // setup() failed somewhere; running LVGL or the panel from here is undefined.
    if (!state.ui.init_complete) {
        delay(100);
        return;
    }

    if (state.startup.active) {
        uint32_t tick = lv_tick_get();
        if (tick - state.startup.last_run >= Timers::STARTUP_ANIM_INTERVAL_MS) {
            state.startup.last_run = tick;
            Serial.println(state.startup.value);
            state.fuel.left_percentage = state.startup.value * 5;
            state.fuel.right_percentage = state.startup.value * 5;
            state.adc.flaps_position = state.startup.value;
            state.adc.aileron_trim = state.startup.value * 5;
            state.adc.elevator_trim = state.startup.value * 5;
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

    // Pushing the canvas is ~307 KB over QSPI (~15 ms), so only do it when LVGL
    // actually drew something rather than on every pass through loop().
    if (canvas_dirty) {
        canvas_dirty = false;
        gfx->flush();
    }
}
