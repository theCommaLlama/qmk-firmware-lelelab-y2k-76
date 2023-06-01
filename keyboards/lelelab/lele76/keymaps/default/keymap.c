#include QMK_KEYBOARD_H
#include "quantum.h"
#include "matrix.h"
#include "screen_app.h"
#include "apm.h"
#include "cube.h"
#include "eeprom_24c512A.h"


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_ESC,KC_GRAVE,KC_1,KC_2,KC_3,KC_4,KC_5,KC_6,KC_7,KC_8,KC_9,KC_0,KC_MINUS,KC_EQUAL,KC_BSPACE,KC_PGUP,
        KC_ESC,KC_TAB,KC_Q,KC_W,KC_E,KC_R,KC_T,KC_Y,KC_U,KC_I,KC_O,KC_P,KC_LBRC,KC_RBRC,KC_BSLASH,KC_PGDOWN,
        KC_ESC,KC_CAPSLOCK,KC_A,KC_S,KC_D,KC_F,KC_G,KC_H,KC_J,KC_K,KC_L,KC_SCLN,KC_QUOT,KC_ENT,KC_NO,KC_HOME,
        KC_ESC,KC_LSHIFT,KC_NO,KC_Z,KC_X,KC_C,KC_V,KC_B,KC_N,KC_M,KC_COMMA,KC_DOT,KC_SLASH,KC_RSHIFT,KC_UP,KC_END,
        KC_ESC,KC_F5,KC_LCTL,KC_LALT,KC_LCMD,KC_SPACE,KC_RCMD,KC_RCTL,KC_NO,KC_LEFT,KC_DOWN,KC_RIGHT
    )
};

#ifdef ENCODER_MAP_ENABLE
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [0] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [1] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [2] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [3] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
};
#endif


oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_90;
}

bool oled_task_user(void) {
    app_draw();
    return true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) update_apm();

    return app_on_key(keycode, record);
}

void matrix_scan_user(void) {
    decay_apm();
}

void keyboard_post_init_user(void) {
    app_init();
    ResizeCube(10);
}

bool encoder_update_user(uint8_t index, bool clockwise) {
#ifdef ENCODER_MAP_ENABLE
    return app_on_rotate(clockwise);

#else
    if (index == 0) {
        if (!app_on_rotate(clockwise)) return false;
        
        uint16_t kc = clockwise ? 6 : 5;
        kc = dynamic_keymap_get_keycode(0,4,kc);
        tap_code(kc);
    }

    return true;
#endif
}

