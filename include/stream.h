/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __STREAM_H
#define __STREAM_H

#include <stdbool.h>

#define M_STREAM_DATA_LEN (255)

#define STREAM_EVENT_IRQn       (SWI5_EGU5_IRQn)
#define STREAM_EVENT_IRQHandler (SWI5_EGU5_IRQHandler)

typedef struct {
  uint8_t len;
  uint8_t buf[M_STREAM_DATA_LEN];
  int8_t  timestamp;
} stream_data_t;

void stream_tx_start(void);
void stream_rx_start(void);

int stream_q_put(stream_data_t *data);

bool stream_q_empty(void);

bool stream_q_full(void);

stream_data_t * stream_q_head_peek(void);

stream_data_t * stream_q_get(void);

#endif // __STREAM_H