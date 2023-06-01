#include "quantum.h"
#include "screen_app.h"
#include "progmem.h"
#include "apm.h"
#include "aht_sensor.h"
#include "app_eeconfig.h"
#include "cube.h"
#include "tiny_mcu.h"
#include "tiny_mcu_protocol.h"
#include "eeprom.h"
#include <string.h>
#include <stdio.h>


#define I2C_ROM_ADDR_TOMATO_COMPRESSED      0
#define I2C_ROM_ADDR_LelePig                0xa0
#define I2C_ROM_ADDR_bongo_up               0x160
#define I2C_ROM_ADDR_bongo_left             0x200
#define I2C_ROM_ADDR_bongo_right            0x260
#define I2C_ROM_ADDR_bongo_both             0x2a0

#define SCREEN_LINES 8
#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128

typedef enum {
    HOMEART_PIG = 0,
    HOMEART_BONGO,
    HOMEART_CUBE,
    HOMEART_PIG_SPACE,
} homeart_enum_t;

// function prototypes
static void switchApp(void (*draw)(void), bool (*rotate)(bool), bool (*keydown)(void));
static void enableFastRot(uint16_t max_rot_value);
static void _draw_menu(void);
static void _load_menu(const char* const *list, uint8_t cursor, uint8_t length);
static void menu_draw_v_scroll(void);
static bool _menu_on_rot(bool moveDown);
static bool go_rgb_settings(void);
static void drawValueAndBar(uint8_t value, uint16_t max);
static void draw5PxStar(uint8_t x, uint8_t y, uint8_t value);
static uint8_t getLedIdFromKey(uint8_t row, uint8_t col);
static void app_cfg_init(void);
static void app_eeconfig_save(void);


// variables
bool (*f_on_rotate)(bool);
bool (*f_rot_press)(void);
bool (*f_keydown)(uint16_t, keyrecord_t*);
void (*f_draw)(void);

static uint8_t app_autoback_sec;

static uint32_t user_lastact32; // in ms 
static uint8_t is_rgb_idle_off;
static uint8_t is_side_idle_off;

static app_eeconfig_t eecfg;
static uint8_t _key_led_i; // for keystroke related led effect


static char str_buf[23] = {0};
static uint8_t draw_once_flag = 0;
static uint8_t _shared_u8;
static uint16_t _shared_u16;
static uint8_t init_cnt = 1;

#define MENU_NO_INVERT 0
#define MENU_INVERT_CURSOR  1
#define MENU_INVERT_SELECTED  2 // NOT IMPLEMENTED

#define arrLength(arr)  (sizeof(arr)/sizeof(arr[0]))

struct {
    const char * const *list;
    uint8_t cursor;
    uint8_t offset;
    uint8_t length;
    //uint8_t invertOptions; // not implemented
} menuS;

static uint16_t rot_enable_fast = 0; // value as max rot value

// default callback functions
static bool empty_on_rotate(bool moveDown) { 
    return true;
}

// function prototype
static void dashboard_draw(void);
static bool dashboard_on_rot(bool moveDown);
static bool go_settings(void);
static bool go_oled_settings(void);
static void go_oled_off_setting(void);


const char TEXT_AUTO_OFF[] PROGMEM  = "Auto Off";

/// side led effect ///
const char SIDE_LED_EFF_TITLE[] PROGMEM = "-- side LED Mode";
const char SIDE_LED_EFF_MENU_1[] PROGMEM = "Off";
const char SIDE_LED_EFF_MENU_2[] PROGMEM = "Breath";
const char SIDE_LED_EFF_MENU_3[] PROGMEM = "Flash on Type";
const char SIDE_LED_EFF_MENU_4[] PROGMEM = "Static";
const char * const SIDE_LED_EFF_menu_items[] PROGMEM = {
    SIDE_LED_EFF_TITLE,
    SIDE_LED_EFF_MENU_1,
    SIDE_LED_EFF_MENU_2,
    SIDE_LED_EFF_MENU_3,
    SIDE_LED_EFF_MENU_4,
};

static void _side_led_set_eff(void) {
    uint8_t eff = eecfg.side.mode;
    if (eff > SideLed_Mode_solid) return;

    const uint8_t data[] = {CMD_SIDE_LED_EFFECT, eff};
    tiny85_i2c_tx(data, sizeof(data));
}

static bool side_led_effect_rot(bool moveDown) {
    _menu_on_rot(moveDown);
    if (menuS.cursor) {
        eecfg.side.mode = menuS.cursor - 1;
        _side_led_set_eff();
    }
    return false;
}

/// side led brightness
static void side_led_bright_draw(void) {
    oled_write_P(PSTR("Bright\n"), false);
    drawValueAndBar(eecfg.side.bright, MAX_SIDE_LED_BRIGHT);
}

static void _side_led_set_bright(uint8_t value) {
    const uint8_t data[] = {CMD_SIDE_LED_BRIGHT, value};
    tiny85_i2c_tx(data, sizeof(data));
}

static bool side_led_bright_on_rotate(bool moveDown) {
    if (moveDown) {
        if (eecfg.side.bright < MAX_SIDE_LED_BRIGHT) eecfg.side.bright++;
    } else {
        if (eecfg.side.bright > 0) eecfg.side.bright--;
    }
    if (eecfg.side.bright > MAX_SIDE_LED_BRIGHT) eecfg.side.bright = MAX_SIDE_LED_BRIGHT;

    _side_led_set_bright(eecfg.side.bright);
    return false;
}

/// side led speed
static void side_led_speed_draw(void) {
    oled_write_P(PSTR("Speed\n"), false);
    drawValueAndBar(eecfg.side.speed, MAX_SIDE_LED_SPEED);
}

static void _side_led_set_speed(uint8_t value) {
    const uint8_t data[] = {CMD_SIDE_LED_SPEED, value};
    tiny85_i2c_tx(data, sizeof(data));
}

