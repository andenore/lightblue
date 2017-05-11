/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MIC_H
#define __MIC_H

void mic_init();

void mic_freq_set(uint32_t freq);

void mic_gain_up(void);

void mic_gain_down(void);

void mic_start(void);

uint32_t mic_start_task_get(void);

void mic_stop(void);

void mic_rxptr_cfg(uint32_t *p, uint32_t size);

bool mic_rxptr_upd(void);

bool mic_events_end(void);

#endif // __MIC_H