#include "sensors.h"
#include <cstring>

SmoothingBuffer::SmoothingBuffer(int size)
    : read_index(0), total(0), buffer_size(size), samples_filled(0) {
    readings = new int[size];
    memset(readings, 0, size * sizeof(int));
}

SmoothingBuffer::~SmoothingBuffer() {
    delete[] readings;
}

int32_t SmoothingBuffer::add_reading(int32_t value) {
    total -= readings[read_index];
    readings[read_index] = value;
    total += value;
    read_index = (read_index + 1) % buffer_size;
    // Divide by the samples actually taken, not the window size — otherwise the
    // zero-filled tail drags the average toward 0 until the window fills.
    if (samples_filled < buffer_size) samples_filled++;
    return total / samples_filled;
}

void SmoothingBuffer::fill(int32_t value) {
    for (int i = 0; i < buffer_size; i++) readings[i] = value;
    total = (int64_t)value * buffer_size;
    read_index = 0;
    samples_filled = buffer_size;
}

void SmoothingBuffer::reset() {
    memset(readings, 0, buffer_size * sizeof(int));
    total = 0;
    read_index = 0;
    samples_filled = 0;
}
