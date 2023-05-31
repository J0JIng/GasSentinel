
#ifndef APP_MAIN_H
#define APP_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "openthread/instance.h"


void app_init(void);


void app_exit(void);

void app_process_action(void);
void applicationTick(void);

otInstance *otGetInstance(void);

void sleepyInit(void);
void setNetworkConfiguration(void);
void initUdp(void);

void sl_ot_create_instance(void);
void sl_ot_cli_init(void);


#define IP_LED_PORT      gpioPortC
#define IP_LED_PIN       5
#define ACT_LED_PORT     gpioPortC
#define ACT_LED_PIN      6
#define ERR_LED_PORT     gpioPortC
#define ERR_LED_PIN      7

#define I2C_SCL_PORT     gpioPortB
#define I2C_SCL_PIN      1
#define I2C_SDA_PORT     gpioPortB
#define I2C_SDA_PIN      0
#define TS_INT_PORT      gpioPortC
#define TS_INT_PIN       0
#define VBAT_SENSE_PORT  gpioPortC
#define VBAT_SENSE_PIN   1



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // APP_MAIN_H
