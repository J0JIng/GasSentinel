#include "app_thread.h"
#include "app_main.h"
#include <string.h>
#include <assert.h>

#include <openthread/cli.h>
#include <openthread/dataset_ftd.h>
#include <openthread/instance.h>
#include <openthread/message.h>
#include <openthread/thread.h>
#include <openthread/udp.h>
#include <openthread/platform/logging.h>
#include <common/code_utils.hpp>
#include <common/logging.hpp>
#include <openthread/srp_client.h>
#include <openthread/srp_client_buffers.h>

#include "stdio.h"
#include "string.h"

#include "sl_component_catalog.h"
#ifdef SL_CATALOG_POWER_MANAGER_PRESENT
#include "sl_power_manager.h"
#endif

// Constants
#define MULTICAST_ADDR "ff03::1"
#define MULTICAST_PORT 123
#define RECV_PORT 234
#define SLEEPY_POLL_PERIOD_MS 2000


// Forward declarations

//extern void otSysEventSignalPending(void);

otInstance *otGetInstance(void);


static bool                sAllowSleep                    = true;

void sleepyInit(void)
{
    otError error;
    otCliOutputFormat("[APP THREAD][I] app thread started\r\n");

    otLinkModeConfig config;
    SuccessOrExit(error = otLinkSetPollPeriod(otGetInstance(), SLEEPY_POLL_PERIOD_MS));

    config.mRxOnWhenIdle = false;
    config.mDeviceType   = 0;
    config.mNetworkData  = 0;
    SuccessOrExit(error = otThreadSetLinkMode(otGetInstance(), config));

exit:
    if (error != OT_ERROR_NONE)
    {
        otCliOutputFormat("[APP THREAD][E] Initialization failed with: %d, %s\r\n", error, otThreadErrorToString(error));
    }
    return;
}

/*
 * Callback from sl_ot_is_ok_to_sleep to check if it is ok to go to sleep.
 */
bool efr32AllowSleepCallback(void)
{
	otCliOutputFormat("[APP THREAD][I] Sleep \n");
    return sAllowSleep;
}

/*
 * Override default network settings, such as panid, so the devices can join a network
 */
void setNetworkConfiguration(void)
{
    static char          aNetworkName[] = "";
    otError              error;
    otOperationalDataset aDataset;

    memset(&aDataset, 0, sizeof(otOperationalDataset));

    /*
     * Fields that can be configured in otOperationDataset to override defaults:
     *     Network Name, Mesh Local Prefix, Extended PAN ID, PAN ID, Delay Timer,
     *     Channel, Channel Mask Page 0, Network Key, PSKc, Security Policy
     */
    //aDataset.mActiveTimestamp.mSeconds             = 1;
    aDataset.mComponents.mIsActiveTimestampPresent = true;

    /* Set Channel to 15 */
    aDataset.mChannel                      = 15;
    aDataset.mComponents.mIsChannelPresent = true;

    /* Set Pan ID to 2222 */
    aDataset.mPanId                      = (otPanId)0;
    aDataset.mComponents.mIsPanIdPresent = true;

    /* Set Extended Pan ID to C0DE1AB5C0DE1AB5 */
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = { };
    memcpy(aDataset.mExtendedPanId.m8, extPanId, sizeof(aDataset.mExtendedPanId));
    aDataset.mComponents.mIsExtendedPanIdPresent = true;

    /* Set network key to 1234C0DE1AB51234C0DE1AB51234C0DE */
    uint8_t key[OT_NETWORK_KEY_SIZE] = { };
    memcpy(aDataset.mNetworkKey.m8, key, sizeof(aDataset.mNetworkKey));
    aDataset.mComponents.mIsNetworkKeyPresent = true;

    /* Set Network Name to SleepyEFR32 */
    size_t length = strlen(aNetworkName);
    assert(length <= OT_NETWORK_NAME_MAX_SIZE);
    memcpy(aDataset.mNetworkName.m8, aNetworkName, length);
    aDataset.mComponents.mIsNetworkNamePresent = true;

    /* Set the Active Operational Dataset to this dataset */
    error = otDatasetSetActive(otGetInstance(), &aDataset);
    if (error != OT_ERROR_NONE)
    {
        otCliOutputFormat("[APP THREAD][E] otDatasetSetActive failed with: %d, %s\r\n", error, otThreadErrorToString(error));
        return;
    }
}




void initUdp(void)
{

}



void appSrpInit(void)
{
    otError error = OT_ERROR_NONE;

    char *hostName;
    const char *HOST_NAME = "OT-test-0";
    uint16_t size;
    hostName = otSrpClientBuffersGetHostNameString(otGetInstance(), &size);
    error = otSrpClientSetHostName(otGetInstance(), HOST_NAME);
    memcpy(hostName, HOST_NAME, sizeof(HOST_NAME) + 1);


    otSrpClientEnableAutoHostAddress(otGetInstance());


    otSrpClientBuffersServiceEntry *entry = NULL;
    char *string;

    entry = otSrpClientBuffersAllocateService(otGetInstance());

    entry->mService.mPort = 33434;
    char INST_NAME[32];
    snprintf(INST_NAME, 32, "ipv6bc%d", (uint8_t)(1));
    const char *SERV_NAME = "_ot._udp";
    string = otSrpClientBuffersGetServiceEntryInstanceNameString(entry, &size);
    memcpy(string, INST_NAME, size);


    string = otSrpClientBuffersGetServiceEntryServiceNameString(entry, &size);
    memcpy(string, SERV_NAME, size);

    error = otSrpClientAddService(otGetInstance(), &entry->mService);

    assert(OT_ERROR_NONE == error);

    entry = NULL;

    otSrpClientEnableAutoStartMode(otGetInstance(), /* aCallback */ NULL, /* aContext */ NULL);
}


#ifdef SL_CATALOG_KERNEL_PRESENT
#define applicationTick sl_ot_rtos_application_tick
#endif

void applicationTick(void)
{


#if (defined(SL_CATALOG_KERNEL_PRESENT) && defined(SL_CATALOG_POWER_MANAGER_PRESENT))
        if (sAllowSleep)
        {
            sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
        }
        else
        {
            sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
        }
#endif

}
