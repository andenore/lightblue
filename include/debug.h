#ifndef __DEBUG_H
#define __DEBUG_H

#include <nrf51_to_nrf52.h>

#if 0

#define DEBUG_INIT() NRF_GPIO->DIRSET = (0xF << 6); NRF_GPIO->OUTSET = (0xF << 6);

#define DEBUG_SET(x) (NRF_GPIO->OUTCLR = (0x1 << (6+x)))
#define DEBUG_CLR(x) (NRF_GPIO->OUTSET = (0x1 << (6+x)))
#define DEBUG_TOGGLE(x) ((NRF_GPIO->OUT & (0x1 << (6+x))) ? (NRF_GPIO->OUTCLR = ((0x1 << (6+x)))) : (NRF_GPIO->OUTSET = ((0x1 << (6+x)))))

#define TEST_BEGIN(TESTNAME) void  TESTNAME (void) { printf("Begin test: %s\n", #TESTNAME); 

#define TEST_END() printf("\n"); }

#endif

#define DEBUG_INIT() 

#define DEBUG_SET(x) 
#define DEBUG_CLR(x) 
#define DEBUG_TOGGLE(x) 

#define TEST_BEGIN(TESTNAME) void  TESTNAME (void) { printf("Begin test: %s\n", #TESTNAME); 

#define TEST_END() printf("\n"); }

#endif