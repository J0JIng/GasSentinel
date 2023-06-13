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

static otDnsQueryConfig config;

char label[OT_DNS_MAX_LABEL_SIZE];

static void outputPrintDnsInfo(uint8_t aIndentSize,
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

		otDnsTxtEntry entry;
		otDnsTxtEntryIterator iterator;
		bool isFirst = true;

		otDnsInitTxtEntryIterator(&iterator, aServiceInfo.mTxtData,
				aServiceInfo.mTxtDataSize);

		otCliOutputFormat("[");

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

		otCliOutputFormat("]");
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
				sizeof(label)) == OT_ERROR_NONE) {

			if (otDnsBrowseResponseGetServiceInfo(aResponse, label,
					&browseRespInfo[index]) == OT_ERROR_NONE) {

#define APP_DNS_DEBUG_PRINT
#ifdef APP_DNS_DEBUG_PRINT
				outputPrintDnsInfo(4, browseRespInfo[index]);
#endif // APP_DNS_DEBUG_PRINT
			}

			index++;
		}
		browseRespInfoValid = index;
		browseRespInfoReady = true;
	}

}

otError dns::browse(char *name) {
	browseRespInfoReady = false;
	if (otDnsClientBrowse(otGetInstance(), name, &dns::browseRespHandlerWrapper,
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

#ifdef APP_DNS_RESOLVE_IMPLEMENTED
void dns::resolveRespHandler(otError aError,
		const otDnsAddressResponse *aResponse, void *aContext) {
	char hostName[OT_DNS_MAX_NAME_SIZE];
	otIp6Address address;
	uint32_t ttl;

	// IgnoreError(otDnsAddressResponseGetHostName(aResponse, hostName, sizeof(hostName)));

	//OutputFormat("DNS response for %s - ", hostName);

	if (aError == OT_ERROR_NONE) {
		uint16_t index = 0;

		while (otDnsAddressResponseGetAddress(aResponse, index, &address, &ttl)
				== OT_ERROR_NONE) {
			//  OutputIp6Address(address);
			// OutputFormat(" TTL:%lu ", ToUlong(ttl));
			index++;
		}
	}

}

otError dns::resolveHost(char *host) {

	//assert(otDnsClientResolveAddress(otGetInstance(), host, &(this->resolveRespHandler),
	// NULL, &config) == OT_ERROR_NONE);
	return OT_ERROR_PENDING;

}
#endif // APP_DNS_RESOLVE_IMPLEMENTED
