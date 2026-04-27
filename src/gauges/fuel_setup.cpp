#include "fuel_setup.h"

#include <lvgl.h>
#include <Preferences.h>

#include "app_state.h"
#include "hardware_config.h"
#include "ui_utils.h"
#include "flow_gauge.h"

static Preferences prefs;
static const char *fuel_options = "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12";

static void get_fuel_settings() {
    AppState &state = AppState::instance();
    prefs.begin("fuel_data", true);
    state.fuel.left_user_setting = prefs.getInt("left", 0);
    state.fuel.right_user_setting = prefs.getInt("right", 0);
    prefs.end();
}

static void save_fuel_settings() {
    AppState &state = AppState::instance();
    prefs.begin("fuel_data", false);
    prefs.putInt("left", state.fuel.left_user_setting);
    prefs.putInt("right", state.fuel.right_user_setting);
    prefs.end();
    Serial.printf("Saved: Left %d, Right %d\n", state.fuel.left_user_setting, state.fuel.right_user_setting);

    // Refuel event: reset consumption so remaining/TTE compute from the new load
    state.flow.total_gallons_used   = 0.0f;
    state.flow.avg_gph_sample_count = 0;
    if (state.flow.smooth_flow) state.flow.smooth_flow->reset();
    save_flow_totals();

    flow_used_value = 0.0f;
    remain_value = (float)(state.fuel.left_user_setting + state.fuel.right_user_setting);
    avg_gph_value = 0.0f;
}

void load_flow_totals() {
    AppState &state = AppState::instance();
    prefs.begin("flow_data", true);
    state.flow.total_gallons_used    = prefs.getFloat("total_gal_used",  0.0f);
    avg_gph_value                    = prefs.getFloat("avg_gph",         0.0f);
    state.flow.avg_gph_sample_count  = prefs.getUInt( "avg_gph_samples", 0);
    prefs.end();
    Serial.printf("Loaded total gallons: %.3f  avg gph: %.2f  samples: %u\n",
                  state.flow.total_gallons_used, avg_gph_value, state.flow.avg_gph_sample_count);

    get_fuel_settings();
    flow_used_value = state.flow.total_gallons_used;
    remain_value = (float)(state.fuel.left_user_setting + state.fuel.right_user_setting) - flow_used_value;
    if (remain_value < 0.0f) remain_value = 0.0f;
}

void save_flow_totals() {
    AppState &state = AppState::instance();
    prefs.begin("flow_data", false);
    prefs.putFloat("total_gal_used",  state.flow.total_gallons_used);
    prefs.putFloat("avg_gph",         avg_gph_value);
    prefs.putUInt( "avg_gph_samples", state.flow.avg_gph_sample_count);
    prefs.end();
}

static void back_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
         AppState &state = AppState::instance();
         lv_screen_load(state.ui.screen_gauges);
    }
}

static void full_fuel_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        AppState &state = AppState::instance();
        lv_roller_set_selected(state.ui.roller_left, FuelSensors::MAX_FUEL_TANKS, LV_ANIM_ON);
        lv_roller_set_selected(state.ui.roller_right, FuelSensors::MAX_FUEL_TANKS, LV_ANIM_ON);
        state.fuel.left_user_setting = FuelSensors::MAX_FUEL_TANKS;
        state.fuel.right_user_setting = FuelSensors::MAX_FUEL_TANKS;
        save_fuel_settings();
        lv_screen_load(state.ui.screen_gauges);
    }
}

static void update_fuel_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        AppState &state = AppState::instance();
        state.fuel.left_user_setting = lv_roller_get_selected(state.ui.roller_left);
        state.fuel.right_user_setting = lv_roller_get_selected(state.ui.roller_right);
        save_fuel_settings();
        lv_screen_load(state.ui.screen_gauges);
    }
}

static void clear_avg_gph_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        AppState &state = AppState::instance();
        state.flow.avg_gph_sample_count = 0;
        avg_gph_value = 0.0f;
        save_flow_totals();
    }
}

void switch_to_setup_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        AppState &state = AppState::instance();
        get_fuel_settings();
        lv_roller_set_selected(state.ui.roller_left, state.fuel.left_user_setting, LV_ANIM_OFF);
        lv_roller_set_selected(state.ui.roller_right, state.fuel.right_user_setting, LV_ANIM_OFF);
        lv_screen_load(state.ui.screen_setup);
    }
}

static lv_obj_t *make_roller(lv_obj_t *parent, int initial_selection) {
    lv_obj_t *roller = lv_roller_create(parent);
    lv_roller_set_options(roller, fuel_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(roller, initial_selection, LV_ANIM_OFF);
    lv_roller_set_visible_row_count(roller, 3);
    lv_obj_set_style_text_font(roller, &lv_font_montserrat_20, 0);
    lv_obj_set_style_bg_color(roller, lv_color_make(20, 35, 55), LV_PART_MAIN);
    lv_obj_set_style_text_color(roller, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(roller, lv_palette_main(LV_PALETTE_BLUE), LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, lv_color_white(), LV_PART_SELECTED);
    return roller;
}

static lv_obj_t *make_action_btn(lv_obj_t *parent, const char *text, lv_color_t color,
                                 lv_event_cb_t cb) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 130, 42);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
    return btn;
}

