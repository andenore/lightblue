#include <nrf.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <radio_timer.h>
#include <debug.h>
#include <assert.h>
#include "SEGGER_RTT.h"

void assert_handler(char *buf, uint16_t line)
{
	DEBUG_SET(3);

	printf("Test failed: %s @ %d\n", buf, line);

	while (1);
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

void test_sequential_schedule(void)
{
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

	ASSERT(timer0._next == &timer1);
	ASSERT(timer1._next == &timer2)
	ASSERT(timer2._next == NULL);

	ASSERT(radio_timer_head_get() == &timer0);
}

void test_insert_between_events(void)
{
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

	ASSERT(timer0._next == &timer2);
	ASSERT(timer1._next == NULL)
	ASSERT(timer2._next == &timer1);
	ASSERT(radio_timer_head_get() == &timer0);
}

void test_request_new_head(void)
{
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

	ASSERT(timer0._next == NULL);
	ASSERT(timer1._next == &timer0)
	ASSERT(timer2._next == &timer1);
	ASSERT(radio_timer_head_get() == &timer2);
}

void test_request_wraps_around(void)
{
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

	ASSERT(timer0._next == &timer1);
	ASSERT(timer1._next == &timer2)
	ASSERT(timer2._next == NULL);
}

void test_request_before_event_with_wraparound(void)
{
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

	ASSERT(timer0._next == &timer2);
	ASSERT(timer1._next == &timer0)
	ASSERT(timer2._next == NULL);
}

void test_request_just_shorter_than_allowed(void)
{
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

	ASSERT(timer0._next == &timer1);
	ASSERT(timer1._next == &timer2)
	ASSERT(timer2._next == NULL);
}

int main(void)
{
	DEBUG_INIT();

	printf("Starting radio_timer_test\n");

	test_sequential_schedule();
	test_insert_between_events();
	test_request_new_head();
	test_request_wraps_around();
	test_request_before_event_with_wraparound();
	test_request_just_shorter_than_allowed();


	DEBUG_SET(0);
	printf("Tests passed\n");

	while (1)
	{
	}
	return 0;
}