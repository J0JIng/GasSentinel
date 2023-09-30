#ifndef APP_COAP_H_
#define APP_COAP_H_


#include "openthread/ip6.h"
#include "app_sensor.h"
#include <iostream>
#include <utility>
#include <queue>
#include <deque>

#define COAP_MAILBOX_QUEUE_DEPTH 3
#define COAP_MAX_CONSECUTIVE_TX 1
#define COAP_LFACTOR_DECIMAL_COUNT 1000


template <typename T, size_t len, typename Container=std::deque<T>>
class MailboxQueue : public std::queue<T, Container> {
public:
    void push(const T& value) {
        if (this->size() == len) {
           this->c.pop_front();
        }
        std::queue<T, Container>::push(value);
    }
};



namespace coap {

extern MailboxQueue<sensor::sig_if_t, COAP_MAILBOX_QUEUE_DEPTH> ux_queue;

extern volatile uint32_t vsense_batt;

void init(void);
void updateAddr(otIp6Address server_ip);
bool service(void);

}

#endif /* APP_COAP_H_ */