static bool side_led_speed_on_rotate(bool moveDown) {
    if (moveDown) {
        if (eecfg.side.speed < MAX_SIDE_LED_SPEED) eecfg.side.speed ++;
    } else {
        if (eecfg.side.speed > 0) eecfg.side.speed --;
    }
    if (eecfg.side.speed > MAX_SIDE_LED_SPEED) eecfg.side.speed = MAX_SIDE_LED_SPEED;

    _side_led_set_speed(eecfg.side.speed);
    return false;
}


/// home artwork settings
const char ARTWORK_CFG_TITLE[] PROGMEM  = "-- Select Artwork";
const char ARTWORK_CFG_MENU_1[] PROGMEM = "Lele Logo";
const char ARTWORK_CFG_MENU_2[] PROGMEM = "BongoCat";
const char ARTWORK_CFG_MENU_3[] PROGMEM = "Cube";
const char ARTWORK_CFG_MENU_4[] PROGMEM = "Lele in Space";

const char * const artwork_menu_items[] PROGMEM = {
    ARTWORK_CFG_TITLE,
    ARTWORK_CFG_MENU_1,
    ARTWORK_CFG_MENU_2,
    ARTWORK_CFG_MENU_3,
    ARTWORK_CFG_MENU_4,
};

static bool artwork_set_on_press(void) {
    if (menuS.cursor > 0) {
        eecfg.homeart_id = menuS.cursor-1;
        app_eeconfig_save();
    }

    app_init();
    return false;
}

static bool go_artwork_setting(void) {
    _load_menu(
        artwork_menu_items,
        1 + eecfg.homeart_id,
        arrLength(artwork_menu_items)
    );
    switchApp(_draw_menu, _menu_on_rot, artwork_set_on_press);
    return false;
}



/// side led main menu ///
static bool go_side_led_setting(void);
static void go_side_led_auto_off_setting(void);

const char SIDE_LED_TITLE[] PROGMEM  = "-- Side Led --";
const char SIDE_LED_MENU_1[] PROGMEM = "Mode";
const char SIDE_LED_MENU_2[] PROGMEM = "Bright";
const char SIDE_LED_MENU_3[] PROGMEM = "Speed";

const char * const side_menu_items[] PROGMEM = {
    SIDE_LED_TITLE,
    SIDE_LED_MENU_1,
    SIDE_LED_MENU_2,
    SIDE_LED_MENU_3,
    TEXT_AUTO_OFF
};


static bool side_led_on_press(void) {
    if (menuS.list == side_menu_items) {
        switch (menuS.cursor) {
            case 0:
                go_settings();
                break;
            case 1:
                _load_menu(
                    SIDE_LED_EFF_menu_items,
                    1 + eecfg.side.mode,
                    arrLength(SIDE_LED_EFF_menu_items)
                );
                switchApp(_draw_menu, side_led_effect_rot, go_side_led_setting);
                break;
            case 2:
                switchApp(side_led_bright_draw, side_led_bright_on_rotate, go_side_led_setting);
                enableFastRot(10);
                break;
            case 3:
                switchApp(side_led_speed_draw, side_led_speed_on_rotate, go_side_led_setting);
                break;
            case 4:
                go_side_led_auto_off_setting();
                break;
            default:
                break;
        }
    }
    return false;
}

static bool go_side_led_setting(void) {
    _load_menu(
        side_menu_items,
        1,
        arrLength(side_menu_items)
    );
    switchApp(_draw_menu, _menu_on_rot, side_led_on_press);
    return false;
}



// APP: menu help
static void help_draw(void) {
    if (draw_once_flag) {
        oled_write_P(PSTR("-- Help --\n\nrotate: move\npress: select\n\n"), false);
        oled_write_P(PSTR("www.lelelab.work\n"), false);
    }
}


/// led brightness adjust
static void oled_bright_draw(void) {
    if (draw_once_flag) {
        _shared_u8 = oled_get_brightness();
        return;
    }

    oled_write_P(PSTR("OLED Bright\n"), false);
    drawValueAndBar(oled_get_brightness(), 255);
}

static bool oled_bright_on_rotate(bool moveDown) {
    if (moveDown) {
        if (_shared_u8 < 0xff) _shared_u8++;
    } else {
        if (_shared_u8) _shared_u8--;
    }
    eecfg.oled.bright = (uint8_t)_shared_u8;
    oled_set_brightness(eecfg.oled.bright);
    return false;
}

/// oled auto off timer adjust
const char OLED_OFF_TITLE[] PROGMEM  = "-- Auto-off Timer";
const char OLED_OFF_MENU_1[] PROGMEM = "never";
const char OLED_OFF_MENU_2[] PROGMEM = "10s";
const char OLED_OFF_MENU_3[] PROGMEM = "60s";
const char OLED_OFF_MENU_4[] PROGMEM = "300s";
const char OLED_OFF_MENU_5[] PROGMEM = "600s";
const char OLED_OFF_MENU_6[] PROGMEM = "1800s";
const char OLED_OFF_MENU_7[] PROGMEM = "3600s";

const char * const oled_off_menu_items[] PROGMEM = {
    OLED_OFF_TITLE,
    OLED_OFF_MENU_1,
    OLED_OFF_MENU_2,
    OLED_OFF_MENU_3,
    OLED_OFF_MENU_4,
    OLED_OFF_MENU_5,
    OLED_OFF_MENU_6,
    OLED_OFF_MENU_7,
};

const uint16_t AutoOffCfgValues[] PROGMEM = {
    0,10,60,300,600,1800,3600
};
#define AutoOffCfgLength    (sizeof(AutoOffCfgValues)/sizeof(AutoOffCfgValues[0]))

static bool oled_off_on_press(void) {
    if (menuS.cursor > 0) {
        eecfg.oled.autooff = menuS.cursor-1;
    }

    go_oled_settings();
    return false;
}

static void go_oled_off_setting(void) {
    _load_menu(
        oled_off_menu_items,
        1 + eecfg.oled.autooff,
        arrLength(oled_off_menu_items)
    );
    switchApp(_draw_menu, _menu_on_rot, oled_off_on_press);
}

