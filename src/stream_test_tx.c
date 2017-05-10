/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <nrf.h>
#include <debug.h>
#include <assert.h>
#include <stream.h>
#include "SEGGER_RTT.h"

void assert_handler(char *buf, uint16_t line)
{
	DEBUG_SET(3);

	printf("Assertion %s @ %d\n", buf, line);

	debug_print();

	while (1);
}

void HardFault_Handler(void)
{
	DEBUG_SET(3);
	printf("HardFault_Handler...\n");
	while (1);
}

stream_data_t data;

int main(int argc, char *argv[])
{
	printf("Starting TX stream...\n");

	DEBUG_INIT();

	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	memset(&data, 0, sizeof(data));
	data.len = M_STREAM_DATA_LEN;

	stream_tx_start();

	while (1)
	{

		if (stream_q_put(&data) == 0)
		{
			//printf("new packet: %d\n", (unsigned int)buf[0]);
			data.buf[0] ++;
		}
		
		__WFE();

	}

	return 0;
}
