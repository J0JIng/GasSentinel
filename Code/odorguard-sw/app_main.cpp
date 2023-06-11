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

#include "bme68x.h"
#include "bsec_interface.h"
#include "sl_i2cspm.h"
#include "sl_sleeptimer.h"
#include "sl_udelay.h"
#include "app_dns.h"

#include <cstring>

void sleepyInit(void);
void setNetworkConfiguration(void);
void initUdp(void);
void app_init_bme(void);


struct bme68x_dev bme;
static const uint8_t bme_addr = BME68X_I2C_ADDR_LOW;

static __IO union {
    uint64_t _64b;
    struct {
        uint32_t l;
        uint32_t h;
    } _32b;
} eui;

extern "C" void otAppCliInit(otInstance *aInstance);

static otInstance* sInstance = NULL;

otInstance *otGetInstance(void)
{
    return sInstance;
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

BME68X_INTF_RET_TYPE app_i2c_plat_read(uint8_t reg_addr, uint8_t *reg_data,
		uint32_t length, void *intf_ptr) {

	I2C_TransferSeq_TypeDef i2cTransfer;

	// Initialize I2C transfer
	i2cTransfer.addr = bme_addr << 1;
	i2cTransfer.flags = I2C_FLAG_WRITE_READ; // must write target address before reading
	i2cTransfer.buf[0].data = &reg_addr;
	i2cTransfer.buf[0].len = 1;
	i2cTransfer.buf[1].data = reg_data;
	i2cTransfer.buf[1].len = (uint16_t)length;

	return I2CSPM_Transfer(I2C0, &i2cTransfer);

}

BME68X_INTF_RET_TYPE app_i2c_plat_write(uint8_t reg_addr, const uint8_t *reg_data,
		uint32_t length, void *intf_ptr) {

	I2C_TransferSeq_TypeDef i2cTransfer;

	// Initialize I2C transfer
	i2cTransfer.addr = bme_addr << 1;
	i2cTransfer.flags = I2C_FLAG_WRITE_WRITE;
	i2cTransfer.buf[0].data = &reg_addr;
	i2cTransfer.buf[0].len = 1;
	i2cTransfer.buf[1].data = (uint8_t *)reg_data;
	i2cTransfer.buf[1].len = (uint16_t)length;

	return I2CSPM_Transfer(I2C0, &i2cTransfer);

}

void app_init(void) {
	app_init_bme();
	sleepyInit();
	setNetworkConfiguration();
	initUdp();
	assert(otIp6SetEnabled(sInstance, true) == OT_ERROR_NONE);
	assert(otThreadSetEnabled(sInstance, true) == OT_ERROR_NONE);
	eui._64b = SYSTEM_GetUnique();
}

void app_init_bme(void)
{
	memset(&bme, 0, sizeof(bme));
	bme.intf = BME68X_I2C_INTF;
	bme.read = app_i2c_plat_read;
	bme.write = app_i2c_plat_write;
	//bme.delay_us = sl_udelay_wait;
	bme.intf_ptr = NULL;
	bme.amb_temp = 25; // TODO ref. sht22 bl calib
	assert(bme68x_init(&bme) == BME68X_OK);

	assert(bsec_init() == BSEC_OK);

	bsec_sensor_configuration_t requested_virtual_sensors[1];
	uint8_t n_requested_virtual_sensors = 1;

	bsec_sensor_configuration_t required_sensor_settings[1];
	uint8_t n_required_sensor_settings = 1;

	requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_IAQ;
	requested_virtual_sensors[0].sample_rate = 0.33f;

	assert(
			bsec_update_subscription(requested_virtual_sensors,
					n_requested_virtual_sensors, required_sensor_settings,
					&n_required_sensor_settings) == BSEC_OK);


	dns::resolveHost(NULL);
}


void app_process_action(void)
{
    otTaskletsProcess(sInstance);
    otSysProcessDrivers(sInstance);
    applicationTick();
}


void app_exit(void)
{
	// TODO replace with reset irqn handler
    otInstanceFinalize(sInstance);
}
