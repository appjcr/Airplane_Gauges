#include "sensor_utils.h"
#include "hardware_config.h"
#include <lvgl.h>
#include <cmath>

namespace SensorUtils {

int32_t capacity_to_percentage(int32_t avg_capacitance,
                               int16_t empty_threshold,
                               const CapacityEntry *table,
                               int table_size) {
    if (avg_capacitance >= empty_threshold) return 0;

    // Binary search for efficiency
    int lo = 0, hi = table_size - 1;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (avg_capacitance < table[mid].threshold) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return (lo < table_size) ? table[lo].percentage : 0;
}

int32_t read_and_clamp_adc(int raw_value,
                           int16_t lo, int16_t hi,
                           float scale) {
    if (raw_value < lo) raw_value = lo;
    if (raw_value > hi) raw_value = hi;
    return (int32_t)std::round(raw_value * scale);
}

void calculate_flow_metrics(uint32_t current_pulses,
                           uint32_t last_pulses,
                           float &gph_out,
                           float &gallons_out) {
    uint32_t pulses_in_interval = current_pulses - last_pulses;
    float k_factor = FlowSensor::PULSES_PER_GALLON;
    gph_out = ((float)pulses_in_interval / k_factor) * 3600.0f;
    gallons_out += (float)pulses_in_interval / k_factor;
}

lv_color_t get_fuel_zone_color(int32_t fuel_percentage) {
    if (fuel_percentage < 15) {
        return lv_palette_main(LV_PALETTE_RED);
    } else if (fuel_percentage < 25) {
        return lv_palette_main(LV_PALETTE_YELLOW);
    }
    return lv_palette_main(LV_PALETTE_GREEN);
}

} // namespace SensorUtils
