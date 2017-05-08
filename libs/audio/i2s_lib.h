/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __I2S_LIB_H
#define __I2S_LIB_H

#include <stdint.h>
#include <stdbool.h>

void i2s_init(void);

void i2s_start(void);

void i2s_stop(void);

bool i2s_txptr_upd(void);

void i2s_txptr_cfg(uint8_t *ptr, uint32_t size);

#endif
