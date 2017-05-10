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
#include "SEGGER_RTT.h"

#define M_ADV_ACCESS_ADDR (0x8E89BED6)
#define M_ADV_CRC_INIT 		(0x00555555)
#define M_RADIO_MODE 			(RADIO_MODE_MODE_Ble_1Mbit)

static radio_timer_t m_adv_timer;
static struct pdu_adv *adv_pdu;

uint32_t pkt[32];

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

void m_adv_timeout_handler(uint32_t state)
{
	uint32_t err;
	uint32_t aa = 0x8e89bed6;
	uint8_t *packet = NULL;

	switch (state)
	{
		case RADIO_TIMER_SIG_PREPARE:
			printf("prepare\n");
			hal_radio_init(M_RADIO_MODE);
			hal_radio_pkt_configure(0, 6, 37);
			channel_set(37);
			hal_radio_access_address_set((uint8_t *)&aa);
			hal_radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)), 0x555555);

			adv_pdu = (struct pdu_adv *)&pkt[0];
			hal_radio_packetptr_set((uint8_t *)adv_pdu);

			
			adv_pdu->type = PDU_ADV_TYPE_NONCONN_IND;
			adv_pdu->tx_addr = 1;
			adv_pdu->len = 6 + 7;
			memcpy(adv_pdu->payload.adv_ind.addr, (uint8_t *)&NRF_FICR->DEVICEADDR[0], 6);
			adv_pdu->payload.adv_ind.addr[0] = 0xFF;
			adv_pdu->payload.adv_ind.addr[5] = 0xFF;
			adv_pdu->payload.adv_ind.data[0] = 0x2;
			adv_pdu->payload.adv_ind.data[1] = 0x1;
			adv_pdu->payload.adv_ind.data[2] = 0x6;
			adv_pdu->payload.adv_ind.data[3] = 0x3;
			adv_pdu->payload.adv_ind.data[4] = 0x9;
			adv_pdu->payload.adv_ind.data[5] = 'a';
			adv_pdu->payload.adv_ind.data[6] = 'b';

			packet = (uint8_t *)&adv_pdu;

			// printf("RANDOMADDR = %d, DEVICEADDR = 0x%02x%02x%02x%02x%02x%02x\n", (int)(NRF_FICR->DEVICEADDRTYPE & 0x1), packet[2], packet[3], packet[4], packet[5], packet[6], packet[7]);
			break;

		case RADIO_TIMER_SIG_START:
			printf("start\n");


			uint8_t *ptr = (uint8_t *)NRF_RADIO->PACKETPTR;
			uint8_t i;
			for (i=0; i<15; i++)
			{
				// printf("x[%d] = %02x ", (int)i, ptr[i]);
			}
			// printf("\n");
			
			ASSERT((NRF_CLOCK->HFCLKSTAT & (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk)) == (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk));
			hal_radio_start_tx();
			break;

		case RADIO_TIMER_SIG_RADIO:
			printf("radio\n");
			m_adv_timer.start_us += 66000;
			m_adv_timer.func = m_adv_timeout_handler;

			err = radio_timer_req(&m_adv_timer);
			ASSERT(err == 0);

			radio_timer_sig_end();
			break;

		default:
			ASSERT(false);
			break;
	}
}

void adv_start(void)
{
	uint32_t err;

	printf("Starting advertiser...\n");

	radio_timer_init();

	memset(&m_adv_timer, 0, sizeof(radio_timer_t));

	m_adv_timer.start_us = radio_timer_time_get() + 500000;
	m_adv_timer.func = m_adv_timeout_handler;
	err = radio_timer_req(&m_adv_timer);
	ASSERT(err == 0);
}