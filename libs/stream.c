/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <nrf.h>
#include <radio_timer.h>
#include <pdu.h>
#include <debug.h>
#include <assert.h>
#include <hal_radio.h>
#include <stream.h>
#include "SEGGER_RTT.h"

#define M_TIMING_ACCURACY_PPM		(500)
#define M_MISSED_EVENTS_MAX 		(5)
#define M_CONN_INTERVAL_US 			(10000)
#define M_CONN_INTERVAL_MARGIN	((M_TIMING_ACCURACY_PPM * M_CONN_INTERVAL_US) / 1000000)

#define M_STREAM_ACCESS_ADDR 	(0x8E89BED6)
#define M_STREAM_CRC_INIT 		(0x00555555)
#define M_STREAM_Q_NUM 				(8)

/** Local functions */
static void m_stream_rx_pkt(void);

static radio_timer_t m_stream_timer;
static struct pdu_stream m_pdu;

typedef enum {
	STREAM_RX_STATE_SCANNING = 0,
	STREAM_RX_STATE_CONNECTED,
} m_stream_rx_state_t;

typedef struct {
	m_stream_rx_state_t state;
	uint8_t missed_event_counter;
} m_stream_rx_t;

typedef struct {
	stream_data_t data[M_STREAM_Q_NUM];
	volatile uint32_t head;
	volatile uint32_t tail;
} m_stream_q_t;


static m_stream_rx_t m_stream_rx;
static volatile m_stream_q_t m_stream_q;

void m_stream_q_init(void)
{
	m_stream_q.head = m_stream_q.tail = 0;
}

int stream_q_put(stream_data_t *p_data)
{
	if (p_data) ASSERT(p_data->len <= M_STREAM_DATA_LEN);
	
	uint32_t next = m_stream_q.tail + 1;
	if (next == M_STREAM_Q_NUM)
	{
		next = 0;
	}

	if (next != m_stream_q.head)
	{
		if (p_data != NULL)
		{
			memcpy(&m_stream_q.data[m_stream_q.tail], p_data, sizeof(stream_data_t));
		}
		m_stream_q.tail = next;
	}
	else
	{
		return -1;
	}

	return 0;
}

stream_data_t * stream_q_head_peek(void)
{
	stream_data_t *retval;
	ASSERT(m_stream_q.head != m_stream_q.tail);

	retval = &m_stream_q.data[m_stream_q.head];
	return retval;
}

stream_data_t * stream_q_tail_peek(void)
{
	ASSERT(!stream_q_full());

	return &m_stream_q.data[m_stream_q.tail];
}

/**@todo make into copy get */
stream_data_t * stream_q_get(void)
{
	stream_data_t *retval = &m_stream_q.data[m_stream_q.head];
	uint32_t next;

	ASSERT(m_stream_q.head != m_stream_q.tail);

	next = m_stream_q.head + 1;
	if (next == M_STREAM_Q_NUM)
	{
		next = 0;
	}
	m_stream_q.head = next;

	return retval;
}

bool stream_q_empty(void)
{
	return (m_stream_q.tail == m_stream_q.head);
}

bool stream_q_full(void)
{
	uint32_t next = m_stream_q.tail + 1;
	if (next == M_STREAM_Q_NUM)
	{
		next = 0;
	}
	return (next == m_stream_q.head);
}


void stream_packet_print(stream_data_t *p_data)
{
	printf("Len = %d,  timestamp = %d\n", p_data->len, p_data->timestamp);
	printf("Data = ");
	for (int i=0; i < p_data->len; i++) {
		printf("%02x", p_data->buf[i]);
	}
	printf("\n");
}

static void channel_set(uint32_t channel)
{
	switch (channel) {
	case 37:
		hal_radio_frequency_set(2);
		break;

	case 38:
		hal_radio_frequency_set(26);
		break;

	case 39:
		hal_radio_frequency_set(80);
		break;

	default:
		if (channel < 11) {
			hal_radio_frequency_set(4 + (2 * channel));
		} else if (channel < 40) {
			hal_radio_frequency_set(28 + (2 * (channel - 11)));
		} else {
			ASSERT(0);
		}
		break;
	}

	hal_radio_datawhiteiv_set(channel);
}

