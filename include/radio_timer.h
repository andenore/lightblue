/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RADIO_TIMER_H
#define __RADIO_TIMER_H

#include <stdint.h>

typedef void (* timer_func_t)(uint32_t state);

#define PRETICK_NORMAL 	(0)
#define PRETICK_SHORT 	(1)

#define RADIO_TIMER_SIG_PREPARE (0)
#define RADIO_TIMER_SIG_START 	(1)
#define RADIO_TIMER_SIG_TIMEOUT (2)
#define RADIO_TIMER_SIG_RADIO 	(3)

typedef struct radio_timer {
	timer_func_t func;
	volatile uint32_t start_us;
	uint32_t length_us;
	uint32_t pretick;

	/** internal */
	struct {
		volatile uint32_t start_rtc_unit;
		volatile uint32_t start_timer_unit;
		volatile uint32_t state;
		volatile struct radio_timer *next;
	} _internal;
} radio_timer_t;

volatile radio_timer_t * radio_timer_head_get(void);

void radio_timer_init(void);

uint32_t radio_timer_time_get(void);

uint32_t radio_timer_req(radio_timer_t *p_timer);

void radio_timer_sig_end(void);

void radio_timer_timeout_set(uint32_t tmo);

uint32_t radio_timer_timeout_evt_get(void);

void radio_timer_debug_print(void);

#endif