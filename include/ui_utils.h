#pragma once

#include <lvgl.h>

namespace UIUtils {

// Create a styled label with common properties
lv_obj_t* create_label(lv_obj_t *parent,
                       const char *text,
                       const lv_font_t *font = LV_FONT_DEFAULT,
                       lv_color_t color = lv_color_white());

// Create a button with label
lv_obj_t* create_button_with_label(lv_obj_t *parent,
                                   const char *label_text);

// Create a container with common styles
lv_obj_t* create_container(lv_obj_t *parent,
                          uint16_t width, uint16_t height,
                          lv_color_t bg_color = lv_color_black());

// Create an object (rectangle) with size and color
lv_obj_t* create_rectangle(lv_obj_t *parent,
                          uint16_t width, uint16_t height,
                          lv_color_t color,
                          uint16_t x = 0, uint16_t y = 0);

} // namespace UIUtils
