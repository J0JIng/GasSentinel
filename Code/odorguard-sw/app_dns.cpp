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

#include <openthread/dns_client.h>
#include <openthread/dns.h>


#include "utils/code_utils.h"

#include "stdio.h"
#include "string.h"


namespace dns {

 static otDnsQueryConfig config;

void browseRespHandler(otError aError, const otDnsBrowseResponse *aResponse, void *aContext)
{
    char             name[OT_DNS_MAX_NAME_SIZE];
    char             label[OT_DNS_MAX_LABEL_SIZE];
    uint8_t          txtBuffer[512];
    otDnsServiceInfo serviceInfo;


    if (aError == OT_ERROR_NONE)
    {
        uint16_t index = 0;

        while (otDnsBrowseResponseGetServiceInstance(aResponse, index, label, sizeof(label)) == OT_ERROR_NONE)
        {

            index++;

            serviceInfo.mHostNameBuffer     = name;
            serviceInfo.mHostNameBufferSize = sizeof(name);
            serviceInfo.mTxtData            = NULL;
            serviceInfo.mTxtDataSize        = sizeof(txtBuffer);

            // todo hand over struct to main

        }
    }

}


void resolveRespHandler(otError aError, const otDnsAddressResponse *aResponse, void *aContext)
{
    char         hostName[OT_DNS_MAX_NAME_SIZE];
    otIp6Address address;
    uint32_t     ttl;

   // IgnoreError(otDnsAddressResponseGetHostName(aResponse, hostName, sizeof(hostName)));

    //OutputFormat("DNS response for %s - ", hostName);

    if (aError == OT_ERROR_NONE)
    {
        uint16_t index = 0;

        while (otDnsAddressResponseGetAddress(aResponse, index, &address, &ttl) == OT_ERROR_NONE)
        {
          //  OutputIp6Address(address);
           // OutputFormat(" TTL:%lu ", ToUlong(ttl));
            index++;
        }
    }

}


otError browse(char *name) {


  assert( otDnsClientBrowse(otGetInstance(), name, &browseRespHandler, NULL, &config) == OT_ERROR_NONE);
  return OT_ERROR_PENDING;

}


otError resolveHost(char *host) {

  assert(otDnsClientResolveAddress(otGetInstance(), host, &resolveRespHandler,
                                                      NULL, &config) == OT_ERROR_NONE);
  return OT_ERROR_PENDING;

}
}
