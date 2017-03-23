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

set(BUILD_FLAGS
  "-DUNIT_TEST"
  )

add_library(LightblueCore stream.c adv.c event_q.c hal_radio.c hal_radio_timer.c radio_timer.c )
add_library(LightblueCoreDebug stream.c adv.c event_q.c hal_radio.c hal_radio_timer.c radio_timer.c )
set_target_properties(LightblueCoreDebug PROPERTIES COMPILE_FLAGS ${BUILD_FLAGS})