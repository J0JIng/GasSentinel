#ifndef APP_DNS_H_
#define APP_DNS_H_


#include <openthread/dns.h>
#include <openthread/error.h>
#include <cli_output.hpp>
#include "cli/cli_config.h"
#include "cli/cli_output.hpp"
#include <cli.hpp>

 /*
  * Interpreter getInterpreter(void)
    {
    	return *sInterpreter;
    }

 [[nodiscard]] __attribute__((weak)) ot::Cli::Interpreter getInterpreter(void)
 {
	 // linker overrides with c++ fn
 }
 */

class dns {
public:
	dns(uint8_t max_entries){
		browseRespInfo = new otDnsServiceInfo[max_entries];
		for(uint8_t i=0; i<max_entries; i++)
		{
			browseRespInfo[i].mHostNameBuffer =  (char *)calloc(255, sizeof(char));
			browseRespInfo[i].mHostNameBufferSize = 255;

			browseRespInfo[i].mTxtData = (uint8_t *)calloc(512, sizeof(uint8_t));
			browseRespInfo[i].mTxtDataSize = 512;
		}
	}

	otError resolveHost(char *host);
	otError browse(char *name);

	bool browseResultReady(otDnsServiceInfo **info, uint8_t *num);

private:


	otDnsServiceInfo *browseRespInfo;
	uint8_t browseRespInfoValid = 0;
	bool browseRespInfoReady = false;

	void browseRespHandler(otError aError, const otDnsBrowseResponse *aResponse,
			void *aContext);
	void resolveRespHandler(otError aError,
			const otDnsAddressResponse *aResponse, void *aContext);

	static void browseRespHandlerWrapper(otError error,
			const otDnsBrowseResponse *response, void *context) {
		dns *instance = static_cast<dns*>(context);
		instance->browseRespHandler(error, response, context);
	}
};

#endif /* APP_DNS_H_ */
