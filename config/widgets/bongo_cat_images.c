#include <lvgl.h>

// These are placeholder image data structures
// In a real implementation, you would need to convert actual images to C arrays

// Idle state image data
const uint8_t bongo_idle_map[] = {
    // This would be your actual image data
    // For now, just a placeholder
};

const lv_img_dsc_t bongo_idle = {
    .header.always_zero = 0,
    .header.w = 64,
    .header.h = 32,
    .data_size = sizeof(bongo_idle_map),
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .data = bongo_idle_map,
};

// Tap state image data
const uint8_t bongo_tap_map[] = {
    // This would be your actual image data
    // For now, just a placeholder
};

const lv_img_dsc_t bongo_tap = {
    .header.always_zero = 0,
    .header.w = 64,
    .header.h = 32,
    .data_size = sizeof(bongo_tap_map),
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .data = bongo_tap_map,
};

// Down state image data
const uint8_t bongo_down_map[] = {
    // This would be your actual image data
    // For now, just a placeholder
};

const lv_img_dsc_t bongo_down = {
    .header.always_zero = 0,
    .header.w = 64,
    .header.h = 32,
    .data_size = sizeof(bongo_down_map),
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .data = bongo_down_map,
};
