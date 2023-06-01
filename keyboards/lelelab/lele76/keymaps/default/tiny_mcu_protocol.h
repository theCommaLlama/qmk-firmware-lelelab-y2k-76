#ifndef __TINY_I2C_PROTOCOL_H__
#define __TINY_I2C_PROTOCOL_H__

// ADDRESS
#define TINY_ADDR        0x5C
#define TINY_ADDR_WR     ((TINY_ADDR<<1) & 0xfe)


// COMMANDS
typedef enum {
    CMD3_RGB_c1 = 0,   // r,g,b, not used
    CMD3_RGB_c2,       // r,g,b, not used
    CMD3_RGB_hsv,        // h,s,v, not used
    CMD_RGB_eff,
    CMD_RGB_hue, // hue is [0,0xff], represents [0,360)
    CMD_RGB_hue2, // hue is [0,0xff], represents [0,360)
    CMD_RGB_bright, // [0, MAX_RGB_BRIGHT]
    CMD_RGB_satu,
    CMD_RGB_speed, // [0,MAX_RGB_SPEED]
    CMD_RGB_keypress, // for keystroke trig

    CMD_SIDE_LED_EFFECT = 20, // mode
    CMD_SIDE_LED_BRIGHT, // bright
    CMD_SIDE_LED_SPEED, // speed

    CMD0_Diag_ping = 50, // length 0, always ack
} payload_type_enum;


// RGB modes
typedef enum
{
    RGB_Mode_off = 0,
    RGB_Mode_solid, // rgb1
    RGB_Mode_breath, // rgb1
    RGB_Mode_rainbow, // brightness, period
    RGB_Mode_rainbow_h, // brightness, period
    RGB_Mode_rainbow_v, // brightness, period
    RGB_Mode_gradient_h, // rgb1, rgb2, period
    RGB_Mode_gradient_v, // rgb1, rgb2, period
    RGB_Mode_rand_drops, // rgb1, period
    RGB_Mode_track, // rgb1
    RGB_Mode_track_rgb,
    RGB_Mode_ripple, // rgb1
    RGB_Mode_ripple_rgb,
    RGB_Mode_noise,
    RGB_Mode_RGB, // for diagnosis, RGB loop
    RGB_Mode_two_color, // internal use
} led_mode_enum;

#define MAX_RGB_VALUE       50
#define MAX_RGB_BRIGHT      50
#define MAX_RGB_SPEED       50 


typedef enum
{
    rgb_dir_hori = 0,
    rgb_dir_vert,
    rgb_dir_raidal,
} payload_direction_enum;

#define direction_oneway 0x10
#define direction_back_forth 0x20



// side LED modes

typedef enum {
    SideLed_Mode_off = 0,
    SideLed_Mode_breath,
    SideLed_Mode_keyflash,
    SideLed_Mode_solid,
} SideLed_mode_enum;

#define MAX_SIDE_LED_SPEED  10
#define MAX_SIDE_LED_BRIGHT  127


#endif

