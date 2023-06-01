#include <stdint.h>
#include <string.h>
#include "wait.h"
#include "i2c_master.h"
#include "eeprom_24c512A.h"

#define EXTERNAL_EEPROM_ADDRESS_SIZE 2
#define EXTERNAL_EEPROM_BYTE_COUNT 65536
#define EXTERNAL_EEPROM_PAGE_SIZE 128
#define EXTERNAL_EEPROM_WRITE_TIME 3
#define EXTERNAL_EEPROM_I2C_BASE_ADDRESS 0b10101110
#define EXTERNAL_EEPROM_I2C_ADDRESS(loc) (EXTERNAL_EEPROM_I2C_BASE_ADDRESS)


static inline void fill_target_address(uint8_t *buffer, const void *addr) {
    uintptr_t p = (uintptr_t)addr;
    for (int i = 0; i < EXTERNAL_EEPROM_ADDRESS_SIZE; ++i) {
        buffer[EXTERNAL_EEPROM_ADDRESS_SIZE - 1 - i] = p & 0xFF;
        p >>= 8;
    }
}

void i2c_eeprom_rd_block(void *buf, const void *addr, size_t len) {
    uint8_t complete_packet[EXTERNAL_EEPROM_ADDRESS_SIZE];
    fill_target_address(complete_packet, addr);

    i2c_transmit(EXTERNAL_EEPROM_I2C_ADDRESS((uintptr_t)addr), complete_packet, EXTERNAL_EEPROM_ADDRESS_SIZE, 100);
    i2c_receive(EXTERNAL_EEPROM_I2C_ADDRESS((uintptr_t)addr), buf, len, 100);
}

void i2c_eeprom_wr_block(const void *buf, void *addr, size_t len) {
    uint8_t   complete_packet[EXTERNAL_EEPROM_ADDRESS_SIZE + EXTERNAL_EEPROM_PAGE_SIZE];
    uint8_t * read_buf    = (uint8_t *)buf;
    uintptr_t target_addr = (uintptr_t)addr;

    while (len > 0) {
        uintptr_t page_offset  = target_addr % EXTERNAL_EEPROM_PAGE_SIZE;
        int       write_length = EXTERNAL_EEPROM_PAGE_SIZE - page_offset;
        if (write_length > len) {
            write_length = len;
        }

        fill_target_address(complete_packet, (const void *)target_addr);
        for (uint8_t i = 0; i < write_length; i++) {
            complete_packet[EXTERNAL_EEPROM_ADDRESS_SIZE + i] = read_buf[i];
        }

        i2c_transmit(EXTERNAL_EEPROM_I2C_ADDRESS((uintptr_t)addr), complete_packet, EXTERNAL_EEPROM_ADDRESS_SIZE + write_length, 100);
        wait_ms(EXTERNAL_EEPROM_WRITE_TIME);

        read_buf += write_length;
        target_addr += write_length;
        len -= write_length;
    }
}



#define I2C_CACHE_SIZE 16
static uint8_t i2c_cache_buf[I2C_CACHE_SIZE];
static uint8_t i2c_precache_offset;
static uint16_t i2c_precache_addr;
void i2c_eeprom_load_cache(uint16_t eeprom_addr) {
    i2c_eeprom_rd_block(i2c_cache_buf, (void*)eeprom_addr, I2C_CACHE_SIZE);
    i2c_precache_offset = 0; 
    i2c_precache_addr = eeprom_addr;
}
uint8_t i2c_eeprom_cache_rd(void) {
    if (i2c_precache_offset >= I2C_CACHE_SIZE) {
        i2c_precache_addr += I2C_CACHE_SIZE;
        i2c_eeprom_load_cache(i2c_precache_addr);
    }    

    return i2c_cache_buf[i2c_precache_offset++];
}

