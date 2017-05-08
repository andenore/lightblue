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

uint32_t m_stream[2][NUM_SAMPLES];
uint8_t i2s_stream_sel = 0;
volatile uint32_t got_packet = 0;
uint32_t m_received_pkts = 0;

uint8_t m_dec_buf[17224];

int main(int argc, char *argv[])
{
  int compressed_length;
  printf("Starting PDM RX stream...\n");

  DEBUG_INIT();

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

  printf("decoder init\n");
  decoder_wrapper_init(m_dec_buf, sizeof(m_dec_buf));


  printf("stream rx start\n");
  NVIC_SetPriority(STREAM_EVENT_IRQn, 3);
  NVIC_EnableIRQ(STREAM_EVENT_IRQn);
  stream_rx_start();

  printf("max9850_start\n");
  max9850_start();

  printf("i2s init\n");

  i2s_init();
  i2s_txptr_cfg(&m_stream[0][0], NUM_SAMPLES);

  printf("i2s_start\n");
  i2s_start();

  NRF_TIMER2->PRESCALER = 4;
  NRF_TIMER2->TASKS_START = 1;

  while (1)
  {    

    if (i2s_txptr_upd() && m_received_pkts > 4)
    {
      int frame_size;

      i2s_stream_sel ^= 0x1;

      if (!stream_q_empty())
      {
        stream_data_t *p_data = stream_q_head_peek();

        NRF_TIMER2->TASKS_CLEAR = 1;
        if (p_data->len != 0)
        {
          frame_size = codec_wrapper_decode(&p_data->buf[0], p_data->len, &m_stream[i2s_stream_sel][0], NUM_SAMPLES);
        }
        else
        {
          frame_size = codec_wrapper_decode(NULL, p_data->len, &m_stream[i2s_stream_sel][0], NUM_SAMPLES);
        }
        NRF_TIMER2->TASKS_CAPTURE[0] = 1;
        printf("Decoded: %d %d in %d us\n", p_data->len, frame_size, NRF_TIMER2->CC[0]);
        if (frame_size <= 0)
        {
          printf("codec_wrapper_decode Err_code = %d\n", frame_size);
        }
        (void)stream_q_get();
      }
      else
      {
        printf("Buffer underrun or packet lost\n");
        m_received_pkts = 0;
        frame_size = codec_wrapper_decode(NULL, 0, &m_stream[i2s_stream_sel][0], NUM_SAMPLES);
        if (frame_size <= 0)
        {
          printf("codec_wrapper_decode Err_code = %d\n", frame_size);
        }

      }

      i2s_txptr_cfg(&m_stream[i2s_stream_sel][0], NUM_SAMPLES);
      // printf("switch stream %d\n", NRF_TIMER2->CC[0]);
    };

    while (got_packet != 0)
    {
      got_packet = 0;
      m_received_pkts++;
      printf("recv_pkts = %d\n", m_received_pkts);
    }

    __WFE();
  }

  return 0;
}

void STREAM_EVENT_IRQHandler(void)
{
  got_packet = 1;
}