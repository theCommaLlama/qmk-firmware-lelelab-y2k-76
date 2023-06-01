#pragma once

bool app_init(void);
void app_draw(void);
bool app_on_key(uint16_t keycode, keyrecord_t *record);
bool app_on_rotate(bool clockwise);
void tiny_cfg_init(void);
