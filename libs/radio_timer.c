/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <radio_timer.h>
#include <hal_radio_timer.h>
#include <debug.h>
#include <assert.h>

#define M_RADIO_TIMER_BITS 									(24)
#define M_RADIO_TIMER_MAX_VAL 							((1 << M_RADIO_TIMER_BITS)-1)
#define M_RADIO_TIMER_MAX_VAL_US						(512000000)
#define M_RADIO_TIMER_MAX_SCHED_INTERVAL_US	(M_RADIO_TIMER_MAX_VAL_US / 2)
#define M_RADIO_TIMER_US_TO_FEMTO(x)				((x)*(uint64_t)1000000000)
#define M_RADIO_TIMER_UNIT_FEMTO						((uint64_t)30517578125)
#define M_RADIO_TIMER_FEMTO_TO_US_FACTOR    (1000000000)


#define RADIO_TIMER_STATE_IDLE 			(0)
#define RADIO_TIMER_STATE_SCHEDULED (1)
#define RADIO_TIMER_STATE_PREPARE 	(2)
#define RADIO_TIMER_STATE_START 		(3)
#define RADIO_TIMER_STATE_TIMEOUT 	(4)
#define RADIO_TIMER_STATE_RADIO 		(5)


static volatile radio_timer_t *p_timer_head = NULL;
static volatile radio_timer_t *p_timer_active = NULL;

volatile radio_timer_t * radio_timer_head_get(void)
{
	return p_timer_head;
}

void radio_timer_init(void)
{
	p_timer_head = NULL;
	rtc_init();
	timer_init();
}

uint32_t radio_timer_time_get(void)
{
	return (uint32_t)((uint64_t)rtc_counter_get() * (uint64_t)M_RADIO_TIMER_UNIT_FEMTO / (uint64_t)M_RADIO_TIMER_FEMTO_TO_US_FACTOR);
}

uint32_t radio_timer_req(radio_timer_t *p_timer)
{
	int32_t diff;
	bool new_head = false;
	volatile radio_timer_t *p_curr, *p_last;

	// printf("radio_timer_req @0x%08x, state = %d\n", (unsigned int)p_timer, (int)p_timer->_internal.state);
	if (!(p_timer->_internal.state == RADIO_TIMER_STATE_IDLE || p_timer->_internal.state == RADIO_TIMER_STATE_START))
	{
		return -1;
	}
	
	// TODO: Greater than or equal?
	if (p_timer->start_us >= M_RADIO_TIMER_MAX_VAL_US) {
		p_timer->start_us -= M_RADIO_TIMER_MAX_VAL_US;
	}

	
	p_timer->_internal.start_rtc_unit = ((uint64_t)M_RADIO_TIMER_US_TO_FEMTO(p_timer->start_us) + (M_RADIO_TIMER_UNIT_FEMTO/2)) / M_RADIO_TIMER_UNIT_FEMTO;
	p_timer->_internal.start_timer_unit = ((M_RADIO_TIMER_US_TO_FEMTO(p_timer->start_us) + 2*M_RADIO_TIMER_UNIT_FEMTO) - ((uint64_t)p_timer->_internal.start_rtc_unit * M_RADIO_TIMER_UNIT_FEMTO)) / M_RADIO_TIMER_FEMTO_TO_US_FACTOR; 
	p_timer->_internal.start_rtc_unit = (p_timer->_internal.start_rtc_unit - 2) & M_RADIO_TIMER_MAX_VAL;

	// printf("start_us: %d\n", (unsigned int)p_timer->start_us);
	// printf("start_rtc_unit: %d\n", (unsigned int)p_timer->_internal.start_rtc_unit);
	// printf("start_timer_unit: %d\n", (unsigned int)p_timer->_internal.start_timer_unit);
	

	p_timer->_internal.next = NULL;


	__disable_irq();
	
	if (p_timer_head == NULL)
	{
		new_head = true;
		p_timer_head = p_timer;
	}
	else
	{
		p_curr = p_last = p_timer_head;
		while (p_curr != NULL)
		{
			// Check if   p_curr->start_us > p_timer->start_us
			diff = p_curr->start_us - p_timer->start_us;
			// printf("t0 = 0x%08x, t1 = 0x%08x, diff %d\n", (unsigned int)p_curr->start_us, (unsigned int)p_timer->start_us, (int)diff);
			if ((diff >= 0 && diff < M_RADIO_TIMER_MAX_SCHED_INTERVAL_US) || (diff < -M_RADIO_TIMER_MAX_SCHED_INTERVAL_US))
			{
				break;
			}
			p_last = p_curr;
			p_curr = p_curr->_internal.next;			
		}

		if (p_curr == p_timer_head)
		{
			new_head = true;
			p_timer_head = p_timer;
			p_timer->_internal.next = p_curr;
		}
		else
		{
			p_last->_internal.next = p_timer;
			p_timer->_internal.next = p_curr;
		}
	}

	p_timer->_internal.state = RADIO_TIMER_STATE_SCHEDULED;

	if (new_head)
	{
		// printf("new head@0x%08x\n", (unsigned int)p_timer_head);
		uint32_t pretick_time_rtc_units;
		uint32_t rtc_compare_val;
		switch (p_timer_head->pretick)
		{
			case PRETICK_NORMAL:
				pretick_time_rtc_units = 49;
				break;
			default:
				ASSERT(false);
				break;
		}
		rtc_compare_val = (p_timer_head->_internal.start_rtc_unit - pretick_time_rtc_units) & M_RADIO_TIMER_MAX_VAL;

		rtc_compare_set(rtc_compare_val, RTC_COMPARE_PRETICK, true);
		rtc_compare_set(p_timer_head->_internal.start_rtc_unit, RTC_COMPARE_START, false);
		timer_compare_set(p_timer_head->_internal.start_timer_unit);
	}

	__enable_irq();

	return 0;
}

