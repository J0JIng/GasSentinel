#include "app_dns.h"
#include "app_thread.h"
#include "app_sensor.h"
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
#include "em_system.h"
#include "sl_component_catalog.h"
#include "em_burtc.h"
#include "bme68x.h"
#include "bsec_interface.h"
#include "sl_i2cspm.h"
#include "sl_sleeptimer.h"
#include "sl_udelay.h"

#include <cstring>

#include "bme680_iaq_33v_3s_28d/bsec_iaq.h"

void sleepyInit(void);
void setNetworkConfiguration(void);
void initUdp(void);
void app_init_bme(void);

dns DNS(otGetInstance, 4, 4);
static otIp6Address server;
static bool found_server = false;
static bool pend_resolve_server = false;

struct bme68x_dev bme;
static uint8_t bme_addr = BME68X_I2C_ADDR_LOW;
static volatile bool bsec_run = true;
static uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE];


sl_sleeptimer_timer_handle_t resolve_server_timer;
constexpr static uint32_t RESOLVE_POST_EST_CONN_MS = 5000;

static union {
    uint64_t _64b;
    struct {
        uint32_t l;
        uint32_t h;
    } _32b;
} eui;


extern "C" void otAppCliInit(otInstance *aInstance);

static otInstance* sInstance = NULL;
constexpr static char *endpoint_dnssd_name = "_ot._udp.default.service.arpa.";
static BME68X_INTF_RET_TYPE app_i2c_plat_read(uint8_t reg_addr, uint8_t *reg_data,
		uint32_t length, void *intf_ptr);
static BME68X_INTF_RET_TYPE app_i2c_plat_write(uint8_t reg_addr, const uint8_t *reg_data,
		uint32_t length, void *intf_ptr);
static I2C_TransferReturn_TypeDef I2C_detect1(I2C_TypeDef *i2c, uint8_t addr);
void bme68x_delay_us(uint32_t period, void *intf_ptr);
void app_init_bme(void);
void app_burtc_callback(uint32_t dur);

void BURTC_IRQHandler(void)
{
    BURTC_IntClear(BURTC_IF_COMP); // compare match
	BURTC_CounterReset();
	BURTC_Stop();
	bsec_run = true;
	BURTC_IntDisable(BURTC_IEN_COMP);
}

void resolveServerHandler(sl_sleeptimer_timer_handle_t *handle, void *data)
{
	pend_resolve_server = true;
}

otInstance *otGetInstance(void)
{
    return sInstance;
}

/** Application initialization **/
void app_init(void) {


	app_init_bme();
	sleepyInit();
	setNetworkConfiguration();
	initUdp();
	assert(otIp6SetEnabled(sInstance, true) == OT_ERROR_NONE);
	assert(otThreadSetEnabled(sInstance, true) == OT_ERROR_NONE);
	appSrpInit();
	sl_sleeptimer_start_timer_ms(&resolve_server_timer, RESOLVE_POST_EST_CONN_MS, resolveServerHandler, NULL, 0, 0);
	found_server = false;
	pend_resolve_server = false;

	eui._64b = SYSTEM_GetUnique();

}


/** Application main loop **/

void app_process_action(void)
{
	otTaskletsProcess(sInstance);
	otSysProcessDrivers(sInstance);
	applicationTick();
	if(pend_resolve_server) {
		pend_resolve_server = false;
		DNS.browse(endpoint_dnssd_name);
		otCliOutputFormat("search started");
	}
	if(!found_server)
	{
		otDnsServiceInfo *info;
		uint8_t sz = 0;
		if(DNS.browseResultReady(&info, &sz))
		{
			if(sz == 0) {
				otCliOutputFormat("Failed search \n");
				//pend_resolve_server = false;
			}
			else {
				otCliOutputFormat("Server found \n");
				server = (*info).mHostAddress;
				char string[OT_IP6_ADDRESS_STRING_SIZE];
				otIp6AddressToString(&server, string, sizeof(string));
				otCliOutputFormat("%s\n ", string);
				found_server = true;
				coap::init(server);
			}
		}

	}
	if (bsec_run) {
		bsec_run = false;
		sensor::proc(app_burtc_callback, sl_sleeptimer_get_time);
	}
	bool ret = coap::service();
	//if(!ret) otCliOutputFormat("Not sent \n");

}



void app_delay_us(uint32_t period, void *intf_ptr)
{
	sl_sleeptimer_delay_millisecond(period/1000);
}




