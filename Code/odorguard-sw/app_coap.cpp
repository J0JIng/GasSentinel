#include "app_coap.h"
#include <app_main.h>
#include <assert.h>
#include <openthread-core-config.h>
#include <openthread/config.h>

#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>

#include "openthread-system.h"
#include "sl_component_catalog.h"

#include <openthread/coap.h>
#include "utils/code_utils.h"

#include "stdio.h"
#include "string.h"

namespace coap {

static otIp6Address destAddr;
static otIp6Address selfAddr;

static bool discovered = false;
static uint32_t txFailCtr = 0;

static bool parseIntoBuffer(const sensor::sig_if_t *in);
static void coapTxMsg(void);

static char buffer[255];
constexpr static char *payload_fmt = "%lx,%.8lx%.8lx,%lu,%ld,%lu,%lu,%lu,%lu,%d,%d,";
constexpr static char *uri = "path";
constexpr static uint32_t token = 0x1234ABCD;


MailboxQueue<sensor::sig_if_t, COAP_MAILBOX_QUEUE_DEPTH> ux_queue;

void init(otIp6Address server_ip) {
	discovered = true;
	otCoapStart(otGetInstance(), OT_DEFAULT_COAP_PORT);
	updateAddr(server_ip);
}


void updateAddr(otIp6Address server_ip)
{
	destAddr = server_ip;
}

bool service(void)
{
	if(!discovered) return false;
	if(ux_queue.empty()) {
		//otCliOutputFormat("empty queue\n");
		return false;
	}
	sensor::sig_if_t data = ux_queue.front();
	otCliOutputFormat("pop\n");
	if(parseIntoBuffer(&data)) {
		ux_queue.pop();
		//coapTxMsg();
		otCliOutputFormat("sent\n");
		return true;
	}
	return false;
}


static bool parseIntoBuffer(const sensor::sig_if_t *in)
{
	uint32_t iaq = static_cast<uint32_t>(in->iaq.first);
	int32_t temp = static_cast<int32_t>(in->comp_temp.first*COAP_LFACTOR_DECIMAL_COUNT);
	uint32_t hum = static_cast<uint32_t>(in->comp_hum.first*COAP_LFACTOR_DECIMAL_COUNT);
	uint32_t pres = static_cast<uint32_t>(in->pres.first);

	uint32_t cl1 = static_cast<uint32_t>(in->comp_temp.first*COAP_LFACTOR_DECIMAL_COUNT);
	uint32_t cl2 = static_cast<uint32_t>(in->comp_temp.first*COAP_LFACTOR_DECIMAL_COUNT);
	int8_t rssi;

	otThreadGetParentLastRssi(otGetInstance(), &rssi);

	uint32_t ret = snprintf(buffer, 254, payload_fmt, token, 0,
			0 ,iaq, temp, hum, pres, cl1, cl2, rssi, 0 /* voltage */);
	if(ret >= 254) return false;

	return true;
}

static void coapTxMsg(void) {

	otError error = OT_ERROR_NONE;
	otMessage *message = NULL;
	otMessageInfo messageInfo;
	uint16_t payloadLength = 0;

	// Default parameters
	otCoapType coapType = OT_COAP_TYPE_CONFIRMABLE;
	otIp6Address coapDestinationIp = destAddr;
	message = otCoapNewMessage(otGetInstance(), NULL);

	otCoapMessageInit(message, coapType, OT_COAP_CODE_PUT);
	otCoapMessageGenerateToken(message, OT_COAP_DEFAULT_TOKEN_LENGTH);
	error = otCoapMessageAppendUriPathOptions(message, uri);
	otEXPECT(OT_ERROR_NONE == error);

	payloadLength = strlen(buffer);

	if (payloadLength > 0) {
		error = otCoapMessageSetPayloadMarker(message);
		otEXPECT(OT_ERROR_NONE == error);
	}

	// Embed content into message if given
	if (payloadLength > 0) {
		error = otMessageAppend(message, buffer, payloadLength);
		otEXPECT(OT_ERROR_NONE == error);
	}

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr = coapDestinationIp;
	messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;
	error = otCoapSendRequestWithParameters(otGetInstance(), message,
			&messageInfo, NULL, NULL,
			NULL);
	otEXPECT(OT_ERROR_NONE == error);

	exit: if ((error != OT_ERROR_NONE) && (message != NULL)) {
		txFailCtr = 0;
		otMessageFree(message);
	}

	if (error != OT_ERROR_NONE) {
		txFailCtr++;
	}

}

}


