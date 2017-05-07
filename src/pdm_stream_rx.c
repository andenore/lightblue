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
uint32_t m_stream[2][NUM_SAMPLES];
uint8_t stream_sel = 0;
volatile uint32_t got_packet = 0;

static unsigned char out[MAX_COMPRESSED_SIZE];

int main(int argc, char *argv[])
{
  int compressed_length;
  uint8_t *play_stream = (uint8_t *)&m_stream[0][0];
  printf("Starting PDM RX stream...\n");

  DEBUG_INIT();

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

  memset(&data, 0, sizeof(data));
  data.len = 255;

  printf("decoder init\n");
  decoder_wrapper_init();


  printf("stream rx start\n");
  NVIC_SetPriority(STREAM_EVENT_IRQn, 3);
  NVIC_EnableIRQ(STREAM_EVENT_IRQn);
  stream_rx_start();

  printf("max9850_start\n");
  max9850_start();


  printf("i2s init\n");

  i2s_init();
  i2s_txptr_cfg(play_stream, NUM_SAMPLES);

  printf("i2s_start\n");
  i2s_start();

  NRF_TIMER2->PRESCALER = 4;
  NRF_TIMER2->TASKS_START = 1;

  while (1)
  {    

    if (i2s_txptr_upd())
    {
      // NRF_TIMER2->TASKS_CAPTURE[0] = 1;
      // NRF_TIMER2->TASKS_CLEAR = 1;
      play_stream = (play_stream == (uint8_t *)&m_stream[0][0]) ? (uint8_t *)&m_stream[1][0] : (uint8_t *)&m_stream[0][0];
      i2s_txptr_cfg(play_stream, NUM_SAMPLES);
      //printf("switch stream %d\n", NRF_TIMER2->CC[0]);
    };

    while (got_packet != 0)
    {
      int frame_size;
      uint8_t *upd_stream = (play_stream == (uint8_t *)&m_stream[0][0]) ? (uint8_t *)&m_stream[1][0] : (uint8_t *)&m_stream[0][0];
      got_packet = 0;

      stream_data_t *p_data = stream_q_head_peek();
      // m_cumulative_error += p_data->timestamp;
      // printf("Got new packet 0x%02x %d %d\n", p_data->buf[0], p_data->timestamp, (int)m_cumulative_error);

      
      NRF_TIMER2->TASKS_CLEAR = 1;
      frame_size = codec_wrapper_decode(p_data->buf[0], p_data->len, upd_stream, NUM_SAMPLES);
      NRF_TIMER2->TASKS_CAPTURE[0] = 1;
      printf("Decoded frame, len = %d, size = %d, time = %d\n",p_data->len, frame_size, NRF_TIMER2->CC[0]);

      (void)stream_q_get();
    }

    __WFE();

  }

  i2s_stop();

  return 0;
}

void STREAM_EVENT_IRQHandler(void)
{
  got_packet = 1;
  // printf("got packet\n");
}