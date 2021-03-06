/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EVENT_Q_H
#define __EVENT_Q_H

#include <stdint.h>

typedef enum {
	EVENT_TYPE_NONE,
	EVENT_TYPE_STREAM,
} lightblue_evt_t;

lightblue_evt_t event_get();


#endif