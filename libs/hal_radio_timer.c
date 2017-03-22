#include "hal_radio_timer.h"

static NRF_RTC_Type *RTC = (NRF_RTC_Type *)NRF_RTC0_BASE;
static NRF_TIMER_Type *TIMER = (NRF_TIMER_Type *)NRF_TIMER0_BASE;

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

void timer_init(void)
{
	NVIC_ClearPendingIRQ(TIMER_IRQn);
	NVIC_SetPriority(TIMER_IRQn, 0);
	NVIC_EnableIRQ(TIMER_IRQn);

	TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	TIMER->PRESCALER = 4 << TIMER_PRESCALER_PRESCALER_Pos;

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
}

void timer_timeout_set(uint32_t compare_value)
{
	/**@todo should be saved in variable and applied when START event occurs */
	TIMER->CC[1] = compare_value;
	TIMER->INTENSET = TIMER_INTENSET_COMPARE1_Set << TIMER_INTENSET_COMPARE1_Pos;

	TIMER->SHORTS = (TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos) |
									(TIMER_SHORTS_COMPARE1_STOP_Enabled << TIMER_SHORTS_COMPARE1_STOP_Pos);


}

void timer_timeout_clr(void)
{
	TIMER->INTENCLR = TIMER_INTENSET_COMPARE1_Set << TIMER_INTENSET_COMPARE1_Pos;
	TIMER->TASKS_STOP = 1;
	TIMER->TASKS_CLEAR = 1;
	NVIC_ClearPendingIRQ(TIMER_IRQn);
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
	if (TIMER->EVENTS_COMPARE[0] != 0)
	{
		TIMER->EVENTS_COMPARE[0] = 0;
		radio_timer_handler(HAL_RADIO_TIMER_EVT_START);
	}

	if (TIMER->EVENTS_COMPARE[1] != 0)
	{
		TIMER->EVENTS_COMPARE[1] = 0;
		radio_timer_handler(HAL_RADIO_TIMER_EVT_TIMEOUT);
	}
}

#else 


void rtc_init(void)
{
}

uint32_t rtc_counter_get(void)
{
	return 0;
}

void rtc_compare_set(uint32_t compare_value, uint32_t compare_reg, bool enable_irq)
{
	
}

void rtc_compare_clr(uint32_t compare_reg)
{
	
}

uint32_t rtc_compare_evt(void)
{
	return 1;
}

void timer_init(void)
{
}

void timer_compare_set(uint32_t compare_value)
{
}

void timer_compare_clr(void)
{
}

void timer_stop(void)
{
}

uint32_t timer_compare_evt(void)
{
	return 1;
}

#endif // UNIT_TEST
