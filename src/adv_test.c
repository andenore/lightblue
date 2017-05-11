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
#include <adv.h>
#include "SEGGER_RTT.h"

void assert_handler(char *buf, uint16_t line)
{
	DEBUG_SET(3);

	printf("Assertion %s @ %d\n", buf, line);

	radio_timer_debug_print();

	while (1);
}

void HardFault_Handler(void)
{
	DEBUG_SET(3);
	printf("HardFault_Handler...\n");
	while (1);
}

int main(int argc, char *argv[])
{
	printf("Starting advertiser...\n");

	DEBUG_INIT();

	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	adv_start();

	while (1)
	{
		//__WFE();
	}

	return 0;
}
