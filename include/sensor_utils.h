#pragma once

#include <cstdint>
#include "sensors.h"

namespace SensorUtils {

// Binary search in capacity table for faster fuel percentage lookup
int32_t capacity_to_percentage(int32_t avg_capacitance,
                               int16_t empty_threshold,
                               const CapacityEntry *table,
                               int table_size);

// Clamp and scale ADC reading
int32_t read_and_clamp_adc(int raw_value,
                           int16_t lo, int16_t hi,
                           float scale);

// Calculate flow metrics from pulse count
void calculate_flow_metrics(uint32_t current_pulses,
                           uint32_t last_pulses,
                           float &gph_out,
                           float &gallons_out);

// Determine fuel color zone (red/yellow/green)
uint32_t get_fuel_zone_color(int32_t fuel_percentage);

} // namespace SensorUtils