static bool side_led_auto_off_press(void) {
    if (menuS.cursor > 0) {
        eecfg.side.autooff = menuS.cursor-1;
    }
    go_side_led_setting();
    return false;
}

static void go_side_led_auto_off_setting(void) {
    _load_menu(
        oled_off_menu_items,
        1 + eecfg.side.autooff,
        arrLength(oled_off_menu_items)
    );
    switchApp(_draw_menu, _menu_on_rot, side_led_auto_off_press);
}

static void drawBar(uint16_t apm, uint16_t max) {
    if (apm >= max) apm = max;

    apm = 128 * apm / max;

    uint8_t a = 0;
    while (apm >= 6) {
        oled_write_char(128 + 5,false);
        a++;
        apm-=6;
    }

    if (apm && a < (128/6)) {
        oled_write_char(128 + (apm % 6), false); // some progress bar style font is added at 218 in glcdfont.c
        a++;
    }

    while (a < (128 / 6)) {
        if ((a % 5) == 0) oled_write_char(128, false); // draw grid bar
        else oled_write_char(' ', false);
        a++;
    }
}

static void drawValueAndBar(uint8_t value, uint16_t max) {
    itoa(value, str_buf, 10);
    oled_write(str_buf, false);
    oled_write_char('\n', false);
    drawBar(value, max);
}


// SUBAPP: rgb brightness
static void rgb_bright_draw(void) {
    oled_write_P(PSTR("Bright\n\n"), false);
    drawValueAndBar(eecfg.rgb.bright, MAX_RGB_BRIGHT);
}
static bool rgb_bright_rot(bool moveDown) {
    if (moveDown) eecfg.rgb.bright++;
    else if (!moveDown && eecfg.rgb.bright>0) eecfg.rgb.bright--;
    
    if (eecfg.rgb.bright > MAX_RGB_BRIGHT) {
        eecfg.rgb.bright = MAX_RGB_BRIGHT;
    }

    tiny85_i2c_tx_2b(CMD_RGB_bright, eecfg.rgb.bright);
    return false;
}

// SUBAPP: rgb saturation
static void rgb_sat_draw(void) {
    oled_write_P(PSTR("Saturation\n\n"), false);
    drawValueAndBar(eecfg.rgb.satu, 255);
}
static bool rgb_sat_rot(bool moveDown) {
    if (moveDown && eecfg.rgb.satu < 0xff) eecfg.rgb.satu++;
    else if (!moveDown && eecfg.rgb.satu) eecfg.rgb.satu--;

    tiny85_i2c_tx_2b(CMD_RGB_satu, eecfg.rgb.satu);
    return false;
}

// SUBAPP: rgb hue
static void rgb_hue_draw(void) {
    oled_write_P(PSTR("Color\n\n"), false);
    drawBar(_shared_u8==0 ? eecfg.rgb.hue : eecfg.rgb.hue2, 0xff);
}
static bool rgb_hue_rot(bool moveDown) {
    // no up/down limit, let that cycle from 0 to 0xff
    uint8_t* p = _shared_u8 == 0 ? &eecfg.rgb.hue : &eecfg.rgb.hue2;
    if (moveDown) (*p)++;
    else (*p)--;

    tiny85_i2c_tx_2b(_shared_u8 == 0 ? CMD_RGB_hue : CMD_RGB_hue2, *p);
    return false;
}

// SUBAPP: rgb speed
static void _rgb_set_speed(void) {
    tiny85_i2c_tx_2b(CMD_RGB_speed, 10+MAX_RGB_SPEED-eecfg.rgb.speed);
}
static void rgb_spd_draw(void) {
    oled_write_P(PSTR("Speed\n\n"), false);
    drawValueAndBar(eecfg.rgb.speed, MAX_RGB_SPEED);
}

static bool rgb_spd_rot(bool moveDown) {
    if (moveDown && eecfg.rgb.speed < MAX_RGB_SPEED) eecfg.rgb.speed++;
    else if (!moveDown && eecfg.rgb.speed) eecfg.rgb.speed--;

    _rgb_set_speed();
    return false;
}

// SUBAPP: rgb effect
const char RGB_EFFECT_TITLE[] PROGMEM = "-- Effect --";
const char RGB_EFFECT_MENU_1[] PROGMEM = "Off";
const char RGB_EFFECT_MENU_2[] PROGMEM = "Solid";
const char RGB_EFFECT_MENU_3[] PROGMEM = "Breath";
const char RGB_EFFECT_MENU_4[] PROGMEM = "Rainbow";
const char RGB_EFFECT_MENU_5[] PROGMEM = "Rainbow H.";
const char RGB_EFFECT_MENU_6[] PROGMEM = "Rainbow V.";
const char RGB_EFFECT_MENU_7[] PROGMEM = "Grad H.";
const char RGB_EFFECT_MENU_8[] PROGMEM = "Grad V.";
const char RGB_EFFECT_MENU_9[] PROGMEM = "Raindrops";
const char RGB_EFFECT_MENU_10[] PROGMEM = "Track";
const char RGB_EFFECT_MENU_11[] PROGMEM = "Track RGB";
const char RGB_EFFECT_MENU_12[] PROGMEM = "Ripple";
const char RGB_EFFECT_MENU_13[] PROGMEM = "Ripple RGB";
const char RGB_EFFECT_MENU_14[] PROGMEM = "Noise";
const char RGB_EFFECT_MENU_15[] PROGMEM = "Test";

const char * const RGB_EFFECT_menu_items[] PROGMEM = {
    RGB_EFFECT_TITLE,
    RGB_EFFECT_MENU_1,
    RGB_EFFECT_MENU_2,
    RGB_EFFECT_MENU_3,
    RGB_EFFECT_MENU_4,
    RGB_EFFECT_MENU_5,
    RGB_EFFECT_MENU_6,
    RGB_EFFECT_MENU_7,
    RGB_EFFECT_MENU_8,
    RGB_EFFECT_MENU_9,
    RGB_EFFECT_MENU_10,
    RGB_EFFECT_MENU_11,
    RGB_EFFECT_MENU_12,
    RGB_EFFECT_MENU_13,
    RGB_EFFECT_MENU_14,
    RGB_EFFECT_MENU_15,
};

