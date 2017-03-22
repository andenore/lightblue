#ifndef __DEBUG_H
#define __DEBUG_H

#include <nrf51_to_nrf52.h>

#define DEBUG_INIT() NRF_GPIO->DIRSET = (0xF << 17); NRF_GPIO->OUTSET = (0xF << 17);

#define DEBUG_SET(x) (NRF_GPIO->OUTCLR = (0x1 << (17+x)))
#define DEBUG_CLR(x) (NRF_GPIO->OUTSET = (0x1 << (17+x)))
#define DEBUG_TOGGLE(x) ((NRF_GPIO->OUT & (0x1 << (17+x))) ? (NRF_GPIO->OUTCLR = ((0x1 << (17+x)))) : (NRF_GPIO->OUTSET = ((0x1 << (17+x)))))

#endif