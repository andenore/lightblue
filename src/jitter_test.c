#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <nrf.h>
#include <radio_timer.h>
#include <debug.h>
#include <assert.h>
#include "SEGGER_RTT.h"

static radio_timer_t timer[2];

#define M_SCHEDULING_INTERVAL (10000)

void assert_handler(char *buf, uint16_t line)
{
	DEBUG_SET(3);

	printf("Assertion %s @ %d\n", buf, line);

	while (1);
}

void radio_timeout_0(uint32_t state)
{
	uint32_t err;
	

	if (state == RADIO_TIMER_SIG_PREPARE)
	{
		DEBUG_TOGGLE(0);
		__NOP();
	}
	if (state == RADIO_TIMER_SIG_START)
	{
		DEBUG_TOGGLE(1);
		timer[0].start_us += M_SCHEDULING_INTERVAL;
		timer[0].func = radio_timeout_0;

		err = radio_timer_req(&timer[0]);
		ASSERT(err == 0);

		radio_timer_sig_end();
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

	DEBUG_INIT();

	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	radio_timer_init();

	timer[0].start_us = radio_timer_time_get() + 500000;
	timer[0].func = radio_timeout_0;
	err = radio_timer_req(&timer[0]);
	ASSERT(err == 0);

	NVIC_ClearPendingIRQ(SWI0_EGU0_IRQn);
	NVIC_SetPriority(SWI0_EGU0_IRQn, 3);
	NVIC_EnableIRQ(SWI0_EGU0_IRQn);

	NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	NRF_TIMER1->PRESCALER = 4 << TIMER_PRESCALER_PRESCALER_Pos;
	NRF_TIMER1->TASKS_START = 1;

	/* Capture timer when TIMER0 Compare is reached */
	NRF_PPI->CH[0].TEP = (uint32_t)&NRF_TIMER1->TASKS_CAPTURE[0];
	NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[0];

	NRF_PPI->CH[1].TEP = (uint32_t)&NRF_TIMER1->TASKS_CLEAR;
	NRF_PPI->CH[1].EEP = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[0];

	NRF_PPI->CH[2].TEP = (uint32_t)&NRF_EGU0->TASKS_TRIGGER[0];
	NRF_PPI->CH[2].EEP = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[0];
	NRF_PPI->CHENSET = (1UL << 0) | (1UL << 1) | (1UL << 2);

	NRF_EGU0->INTENSET = (EGU_INTENSET_TRIGGERED0_Set << EGU_INTENSET_TRIGGERED0_Pos);


	while (1)
	{
		__WFE();
	}

	return 0;
}

void SWI0_EGU0_IRQHandler(void)
{
	uint32_t cc;
	int diff;
	if (NRF_EGU0->EVENTS_TRIGGERED[0] != 0)
	{
		NRF_EGU0->EVENTS_TRIGGERED[0] = 0;
		cc = NRF_TIMER1->CC[0];
		diff = M_SCHEDULING_INTERVAL - cc;
		printf("CC = %d\n", diff);
		// debug_print();
	}
}