/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "event_q.h"
#include "stream.h"

lightblue_evt_t event_get()
{
	if (!stream_q_empty())
	{
		return EVENT_TYPE_STREAM;
	}

	return EVENT_TYPE_NONE;
}