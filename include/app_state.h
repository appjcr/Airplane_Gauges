#pragma once

#include <cstdint>
#include <lvgl.h>
#include "sensors.h"
#include "hardware_config.h"

// ── Fuel System State ─────────────────────────────────
struct FuelSystem {
    // Tank contents in percent, from the capacitance sensors via the lookup tables.
    int32_t left_percentage = 0;
    int32_t right_percentage = 0;

    // Gallons aboard as entered on the setup screen; persisted to NVS.
    int left_user_setting = 0;
    int right_user_setting = 0;

    SmoothingBuffer* smooth_left = nullptr;
    SmoothingBuffer* smooth_right = nullptr;
};

// ── ADC System State ──────────────────────────────────
struct ADCSystem {
    int32_t flaps_raw = 0;        // straight off the ADS7830, before scaling
    int32_t flaps_position = 0;   // 0..11 bar units
    int32_t elevator_trim = 0;    // percent
    int32_t aileron_trim = 0;     // percent
};

// ── Flow System State ─────────────────────────────────
struct FlowSystem {
    float current_gph = 0.0f;
    float total_gallons_used = 0.0f;
    float remaining_gallons = 0.0f;
    int32_t time_to_empty_hours = 0;
    int32_t time_to_empty_minutes = 0;

    // Running mean of raw_gph and the sample count it is divided by. These are two
    // halves of ONE value: they must be reset together and persisted together, so
    // reset them only through reset_average() and never touch either alone.
    float avg_gph = 0.0f;
    uint32_t avg_gph_sample_count = 0;

    void reset_average() {
        avg_gph = 0.0f;
        avg_gph_sample_count = 0;
    }

    // Fold one sample into the running mean. Incrementing the count first is what
    // makes the first sample land on itself rather than on half of itself.
    void add_average_sample(float raw_gph) {
        avg_gph_sample_count++;
        avg_gph += (raw_gph - avg_gph) / (float)avg_gph_sample_count;
    }

    // Owned by flow_sensor_timer_cb(): watermark and timestamp of the last rate calc.
    // The raw ISR counters live at file scope in main.cpp — see sensors.h.
    uint32_t last_pulse_count = 0;
    uint32_t last_calc_time_ms = 0;

    // Transducer K-factor in thousands of pulses per gallon (68 => 68,000). Set on
    // the K-FAC screen, persisted to NVS, restored at startup.
    int k_factor_thousands = FlowSensor::DEFAULT_K_FACTOR_THOUSANDS;
    SmoothingBuffer *smooth_flow = nullptr;
};

// ── Serial Buffer State ───────────────────────────────
struct SerialBuffer {
    uint8_t data[10] = {0};
    int32_t counter = 0;
    int32_t bytes_read = 0;
};

// ── Startup Animation State ──────────────────────────
struct StartupAnimation {
    bool active = true;
    bool reverse = false;
    int16_t value = 0;
    uint32_t last_run = 0;
};

// ── LVGL UI State ─────────────────────────────────────
struct LVGLState {
    lv_obj_t *screen_gauges = nullptr;
    lv_obj_t *screen_setup = nullptr;
    lv_obj_t *screen_kfactor = nullptr;
    lv_obj_t *roller_left = nullptr;
    lv_obj_t *roller_right = nullptr;
    lv_obj_t *roller_kfactor = nullptr;
    bool first_initialization = true;
    // False if setup() bailed out early; loop() must not touch LVGL or the panel.
    bool init_complete = false;
};

// ── Global Application State ──────────────────────────
class AppState {
public:
    static AppState& instance();

    FuelSystem fuel;
    ADCSystem adc;
    FlowSystem flow;
    SerialBuffer serial_left;
    SerialBuffer serial_right;
    StartupAnimation startup;
    LVGLState ui;

private:
    AppState() = default;
};
