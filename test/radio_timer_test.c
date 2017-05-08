/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <radio_timer.h>
#include <debug.h>
#include <assert.h>
#include "SEGGER_RTT.h"

#define M_RADIO_TIMER_BITS 									(24)
#define M_RADIO_TIMER_MAX_VAL 							((1 << M_RADIO_TIMER_BITS)-1)
#define M_RADIO_TIMER_MAX_VAL_US						(512000000)
#define M_RADIO_TIMER_MAX_SCHED_INTERVAL_US	(M_RADIO_TIMER_MAX_VAL_US / 2)
#define M_RADIO_TIMER_US_TO_FEMTO(x)				((x)*(uint64_t)1000000000)
#define M_RADIO_TIMER_UNIT_FEMTO						((uint64_t)30517578125)
#define M_RADIO_TIMER_FEMTO_TO_US_FACTOR    (1000000000)

static bool m_tests_passed = true;

void assert_handler(char *buf, uint16_t line)
{
	DEBUG_SET(3);

	printf("Test failed: %s @ %d\n", buf, line);
	m_tests_passed = false;
}

void radio_timeout(uint32_t state)
{
	DEBUG_SET(3);

	while (1);
}

void HardFault_Handler(void)
{
	DEBUG_SET(3);
	printf("HardFault Handler\n");

	while (1);
}

static void m_check_internal(radio_timer_t *p_timer, uint32_t start_us)
{
	// uint32_t start_rtc_unit, start_timer_unit;
	// start_rtc_unit = ((uint64_t)M_RADIO_TIMER_US_TO_FEMTO(p_timer->start_us) + (M_RADIO_TIMER_UNIT_FEMTO/2)) / M_RADIO_TIMER_UNIT_FEMTO;
	// start_rtc_unit = (start_rtc_unit - 2) & M_RADIO_TIMER_MAX_VAL;
	// start_timer_unit = (M_RADIO_TIMER_US_TO_FEMTO(start_us) - ((uint64_t)p_timer->_internal.start_rtc_unit * M_RADIO_TIMER_UNIT_FEMTO)) / M_RADIO_TIMER_FEMTO_TO_US_FACTOR; 
	ASSERT(p_timer->_internal.start_timer_unit >= 45 && 
				 p_timer->_internal.start_timer_unit <= 61+16)
}

TEST_BEGIN(test_sequential_schedule)
	uint32_t err;
	radio_timer_t timer0, timer1, timer2;

	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));
	memset(&timer2, 0, sizeof(timer2));

	radio_timer_init();

	timer0.func = radio_timeout;
	timer0.start_us = 500;
	err = radio_timer_req(&timer0);
	ASSERT(err == 0);

	timer1.func = radio_timeout;
	timer1.start_us = 1000;
	err = radio_timer_req(&timer1);
	ASSERT(err == 0);

	timer2.func = radio_timeout;
	timer2.start_us = 1500;
	err = radio_timer_req(&timer2);
	ASSERT(err == 0);

	ASSERT(timer0._internal.next == &timer1);
	ASSERT(timer1._internal.next == &timer2);
	ASSERT(timer2._internal.next == NULL);

	ASSERT(radio_timer_head_get() == &timer0);
TEST_END()

TEST_BEGIN(test_insert_between_events)
	uint32_t err;
	radio_timer_t timer0, timer1, timer2;

	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));
	memset(&timer2, 0, sizeof(timer2));

	radio_timer_init();

	timer0.func = radio_timeout;
	timer0.start_us = 500;
	err = radio_timer_req(&timer0);
	ASSERT(err == 0);

	timer1.func = radio_timeout;
	timer1.start_us = 1500;
	err = radio_timer_req(&timer1);
	ASSERT(err == 0);

	timer2.func = radio_timeout;
	timer2.start_us = 1000;
	err = radio_timer_req(&timer2);
	ASSERT(err == 0);

	ASSERT(timer0._internal.next == &timer2);
	ASSERT(timer1._internal.next == NULL);
	ASSERT(timer2._internal.next == &timer1);
	ASSERT(radio_timer_head_get() == &timer0);
TEST_END()

TEST_BEGIN(test_request_new_head)
	uint32_t err;
	radio_timer_t timer0, timer1, timer2;

	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));
	memset(&timer2, 0, sizeof(timer2));

	radio_timer_init();

	timer0.func = radio_timeout;
	timer0.start_us = 1500;
	err = radio_timer_req(&timer0);
	ASSERT(err == 0);

	timer1.func = radio_timeout;
	timer1.start_us = 1000;
	err = radio_timer_req(&timer1);
	ASSERT(err == 0);

	timer2.func = radio_timeout;
	timer2.start_us = 500;
	err = radio_timer_req(&timer2);
	ASSERT(err == 0);

	ASSERT(timer0._internal.next == NULL);
	ASSERT(timer1._internal.next == &timer0);
	ASSERT(timer2._internal.next == &timer1);
	ASSERT(radio_timer_head_get() == &timer2);
TEST_END()

