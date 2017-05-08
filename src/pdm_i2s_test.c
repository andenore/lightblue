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

  printf("Stack pointer 0x%08x", __get_MSP());

  debug_print();

  while (1);
}

void HardFault_Handler(void)
{
  DEBUG_SET(3);

  printf("Stack pointer 0x%08x\n", __get_MSP());
  printf("HardFault_Handler...\n");
  while (1);
}

uint32_t m_stream[2][NUM_SAMPLES];
uint8_t pdm_stream_sel = 0;
uint8_t i2s_stream_sel = 0;
volatile uint32_t got_packet = 0;

uint8_t m_enc_buf[8964];
uint8_t m_dec_buf[17224];
uint8_t m_tmp_buf[255];

int main(int argc, char *argv[])
{
  printf("Starting PDM RX stream...\n");

  DEBUG_INIT();

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

  decoder_wrapper_init(m_dec_buf, sizeof(m_dec_buf));
  encoder_wrapper_init(m_enc_buf, sizeof(m_enc_buf));
  memset(m_stream, 0, sizeof(m_stream));

  mic_init();

  mic_rxptr_cfg(&m_stream[0][0], NUM_SAMPLES*4);

  printf("max9850_start\n");
  max9850_start();

  printf("i2s init\n");
  i2s_init();
  i2s_txptr_cfg(&m_stream[0][0], NUM_SAMPLES);

  printf("mic_start\n");
  mic_start();

  NRF_TIMER2->PRESCALER = 4;
  NRF_TIMER2->CC[0] = 5000;
  NRF_TIMER2->EVENTS_COMPARE[0] = 0;
  NRF_TIMER2->TASKS_START = 1;
  while (NRF_TIMER2->EVENTS_COMPARE[0] != 0)
  {

  }
  NRF_TIMER2->EVENTS_COMPARE[0] = 0;


  printf("i2s_start\n");
  i2s_start();
  
  while (1)
  {    

    if (i2s_txptr_upd())
    {
      // NRF_TIMER2->TASKS_CAPTURE[0] = 1;
      // NRF_TIMER2->TASKS_CLEAR = 1;
      i2s_stream_sel ^= 0x1;
      i2s_txptr_cfg(&m_stream[i2s_stream_sel][0], NUM_SAMPLES);
      // printf("switch stream %d\n", NRF_TIMER2->CC[0]);
    };

    if (mic_rxptr_upd()) {
      int compressed_length = 0;
      int frame_size = 0;

      // NRF_TIMER2->TASKS_CLEAR = 1;
      // compressed_length = codec_wrapper_encode(&m_stream[pdm_stream_sel][0], m_tmp_buf, sizeof(m_tmp_buf));
      // frame_size = codec_wrapper_decode(m_tmp_buf, compressed_length , &m_stream[pdm_stream_sel][0], NUM_SAMPLES);
      // NRF_TIMER2->TASKS_CAPTURE[0] = 1;

      for (int i=0; i<NUM_SAMPLES; i++)
      {
        printf("%08x ", m_stream[pdm_stream_sel][i]);
      }
      printf("\n");
      printf("frame_size = %d, encode+decode in %d us", frame_size, NRF_TIMER2->CC[0]); 

      pdm_stream_sel ^= 0x1;
      mic_rxptr_cfg(&m_stream[pdm_stream_sel][0], NUM_SAMPLES*4);
    }

    __WFE();

  }

  i2s_stop();

  return 0;
}
