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

/* 10 ms of sound @ 16kHz */
#define NUM_SAMPLES (160)

#define MAX_COMPRESSED_SIZE (255)

void assert_handler(char *buf, uint16_t line)
{
  DEBUG_SET(3);

  printf("Assertion %s @ %d\n", buf, line);

  debug_print();

  while (1);
}

void HardFault_Handler(void)
{
  DEBUG_SET(3);
  printf("HardFault_Handler...\n");

  while (1);
}

stream_data_t data;
uint32_t pdm_samples[2][NUM_SAMPLES];
uint8_t pdm_buf_sel = 0;

static unsigned char out[MAX_COMPRESSED_SIZE];

uint8_t m_enc_buf[8964];

int main(int argc, char *argv[])
{
  int compressed_length;
  uint32_t pdm_end_t0, radio_addr_t0;
  printf("Starting PDM TX stream...\n");

  DEBUG_INIT();

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

  memset(&data, 0, sizeof(data));

  encoder_wrapper_init(m_enc_buf, sizeof(m_enc_buf));
  stream_tx_start();
  mic_init();

  mic_rxptr_cfg(&pdm_samples[0][0], NUM_SAMPLES*4);

  mic_start();

  NRF_TIMER2->PRESCALER = 4;
  NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
  NRF_TIMER2->TASKS_START = 1;

  NRF_PPI->CH[0].TEP = (uint32_t)&NRF_TIMER2->TASKS_CAPTURE[0];
  NRF_PPI->CH[0].EEP = (uint32_t)&NRF_PDM->EVENTS_END;
  NRF_PPI->CH[1].TEP = (uint32_t)&NRF_TIMER2->TASKS_CAPTURE[1];
  NRF_PPI->CH[1].EEP = (uint32_t)&NRF_RADIO->EVENTS_ADDRESS;
  NRF_PPI->CHENSET = 3;


  while (1)
  {
    if (mic_rxptr_upd()) 
    {

      // printf("time diff: %d %d\n", NRF_TIMER2->CC[0] - pdm_end_t0, NRF_TIMER2->CC[1] - radio_addr_t0);
      pdm_end_t0 = NRF_TIMER2->CC[0];
      radio_addr_t0 = NRF_TIMER2->CC[1];

      pdm_buf_sel ^= 0x1;
      mic_rxptr_cfg(&pdm_samples[pdm_buf_sel][0], NUM_SAMPLES*4);
    }

    if (mic_events_end()) 
    {
      compressed_length = codec_wrapper_encode(&pdm_samples[pdm_buf_sel ^ 0x1][0], out, sizeof(out));
      memcpy(&data.buf[0], &out[0], compressed_length);
      data.len = compressed_length;

      if (stream_q_put(&data) == 0)
      {
        // printf("new packet: len = %d\n", compressed_length);
      }
      else
      {
        printf("overrun\n");
        //ASSERT(false);
      }
    }
    
    __WFE();

  }

  return 0;
}
