/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <nrf.h>
#include <debug.h>
#include <assert.h>
#include <stream.h>
#include "SEGGER_RTT.h"

void assert_handler(char *buf, uint16_t line)
{
  DEBUG_SET(3);

  printf("Assertion %s @ %d\n", buf, line);

  radio_timer_debug_print();

  while (1);
}

void HardFault_Handler(void)
{
  DEBUG_SET(3);
  printf("HardFault_Handler...\n");
  while (1);
}

stream_data_t data;
int32_t m_cumulative_error = 0;

int main(int argc, char *argv[])
{
  printf("Starting RX stream...\n");

  DEBUG_INIT();

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

  memset(&data, 0, sizeof(data));

  NVIC_SetPriority(STREAM_EVENT_IRQn, 3);
  NVIC_EnableIRQ(STREAM_EVENT_IRQn);
  stream_rx_start();

  while (1)
  {
    
    __WFE();
  }

  return 0;
}

void STREAM_EVENT_IRQHandler(void)
{
  stream_data_t *p_data = stream_q_head_peek();
  m_cumulative_error += p_data->timestamp;
  printf("Got new packet 0x%02x %d %d\n", p_data->buf[0], p_data->timestamp, (int)m_cumulative_error);

  (void)stream_q_get();


}