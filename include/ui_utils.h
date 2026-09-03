#pragma once

#include <lvgl.h>

namespace UIUtils {

// Create a button with label
lv_obj_t* create_button_with_label(lv_obj_t *parent,
                                   const char *label_text);

} // namespace UIUtils
