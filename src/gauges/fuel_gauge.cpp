#include <lvgl.h>
#include "fuel_gauge.h"
#include "sensor_utils.h"
#include "app_state.h"

int32_t Fuel_L_value = 0;
int32_t Fuel_R_value = 0;
static int32_t old_Fuel_L_value = -1;
static int32_t old_Fuel_R_value = -1;

static const char *custom_labels[] = {"E", "1/4", "1/2", "3/4", "F", NULL};

static lv_obj_t *scale1 = nullptr;
static lv_obj_t *needle_line1 = nullptr;
static lv_obj_t *fuel_value_label1 = nullptr;

static lv_obj_t *scale2 = nullptr;
static lv_obj_t *needle_line2 = nullptr;
static lv_obj_t *fuel_value_label2 = nullptr;

struct SectionStyles {
    lv_style_t items;
    lv_style_t indicator;
    lv_style_t main;
};

static SectionStyles zone1_styles;
static SectionStyles zone2_styles;
static SectionStyles zone3_styles;
static SectionStyles zone4_styles;
static SectionStyles zone5_styles;

static void init_section_styles(SectionStyles *styles, lv_color_t color) {
    lv_style_init(&styles->items);
    lv_style_set_line_color(&styles->items, color);
    lv_style_set_line_width(&styles->items, 0);

    lv_style_init(&styles->indicator);
    lv_style_set_line_color(&styles->indicator, color);
    lv_style_set_line_width(&styles->indicator, 0);
    lv_style_set_text_font(&styles->indicator, LV_FONT_DEFAULT);
    lv_style_set_text_color(&styles->indicator, lv_color_white());

    lv_style_init(&styles->main);
    lv_style_set_arc_color(&styles->main, color);
    lv_style_set_arc_width(&styles->main, 20);
}

static void add_section(lv_obj_t *target_scale, int32_t from, int32_t to,
                        const SectionStyles *styles) {
    lv_scale_section_t *sec = lv_scale_add_section(target_scale);
    lv_scale_set_section_range(target_scale, sec, from, to);
    lv_scale_set_section_style_items(target_scale, sec, &styles->items);
    lv_scale_set_section_style_indicator(target_scale, sec, &styles->indicator);
    lv_scale_set_section_style_main(target_scale, sec, &styles->main);
}

static void ensure_zone_styles() {
    static bool ready = false;
    if (ready) return;
    init_section_styles(&zone1_styles, lv_palette_main(LV_PALETTE_RED));
    init_section_styles(&zone2_styles, lv_palette_main(LV_PALETTE_YELLOW));
    init_section_styles(&zone3_styles, lv_palette_main(LV_PALETTE_GREEN));
    init_section_styles(&zone4_styles, lv_palette_main(LV_PALETTE_GREEN));
    init_section_styles(&zone5_styles, lv_palette_main(LV_PALETTE_GREEN));
    ready = true;
}

struct FuelGaugeConfig {
    lv_obj_t **scale_out;
    lv_obj_t **needle_out;
    lv_obj_t **value_label_out;
    lv_align_t scale_align;
    lv_coord_t scale_x;
    lv_coord_t scale_y;
    lv_align_t circle_align;
    lv_coord_t circle_x;
    lv_coord_t circle_y;
    bool needle_rounded;
    lv_coord_t needle_pad_right;
    const char *tank_name;
    int32_t initial_value;
    lv_timer_cb_t timer_cb;
    int timer_period;
};

