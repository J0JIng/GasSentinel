#include "app_dns.h"
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

#include "ip6.h"
#include "utils/code_utils.h"

#include "stdio.h"
#include "string.h"

#define APP_DNS_DEBUG_PRINT

dns::dns(otInstance* (*getotInst)(void), uint8_t max_dns, uint8_t max_addr) {
	getInst = getotInst;
	browseRespInfo = new otDnsServiceInfo[max_dns];
	for (uint8_t i = 0; i < max_dns; i++) {
		browseRespInfo[i].mHostNameBuffer = new char[OT_DNS_MAX_NAME_SIZE]();
		browseRespInfo[i].mHostNameBufferSize = OT_DNS_MAX_NAME_SIZE;

		browseRespInfo[i].mTxtData =
				new uint8_t[OPENTHREAD_CONFIG_CLI_TXT_RECORD_MAX_SIZE]();
		browseRespInfo[i].mTxtDataSize =
				OPENTHREAD_CONFIG_CLI_TXT_RECORD_MAX_SIZE;

	}
	serviceRespInfo.mHostNameBuffer = new char[OT_DNS_MAX_NAME_SIZE]();
	serviceRespInfo.mHostNameBufferSize = OT_DNS_MAX_NAME_SIZE;

	serviceRespInfo.mTxtData =
			new uint8_t[OPENTHREAD_CONFIG_CLI_TXT_RECORD_MAX_SIZE]();
	serviceRespInfo.mTxtDataSize = OPENTHREAD_CONFIG_CLI_TXT_RECORD_MAX_SIZE;

	browseRespInfoCount = max_dns;
	addrRespInfo = new otIp6Address[max_addr];
	addrRespInfoCount = max_addr;
}

static void outputPrintDnsTextData(const uint8_t *aTxtData,
		uint16_t aTxtDataLength) {
	otDnsTxtEntry entry;
	otDnsTxtEntryIterator iterator;
	bool isFirst = true;

	otDnsInitTxtEntryIterator(&iterator, aTxtData, aTxtDataLength);

	while (otDnsGetNextTxtEntry(&iterator, &entry) == OT_ERROR_NONE) {
		if (!isFirst) {
			otCliOutputFormat(", ");
		}

		if (entry.mKey == nullptr) {
			// A null `mKey` indicates that the key in the entry is
			// longer than the recommended max key length, so the entry
			// could not be parsed. In this case, the whole entry is
			// returned encoded in `mValue`.

			otCliOutputFormat("[");
			otCliOutputBytes(entry.mValue, entry.mValueLength);
			otCliOutputFormat("]");
		} else {
			otCliOutputFormat("%s", entry.mKey);

			if (entry.mValue != nullptr) {
				otCliOutputFormat("=");
				otCliOutputBytes(entry.mValue, entry.mValueLength);
			}
		}
		isFirst = false;
	}
}

static void outputPrintDnsServiceInfo(uint8_t aIndentSize,
		const otDnsServiceInfo &aServiceInfo) {
	otCliOutputFormat("Port:%d, Priority:%d, Weight:%d, TTL:%lu\n",
			aServiceInfo.mPort, aServiceInfo.mPriority, aServiceInfo.mWeight,
			aServiceInfo.mTtl);
	otCliOutputFormat("Host:%s\n", aServiceInfo.mHostNameBuffer);
	otCliOutputFormat("HostAddress:");
	char string[OT_IP6_ADDRESS_STRING_SIZE];

	otIp6AddressToString(&aServiceInfo.mHostAddress, string, sizeof(string));
	otCliOutputFormat("%s\n ", string);
	otCliOutputFormat(" TTL:%lu\n", aServiceInfo.mHostAddressTtl);
	otCliOutputFormat("TXT:");

	if (!aServiceInfo.mTxtDataTruncated) {

		otCliOutputFormat("[");
		outputPrintDnsTextData(aServiceInfo.mTxtData,
				aServiceInfo.mTxtDataSize);
		otCliOutputFormat("]");
		otCliOutputBytes(aServiceInfo.mTxtData, aServiceInfo.mTxtDataSize);
	} else {
		otCliOutputFormat("[");
		otCliOutputBytes(aServiceInfo.mTxtData, aServiceInfo.mTxtDataSize);
		otCliOutputFormat("...]");
	}

}

void dns::browseRespHandler(otError aError,
		const otDnsBrowseResponse *aResponse, void *aContext) {

	if (aError == OT_ERROR_NONE) {
		uint16_t index = 0;

		while (otDnsBrowseResponseGetServiceInstance(aResponse, index, label,
				sizeof(label)) == OT_ERROR_NONE && index < browseRespInfoCount) {

			if (otDnsBrowseResponseGetServiceInfo(aResponse, label,
					&browseRespInfo[index]) == OT_ERROR_NONE) {

#ifdef APP_DNS_DEBUG_PRINT
				outputPrintDnsServiceInfo(4, browseRespInfo[index]);
#endif // APP_DNS_DEBUG_PRINT
			}

			index++;
		}
		if (index >= browseRespInfoCount)
			browseRespInfoOverflow = true;
		else
			browseRespInfoOverflow = false;
		browseRespInfoValid = index;
		browseRespInfoReady = true;
	}

}

