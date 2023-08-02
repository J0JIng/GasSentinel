#include <app_main.h>
#include <mx25_driver.h>
#include "sl_component_catalog.h"
#include "sl_system_init.h"
#include "sl_power_manager.h"
#include "sl_system_process_action.h"
#include "em_burtc.h"
#include "em_cmu.h"
#include "em_prs.h"
#include "em_iadc.h"
#include "sl_udelay.h"

void initBURTC(void) {
	CMU_ClockSelectSet(cmuClock_EM4GRPACLK, cmuSelect_ULFRCO);
	CMU_ClockEnable(cmuClock_BURTC, true);
	CMU_ClockEnable(cmuClock_BURAM, true);

	BURTC_Init_TypeDef burtcInit = BURTC_INIT_DEFAULT;
	burtcInit.compare0Top = true; // reset counter when counter reaches compare value
	burtcInit.em4comp = true; // BURTC compare interrupt wakes from EM4 (causes reset)
	BURTC_Init(&burtcInit);
	BURTC_IntEnable(BURTC_IEN_COMP);      // compare match
	NVIC_EnableIRQ(BURTC_IRQn);
	BURTC_Enable(true);
}


void initGPIO(void) {
	CMU_ClockEnable(cmuClock_GPIO, true);

	GPIO_PinModeSet(IP_LED_PORT, IP_LED_PIN, gpioModePushPull, 0);
	GPIO_PinModeSet(ACT_LED_PORT, ACT_LED_PIN, gpioModePushPull, 0);
	GPIO_PinModeSet(ERR_LED_PORT, ERR_LED_PIN, gpioModePushPull, 0);
	GPIO_PinModeSet(SW_PWR_SRC_PORT, SW_PWR_SRC_PIN, gpioModeInputPullFilter, 1);
}

void initSupplyMonitor(void)
{
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t singleInput = IADC_SINGLEINPUT_DEFAULT;

  CMU_ClockEnable (cmuClock_PRS, true);
  PRS_SourceAsyncSignalSet (0,
  PRS_ASYNC_CH_CTRL_SOURCESEL_MODEM,
                            PRS_MODEMH_PRESENT);
  PRS_ConnectConsumer (0, prsTypeAsync, prsConsumerIADC0_SINGLETRIGGER);
  CMU_ClockEnable (cmuClock_IADC0, true);
  initAllConfigs.configs[0].reference = iadcCfgReferenceInt1V2;
  initAllConfigs.configs[0].vRef = 1200;
  initAllConfigs.configs[0].osrHighSpeed = iadcCfgOsrHighSpeed2x;

  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale (
      IADC0, 1000000, 0, iadcCfgModeNormal, init.srcClkPrescale);
  initSingle.triggerSelect = iadcTriggerSelPrs0PosEdge;
  initSingle.dataValidLevel = iadcFifoCfgDvl4;
  initSingle.start = true;
  singleInput.posInput = iadcPosInputPortBPin4;
  singleInput.negInput = iadcNegInputGnd;
  IADC_init (IADC0, &init, &initAllConfigs);
  IADC_initSingle (IADC0, &initSingle, &singleInput);
  IADC_clearInt (IADC0, _IADC_IF_MASK);
  IADC_enableInt (IADC0, IADC_IEN_SINGLEDONE);
  NVIC_ClearPendingIRQ (IADC_IRQn);
  NVIC_SetPriority(GPIO_ODD_IRQn, 7);
  NVIC_EnableIRQ (IADC_IRQn);
}



int main(void)
{

  sl_system_init();


  // LL check flash HW
  MX25_init(); // wait tPUW
  uint8_t dat = 0x00;
  MX25_RES(&dat); // read eID
  if(dat != 0x14) GPIO_PinOutSet(ERR_LED_PORT, ERR_LED_PIN);

  initBURTC();
  initGPIO();



  app_init();



  while (1) {

    sl_system_process_action(); // System tasks

    app_process_action(); // App tasks

    sl_power_manager_sleep(); // Hand over to power manager

  }
  // SHOULD NEVER EXECUTE
  app_exit();

}
