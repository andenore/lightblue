/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hal_radio_timer.h"

static volatile NRF_RTC_Type *RTC = (NRF_RTC_Type *)NRF_RTC0_BASE;
static volatile NRF_TIMER_Type *TIMER = (NRF_TIMER_Type *)NRF_TIMER0_BASE;

#ifndef UNIT_TEST

void rtc_init(void)
{
	NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);


	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_SetPriority(RTC_IRQn, 0);
	NVIC_EnableIRQ(RTC_IRQn);

	RTC->TASKS_START = 1;
}

uint32_t rtc_counter_get(void)
{
	return RTC->COUNTER;
}

void rtc_compare_set(uint32_t compare_value, uint32_t compare_reg, bool enable_irq)
{
	ASSERT(compare_reg <= 1);
	// printf("rtc compare[%d] set = %d  irq_enabled = %d\n",(int)compare_reg, (int)compare_value, (int)enable_irq);
	RTC->CC[compare_reg] = compare_value;
	if (enable_irq)
	{
		RTC->INTENSET = (RTC_INTENSET_COMPARE0_Set << (RTC_INTENSET_COMPARE0_Pos + compare_reg));
	}
	RTC->EVTENSET = (RTC_EVTENSET_COMPARE0_Set << (RTC_EVTENSET_COMPARE0_Pos + compare_reg));
}

void rtc_compare_clr(uint32_t compare_reg)
{
	// printf("rtc compare[%d] clear\n",(int)compare_reg);
	RTC->INTENCLR = (RTC_INTENSET_COMPARE0_Set << (RTC_INTENSET_COMPARE0_Pos + compare_reg));
	RTC->EVTENCLR = (RTC_EVTENSET_COMPARE0_Set << (RTC_EVTENSET_COMPARE0_Pos + compare_reg));
}

// uint32_t rtc_compare_evt(void)
// {
// 	uint32_t retval = (RTC->EVENTS_COMPARE[0] != 0);

// 	if(retval)
// 	{
// 		RTC->EVENTS_COMPARE[0] = 0;
// 	}
// 	return retval;
// }

typedef struct {
	bool timeout_enabled;
	uint32_t timeout_val;
} m_hal_timer_config_t;

static m_hal_timer_config_t m_hal_timer_config;

void timer_init(void)
{
	NVIC_ClearPendingIRQ(TIMER_IRQn);
	NVIC_SetPriority(TIMER_IRQn, 0);
	NVIC_EnableIRQ(TIMER_IRQn);

	TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	TIMER->PRESCALER = 4 << TIMER_PRESCALER_PRESCALER_Pos;

	TIMER->INTENCLR = 0xFFFFFFFF;

	/* Start timer when RTC Compare is reached */
	// NRF_PPI->CH[M_RTC_TIMER_PPICH].TEP = (uint32_t)&TIMER->TASKS_START;
	// NRF_PPI->CH[M_RTC_TIMER_PPICH].EEP = (uint32_t)&RTC->EVENTS_COMPARE[0];

	NRF_PPI->CHENCLR = (1UL << M_RTC_TIMER_PPICH);
}

void timer_compare_set(uint32_t compare_value)
{
	// printf("Timer compare set = %d\n", (int)compare_value);
	// Start timer when RTC Compare is reached 
	NRF_PPI->CHENSET = (1UL << M_RTC_TIMER_PPICH);

	TIMER->CC[0] = compare_value;
	TIMER->INTENSET = TIMER_INTENSET_COMPARE0_Set << TIMER_INTENSET_COMPARE0_Pos;

	// Clear timer on START event
	TIMER->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
}

void timer_compare_clr(void)
{
	// printf("timer compare clear\n");
	TIMER->INTENCLR = TIMER_INTENSET_COMPARE0_Set << TIMER_INTENSET_COMPARE0_Pos;
	NRF_PPI->CHENCLR = (1UL << M_RTC_TIMER_PPICH);
	NVIC_ClearPendingIRQ(TIMER_IRQn);
	TIMER->EVENTS_COMPARE[0] = 0;
}

void timer_timeout_set(uint32_t compare_value)
{
	ASSERT(compare_value > 10); /**@todo: check how short the timeout can be */
	m_hal_timer_config.timeout_val = compare_value;
	m_hal_timer_config.timeout_enabled = true;
	// printf("timeout set\n");
}

void timer_timeout_clr(void)
{
	TIMER->INTENCLR = TIMER_INTENSET_COMPARE1_Set << TIMER_INTENSET_COMPARE1_Pos;
	TIMER->TASKS_STOP = 1;
	TIMER->TASKS_CLEAR = 1;
	NVIC_ClearPendingIRQ(TIMER_IRQn);
	TIMER->EVENTS_COMPARE[1] = 0;
}

uint32_t timer_timeout_evt_get(void)
{
	return (uint32_t)&TIMER->EVENTS_COMPARE[1];
}

