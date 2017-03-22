include_directories(
  "../include"
  "../segger"
  "${SDK_ROOT}/"
  "${CMSIS_ROOT}/CMSIS/Core/Include"
  )

  add_definitions(
  "-DNRF52"
  "-DNRF52832_XXAA"
  )

add_library(LightblueAdvertiser adv.c)
add_library(LightblueCore event_q.c hal_radio.c hal_radio_timer.c radio_timer.c ../include/)
add_library(LightblueCoreStream stream.c)