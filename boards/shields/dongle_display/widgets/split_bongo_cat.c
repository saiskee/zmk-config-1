/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

 #include <zephyr/kernel.h>
 #include <zephyr/bluetooth/services/bas.h>
 
 #include <zephyr/logging/log.h>
 LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
 
 #include <zmk/display.h>
 #include <zmk/event_manager.h>
 #include <zmk/events/position_state_changed.h>
 #include <zmk/events/keycode_state_changed.h>
 #include <zmk/split/bluetooth/central.h>
 
 #include "split_bongo_cat.h"
 
 #define SRC(array) (const void **)array, sizeof(array) / sizeof(lv_img_dsc_t *)
 
 static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
 
 LV_IMG_DECLARE(bongo_cat_none); // both hands in the air, cheery
 LV_IMG_DECLARE(bongo_cat_left1); // left hand only on table
 LV_IMG_DECLARE(bongo_cat_left2); // left hand only on table, hitting
 LV_IMG_DECLARE(bongo_cat_right1); // right hand only on table
 LV_IMG_DECLARE(bongo_cat_right2); // right hand only on table, hitting
 LV_IMG_DECLARE(bongo_cat_both1); // both hands resting on table, cheery
 LV_IMG_DECLARE(bongo_cat_both1_open); // both hands resting on table, plain face
 LV_IMG_DECLARE(bongo_cat_both2); // both hands hitting table
 
 // Split board positions
 #define SPLIT_LEFT 0
 #define SPLIT_RIGHT 1
 
 // Animation states
 #define STATE_IDLE 0
 #define STATE_ACTIVE 1
 #define STATE_LEFT_ACTIVE 2
 #define STATE_RIGHT_ACTIVE 3
 #define STATE_BOTH_ACTIVE 4
 
 struct split_cat_animation_state {
     // Current animation state
     uint8_t state;
     // Track activity from each side
     bool left_active;
     bool right_active;
     // Timestamp for cooldown timer
     int64_t last_activity;
 };
 
 // Global animation state
 static struct split_cat_animation_state animation_state = {
     .state = STATE_IDLE,
     .left_active = false,
     .right_active = false,
     .last_activity = 0
 };
 
 // Cooldown timeout in milliseconds (5 seconds)
 #define COOLDOWN_TIMEOUT 5000
 
 static void update_animation(lv_obj_t *anim_img) {
     int64_t now = k_uptime_get();
     
     // Check if we need to transition from active to idle
     if (animation_state.state == STATE_ACTIVE) {
         if ((now - animation_state.last_activity) > COOLDOWN_TIMEOUT) {
             // Transition to idle state after cooldown
             animation_state.state = STATE_IDLE;
         }
     }
 
     // Set the appropriate image based on current state
     switch (animation_state.state) {
         case STATE_IDLE:
             lv_img_set_src(anim_img, &bongo_cat_none);
             break;
         case STATE_ACTIVE:
             lv_img_set_src(anim_img, &bongo_cat_both1);
             break;
         case STATE_LEFT_ACTIVE:
             lv_img_set_src(anim_img, &bongo_cat_left2);
             break;
         case STATE_RIGHT_ACTIVE:
             lv_img_set_src(anim_img, &bongo_cat_right2);
             break;
         case STATE_BOTH_ACTIVE:
             lv_img_set_src(anim_img, &bongo_cat_both2);
             break;
         default:
             lv_img_set_src(anim_img, &bongo_cat_both1_open);
     }
 }
 
 struct split_bongo_cat_event_state {
     uint8_t position;
     bool pressed;
     uint8_t source;
 };
 
 static void set_animation_state(lv_obj_t *anim_img, struct split_bongo_cat_event_state state) {
     int64_t now = k_uptime_get();
     
     // Only process key presses, not releases
     if (state.pressed) {
         // Update the active status for each side
         if (state.source == SPLIT_LEFT) {
             animation_state.left_active = true;
         } else {
             animation_state.right_active = true;
         }
         
         // Determine the current animation state
         if (animation_state.left_active && animation_state.right_active) {
             animation_state.state = STATE_BOTH_ACTIVE;
         } else if (animation_state.left_active) {
             animation_state.state = STATE_LEFT_ACTIVE;
         } else if (animation_state.right_active) {
             animation_state.state = STATE_RIGHT_ACTIVE;
         }
         
         // Update the timestamp for activity
         animation_state.last_activity = now;
     } else {
         // Key release event
         if (state.source == SPLIT_LEFT) {
             animation_state.left_active = false;
         } else {
             animation_state.right_active = false;
         }
         
         // If both sides are inactive, transition to active cooldown state
         if (!animation_state.left_active && !animation_state.right_active) {
             animation_state.state = STATE_ACTIVE;
         }
         // Otherwise update the current state for the one that's still active
         else if (animation_state.left_active) {
             animation_state.state = STATE_LEFT_ACTIVE;
         } else if (animation_state.right_active) {
             animation_state.state = STATE_RIGHT_ACTIVE;
         }
         
         // Update the timestamp
         animation_state.last_activity = now;
     }
     
     // Update the image based on current state
     update_animation(anim_img);
 }
 
 static struct split_bongo_cat_event_state split_bongo_cat_get_state(const zmk_event_t *eh) {
     const struct zmk_position_state_changed *position_event = as_zmk_position_state_changed(eh);
     
     if (position_event != NULL) {
         uint8_t source = SPLIT_LEFT; // Default to left keyboard
         
         // Determine if this is from left or right keyboard
         // The position can indicate which half it came from
         if (position_event->position >= (6*3)) { // Assuming 6 columns and 3 rows for left side
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
         update_animation(widget->obj);
     }
 }
 
 K_WORK_DEFINE(split_bongo_cat_idle_work, split_bongo_cat_idle_work_callback);
 
 // Timer for animation updates
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
 
     // Start the timer for animation updates (every 100ms for smoother transitions)
     k_timer_start(&split_bongo_cat_timer, K_MSEC(100), K_MSEC(100));
 
     widget_split_bongo_cat_init();
 
     return 0;
 }
 
 lv_obj_t *zmk_widget_split_bongo_cat_obj(struct zmk_widget_split_bongo_cat *widget) {
     return widget->obj;
 }