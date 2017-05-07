include_directories(
  "../include"
  "../segger"
  "${SDK_ROOT}/"
  "${CMSIS_ROOT}/CMSIS/Core/Include"
  "opus/src/"
  "opus/include/"
  "opus/celt/"
  "opus/silk/"
  "audio/"
  )

  add_definitions(
  "-DNRF52"
  "-DNRF52832_XXAA"
  "-DHAVE_CONFIG_H"
  "-DCUSTOM_MODES"
  )

set(BUILD_FLAGS
  "-DUNIT_TEST"
  )

add_library(LightblueCore stream.c adv.c event_q.c hal_radio.c hal_radio_timer.c radio_timer.c )
add_library(LightblueCoreDebug stream.c adv.c event_q.c hal_radio.c hal_radio_timer.c radio_timer.c )
add_library(Audio audio/mic.c audio/codec_wrapper.c audio/i2c_lib.c audio/i2s_lib.c audio/max9850.c)
add_library(Celt opus/celt/bands.c opus/celt/celt.c opus/celt/celt_encoder.c opus/celt/celt_decoder.c opus/celt/cwrs.c opus/celt/entcode.c opus/celt/entdec.c opus/celt/entenc.c opus/celt/kiss_fft.c opus/celt/laplace.c opus/celt/mathops.c opus/celt/mdct.c opus/celt/modes.c opus/celt/pitch.c opus/celt/celt_lpc.c opus/celt/quant_bands.c opus/celt/rate.c opus/celt/vq.c)
add_library(Opus opus/src/opus.c opus/src/opus_decoder.c opus/src/opus_encoder.c opus/src/opus_multistream.c opus/src/opus_multistream_encoder.c opus/src/opus_multistream_decoder.c opus/src/repacketizer.c)
set_target_properties(LightblueCoreDebug PROPERTIES COMPILE_FLAGS ${BUILD_FLAGS})