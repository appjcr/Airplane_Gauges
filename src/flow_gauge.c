#include "lvgl.h"
#include "flow_gauge.h"

float flow_value = 12.52f;
float remain_value = 10.01f;
float flow_used_value = 12.02f;
int32_t time_to_empty_seconds_value = 8400;
int32_t time_to_empty_minutes_value = 20;
int32_t time_to_empty_hours_value = 2;

static lv_obj_t * flow_value_label = NULL;
static lv_obj_t * remain_value_label = NULL;
static lv_obj_t * flow_used_value_label = NULL;
static lv_obj_t * time_to_empty_value_label = NULL;
static lv_obj_t * flow_label = NULL;
static lv_obj_t * remain_label = NULL;
static lv_obj_t * flow__used_label = NULL;
static lv_obj_t * time_to_empty_label = NULL;
static lv_obj_t * gph_label = NULL;
static lv_obj_t * g_label = NULL;

// Define ranges and structures
#define MIN_VAL 0
#define MAX_VAL 100
static int32_t counter = 0;
static bool increasing = true;

static void flow_anim_timer_cb(lv_timer_t * timer1)
{
    LV_UNUSED(timer1);

    if (increasing) {
        counter++;
        if (counter >= MAX_VAL) {
            counter = MAX_VAL;
            increasing = false; // Switch to descending
        }
    } else {
        counter--;
        if (counter <= MIN_VAL) {
            counter = MIN_VAL;
            increasing = true; // Switch to ascending
        }
    }

    flow_value = counter;

}


void flow_gauge(void)
{
    // Set-create gauge design
    // Create the container
    lv_obj_t * cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, 160, 180);
    lv_obj_set_style_bg_color(cont, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, 0); 
    lv_obj_align(cont,LV_ALIGN_BOTTOM_LEFT,0,0);

    flow_label = lv_label_create(cont);
    lv_label_set_text(flow_label, "Flow:");
    lv_obj_set_style_text_font(flow_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(flow_label, lv_color_white(), 0);
    lv_obj_align(flow_label, LV_ALIGN_TOP_LEFT, -15, 4);

    flow_value_label = lv_label_create(cont);
    lv_label_set_text_fmt(flow_value_label, "%.2f", flow_value);
    lv_obj_set_style_text_font(flow_value_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(flow_value_label, lv_color_white(), 0);
    lv_obj_align(flow_value_label, LV_ALIGN_TOP_LEFT, 38, 0);

    gph_label = lv_label_create(cont);
    lv_label_set_text(gph_label, "gph");
    lv_obj_set_style_text_font(gph_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(gph_label, lv_color_white(), 0);
    lv_obj_align(gph_label, LV_ALIGN_TOP_LEFT, 100, 4);

    remain_label = lv_label_create(cont);
    lv_label_set_text(remain_label, "Rem: ");
    lv_obj_set_style_text_font(remain_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(remain_label, lv_color_white(), 0);
    lv_obj_align(remain_label, LV_ALIGN_TOP_LEFT, -15, 44);

    remain_value_label = lv_label_create(cont);
    lv_label_set_text_fmt(remain_value_label, "%.2f", remain_value);
    lv_obj_set_style_text_font(remain_value_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(remain_value_label, lv_color_white(), 0);
    lv_obj_align(remain_value_label, LV_ALIGN_TOP_LEFT, 38, 40);

    g_label = lv_label_create(cont);
    lv_label_set_text(g_label, " g");
    lv_obj_set_style_text_font(g_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(g_label, lv_color_white(), 0);
    lv_obj_align(g_label, LV_ALIGN_TOP_LEFT, 100, 44);

    flow__used_label = lv_label_create(cont);
    lv_label_set_text(flow__used_label, "Used: ");
    lv_obj_set_style_text_font(flow__used_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(flow__used_label, lv_color_white(), 0);
    lv_obj_align(flow__used_label, LV_ALIGN_TOP_LEFT, -15, 84);

    flow_used_value_label = lv_label_create(cont);
    lv_label_set_text_fmt(flow_used_value_label, "%.2f", flow_used_value);
    lv_obj_set_style_text_font(flow_used_value_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(flow_used_value_label, lv_color_white(), 0);
    lv_obj_align(flow_used_value_label, LV_ALIGN_TOP_LEFT, 38, 80);

    lv_obj_t * g_label1 = lv_label_create(cont);
    lv_label_set_text(g_label1, " g");
    lv_obj_set_style_text_font(g_label1, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(g_label1, lv_color_white(), 0);
    lv_obj_align(g_label1, LV_ALIGN_TOP_LEFT, 100, 84);

    time_to_empty_label = lv_label_create(cont);
    lv_label_set_text(time_to_empty_label, "T to E: ");
    lv_obj_set_style_text_font(time_to_empty_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(time_to_empty_label, lv_color_white(), 0);
    lv_obj_align(time_to_empty_label, LV_ALIGN_TOP_LEFT, -15, 124);

    time_to_empty_value_label = lv_label_create(cont);
    lv_label_set_text_fmt(time_to_empty_value_label, "%02d:%02d", (int)time_to_empty_hours_value, (int)time_to_empty_minutes_value);
    lv_obj_set_style_text_font(time_to_empty_value_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(time_to_empty_value_label, lv_color_white(), 0);
    lv_obj_align(time_to_empty_value_label, LV_ALIGN_TOP_LEFT, 45, 120);

    lv_timer_create(flow_anim_timer_cb, 1000, NULL);
}
