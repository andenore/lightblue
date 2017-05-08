#include <nrf.h>
#include <i2c_lib.h>
#include <assert.h>

#define REG_STATUS_A                      (0x00)
#define REG_STATUS_B                      (0x01)
#define REG_VOLUME                        (0x02)
#define REG_GENERAL_PURPOSE               (0x03)
#define REG_INTERRUPT_ENABLE              (0x04)
#define REG_ENABLE                        (0x05)
#define REG_CLOCK                         (0x06)
#define REG_CHARGE_PUMP                   (0x07)
#define REG_LRCLK_MSB                     (0x08)
#define REG_LRCLK_LSB                     (0x09)
#define REG_DIGITAL_AUDIO                 (0x0A)

#define DEVICE_ADDR (0x10)
#define PIN_SCL     (27)
#define PIN_SDA     (26)
#define PIN_SD      (11)

// Start with -7.5 dB
static uint8_t m_volume = 0x0E;

void max9850_debug_print(void)
{
  uint8_t i;
  for (i=0x0; i <= 0x0A; i++)
  {
    uint8_t val = i2c_read(i);
    
    printf("%02x: %02x\n", i, val);
    //printf("test");
  }
}

void max9850_volume_up(void)
{
  if (m_volume > 0x01)
  {
    m_volume--;
    i2c_write(REG_VOLUME, (1 << 6) | m_volume);
  }
}

void max9850_volume_down(void)
{
  if (m_volume <= 0x28)
  {
    m_volume++;
    i2c_write(REG_VOLUME, (1 << 6) | m_volume);
  }
}

void max9850_start(void)
{
  uint32_t tmo;
  
  NRF_GPIO->PIN_CNF[PIN_SD] =  (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                               (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
                               (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
                               (GPIO_PIN_CNF_PULL_Disabled <<GPIO_PIN_CNF_PULL_Pos) |
                               (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
  
  NRF_GPIO->OUTCLR = (1 << PIN_SD);
  
  
  tmo = 50000;
  while (tmo-- ) ;
  
  i2c_init(DEVICE_ADDR, PIN_SCL, PIN_SDA);
  
  NRF_GPIO->OUTSET = (1 << PIN_SD);
  
  tmo = 1700000 * 100 ;
  while (tmo-- ) ;
  
  max9850_debug_print();
  // Enable amplifier 
  i2c_write(REG_ENABLE, 0xFF);
  tmo = 100000 ; while (tmo-- ) ;
  
  i2c_write(REG_VOLUME, 0x4E);
  tmo = 100000 ; while (tmo-- ) ;
  
  i2c_write(REG_LRCLK_MSB, 0x80);
  tmo = 100000 ; while (tmo-- ) ;
  
  i2c_write(REG_LRCLK_LSB, 0x04);
  tmo = 100000 ; while (tmo-- ) ;
  
  i2c_write(REG_DIGITAL_AUDIO, 0x08);
  tmo = 100000 ; while (tmo-- ) ;
  
  max9850_debug_print();
        
  //i2c_write(REG_VOLUME_AND_MUTE_CONTROL, 0x80 | (0 << 2));
  
  //i2c_write(REG_EDGE_CLOCK_CONTROL, 0x01);
  //i2c_write(REG_SERIAL_INTERFACE_SAMPLE_RATE_CONTROL, (1 << 5) | 0x02);
  
  //i2c_write(REG_LEFT_VOLUME_CONTROL, 0x70);
  //i2c_write(REG_RIGHT_VOLUME_CONTROL, 0x70);
  //i2c_write(REG_CHANNEL_MAPPING_CONTROL, 0x01);
  
  //i2c_write(REG_POWER_AND_FAULT_CONTROL, 0x00 | (0 << 2));
}