static void _rgb_set_eff(void) {
    tiny85_i2c_tx_2b(CMD_RGB_eff, eecfg.rgb.mode);
}

static bool rgb_effect_rot(bool moveDown) {
    _menu_on_rot(moveDown);
    if (menuS.cursor == 0) return false;
    eecfg.rgb.mode = menuS.cursor-1;
    _rgb_set_eff();
    return false;
}

// APP: light control
const char RGBTXT_TITLE[] PROGMEM  = "-- RGB Light --";
const char RGBTXT_L1[] PROGMEM  = "Effect";
const char RGBTXT_L2[] PROGMEM  = "Speed";
const char RGBTXT_L3[] PROGMEM  = "Bright";
const char RGBTXT_L4[] PROGMEM  = "Color 1";
const char RGBTXT_L5[] PROGMEM  = "Color 2";
const char RGBTXT_L6[] PROGMEM  = "Saturation";

const char * const RGB_menu_items[] PROGMEM = {
    RGBTXT_TITLE,
    RGBTXT_L1,
    RGBTXT_L2,
    RGBTXT_L3,
    RGBTXT_L4,
    RGBTXT_L5,
    RGBTXT_L6,
    TEXT_AUTO_OFF
};

static bool rgb_auto_off_press(void) {
    if (menuS.cursor > 0) {
        eecfg.rgb.autooff = menuS.cursor-1;
    }
    go_rgb_settings();
    return false;
}

static void go_rgb_auto_off_setting(void) {
    _load_menu(
        oled_off_menu_items,
        1 + eecfg.rgb.autooff,
        arrLength(oled_off_menu_items)
    );
    switchApp(_draw_menu, _menu_on_rot, rgb_auto_off_press);
}

static bool rgb_ctrl_keydown(void) {
    switch (menuS.cursor) {
        case 0:
            app_init();
            break;
        case 1:
            _load_menu(
                RGB_EFFECT_menu_items,
                1 + eecfg.rgb.mode,
                arrLength(RGB_EFFECT_menu_items)
            );
            switchApp(_draw_menu, rgb_effect_rot, go_rgb_settings);
            break;
        case 2:
            switchApp(rgb_spd_draw, rgb_spd_rot, go_rgb_settings);
            enableFastRot(1);
            break;
        case 3:
            switchApp(rgb_bright_draw, rgb_bright_rot, go_rgb_settings);
            break;
        case 4: // hue 1
        case 5: // hue 2
            _shared_u8=(menuS.cursor-4);
            switchApp(rgb_hue_draw, rgb_hue_rot, go_rgb_settings);
            enableFastRot(4);
            break;
        case 6:
            switchApp(rgb_sat_draw, rgb_sat_rot, go_rgb_settings);
            enableFastRot(10);
            break;
        case 7:
            go_rgb_auto_off_setting();
            break;
        default:
            break;
    }
    return true;
}


static bool go_rgb_settings(void) {
    app_eeconfig_save();
    _load_menu(
        RGB_menu_items,
        1,
        arrLength(RGB_menu_items)
    );
    switchApp(_draw_menu, _menu_on_rot, rgb_ctrl_keydown);
    return 0;
}

/// space travel

typedef struct {
    unsigned char speed;
    int8_t dis_x; // distance from center (64,32), max is 64
    int8_t dis_y; // max is 32
    uint8_t target_edge; // target edge, from 0-64-96
} space_star_t;

#define N_STARS     8
space_star_t stars[N_STARS];

static void draw_space_travel(void) {

    void drawStar(space_star_t* star, uint8_t isDrawNotErase) {
        int x = 128/2;
        int y = 64/2;

        y += star->dis_y;
        x += star->dis_x;
        
        if (star->dis_x > 16 || star->dis_x < -16 
        || star->dis_y > 10 || star->dis_y < -10) {
            draw5PxStar(x, y, isDrawNotErase); // 5 pixel star
        }
        else {
            oled_write_pixel(x, y, isDrawNotErase); // 1 pixel star
        }
    }

    void randStar(space_star_t* star) {
        uint16_t tmp = (uint16_t) rand();
        star->speed = (tmp & 0x03); tmp >>= 2;
        if (star->speed == 0) star->speed++;

        tmp = (uint16_t) rand();
        star->dis_y = (tmp & 0x0f); tmp >>= 4;
        star->dis_x = (tmp & 0x1f); tmp >>= 5;
        if (tmp & 1) star->dis_x = -star->dis_x;
        if (tmp & 2) star->dis_y = -star->dis_y;

        star->target_edge = rand() % 95;
        if (star->target_edge == 0) star->target_edge++;
    }

    if (draw_once_flag) {
        for (int i=0;i<N_STARS;i++) {
            randStar(stars+i);
        }
    }

    // clear old star
    if (!draw_once_flag) {
        for (int i = 0; i < N_STARS; i++) {
            drawStar(&stars[i], 2);
        }
    }

    // move star
    for (int i = 0; i < N_STARS; i++) {
        int x = stars[i].dis_x;
        int y;

        if (x < 0) x = -x; // make x positive, so y is positive
        x += stars[i].speed; // advance x

        if (stars[i].target_edge < 64) {
            y = 32 * x / stars[i].target_edge;
        } else { // edge is from 64 to 95
            y = x * (95 - stars[i].target_edge) / 64;
        }

        // check out-of bound
        if (x > 61 || y > 29) {
            randStar(&stars[i]);
        }
        else {
            if (stars[i].dis_y < 0) stars[i].dis_y = -y;
            else stars[i].dis_y = y;

            if (stars[i].dis_x < 0) stars[i].dis_x = -x;
            else stars[i].dis_x = x;
        }

        // draw new star position
        drawStar(&stars[i], 2); // 1
    }
}


/// bongo cat


uint8_t bongo_cat_status = 0;