void app_init_bme(void)
{

	memset(&bme, 0, sizeof(bme));
	bme.intf = BME68X_I2C_INTF;
	bme.read = app_i2c_plat_read;
	bme.write = app_i2c_plat_write;
	bme.delay_us = app_delay_us;
	bme.intf_ptr = &bme_addr;
	bme.amb_temp = 31; // TODO ref. sht22 bl calib
	assert(bme68x_init(&bme) == BME68X_OK);
	volatile int8_t ret = 0;

	 otCliOutputFormat("%d / ", ret);

	ret = bsec_init();
	otCliOutputFormat("%d / ", ret);

	uint32_t bsec_config_len = sizeof(bsec_config_iaq);

	ret = bsec_set_configuration(bsec_config_iaq, bsec_config_len, work_buffer, sizeof(work_buffer));

	otCliOutputFormat("%d / ", ret);

	bsec_sensor_configuration_t requested_virtual_sensors[8];
	uint8_t n_requested_virtual_sensors = 8;

	bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
	uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;


    requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_IAQ;
    requested_virtual_sensors[0].sample_rate = BSEC_SAMPLE_RATE_LP;
    requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_STABILIZATION_STATUS;
    requested_virtual_sensors[1].sample_rate = BSEC_SAMPLE_RATE_LP;
    requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_RUN_IN_STATUS;
    requested_virtual_sensors[2].sample_rate = BSEC_SAMPLE_RATE_LP;
    requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE;
    requested_virtual_sensors[3].sample_rate = BSEC_SAMPLE_RATE_LP;
    requested_virtual_sensors[4].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY;
    requested_virtual_sensors[4].sample_rate = BSEC_SAMPLE_RATE_LP;
    requested_virtual_sensors[5].sensor_id = BSEC_OUTPUT_RAW_PRESSURE;
    requested_virtual_sensors[5].sample_rate = BSEC_SAMPLE_RATE_LP;
    requested_virtual_sensors[6].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_1;
    requested_virtual_sensors[6].sample_rate = BSEC_SAMPLE_RATE_DISABLED;
    requested_virtual_sensors[7].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_2;
    requested_virtual_sensors[7].sample_rate = BSEC_SAMPLE_RATE_DISABLED;



	ret = bsec_update_subscription(requested_virtual_sensors,
					n_requested_virtual_sensors, required_sensor_settings,
					&n_required_sensor_settings);
	 otCliOutputFormat("%d / ", ret);
}


void app_burtc_callback(uint32_t dur)
{
	BURTC_CounterReset();
	BURTC_CompareSet(0, dur);
	BURTC_IntEnable(BURTC_IEN_COMP);
}



void app_exit(void)
{
	// TODO replace with reset irqn handler
    otInstanceFinalize(sInstance);
}


/*
 * Provide, if required an "otPlatLog()" function
 */
#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_APP
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
    OT_UNUSED_VARIABLE(aFormat);

    va_list ap;
    va_start(ap, aFormat);
    otCliPlatLogv(aLogLevel, aLogRegion, aFormat, ap);
    va_end(ap);
}
#endif

void sl_ot_create_instance(void)
{
#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    size_t   otInstanceBufferLength = 0;
    uint8_t *otInstanceBuffer       = NULL;

    // Call to query the buffer size
    (void)otInstanceInit(NULL, &otInstanceBufferLength);

    // Call to allocate the buffer
    otInstanceBuffer = (uint8_t *)malloc(otInstanceBufferLength);
    assert(otInstanceBuffer);

    // Initialize OpenThread with the buffer
    sInstance = otInstanceInit(otInstanceBuffer, &otInstanceBufferLength);
#else
    sInstance = otInstanceInitSingle();
#endif
    assert(sInstance);
}

void sl_ot_cli_init(void)
{
    otAppCliInit(sInstance);
}

static BME68X_INTF_RET_TYPE app_i2c_plat_read(uint8_t reg_addr, uint8_t *reg_data,
		uint32_t length, void *intf_ptr) {

	I2C_TransferSeq_TypeDef i2cTransfer;
	uint8_t device_addr = *(uint8_t*)intf_ptr;
	// Initialize I2C transfer
	i2cTransfer.addr = device_addr << 1;
	i2cTransfer.flags = I2C_FLAG_WRITE_READ; // must write target address before reading
	i2cTransfer.buf[0].data = &reg_addr;
	i2cTransfer.buf[0].len = 1;
	i2cTransfer.buf[1].data = reg_data;
	i2cTransfer.buf[1].len = (uint16_t)length;

	return I2CSPM_Transfer(I2C0, &i2cTransfer);

}

static BME68X_INTF_RET_TYPE app_i2c_plat_write(uint8_t reg_addr, const uint8_t *reg_data,
		uint32_t length, void *intf_ptr) {

	I2C_TransferSeq_TypeDef i2cTransfer;
	uint8_t device_addr = *(uint8_t*)intf_ptr;
	// Initialize I2C transfer
	i2cTransfer.addr = device_addr << 1;
	i2cTransfer.flags = I2C_FLAG_WRITE_WRITE;
	i2cTransfer.buf[0].data = &reg_addr;
	i2cTransfer.buf[0].len = 1;
	i2cTransfer.buf[1].data = (uint8_t *)reg_data;
	i2cTransfer.buf[1].len = (uint16_t)length;

	return I2CSPM_Transfer(I2C0, &i2cTransfer);

}
static I2C_TransferReturn_TypeDef I2C_detect1(I2C_TypeDef *i2c, uint8_t addr) {

	I2C_TransferSeq_TypeDef i2cTransfer;
	I2C_TransferReturn_TypeDef result;

	// Initialize I2C transfer
	i2cTransfer.addr = addr << 1;
	i2cTransfer.flags = I2C_FLAG_WRITE;
	i2cTransfer.buf[0].data = NULL;
	i2cTransfer.buf[0].len = 0;

	return I2CSPM_Transfer(i2c, &i2cTransfer);

}
