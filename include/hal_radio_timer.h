#ifndef __RTC_HAL_H
#define __RTC_HAL_H

#include <nrf.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

/** This is a fixed PPI channel from RTC0->EVENTS_COMPARE[0] -> NRF_TIMER0->TASKS_START */
#define M_RTC_TIMER_PPICH 	(31)
#define RTC_COMPARE_PRETICK (1)
#define RTC_COMPARE_START 	(0)

#define TIMER_IRQHandler TIMER0_IRQHandler
#define TIMER_IRQn TIMER0_IRQn

#define RTC_IRQHandler RTC0_IRQHandler
#define RTC_IRQn RTC0_IRQn

#define HAL_RADIO_TIMER_EVT_PRETICK 	(0)
#define HAL_RADIO_TIMER_EVT_PRESTART 	(1)
#define HAL_RADIO_TIMER_EVT_START 		(2)
#define HAL_RADIO_TIMER_EVT_TIMEOUT   (3)
#define HAL_RADIO_TIMER_EVT_RADIO			(4)

void radio_timer_handler(uint32_t evt);

void rtc_init(void);

uint32_t rtc_counter_get(void);

void rtc_compare_set(uint32_t compare_value, uint32_t compare_reg, bool enable_irq);

void rtc_compare_clr(uint32_t compare_reg);

void timer_init(void);

void timer_compare_set(uint32_t compare_value);

void timer_compare_clr(void);

void timer_timeout_set(uint32_t compare_value);

void timer_timeout_clr(void);

uint32_t timer_timeout_evt_get(void);


#endif