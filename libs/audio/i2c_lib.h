#ifndef __I2C_LIB_H
#define __I2C_LIB_H

#include <stdint.h>

void i2c_init(uint8_t device_address, uint32_t scl_pin, uint32_t sda_pin);
void i2c_write(uint8_t addr, uint8_t data);
uint8_t i2c_read(uint8_t addr);

#endif