void RTC_IRQHandler(void)
{
	if (RTC->EVENTS_COMPARE[RTC_COMPARE_PRETICK] != 0 &&
		(RTC->INTENSET == (RTC_INTENSET_COMPARE0_Set << (RTC_INTENSET_COMPARE0_Pos + RTC_COMPARE_PRETICK))) )
	{
		RTC->EVENTS_COMPARE[RTC_COMPARE_PRETICK] = 0;
		radio_timer_handler(HAL_RADIO_TIMER_EVT_PRETICK);
	}

	if (RTC->EVENTS_COMPARE[RTC_COMPARE_START] != 0 && 
		(RTC->INTENSET == (RTC_INTENSET_COMPARE0_Set << (RTC_INTENSET_COMPARE0_Pos + RTC_COMPARE_START))) )
	{
		RTC->EVENTS_COMPARE[RTC_COMPARE_START] = 0;
		radio_timer_handler(HAL_RADIO_TIMER_EVT_PRESTART);
	}
}

void TIMER_IRQHandler(void)
{
	if (TIMER->EVENTS_COMPARE[1] != 0)
	{
		TIMER->EVENTS_COMPARE[1] = 0;
		TIMER->EVENTS_COMPARE[0] = 0;
		TIMER->INTENCLR = (TIMER_INTENCLR_COMPARE0_Clear << TIMER_INTENCLR_COMPARE0_Pos) |
											(TIMER_INTENCLR_COMPARE1_Clear << TIMER_INTENCLR_COMPARE1_Pos);
		NVIC_ClearPendingIRQ(TIMER_IRQn);

		radio_timer_handler(HAL_RADIO_TIMER_EVT_TIMEOUT);
	}

	if (TIMER->EVENTS_COMPARE[0] != 0)
	{
		TIMER->EVENTS_COMPARE[0] = 0;
		if (m_hal_timer_config.timeout_enabled)
		{
			m_hal_timer_config.timeout_enabled = false;

			TIMER->CC[1] = m_hal_timer_config.timeout_val;			
			TIMER->INTENCLR = TIMER_INTENCLR_COMPARE0_Clear << TIMER_INTENCLR_COMPARE0_Pos;
			TIMER->INTENSET = TIMER_INTENSET_COMPARE1_Set << TIMER_INTENSET_COMPARE1_Pos;

			TIMER->SHORTS = (TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos) |
											(TIMER_SHORTS_COMPARE1_STOP_Enabled << TIMER_SHORTS_COMPARE1_STOP_Pos);
		}
		else
		{	
			TIMER->TASKS_STOP = 1;
			TIMER->TASKS_CLEAR = 1;
			TIMER->INTENCLR = (TIMER_INTENCLR_COMPARE0_Clear << TIMER_INTENCLR_COMPARE0_Pos) |
												(TIMER_INTENCLR_COMPARE1_Clear << TIMER_INTENCLR_COMPARE1_Pos);
		}

		radio_timer_handler(HAL_RADIO_TIMER_EVT_START);
	}
}

#else // UNIT_TEST BEGIN

typedef struct {
	uint32_t compare_val;
	bool enable_irq;
} compare_reg_t;

typedef struct {
	compare_reg_t compare_reg[2];
	bool enabled;
	uint32_t counter;
} rtc_t;

typedef struct {
	compare_reg_t compare_reg[1];
	bool enabled;
	uint32_t counter;
	bool clear_on_evt;
	uint32_t shorts;
} timer_t;

static rtc_t rtc;
static timer_t timer;


void rtc_init(void)
{
	rtc.enabled = true;
}

uint32_t rtc_counter_get(void)
{
	return rtc.counter;
}

void rtc_compare_set(uint32_t compare_value, uint32_t compare_reg, bool enable_irq)
{
	ASSERT(compare_reg < 2);
	rtc.compare_reg[compare_reg].compare_val = compare_value & 0x00FFFFFF;
	rtc.compare_reg[compare_reg].enable_irq = enable_irq;

	printf("RTC.CC[%d] = %d   irq_enable = %d\n", (int)compare_reg, (unsigned int)compare_value, (int)enable_irq);
}

void rtc_compare_clr(uint32_t compare_reg)
{
	ASSERT(compare_reg < 2);
	rtc.compare_reg[compare_reg].enable_irq = false;	
}

void timer_init(void)
{
	timer.enabled = true;
	timer.counter = 0;
}

void timer_compare_set(uint32_t compare_value)
{
	timer.compare_reg[0].compare_val = compare_value;
	timer.compare_reg[0].enable_irq = true;
	printf("TIMER.CC[0] = %d", (unsigned int)compare_value);
}

void timer_compare_clr(void)
{
	timer.compare_reg[0].enable_irq = false;
}

void timer_stop(void)
{
	timer.enabled = false;
	timer.counter = 0;
}


#endif // UNIT_TEST END
