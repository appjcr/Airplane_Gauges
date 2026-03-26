#include "lvgl.h"
#include "flaps_gauge.h"

static int32_t Flaps_value = 5;
static int8_t flaps_step1 = 1;

static const char * custom_labels1[] = {"0", "5", "10", "15", "20", NULL};

static lv_obj_t * scale1 = NULL;
static lv_obj_t * needle_line1 = NULL;
static lv_obj_t * flaps_value_label1 = NULL;
static lv_obj_t * flaps_label1 = NULL;

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


static lv_color_t get_flaps_zone_color(int32_t flap_color)
{
    if(flap_color < 6) return lv_palette_main(LV_PALETTE_GREEN); /* Zone 1 */
    else if(flap_color < 16) return lv_palette_main(LV_PALETTE_YELLOW); /* Zone 2 */
    else return lv_palette_main(LV_PALETTE_GREY);
}

static void flaps_anim_timer_cb(lv_timer_t * timer1)
{
    LV_UNUSED(timer1);

    Flaps_value += flaps_step1;

    if(Flaps_value >= 20) {
        Flaps_value = 20;
        flaps_step1 = -1;
    }
    else if(Flaps_value <= 0) {
        Flaps_value = 0;
        flaps_step1 = 1;
    }

    /* Update needle */
    lv_scale_set_line_needle_value(scale1, needle_line1, 100, Flaps_value);

    /* Update Fuel percentage text */
    lv_label_set_text_fmt(flaps_value_label1, "%d degrees", Flaps_value);

    /* Update text color based on zone */
    lv_color_t zone_color = get_flaps_zone_color(Flaps_value);
    lv_obj_set_style_text_color(flaps_value_label1, zone_color, 0);
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

void flaps_gauge(void)
{
    // Set-create gauge design
    scale1 = lv_scale_create(lv_screen_active());
    lv_obj_align(scale1,LV_ALIGN_BOTTOM_RIGHT,0,0);
    lv_obj_set_size(scale1, 150, 150);
    lv_scale_set_mode(scale1, LV_SCALE_MODE_VERTICAL_RIGHT);
    lv_scale_set_range(scale1, 0, 20);
    lv_scale_set_total_tick_count(scale1, 9);
    lv_scale_set_major_tick_every(scale1, 2);
    lv_scale_set_text_src(scale1, custom_labels1);
    //lv_scale_set_angle_range(scale1, 180);
    //lv_scale_set_rotation(scale1, 180);
    lv_scale_set_label_show(scale1, true);

    lv_obj_set_style_length(scale1, 0, LV_PART_ITEMS);
    lv_obj_set_style_line_color(scale1, lv_color_white(), LV_PART_ITEMS); // Major ticks
    lv_obj_set_style_length(scale1, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(scale1, lv_color_white(), LV_PART_INDICATOR); // Minor ticks
    lv_obj_set_style_arc_width(scale1, 18, LV_PART_MAIN);

    /* Zone 1: (GREEN) */
    init_section_styles(&zone1_styles, lv_palette_main(LV_PALETTE_GREEN));
    add_section(scale1, 0, 5, &zone1_styles);

    /* Zone 2: (YELLOW) */
    init_section_styles(&zone2_styles, lv_palette_main(LV_PALETTE_YELLOW));
    add_section(scale1, 6, 15, &zone2_styles);

    /* Zone 3: (GREY) */
    init_section_styles(&zone3_styles, lv_palette_main(LV_PALETTE_GREY));
    add_section(scale1, 16, 20, &zone3_styles);

    needle_line1 = lv_line_create(scale1);

    /* Optional styling */
    lv_obj_set_style_line_color(needle_line1, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_line_width(needle_line1, 8, LV_PART_MAIN);
    lv_obj_set_style_length(needle_line1, 40, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(needle_line1, false, LV_PART_MAIN);
    lv_obj_set_style_pad_right(needle_line1, 50, LV_PART_MAIN);

    int32_t current_hr1 = 0;

    lv_scale_set_line_needle_value(scale1, needle_line1, 50, current_hr1);

    flaps_label1 = lv_label_create(lv_scr_act());
    lv_label_set_text(flaps_label1, "Flaps");
    lv_obj_set_style_text_font(flaps_label1, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(flaps_label1, lv_color_white(), 0);
    lv_obj_set_style_text_align(flaps_label1, LV_TEXT_ALIGN_CENTER, 0);

    lv_timer_create(flaps_anim_timer_cb, 500, NULL);
}
