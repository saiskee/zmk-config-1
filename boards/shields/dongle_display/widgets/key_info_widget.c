#include <zmk/display/widgets/output_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/wpm_status.h>
#include <zmk/display/status_screen.h>

#include <zmk/display.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/keys.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>
#include "key_info_widget.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// Structure to hold comprehensive key information
struct key_info {
    // Position information
    uint8_t row;
    uint8_t column;
    uint8_t side; // 0 for left, 1 for right
    uint32_t position; // Raw position value

    // Key state
    bool pressed;

    // Keycode information
    uint16_t keycode;
    char keycode_str[32];

    // Layer information
    uint8_t active_layer;

    // Modifier state
    uint8_t modifiers;

    // Timing information
    int64_t press_time;
    int64_t release_time;
    int32_t hold_time_ms;

    // Combo information
    bool is_combo;
    uint8_t combo_keys;
};

static struct key_info current_key = {0};
static lv_obj_t *key_info_container;
static lv_obj_t *position_label;
static lv_obj_t *keycode_label;
static lv_obj_t *layer_label;
static lv_obj_t *timing_label;
static lv_obj_t *modifier_label;

// Helper function to convert keycode to string
static void keycode_to_str(uint16_t keycode, char *buf, size_t buf_size) {
    // Handle common keycodes
    switch (keycode) {
        case KC_A: strncpy(buf, "A", buf_size); break;
        case KC_B: strncpy(buf, "B", buf_size); break;
        case KC_C: strncpy(buf, "C", buf_size); break;
        case KC_D: strncpy(buf, "D", buf_size); break;
        case KC_E: strncpy(buf, "E", buf_size); break;
        case KC_F: strncpy(buf, "F", buf_size); break;
        case KC_G: strncpy(buf, "G", buf_size); break;
        case KC_H: strncpy(buf, "H", buf_size); break;
        case KC_I: strncpy(buf, "I", buf_size); break;
        case KC_J: strncpy(buf, "J", buf_size); break;
        case KC_K: strncpy(buf, "K", buf_size); break;
        case KC_L: strncpy(buf, "L", buf_size); break;
        case KC_M: strncpy(buf, "M", buf_size); break;
        case KC_N: strncpy(buf, "N", buf_size); break;
        case KC_O: strncpy(buf, "O", buf_size); break;
        case KC_P: strncpy(buf, "P", buf_size); break;
        case KC_Q: strncpy(buf, "Q", buf_size); break;
        case KC_R: strncpy(buf, "R", buf_size); break;
        case KC_S: strncpy(buf, "S", buf_size); break;
        case KC_T: strncpy(buf, "T", buf_size); break;
        case KC_U: strncpy(buf, "U", buf_size); break;
        case KC_V: strncpy(buf, "V", buf_size); break;
        case KC_W: strncpy(buf, "W", buf_size); break;
        case KC_X: strncpy(buf, "X", buf_size); break;
        case KC_Y: strncpy(buf, "Y", buf_size); break;
        case KC_Z: strncpy(buf, "Z", buf_size); break;
        case KC_0: strncpy(buf, "0", buf_size); break;
        case KC_1: strncpy(buf, "1", buf_size); break;
        case KC_2: strncpy(buf, "2", buf_size); break;
        case KC_3: strncpy(buf, "3", buf_size); break;
        case KC_4: strncpy(buf, "4", buf_size); break;
        case KC_5: strncpy(buf, "5", buf_size); break;
        case KC_6: strncpy(buf, "6", buf_size); break;
        case KC_7: strncpy(buf, "7", buf_size); break;
        case KC_8: strncpy(buf, "8", buf_size); break;
        case KC_9: strncpy(buf, "9", buf_size); break;
        case KC_SPACE: strncpy(buf, "SPACE", buf_size); break;
        case KC_ENTER: strncpy(buf, "ENTER", buf_size); break;
        case KC_ESCAPE: strncpy(buf, "ESC", buf_size); break;
        case KC_BSPC: strncpy(buf, "BKSP", buf_size); break;
        case KC_TAB: strncpy(buf, "TAB", buf_size); break;
        case KC_MINUS: strncpy(buf, "-", buf_size); break;
        case KC_EQUAL: strncpy(buf, "=", buf_size); break;
        case KC_LBRC: strncpy(buf, "[", buf_size); break;
        case KC_RBRC: strncpy(buf, "]", buf_size); break;
        case KC_BSLH: strncpy(buf, "\\", buf_size); break;
        case KC_SCLN: strncpy(buf, ";", buf_size); break;
        case KC_QUOT: strncpy(buf, "'", buf_size); break;
        case KC_GRAVE: strncpy(buf, "`", buf_size); break;
        case KC_COMMA: strncpy(buf, ",", buf_size); break;
        case KC_DOT: strncpy(buf, ".", buf_size); break;
        case KC_SLASH: strncpy(buf, "/", buf_size); break;
        case KC_LSHIFT: strncpy(buf, "LSHIFT", buf_size); break;
        case KC_RSHIFT: strncpy(buf, "RSHIFT", buf_size); break;
        case KC_LCTRL: strncpy(buf, "LCTRL", buf_size); break;
        case KC_RCTRL: strncpy(buf, "RCTRL", buf_size); break;
        case KC_LALT: strncpy(buf, "LALT", buf_size); break;
        case KC_RALT: strncpy(buf, "RALT", buf_size); break;
        case KC_LGUI: strncpy(buf, "LGUI", buf_size); break;
        case KC_RGUI: strncpy(buf, "RGUI", buf_size); break;
        default: snprintf(buf, buf_size, "KC_0x%04X", keycode);
    }
}

