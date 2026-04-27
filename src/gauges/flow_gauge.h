#pragma once

#include <lvgl.h>
#include <cstdint>

extern lv_obj_t *screen_gauges;
extern float flow_value;
extern float remain_value;
extern float flow_used_value;
extern float avg_gph_value;
extern int32_t time_to_empty_hours_value;
extern int32_t time_to_empty_minutes_value;

void flow_gauge(int gauge_timer_value);

