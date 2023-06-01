#pragma once

void i2c_eeprom_rd_block(void *buf, const void *addr, size_t len);
void i2c_eeprom_wr_block(const void *buf, void *addr, size_t len);
void i2c_eeprom_load_cache(uint16_t eeprom_addr);
uint8_t i2c_eeprom_cache_rd(void);

