#pragma once

#include <lvgl.h>
#include <cstdint>

extern lv_obj_t *screen_gauges;
extern int32_t Flaps_position_value;

void flaps_gauge(int gauge_timer_value);

