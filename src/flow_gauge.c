#include "lvgl.h"
#include "flow_gauge.h"

int32_t flow_value = 0;
int32_t remail_value = 0;
int32_t flow_used_value = 0;
int32_t time_to_empty_value = 0;


static lv_obj_t * flow_value_label = NULL;
static lv_obj_t * remail_value_label = NULL;
static lv_obj_t * flow_used_value_label = NULL;
static lv_obj_t * time_to_empty_value_label = NULL;
static lv_obj_t * flow_label = NULL;
static lv_obj_t * remain_label = NULL;
static lv_obj_t * flow__used_label = NULL;
static lv_obj_t * time_to_empty_label = NULL;

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


void trim_gauge(void)
{
    // Set-create gauge design
    // Create the container
    lv_obj_t * cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, 180, 180);
    lv_obj_set_style_bg_color(cont, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 2, 0); 
    lv_obj_align(cont,LV_ALIGN_BOTTOM_LEFT,0,0);

    flow_label = lv_label_create(cont);
    lv_label_set_text(flow_label, "Elevator");
    lv_obj_set_style_text_font(flow_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(flow_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(flow_label, LV_ALIGN_TOP_LEFT, 0, 0);

    remain_label = lv_label_create(cont);
    lv_label_set_text(remain_label, "Aileron");
    lv_obj_set_style_text_font(remain_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(remain_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(remain_label, LV_ALIGN_LEFT_MID, 0, 20);

    lv_timer_create(flow_anim_timer_cb, 1000, NULL);
}
