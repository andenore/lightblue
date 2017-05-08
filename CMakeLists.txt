cmake_minimum_required(VERSION 3.0)

set(TARGET_PLATFORM "NRF52" CACHE STRING "Target platform")
set(SDK_ROOT        ""      CACHE STRING "SDK root folder")
set(CMSIS_ROOT      ""      CACHE STRING "CMSIS root folder")

if (SDK_ROOT MATCHES "" AND DEFINED ENV{SDK_ROOT})
  set(SDK_ROOT "$ENV{SDK_ROOT}")
endif ()

if (CMSIS_ROOT MATCHES "" AND DEFINED ENV{CMSIS_ROOT})
  set(CMSIS_ROOT "$ENV{CMSIS_ROOT}")
endif ()

if (EXISTS "${SDK_ROOT}/system_nrf52.c")
  message(STATUS "Using SDK from ${SDK_ROOT}")
else ()
  message(FATAL_ERROR "${SDK_ROOT} is not a valid path to the nRF5x MDK...")
endif ()

set(CMAKE_C_FLAGS "" CACHE STRING "")
include(${CMAKE_SOURCE_DIR}/CMake/toolchain_gcc.cmake)

project(lightblue C ASM)
add_subdirectory(libs)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

function (add_flash_target target)
  set(ftarget f_${target})

  add_custom_target(${ftarget}
    COMMAND nrfjprog -f ${TARGET_PLATFORM} --program ${CMAKE_CURRENT_BINARY_DIR}/${target}.hex --chiperase
    COMMAND nrfjprog -f ${TARGET_PLATFORM} -r
    )
endfunction ()

set(SRCS
  "${SDK_ROOT}/system_nrf52.c"
  "${SDK_ROOT}/gcc_startup_nrf52.S"
  "segger/SEGGER_RTT.c"
  "segger/SEGGER_RTT_Syscalls_SES.c"
  )

include_directories(
  "include"
  "segger"
  "${SDK_ROOT}/"
  "${CMSIS_ROOT}/CMSIS/Core/Include"
  )

link_directories(
  "${SDK_ROOT}/"
  )

add_definitions(
  "-DNRF52"
  "-DNRF52832_XXAA"
  "-D__HEAP_SIZE=17384"
  "-D__STACK_SIZE=4092"
  )

file( GLOB APP_SOURCES src/adv_test.c src/main.c src/jitter_test.c src/stream_test_tx.c src/stream_test_rx.c src/codec_test.c src/pdm_stream_tx.c  src/pdm_stream_rx.c src/pdm_i2s_test.c)
foreach( testsourcefile ${APP_SOURCES} )
    # I used a simple string replace, to cut off .cpp.
    #string( REPLACE ".c" "" testname ${testsourcefile} )
    get_filename_component (testname ${testsourcefile} NAME_WE)

    set(MY_LINK_FLAGS
    "-Xlinker -Map=${CMAKE_CURRENT_BINARY_DIR}/${testname}.map"
    "-mthumb -mabi=aapcs -L${SDK_ROOT}/components/toolchain/gcc"
    "-T${SDK_ROOT}/nrf52_xxaa.ld"
    "-Wl,--gc-sections"
    "-lc -lnosys"
    )
    string(REGEX REPLACE ";" " " MY_LINK_FLAGS "${MY_LINK_FLAGS}")

    add_executable( ${testname} ${testsourcefile} ${SRCS})
    # Make sure YourLib is linked to each app
    set_target_properties(${testname} PROPERTIES LINK_FLAGS "${MY_LINK_FLAGS}")
    target_link_libraries (${testname} LightblueCore Audio Opus Celt m)

    create_hex(${testname})
    add_flash_target(${testname})
endforeach( testsourcefile ${APP_SOURCES} )

file( GLOB APP_SOURCES test/*.c)
foreach( testsourcefile ${APP_SOURCES} )
    # I used a simple string replace, to cut off .cpp.
    #string( REPLACE ".c" "" testname ${testsourcefile} )
    get_filename_component (testname ${testsourcefile} NAME_WE)

    set(MY_LINK_FLAGS
    "-Xlinker -Map=${CMAKE_CURRENT_BINARY_DIR}/${testname}.map"
    "-mthumb -mabi=aapcs -L${SDK_ROOT}/components/toolchain/gcc"
    "-T${SDK_ROOT}/nrf52_xxaa.ld"
    "-Wl,--gc-sections"
    "-lc -lnosys"
    )
    string(REGEX REPLACE ";" " " MY_LINK_FLAGS "${MY_LINK_FLAGS}")

    add_executable( ${testname} ${testsourcefile} ${SRCS})
    # Make sure YourLib is linked to each app
    set_target_properties(${testname} PROPERTIES LINK_FLAGS "${MY_LINK_FLAGS}")
    target_link_libraries (${testname} LightblueCoreDebug)

    create_hex(${testname})
    add_flash_target(${testname})
endforeach( testsourcefile ${APP_SOURCES} )

