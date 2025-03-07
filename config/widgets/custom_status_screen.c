#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/output_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/wpm_status.h>
#include "bongo_cat_position.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>

static lv_obj_t *screen;
static lv_style_t global_style;

static lv_obj_t *center_frame;
static lv_obj_t *status_bar;

int zmk_display_status_screen_init() {
    screen = lv_obj_create(NULL, NULL);

    lv_style_init(&global_style);
    lv_style_set_text_font(&global_style, LV_STATE_DEFAULT, &lv_font_montserrat_16);
    lv_style_set_text_letter_space(&global_style, LV_STATE_DEFAULT, 1);
    lv_style_set_text_line_space(&global_style, LV_STATE_DEFAULT, 1);
    lv_obj_add_style(screen, LV_OBJ_PART_MAIN, &global_style);

    // Create a status bar at the top
    status_bar = lv_obj_create(screen, NULL);
    lv_obj_set_size(status_bar, LV_HOR_RES, 30);
    lv_obj_align(status_bar, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
    lv_obj_set_style_local_radius(status_bar, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_bg_color(status_bar, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_set_style_local_pad_all(status_bar, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 4);

    // Add status widgets to the status bar
    zmk_widget_output_status_init(status_bar);
    zmk_widget_battery_status_init(status_bar);
    zmk_widget_layer_status_init(status_bar);

    // Position the status widgets
    lv_obj_align(zmk_widget_output_status_get_obj(), NULL, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(zmk_widget_battery_status_get_obj(), NULL, LV_ALIGN_IN_RIGHT_MID, 0, 0);
    lv_obj_align(zmk_widget_layer_status_get_obj(), NULL, LV_ALIGN_CENTER, 0, 0);

    // Create a frame for the bongo cat
    center_frame = lv_obj_create(screen, NULL);
    lv_obj_set_size(center_frame, LV_HOR_RES, LV_VER_RES - 30);
    lv_obj_align(center_frame, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_local_bg_color(center_frame, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_set_style_local_radius(center_frame, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_border_width(center_frame, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);

    // Initialize the bongo cat widget
    bongo_cat_position_init(center_frame);

    return 0;
}

lv_obj_t *zmk_display_status_screen() { return screen; }
