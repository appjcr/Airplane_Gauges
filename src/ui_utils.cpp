#include "ui_utils.h"

namespace UIUtils {

lv_obj_t* create_button_with_label(lv_obj_t *parent,
                                   const char *label_text) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, label_text);
    return btn;
}

} // namespace UIUtils
