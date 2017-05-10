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
#define MAX_COMPRESSED_SIZE (M_STREAM_DATA_LEN)

#define BUTTON_VOLUME_UP    (12)
#define BUTTON_VOLUME_DOWN  (2)

void assert_handler(char *buf, uint16_t line)
{
  DEBUG_CLR(0);
  DEBUG_CLR(1);
  DEBUG_SET(2);

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


void button_cfg(void)
{
  NRF_GPIO->PIN_CNF[BUTTON_VOLUME_UP] = 
                          GPIO_PIN_CNF_DIR_Input     << GPIO_PIN_CNF_DIR_Pos |
                          GPIO_PIN_CNF_DRIVE_S0S1    << GPIO_PIN_CNF_DRIVE_Pos |
                          GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos | 
                          GPIO_PIN_CNF_PULL_Pullup   << GPIO_PIN_CNF_PULL_Pos | 
                          GPIO_PIN_CNF_SENSE_Low     << GPIO_PIN_CNF_SENSE_Pos;
  
  NRF_GPIO->PIN_CNF[BUTTON_VOLUME_DOWN] = 
                          GPIO_PIN_CNF_DIR_Input     << GPIO_PIN_CNF_DIR_Pos |
                          GPIO_PIN_CNF_DRIVE_S0S1    << GPIO_PIN_CNF_DRIVE_Pos |
                          GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos | 
                          GPIO_PIN_CNF_PULL_Pullup   << GPIO_PIN_CNF_PULL_Pos | 
                          GPIO_PIN_CNF_SENSE_Low     << GPIO_PIN_CNF_SENSE_Pos;
  
  NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Enabled << GPIOTE_INTENSET_PORT_Pos;
  NVIC_SetPriority(GPIOTE_IRQn, 3);
  NVIC_EnableIRQ(GPIOTE_IRQn);
}

void GPIOTE_IRQHandler(void)
{
  NRF_GPIOTE->EVENTS_PORT = 0;
  if ((NRF_GPIO->IN & (1UL << BUTTON_VOLUME_UP)) == 0)
  {
    printf("up\n");
    mic_gain_up();
  }
  
  if ((NRF_GPIO->IN & (1UL << BUTTON_VOLUME_DOWN)) == 0)
  {
    printf("down\n");
    mic_gain_down();
  }
}

int main(int argc, char *argv[])
{
  int compressed_length;
  uint32_t pdm_end_t0, radio_addr_t0, encode_t0;
  printf("Starting PDM TX stream...\n");

  DEBUG_INIT();

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

  memset(&data, 0, sizeof(data));

  button_cfg();
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
      NRF_TIMER2->TASKS_CAPTURE[2] = 1;
      encode_t0 = NRF_TIMER2->CC[2];
      compressed_length = codec_wrapper_encode(&pdm_samples[pdm_buf_sel ^ 0x1][0], out, sizeof(out));
      NRF_TIMER2->TASKS_CAPTURE[2] = 1;
      printf("encode time = %d\n", NRF_TIMER2->CC[2] - encode_t0);

      memcpy(&data.buf[0], &out[0], compressed_length);
      data.len = compressed_length;
      ASSERT(compressed_length <= MAX_COMPRESSED_SIZE);

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