void m_stream_tx_handler(uint32_t state)
{
	uint32_t err;
	uint32_t aa = M_STREAM_ACCESS_ADDR;

	stream_data_t *p_stream_data = NULL;

	switch (state)
	{
		case RADIO_TIMER_SIG_PREPARE:
			// DEBUG_TOGGLE(0);
			// printf("prepare\n");
			hal_radio_init();
			hal_radio_pkt_configure(0, M_STREAM_PKT_LEN_BITS, M_STREAM_DATA_LEN);
			channel_set(15);
			hal_radio_access_address_set((uint8_t *)&aa);
			hal_radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)), M_STREAM_CRC_INIT);
			hal_radio_packetptr_set((uint8_t *)&m_pdu);
			hal_radio_start_tx_on_start_evt();

			if (!stream_q_empty())
			{
				/* Update m_pdu with head of stream_q */
				p_stream_data = stream_q_head_peek();
				ASSERT(p_stream_data->len <= M_STREAM_DATA_LEN);
				m_pdu.len = p_stream_data->len;
				memcpy(&m_pdu.payload.data[0], &p_stream_data->buf[0], p_stream_data->len);

				// printf("send: 0x%02x %d\n", m_pdu.payload.data[0], m_pdu.len);

				/* dequeue */
				(void)stream_q_get();
			}
			else
			{
				// printf("empty packet\n");
				m_pdu.len = 0;
			}

			break;

		case RADIO_TIMER_SIG_START:
			DEBUG_TOGGLE(1);
			// printf("start\n");
			
			//ASSERT((NRF_CLOCK->HFCLKSTAT & (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk)) == (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk));
			break;

		case RADIO_TIMER_SIG_RADIO:
			m_stream_timer.start_us += M_CONN_INTERVAL_US;
			m_stream_timer.func = m_stream_tx_handler;

			// printf("scheduled = %d, start_to_addr_time = %d\n", m_stream_timer.start_us, 0);

			err = radio_timer_req(&m_stream_timer);
			ASSERT(err == 0);
			radio_timer_sig_end();
			break;

		default:
			ASSERT(false);
			break;
	}
}