void dns::resolveHostRespHandler(otError aError,
		const otDnsServiceResponse *aResponse, void *aContext) {

	if (aError == OT_ERROR_NONE) {

		if (otDnsServiceResponseGetServiceInfo(aResponse, &serviceRespInfo)
				== OT_ERROR_NONE) {

#ifdef APP_DNS_DEBUG_PRINT
			outputPrintDnsServiceInfo(0, serviceRespInfo);
#endif // APP_DNS_DEBUG_PRINT
		}
	}
}

void dns::resolveIpRespHandler(otError aError,
		const otDnsAddressResponse *aResponse, void *aContext) {

	if (aError == OT_ERROR_NONE) {
		uint16_t index = 0;
		uint32_t ttl;
		while (otDnsAddressResponseGetAddress(aResponse, index,
				&addrRespInfo[index], &ttl) == OT_ERROR_NONE
				&& index < addrRespInfoCount) {

#ifdef APP_DNS_DEBUG_PRINT
			char string[OT_IP6_ADDRESS_STRING_SIZE];
			otIp6AddressToString(&addrRespInfo[index], string, sizeof(string));

			otCliOutputFormat("%s\n ", string);
			otCliOutputFormat(" TTL:%lu ", (ttl));
			outputPrintDnsServiceInfo(0, serviceRespInfo);
#endif // APP_DNS_DEBUG_PRINT

			index++;
		}
		if (index >= addrRespInfoCount)
			addrRespInfoOverflow = true;
		else
			addrRespInfoOverflow = false;
		addrRespInfoValid = index;
		addrRespInfoReady = true;
	}

}

otError dns::browse(char *name) {
	otInstance *inst = getInst();
	if (inst == NULL)
		return OT_ERROR_INVALID_STATE;
	browseRespInfoReady = false;
	if (otDnsClientBrowse(inst, name, &dns::browseRespHandlerWrapper, this,
			&config) != OT_ERROR_NONE)
		return OT_ERROR_NO_BUFS;
	return OT_ERROR_PENDING;

}

otError dns::resolveHost(char *label, char *name) {
	otInstance *inst = getInst();
	if (inst == NULL)
		return OT_ERROR_INVALID_STATE;
	browseRespInfoReady = false;
	if (otDnsClientResolveService(inst, label, name,
			&dns::resolveHostRespHandlerWrapper, this, &config)
			!= OT_ERROR_NONE)
		return OT_ERROR_NO_BUFS;
	return OT_ERROR_PENDING;

}

otError dns::resolve4(char *name) {
	otInstance *inst = getInst();
	if (inst == NULL)
		return OT_ERROR_INVALID_STATE;
	browseRespInfoReady = false;
	if (otDnsClientResolveIp4Address(inst, name,
			&dns::resolveIpRespHandlerWrapper, this, &config) != OT_ERROR_NONE)
		return OT_ERROR_NO_BUFS;
	return OT_ERROR_PENDING;

}
otError dns::resolve6(char *name) {
	otInstance *inst = getInst();
	if (inst == NULL)
		return OT_ERROR_INVALID_STATE;
	browseRespInfoReady = false;
	if (otDnsClientResolveAddress(inst, name, &dns::resolveIpRespHandlerWrapper,
			this, &config) != OT_ERROR_NONE)
		return OT_ERROR_NO_BUFS;
	return OT_ERROR_PENDING;

}

bool dns::browseResultReady(otDnsServiceInfo **info, uint8_t *num) {
	if (!browseRespInfoReady)
		return false;
	*info = browseRespInfo;
	*num = browseRespInfoValid;
	browseRespInfoReady = false;
	return true;
}

bool dns::resolveHostResultReady(otDnsServiceInfo *info) {
	if (!serviceRespInfoReady)
		return false;
	info = browseRespInfo;
	serviceRespInfoReady = false;
	return true;
}

bool dns::resolveIpResultReady(otIp6Address **info, uint8_t *num) {
	if (!addrRespInfoReady)
		return false;
	*info = addrRespInfo;
	*num = addrRespInfoValid;
	addrRespInfoReady = false;
	return true;
}

bool dns::browseResultIsOverflow(void) {
	return addrRespInfoOverflow;
}

bool dns::resolveIpResultReadyIsOverflow(void) {
	return browseRespInfoOverflow;
}