static void create_fuel_gauge(const FuelGaugeConfig &cfg) {
    ensure_zone_styles();

    AppState &state = AppState::instance();
    lv_obj_t *scale = lv_scale_create(state.ui.screen_gauges);
    lv_obj_align(scale, cfg.scale_align, cfg.scale_x, cfg.scale_y);
    lv_obj_set_size(scale, 150, 150);
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_OUTER);
    lv_scale_set_range(scale, 0, 100);
    lv_scale_set_total_tick_count(scale, 17);
    lv_scale_set_major_tick_every(scale, 4);
    lv_scale_set_text_src(scale, custom_labels);
    lv_scale_set_angle_range(scale, 180);
    lv_scale_set_rotation(scale, 180);
    lv_scale_set_label_show(scale, true);

    lv_obj_set_style_length(scale, 0, LV_PART_ITEMS);
    lv_obj_set_style_line_color(scale, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_length(scale, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(scale, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(scale, 18, LV_PART_MAIN);

    add_section(scale, 0, 15, &zone1_styles);
    add_section(scale, 16, 25, &zone2_styles);
    add_section(scale, 26, 50, &zone3_styles);
    add_section(scale, 51, 75, &zone4_styles);
    add_section(scale, 76, 100, &zone5_styles);

    lv_obj_t *needle = lv_line_create(scale);
    lv_obj_set_style_line_color(needle, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_line_width(needle, 8, LV_PART_MAIN);
    lv_obj_set_style_length(needle, 40, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(needle, cfg.needle_rounded, LV_PART_MAIN);
    lv_obj_set_style_pad_right(needle, cfg.needle_pad_right, LV_PART_MAIN);
    lv_scale_set_line_needle_value(scale, needle, 50, cfg.initial_value);

    lv_obj_t *circle = lv_obj_create(state.ui.screen_gauges);
    lv_obj_set_size(circle, 108, 108);
    lv_obj_align(circle, cfg.circle_align, cfg.circle_x, cfg.circle_y);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(circle, lv_obj_get_style_bg_color(state.ui.screen_gauges, LV_PART_MAIN), 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(circle, 0, LV_PART_MAIN);

    lv_obj_t *container = lv_obj_create(circle);
    lv_obj_center(container);
    lv_obj_set_size(container, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(container, 0, 0);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *value_label = lv_label_create(container);
    lv_label_set_text_fmt(value_label, "%d%%", cfg.initial_value);
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(value_label, SensorUtils::get_fuel_zone_color(cfg.initial_value), 0);

    lv_obj_t *tank_label = lv_label_create(container);
    lv_label_set_text(tank_label, cfg.tank_name);
    lv_obj_set_style_text_font(tank_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(tank_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(tank_label, LV_TEXT_ALIGN_CENTER, 0);

    *cfg.scale_out = scale;
    *cfg.needle_out = needle;
    *cfg.value_label_out = value_label;

    lv_timer_create(cfg.timer_cb, cfg.timer_period, nullptr);
}

static void fuel_left_anim_timer_cb(lv_timer_t *) {
    if (old_Fuel_L_value != Fuel_L_value) {
        old_Fuel_L_value = Fuel_L_value;
        lv_scale_set_line_needle_value(scale1, needle_line1, 100, Fuel_L_value);
        lv_label_set_text_fmt(fuel_value_label1, "%d%%", Fuel_L_value);
        lv_obj_set_style_text_color(fuel_value_label1, SensorUtils::get_fuel_zone_color(Fuel_L_value), 0);
    }
}

static void fuel_right_anim_timer_cb(lv_timer_t *) {
    if (old_Fuel_R_value != Fuel_R_value) {
        old_Fuel_R_value = Fuel_R_value;
        lv_scale_set_line_needle_value(scale2, needle_line2, 100, Fuel_R_value);
        lv_label_set_text_fmt(fuel_value_label2, "%d%%", Fuel_R_value);
        lv_obj_set_style_text_color(fuel_value_label2, SensorUtils::get_fuel_zone_color(Fuel_R_value), 0);
    }
}

void fuel_gaugeL(int gaugeL_timer_value) {
    FuelGaugeConfig cfg = {
        .scale_out = &scale1,
        .needle_out = &needle_line1,
        .value_label_out = &fuel_value_label1,
        .scale_align = LV_ALIGN_TOP_LEFT,
        .scale_x = 30,
        .scale_y = 30,
        .circle_align = LV_ALIGN_TOP_LEFT,
        .circle_x = 51,
        .circle_y = 50,
        .needle_rounded = false,
        .needle_pad_right = 50,
        .tank_name = "Left",
        .initial_value = 50,
        .timer_cb = fuel_left_anim_timer_cb,
        .timer_period = gaugeL_timer_value,
    };
    create_fuel_gauge(cfg);
}

void fuel_gaugeR(int gaugeR_timer_value) {
    FuelGaugeConfig cfg = {
        .scale_out = &scale2,
        .needle_out = &needle_line2,
        .value_label_out = &fuel_value_label2,
        .scale_align = LV_ALIGN_TOP_RIGHT,
        .scale_x = -40,
        .scale_y = 30,
        .circle_align = LV_ALIGN_TOP_RIGHT,
        .circle_x = -61,
        .circle_y = 50,
        .needle_rounded = true,
        .needle_pad_right = 5,
        .tank_name = "Right",
        .initial_value = 75,
        .timer_cb = fuel_right_anim_timer_cb,
        .timer_period = gaugeR_timer_value,
    };
    create_fuel_gauge(cfg);
}