static void draw_bongo_img(uint8_t img) {
    if (img == bongo_cat_status) return;

    if (img == 0 || (bongo_cat_status != 0 && img > 0)) {
        oled_set_cursor(128 / 2, 2);
        oled_write_compressed_from_I2C_rom(I2C_ROM_ADDR_bongo_up);
        bongo_cat_status = 0;
        if (img == 0) return;
    }
    
    uint16_t bongo_addr = I2C_ROM_ADDR_bongo_both;
    uint8_t r = 1, c = 14;
    if (img == 1) {
        bongo_addr = I2C_ROM_ADDR_bongo_left;
        r = 0; c = 4;
    }
    else if (img == 2) {
        bongo_addr = I2C_ROM_ADDR_bongo_right;
        r = 1; c = 25;
    }

    oled_set_cursor(128/2 + c, 2 + r);

    oled_write_compressed_from_I2C_rom(bongo_addr);
    bongo_cat_status = img;
}


static void draw_bongo_cat(void) {
    if (draw_once_flag) {
        bongo_cat_status = 3; // use a invalid id
        draw_bongo_img(0);
    }
}

static bool keydown_bongo_cat(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        draw_bongo_img(0);
        return true;
    }

    int newImg;
    if (record->event.key.row == 4) newImg = 3;
    else if (record->event.key.col < 7) newImg = 1;
    else newImg = 2;

    draw_bongo_img(newImg);
    return true;
}

/// tomato timer


typedef enum {
    ST_Idle = 0,
    ST_SetTime,
    ST_WorkTimer,
    ST_Flash,
    ST_RestTimer,
} tomato_st;
#define POMODO_REST_MINUTES    5

struct tomato_cfg {
    tomato_st st;
    uint32_t timer_end;
    uint8_t minutes;
} tomato;

static void _tomato_draw_timer(uint16_t seconds) {
    if (seconds > 3600) return;
    char buf[16];
    sprintf(buf,"%2d : %02d", seconds/60, seconds % 60);
    oled_write(buf, 0);
}
static void _tomato_normalize_setting(void) {
    if (tomato.minutes < 1) tomato.minutes = 1;
    else if (tomato.minutes > 60) tomato.minutes = 60;
}

static void draw_tomato(void) {
    if (draw_once_flag) {
        oled_clear();
        oled_set_cursor(0,2);
        oled_write_compressed_from_I2C_rom(I2C_ROM_ADDR_TOMATO_COMPRESSED);

        if (tomato.st == ST_WorkTimer || tomato.st == ST_RestTimer) {
            oled_set_cursor(42,2);
            oled_write_P(PSTR("Tomato Timer"), 0);
        }
        else if (tomato.st == ST_RestTimer) {
            oled_set_cursor(45,4);
            oled_write_P(PSTR("Have a rest!"), 0);
            oled_set_cursor(45,7);
            oled_write_P(PSTR("Press: exit"), 0);
        }
        draw_once_flag = 0;
    }

    switch (tomato.st) {
        case ST_Idle: // idle screen
            if (tomato.minutes == 0) {
                tomato.minutes = 25; // default value
            }
            tomato.st = ST_SetTime;
            // intentionally no break;

        case ST_SetTime: // wait start
            oled_set_cursor(47,1);
            oled_write_P(PSTR("Tomato Timer"), 0);
            oled_set_cursor(45,5);
            oled_write_P(PSTR("Space:start"), 0);
            oled_set_cursor(45,6);
            oled_write_P(PSTR("Rotate:adjust"), 0);
            oled_set_cursor(45,7);
            oled_write_P(PSTR("Press:exit"), 0);

            oled_set_cursor(54,3);
            _tomato_draw_timer(60 * tomato.minutes);
            break;

        case ST_WorkTimer:
            if (tomato.timer_end < timer_read32()) {
                tomato.st = ST_Flash;
                _shared_u8 = 0;
                break;
            }

            uint32_t seconds = tomato.timer_end - timer_read32();
            oled_set_cursor(54,5);
            _tomato_draw_timer(seconds/1000);
            break;

        case ST_Flash:
            // oled task run at 20Hz, flash at 2Hz, for 6Sec
            _shared_u8++;
            if ((_shared_u8 & 0x07) == 0x07) {
                oled_invert(OLED_INVERT_TOGGLE);
            }
            if (_shared_u8 > 60) {
                oled_invert(0);
                tomato.st = ST_RestTimer;
                tomato.timer_end = timer_read32() + (uint32_t)1000*POMODO_REST_MINUTES*60;
                draw_once_flag = 1;
            }
            break; 

        case ST_RestTimer:
            if (tomato.timer_end <= timer_read32()) {
                tomato.st = ST_Idle;
                draw_once_flag = 1;
                break;
            }

            oled_set_cursor(45,5);
            _tomato_draw_timer((tomato.timer_end - timer_read32())/1000);
            break;

        default:
            tomato.st = ST_Idle;
            break;
    }
}

static bool tomato_on_rot(bool moveDown)  {
    switch (tomato.st) {
        case ST_SetTime:
            if (moveDown) tomato.minutes --;
            else tomato.minutes ++;
            _tomato_normalize_setting();
            return false;
        default:
            return true;;
    }
    
}

static bool tomato_keydown(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;

    if (keycode == KC_ESC) {
        // go to home
        app_init();
    }

    if (tomato.st == ST_SetTime) {
        if (keycode == KC_SPACE) {
            tomato.timer_end = timer_read32() + (uint32_t)1000 * tomato.minutes * 60;
            tomato.st = ST_WorkTimer;
            draw_once_flag = 1;
            return false;
        }
    }
    return true;
}

/// rotating cube

uint16_t l_rotateX;
uint16_t l_rotateY;
uint16_t l_rotateZ;
static void draw_rot_cube(void) {
    if (draw_once_flag) {
        l_rotateX = 50;
        l_rotateY = 18;
        l_rotateZ = 77;	
        return;
    }
    l_rotateX += 4;
    l_rotateY += 3;
    l_rotateZ += 1;

    if (l_rotateX > 359) l_rotateX = 0;
    if (l_rotateY > 359) l_rotateY = 0;
    if (l_rotateZ > 359) l_rotateZ = 0;

    oled_set_cursor(80,2);
    oled_clear_region(128/4+1,5);
    DrawCubeRotated(l_rotateX, l_rotateY, l_rotateZ);
}


