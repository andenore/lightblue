/*
 * Copyright (c) 2017 Anders Nore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf.h>

#define NRF_TWIM (NRF_TWIM0)

void i2c_init(uint8_t device_address, uint32_t pin_scl, uint32_t pin_sda)
{
//  NRF_GPIO->PIN_CNF[pin_scl] = (GPIO_PIN_CNF_DIR_Output     << GPIO_PIN_CNF_DIR_Pos) |
//                               (GPIO_PIN_CNF_DRIVE_H0H1     << GPIO_PIN_CNF_DRIVE_Pos) |
//                               (GPIO_PIN_CNF_INPUT_Connect  << GPIO_PIN_CNF_INPUT_Pos) |
//                               (GPIO_PIN_CNF_PULL_Disabled  << GPIO_PIN_CNF_PULL_Pos) |
//                               (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
//  NRF_GPIO->PIN_CNF[pin_sda] = (GPIO_PIN_CNF_DIR_Output     << GPIO_PIN_CNF_DIR_Pos) |
//                               (GPIO_PIN_CNF_DRIVE_H0H1     << GPIO_PIN_CNF_DRIVE_Pos) |
//                               (GPIO_PIN_CNF_INPUT_Connect  << GPIO_PIN_CNF_INPUT_Pos) |
//                               (GPIO_PIN_CNF_PULL_Disabled  << GPIO_PIN_CNF_PULL_Pos) |
//                               (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
  
  NRF_TWIM->PSEL.SCL = pin_scl;
  NRF_TWIM->PSEL.SDA = pin_sda;
  
  NRF_TWIM->ADDRESS = device_address;
  NRF_TWIM->FREQUENCY = TWIM_FREQUENCY_FREQUENCY_K400 << TWIM_FREQUENCY_FREQUENCY_Pos;
  NRF_TWIM->SHORTS = 0;
  
  NRF_TWIM->ENABLE = TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos;
}

void i2c_write(uint8_t addr, uint8_t data)
{
  volatile uint8_t tx_buf[2];
  NRF_TWIM->SHORTS = TWIM_SHORTS_LASTTX_STOP_Msk;
  
  tx_buf[0] = addr;
  tx_buf[1] = data;
  NRF_TWIM->TXD.MAXCNT = sizeof(tx_buf);
  NRF_TWIM->TXD.PTR = (uint32_t)&tx_buf[0];
  
  NRF_TWIM->EVENTS_STOPPED = 0;
  NRF_TWIM->TASKS_STARTTX = 1;
  while (NRF_TWIM->EVENTS_STOPPED == 0);
}

uint8_t i2c_read(uint8_t addr)
{
  volatile uint8_t tx_buf[1];
  volatile uint8_t rx_buf[1];
  NRF_TWIM->SHORTS = TWIM_SHORTS_LASTTX_STARTRX_Msk | TWIM_SHORTS_LASTRX_STOP_Msk;
  
  tx_buf[0] = addr;
  NRF_TWIM->TXD.MAXCNT = sizeof(tx_buf);
  NRF_TWIM->TXD.PTR = (uint32_t)&tx_buf[0];
  
  NRF_TWIM->RXD.MAXCNT = 1;
  NRF_TWIM->RXD.PTR = (uint32_t)&rx_buf[0];
  
  NRF_TWIM->EVENTS_STOPPED = 0;
  NRF_TWIM->TASKS_STARTTX = 1;
  while (NRF_TWIM->EVENTS_STOPPED == 0);
  
  return rx_buf[0];
}
