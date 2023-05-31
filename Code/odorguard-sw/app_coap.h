#ifndef APP_COAP_H_
#define APP_COAP_H_

#include "openthread/ip6.h"


extern otIp6Address selfAddr;
extern otIp6Address brAddr;
extern bool appCoapConnectionEstablished;


void appCoapInit();
void appCoapCheckConnection(void);
void appCoapSendMsg(char *tx_buf, bool req_ack);

#endif /* APP_COAP_H_ */