void m_stream_rx_handler(uint32_t state)
{
	uint32_t err;
	uint32_t aa = M_STREAM_ACCESS_ADDR;
	int32_t start_to_address_time;

	switch (state)
	{
		case RADIO_TIMER_SIG_PREPARE:
			NRF_TIMER3->TASKS_CLEAR = 1;
			printf("prepare\n");

			hal_radio_init();
			hal_radio_pkt_configure(0, M_STREAM_PKT_LEN_BITS, M_STREAM_DATA_LEN);
			channel_set(15);
			hal_radio_access_address_set((uint8_t *)&aa);
			hal_radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)), M_STREAM_CRC_INIT);
			hal_radio_packetptr_set((uint8_t *)&m_pdu);
			radio_timer_timeout_set(5000);
			hal_radio_disable_on_tmo_evt_set();

			hal_radio_start_rx_on_start_evt();

			break;

		case RADIO_TIMER_SIG_START:
			// DEBUG_TOGGLE(0);
			NRF_TIMER3->TASKS_CAPTURE[0] = 1;
			
			ASSERT((NRF_CLOCK->HFCLKSTAT & (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk)) == (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk));
			
			break;

		case RADIO_TIMER_SIG_TIMEOUT:
			DEBUG_CLR(1);
			DEBUG_SET(0);
			printf("packet loss\n");

			if (m_stream_rx.state == STREAM_RX_STATE_CONNECTED)
			{
				m_stream_rx.missed_event_counter++;
			}

			if (m_stream_rx.missed_event_counter > M_MISSED_EVENTS_MAX) {
				m_stream_rx.state = STREAM_RX_STATE_SCANNING;
				// printf("Lost connection\n");
			}


			if (m_stream_rx.state == STREAM_RX_STATE_CONNECTED)
			{
				m_stream_timer.start_us += M_CONN_INTERVAL_US - M_CONN_INTERVAL_MARGIN;
			}
			else
			{
				m_stream_timer.start_us += M_CONN_INTERVAL_US + 1000; /* todo add pseudo random part*/
			}

			printf("req\n");
			err = radio_timer_req(&m_stream_timer);
			ASSERT(err == 0);

			// printf("scheduled = %d, start_to_addr_time = %d, packet loss\n", m_stream_timer.start_us, 0);

			/* propagate empty packet */
			printf("stream_q_full\n");
			if (!stream_q_full())
			{
				stream_data_t *p_data = stream_q_tail_peek();
				p_data->len = 0;
				p_data->timestamp = 0;
				stream_q_put(NULL);

				NVIC_SetPendingIRQ(STREAM_EVENT_IRQn);
			}

			radio_timer_sig_end();

			NRF_TIMER3->TASKS_CAPTURE[1] = 1;
			printf("tostart = %d, totimeout = %d", NRF_TIMER3->CC[0], NRF_TIMER3->CC[1]);
			break;

		case RADIO_TIMER_SIG_RADIO:
			DEBUG_CLR(0);
			DEBUG_SET(1);
			printf("radio\n");
			m_stream_rx.state = STREAM_RX_STATE_CONNECTED;
			m_stream_rx.missed_event_counter = 0;
			m_stream_rx_pkt();

			start_to_address_time = hal_radio_start_to_address_time_get();
			m_stream_timer.start_us += M_CONN_INTERVAL_US + start_to_address_time - M_CONN_INTERVAL_MARGIN - HAL_RADIO_RXEN_TO_READY_US - HAL_RADIO_AA_AND_ADDR_LEN_US;
			m_stream_timer.func = m_stream_rx_handler;

			// printf("scheduled = %d, start_to_addr_time = %d\n", m_stream_timer.start_us, start_to_address_time);

			err = radio_timer_req(&m_stream_timer);
			ASSERT(err == 0);

			radio_timer_sig_end();
			NRF_TIMER3->TASKS_CAPTURE[1] = 1;
			printf("tostart = %d, toradio = %d", NRF_TIMER3->CC[0], NRF_TIMER3->CC[1]);
			break;

		default:
			ASSERT(false);
			break;
	}
}

static void m_stream_rx_pkt(void)
{
	if (hal_radio_crc_match())
	{
		// printf("Got packet: %d, len = %d\n", (unsigned int)m_pdu.payload.data[0], (unsigned int)m_pdu.len);
		if (!stream_q_full())
		{
			stream_data_t *p_data = stream_q_tail_peek();
			ASSERT(m_pdu.len <= M_STREAM_DATA_LEN);
			p_data->len = m_pdu.len;
			memcpy(&p_data->buf[0], &m_pdu.payload.data[0], m_pdu.len);
			p_data->timestamp = hal_radio_start_to_address_time_get() - M_CONN_INTERVAL_MARGIN - HAL_RADIO_RXEN_TO_READY_US - HAL_RADIO_AA_AND_ADDR_LEN_US;

			printf("stream_q_put\n");
			stream_q_put(NULL);

			NVIC_SetPendingIRQ(STREAM_EVENT_IRQn);
		}
	}
	else
	{
		/** nothing... */
	}
}

void stream_tx_start(void)
{
	uint32_t err;

	m_stream_q_init();

	radio_timer_init();

	memset(&m_stream_timer, 0, sizeof(radio_timer_t));

	m_stream_timer.start_us = radio_timer_time_get() + 500000;
	m_stream_timer.func = m_stream_tx_handler;
	err = radio_timer_req(&m_stream_timer);
	ASSERT(err == 0);
}

void stream_rx_start(void)
{
	uint32_t err;

	NRF_TIMER3->PRESCALER = 4;
	NRF_TIMER3->TASKS_START = 1;

	m_stream_q_init();
	m_stream_rx.state = STREAM_RX_STATE_SCANNING;
	m_stream_rx.missed_event_counter = 0;

	radio_timer_init();

	memset(&m_stream_timer, 0, sizeof(radio_timer_t));

	m_stream_timer.start_us = radio_timer_time_get() + 500000;
	m_stream_timer.func = m_stream_rx_handler;
	err = radio_timer_req(&m_stream_timer);
	ASSERT(err == 0);
}