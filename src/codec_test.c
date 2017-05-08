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

uint32_t pdm_samples[2][NUM_SAMPLES*2];

static unsigned char out[MAX_COMPRESSED_SIZE];

int main(int argc, char *argv[])
{
  printf("Starting Codec test...\n");

  DEBUG_INIT();

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

  NRF_TIMER0->PRESCALER = 4;


  encoder_wrapper_init();

  printf("Encoding..\n");
  NRF_TIMER0->TASKS_START = 1;
  NRF_TIMER0->TASKS_CLEAR = 1;

  int size = codec_wrapper_encode(&pdm_samples[0][0], out, sizeof(out));
  NRF_TIMER0->TASKS_CAPTURE[0] = 1;
  printf("Encoding done..  size = %d,  time = %d\n", size, NRF_TIMER0->CC[0]);

  return 0;
}
