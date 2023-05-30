
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


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // APP_MAIN_H
