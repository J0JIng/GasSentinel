#include "app_dns.h"
#include "app_thread.h"
#include "app_sensor.h"
#include "app_coap.h"
#include <app_main.h>
#include <assert.h>
#include <config/bsec_selectivity.h>
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
#include "em_prs.h"
#include "em_iadc.h"
#include "bme68x.h"
#include "bsec_interface.h"
#include "sl_i2cspm.h"
#include "sl_sleeptimer.h"
#include "sl_udelay.h"
#include "mx25_driver.h"

#include <cstring>
#include <cmath>

// Forward declarations
void sleepyInit(void);
void setNetworkConfiguration(void);
void initUdp(void);
void app_init_bme(uint8_t sense_pin);

// DNS related variables
dns DNS(otGetInstance, 4, 4);
static otIp6Address server;
static bool found_server = false;
static bool pend_resolve_server = false;
constexpr static char *endpoint_dnssd_name = "_coap._udp.default.service.arpa.";

// BME/BSEC related variables
struct bme68x_dev bme;
static uint8_t bme_addr = BME68X_I2C_ADDR_LOW;
static volatile bool bsec_run = true;
uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE];
uint8_t bsec_state[BSEC_MAX_STATE_BLOB_SIZE];

sl_sleeptimer_timer_handle_t resolve_server_timer;
constexpr static uint32_t RESOLVE_POST_EST_CONN_MS = 5000;

static uint32_t FLASH_PAGE_SIZE_U = 0x100;
static uint32_t FLASH_DATA_VALID_REGION_START = 0x00000000;
static uint32_t FLASH_DATA_VALID_REGION_LEN = 0x4;
static uint8_t FLASH_DATA_VALID_MARKER[4] = { 0x2F, 0xC6, 0x9A, 0x44 };