TEST_BEGIN(test_request_wraps_around)
	uint32_t err;
	radio_timer_t timer0, timer1, timer2;

	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));
	memset(&timer2, 0, sizeof(timer2));

	radio_timer_init();

	timer0.func = radio_timeout;
	timer0.start_us = 511000000;
	err = radio_timer_req(&timer0);
	ASSERT(err == 0);

	timer1.func = radio_timeout;
	timer1.start_us = 50;
	err = radio_timer_req(&timer1);
	ASSERT(err == 0);

	timer2.func = radio_timeout;
	timer2.start_us = 500;
	err = radio_timer_req(&timer2);
	ASSERT(err == 0);

	ASSERT(timer0._internal.next == &timer1);
	ASSERT(timer1._internal.next == &timer2);
	ASSERT(timer2._internal.next == NULL);
TEST_END()

TEST_BEGIN(test_request_before_event_with_wraparound)
	uint32_t err;
	radio_timer_t timer0, timer1, timer2;

	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));
	memset(&timer2, 0, sizeof(timer2));

	radio_timer_init();

	timer0.func = radio_timeout;
	timer0.start_us = 50;
	err = radio_timer_req(&timer0);
	ASSERT(err == 0);

	timer1.func = radio_timeout;
	timer1.start_us = 511000000;
	err = radio_timer_req(&timer1);
	ASSERT(err == 0);

	timer2.func = radio_timeout;
	timer2.start_us = 500;
	err = radio_timer_req(&timer2);
	ASSERT(err == 0);

	ASSERT(timer0._internal.next == &timer2);
	ASSERT(timer1._internal.next == &timer0);
	ASSERT(timer2._internal.next == NULL);
TEST_END()

TEST_BEGIN(test_request_just_shorter_than_allowed)
	uint32_t err;
	radio_timer_t timer0, timer1, timer2;
	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));
	memset(&timer2, 0, sizeof(timer2));

	radio_timer_init();

	timer0.func = radio_timeout;
	timer0.start_us = 50;
	err = radio_timer_req(&timer0);
	ASSERT(err == 0);

	timer1.func = radio_timeout;
	timer1.start_us = 40 + (1 << 23);
	err = radio_timer_req(&timer1);
	ASSERT(err == 0);

	timer2.func = radio_timeout;
	timer2.start_us = 49 + (1 << 23);
	err = radio_timer_req(&timer2);
	ASSERT(err == 0);

	ASSERT(timer0._internal.next == &timer1);
	ASSERT(timer1._internal.next == &timer2);
	ASSERT(timer2._internal.next == NULL);
TEST_END()

TEST_BEGIN(test_request_start_and_prepare_bigger_than_allowed)
	uint32_t err;
	radio_timer_t timer0, timer1, timer2;
	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));
	memset(&timer2, 0, sizeof(timer2));

	radio_timer_init();

	timer0.func = radio_timeout;
	timer0.start_us = M_RADIO_TIMER_MAX_VAL_US + 10000;
	err = radio_timer_req(&timer0);
	ASSERT(err == 0);

	m_check_internal(&timer0, M_RADIO_TIMER_MAX_VAL_US + 10000);
TEST_END()

TEST_BEGIN(test_request_start_bigger_than_allowed)
	uint32_t err;
	radio_timer_t timer0, timer1, timer2;
	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));
	memset(&timer2, 0, sizeof(timer2));

	radio_timer_init();

	timer0.func = radio_timeout;
	timer0.start_us = M_RADIO_TIMER_MAX_VAL_US + 900;
	err = radio_timer_req(&timer0);
	ASSERT(err == 0);

	m_check_internal(&timer0, M_RADIO_TIMER_MAX_VAL_US + 900);
TEST_END()

TEST_BEGIN(test_request_at_zero)
	uint32_t err;
	radio_timer_t timer0, timer1, timer2;
	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));
	memset(&timer2, 0, sizeof(timer2));

	radio_timer_init();

	timer0.func = radio_timeout;
	timer0.start_us = 0;
	err = radio_timer_req(&timer0);
	ASSERT(err == 0);

	m_check_internal(&timer0, 0);
TEST_END()

TEST_BEGIN(test_request_before_pretick_length)
	uint32_t err;
	radio_timer_t timer0, timer1, timer2;
	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));
	memset(&timer2, 0, sizeof(timer2));

	radio_timer_init();

	timer0.func = radio_timeout;
	timer0.start_us = 900;
	err = radio_timer_req(&timer0);
	ASSERT(err == 0);

	m_check_internal(&timer0, 900);
TEST_END()


int main(void)
{
	DEBUG_INIT();

	printf("\nStarting radio_timer_test\n");

	// test_sequential_schedule();
	// test_insert_between_events();
	// test_request_new_head();
	// test_request_wraps_around();
	// test_request_before_event_with_wraparound();
	// test_request_just_shorter_than_allowed();
	test_request_start_and_prepare_bigger_than_allowed();
	test_request_start_bigger_than_allowed();
	test_request_at_zero();
	test_request_before_pretick_length();
	

	if (m_tests_passed)
	{
		printf("Tests passed\n");
		DEBUG_SET(0);
	}
	else
	{
		printf("Tests failed\n");
	}
	

	while (1)
	{
	}
	return 0;
}