void radio_timer_timeout_set(uint32_t tmo)
{
	timer_timeout_set(tmo);
}

uint32_t radio_timer_timeout_evt_get(void)
{
	return timer_timeout_evt_get();
}

void radio_timer_debug_print(void)
{
	volatile radio_timer_t *p_curr = p_timer_head;
	printf("debug_print:\n");

	if (p_timer_active != NULL)
	{		
		printf("active event: start_us = %d, state = %d\n", (int)p_timer_active->start_us, (int)p_timer_active->_internal.state);
	}

	while (p_curr != NULL)
	{		
		printf("event: start_us = %d, state = %d\n", (int)p_curr->start_us, (int)p_curr->_internal.state);
		p_curr = p_curr->_internal.next;			
	}

	printf("RTC   CC[RTC_COMPARE_PRETICK] = %d  0x%08x\n", (unsigned int)NRF_RTC0->CC[RTC_COMPARE_PRETICK], (unsigned int)NRF_RTC0->INTENSET);
	printf("RTC   CC[RTC_COMPARE_START]   = %d\n", (unsigned int)NRF_RTC0->CC[RTC_COMPARE_START]);
	printf("TIMER CC[START_OFFSET]        = %d %d 0x%08x\n", (unsigned int)NRF_TIMER0->CC[0], (unsigned int)NRF_TIMER0->EVENTS_COMPARE[0], (unsigned int)NRF_TIMER0->INTENSET);
	printf("TIMER CC[TIMEOUT]             = %d %d\n", (unsigned int)NRF_TIMER0->CC[1], (unsigned int)NRF_TIMER0->EVENTS_COMPARE[0]);
	printf("PPI ENABLED                   = 0x%08x\n", (unsigned int)NRF_PPI->CHEN);
}

void radio_timer_sig_end(void)
{
	// printf("rem_sig_end\n");
	ASSERT(p_timer_active != NULL);
	p_timer_active = NULL;
	if (p_timer_head == NULL)
	{
		return;
	}

	uint32_t pretick_time_rtc_units;
	uint32_t rtc_compare_val;
	switch (p_timer_head->pretick)
	{
		case PRETICK_NORMAL:
			pretick_time_rtc_units = 49;
			break;
		default:
			ASSERT(false);
			break;
	}
	rtc_compare_val = (p_timer_head->_internal.start_rtc_unit - pretick_time_rtc_units) & M_RADIO_TIMER_MAX_VAL;

	timer_timeout_clr();
	rtc_compare_set(rtc_compare_val, RTC_COMPARE_PRETICK, true);
	rtc_compare_set(p_timer_head->_internal.start_rtc_unit, RTC_COMPARE_START, false);
	timer_compare_set(p_timer_head->_internal.start_timer_unit);	
}

void radio_timer_handler(uint32_t evt)
{
	switch(evt)
	{
		case HAL_RADIO_TIMER_EVT_TIMEOUT:
		{
			ASSERT(p_timer_active->_internal.state == RADIO_TIMER_STATE_START);
			//p_timer_active->_internal.state = RADIO_TIMER_STATE_TIMEOUT;
			timer_timeout_clr();
			p_timer_active->func(RADIO_TIMER_SIG_TIMEOUT);
		}
		break;

		case HAL_RADIO_TIMER_EVT_PRETICK:
		{
			p_timer_active = p_timer_head;
			p_timer_head = p_timer_head->_internal.next;
			p_timer_active->_internal.next = NULL;

			rtc_compare_clr(RTC_COMPARE_PRETICK);
			ASSERT(p_timer_active->_internal.state == RADIO_TIMER_STATE_SCHEDULED);
			p_timer_active->_internal.state = RADIO_TIMER_STATE_PREPARE;
			p_timer_active->func(RADIO_TIMER_SIG_PREPARE);
		}
		break;

		case HAL_RADIO_TIMER_EVT_START:
		{
			rtc_compare_clr(RTC_COMPARE_START);
			timer_compare_clr();

			ASSERT(p_timer_active->_internal.state == RADIO_TIMER_STATE_PREPARE);
			p_timer_active->_internal.state = RADIO_TIMER_STATE_START;
			p_timer_active->func(RADIO_TIMER_SIG_START);
		}
		break;

		case HAL_RADIO_TIMER_EVT_RADIO:
		{
			timer_compare_clr();
			timer_timeout_clr();
			ASSERT(p_timer_active->_internal.state == RADIO_TIMER_STATE_START);
			//p_timer_active->_internal.state = RADIO_TIMER_STATE_RADIO;
			p_timer_active->func(RADIO_TIMER_SIG_RADIO);
		}
		break;

		default:
			ASSERT(false);
			break;
	}
}