void setup_fuel_gui() {
    AppState &state = AppState::instance();
    get_fuel_settings();

    // ── Screen ────────────────────────────────────────────────
    state.ui.screen_setup = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(state.ui.screen_setup, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(state.ui.screen_setup, LV_OPA_COVER, LV_PART_MAIN);

    // ── Top bar ───────────────────────────────────────────────
    lv_obj_t *topbar = lv_obj_create(state.ui.screen_setup);
    lv_obj_set_size(topbar, 300, 52);
    lv_obj_set_pos(topbar, 1, 2);
    lv_obj_set_style_bg_color(topbar, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(topbar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(topbar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(topbar, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(topbar, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *btn_back = lv_button_create(topbar);
    lv_obj_set_size(btn_back, 75, 34);
    lv_obj_set_pos(btn_back, 26, 10);
    lv_obj_set_style_bg_color(btn_back, lv_palette_darken(LV_PALETTE_BLUE_GREY, 2), 0);
    lv_obj_set_style_radius(btn_back, 5, 0);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "< Back");
    lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(btn_back, back_event_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *title = lv_label_create(topbar);
    lv_label_set_text(title, "Fuel Setup");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_pos(title, 150, 10);

    // ── Roller card ───────────────────────────────────────────
    lv_obj_t *card = lv_obj_create(state.ui.screen_setup);
    lv_obj_set_size(card, 380, 150);
    lv_obj_set_pos(card, 50, 65);
    lv_obj_set_style_bg_color(card, lv_color_make(18, 32, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_palette_darken(LV_PALETTE_BLUE, 1), LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);

    // Left column
    lv_obj_t *col_l = lv_obj_create(card);
    lv_obj_set_size(col_l, 160, LV_SIZE_CONTENT);
    lv_obj_align(col_l, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_set_style_bg_opa(col_l, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(col_l, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(col_l, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(col_l, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col_l, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col_l, 6, LV_PART_MAIN);

    lv_obj_t *lbl_l = lv_label_create(col_l);
    lv_label_set_text(lbl_l, "Left Tank");
    lv_obj_set_style_text_font(lbl_l, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_l, lv_palette_lighten(LV_PALETTE_BLUE, 3), 0);

    state.ui.roller_left = make_roller(col_l, state.fuel.left_user_setting);

    // Divider
    lv_obj_t *div = lv_obj_create(card);
    lv_obj_set_size(div, 1, 130);
    lv_obj_align(div, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(div, lv_palette_darken(LV_PALETTE_BLUE_GREY, 1), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(div, 0, LV_PART_MAIN);

    // Right column
    lv_obj_t *col_r = lv_obj_create(card);
    lv_obj_set_size(col_r, 160, LV_SIZE_CONTENT);
    lv_obj_align(col_r, LV_ALIGN_RIGHT_MID, -5, 0);
    lv_obj_set_style_bg_opa(col_r, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(col_r, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(col_r, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(col_r, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col_r, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col_r, 6, LV_PART_MAIN);

    lv_obj_t *lbl_r = lv_label_create(col_r);
    lv_label_set_text(lbl_r, "Right Tank");
    lv_obj_set_style_text_font(lbl_r, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_r, lv_palette_lighten(LV_PALETTE_BLUE, 3), 0);

    state.ui.roller_right = make_roller(col_r, state.fuel.right_user_setting);

    // ── Warning text ──────────────────────────────────────────
    lv_obj_t *warn = lv_label_create(state.ui.screen_setup);
    lv_label_set_text(warn, "Updating resets the fuel flow counters to zero");
    lv_obj_set_width(warn, 440);
    lv_label_set_long_mode(warn, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(warn, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(warn, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_obj_set_style_text_align(warn, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(warn, LV_ALIGN_BOTTOM_MID, 0, -65);

    // ── Action buttons ────────────────────────────────────────
    lv_obj_t *btn_row = lv_obj_create(state.ui.screen_setup);
    lv_obj_set_size(btn_row, 440, 50);
    lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_row, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(btn_row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    make_action_btn(btn_row, "Update",    lv_palette_main(LV_PALETTE_GREEN),       update_fuel_event_cb);
    make_action_btn(btn_row, "Full Fuel", lv_palette_main(LV_PALETTE_BLUE),        full_fuel_event_cb);
    make_action_btn(btn_row, "Clr Avg",   lv_palette_main(LV_PALETTE_DEEP_ORANGE), clear_avg_gph_event_cb);
}
