#pragma once

#include <lvgl.h>
#include <cstdint>

extern lv_obj_t *screen_gauges;
extern int32_t Fuel_L_value;
extern int32_t Fuel_R_value;

void fuel_gaugeL(int gaugeL_timer_value);
void fuel_gaugeR(int gaugeR_timer_value);

