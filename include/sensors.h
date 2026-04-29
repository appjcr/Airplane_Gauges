#pragma once

#include <cstdint>

// ── Fuel Tank Capacitance Sensors ──────────────────────
struct FuelSensorConfig {
    int pin;
    int baud;
    int16_t cap_full;
    int16_t cap_empty;
    int16_t empty_threshold;
    const char *name;
};

struct FuelReading {
    int32_t raw;
    int32_t smoothed;
    int32_t percentage;
};

// ── ADC Channels & Scaling ────────────────────────────
enum class ADCChannel : uint8_t {
    FLAPS = 0,
    AILERON = 1,
    ELEVATOR = 2
};

struct ADCRange {
    int16_t lo;
    int16_t hi;
    float scale;
};

// ── Flow Sensor ───────────────────────────────────────
struct FlowMetrics {
    float current_gph;
    float total_gallons_used;
    uint32_t pulse_count;
};

// ── Trim Positions ────────────────────────────────────
struct TrimState {
    int32_t elevator;
    int32_t aileron;
    float position_scale;
};

// Capacity lookup table entry
struct CapacityEntry {
    int16_t threshold;
    int8_t percentage;
};

// Rolling average buffer
class SmoothingBuffer {
public:
    explicit SmoothingBuffer(int size);
    ~SmoothingBuffer();
    int32_t add_reading(int32_t value);
    void fill(int32_t value);
    void reset();

private:
    int *readings;
    int read_index;
    int64_t total;
    int buffer_size;
};
