/* Copyright (c) 2007-2008 CSIRO
   Copyright (c) 2007-2009 Xiph.Org Foundation
   Written by Jean-Marc Valin */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#define BITRATE         (64)
#define COMPLEXITY      (5)

#define CHANNELS (2)
#define MAX_FRAME_SIZE ((SAMPLE_RATE_HZ*FRAME_LENGTH_MS)/1000)

static uint8_t codec_buf[17220];

void codec_wrapper_init(void)
{
  OpusCustomEncoder *enc = (OpusCustomEncoder *)&codec_buf;
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
  if (size > sizeof(codec_buf))
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

void decoder_wrapper_init(void)
{
  OpusCustomDecoder *dec = (OpusCustomDecoder *)&codec_buf;
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
  if (size > sizeof(codec_buf))
  {
    printf("Not enough memory, required = %d\n", size);
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

int codec_wrapper_decode(const unsigned char *data, int len, opus_int16 *pcm, int frame_size)
{
  OpusCustomDecoder *enc = (OpusCustomDecoder *)&codec_buf;
  int err_code = opus_custom_decode(enc, data, len, pcm, frame_size);
  ASSERT (err_code >= 0);

  return err_code;
}

int codec_wrapper_encode(const opus_int16 *pcm, unsigned char *compressed, int maxCompressedBytes)
{
  OpusCustomEncoder *enc = (OpusCustomEncoder *)&codec_buf;
  int err_code = opus_custom_encode(enc, pcm, MAX_FRAME_SIZE, compressed, maxCompressedBytes);
  ASSERT (err_code >= 0);

  return err_code;
}
