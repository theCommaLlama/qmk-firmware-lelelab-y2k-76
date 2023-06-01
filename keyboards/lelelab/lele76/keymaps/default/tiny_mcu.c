#include "tiny_mcu.h"
#include "quantum.h"
#include "i2c_master.h"
#include <string.h>
#include "tiny_mcu_protocol.h"

#define TINY_BUSY_PIN   6

#define I2C_DEF_TIMEOUT 40

#define I2C_ERROR_TINY_BUSY (-10)

#define TINYRGB_ERROR_TX_FULL   1
#define TINYRGB_ERROR_LENGTH    2


static uint8_t tinyrgb_cmd_buf[5];
static uint8_t tinyrgb_cmd_len;


void tiny_busy_port_init(void) {
    DDRD &= ~(1<<TINY_BUSY_PIN); // INPUT
    PORTD &= ~(1<<TINY_BUSY_PIN); // no pull-up
}

uint8_t is_tiny_busy(void) {
    return (PIND & (1<<TINY_BUSY_PIN)) ? 1 : 0;
}


static i2c_status_t i2c_transmit_buf(uint8_t address, const uint8_t *data, uint16_t length, uint16_t timeout) {
    i2c_status_t status = i2c_start(address, timeout);

    for (uint16_t i = 0; i < length && status >= 0; i++) {
        status = i2c_write(*(data++), timeout);
        if (status) break;
    }

    i2c_stop();
    return status;
}

i2c_status_t tiny85_i2c_tx(const uint8_t * data, uint8_t length) {
    if (is_tiny_busy()) {
        tinyrgb_cmd_len = length;
        memcpy(tinyrgb_cmd_buf, data, length);
        return I2C_ERROR_TINY_BUSY;
    }
    return i2c_transmit_buf(TINY_ADDR_WR, data, length, I2C_DEF_TIMEOUT);
}

i2c_status_t tiny85_i2c_tx_2b(uint8_t cmd, uint8_t byte) {
    uint8_t buf[2];
    buf[0] = cmd;
    buf[1] = byte;
    return tiny85_i2c_tx(buf,2);
}

bool tinyrgb_task(void) {
    if (!tinyrgb_cmd_len) return false;
    if (is_tiny_busy()) return false;

    uint8_t ans = i2c_transmit_buf(TINY_ADDR_WR, tinyrgb_cmd_buf, tinyrgb_cmd_len, I2C_DEF_TIMEOUT);
    if (ans == I2C_STATUS_SUCCESS) {
        tinyrgb_cmd_len = 0;
    }
    return true;
}