// Helper function to get modifier string
static void get_modifier_str(uint8_t modifiers, char *buf, size_t buf_size) {
    buf[0] = '\0';

    if (modifiers & MOD_LCTL) strncat(buf, "LCTL ", buf_size);
    if (modifiers & MOD_LSFT) strncat(buf, "LSFT ", buf_size);
    if (modifiers & MOD_LALT) strncat(buf, "LALT ", buf_size);
    if (modifiers & MOD_LGUI) strncat(buf, "LGUI ", buf_size);
    if (modifiers & MOD_RCTL) strncat(buf, "RCTL ", buf_size);
    if (modifiers & MOD_RSFT) strncat(buf, "RSFT ", buf_size);
    if (modifiers & MOD_RALT) strncat(buf, "RALT ", buf_size);
    if (modifiers & MOD_RGUI) strncat(buf, "RGUI ", buf_size);

    // Remove trailing space if any modifiers were added
    size_t len = strlen(buf);
    if (len > 0) buf[len - 1] = '\0';

    // If no modifiers, show "None"
    if (buf[0] == '\0') strncpy(buf, "None", buf_size);
}

// Calculate position based on keyboard layout
static void calculate_position(uint32_t position, struct key_info *info) {
    // For Corne keyboard with 3x6 matrix per side
    // Adjust these calculations based on your specific keyboard layout

    // Store raw position
    info->position = position;

    // Determine which side the key is on (left or right)
    if (position < 36) {
        // Left side
        info->side = 0;

        // Calculate row and column
        info->row = position / 6;
        info->column = position % 6;
    } else {
        // Right side
        info->side = 1;

        // Adjust position for right side calculation
        uint32_t adjusted_pos = position - 36;

        // Calculate row and column
        info->row = adjusted_pos / 6;
        info->column = adjusted_pos % 6;
    }
}

// Update all labels with current key information
static void update_key_info_labels() {
    char buf[64];

    // Update position label
    snprintf(buf, sizeof(buf), "Pos: %d,%d,%d (#%d)",
             current_key.row, current_key.column, current_key.side, current_key.position);
    lv_label_set_text(position_label, buf);

    // Update keycode label
    if (current_key.pressed) {
        snprintf(buf, sizeof(buf), "Key: %s (0x%04X)",
                 current_key.keycode_str, current_key.keycode);
    } else {
        snprintf(buf, sizeof(buf), "Key: ---");
    }
    lv_label_set_text(keycode_label, buf);

    // Update layer label
    snprintf(buf, sizeof(buf), "Layer: %d", current_key.active_layer);
    lv_label_set_text(layer_label, buf);

    // Update timing label
    if (current_key.pressed) {
        snprintf(buf, sizeof(buf), "Hold: %d ms", current_key.hold_time_ms);
    } else if (current_key.release_time > 0) {
        snprintf(buf, sizeof(buf), "Last: %d ms", current_key.hold_time_ms);
    } else {
        snprintf(buf, sizeof(buf), "Time: ---");
    }
    lv_label_set_text(timing_label, buf);

    // Update modifier label
    char mod_str[32];
    get_modifier_str(current_key.modifiers, mod_str, sizeof(mod_str));
    snprintf(buf, sizeof(buf), "Mods: %s", mod_str);
    lv_label_set_text(modifier_label, buf);
}

