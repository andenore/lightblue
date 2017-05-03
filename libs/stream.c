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

#define M_STREAM_ACCESS_ADDR (0x8E89BED6)
#define M_STREAM_CRC_INIT (0x00555555)
#define M_STREAM_Q_NUM (8)

/** Local functions */
static void m_stream_rx_pkt(void);

static radio_timer_t m_stream_timer;
static struct pdu_stream m_pdu;

typedef struct {
	stream_data_t data[M_STREAM_Q_NUM];
	volatile uint32_t head;
	volatile uint32_t tail;
} m_stream_q_t;

m_stream_q_t m_stream_q;

void m_stream_q_init(void)
{
	m_stream_q.head = m_stream_q.tail = 0;
}

int stream_q_put(stream_data_t *p_data)
{
	ASSERT(p_data->len <= M_STREAM_DATA_LEN);
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
			DEBUG_TOGGLE(0);
			printf("prepare\n");
			hal_radio_init();
			hal_radio_pkt_configure(0, 8, M_STREAM_DATA_LEN);
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

				printf("send: 0x%02x %d\n", m_pdu.payload.data[0], m_pdu.len);

				/* dequeue */
				(void)stream_q_get();
			}
			else
			{
				printf("empty packet\n");
				m_pdu.len = 0;
			}

			break;

		case RADIO_TIMER_SIG_START:
			DEBUG_TOGGLE(1);
			printf("start\n");
			
			ASSERT((NRF_CLOCK->HFCLKSTAT & (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk)) == (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk));
			break;

		case RADIO_TIMER_SIG_RADIO:
			m_stream_timer.start_us += 10000;
			m_stream_timer.func = m_stream_tx_handler;

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
	static int32_t cumulative_start_to_address_time = 0;

	// stream_data_t *p_pdu = NULL;

	switch (state)
	{
		case RADIO_TIMER_SIG_PREPARE:
			// printf("prepare\n");

			hal_radio_init();
			hal_radio_pkt_configure(0, 8, M_STREAM_DATA_LEN);
			channel_set(15);
			hal_radio_access_address_set((uint8_t *)&aa);
			hal_radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)), M_STREAM_CRC_INIT);
			hal_radio_packetptr_set((uint8_t *)&m_pdu);
			radio_timer_timeout_set(3000);
			hal_radio_disable_on_tmo_evt_set();

			hal_radio_start_rx_on_start_evt();

			break;

		case RADIO_TIMER_SIG_START:
			DEBUG_TOGGLE(0);
			// printf("start\n");

			// debug_print();
			
			ASSERT((NRF_CLOCK->HFCLKSTAT & (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk)) == (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk));
			
			break;

		case RADIO_TIMER_SIG_TIMEOUT:
			DEBUG_TOGGLE(1);
			// printf("timeout\n");

			m_stream_timer.start_us += 11000;
			m_stream_timer.func = m_stream_rx_handler;

			err = radio_timer_req(&m_stream_timer);
			ASSERT(err == 0);

			radio_timer_sig_end();
			break;

		case RADIO_TIMER_SIG_RADIO:
			// printf("radio\n");
			m_stream_rx_pkt();

			start_to_address_time = hal_radio_start_to_address_time_get();
			cumulative_start_to_address_time += start_to_address_time - 200;
			// printf("start-to-addr: %d  cumulative: %d\n", (int)start_to_address_time, (int)cumulative_start_to_address_time);
			m_stream_timer.start_us += 10000 + start_to_address_time - 200;
			m_stream_timer.func = m_stream_rx_handler;

			err = radio_timer_req(&m_stream_timer);
			ASSERT(err == 0);

			radio_timer_sig_end();
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
			p_data->len = m_pdu.len;
			memcpy(&p_data->buf[0], &m_pdu.payload.data[0], m_pdu.len);
			p_data->timestamp = hal_radio_start_to_address_time_get() - 200;
			stream_q_put(NULL);
			//printf("Got packet: %d", m_pdu.payload.data[0]);
			/**@todo notify of packet reception */

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

	m_stream_q_init();

	radio_timer_init();

	memset(&m_stream_timer, 0, sizeof(radio_timer_t));

	m_stream_timer.start_us = radio_timer_time_get() + 500000;
	m_stream_timer.func = m_stream_rx_handler;
	err = radio_timer_req(&m_stream_timer);
	ASSERT(err == 0);
}