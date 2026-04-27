#pragma once

#include <cstdint>
#include <lvgl.h>
#include "sensors.h"

// ── Fuel System State ─────────────────────────────────
struct FuelSystem {
    FuelReading left_tank;
    FuelReading right_tank;
    int left_user_setting;
    int right_user_setting;

    SmoothingBuffer* smooth_left = nullptr;
    SmoothingBuffer* smooth_right = nullptr;
};

// ── ADC System State ──────────────────────────────────
struct ADCSystem {
    int32_t flaps_position;
    int32_t elevator_trim;
    int32_t aileron_trim;
};

// ── Flow System State ─────────────────────────────────
struct FlowSystem {
    float current_gph;
    float total_gallons_used;
    uint32_t avg_gph_sample_count = 0;
    float remaining;
    float used;
    int32_t time_to_empty_hours;
    int32_t time_to_empty_minutes;
    uint32_t pulse_count;
    uint32_t last_pulse_count;
    uint32_t last_pulse_time_ms = 0;
    SmoothingBuffer *smooth_flow = nullptr;
};

// ── Serial Buffer State ───────────────────────────────
struct SerialBuffer {
    uint8_t data[10];
    int32_t counter;
    int32_t bytes_read;
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
    lv_obj_t *roller_left = nullptr;
    lv_obj_t *roller_right = nullptr;
    bool first_initialization = true;
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
