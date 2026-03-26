#include "lvgl.h"
#include "flaps_gauge.h"

// Define a structure to hold line references
typedef struct {
    lv_obj_t *lines[11];
} line_array_t;

// Create container and store line references
line_array_t line_group;

int32_t Flaps_position_value=5;
static int8_t flaps_step1 = 1;
static lv_obj_t * flaps_label1 = NULL;

static const char * custom_labels1[] = {"0", "5", "10", "15", "20", NULL};




// SUBROUTINE: Switches lines from hidden to visible
void set_lines_hidden(bool hidden) {
    for (int i = 0; i < 11; i++) {
        if (hidden) {
            lv_obj_add_flag(line_group.lines[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(line_group.lines[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void flaps_anim_timer_cb(lv_timer_t * timer1)
{
    LV_UNUSED(timer1);

    Flaps_position_value += flaps_step1;

    if(Flaps_position_value >= 20) {
        Flaps_position_value = 20;
        flaps_step1 = -1;
    }
    else if(Flaps_position_value <= 0) {
        Flaps_position_value = 0;
        flaps_step1 = 1;
    }

/*
    if (Flaps_position_value>=0) lv_obj_remove_flag(line_group.lines[1], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(line_group.lines[1], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>0) lv_obj_remove_flag(line_group.lines[2], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(line_group.lines[2], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=2) lv_obj_remove_flag(line_group.lines[3], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(line_group.lines[3], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=4) lv_obj_remove_flag(line_group.lines[4], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(line_group.lines[4], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=6) lv_obj_remove_flag(line_group.lines[5], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(line_group.lines[5], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=8) lv_obj_remove_flag(line_group.lines[6], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(line_group.lines[6], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=10) lv_obj_remove_flag(line_group.lines[7], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(line_group.lines[7], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=12) lv_obj_remove_flag(line_group.lines[8], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(line_group.lines[8], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=14) lv_obj_remove_flag(line_group.lines[9], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(line_group.lines[9], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=16) lv_obj_remove_flag(line_group.lines[10], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(line_group.lines[10], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=18) lv_obj_remove_flag(line_group.lines[11], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(line_group.lines[11], LV_OBJ_FLAG_HIDDEN);
*/
    
}


void flaps_gauge(void)
{
    // 1. Create the container (transparent, no border, bottom-right)
    lv_obj_t * cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, 150, 150);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    // Align to bottom-right with some padding (e.g., -10px)
    lv_obj_align(cont, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    // 2. Create the vertical scale (100px wide, 150px high, 0-20)
    lv_obj_t * scale = lv_scale_create(cont);
    lv_obj_set_size(scale, 100, 110);
    lv_scale_set_mode(scale, LV_SCALE_MODE_VERTICAL_RIGHT);
    lv_scale_set_range(scale, 0, 20);

    // Configure scale ticks (total 21 ticks for 0-20, one every 1)
    lv_scale_set_total_tick_count(scale, 9);
    lv_scale_set_major_tick_every(scale, 2); // Label every 5
    lv_scale_set_text_src(scale, custom_labels1);
    lv_scale_set_label_show(scale, true);

    // Style the scale to fit the 100x150 area
    lv_obj_set_style_pad_hor(scale, 10, LV_PART_MAIN); // Padding for labels
    lv_obj_set_style_length(scale, 10, LV_PART_INDICATOR); // Major tick length
    lv_obj_set_style_length(scale, 5, LV_PART_ITEMS); // Minor tick length
    lv_obj_align(scale, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    //lv_obj_set_style_length(scale1, 0, LV_PART_ITEMS);
    //lv_obj_set_style_line_color(scale1, lv_color_white(), LV_PART_ITEMS); // Major ticks
    //lv_obj_set_style_length(scale1, 0, LV_PART_INDICATOR);
    //lv_obj_set_style_line_color(scale1, lv_color_white(), LV_PART_INDICATOR); // Minor ticks

    // Ensure the scale is positioned inside the container (if necessary)
    lv_obj_center(scale);


    // Create lines for flap positions
    // Style for all lines (35px width, 2px height, no border)
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_width(&style_line, 35);
    lv_style_set_height(&style_line, 5);
    lv_style_set_border_width(&style_line, 0);

    for (int i = 0; i < 11; i++) {
        // Create the line as a rectangle object
        line_group.lines[i] = lv_obj_create(cont);
        lv_obj_add_style(line_group.lines[i], &style_line, 0);
        
        // Position one under another (10px spacing + 2px height)
        lv_obj_set_pos(line_group.lines[i], 5, 5 + (i * 12)); 

        // Set colors based on position
        if (i == 0) {
            // First line: Blue
            lv_obj_set_style_bg_color(line_group.lines[i], lv_palette_main(LV_PALETTE_BLUE), 0);
        } else if (i == 1 || i == 2) {
            // Next two: Yellow
            lv_obj_set_style_bg_color(line_group.lines[i], lv_palette_main(LV_PALETTE_YELLOW), 0);
        } else {
            // Remainder: Orange
            lv_obj_set_style_bg_color(line_group.lines[i], lv_palette_main(LV_PALETTE_ORANGE), 0);
        }
    }

    flaps_label1 = lv_label_create(lv_scr_act());
    lv_label_set_text(flaps_label1, "Flaps");
    lv_obj_set_style_text_font(flaps_label1, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(flaps_label1, lv_color_white(), 0);
    lv_obj_set_style_text_align(flaps_label1, LV_TEXT_ALIGN_CENTER, 0);

    lv_timer_create(flaps_anim_timer_cb, 500, NULL);
}
