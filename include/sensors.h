#pragma once

#include <cstdint>

// ── Flow Sensor Pulse Counters ────────────────────────
// Defined in main.cpp and written ONLY by pulse_isr(). Deliberately kept at file
// scope instead of inside AppState: the ISR is IRAM_ATTR and must not reach into
// flash-resident code, and AppState::instance() is an out-of-line singleton. The
// flash cache is disabled during NVS writes, which this firmware does regularly.
// Read them together under noInterrupts() so the pair stays consistent.
extern volatile uint32_t isr_pulse_count;
extern volatile uint32_t isr_last_pulse_ms;

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
    int samples_filled;   // averages over real samples only until the window fills
};
