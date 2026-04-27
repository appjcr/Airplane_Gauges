#pragma once

#include <lvgl.h>
#include <cstdint>

extern lv_obj_t *screen_gauges;
extern int32_t elev_trim_value;
extern int32_t ailer_trim_value;

void trim_gauge(int gauge_timer_value);

