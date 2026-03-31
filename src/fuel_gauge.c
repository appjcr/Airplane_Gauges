#include "lvgl.h"
#include "fuel_gauge.h"

int32_t Fuel_L_value = 0;
int32_t Fuel_R_value = 0;
int32_t old_Fuel_L_value = -1;
int32_t old_Fuel_R_value = -1;

static const char * custom_labels1[] = {"E", "1/4", "1/2", "3/4", "F", NULL};
static const char * custom_labels2[] = {"E", "1/4", "1/2", "3/4", "F", NULL};

// FUEL LEFT
static lv_obj_t * scale1 = NULL;
static lv_obj_t * needle_line1 = NULL;
static lv_obj_t * fuel_value_label1 = NULL;
static lv_obj_t * fuel_tank_label1 = NULL;

// FUEL RIGHT
static lv_obj_t * scale2 = NULL;
static lv_obj_t * needle_line2 = NULL;
static lv_obj_t * fuel_value_label2 = NULL;
static lv_obj_t * fuel_tank_label2 = NULL;

typedef struct {
    lv_style_t items;
    lv_style_t indicator;
    lv_style_t main;
} section_styles_t;

static section_styles_t zone1_styles;
static section_styles_t zone2_styles;
static section_styles_t zone3_styles;
static section_styles_t zone4_styles;
static section_styles_t zone5_styles;


static lv_color_t get_fuel_zone_color(int32_t hr)
{
    if(hr < 15) return lv_palette_main(LV_PALETTE_RED); /* Zone 1 */
    else if(hr < 25) return lv_palette_main(LV_PALETTE_YELLOW); /* Zone 2 */
    else if(hr < 50) return lv_palette_main(LV_PALETTE_GREEN); /* Zone 3 */
    else if(hr < 75) return lv_palette_main(LV_PALETTE_GREEN); /* Zone 4 */
    else return lv_palette_main(LV_PALETTE_GREEN); /* Zone 5 */
}

static void fuel_left_anim_timer_cb(lv_timer_t * timer1)
{
    LV_UNUSED(timer1);
    if (old_Fuel_L_value!=Fuel_L_value) {
        old_Fuel_L_value=Fuel_L_value;
        /* Update needle */
        lv_scale_set_line_needle_value(scale1, needle_line1, 100, Fuel_L_value);
        /* Update Fuel percentage text */
        lv_label_set_text_fmt(fuel_value_label1, "%d%%", Fuel_L_value);
        /* Update text color based on zone */
        lv_color_t zone_color = get_fuel_zone_color(Fuel_L_value);
        lv_obj_set_style_text_color(fuel_value_label1, zone_color, 0);
        //lv_obj_set_style_text_color(fuel_tank_label, zone_color, 0);
    }
}

static void fuel_right_anim_timer_cb(lv_timer_t * timer2)
{
    LV_UNUSED(timer2);
    if (old_Fuel_R_value!=Fuel_R_value) {
        old_Fuel_R_value=Fuel_R_value;
        /* Update needle */
        lv_scale_set_line_needle_value(scale2, needle_line2, 100, Fuel_R_value);
        /* Update Fuel percentag */
        lv_label_set_text_fmt(fuel_value_label2, "%d%%", Fuel_R_value);
        /* Update text color based on zone */
        lv_color_t zone_color = get_fuel_zone_color(Fuel_R_value);
        lv_obj_set_style_text_color(fuel_value_label2, zone_color, 0);
        //lv_obj_set_style_text_color(fuel_tank_label2, zone_color, 0);
    }
}

