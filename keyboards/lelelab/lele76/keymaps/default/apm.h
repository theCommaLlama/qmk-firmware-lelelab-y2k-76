#pragma once

#include "quantum.h"


void    set_current_apm(uint16_t);
uint16_t get_current_apm(void);
uint32_t get_key_cnt(void);
void    update_apm(void);

void decay_apm(void);
