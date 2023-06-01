RGBLIGHT_ENABLE = no

OLED_ENABLE = yes
OLED_DRIVER = SSD1306
OLED_IC = OLED_IC_SH1106

EXTRAKEY_ENABLE = yes

VIA_ENABLE = yes
ENCODER_MAP_ENABLE = no

SRC += screen_app.c apm.c aht_sensor.c tiny_mcu.c
SRC += eeprom_24c512A.c
SRC += sin.c cube.c

