#include "lvgl.h"
#include "flaps_gauge.h"

// Define a structure to hold line and style references
typedef struct {
    lv_obj_t *lines[11];
} line_array_t;

// Create container and store line references
line_array_t line_group;

int32_t Flaps_position_value=5;

static lv_obj_t * flaps_label = NULL;
static lv_obj_t * flaps_up_label = NULL;

// Define ranges and structures
#define MIN_VAL 0
#define MAX_VAL 20
static int32_t counter = 0;
static bool increasing = true;

static const char * custom_labels1[] = {"20", "15", "10", "5", "0", NULL};

static void flaps_anim_timer_cb(lv_timer_t * timer1)
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

    Flaps_position_value = counter;

    //  Show "UP" when position is 0
    if (Flaps_position_value==0) lv_obj_remove_flag(flaps_up_label, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(flaps_up_label, LV_OBJ_FLAG_HIDDEN);

    // Show lines as flaps are lowered
    if (Flaps_position_value>0) lv_obj_remove_flag(line_group.lines[0], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(line_group.lines[0], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=2) lv_obj_remove_flag(line_group.lines[1], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(line_group.lines[1], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=4) lv_obj_remove_flag(line_group.lines[2], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(line_group.lines[2], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=6) lv_obj_remove_flag(line_group.lines[3], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(line_group.lines[3], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=8) lv_obj_remove_flag(line_group.lines[4], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(line_group.lines[4], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=10) lv_obj_remove_flag(line_group.lines[5], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(line_group.lines[5], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=12) lv_obj_remove_flag(line_group.lines[6], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(line_group.lines[6], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=14) lv_obj_remove_flag(line_group.lines[7], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(line_group.lines[7], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=16) lv_obj_remove_flag(line_group.lines[8], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(line_group.lines[8], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=18) lv_obj_remove_flag(line_group.lines[9], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(line_group.lines[9], LV_OBJ_FLAG_HIDDEN);
    if (Flaps_position_value>=20) lv_obj_remove_flag(line_group.lines[10], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(line_group.lines[10], LV_OBJ_FLAG_HIDDEN);
    
}

void flaps_gauge(int gauge_timer_value)
{
    // 1. Create the container (transparent, no border, bottom-right)
    lv_obj_t * cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, 115, 150);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_RIGHT, -30, -15);

    // 2. Create the vertical scale (100px wide, 150px high, 0-20)
    lv_obj_t * scale = lv_scale_create(cont);
    lv_obj_set_size(scale, 20, 120);
    lv_scale_set_mode(scale, LV_SCALE_MODE_VERTICAL_RIGHT);
    lv_scale_set_range(scale, 0, 20);

    // Configure scale ticks (total 21 ticks for 0-20, one every 1)
    lv_scale_set_total_tick_count(scale, 9);
    lv_scale_set_major_tick_every(scale, 2); // Label every 5
    lv_scale_set_text_src(scale, custom_labels1);
    lv_scale_set_label_show(scale, true);

    // Style the scale to fit the area
    lv_obj_set_style_pad_hor(scale, 10, LV_PART_MAIN); // Padding for labels
    lv_obj_set_style_length(scale, 5, LV_PART_INDICATOR); // Major tick length
    lv_obj_set_style_length(scale, 0, LV_PART_ITEMS); // Minor tick length
    lv_obj_align(scale, LV_ALIGN_BOTTOM_RIGHT, -5, 0);

    // Style for all lines (35px width, 10px height, no border)
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_width(&style_line, 35);
    lv_style_set_height(&style_line, 10);
    lv_style_set_border_width(&style_line, 0);

    // Create lines for flap positions
    // Style for all lines (35px width, 10px height, no border)
    for (int i = 0; i < 11; i++) {
        // Create the line as a rectangle object
        line_group.lines[i] = lv_obj_create(cont);
        lv_obj_add_style(line_group.lines[i], &style_line, 0);
        
        // Position one under another 
        lv_obj_set_pos(line_group.lines[i], 20, -5 + (i * 11)); 

        // Set colors based on position
        if (i == 0) {
            // First line: Yellow
            lv_obj_set_style_bg_color(line_group.lines[i], lv_palette_main(LV_PALETTE_YELLOW), 0);
        } else if (i == 1 || i == 2) {
            // Next two: Yellow
            lv_obj_set_style_bg_color(line_group.lines[i], lv_palette_main(LV_PALETTE_YELLOW), 0);
        } else {
            // Remainder: Orange
            lv_obj_set_style_bg_color(line_group.lines[i], lv_palette_main(LV_PALETTE_ORANGE), 0);
        }
    }

    flaps_label = lv_label_create(cont);
    lv_label_set_text(flaps_label, "F\nl\na\np\ns");
    lv_obj_set_style_text_font(flaps_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(flaps_label, lv_color_white(), 0);
    lv_obj_align(flaps_label, LV_ALIGN_LEFT_MID, 0, -5);

    flaps_up_label = lv_label_create(cont);
    lv_label_set_text(flaps_up_label, "U\nP");
    lv_obj_set_style_text_font(flaps_up_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(flaps_up_label, lv_color_white(), 0);
    lv_obj_align(flaps_up_label, LV_ALIGN_CENTER, 0, -5);

    lv_timer_create(flaps_anim_timer_cb, gauge_timer_value, NULL);
}