// bouncing ball
#define BALL_X str_buf[0]
#define BALL_Y str_buf[1]
#define BALL_DIR_X str_buf[2]
#define BALL_DIR_Y str_buf[3]

// draw a 5px star from center
static void draw5PxStar(uint8_t x, uint8_t y, uint8_t value) {
    for (int i=0;i<3;i++) {
        oled_write_pixel(x+i-1, y, value);
    }
    oled_write_pixel(x, y-1, value);
    oled_write_pixel(x, y+1, value);
}


static void _load_menu(const char* const *list, uint8_t cursor, uint8_t length) {
    if (cursor >= length) cursor = 0; // out-of-boundary check

    menuS.list = list;
    menuS.cursor = cursor;
    menuS.length = length;

    menuS.offset = 0;
    if (length > 8 && cursor > 7) {
        menuS.offset = cursor - 3;
        if (menuS.offset > length - 8)
            menuS.offset = length - 8;
    }
}

static void _draw_menu(void) {
    int row, item_i;
    
    row = 0;
    while (row < SCREEN_LINES && row < menuS.length ) {
        item_i = row + menuS.offset;

        if (item_i == menuS.cursor) {
            oled_write_char('>', false);  // active menu indicator
        } else {
            oled_write_char(' ', false);
        }
        oled_write_char(' ', false);

        // referece on progmem 2D string array https://www.arduino.cc/reference/en/language/variables/utilities/progmem/
        strcpy_P(str_buf, (char *)pgm_read_word(&(menuS.list[item_i])));
        oled_write(str_buf, false);
        oled_advance_page(true);
        row++;
    }

    menu_draw_v_scroll();
}


// do not use rot_press and setAppKeyboardListener at the same time. the latter has higher priority
static void switchApp(void (*draw)(void), bool (*rotate)(bool), bool (*rot_press)(void)) {
    if (rotate == NULL) rotate = empty_on_rotate;
    if (rot_press == NULL) rot_press = app_init;

    f_draw = draw;
    f_on_rotate = rotate;
    f_rot_press = rot_press;
    f_keydown = NULL;
    oled_clear();
    draw_once_flag = 1;
    rot_enable_fast = 0;
    app_autoback_sec = 0;
}

static void setAppAutoBack(uint8_t seconds) {
    app_autoback_sec = seconds;
    user_lastact32 = timer_read32();
}


// NOTE, always call this after switchApp. otherwise f_keydown is erased.
static void setAppKeyboardListener(bool (*f)(uint16_t, keyrecord_t *)) {
    f_keydown = f;
}

static void enableFastRot(uint16_t max_rot_value) {
    rot_enable_fast = max_rot_value;
}


static bool settings_on_press(void) {
    switch (menuS.cursor) {
        case 1:
            go_rgb_settings();
            break;
        case 2:
            go_oled_settings();
            break;
        case 3:
            go_side_led_setting();
            break;
        case 4:
            go_artwork_setting();
            break;
        case 5:
            switchApp(draw_tomato, tomato_on_rot, NULL);
            setAppKeyboardListener(tomato_keydown);
            break;
        case 6:
            switchApp(help_draw, NULL, NULL);
            break;
        case 0:
        default:
            app_init();
            break;
    }
    return false;
}

static bool _menu_on_rot(bool moveDown) {
    if (moveDown) {
        if (menuS.cursor < menuS.length - 1) menuS.cursor++;

        if (menuS.cursor > menuS.offset + SCREEN_LINES - 1) menuS.offset++;
    }
    else {
        if (menuS.cursor > 0) menuS.cursor--;

        if (menuS.cursor < menuS.offset) menuS.offset--;
    }
    return false;
}

static void menu_draw_v_scroll(void) {
    uint8_t indicator_len, indicator_pos;

    if (menuS.length <= SCREEN_LINES) return;

    indicator_len = ((uint16_t)SCREEN_HEIGHT * SCREEN_LINES) / menuS.length;
    if (indicator_len < 4) indicator_len = 4;
    indicator_pos = ((uint16_t)SCREEN_HEIGHT) * menuS.offset / menuS.length;

    indicator_len += indicator_pos;
    for (uint8_t c = 1; c <= 2; c++) {
        uint8_t i;
        for (i = 0; i < indicator_pos; i++) oled_write_pixel(SCREEN_WIDTH - c, i, false);
        for (; i < indicator_len; i++) oled_write_pixel(SCREEN_WIDTH - c, i, true);
        for (; i < SCREEN_HEIGHT; i++) oled_write_pixel(SCREEN_WIDTH - c, i, false);
    }
}



/// dashboard part ///
static led_t last_led_state;


