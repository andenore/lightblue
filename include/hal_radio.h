/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_RADIO_
#define __HAL_RADIO_

#include <stdint.h>
#include <stdbool.h>

#define HAL_RADIO_MODE        (RADIO_MODE_MODE_Nrf_250Kbit)
#define HAL_RADIO_US_PER_BIT  (4)

#define HAL_RADIO_RXEN_TO_READY_US    (150)
#define HAL_RADIO_AA_AND_ADDR_LEN_US  (8*5*HAL_RADIO_US_PER_BIT)

void hal_radio_init(void);
void hal_radio_pkt_configure(uint8_t preamble16, uint8_t bits_len, uint8_t max_len);

void hal_radio_packetptr_set(uint8_t *buf);
void hal_radio_frequency_set(uint32_t freq);
void hal_radio_txpower_set(uint32_t power);

void hal_radio_access_address_set(uint8_t *aa);

void hal_radio_crc_configure(uint32_t polynomial, uint32_t iv);
bool hal_radio_crc_match(void);

void hal_radio_datawhiteiv_set(uint32_t channel);

void hal_radio_disable_on_tmo_evt_set(void);
void hal_radio_disable_on_tmo_evt_clr(void);

void hal_radio_start_tx(void);
void hal_radio_start_tx_on_start_evt(void);
void hal_radio_start_rx(void);
void hal_radio_start_rx_on_start_evt(void);

uint32_t hal_radio_start_to_address_time_get(void);

#endif // __HAL_RADIO_