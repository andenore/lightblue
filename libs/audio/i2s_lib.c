
#include <nrf.h>
#include <stdbool.h>

#define PIN_MCK    (13)
#define PIN_SCK    (14)
#define PIN_LRCK   (15)
#define PIN_SDOUT  (16)

static bool txptrupd;

static void m_i2s_pin_cnf(void)
{
  NRF_GPIO->PIN_CNF[PIN_MCK] =   (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                 (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
                                 (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
                                 (GPIO_PIN_CNF_PULL_Disabled <<GPIO_PIN_CNF_PULL_Pos) |
                                 (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
  NRF_GPIO->PIN_CNF[PIN_SCK] =   (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                 (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
                                 (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
                                 (GPIO_PIN_CNF_PULL_Disabled <<GPIO_PIN_CNF_PULL_Pos) |
                                 (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
  NRF_GPIO->PIN_CNF[PIN_LRCK] =  (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                 (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
                                 (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
                                 (GPIO_PIN_CNF_PULL_Disabled <<GPIO_PIN_CNF_PULL_Pos) |
                                 (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
  NRF_GPIO->PIN_CNF[PIN_SDOUT] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                                 (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
                                 (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
                                 (GPIO_PIN_CNF_PULL_Disabled <<GPIO_PIN_CNF_PULL_Pos) |
                                 (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
  
  NRF_I2S->PSEL.MCK = (PIN_MCK << I2S_PSEL_MCK_PIN_Pos);
  NRF_I2S->PSEL.SCK = (PIN_SCK << I2S_PSEL_SCK_PIN_Pos); 
  NRF_I2S->PSEL.LRCK = (PIN_LRCK << I2S_PSEL_LRCK_PIN_Pos); 
  NRF_I2S->PSEL.SDOUT = (PIN_SDOUT << I2S_PSEL_SDOUT_PIN_Pos);
}

void i2s_init(void)
{
  // Enable transmission
  NRF_I2S->CONFIG.TXEN = (I2S_CONFIG_TXEN_TXEN_ENABLE << I2S_CONFIG_TXEN_TXEN_Pos);
  
  // Enable MCK generator
  NRF_I2S->CONFIG.MCKEN = (I2S_CONFIG_MCKEN_MCKEN_ENABLE << I2S_CONFIG_MCKEN_MCKEN_Pos);

    
  // MCKFREQ = 2909090.9 hz
  // Ratio = 64 
  // LRCLK = 45454.5 hz
  // NRF_I2S->CONFIG.MCKFREQ = 0x08000000UL  << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
  // NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_64X << I2S_CONFIG_RATIO_RATIO_Pos;
  
  // MIC MCLK = 1032258.1
  NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV31  << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
  NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_64X << I2S_CONFIG_RATIO_RATIO_Pos;
    
  
  NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_MASTER << I2S_CONFIG_MODE_MODE_Pos;
  NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_16BIT << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
  NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_LEFT << I2S_CONFIG_ALIGN_ALIGN_Pos;
  
  // Format = I2S
  NRF_I2S->CONFIG.FORMAT = I2S_CONFIG_FORMAT_FORMAT_I2S << I2S_CONFIG_FORMAT_FORMAT_Pos;
  
  // Use stereo 
  NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_STEREO << I2S_CONFIG_CHANNELS_CHANNELS_Pos;
  
  m_i2s_pin_cnf();
  
  NRF_I2S->ENABLE = 1;
}

void i2s_txptr_cfg(uint8_t *ptr, uint32_t size)
{
  NRF_I2S->TXD.PTR = (uint32_t)ptr;
  NRF_I2S->RXTXD.MAXCNT = size;
}

void i2s_start(void)
{
  txptrupd = 0;
  NRF_I2S->EVENTS_TXPTRUPD = 0;
  NRF_I2S->INTENSET = I2S_INTENSET_TXPTRUPD_Enabled << I2S_INTENSET_TXPTRUPD_Pos;
    
  NRF_I2S->TASKS_START = 1;
  NVIC_SetPriority(I2S_IRQn, 3);
  NVIC_EnableIRQ(I2S_IRQn);
}

void i2s_stop(void)
{
  NRF_I2S->TASKS_STOP = 1;
}

bool i2s_txptr_upd(void)
{
  if (txptrupd)
  {
    txptrupd = 0;
    return true;
  }
  
  return false;
}

void I2S_IRQHandler(void)
{
  volatile uint32_t dumb;
  if (NRF_I2S->EVENTS_TXPTRUPD != 0)
  {
    printf("i2s_txptrupd\n");
    NRF_I2S->EVENTS_TXPTRUPD = 0;
    dumb = NRF_I2S->EVENTS_TXPTRUPD;
    txptrupd = 1;
  }
}
