#pragma once
#include "quantum.h"
#include "i2c_master.h"

void tiny_busy_port_init(void);
uint8_t is_tiny_busy(void);
i2c_status_t tiny85_i2c_tx(const uint8_t * data, uint8_t length);
i2c_status_t tiny85_i2c_tx_2b(uint8_t cmd, uint8_t byte);
bool tinyrgb_task(void);

