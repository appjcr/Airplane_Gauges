#include "kfactor_setup.h"

#include <lvgl.h>
#include <Preferences.h>

#include "app_state.h"
#include "hardware_config.h"
#include "sensors.h"

static Preferences kfac_prefs;

static lv_obj_t *kfac_current_label = nullptr;

// "10\n11\n...\n100", built once on first use. Generated from the range constants so
// the option list and the index arithmetic below cannot drift apart — the roller's
// selected index is MIN_K_FACTOR_THOUSANDS less than the value it represents.
static const char *kfactor_roller_options() {
    static constexpr int OPTION_COUNT =
        FlowSensor::MAX_K_FACTOR_THOUSANDS - FlowSensor::MIN_K_FACTOR_THOUSANDS + 1;
    static char options[OPTION_COUNT * 5 + 1];
    static bool built = false;
    if (!built) {
        size_t len = 0;
        for (int v = FlowSensor::MIN_K_FACTOR_THOUSANDS;
             v <= FlowSensor::MAX_K_FACTOR_THOUSANDS; v++) {
            int n = snprintf(options + len, sizeof(options) - len,
                             (v == FlowSensor::MIN_K_FACTOR_THOUSANDS) ? "%d" : "\n%d", v);
            // snprintf returns the length it *would* have written, so a truncated
            // write would push len past the buffer and underflow the remaining-space
            // argument on the next pass. Stop instead.
            if (n < 0 || (size_t)n >= sizeof(options) - len) break;
            len += (size_t)n;
        }
        built = true;
    }
    return options;
}

static void save_k_factor() {
    AppState &state = AppState::instance();
    kfac_prefs.begin("flow_data", false);
    kfac_prefs.putInt("k_thousands", state.flow.k_factor_thousands);
    kfac_prefs.end();
    Serial.printf("Saved K-factor: %d000 pulses/gal\n", state.flow.k_factor_thousands);
}

void load_k_factor() {
    AppState &state = AppState::instance();
    kfac_prefs.begin("flow_data", true);
    int stored = kfac_prefs.getInt("k_thousands", FlowSensor::DEFAULT_K_FACTOR_THOUSANDS);
    kfac_prefs.end();

    // A value outside the roller's range can only come from a corrupt or stale NVS
    // entry; fall back rather than letting it divide the flow calculation.
    if (stored < FlowSensor::MIN_K_FACTOR_THOUSANDS ||
        stored > FlowSensor::MAX_K_FACTOR_THOUSANDS) {
        Serial.printf("Stored K-factor %d out of range, using default %d\n",
                      stored, FlowSensor::DEFAULT_K_FACTOR_THOUSANDS);
        stored = FlowSensor::DEFAULT_K_FACTOR_THOUSANDS;
    }

    state.flow.k_factor_thousands = stored;
    Serial.printf("Loaded K-factor: %d000 pulses/gal\n", state.flow.k_factor_thousands);
}

static void kfac_back_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        AppState &state = AppState::instance();
        lv_screen_load(state.ui.screen_setup);
    }
}

static void kfac_update_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    AppState &state = AppState::instance();
    state.flow.k_factor_thousands =
        FlowSensor::MIN_K_FACTOR_THOUSANDS + (int)lv_roller_get_selected(state.ui.roller_kfactor);
    save_k_factor();

    // Pulses banked since the last flow tick were collected under the old K-factor.
    // Resync the watermark so they aren't re-rated against the new one.
    noInterrupts();
    state.flow.last_pulse_count = isr_pulse_count;
    interrupts();
    state.flow.last_calc_time_ms = millis();

    lv_label_set_text_fmt(kfac_current_label, "Current: %d,000 pulses/gal",
                          state.flow.k_factor_thousands);
    lv_screen_load(state.ui.screen_setup);
}

