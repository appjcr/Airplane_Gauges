#include "sensor_utils.h"
#include "hardware_config.h"
#include <lvgl.h>
#include <cmath>

namespace SensorUtils {

int32_t capacity_to_percentage(int32_t avg_capacitance,
                               int16_t empty_threshold,
                               const CapacityEntry *table,
                               int table_size) {
    if (table_size <= 0) return 0;
    if (avg_capacitance >= empty_threshold) return 0;

    // Binary search for the first entry whose threshold exceeds the reading.
    // lo stays within [0, table_size - 1] throughout, so the result is always valid.
    int lo = 0, hi = table_size - 1;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (avg_capacitance < table[mid].threshold) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return table[lo].percentage;
}

int32_t read_and_clamp_adc(int raw_value,
                           int16_t lo, int16_t hi,
                           float scale) {
    if (raw_value < lo) raw_value = lo;
    if (raw_value > hi) raw_value = hi;
    return (int32_t)std::round(raw_value * scale);
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
