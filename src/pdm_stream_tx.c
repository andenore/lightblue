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
#define NUM_SAMPLES         (160)
#define MAX_COMPRESSED_SIZE (M_STREAM_DATA_LEN)

#define BUTTON_VOLUME_UP    (12)
#define BUTTON_VOLUME_DOWN  (2)

#define PDM_END_TO_TIMER_CAPTURE_PPICH    (0)
#define RADIO_ADDR_TO_TIMER_CAPTURE_PPICH (0)
#define TIMER_TO_MIC_START_PPICH          (2)
#define RADIO_TO_TIMER_START_PPICH        (3)
#define RADIO_TO_MIC_START_US             (5000)
#define SYNC_TIMER                        (NRF_TIMER1)

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
    mic_gain_up();
  }
  
  if ((NRF_GPIO->IN & (1UL << BUTTON_VOLUME_DOWN)) == 0)
  {
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

  //mic_start();  
  SYNC_TIMER->PRESCALER = 4;
  SYNC_TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
  SYNC_TIMER->CC[0] = RADIO_TO_MIC_START_US; 

  /* start mic at a "sync-point" after radio address event is triggered 
   * this point will be used as a reference when syncing radio/pdm */
  NRF_PPI->CH[TIMER_TO_MIC_START_PPICH].TEP   = mic_start_task_get();
  NRF_PPI->CH[TIMER_TO_MIC_START_PPICH].EEP   = (uint32_t)&SYNC_TIMER->EVENTS_COMPARE[0];
  NRF_PPI->CH[RADIO_TO_TIMER_START_PPICH].TEP = (uint32_t)&SYNC_TIMER->TASKS_START;
  NRF_PPI->CH[RADIO_TO_TIMER_START_PPICH].EEP = (uint32_t)&NRF_RADIO->EVENTS_ADDRESS;
  NRF_PPI->CHENSET = (1 << TIMER_TO_MIC_START_PPICH) | (1 << RADIO_TO_TIMER_START_PPICH);

  /* continuous measurement of sync-point */
  NRF_PPI->CH[PDM_END_TO_TIMER_CAPTURE_PPICH].TEP     = (uint32_t)&SYNC_TIMER->TASKS_CAPTURE[0];
  NRF_PPI->CH[PDM_END_TO_TIMER_CAPTURE_PPICH].EEP     = (uint32_t)&NRF_PDM->EVENTS_END;
  NRF_PPI->CH[RADIO_ADDR_TO_TIMER_CAPTURE_PPICH].TEP  = (uint32_t)&SYNC_TIMER->TASKS_CAPTURE[1];
  NRF_PPI->CH[RADIO_ADDR_TO_TIMER_CAPTURE_PPICH].EEP  = (uint32_t)&NRF_RADIO->EVENTS_ADDRESS;
  NRF_PPI->CHENSET = (1 << PDM_END_TO_TIMER_CAPTURE_PPICH) | (1 << RADIO_ADDR_TO_TIMER_CAPTURE_PPICH);

  while (1)
  {
    if (mic_rxptr_upd()) 
    {
      pdm_buf_sel ^= 0x1;
      mic_rxptr_cfg(&pdm_samples[pdm_buf_sel][0], NUM_SAMPLES*4);
    }

    if (mic_events_end()) 
    {
      /* measured sync-point */
      uint32_t radio_to_mic_us = SYNC_TIMER->CC[0] - SYNC_TIMER->CC[1];

      /* stop ppi-channels used for inital start of mic */
      NRF_PPI->CHENCLR = (1 << TIMER_TO_MIC_START_PPICH) | (1 << RADIO_TO_TIMER_START_PPICH);

      // printf("radio to pdm: %d\n", radio_to_mic_us);
      // printf("time diff: %d %d\n", SYNC_TIMER->CC[0] - pdm_end_t0, SYNC_TIMER->CC[1] - radio_addr_t0);
      // pdm_end_t0 = SYNC_TIMER->CC[0];
      // radio_addr_t0 = SYNC_TIMER->CC[1];

      /** adjust mic frequency based on sync point 
          if measured sync-point is below ideal sync-point, 
            - decrease mic frequency (mic is going to fast)
          otherwise 
            - increase mic frequency (mic is going too slow)
       */
      if (radio_to_mic_us < RADIO_TO_MIC_START_US) 
      {
        mic_freq_set(PDM_PDMCLKCTRL_FREQ_1000K);
      } 
      else
      {
        mic_freq_set(PDM_PDMCLKCTRL_FREQ_Default);
      }

      SYNC_TIMER->TASKS_CAPTURE[2] = 1;
      encode_t0 = SYNC_TIMER->CC[2];
      compressed_length = codec_wrapper_encode(&pdm_samples[pdm_buf_sel ^ 0x1][0], out, sizeof(out));
      SYNC_TIMER->TASKS_CAPTURE[2] = 1;
      // printf("encode time = %d\n", SYNC_TIMER->CC[2] - encode_t0);

      memcpy(&data.buf[0], &out[0], compressed_length);
      data.len = compressed_length;
      ASSERT(compressed_length <= MAX_COMPRESSED_SIZE);

      if (stream_q_put(&data) == 0)
      {
        // printf("new packet: len = %d\n", compressed_length);
      }
      else
      {
        /* when syncing overrun should not happen..*/
        printf("overrun\n");
        //ASSERT(false);
      }
    }
    
    __WFE();

  }

  return 0;
}
