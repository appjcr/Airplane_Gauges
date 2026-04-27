#include <lvgl.h>
#include "flow_gauge.h"
#include "app_state.h"

float flow_value = 0.00f;
float remain_value = 0.00f;
float flow_used_value = 0.00f;
float avg_gph_value = 0.00f;
int32_t time_to_empty_minutes_value = 0;
int32_t time_to_empty_hours_value = 0;

static lv_obj_t *flow_value_label = nullptr;
static lv_obj_t *remain_value_label = nullptr;
static lv_obj_t *flow_used_value_label = nullptr;
static lv_obj_t *avg_gph_value_label = nullptr;
static lv_obj_t *time_to_empty_value_label = nullptr;

static void flow_anim_timer_cb(lv_timer_t *) {
    lv_label_set_text_fmt(flow_value_label, "%.2f", flow_value);
    lv_label_set_text_fmt(remain_value_label, "%.2f", remain_value);
    lv_label_set_text_fmt(flow_used_value_label, "%.2f", flow_used_value);
    lv_label_set_text_fmt(avg_gph_value_label, "%.2f", avg_gph_value);
    if (flow_value > 0.0f) {
        lv_label_set_text_fmt(time_to_empty_value_label, "%02d:%02d",
                             (int)time_to_empty_hours_value, (int)time_to_empty_minutes_value);
    } else {
        lv_label_set_text(time_to_empty_value_label, "HH:MM");
    }
}

struct FlowLabel {
    const char *label_text;
    int label_x;
    int label_y;
    const char *value_format;
    int value_x;
    int value_y;
    lv_obj_t **value_ref;
    const char *unit_text;
    int unit_x;
    int unit_y;
};

static lv_obj_t *create_flow_gph(lv_obj_t *cont, const FlowLabel &cfg) {
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, cfg.label_text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, cfg.label_x, cfg.label_y);

    lv_obj_t *value = lv_label_create(cont);
    lv_label_set_text(value, "0.00");
    lv_obj_set_style_text_font(value, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(value, lv_color_white(), 0);
    lv_obj_align(value, LV_ALIGN_TOP_LEFT, cfg.value_x, cfg.value_y);
    *cfg.value_ref = value;

    lv_obj_t *unit = lv_label_create(cont);
    lv_label_set_text(unit, cfg.unit_text);
    lv_obj_set_style_text_font(unit, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(unit, lv_color_white(), 0);
    lv_obj_align(unit, LV_ALIGN_TOP_LEFT, cfg.unit_x, cfg.unit_y);

    return value;
}

void flow_gauge(int gauge_timer_value) {
    AppState &state = AppState::instance();
    lv_obj_t *flow_cont = lv_obj_create(state.ui.screen_gauges);
    lv_obj_set_size(flow_cont, 165, 210);
    lv_obj_set_style_bg_color(flow_cont, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(flow_cont, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(flow_cont, 0, 0);
    lv_obj_align(flow_cont, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    FlowLabel flow_cfg = {
        "Flow:", -15, 4, "%.2f", 38, 0, &flow_value_label, "gph", 95, 4};
    create_flow_gph(flow_cont, flow_cfg);

    FlowLabel remain_cfg = {
        "Rem: ", -15, 44, "%.2f", 38, 40, &remain_value_label, " g", 95, 44};
    create_flow_gph(flow_cont, remain_cfg);

    FlowLabel used_cfg = {
        "Used: ", -15, 84, "%.2f", 38, 80, &flow_used_value_label, " g", 95, 84};
    create_flow_gph(flow_cont, used_cfg);

    FlowLabel avg_cfg = {
        "Avg: ", -15, 124, "%.2f", 38, 120, &avg_gph_value_label, "gph", 95, 124};
    create_flow_gph(flow_cont, avg_cfg);

    lv_obj_t *time_label = lv_label_create(flow_cont);
    lv_label_set_text(time_label, "T to E: ");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, -15, 164);

    time_to_empty_value_label = lv_label_create(flow_cont);
    lv_label_set_text_fmt(time_to_empty_value_label, "%02d:%02d",
                         (int)time_to_empty_hours_value, (int)time_to_empty_minutes_value);
    lv_obj_set_style_text_font(time_to_empty_value_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(time_to_empty_value_label, lv_color_white(), 0);
    lv_obj_align(time_to_empty_value_label, LV_ALIGN_TOP_LEFT, 45, 160);

    lv_timer_create(flow_anim_timer_cb, gauge_timer_value, nullptr);
}