static void init_section_styles(section_styles_t * styles, lv_color_t color)
{
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

static void add_section(lv_obj_t * target_scale,
                        int32_t from,
                        int32_t to,
                        const section_styles_t * styles)
{
    lv_scale_section_t * sec = lv_scale_add_section(target_scale);
    lv_scale_set_section_range(target_scale, sec, from, to);
    lv_scale_set_section_style_items(target_scale, sec, &styles->items);
    lv_scale_set_section_style_indicator(target_scale, sec, &styles->indicator);
    lv_scale_set_section_style_main(target_scale, sec, &styles->main);
}

void fuel_gaugeL(int gaugeL_timer_value) {
    // Expect vales in the range of 0-100 in Fuel_L_value
    // Set-create gauge design
    scale1 = lv_scale_create(lv_screen_active());
    lv_obj_align(scale1,LV_ALIGN_TOP_LEFT,30,30);
    lv_obj_set_size(scale1, 150, 150);
    lv_scale_set_mode(scale1, LV_SCALE_MODE_ROUND_OUTER);
    lv_scale_set_range(scale1, 0, 100);
    lv_scale_set_total_tick_count(scale1, 17);
    lv_scale_set_major_tick_every(scale1, 4);
    lv_scale_set_text_src(scale1, custom_labels1);
    lv_scale_set_angle_range(scale1, 180);
    lv_scale_set_rotation(scale1, 180);
    lv_scale_set_label_show(scale1, true);

    lv_obj_set_style_length(scale1, 0, LV_PART_ITEMS);
    lv_obj_set_style_line_color(scale1, lv_color_white(), LV_PART_ITEMS); // Major ticks
    lv_obj_set_style_length(scale1, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(scale1, lv_color_white(), LV_PART_INDICATOR); // Minor ticks
    lv_obj_set_style_arc_width(scale1, 18, LV_PART_MAIN);

    /* Zone 1: (RED) */
    init_section_styles(&zone1_styles, lv_palette_main(LV_PALETTE_RED));
    add_section(scale1, 0, 15, &zone1_styles);

    /* Zone 2: (YELLOW) */
    init_section_styles(&zone2_styles, lv_palette_main(LV_PALETTE_YELLOW));
    add_section(scale1, 16, 25, &zone2_styles);

    /* Zone 3: (Green) */
    init_section_styles(&zone3_styles, lv_palette_main(LV_PALETTE_GREEN));
    add_section(scale1, 26, 50, &zone3_styles);

    /* Zone 4: (GREEN) */
    init_section_styles(&zone4_styles, lv_palette_main(LV_PALETTE_GREEN));
    add_section(scale1, 51, 75, &zone4_styles);

    /* Zone 5: (GREEN) */
    init_section_styles(&zone5_styles, lv_palette_main(LV_PALETTE_GREEN));
    add_section(scale1, 76, 100, &zone5_styles);

    needle_line1 = lv_line_create(scale1);

    /* Optional styling */
    lv_obj_set_style_line_color(needle_line1, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_line_width(needle_line1, 8, LV_PART_MAIN);
    lv_obj_set_style_length(needle_line1, 40, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(needle_line1, false, LV_PART_MAIN);
    lv_obj_set_style_pad_right(needle_line1, 50, LV_PART_MAIN);

    int32_t current_hr1 = 50;

    lv_scale_set_line_needle_value(scale1, needle_line1, 50, current_hr1);

    lv_obj_t * circle1 = lv_obj_create(lv_screen_active());
    lv_obj_set_size(circle1, 108, 108);
    lv_obj_align(circle1,LV_ALIGN_TOP_LEFT,51,50);


    lv_obj_set_style_radius(circle1, LV_RADIUS_CIRCLE, 0);

    lv_obj_set_style_bg_color(circle1, lv_obj_get_style_bg_color(lv_screen_active(), LV_PART_MAIN), 0);
    lv_obj_set_style_bg_opa(circle1, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(circle1, 0, LV_PART_MAIN);

    lv_obj_t * fuel_container1 = lv_obj_create(circle1);
    lv_obj_center(fuel_container1);
    lv_obj_set_size(fuel_container1, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(fuel_container1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(fuel_container1, 0, 0);
    lv_obj_set_layout(fuel_container1, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(fuel_container1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(fuel_container1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(fuel_container1, 0, 0);
    lv_obj_set_flex_align(fuel_container1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    fuel_value_label1 = lv_label_create(fuel_container1);
    lv_label_set_text_fmt(fuel_value_label1, "%d%%", current_hr1);
    lv_obj_set_style_text_font(fuel_value_label1, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(fuel_value_label1, LV_TEXT_ALIGN_CENTER, 0);

    fuel_tank_label1 = lv_label_create(fuel_container1);
    lv_label_set_text(fuel_tank_label1, "Left");
    lv_obj_set_style_text_font(fuel_tank_label1, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(fuel_tank_label1, lv_color_white(), 0);
    lv_obj_set_style_text_align(fuel_tank_label1, LV_TEXT_ALIGN_CENTER, 0);

    lv_color_t zone_color = get_fuel_zone_color(current_hr1);
    lv_obj_set_style_text_color(fuel_value_label1, zone_color, 0);

    lv_timer_create(fuel_left_anim_timer_cb, gaugeL_timer_value, NULL);
}

void fuel_gaugeR(int gaugeR_timer_value) {
    // Expect vales in the range of 0-100 in Fuel_R_value
    // Set-create gauge design
    scale2 = lv_scale_create(lv_screen_active());
    lv_obj_align(scale2,LV_ALIGN_TOP_RIGHT,-40,30);
    lv_obj_set_size(scale2, 150, 150);
    lv_scale_set_mode(scale2, LV_SCALE_MODE_ROUND_OUTER);
    lv_scale_set_range(scale2, 0, 100);
    lv_scale_set_total_tick_count(scale2, 17);
    lv_scale_set_major_tick_every(scale2, 4);
    lv_scale_set_text_src(scale2, custom_labels2);
    lv_scale_set_angle_range(scale2, 180);
    lv_scale_set_rotation(scale2, 180);
    lv_scale_set_label_show(scale2, true);

    lv_obj_set_style_length(scale2, 0, LV_PART_ITEMS);
    lv_obj_set_style_line_color(scale2, lv_color_white(), LV_PART_ITEMS); // Major ticks
    lv_obj_set_style_length(scale2, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(scale2, lv_color_white(), LV_PART_INDICATOR); // Minor ticks
    lv_obj_set_style_arc_width(scale2, 18, LV_PART_MAIN);

    /* Zone 1: (RED) */
    init_section_styles(&zone1_styles, lv_palette_main(LV_PALETTE_RED));
    add_section(scale2, 0, 15, &zone1_styles);

    /* Zone 2: (YELLOW) */
    init_section_styles(&zone2_styles, lv_palette_main(LV_PALETTE_YELLOW));
    add_section(scale2, 16, 25, &zone2_styles);

    /* Zone 3: (Green) */
    init_section_styles(&zone3_styles, lv_palette_main(LV_PALETTE_GREEN));
    add_section(scale2, 26, 50, &zone3_styles);

    /* Zone 4: (GREEN) */
    init_section_styles(&zone4_styles, lv_palette_main(LV_PALETTE_GREEN));
    add_section(scale2, 51, 75, &zone4_styles);

    /* Zone 5: (GREEN) */
    init_section_styles(&zone5_styles, lv_palette_main(LV_PALETTE_GREEN));
    add_section(scale2, 76, 100, &zone5_styles);


    needle_line2 = lv_line_create(scale2);

    /* Optional styling */
    lv_obj_set_style_line_color(needle_line2, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_line_width(needle_line2, 8, LV_PART_MAIN);
    lv_obj_set_style_length(needle_line2, 40, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(needle_line2, true, LV_PART_MAIN);
    lv_obj_set_style_pad_right(needle_line2, 5, LV_PART_MAIN);

    int32_t current_hr2 = 75;

    lv_scale_set_line_needle_value(scale2, needle_line2, 50, current_hr2);

    lv_obj_t * circle2 = lv_obj_create(lv_screen_active());
    lv_obj_set_size(circle2, 108, 108);
    lv_obj_align(circle2,LV_ALIGN_TOP_RIGHT,-61,50);


    lv_obj_set_style_radius(circle2, LV_RADIUS_CIRCLE, 0);

    lv_obj_set_style_bg_color(circle2, lv_obj_get_style_bg_color(lv_screen_active(), LV_PART_MAIN), 0);
    lv_obj_set_style_bg_opa(circle2, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(circle2, 0, LV_PART_MAIN);

    lv_obj_t * fuel_container2 = lv_obj_create(circle2);
    lv_obj_center(fuel_container2);
    lv_obj_set_size(fuel_container2, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(fuel_container2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(fuel_container2, 0, 0);
    lv_obj_set_layout(fuel_container2, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(fuel_container2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(fuel_container2, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(fuel_container2, 0, 0);
    lv_obj_set_flex_align(fuel_container2, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    fuel_value_label2 = lv_label_create(fuel_container2);
    lv_label_set_text_fmt(fuel_value_label2, "%d%%", current_hr2);
    lv_obj_set_style_text_font(fuel_value_label2, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(fuel_value_label2, LV_TEXT_ALIGN_CENTER, 0);

    fuel_tank_label2 = lv_label_create(fuel_container2);
    lv_label_set_text(fuel_tank_label2, "Right");
    lv_obj_set_style_text_font(fuel_tank_label2, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(fuel_tank_label2, LV_TEXT_ALIGN_CENTER, 0);

    lv_color_t zone_color = get_fuel_zone_color(current_hr2);
    lv_obj_set_style_text_color(fuel_value_label2, zone_color, 0);
    lv_obj_set_style_text_color(fuel_tank_label2, lv_color_white(), 0);

    lv_timer_create(fuel_right_anim_timer_cb, gaugeR_timer_value, NULL);
}
