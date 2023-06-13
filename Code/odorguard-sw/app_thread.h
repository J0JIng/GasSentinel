#ifndef APP_THREAD_H
#define APP_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif


void applicationTick(void);
void sleepyInit(void);
void setNetworkConfiguration(void);

void appSrpInit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // APP_THREAD_H
