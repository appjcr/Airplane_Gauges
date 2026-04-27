#include "sensors.h"
#include <cstring>

SmoothingBuffer::SmoothingBuffer(int size)
    : read_index(0), total(0), buffer_size(size) {
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
    return total / buffer_size;
}

void SmoothingBuffer::reset() {
    memset(readings, 0, buffer_size * sizeof(int));
    total = 0;
    read_index = 0;
}
