#include <lvgl.h>
#include "flaps_gauge.h"

int32_t Flaps_position_value = 0;
int32_t old_Flaps_position_value = -1;

static lv_obj_t *flaps_label = nullptr;
static lv_obj_t *flaps_up_label = nullptr;
static lv_obj_t *lines[11] = {nullptr};

static const char *custom_labels[] = {"20", "15", "10", "5", "0", nullptr};

static void flaps_anim_timer_cb(lv_timer_t *) {
    if (old_Flaps_position_value == Flaps_position_value) return;

    old_Flaps_position_value = Flaps_position_value;
    if (Flaps_position_value == 0) {
        lv_obj_remove_flag(flaps_up_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(flaps_up_label, LV_OBJ_FLAG_HIDDEN);
    }

    for (int i = 0; i < 11; i++) {
        int32_t threshold = (i == 0) ? 1 : i * 2;
        if (Flaps_position_value >= threshold) {
            lv_obj_remove_flag(lines[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(lines[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void flaps_gauge(int gauge_timer_value) {
    lv_obj_t *cont = lv_obj_create(screen_gauges);
    lv_obj_set_size(cont, 115, 150);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_RIGHT, -30, -15);

    lv_obj_t *scale = lv_scale_create(cont);
    lv_obj_set_size(scale, 20, 120);
    lv_scale_set_mode(scale, LV_SCALE_MODE_VERTICAL_RIGHT);
    lv_scale_set_range(scale, 0, 20);
    lv_scale_set_total_tick_count(scale, 9);
    lv_scale_set_major_tick_every(scale, 2);
    lv_scale_set_text_src(scale, custom_labels);
    lv_scale_set_label_show(scale, true);
    lv_obj_set_style_pad_hor(scale, 10, LV_PART_MAIN);
    lv_obj_set_style_length(scale, 5, LV_PART_INDICATOR);
    lv_obj_set_style_length(scale, 0, LV_PART_ITEMS);
    lv_obj_align(scale, LV_ALIGN_BOTTOM_RIGHT, -5, 0);

    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_width(&style_line, 35);
    lv_style_set_height(&style_line, 10);
    lv_style_set_border_width(&style_line, 0);

    for (int i = 0; i < 11; i++) {
        lines[i] = lv_obj_create(cont);
        lv_obj_add_style(lines[i], &style_line, 0);
        lv_obj_set_pos(lines[i], 20, -5 + (i * 11));
        lv_palette_t color = (i < 3) ? LV_PALETTE_YELLOW : LV_PALETTE_ORANGE;
        lv_obj_set_style_bg_color(lines[i], lv_palette_main(color), 0);
    }

    flaps_label = lv_label_create(cont);
    lv_label_set_text(flaps_label, "F\nl\na\np\ns");
    lv_obj_set_style_text_font(flaps_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(flaps_label, lv_color_white(), 0);
    lv_obj_align(flaps_label, LV_ALIGN_LEFT_MID, 0, -5);

    flaps_up_label = lv_label_create(cont);
    lv_label_set_text(flaps_up_label, "U\nP");
    lv_obj_set_style_text_font(flaps_up_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(flaps_up_label, lv_color_white(), 0);
    lv_obj_align(flaps_up_label, LV_ALIGN_CENTER, 0, -5);

    lv_timer_create(flaps_anim_timer_cb, gauge_timer_value, nullptr);
}