static void dashboard_draw(void) {
    if (draw_once_flag) {
        oled_clear();
        _shared_u16 = 0;
        last_led_state.raw = 0;
    }
    else {
        // led status
        led_t led_state = host_keyboard_led_state();
        if (last_led_state.raw != led_state.raw) {
            last_led_state.raw = led_state.raw;
            oled_set_cursor(0,0);
            oled_advance_page(1); // clear current row

            oled_set_cursor(0,0);
            if (led_state.caps_lock) {
                oled_write_P(PSTR(" CAPS "), 1);
                oled_advance_char();
            }
            if (led_state.num_lock) {
                oled_write_P(PSTR(" NUM "), 1);
                //oled_advance_char();
            }
        }

        // APM part & key cnt part
        sprintf(str_buf, "%u KPM   ",get_current_apm());
        oled_set_cursor(0,3);
        oled_write(str_buf,0);

        sprintf(str_buf, "%lu keys   ",get_key_cnt());
        oled_set_cursor(0,4);
        oled_write(str_buf,0);

        // sensor part
        if (_shared_u16 == 0) {
            trig_aht21();
        }
        _shared_u16++;
    }

    static sensor_data_t sensor_data;
    if (_shared_u16 == 50) {
        read_aht21(&sensor_data);
    }
    else if (_shared_u16 == 100) {
        if (is_oled_on()) { // not wake up oled if already off.
        oled_set_cursor(0,6); 
        sprintf(str_buf, "%d.%d",sensor_data.t_int, sensor_data.t_dec);
        oled_write(str_buf,0);
        oled_write_char(0,0); // degree symbol

        sprintf(str_buf, "C %d%% ",sensor_data.h_int);
        oled_write(str_buf,0);
        }
    }
    else if (_shared_u16 >= 1100) {
        _shared_u16 = 0;
    }

    if (draw_once_flag) {
        oled_set_cursor(0,7); oled_write("Lele76 v3.9",0);
        if (eecfg.homeart_id == HOMEART_PIG  || eecfg.homeart_id == HOMEART_PIG_SPACE) {
            oled_set_cursor(80,2); 
            oled_write_compressed_from_I2C_rom(I2C_ROM_ADDR_LelePig);
        }
    }

    if (init_cnt) {
        app_cfg_init();
    }
    else {
        tinyrgb_task();
    }
    

    // homeart part
    switch (eecfg.homeart_id) {
        case HOMEART_PIG_SPACE:
            draw_space_travel();
            break;
        case HOMEART_CUBE:
            draw_rot_cube();
            break;
        case HOMEART_BONGO:
            draw_bongo_cat();
            break;
        default:
            break;
    }
}


static uint8_t dashboard_fn_flag = 0;

static bool dashboard_on_rot(bool moveDown) {
    if (dashboard_fn_flag) {
        if (moveDown) eecfg.homeart_id ++;
        else eecfg.homeart_id--;

        eecfg.homeart_id += (HOMEART_PIG_SPACE+1);
        eecfg.homeart_id %= (HOMEART_PIG_SPACE+1);
        app_init();
        return false;
    }
    return true;
}

static bool on_keydown_dashboard(uint16_t keycode, keyrecord_t *record) {
    if (record->event.key.row == 4 && record->event.key.col == 1) {
        dashboard_fn_flag = record->event.pressed;
    }

    if (eecfg.homeart_id == HOMEART_BONGO)
        return keydown_bongo_cat(keycode, record);
    return true;
}

/// oled settings menu
const char OLED_TITLE[] PROGMEM  = "-- OLED Settings";
const char OLED_MENU_1[] PROGMEM = "Bright";
const char OLED_MENU_2[] PROGMEM = "Invert display";

const char * const oled_menu_items[] PROGMEM = {
    OLED_TITLE,
    OLED_MENU_1,
    OLED_MENU_2,
    TEXT_AUTO_OFF
};
static bool oled_settings_on_press(void) {
    switch (menuS.cursor) {
        case 1:
            switchApp(oled_bright_draw, oled_bright_on_rotate, go_oled_settings);
            enableFastRot(10);
            break;
        case 2:
            eecfg.oled.invert ^= 1;
            oled_invert(eecfg.oled.invert);
            break;
        case 3:
            go_oled_off_setting();
            break;
        case 0:
        default:
            go_settings();
            break;
    }
    return false;
}

static bool go_oled_settings(void) {
    _load_menu(
        oled_menu_items,
        1,
        arrLength(oled_menu_items)
    );
    switchApp(_draw_menu, _menu_on_rot, oled_settings_on_press);
    return false;
}


/// settings menu app ///

const char TXT_TITLE[] PROGMEM  = "-- Settings --";
const char TXT_MENU_1[] PROGMEM = "RGB Lighting";
const char TXT_MENU_2[] PROGMEM = "OLED Screen";
const char TXT_MENU_3[] PROGMEM = "Side light";
const char TXT_MENU_4[] PROGMEM = "Home Artwork";
const char TXT_MENU_5[] PROGMEM = "Tomato timer";
const char TXT_MENU_6[] PROGMEM = "Help";

const char * const setting_menu_items[] PROGMEM = {
    TXT_TITLE,
    TXT_MENU_1,
    TXT_MENU_2,
    TXT_MENU_3,
    TXT_MENU_4,
    TXT_MENU_5,
    TXT_MENU_6,
};

static bool go_settings(void) {
    app_eeconfig_save();

    _load_menu(
        setting_menu_items,
        0,
        arrLength(setting_menu_items)
    );
    switchApp(_draw_menu, _menu_on_rot, settings_on_press);
    setAppAutoBack(4);
    return true;
}

static uint16_t diff_sec(uint32_t from_ms) {
    uint32_t a = timer_read32();
    a = a - from_ms; // timer32 will only overflow after a month
    a = a / 1000;
    return a;
}


static void _auto_off_task(void) {
    int idle_sec = diff_sec(user_lastact32);

    // we don't use oled auto off as that is related to oled content change,
    // which makes the timing confusing.
    if (is_oled_on() && eecfg.oled.autooff) { // oled goto auto-off
        if (idle_sec > pgm_read_word(&AutoOffCfgValues[eecfg.oled.autooff])) {
            oled_off();
            return;
        }
    }

    // by using else-chain to send only one i2c command at each tick
    if (!is_rgb_idle_off && eecfg.rgb.autooff) { // RGB goto auto-off
        if (idle_sec > pgm_read_word(&AutoOffCfgValues[eecfg.rgb.autooff])) {
            if (I2C_STATUS_SUCCESS == tiny85_i2c_tx_2b(CMD_RGB_eff, RGB_Mode_off)) {
                is_rgb_idle_off = 1;
                _key_led_i = 0xff;
                return;
            }
        }
    }

    if (!is_side_idle_off && eecfg.side.autooff) { // side goto auto-off
        if (idle_sec > pgm_read_word(&AutoOffCfgValues[eecfg.side.autooff])) {
            if (I2C_STATUS_SUCCESS == tiny85_i2c_tx_2b(CMD_SIDE_LED_EFFECT, SideLed_Mode_off)) {
                is_side_idle_off = 1;
                _key_led_i = 0xff;
            }
        }
    }
}

