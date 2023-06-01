#include "apm.h"

static uint32_t key_cnt     = 0;
static uint16_t current_apm = 0;
static uint16_t apm_timer   = 0;

void set_current_apm(uint16_t new_apm) { current_apm = new_apm; }

uint16_t get_current_apm(void) { return current_apm; }
uint32_t get_key_cnt(void) { return key_cnt; }

void update_apm(void) {
    key_cnt++;

    if (apm_timer > 0) {
        current_apm = ((60000 / timer_elapsed(apm_timer)) + current_apm * 10)/13;
    }
    apm_timer = timer_read();
}

void decay_apm(void) {
    if (timer_elapsed(apm_timer) > 500) {
        current_apm = current_apm * 10 / 13;
        apm_timer = timer_read();
    }
}
