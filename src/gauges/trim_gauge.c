#include "lvgl.h"
#include "trim_gauge.h"

int32_t elev_trim_value = 0;
int32_t ailer_trim_value = 0;
int32_t old_elev_trim_value = -1;
int32_t old_ailer_trim_value = -1;

static lv_obj_t * elev_label = NULL;
static lv_obj_t * ailer_label = NULL;
static lv_obj_t * elev_line = NULL;
static lv_obj_t * ailer_line = NULL;


static void trim_anim_timer_cb(lv_timer_t * timer1)
{
    LV_UNUSED(timer1);
    if (old_elev_trim_value != elev_trim_value) {
        old_elev_trim_value=elev_trim_value;
        // set elev trim position
        lv_obj_set_pos(elev_line, 55, (elev_trim_value * 1.4));
    }
    if (old_ailer_trim_value!=ailer_trim_value) {
        old_ailer_trim_value=ailer_trim_value;
        // set ailer trim position
        lv_obj_set_pos(ailer_line, (1.4 * ailer_trim_value), 55); 
    }
}

void trim_gauge(int gauge_timer_value)
{
    //Expecting value range from 0-100 in elev_trim_value and ailer_trim_value
    // Set-create gauge design
    // Create the container
    lv_obj_t * cont = lv_obj_create(screen_gauges);
    lv_obj_set_size(cont, 180, 180);
    lv_obj_set_style_bg_color(cont, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, 0); // Remove border
    lv_obj_align(cont,LV_ALIGN_BOTTOM_MID,0,0);

    // Create Horizontal White Line
    lv_obj_t * h_line = lv_obj_create(cont);
    lv_obj_set_size(h_line, 140, 5);
    lv_obj_set_style_bg_color(h_line, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_border_width(h_line, 0, 0); // Remove border
    lv_obj_align(h_line, LV_ALIGN_CENTER, 0, 0);

    // Create Vertical White Line
    lv_obj_t * v_line = lv_obj_create(cont);
    lv_obj_set_size(v_line, 5, 140);
    lv_obj_set_style_bg_color(v_line, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_border_width(v_line, 0, 0); // Remove border
    lv_obj_align(v_line, LV_ALIGN_CENTER, 0, 0);

    // Create elevator indicator
    elev_line = lv_obj_create(cont);
    lv_obj_set_size(elev_line, 40, 5);
    lv_obj_set_style_bg_color(elev_line, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_border_width(elev_line, 0, 0); // Remove border
    lv_obj_set_pos(elev_line, 55, 140);

    // Create aileron indicator
    ailer_line = lv_obj_create(cont);
    lv_obj_set_size(ailer_line, 5, 40);
    lv_obj_set_style_bg_color(ailer_line, lv_palette_main(LV_PALETTE_YELLOW), 0);
    lv_obj_set_style_border_width(ailer_line, 0, 0);
    lv_obj_set_pos(ailer_line, 140, 55);

    elev_label = lv_label_create(cont);
    lv_label_set_text(elev_label, "Elev");
    lv_obj_set_style_text_font(elev_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(elev_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(elev_label, LV_ALIGN_BOTTOM_MID, 50, 0);

    ailer_label = lv_label_create(cont);
    lv_label_set_text(ailer_label, "Ailr");
    lv_obj_set_style_text_font(ailer_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ailer_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(ailer_label, LV_ALIGN_LEFT_MID, 0, -30);

    lv_timer_create(trim_anim_timer_cb, gauge_timer_value, NULL);
}
