/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/logging/log.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/split/bluetooth/central.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/events/mouse_button_state_changed.h>
#include <zmk/events/mouse_move_state_changed.h>
#include <zmk/events/mouse_scroll_state_changed.h>
#include <lvgl.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define SRC(array) (const void **)array, sizeof(array) / sizeof(lv_img_dsc_t *)

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// Include the bongo cat image declarations
extern const lv_img_dsc_t bongo_cat_none;
extern const lv_img_dsc_t bongo_cat_left1;
extern const lv_img_dsc_t bongo_cat_left2;
extern const lv_img_dsc_t bongo_cat_right1;
extern const lv_img_dsc_t bongo_cat_right2;
extern const lv_img_dsc_t bongo_cat_both1;
extern const lv_img_dsc_t bongo_cat_both1_open;
extern const lv_img_dsc_t bongo_cat_both2;

// Animation state
enum bongo_cat_state {
    STATE_IDLE_OPEN,  // Initial state, mouth open
    STATE_IDLE,       // Waiting for next input
    STATE_LEFT_TAP,   // Left side tap
    STATE_RIGHT_TAP,  // Right side tap
    STATE_BOTH_TAP    // Both sides tap
};

// Animation timing
#define ANIMATION_DURATION 150
#define IDLE_TIMEOUT 5000  // Time to return to open mouth idle state

struct split_cat_animation_state {
    // Current animation state
    enum bongo_cat_state current_state;
    // Timestamp for animation control
    int64_t last_activity;
    // Flag to track if typing has started
    bool typing_started;
};

// Global animation state
static struct split_cat_animation_state animation_state = {
    .current_state = STATE_IDLE_OPEN,
    .last_activity = 0,
    .typing_started = false
};

static lv_obj_t *bongo_cat_img;

// Forward declarations
static void update_animation(lv_obj_t *anim_img);
static void set_animation_state(lv_obj_t *anim_img, struct split_bongo_cat_event_state state);

// Update the displayed image based on current state
static void update_animation(lv_obj_t *anim_img) {
    // Set the appropriate image based on current state
    switch (animation_state.current_state) {
        case STATE_IDLE_OPEN:
            lv_img_set_src(anim_img, &bongo_cat_both1_open);
            break;
        case STATE_IDLE:
            lv_img_set_src(anim_img, &bongo_cat_none);
            break;
        case STATE_LEFT_TAP:
            lv_img_set_src(anim_img, &bongo_cat_left2);
            break;
        case STATE_RIGHT_TAP:
            lv_img_set_src(anim_img, &bongo_cat_right2);
            break;
        case STATE_BOTH_TAP:
            lv_img_set_src(anim_img, &bongo_cat_both2);
            break;
        default:
            lv_img_set_src(anim_img, &bongo_cat_none);
            break;
    }
}

// Set the animation state based on keyboard events
static void set_animation_state(lv_obj_t *anim_img, struct split_bongo_cat_event_state state) {
    int64_t now = k_uptime_get();
    animation_state.last_activity = now;
    
    // If this is the first activity, transition from idle open to idle
    if (animation_state.current_state == STATE_IDLE_OPEN && !animation_state.typing_started) {
        animation_state.typing_started = true;
        animation_state.current_state = STATE_IDLE;
        update_animation(anim_img);
    }
    
    // Update animation state based on which keyboard half was pressed
    if (state.pressed) {
        if (state.source == SPLIT_LEFT) {
            // Left keyboard
            animation_state.current_state = STATE_LEFT_TAP;
        } else {
            // Right keyboard
            animation_state.current_state = STATE_RIGHT_TAP;
        }
        
        // Update the image based on current state
        update_animation(anim_img);
    }
}

struct split_bongo_cat_event_state {
    uint8_t position;
    bool pressed;
    uint8_t source;
};

// Get the event state from ZMK events
static struct split_bongo_cat_event_state split_bongo_cat_get_state(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *position_event = as_zmk_position_state_changed(eh);
    
    if (position_event != NULL) {
        uint8_t source = SPLIT_LEFT; // Default to left keyboard
        
        // Determine if this is from left or right keyboard
        // The position can indicate which half it came from
        if (position_event->position >= 40) { // Adjust based on your keyboard layout
            source = SPLIT_RIGHT;
        }
        
        return (struct split_bongo_cat_event_state) {
            .position = position_event->position,
            .pressed = position_event->state > 0,
            .source = source
        };
    }
    
    // Default return for other event types
    return (struct split_bongo_cat_event_state) {
        .position = 0,
        .pressed = false,
        .source = SPLIT_LEFT
    };
}

// Callback for widget updates
void split_bongo_cat_update_cb(struct split_bongo_cat_event_state state) {
    struct zmk_widget_split_bongo_cat *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_animation_state(widget->obj, state);
    }
}

// Idle animation update function
static void split_bongo_cat_idle_work_callback(struct k_work *work) {
    struct zmk_widget_split_bongo_cat *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        int64_t now = k_uptime_get();
        
        // Check if we need to return to idle state after animation
        if (animation_state.current_state == STATE_LEFT_TAP || 
            animation_state.current_state == STATE_RIGHT_TAP || 
            animation_state.current_state == STATE_BOTH_TAP) {
            animation_state.current_state = STATE_IDLE;
            update_animation(widget->obj);
        }
        
        // Check if we need to return to idle open state after timeout
        if (animation_state.typing_started && 
            (now - animation_state.last_activity) > IDLE_TIMEOUT) {
            animation_state.current_state = STATE_IDLE_OPEN;
            animation_state.typing_started = false;
            update_animation(widget->obj);
        }
    }
}

K_WORK_DEFINE(split_bongo_cat_idle_work, split_bongo_cat_idle_work_callback);

// Timer for idle animation updates
static void split_bongo_cat_timer_callback(struct k_timer *timer) {
    k_work_submit(&split_bongo_cat_idle_work);
}

K_TIMER_DEFINE(split_bongo_cat_timer, split_bongo_cat_timer_callback, NULL);

ZMK_DISPLAY_WIDGET_LISTENER(widget_split_bongo_cat, struct split_bongo_cat_event_state,
                             split_bongo_cat_update_cb, split_bongo_cat_get_state)

ZMK_SUBSCRIPTION(widget_split_bongo_cat, zmk_position_state_changed);

int zmk_widget_split_bongo_cat_init(struct zmk_widget_split_bongo_cat *widget, lv_obj_t *parent) {
    widget->obj = lv_img_create(parent);
    lv_obj_center(widget->obj);
    lv_img_set_src(widget->obj, &bongo_cat_both1_open);

    sys_slist_append(&widgets, &widget->node);

    // Start the timer for periodic checks (every 100ms)
    k_timer_start(&split_bongo_cat_timer, K_MSEC(100), K_MSEC(100));

    widget_split_bongo_cat_init();

    return 0;
}

lv_obj_t *zmk_widget_split_bongo_cat_obj(struct zmk_widget_split_bongo_cat *widget) {
    return widget->obj;
}