// Work callback for updating the display
static void key_info_update_cb(struct k_work *work) {
    // Update hold time if key is pressed
    if (current_key.pressed) {
        int64_t now = k_uptime_get();
        current_key.hold_time_ms = (now - current_key.press_time) / 1000;
    }

    update_key_info_labels();
}

K_WORK_DEFINE(key_info_work, key_info_update_cb);

// Timer for updating hold time while key is pressed
static void key_info_timer_cb(struct k_timer *timer) {
    if (current_key.pressed) {
        k_work_submit(&key_info_work);
    }
}

K_TIMER_DEFINE(key_info_timer, key_info_timer_cb, NULL);

// Position state changed listener
static int position_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    calculate_position(ev->position, &current_key);
    current_key.pressed = ev->state;

    if (ev->state) {
        // Key pressed
        current_key.press_time = k_uptime_get();
        current_key.release_time = 0;
        current_key.hold_time_ms = 0;

        // Start timer to update hold time
        k_timer_start(&key_info_timer, K_MSEC(100), K_MSEC(100));
    } else {
        // Key released
        current_key.release_time = k_uptime_get();
        current_key.hold_time_ms = (current_key.release_time - current_key.press_time) / 1000;

        // Stop timer
        k_timer_stop(&key_info_timer);
    }

    k_work_submit(&key_info_work);
    return ZMK_EV_EVENT_BUBBLE;
}

// Keycode state changed listener
static int keycode_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->state) {
        // Key pressed
        current_key.keycode = ev->keycode;
        keycode_to_str(ev->keycode, current_key.keycode_str, sizeof(current_key.keycode_str));
    }

    k_work_submit(&key_info_work);
    return ZMK_EV_EVENT_BUBBLE;
}

// Layer state changed listener
static int layer_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // Update active layer
    if (ev->state) {
        current_key.active_layer = ev->layer;
    }

    k_work_submit(&key_info_work);
    return ZMK_EV_EVENT_BUBBLE;
}

// Modifiers state changed listener
static int modifiers_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_modifiers_state_changed *ev = as_zmk_modifiers_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // Update modifiers
    current_key.modifiers = ev->modifiers;

    k_work_submit(&key_info_work);
    return ZMK_EV_EVENT_BUBBLE;
}

// Activity state changed listener
static int activity_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    k_work_submit(&key_info_work);
    return ZMK_EV_EVENT_BUBBLE;
}

// Register listeners
ZMK_LISTENER(key_info_position_widget, position_state_changed_listener);
ZMK_SUBSCRIPTION(key_info_position_widget, zmk_position_state_changed);

ZMK_LISTENER(key_info_keycode_widget, keycode_state_changed_listener);
ZMK_SUBSCRIPTION(key_info_keycode_widget, zmk_keycode_state_changed);

ZMK_LISTENER(key_info_layer_widget, layer_state_changed_listener);
ZMK_SUBSCRIPTION(key_info_layer_widget, zmk_layer_state_changed);

ZMK_LISTENER(key_info_modifiers_widget, modifiers_state_changed_listener);
ZMK_SUBSCRIPTION(key_info_modifiers_widget, zmk_modifiers_state_changed);

ZMK_LISTENER(key_info_activity_widget, activity_state_changed_listener);
ZMK_SUBSCRIPTION(key_info_activity_widget, zmk_activity_state_changed);

// Initialize the widget
int key_info_widget_init(lv_obj_t *parent) {
    key_info_container = lv_obj_create(parent, NULL);
    lv_obj_set_size(key_info_container, LV_HOR_RES, LV_VER_RES - 30);
    lv_obj_align(key_info_container, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
    lv_obj_set_style_local_bg_opa(key_info_container, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_opa(key_info_container, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);

    // Create labels for different information
    position_label = lv_label_create(key_info_container, NULL);
    lv_label_set_text(position_label, "Pos: ---");
    lv_obj_align(position_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);

    keycode_label = lv_label_create(key_info_container, NULL);
    lv_label_set_text(keycode_label, "Key: ---");
    lv_obj_align(keycode_label, position_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    layer_label = lv_label_create(key_info_container, NULL);
    lv_label_set_text(layer_label, "Layer: 0");
    lv_obj_align(layer_label, keycode_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    timing_label = lv_label_create(key_info_container, NULL);
    lv_label_set_text(timing_label, "Time: ---");
    lv_obj_align(timing_label, layer_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    modifier_label = lv_label_create(key_info_container, NULL);
    lv_label_set_text(modifier_label, "Mods: None");
    lv_obj_align(modifier_label, timing_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    return 0;
}
