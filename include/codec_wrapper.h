/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CODEC_WRAPPER_H
#define __CODEC_WRAPPER_H

void encoder_wrapper_init(uint8_t *codec_buf, uint32_t buf_length);

void decoder_wrapper_init(uint8_t *codec_buf, uint32_t buf_length);

int codec_wrapper_decode(const unsigned char *data, int len, int16_t *pcm, int frame_size);

int codec_wrapper_encode(const int16_t *pcm, unsigned char *compressed, int maxCompressedBytes);

#endif // __CODEC_WRAPPER_H