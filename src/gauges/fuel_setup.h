#pragma once

#include <lvgl.h>

void setup_fuel_gui();
void switch_to_setup_event_cb(lv_event_t *e);
void load_flow_totals();
void save_flow_totals();
