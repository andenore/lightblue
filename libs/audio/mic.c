
#include <nrf.h>
#include <stdbool.h>

#define MIC_PWR_CTRL (5)
#define MIC_DOUT     (4)
#define MIC_CLK      (3)


static uint32_t rxptrupd;
static uint32_t events_end;

void mic_init()
{
  rxptrupd = 0;
  events_end = 0;
  
  NRF_GPIO->PIN_CNF[MIC_DOUT] =(GPIO_PIN_CNF_DIR_Input      << GPIO_PIN_CNF_DIR_Pos) |
                               (GPIO_PIN_CNF_DRIVE_S0S1     << GPIO_PIN_CNF_DRIVE_Pos) |
                               (GPIO_PIN_CNF_INPUT_Connect  << GPIO_PIN_CNF_INPUT_Pos) |
                               (GPIO_PIN_CNF_PULL_Disabled  << GPIO_PIN_CNF_PULL_Pos) |
                               (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
  
  NRF_GPIO->PIN_CNF[MIC_CLK] = (GPIO_PIN_CNF_DIR_Output     << GPIO_PIN_CNF_DIR_Pos) |
                               (GPIO_PIN_CNF_DRIVE_H0H1     << GPIO_PIN_CNF_DRIVE_Pos) |
                               (GPIO_PIN_CNF_INPUT_Connect  << GPIO_PIN_CNF_INPUT_Pos) |
                               (GPIO_PIN_CNF_PULL_Disabled  << GPIO_PIN_CNF_PULL_Pos) |
                               (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
  NRF_GPIO->PIN_CNF[MIC_PWR_CTRL] = 
                               (GPIO_PIN_CNF_DIR_Output     << GPIO_PIN_CNF_DIR_Pos) |
                               (GPIO_PIN_CNF_DRIVE_S0S1     << GPIO_PIN_CNF_DRIVE_Pos) |
                               (GPIO_PIN_CNF_INPUT_Connect  << GPIO_PIN_CNF_INPUT_Pos) |
                               (GPIO_PIN_CNF_PULL_Disabled  << GPIO_PIN_CNF_PULL_Pos) |
                               (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
  
  // Power up microphones
  NRF_GPIO->OUTCLR = (1UL << MIC_PWR_CTRL);
  
  NRF_PDM->PSEL.CLK = MIC_CLK;
  NRF_PDM->PSEL.DIN = MIC_DOUT;
  
  NRF_PDM->GAINL = PDM_GAINL_GAINL_DefaultGain + 28;// << PDM_GAINL_GAINL_Pos;
  NRF_PDM->GAINR = PDM_GAINR_GAINR_DefaultGain + 28;// << PDM_GAINR_GAINR_Pos;
  
  NRF_PDM->PDMCLKCTRL = PDM_PDMCLKCTRL_FREQ_1067K << PDM_PDMCLKCTRL_FREQ_Pos;
  NRF_PDM->MODE = PDM_MODE_EDGE_LeftRising << PDM_MODE_EDGE_Pos |
                  PDM_MODE_OPERATION_Stereo << PDM_MODE_OPERATION_Pos;
  
  NRF_PDM->ENABLE = PDM_ENABLE_ENABLE_Enabled << PDM_ENABLE_ENABLE_Pos;
  
  NRF_PDM->INTENSET = (PDM_INTENSET_STARTED_Set << PDM_INTENSET_STARTED_Pos) | (PDM_INTENSET_END_Set << PDM_INTENSET_END_Pos);
  NVIC_SetPriority(PDM_IRQn, 3);
  NVIC_EnableIRQ(PDM_IRQn);
}

void mic_start(void)
{
  rxptrupd = 0;
  events_end = 0;
  NRF_PDM->TASKS_START = 1;
}

void mic_stop(void)
{
  NRF_PDM->TASKS_STOP = 1;
}

void mic_rxptr_cfg(uint32_t *p, uint32_t size)
{
  NRF_PDM->SAMPLE.PTR = (uint32_t)p;
  NRF_PDM->SAMPLE.MAXCNT = size / 2;
}

bool mic_rxptr_upd(void)
{
  if (rxptrupd)
  {
    rxptrupd = 0;
    return true;
  }
  
  return false;
}

bool mic_events_end(void)
{
  if (events_end)
  {
    events_end = 0;
    return true;
  }
  
  return false;
}


void PDM_IRQHandler(void)
{
  if (NRF_PDM->EVENTS_STARTED != 0)
  {
    NRF_PDM->EVENTS_STARTED = 0;
    rxptrupd = 1;
  }

  if (NRF_PDM->EVENTS_END != 0)
  {
    NRF_PDM->EVENTS_END = 0;
    events_end = 1;
  }
}