/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "hal_radio.h"
#include "hal_radio_timer.h"

#define M_PPICH_TIMER0_CC1_TO_RADIO_DISABLE 		(22)
#define M_PPICH_TIMER0_CC0_TO_RADIO_RXEN 				(21)
#define M_PPICH_TIMER0_CC0_TO_RADIO_TXEN 				(20)
#define M_PPICH_RADIO_ADDRESS_TO_TIMER_CAPTURE1 (26)

void hal_radio_init(void)
{
	ASSERT(NRF_RADIO->STATE == (RADIO_STATE_STATE_Disabled << RADIO_STATE_STATE_Pos));

	NRF_RADIO->MODE = HAL_RADIO_MODE << RADIO_MODE_MODE_Pos;

	//x 24 + x 10 + x 9 + x 6 + x 4 + x 3 + x + 1
	// NRF_RADIO->CRCPOLY = (0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16);
	//(1 << 24) | (1 << 10) | (1 << 9) | (1 << 6) | (1 << 4) | (1 << 3) | (1 << 1) | 1;


	NRF_RADIO->TIFS = 150;

	NRF_RADIO->TXPOWER = 4;

	NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Msk;
	NVIC_EnableIRQ(RADIO_IRQn);


	// NRF_RADIO->MODECNF0 = 	(RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos) |
	// 						(RADIO_MODECNF0_DTX_Center << RADIO_MODECNF0_DTX_Pos);
}

void hal_radio_pkt_configure(uint8_t preamble16, uint8_t bits_len, uint8_t max_len)
{

	NRF_RADIO->PCNF0 = ((((1UL) << RADIO_PCNF0_S0LEN_Pos) &
			     RADIO_PCNF0_S0LEN_Msk) |
			     ((((uint32_t)bits_len) << RADIO_PCNF0_LFLEN_Pos) &
			       RADIO_PCNF0_LFLEN_Msk) |
			     (((RADIO_PCNF0_S1INCL_Include) <<
			       RADIO_PCNF0_S1INCL_Pos) &
			       RADIO_PCNF0_S1INCL_Msk) |
			     ((((preamble16) ? RADIO_PCNF0_PLEN_16bit :
			       RADIO_PCNF0_PLEN_8bit) << RADIO_PCNF0_PLEN_Pos) &
			       RADIO_PCNF0_PLEN_Msk) |
			     ((((uint32_t)8-bits_len) <<
			       RADIO_PCNF0_S1LEN_Pos) &
			       RADIO_PCNF0_S1LEN_Msk));

	NRF_RADIO->PCNF1 = (((((uint32_t)max_len) << RADIO_PCNF1_MAXLEN_Pos) &
			     RADIO_PCNF1_MAXLEN_Msk) |
			     (((0UL) << RADIO_PCNF1_STATLEN_Pos) &
			       RADIO_PCNF1_STATLEN_Msk) |
			     (((3UL) << RADIO_PCNF1_BALEN_Pos) &
			       RADIO_PCNF1_BALEN_Msk) |
			     (((RADIO_PCNF1_ENDIAN_Little) <<
			       RADIO_PCNF1_ENDIAN_Pos) &
			      RADIO_PCNF1_ENDIAN_Msk) |
			     (((1UL) << RADIO_PCNF1_WHITEEN_Pos) &
			       RADIO_PCNF1_WHITEEN_Msk));
}

void hal_radio_packetptr_set(uint8_t *buf)
{
	NRF_RADIO->PACKETPTR = (uint32_t)buf;
}

void hal_radio_frequency_set(uint32_t freq)
{
	NRF_RADIO->FREQUENCY = freq;
}

void hal_radio_txpower_set(uint32_t power)
{
	NRF_RADIO->TXPOWER = power;
}

void hal_radio_access_address_set(uint8_t *aa)
{
	NRF_RADIO->TXADDRESS =
	    (((0UL) << RADIO_TXADDRESS_TXADDRESS_Pos) &
	     RADIO_TXADDRESS_TXADDRESS_Msk);
	NRF_RADIO->RXADDRESSES =
	    ((RADIO_RXADDRESSES_ADDR0_Enabled) << RADIO_RXADDRESSES_ADDR0_Pos);
	NRF_RADIO->PREFIX0 = aa[3];
	NRF_RADIO->BASE0 = (aa[2] << 24) | (aa[1] << 16) | (aa[0] << 8);

	// printf("Base address = 0x%08x PREFIX0 = 0x%08x\n", (unsigned int)NRF_RADIO->BASE0, (unsigned int)NRF_RADIO->PREFIX0);
}

void hal_radio_datawhiteiv_set(uint32_t channel)
{
	NRF_RADIO->DATAWHITEIV = channel;
}

void hal_radio_crc_configure(uint32_t polynomial, uint32_t iv)
{
	NRF_RADIO->CRCCNF =
	    (((RADIO_CRCCNF_SKIPADDR_Skip) << RADIO_CRCCNF_SKIPADDR_Pos) &
	     RADIO_CRCCNF_SKIPADDR_Msk) |
	    (((RADIO_CRCCNF_LEN_Three) << RADIO_CRCCNF_LEN_Pos) &
	       RADIO_CRCCNF_LEN_Msk);
	NRF_RADIO->CRCPOLY = polynomial;
	NRF_RADIO->CRCINIT = iv;
}

bool hal_radio_crc_match(void)
{
	bool retval;

	retval = (NRF_RADIO->CRCSTATUS & RADIO_CRCSTATUS_CRCSTATUS_Msk) == (RADIO_CRCSTATUS_CRCSTATUS_CRCOk << RADIO_CRCSTATUS_CRCSTATUS_Pos);

	return retval;
}

void hal_radio_start_tx(void)
{
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;
	NRF_RADIO->TASKS_TXEN = 1;
}

void hal_radio_start_tx_on_start_evt(void)
{
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;
	NRF_PPI->CHENSET = (1UL << M_PPICH_TIMER0_CC0_TO_RADIO_TXEN);
}

void hal_radio_start_rx(void)
{
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;
	NRF_RADIO->TASKS_RXEN = 1;
}

void hal_radio_start_rx_on_start_evt(void)
{
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;
	NRF_PPI->CHENSET = (1UL << M_PPICH_TIMER0_CC0_TO_RADIO_RXEN) | (1UL << M_PPICH_RADIO_ADDRESS_TO_TIMER_CAPTURE1);
}

uint32_t hal_radio_start_to_address_time_get(void)
{
	return NRF_TIMER0->CC[1];
}

void hal_radio_disable_on_tmo_evt_set(void)
{
	/*Configure PPI from evt to NRF_RADIO->TASKS_DISABLE*/
	NRF_PPI->CHENSET = (1UL << M_PPICH_TIMER0_CC1_TO_RADIO_DISABLE);
}

void hal_radio_disable_on_tmo_evt_clr(void)
{
	NRF_PPI->CHENCLR = (1UL << M_PPICH_TIMER0_CC1_TO_RADIO_DISABLE);
}

void RADIO_IRQHandler(void)
{
	if (NRF_RADIO->EVENTS_DISABLED != 0)
	{
		hal_radio_disable_on_tmo_evt_clr();
		NRF_RADIO->EVENTS_DISABLED = 0;

		if (NRF_RADIO->EVENTS_END != 0)
		{
			NRF_RADIO->EVENTS_END = 0;
			radio_timer_handler(HAL_RADIO_TIMER_EVT_RADIO);
		}

	}
}