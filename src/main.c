#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <nrf.h>
#include <radio_timer.h>
#include <debug.h>
#include <assert.h>
#include "SEGGER_RTT.h"

static radio_timer_t timer[2];

void assert_handler(char *buf, uint16_t line)
{
	DEBUG_SET(3);

	printf("Assertion %s @ %d\n", buf, line);

	while (1);
}

void radio_timeout_0(uint32_t state)
{
	uint32_t err;

	switch (state)
	{
		case RADIO_TIMER_SIG_PREPARE:
			break;

		case RADIO_TIMER_SIG_START:
			DEBUG_TOGGLE(0);
			timer[0].start_us += 500000;
			timer[0].func = radio_timeout_0;

			err = radio_timer_req(&timer[0]);
			ASSERT(err == 0);

			// radio_timer_sig_end();
			break;

		default:
			ASSERT(false);
			break;
	}
}

void radio_timeout_1(uint32_t state)
{
	uint32_t err;

	switch (state)
	{
		case RADIO_TIMER_SIG_PREPARE:
			break;

		case RADIO_TIMER_SIG_START:
			DEBUG_TOGGLE(1);
			timer[1].start_us += 500000;
			timer[1].func = radio_timeout_1;

			err = radio_timer_req(&timer[1]);
			ASSERT(err == 0);

			// radio_timer_sig_end();
			break;

		default:
			ASSERT(false);
			break;
	}
}

void HardFault_Handler(void)
{
	DEBUG_SET(3);
	printf("HardFault_Handler...\n");
	while (1);
}

int main(int argc, char *argv[])
{
	uint32_t err;

	printf("Starting program...\n");

	memset(&timer[0], 0, sizeof(radio_timer_t));
	memset(&timer[1], 0, sizeof(radio_timer_t));

	DEBUG_INIT();

	radio_timer_init();

	timer[0].start_us = radio_timer_time_get() + 500000;
	timer[0].func = radio_timeout_0;
	err = radio_timer_req(&timer[0]);
	ASSERT(err == 0);

	timer[1].start_us = radio_timer_time_get() + 750000;
	timer[1].func = radio_timeout_1;
	err = radio_timer_req(&timer[1]);
	ASSERT(err == 0);


	while (1)
	{
	}

	return 0;
}