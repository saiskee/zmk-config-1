#include <zmk/display/widgets/output_status.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/wpm_status.h>
#include <zmk/display/status_screen.h>

#include <zmk/display.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

enum bongo_state {
    BONGO_IDLE,
    BONGO_TAP,
    BONGO_DOWN
};

struct position_state {
    uint8_t row;
    uint8_t column;
    uint8_t side; // 0 for left, 1 for right
    bool pressed;
};

static struct position_state current_position = {0, 0, 0, false};
static enum bongo_state bongo_state = BONGO_IDLE;
static lv_obj_t *bongo_cat_container;
static lv_obj_t *bongo_cat_image;
static lv_obj_t *position_label;

// External image declarations
extern const lv_img_dsc_t bongo_idle;
extern const lv_img_dsc_t bongo_tap;
extern const lv_img_dsc_t bongo_down;

static void set_bongo_image(enum bongo_state state) {
    switch (state) {
    case BONGO_IDLE:
        lv_img_set_src(bongo_cat_image, &bongo_idle);
        break;
    case BONGO_TAP:
        lv_img_set_src(bongo_cat_image, &bongo_tap);
        break;
    case BONGO_DOWN:
        lv_img_set_src(bongo_cat_image, &bongo_down);
        break;
    default:
        break;
    }
}

static void update_position_label() {
    if (current_position.pressed) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Pos: %d,%d,%d",
                 current_position.row,
                 current_position.column,
                 current_position.side);
        lv_label_set_text(position_label, buf);
    } else {
        lv_label_set_text(position_label, "Pos: ---");
    }
}

static void bongo_cat_update_cb(struct k_work *work) {
    set_bongo_image(bongo_state);
    update_position_label();
}

K_WORK_DEFINE(bongo_cat_work, bongo_cat_update_cb);

// Calculate position based on your specific keyboard layout
static void calculate_position(uint32_t position, struct position_state *pos) {
    // For Corne keyboard with 3x6 matrix per side
    // Adjust these calculations based on your specific keyboard layout

    // Determine which side the key is on (left or right)
    if (position < 36) {
        // Left side
        pos->side = 0;

        // Calculate row and column
        pos->row = position / 6;
        pos->column = position % 6;
    } else {
        // Right side
        pos->side = 1;

        // Adjust position for right side calculation
        uint32_t adjusted_pos = position - 36;

        // Calculate row and column
        pos->row = adjusted_pos / 6;
        pos->column = adjusted_pos % 6;
    }
}

static int position_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    calculate_position(ev->position, &current_position);
    current_position.pressed = ev->state;

    if (ev->state) {
        bongo_state = BONGO_DOWN;
    } else {
        bongo_state = BONGO_TAP;
        // Schedule a timer to return to idle state after a short delay
        k_timeout_t delay = K_MSEC(100);
        k_work_schedule(&bongo_cat_work, delay);
    }

    k_work_submit(&bongo_cat_work);
    return ZMK_EV_EVENT_BUBBLE;
}

static int activity_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->state) {
        bongo_state = BONGO_IDLE;
        k_work_submit(&bongo_cat_work);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(bongo_cat_position_widget, position_state_changed_listener);
ZMK_SUBSCRIPTION(bongo_cat_position_widget, zmk_position_state_changed);

ZMK_LISTENER(bongo_cat_activity_widget, activity_state_changed_listener);
ZMK_SUBSCRIPTION(bongo_cat_activity_widget, zmk_activity_state_changed);

int bongo_cat_position_init(lv_obj_t *parent) {
    bongo_cat_container = lv_obj_create(parent, NULL);
    lv_obj_set_size(bongo_cat_container, LV_HOR_RES, LV_VER_RES - 30);
    lv_obj_align(bongo_cat_container, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
    lv_obj_set_style_local_bg_opa(bongo_cat_container, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_opa(bongo_cat_container, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);

    bongo_cat_image = lv_img_create(bongo_cat_container, NULL);
    lv_img_set_src(bongo_cat_image, &bongo_idle);
    lv_obj_align(bongo_cat_image, NULL, LV_ALIGN_CENTER, 0, -10);

    position_label = lv_label_create(bongo_cat_container, NULL);
    lv_label_set_text(position_label, "Pos: ---");
    lv_obj_align(position_label, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -5);

    return 0;
}
