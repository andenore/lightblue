/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <nrf.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "debug.h"
#include "opus_types.h"
#include "opus_custom.h"
#include <assert.h>

#define MAX_PACKET 255

static void int_to_char(opus_uint32 i, unsigned char ch[4])
{
    ch[0] = i>>24;
    ch[1] = (i>>16)&0xFF;
    ch[2] = (i>>8)&0xFF;
    ch[3] = i&0xFF;
}

static opus_uint32 char_to_int(unsigned char ch[4])
{
    return ((opus_uint32)ch[0]<<24) | ((opus_uint32)ch[1]<<16)
         | ((opus_uint32)ch[2]<< 8) |  (opus_uint32)ch[3];
}

#define FRAME_LENGTH_MS (10)
#define SAMPLE_RATE_HZ  (16000)
#define APPLICATION     (OPUS_APPLICATION_RESTRICTED_LOWDELAY)
#define BITRATE         (128)
#define COMPLEXITY      (9)

#define CHANNELS (2)
#define MAX_FRAME_SIZE ((SAMPLE_RATE_HZ*FRAME_LENGTH_MS)/1000)

OpusCustomEncoder *enc;
OpusCustomDecoder *dec;

void encoder_wrapper_init(uint8_t *codec_buf, uint32_t buf_length)
{
  enc = (OpusCustomEncoder *)codec_buf;
  OpusCustomMode *custom_mode;
  bool decode_only = false;
  bool encode_only = true;
  opus_int32 sampling_rate = SAMPLE_RATE_HZ;
  int channels = CHANNELS;
  int application = APPLICATION;
  opus_int32 bitrate_bps = BITRATE;
  opus_int32 complexity = COMPLEXITY;
  int err;
  bool stop = false;
  volatile opus_int32 err_code;
  
  NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Enabled << NVMC_ICACHECNF_CACHEEN_Pos |
                        NVMC_ICACHECNF_CACHEPROFEN_Enabled << NVMC_ICACHECNF_CACHEPROFEN_Pos;
  
  custom_mode = opus_custom_mode_create(SAMPLE_RATE_HZ, MAX_FRAME_SIZE, &err);
  if (err != OPUS_OK)
  {
    printf("Custom mode create error");
    ASSERT(false);
  }
  
  volatile int size = opus_custom_encoder_get_size(custom_mode, channels);
  if (size > buf_length)
  {
    printf("Not enough memory, required = %d\n", size);
    ASSERT(false);
  }
  
  err = opus_custom_encoder_init(enc, custom_mode, channels);
  if (err != OPUS_OK)
  {
    printf("Cannot create encoder: %s\n", opus_strerror(err));
    ASSERT(false);
  }
  opus_custom_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate_bps));
  opus_custom_encoder_ctl(enc, OPUS_SET_COMPLEXITY(complexity));
  /*opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(bandwidth));
  opus_encoder_ctl(enc, OPUS_SET_VBR(use_vbr));
  opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(cvbr));
  opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(use_inbandfec));
  opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(forcechannels));
  opus_encoder_ctl(enc, OPUS_SET_DTX(use_dtx));
  opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(packet_loss_perc));

  opus_encoder_ctl(enc, OPUS_GET_LOOKAHEAD(&skip));
  opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(16));
  opus_encoder_ctl(enc, OPUS_SET_EXPERT_FRAME_DURATION(variable_duration));*/
}

void decoder_wrapper_init(uint8_t *codec_buf, uint32_t buf_length)
{
  dec = (OpusCustomDecoder *)codec_buf;
  OpusCustomMode *custom_mode;
  opus_int32 sampling_rate = SAMPLE_RATE_HZ;
  int channels = CHANNELS;
  int application = APPLICATION;
  opus_int32 bitrate_bps = BITRATE;
  opus_int32 complexity = COMPLEXITY;
  int err;
  bool stop = false;
  volatile opus_int32 err_code;
  
  NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Enabled << NVMC_ICACHECNF_CACHEEN_Pos |
                        NVMC_ICACHECNF_CACHEPROFEN_Enabled << NVMC_ICACHECNF_CACHEPROFEN_Pos;
  
  custom_mode = opus_custom_mode_create(SAMPLE_RATE_HZ, MAX_FRAME_SIZE, &err);
  if (err != OPUS_OK)
  {
    printf("Custom mode create error");
    ASSERT(false);
  }
  
  volatile int size = opus_custom_decoder_get_size(custom_mode, channels);
  if (size > buf_length)
  {
    printf("Not enough memory, required = %d, got = %d\n", size, buf_length);
    ASSERT(false);
  }
  
  err = opus_custom_decoder_init(dec, custom_mode, channels);
  if (err != OPUS_OK)
  {
    printf("Cannot create decoder: %s\n", opus_strerror(err));
    ASSERT(false);
  }
  opus_custom_decoder_ctl(dec, OPUS_SET_BITRATE(bitrate_bps));
  opus_custom_decoder_ctl(dec, OPUS_SET_COMPLEXITY(complexity));
  /*opus_decoder_ctl(dec, OPUS_SET_BANDWIDTH(bandwidth));
  opus_decoder_ctl(dec, OPUS_SET_VBR(use_vbr));
  opus_decoder_ctl(dec, OPUS_SET_VBR_CONSTRAINT(cvbr));
  opus_decoder_ctl(dec, OPUS_SET_INBAND_FEC(use_inbandfec));
  opus_decoder_ctl(dec, OPUS_SET_FORCE_CHANNELS(forcechannels));
  opus_decoder_ctl(dec, OPUS_SET_DTX(use_dtx));
  opus_decoder_ctl(dec, OPUS_SET_PACKET_LOSS_PERC(packet_loss_perc));

  opus_decoder_ctl(dec, OPUS_GET_LOOKAHEAD(&skip));
  opus_decoder_ctl(dec, OPUS_SET_LSB_DEPTH(16));
  opus_decoder_ctl(dec, OPUS_SET_EXPERT_FRAME_DURATION(variable_duration));*/
}

int codec_wrapper_decode(const unsigned char *data, int len, int16_t *pcm, int frame_size)
{
  int err_code = opus_custom_decode(dec, data, len, (opus_int16 *)pcm, frame_size);
  //ASSERT (err_code >= 0);

  return err_code;
}

int codec_wrapper_encode(const int16_t *pcm, unsigned char *compressed, int maxCompressedBytes)
{
  int err_code = opus_custom_encode(enc, (opus_int16 *)pcm, MAX_FRAME_SIZE, compressed, maxCompressedBytes);
  // ASSERT (err_code >= 0);

  return err_code;
}
