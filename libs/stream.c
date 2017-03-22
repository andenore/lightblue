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
#include "SEGGER_RTT.h"

#define M_STREAM_ACCESS_ADDR (0x8E89BED6)
#define M_STREAM_CRC_INIT (0x00555555)
#define M_STREAM_Q_NUM (8)
#define M_STREAM_DATA_LEN (255)


/** Local functions */
static void m_stream_rx_pkt(void);

static radio_timer_t m_stream_timer;
static struct pdu_stream m_pdu;

typedef struct {
	uint8_t len;
	uint8_t buf[M_STREAM_DATA_LEN];
} m_stream_data_t;

typedef struct {
	m_stream_data_t data[M_STREAM_Q_NUM];
	volatile uint32_t head;
	volatile uint32_t tail;
} m_stream_q_t;

m_stream_q_t m_stream_q;

void m_stream_q_init(void)
{
	m_stream_q.head = m_stream_q.tail = 0;
}

int stream_q_put(uint8_t *data, uint8_t len)
{
	ASSERT(len <= M_STREAM_DATA_LEN);
	uint32_t next = m_stream_q.tail + 1;
	if (next == M_STREAM_Q_NUM)
	{
		next = 0;
	}
	if (next != m_stream_q.head)
	{
		memcpy(&m_stream_q.data[next].buf[0], data, len);
		m_stream_q.data[next].len = len;
		m_stream_q.tail = next;
	}
	else
	{
		return -1;
	}

	return 0;
}

uint8_t * stream_q_head_peek(void)
{
	uint8_t *retval;
	ASSERT(m_stream_q.head != m_stream_q.tail);

	retval = &m_stream_q.data[m_stream_q.head].buf[0];
	return retval;
}

/**@todo make into copy get */
uint8_t * stream_q_get(void)
{
	uint8_t *retval = &m_stream_q.data[m_stream_q.head].buf[0];
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

	m_stream_data_t *p_pdu = NULL;

	switch (state)
	{
		case RADIO_TIMER_SIG_PREPARE:
			hal_radio_init();
			hal_radio_pkt_configure(0, 8, M_STREAM_DATA_LEN);
			channel_set(15);
			hal_radio_access_address_set((uint8_t *)&aa);
			hal_radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)), M_STREAM_CRC_INIT);
			hal_radio_packetptr_set((uint8_t *)&m_pdu);

			if (!stream_q_empty())
			{
				p_pdu = (m_stream_data_t *)stream_q_get();
				ASSERT(p_pdu->len <= M_STREAM_DATA_LEN);
				m_pdu.len = p_pdu->len;
				memcpy(&m_pdu.payload.data[0], &p_pdu->buf[0], p_pdu->len);
			}
			else
			{
				printf("empty packet\n");
				m_pdu.len = 0;
			}

			break;

		case RADIO_TIMER_SIG_START:
			DEBUG_TOGGLE(0);
			
			ASSERT((NRF_CLOCK->HFCLKSTAT & (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk)) == (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk));
			hal_radio_start_tx();

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

	// m_stream_data_t *p_pdu = NULL;

	switch (state)
	{
		case RADIO_TIMER_SIG_PREPARE:
			hal_radio_init();
			hal_radio_pkt_configure(0, 8, M_STREAM_DATA_LEN);
			channel_set(15);
			hal_radio_access_address_set((uint8_t *)&aa);
			hal_radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)), M_STREAM_CRC_INIT);
			hal_radio_packetptr_set((uint8_t *)&m_pdu);
			radio_timer_timeout_set(1000);
			hal_radio_disable_on_tmo_evt_set();

			hal_radio_start_rx_on_start_evt();

			printf("prepare\n");

			break;

		case RADIO_TIMER_SIG_START:
			DEBUG_TOGGLE(0);
			printf("start\n");
			
			ASSERT((NRF_CLOCK->HFCLKSTAT & (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk)) == (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk));
			
			break;

		case RADIO_TIMER_SIG_TIMEOUT:
			DEBUG_TOGGLE(1);
			printf("timeout\n");

			m_stream_timer.start_us += 10000;
			m_stream_timer.func = m_stream_rx_handler;

			err = radio_timer_req(&m_stream_timer);
			ASSERT(err == 0);

			radio_timer_sig_end();
			break;

		case RADIO_TIMER_SIG_RADIO:
			m_stream_rx_pkt();
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
		if (!stream_q_empty())
		{
			stream_q_put(&m_pdu.payload.data[0], m_pdu.len);
			/**@todo notify of packet reception */
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

	printf("Starting stream...\n");

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

	printf("Starting stream...\n");

	radio_timer_init();

	memset(&m_stream_timer, 0, sizeof(radio_timer_t));

	m_stream_timer.start_us = radio_timer_time_get() + 500000;
	m_stream_timer.func = m_stream_rx_handler;
	err = radio_timer_req(&m_stream_timer);
	ASSERT(err == 0);
}