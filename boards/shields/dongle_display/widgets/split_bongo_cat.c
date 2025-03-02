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
 
 LV_IMG_DECLARE(bongo_cat_none);
 LV_IMG_DECLARE(bongo_cat_left1);
 LV_IMG_DECLARE(bongo_cat_left2);
 LV_IMG_DECLARE(bongo_cat_right1);
 LV_IMG_DECLARE(bongo_cat_right2);
 LV_IMG_DECLARE(bongo_cat_both1);
 LV_IMG_DECLARE(bongo_cat_both1_open);
 LV_IMG_DECLARE(bongo_cat_both2);
 
 // Split board positions
 #define SPLIT_LEFT 0
 #define SPLIT_RIGHT 1
 
 struct split_cat_animation_state {
     // Which hand is currently active
     uint8_t active_hand;
     // Animation state for each hand
     uint8_t left_hand_state;
     uint8_t right_hand_state;
     // Timestamp for animation control
     int64_t left_timestamp;
     int64_t right_timestamp;
     // Idle timer
     int64_t last_activity;
 };
 
 // Global animation state
 static struct split_cat_animation_state animation_state = {
     .active_hand = 0,
     .left_hand_state = 0,
     .right_hand_state = 0,
     .left_timestamp = 0,
     .right_timestamp = 0,
     .last_activity = 0
 };
 
 // Animation duration in milliseconds
 #define ANIMATION_DURATION 200
 // Idle timeout in milliseconds (3 seconds)
 #define IDLE_TIMEOUT 3000
 
 // Animation frames for each state
 const lv_img_dsc_t *idle_imgs[] = {
     &bongo_cat_both1,
     &bongo_cat_both1_open,
 };
 
 const lv_img_dsc_t *left_imgs[] = {
     &bongo_cat_left1,
     &bongo_cat_left2,
 };
 
 const lv_img_dsc_t *right_imgs[] = {
     &bongo_cat_right1,
     &bongo_cat_right2,
 };
 
 const lv_img_dsc_t *both_imgs[] = {
     &bongo_cat_both1,
     &bongo_cat_both2,
 };
 
 static void update_animation(lv_obj_t *anim_img) {
     int64_t now = k_uptime_get();
     
     // Check if we need to return to idle state
     if ((now - animation_state.last_activity) > IDLE_TIMEOUT) {
         // Set to idle animation
         lv_img_set_src(anim_img, (now / 1000) % 2 == 0 ? idle_imgs[0] : idle_imgs[1]);
         return;
     }
     
     // Determine which frame to show based on animation states
     if (animation_state.left_hand_state > 0 && animation_state.right_hand_state > 0) {
         // Both hands active
         lv_img_set_src(anim_img, animation_state.left_hand_state == 2 ? both_imgs[1] : both_imgs[0]);
     } else if (animation_state.left_hand_state > 0) {
         // Only left hand active
         lv_img_set_src(anim_img, animation_state.left_hand_state == 2 ? left_imgs[1] : left_imgs[0]);
     } else if (animation_state.right_hand_state > 0) {
         // Only right hand active
         lv_img_set_src(anim_img, animation_state.right_hand_state == 2 ? right_imgs[1] : right_imgs[0]);
     } else {
         // Default state when no animation is running
         lv_img_set_src(anim_img, idle_imgs[0]);
     }
     
     // Check if we need to advance animation frames
     if (animation_state.left_hand_state > 0) {
         if ((now - animation_state.left_timestamp) > ANIMATION_DURATION) {
             animation_state.left_hand_state = 
                 animation_state.left_hand_state == 1 ? 2 : 
                 animation_state.left_hand_state == 2 ? 0 : 0;
             animation_state.left_timestamp = now;
         }
     }
     
     if (animation_state.right_hand_state > 0) {
         if ((now - animation_state.right_timestamp) > ANIMATION_DURATION) {
             animation_state.right_hand_state = 
                 animation_state.right_hand_state == 1 ? 2 : 
                 animation_state.right_hand_state == 2 ? 0 : 0;
             animation_state.right_timestamp = now;
         }
     }
 }
 
 struct split_bongo_cat_event_state {
     uint8_t position;
     bool pressed;
     uint8_t source;
 };
 
 static void set_animation_state(lv_obj_t *anim_img, struct split_bongo_cat_event_state state) {
     int64_t now = k_uptime_get();
     animation_state.last_activity = now;
     
     // Update animation state based on which keyboard half was pressed
     if (state.pressed) {
         if (state.source == SPLIT_LEFT) {
             // Left keyboard
             if (animation_state.left_hand_state == 0) {
                 animation_state.left_hand_state = 1;
                 animation_state.left_timestamp = now;
             }
         } else {
             // Right keyboard
             if (animation_state.right_hand_state == 0) {
                 animation_state.right_hand_state = 1;
                 animation_state.right_timestamp = now;
             }
         }
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
     lv_img_set_src(widget->obj, &bongo_cat_both1);
 
     sys_slist_append(&widgets, &widget->node);
 
     // Start the timer for idle animation updates (every 500ms)
     k_timer_start(&split_bongo_cat_timer, K_MSEC(500), K_MSEC(500));
 
     widget_split_bongo_cat_init();
 
     return 0;
 }
 
 lv_obj_t *zmk_widget_split_bongo_cat_obj(struct zmk_widget_split_bongo_cat *widget) {
     return widget->obj;
 }