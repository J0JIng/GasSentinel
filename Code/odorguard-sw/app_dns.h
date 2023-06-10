#ifndef APP_DNS_H_
#define APP_DNS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <openthread/error.h>
namespace dns {
otError resolveHost(char *host);
otError browse(char *name);
}
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* APP_DNS_H_ */