static uint32_t FLASH_DATA_WBUF_REGION_START = 0x00000000
		+ ceil(FLASH_DATA_VALID_REGION_LEN / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;
static uint32_t FLASH_DATA_WBUF_REGION_LEN = BSEC_MAX_WORKBUFFER_SIZE;

static uint32_t FLASH_DATA_BLOB_REGION_START = 0x00000000
		+ ceil(FLASH_DATA_VALID_REGION_LEN / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE
		+ ceil(FLASH_DATA_WBUF_REGION_LEN / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;
static uint32_t FLASH_DATA_BLOB_REGION_LEN = BSEC_MAX_STATE_BLOB_SIZE;

eui_t eui; // Device unique identifier


static enum { // Denotes power mode
	BATT = 0,
	USB = 1,
} power_source;

extern "C" void otAppCliInit(otInstance *aInstance);

static otInstance* sInstance = NULL;

static BME68X_INTF_RET_TYPE app_i2c_plat_read(uint8_t reg_addr, uint8_t *reg_data,
		uint32_t length, void *intf_ptr);
static BME68X_INTF_RET_TYPE app_i2c_plat_write(uint8_t reg_addr, const uint8_t *reg_data,
		uint32_t length, void *intf_ptr);


void app_init_bme(void);
void app_burtc_callback(uint32_t dur);

/* Interrupt handler for BURTC */
void BURTC_IRQHandler(void)
{
    BURTC_IntClear(BURTC_IF_COMP); // compare match
	BURTC_CounterReset();
	BURTC_Stop();
	bsec_run = true;
	BURTC_IntDisable(BURTC_IEN_COMP);
}

/* Callback function for resetting BURTC in sensor proc context */
void app_burtc_callback(uint32_t dur)
{
	BURTC_CounterReset();
	BURTC_CompareSet(0, dur);
	BURTC_IntEnable(BURTC_IEN_COMP);
}

/* Interrupt handler for IADC */
void IADC_IRQHandler(void){
  IADC_Result_t sample;
  sample = IADC_pullSingleFifoResult(IADC0);
  coap::vsense_batt = (sample.data * 1200)/1000;
  IADC_clearInt(IADC0, IADC_IF_SINGLEDONE);
}

/* Callback function for DNS resolve */
void resolveServerHandler(sl_sleeptimer_timer_handle_t *handle, void *data)
{
	pend_resolve_server = true;
}



/* Getter function for otinstance */
otInstance *otGetInstance(void)
{
    return sInstance;
}

/** Application initialization **/
void app_init(void) {

	GPIO_PinOutSet(ERR_LED_PORT, ERR_LED_PIN);
	uint8_t sense = 0x1 & GPIO_PinInGet(SW_PWR_SRC_PORT, SW_PWR_SRC_PIN); // Read input power source and mask first bit
	app_init_bme(sense);
	GPIO_PinModeSet(SW_PWR_SRC_PORT, SW_PWR_SRC_PIN, gpioModeDisabled, 0); // Disable after read to reduce static power
	/* ot initialization procedure */
	sleepyInit();
	setNetworkConfiguration();

	assert(otIp6SetEnabled(sInstance, true) == OT_ERROR_NONE);
	assert(otThreadSetEnabled(sInstance, true) == OT_ERROR_NONE);

	/* dns initialization procedure */
	appSrpInit();
	sl_sleeptimer_start_timer_ms(&resolve_server_timer, RESOLVE_POST_EST_CONN_MS, resolveServerHandler, NULL, 0, 0);
	found_server = false;
	pend_resolve_server = false;

	//otCliOutputFormat("[APP MAIN][I] Initialization successful \n");
	eui._64b = SYSTEM_GetUnique();

	GPIO_PinOutSet(IP_LED_PORT, IP_LED_PIN);
	GPIO_PinOutClear(ERR_LED_PORT, ERR_LED_PIN);
}


/** Application main loop **/

void app_process_action(void)
{
	/** System tasks **/
	otTaskletsProcess(sInstance);
	otSysProcessDrivers(sInstance);

	/** DNS resolution yield **/
	if(pend_resolve_server) {
		pend_resolve_server = false;
		DNS.browse(endpoint_dnssd_name);
		//otCliOutputFormat("[APP MAIN][I] search started\n");
		GPIO_PinOutSet(IP_LED_PORT, IP_LED_PIN);
	}
	if(!found_server)
	{
		otDnsServiceInfo *info;
		uint8_t sz = 0;
		if(DNS.browseResultReady(&info, &sz))
		{
			if(sz == 0) {
				//otCliOutputFormat("[APP MAIN][W] Failed search \n");
				pend_resolve_server = true;
				GPIO_PinOutSet(ERR_LED_PORT, ERR_LED_PIN);
			}
			else {
				//otCliOutputFormat("[APP MAIN][I] Server found \n");
				server = (*info).mHostAddress;
				char string[OT_IP6_ADDRESS_STRING_SIZE];
				otIp6AddressToString(&server, string, sizeof(string));
				//otCliOutputFormat("[APP MAIN][I] Server IPv6 resolved as %s\n", string);
				found_server = true;
				coap::init(server);
				GPIO_PinOutClear(ERR_LED_PORT, ERR_LED_PIN);
			}
			GPIO_PinOutClear(IP_LED_PORT, IP_LED_PIN);
		} else GPIO_PinOutToggle(IP_LED_PORT, IP_LED_PIN);

	}
	GPIO_PinOutSet(ACT_LED_PORT, ACT_LED_PIN);

	/** sensor algo yield **/
	if (bsec_run) {
		bsec_run = false;
		sensor::proc(app_burtc_callback);
	}

	/** coap yield **/
	bool ret = coap::service();

	GPIO_PinOutClear(ACT_LED_PORT, ACT_LED_PIN);

}



void app_delay_us(uint32_t period, void *intf_ptr)
{
	sl_sleeptimer_delay_millisecond(period/1000);
}


uint8_t _app_mx25_get_state(void)
{
	MX25_PP(FLASH_DATA_VALID_REGION_START, FLASH_DATA_VALID_MARKER, FLASH_DATA_VALID_REGION_LEN);
	uint8_t buf[4];
	MX25_READ(FLASH_DATA_VALID_REGION_START, buf, FLASH_DATA_VALID_REGION_LEN);
	for(uint8_t i=0; i<sizeof(buf); i++)
	{
		if(buf[i] != FLASH_DATA_VALID_MARKER[i]) return 0;
	}
	return 1;
}

uint8_t _app_mx25_get_data(void)
{
	uint8_t ret = 0;
	ret |= MX25_READ(FLASH_DATA_WBUF_REGION_START, work_buffer, FLASH_DATA_WBUF_REGION_LEN);
	ret |= MX25_READ(FLASH_DATA_BLOB_REGION_START, bsec_state, FLASH_DATA_BLOB_REGION_LEN);
}

uint8_t _app_mx25_write_data(void)
{

	uint8_t ret = 0;
	uint32_t offset_pages = 0;

	if(ret == 0)
		MX25_PP(FLASH_DATA_VALID_REGION_START, FLASH_DATA_VALID_MARKER, FLASH_DATA_VALID_REGION_LEN);
	else GPIO_PinOutSet(ERR_LED_PORT, ERR_LED_PIN);
	GPIO_PinOutClear(ACT_LED_PORT, ACT_LED_PIN);
}

void app_init_bme(uint8_t sense_pin)
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

	 //otCliOutputFormat("[APP MAIN][I] BSEC ret codes: %d / ", ret);

	ret = bsec_init();
	//otCliOutputFormat("%d / ", ret);

	/* Set configuration according to detected power source */
	uint8_t is_exist_wbuf = _app_mx25_get_state();
	// TODO: modify work buffer to save to spi flash
	if (sense_pin == BATT) {
		uint32_t bsec_config_len = sizeof(bsec_config_selectivity);
		ret = bsec_set_configuration(bsec_config_selectivity, bsec_config_len,
				work_buffer, sizeof(work_buffer));
	}

	else if (sense_pin == USB) {
		uint32_t bsec_config_len = sizeof(bsec_config_selectivity_std);
		ret = bsec_set_configuration(bsec_config_selectivity_std, bsec_config_len,
				work_buffer, sizeof(work_buffer));
		GPIO_PinOutSet(ERR_LED_PORT, ERR_LED_PIN);
		GPIO_PinOutSet(ACT_LED_PORT, ACT_LED_PIN);
		GPIO_PinOutSet(IP_LED_PORT, IP_LED_PIN);
		sl_sleeptimer_delay_millisecond(1500);
		GPIO_PinOutClear(ERR_LED_PORT, ERR_LED_PIN);
		GPIO_PinOutClear(ACT_LED_PORT, ACT_LED_PIN);
		GPIO_PinOutClear(IP_LED_PORT, IP_LED_PIN);
	}

	if(is_exist_wbuf) _app_mx25_get_data();

	//MX25_DP();
	//sl_udelay_wait(30); // tDPDD

	bsec_set_state(bsec_state, sizeof(bsec_state), work_buffer, sizeof(work_buffer));

	GPIO_PinOutSet(ACT_LED_PORT, ACT_LED_PIN);
	//_app_mx25_write_data();
	//otCliOutputFormat("%d / ", ret);
	GPIO_PinOutClear(ACT_LED_PORT, ACT_LED_PIN);

	bsec_sensor_configuration_t requested_virtual_sensors[9];
	uint8_t n_requested_virtual_sensors = 9;

	bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
	uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

	// virtual sensor configuration
    requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_1;
    requested_virtual_sensors[0].sample_rate = BSEC_SAMPLE_RATE_SCAN;
    requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_2;
    requested_virtual_sensors[1].sample_rate = BSEC_SAMPLE_RATE_SCAN;
    requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_3;
    requested_virtual_sensors[2].sample_rate = BSEC_SAMPLE_RATE_SCAN;
    requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_4;
    requested_virtual_sensors[3].sample_rate = BSEC_SAMPLE_RATE_SCAN;
    requested_virtual_sensors[4].sensor_id = BSEC_OUTPUT_RAW_PRESSURE;
    requested_virtual_sensors[4].sample_rate = BSEC_SAMPLE_RATE_SCAN;
    requested_virtual_sensors[5].sensor_id = BSEC_OUTPUT_RAW_TEMPERATURE;
    requested_virtual_sensors[5].sample_rate = BSEC_SAMPLE_RATE_SCAN;
    requested_virtual_sensors[6].sensor_id = BSEC_OUTPUT_RAW_HUMIDITY;
    requested_virtual_sensors[6].sample_rate = BSEC_SAMPLE_RATE_SCAN;
    requested_virtual_sensors[7].sensor_id = BSEC_OUTPUT_RAW_GAS;
    requested_virtual_sensors[7].sample_rate = BSEC_SAMPLE_RATE_SCAN;
    requested_virtual_sensors[8].sensor_id = BSEC_OUTPUT_RAW_GAS_INDEX;
    requested_virtual_sensors[8].sample_rate = BSEC_SAMPLE_RATE_SCAN;


	ret = bsec_update_subscription(requested_virtual_sensors,
					n_requested_virtual_sensors, required_sensor_settings,
					&n_required_sensor_settings);
	 //otCliOutputFormat("%d\n", ret);
	 bsec_version_t ver;
	 bsec_get_version(&ver);
	 //otCliOutputFormat("[APP MAIN][I] BSEC Version: v%d.%d.%d.%d\n", ver.major,ver.minor,ver.major_bugfix,ver.minor_bugfix);
}






void app_exit(void)
{
    otInstanceFinalize(sInstance);
}


void sl_ot_create_instance(void)
{

    sInstance = otInstanceInitSingle();
    assert(sInstance);
}

void sl_ot_cli_init(void)
{
    //otAppCliInit(sInstance);
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
