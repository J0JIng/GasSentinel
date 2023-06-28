#ifndef APP_DNS_H_
#define APP_DNS_H_


#include <openthread/dns.h>
#include <openthread/error.h>
#include <cli_output.hpp>
#include "cli/cli_config.h"
#include "cli/cli_output.hpp"
#include <cli.hpp>

 /* if otinterpreter is needed, add the functions below
  * and inherit dns class from ot::Cli::Interpreter
  *
 Interpreter getInterpreter(void) - in cli.cpp
    {
    	return *sInterpreter;
    }

 __attribute__((weak)) ot::Cli::Interpreter getInterpreter(void) - in this file
 {
	 // linker overrides with c++ fn
 }
 */
/**
 * @class dns
 * @brief Provides basic DNS browsing and resolution functionality
 *
 */
class dns {
public:
	/*
	 @brief Constructor for the dns class.
	 @param getotInst Function pointer to function that returns the current ot instance
	 @param max_entries The maximum number of DNS service entries to store.
	 */
	dns(otInstance *(*getotInst)(void), uint8_t max_dns, uint8_t max_addr);
	/**



	 @brief Performs a DNS service browse operation.
	 @param name The name to browse for DNS services.
	 @return OT_ERROR_PENDING if awaiting response, OT_ERROR_NO_BUFS if failed
	 */
	otError browse(char *name);
	/**

	 @brief Checks if a browse result is available.
	 @param[out] info Pointer to the array of otDnsServiceInfo structures holding browse results.
	 @param[out] num Pointer to the number of valid results.
	 @return True if a browse result is ready, false otherwise.
	 */
	bool browseResultReady(otDnsServiceInfo **info, uint8_t *num);
	/**

	 @brief Checks if the browse result has overflowed the available buffers.
	 @return True if the browse result has overflowed, false otherwise.
	 */
	bool browseResultIsOverflow(void);



	/**
	 * @brief Resolves the IP address for a given hostname.
	 * @param label The label to resolve.
	 * @param name The name to resolve.
	 * @return OT_ERROR_PENDING if awaiting response, OT_ERROR_NO_BUFS if failed.
	 */
	otError resolveHost(char *label, char *name);

	/**

	 @brief Checks if a resolve host result is available.
	 @param[out] info Pointer to single otDnsServiceInfo entry result
	 @return True if a resolve result is ready, false otherwise.
	 */
	bool resolveHostResultReady(otDnsServiceInfo *info);


	/**
	 * @brief Resolves the IPv4 (A) addresses for a given hostname.
	 * @param name The name to resolve.
	 * @return OT_ERROR_PENDING if awaiting response, OT_ERROR_NO_BUFS if failed.
	 */
	otError resolve4(char *name);

	/**
	 * @brief Resolves the IPv6 (AAAA) addresses for a given hostname.
	 * @param name The name to resolve.
	 * @return OT_ERROR_PENDING if awaiting response, OT_ERROR_NO_BUFS if failed.
	 */
	otError resolve6(char *name);
	/**

	 @brief Checks if a resolve ip result is available.
	 @param[out] info Pointer to the array of otIp6Address structures holding resolve ip results.
	 @param[out] num Pointer to the number of valid results.
	 @return True if a resolve result is ready, false otherwise.
	 */
	bool resolveIpResultReady(otIp6Address **info, uint8_t *num);

	/**

	 @brief Checks if the resolve result has overflowed the available buffers.
	 @return True if the resolve result has overflowed, false otherwise.
	 */
	bool resolveIpResultReadyIsOverflow(void);

private:

	otInstance *(*getInst)(void) = NULL; // Function pointer to get ot instance during runtime
	char label[OT_DNS_MAX_LABEL_SIZE]; // buffer for dns label info
	otDnsQueryConfig config; // Dns configuration struct

	otDnsServiceInfo *browseRespInfo; /**< Array to store browse response information */
	uint8_t browseRespInfoCount = 0; /**< Number of entries in browseRespInfo array */
	uint8_t browseRespInfoValid = 0; /**< Number of valid entries in browseRespInfo array */
	bool browseRespInfoReady = false; /**< Flag indicating if browse response is ready */
	bool browseRespInfoOverflow = false; /**< Flag indicating if browse response has overflowed */

	otDnsServiceInfo serviceRespInfo; /**< Struct to store service response information */
	bool serviceRespInfoReady = false; /**< Flag indicating if service response is ready */

	otIp6Address *addrRespInfo; /**< Array to store browse response information */
	uint8_t addrRespInfoCount = 0; /**< Number of entries in browseRespInfo array */
	uint8_t addrRespInfoValid = 0; /**< Number of valid entries in browseRespInfo array */
	bool addrRespInfoReady = false; /**< Flag indicating if browse response is ready */
	bool addrRespInfoOverflow = false; /**< Flag indicating if browse response has overflowed */

    /* DNS Handlers */
	/**
	 * @brief Handler for browse response events.
	 * @param aError The error code of the browse response.
	 * @param aResponse The browse response data.
	 * @param aContext The context pointer.
	 */
	void browseRespHandler(otError aError, const otDnsBrowseResponse *aResponse,
	                        void *aContext);

	/**
	 * @brief Handler for resolve host response events.
	 * @param aError The error code of the resolve host response.
	 * @param aResponse The resolve host response data.
	 * @param aContext The context pointer.
	 */
	void resolveHostRespHandler(otError aError, const otDnsServiceResponse *aResponse,
	                        void *aContext);
    /**
     * @brief Handler for resolve IP response events.
     * @param aError The error code of the resolve IP response.
     * @param aResponse The resolve IP response data.
     * @param aContext The context pointer.
     */
	void resolveIpRespHandler(otError aError,
			                const otDnsAddressResponse *aResponse, void *aContext);


	/* DNS Handler Wrapper Functions */
	/**
	 * @brief Wrapper function for browse response handler.
	 * @param error The error code of the browse response.
	 * @param response The browse response data.
	 * @param context The context pointer.
	 */
	static void browseRespHandlerWrapper(otError error, const otDnsBrowseResponse *response,
	                                        void *context) {
	    dns *instance = static_cast<dns*>(context);
	    instance->browseRespHandler(error, response, context);
	}

	/**
	 * @brief Wrapper function for resolve response handler.
	 * @param error The error code of the resolve response.
	 * @param response The resolve response data.
	 * @param context The context pointer.
	 */
	static void resolveHostRespHandlerWrapper(otError error, const otDnsServiceResponse *response,
	                                        void *context) {
	    dns *instance = static_cast<dns*>(context);
	    instance->resolveHostRespHandler(error, response, context);
	}

	/**
	 * @brief Wrapper function for resolve response handler.
	 * @param error The error code of the resolve response.
	 * @param response The resolve response data.
	 * @param context The context pointer.
	 */
	static void resolveIpRespHandlerWrapper(otError error, const otDnsAddressResponse *response,
	                                        void *context) {
	    dns *instance = static_cast<dns*>(context);
	    instance->resolveIpRespHandler(error, response, context);
	}

};

#endif /* APP_DNS_H_ */
