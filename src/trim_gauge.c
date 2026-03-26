#include "lvgl.h"
#include "trim_gauge.h"

static int32_t Trim_value = 5;
static int8_t trim_step1 = 1;


static lv_obj_t * trim_value_label1 = NULL;
static lv_obj_t * trim_label1 = NULL;
static lv_obj_t * trim_value_label2 = NULL;
static lv_obj_t * trim_label2 = NULL;




static void trim_anim_timer_cb(lv_timer_t * timer1)
{
    LV_UNUSED(timer1);

    Trim_value += trim_step1;

    if(Trim_value >= 20) {
        Trim_value = 20;
        trim_step1 = -1;
    }
    else if(Trim_value <= 0) {
        Trim_value = 0;
        trim_step1 = 1;
    }


    /* Update text color based on zone */
    //lv_color_t zone_color = get_trim_zone_color(Trim_value);
    //lv_obj_set_style_text_color(trim_value_label1, zone_color, 0);
}


void trim_gauge(void)
{
    // Set-create gauge design

    // create 9 section horizontal rectangle aliron trim
    // Create the container
    lv_obj_t * horiz_rect = lv_obj_create(lv_scr_act());
    lv_obj_set_size(horiz_rect, 150, 50); // Narrow rectangle
    //lv_obj_center(horiz_rect);
    lv_obj_align(horiz_rect,LV_ALIGN_BOTTOM_MID,0,0);

    // Set white border and background style
    lv_obj_set_style_border_color(horiz_rect, lv_palette_main(LV_PALETTE_GREY), 0); // Inner color
    lv_obj_set_style_border_color(horiz_rect, lv_color_white(), 0); // White border
    lv_obj_set_style_border_width(horiz_rect, 2, 0);
    lv_obj_set_style_radius(horiz_rect, 0, 0); // Square corners

    // Define 9 sections (e.g., 9 columns)
    static int32_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), 
                                LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), 
                                LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), 
                                LV_GRID_TEMPLATE_LAST};
    static int32_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(horiz_rect, col_dsc, row_dsc);

    // Optionally, fill with items
    for(int i = 0; i < 9; i++) {
        lv_obj_t * label = lv_label_create(horiz_rect);
        lv_label_set_text_fmt(label, "%d", i+1);
        lv_obj_set_grid_cell(label, LV_GRID_ALIGN_CENTER, i, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    }

    // create 9 section vertical rectangle elevator trim
    // Create the vertical container
    lv_obj_t * vert_rect = lv_obj_create(lv_scr_act());
    lv_obj_set_size(vert_rect, 50, 150); // Set narrow width, taller height
    //lv_obj_center(vert_rect);
    lv_obj_align(vert_rect,LV_ALIGN_BOTTOM_MID,0,0);
    
    // Remove padding and styling from main container to make children fit perfectly
    lv_obj_set_style_pad_all(vert_rect, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(vert_rect, 0, LV_PART_MAIN); // No gap between sections
    lv_obj_set_style_pad_row(vert_rect, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(vert_rect, 0, LV_PART_MAIN);

    // --- Add White Border --- 
    lv_obj_set_style_border_color(vert_rect, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(vert_rect, 2, LV_PART_MAIN);
    lv_obj_set_style_border_opa(vert_rect, LV_OPA_COVER, LV_PART_MAIN);

    // Use Flex Layout to stack children vertically 
    lv_obj_set_flex_flow(vert_rect, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(vert_rect, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // --- Create 9 Sections --- 
    for(int i = 0; i < 9; i++) {
        lv_obj_t * vert_section = lv_obj_create(vert_rect);
        
        /* Each section takes 1/9th height (minus border spacing if needed) */
        // lv_obj_set_size(section, LV_PCT(100), LV_PCT(11)); // Approx 9 sections
        // Alternative: precise calculation
        lv_obj_set_size(vert_section, lv_obj_get_content_width(vert_rect), lv_obj_get_content_height(vert_rect) / 9);
        
        lv_obj_set_style_radius(vert_section, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(vert_section, 0, LV_PART_MAIN); // No border on inner sections

        /* Example: Color the sections differently */
        if(i % 2 == 0)
            lv_obj_set_style_bg_color(vert_section, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        else
            lv_obj_set_style_bg_color(vert_section, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
    }

    lv_timer_create(trim_anim_timer_cb, 500, NULL);
}