void switch_to_kfactor_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        AppState &state = AppState::instance();
        // Re-sync the roller to the live value so a previous cancel doesn't linger.
        lv_roller_set_selected(state.ui.roller_kfactor,
                               state.flow.k_factor_thousands - FlowSensor::MIN_K_FACTOR_THOUSANDS,
                               LV_ANIM_OFF);
        lv_label_set_text_fmt(kfac_current_label, "Current: %d,000 pulses/gal",
                              state.flow.k_factor_thousands);
        lv_screen_load(state.ui.screen_kfactor);
    }
}

void setup_kfactor_gui() {
    AppState &state = AppState::instance();

    // ── Screen ────────────────────────────────────────────────
    state.ui.screen_kfactor = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(state.ui.screen_kfactor, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(state.ui.screen_kfactor, LV_OPA_COVER, LV_PART_MAIN);

    // ── Top bar ───────────────────────────────────────────────
    lv_obj_t *topbar = lv_obj_create(state.ui.screen_kfactor);
    lv_obj_set_size(topbar, 470, 52);
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
    lv_obj_add_event_cb(btn_back, kfac_back_event_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *title = lv_label_create(topbar);
    lv_label_set_text(title, "K-Factor");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_pos(title, 150, 10);

    // ── Card ──────────────────────────────────────────────────
    lv_obj_t *card = lv_obj_create(state.ui.screen_kfactor);
    lv_obj_set_size(card, 380, 150);
    lv_obj_set_pos(card, 50, 65);
    lv_obj_set_style_bg_color(card, lv_color_make(18, 32, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_palette_darken(LV_PALETTE_BLUE, 1), LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *col = lv_obj_create(card);
    lv_obj_set_size(col, 340, LV_SIZE_CONTENT);
    lv_obj_center(col);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(col, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(col, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col, 6, LV_PART_MAIN);

    lv_obj_t *lbl = lv_label_create(col);
    lv_label_set_text(lbl, "Thousands of pulses per gallon");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_lighten(LV_PALETTE_BLUE, 3), 0);

    state.ui.roller_kfactor = lv_roller_create(col);
    lv_roller_set_options(state.ui.roller_kfactor, kfactor_roller_options(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(state.ui.roller_kfactor,
                           state.flow.k_factor_thousands - FlowSensor::MIN_K_FACTOR_THOUSANDS,
                           LV_ANIM_OFF);
    lv_roller_set_visible_row_count(state.ui.roller_kfactor, 3);
    lv_obj_set_style_text_font(state.ui.roller_kfactor, &lv_font_montserrat_20, 0);
    lv_obj_set_style_bg_color(state.ui.roller_kfactor, lv_color_make(20, 35, 55), LV_PART_MAIN);
    lv_obj_set_style_text_color(state.ui.roller_kfactor, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(state.ui.roller_kfactor, lv_palette_main(LV_PALETTE_BLUE), LV_PART_SELECTED);
    lv_obj_set_style_text_color(state.ui.roller_kfactor, lv_color_white(), LV_PART_SELECTED);

    // ── Current value ─────────────────────────────────────────
    kfac_current_label = lv_label_create(state.ui.screen_kfactor);
    lv_label_set_text_fmt(kfac_current_label, "Current: %d,000 pulses/gal",
                          state.flow.k_factor_thousands);
    lv_obj_set_style_text_font(kfac_current_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(kfac_current_label, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_obj_align(kfac_current_label, LV_ALIGN_BOTTOM_MID, 0, -65);

    // ── Update button ─────────────────────────────────────────
    lv_obj_t *btn_update = lv_button_create(state.ui.screen_kfactor);
    lv_obj_set_size(btn_update, 130, 42);
    lv_obj_align(btn_update, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_update, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_radius(btn_update, 6, 0);
    lv_obj_t *lbl_update = lv_label_create(btn_update);
    lv_label_set_text(lbl_update, "Update");
    lv_obj_set_style_text_font(lbl_update, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_update);
    lv_obj_add_event_cb(btn_update, kfac_update_event_cb, LV_EVENT_CLICKED, nullptr);
}