void app_draw(void) {

    // menu auto back to home
    if (app_autoback_sec) {
        if ((timer_elapsed(user_lastact32 & 0xffff) / 1000 > app_autoback_sec)) {
            app_init();
            return;
        }
    }

    _auto_off_task();


    // key stroke message to tiny85
    if (_key_led_i != 0xff) {
        if (!is_oled_on()) {
            oled_on();
        }

        if (is_rgb_idle_off) { // RGB recover from auto-off
            is_rgb_idle_off = 0;
            _rgb_set_eff();
        }
        else if (is_side_idle_off) { // side recover from auto-off
            is_side_idle_off = 0;
            _side_led_set_eff();
        }
        else if (tiny85_i2c_tx_2b(CMD_RGB_keypress, _key_led_i) == I2C_STATUS_SUCCESS) {
            _key_led_i = 0xff;
        }
    }

    f_draw();
    draw_once_flag = 0;
}


// if return false, the event chain ends
bool app_on_key(uint16_t keycode, keyrecord_t *record) {
    user_lastact32 = timer_read32();

    // is rot press
    if (record->event.key.row == 0 && record->event.key.col == 0) {
        if (!record->event.pressed) return true;

        if (f_rot_press)
            return f_rot_press();
        else
            return true;
    }
    
    // is normal key press
    if (record->event.pressed) {
        _key_led_i = getLedIdFromKey(    
            record->event.key.row,
            record->event.key.col
        );
    }

    if (f_keydown) {
        return f_keydown(keycode, record);
    }

    return true;
}

static uint16_t last_rot_time = 0;
static int rot_fast_n = 1;

// if return false, event chain ends here.
// if return true, event chain continues, key stroke will likely fire
bool app_on_rotate(bool clockwise) {
    user_lastact32 = timer_read32();

    int n = 0;
    if (rot_enable_fast) {
        bool ans;
        if (timer_elapsed(last_rot_time) < 200) {
            if (rot_fast_n < rot_enable_fast) rot_fast_n += 2;
        }
        else {
            rot_fast_n = 1;
        }
        n = rot_fast_n;

        while (n--) {
            ans = f_on_rotate(clockwise);
        }

        last_rot_time = timer_read();
        return ans;
    }
    else {
        return f_on_rotate(clockwise);
    }
}


// framework part
bool app_init(void) {
    switchApp(dashboard_draw, dashboard_on_rot, go_settings);
    setAppKeyboardListener(on_keydown_dashboard);

    if (init_cnt == 1) {
        app_cfg_init();
    }
    return false;
}

static uint8_t getLedIdFromKey(uint8_t row, uint8_t col) {
    uint8_t ans;
    if (row == 0 && col == 0) return 0xff;
    if (col == 0) row--;
    ans = 16*row;
    if ((row & 1) == 0) { // odd rows
        ans += (0xf-col);
    }
    else {
        ans += col;
    }
    return ans;
}

bool app_eeconfig_load(void) {
    // check magicbyte
    if (eeprom_read_byte((uint8_t*) APP_EECONFIG_ADDR_MAGICBYTE) 
        != APP_EECONFIG_MAGICBYTE)
        return false;

    uint8_t *buf = (uint8_t*) &eecfg;
    for (int i=0;i<sizeof(app_eeconfig_t);i++) {
        *buf = eeprom_read_byte((uint8_t*) (i + APP_EECONFIG_ADDR_START));
        buf++;
    }
    return true;
}

static void app_eeconfig_save(void) {
    uint8_t *src = (uint8_t*) &eecfg;
    for (int i=0;i<sizeof(app_eeconfig_t);i++) {
        eeprom_update_byte((uint8_t*)(i + APP_EECONFIG_ADDR_START), *src);
        src++;
    }

    eeprom_update_byte((uint8_t*)APP_EECONFIG_ADDR_MAGICBYTE, APP_EECONFIG_MAGICBYTE);
}

static void app_cfg_init(void) {
    if (is_tiny_busy()) // if busy, call me later
        return;

    switch (init_cnt) {
        case 1:
            if (!app_eeconfig_load()) {
                // set default value
                eecfg.rgb.mode = RGB_Mode_ripple;
                eecfg.rgb.bright = MAX_RGB_BRIGHT/2;
                eecfg.rgb.speed = MAX_RGB_SPEED;
                eecfg.rgb.satu = 0xff;
                eecfg.rgb.hue = 60;
                eecfg.rgb.hue2 = 200;
                eecfg.rgb.autooff = 0;

                eecfg.oled.bright = 180;
                eecfg.oled.invert = 0;
                eecfg.oled.autooff = 0;

                eecfg.side.mode = SideLed_Mode_keyflash;
                eecfg.side.bright = MAX_SIDE_LED_BRIGHT;
                eecfg.side.speed = MAX_SIDE_LED_SPEED;
                eecfg.side.autooff = 0;

                eecfg.homeart_id = HOMEART_PIG;

                app_eeconfig_save();
            }

            oled_set_brightness(eecfg.oled.bright);
            oled_invert(eecfg.oled.invert);
            is_rgb_idle_off = 0;
            is_side_idle_off = 0;
            break;
        case 2:
            _rgb_set_eff();
            break;
        case 3:
            tiny85_i2c_tx_2b(CMD_RGB_satu, eecfg.rgb.satu);
            break;
        case 4:
            tiny85_i2c_tx_2b(CMD_RGB_bright, eecfg.rgb.bright);
            break;
        case 5:
            tiny85_i2c_tx_2b(CMD_RGB_hue, eecfg.rgb.hue);
            break;
        case 6:
            tiny85_i2c_tx_2b(CMD_RGB_hue2, eecfg.rgb.hue2);
            break;
        case 7:
            _rgb_set_speed();
            break;
        case 8:
            _side_led_set_eff();
            break;
        case 9:
            _side_led_set_bright(eecfg.side.bright);
            break;
        case 10:
        default:
            _side_led_set_speed(eecfg.side.speed);
            init_cnt = 0;
            return;
    }

    init_cnt++;
}
