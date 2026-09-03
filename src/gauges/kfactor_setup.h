#pragma once

#include <lvgl.h>

// Builds the K-factor screen. Call once from setup(), after setup_fuel_gui().
void setup_kfactor_gui();

// Opens the K-factor screen. Wired to the K-FAC button on the fuel setup screen.
void switch_to_kfactor_event_cb(lv_event_t *e);

// Restores the stored K-factor into AppState. Call once at startup, before the
// flow timer begins rating pulses.
void load_k_factor();
