#include "ui_utils.h"

namespace UIUtils {

lv_obj_t* create_label(lv_obj_t *parent,
                       const char *text,
                       const lv_font_t *font,
                       lv_color_t color) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    if (font != LV_FONT_DEFAULT) {
        lv_obj_set_style_text_font(label, font, 0);
    }
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

lv_obj_t* create_button_with_label(lv_obj_t *parent,
                                   const char *label_text) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, label_text);
    return btn;
}

lv_obj_t* create_container(lv_obj_t *parent,
                          uint16_t width, uint16_t height,
                          lv_color_t bg_color) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, width, height);
    lv_obj_set_style_bg_color(cont, bg_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, 0);
    return cont;
}

lv_obj_t* create_rectangle(lv_obj_t *parent,
                          uint16_t width, uint16_t height,
                          lv_color_t color,
                          uint16_t x, uint16_t y) {
    lv_obj_t *rect = lv_obj_create(parent);
    lv_obj_set_size(rect, width, height);
    lv_obj_set_pos(rect, x, y);
    lv_obj_set_style_bg_color(rect, color, 0);
    lv_obj_set_style_border_width(rect, 0, 0);
    return rect;
}

} // namespace UIUtils
