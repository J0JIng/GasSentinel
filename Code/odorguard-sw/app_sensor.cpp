#include "app_sensor.h"
#include "app_main.h"
#include "app_coap.h"
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


#define BSEC_CHECK_INPUT(x, shift)		(x & (1 << (shift-1)))
#define BSEC_TOTAL_HEAT_DUR             UINT16_C(140)

namespace sensor {

static struct bme68x_conf bme_conf;
static struct bme68x_heatr_conf bme_heat_conf;
static struct bme68x_data bme_data[3];
static struct bme68x_data data;
static bsec_bme_settings_t bme_stg;
static uint8_t nFieldsLeft = 0;
static uint8_t nFields, iFields;

static uint8_t lastOpMode = BME68X_SLEEP_MODE;
static uint8_t opMode;
static constexpr uint32_t CONST_SCALE_MS_TO_US = 1000000;
static constexpr uint32_t SAVE_CYCLE_COUNT = 10000;


static void output_print(int64_t timestamp, float gas_estimate_1, float gas_estimate_2,
		float gas_estimate_3, float gas_estimate_4, float raw_pressure,
		float raw_temp, float raw_humidity, float raw_gas,
		uint8_t raw_gas_index, bsec_library_return_t bsec_status) {
	otCliOutputFormat("IAQ: %d , Status: %d , raw_temp: %d , raw_gas: %d \n",
			raw_gas_index, (uint32_t) bsec_status,
			(uint32_t) (raw_temp * 1000.0f), (uint32_t) (raw_gas * 1000.0f));
}

static uint32_t bme_get_meas_dur(uint8_t mode)
{
	if (mode == BME68X_SLEEP_MODE)
		mode = lastOpMode;

	return bme68x_get_meas_dur(mode, &bme_conf, &bme);
}


static void bme_set_forced(bsec_bme_settings_t *sensor_settings) {
	int8_t status;

	/* Set the filter, odr, temperature, pressure and humidity settings */
	status = bme68x_get_conf(&bme_conf, &bme);
	if (status != BME68X_OK)
		return;

	bme_conf.os_hum = sensor_settings->humidity_oversampling;
	bme_conf.os_temp = sensor_settings->temperature_oversampling;
	bme_conf.os_pres = sensor_settings->pressure_oversampling;
	status = bme68x_set_conf(&bme_conf, &bme);
	if (status != BME68X_OK)
		return;

	bme_heat_conf.enable = BME68X_ENABLE;
	bme_heat_conf.heatr_temp = sensor_settings->heater_temperature;
	bme_heat_conf.heatr_dur = sensor_settings->heater_duration;
	status = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &bme_heat_conf, &bme);
	if (status != BME68X_OK)
		return;

	status = bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);
	if (status != BME68X_OK)
		return;

	lastOpMode = BME68X_FORCED_MODE;
	opMode = BME68X_FORCED_MODE;
}


static void bme_set_parallel(bsec_bme_settings_t *sensor_settings)
{
    uint16_t sharedHeaterDur = 0;
	int8_t status;

	/* Set the filter, odr, temperature, pressure and humidity settings */
	status = bme68x_get_conf(&bme_conf, &bme);
	if (status != BME68X_OK)
		return;

	bme_conf.os_hum = sensor_settings->humidity_oversampling;
	bme_conf.os_temp = sensor_settings->temperature_oversampling;
	bme_conf.os_pres = sensor_settings->pressure_oversampling;
	status = bme68x_set_conf(&bme_conf, &bme);
    if (status != BME68X_OK)
		return;


    sharedHeaterDur = BSEC_TOTAL_HEAT_DUR - (bme_get_meas_dur(BME68X_PARALLEL_MODE) / INT64_C(1000));
    bme_heat_conf.enable = BME68X_ENABLE;
    bme_heat_conf.heatr_temp_prof = sensor_settings->heater_temperature_profile;
    bme_heat_conf.heatr_dur_prof = sensor_settings->heater_duration_profile;
    bme_heat_conf.shared_heatr_dur = sharedHeaterDur;
    bme_heat_conf.profile_len = sensor_settings->heater_profile_len;
    status = bme68x_set_heatr_conf(BME68X_PARALLEL_MODE, &bme_heat_conf, &bme);
    if (status != BME68X_OK)
		return;

    status = bme68x_set_op_mode(BME68X_PARALLEL_MODE, &bme);
	if (status != BME68X_OK)
		return;

	lastOpMode = BME68X_PARALLEL_MODE;
    opMode = BME68X_PARALLEL_MODE;
}

static bsec_library_return_t _bsec_get_data(bsec_input_t *bsec_inputs,
		uint8_t num_bsec_inputs) {
	/* Output buffer set to the maximum virtual sensor outputs supported */
	bsec_output_t bsec_outputs[BSEC_NUMBER_OUTPUTS];
	uint8_t num_bsec_outputs = 0;
	uint8_t index = 0;

	bsec_library_return_t bsec_status = BSEC_OK;


    int64_t timestamp = 0;
    float iaq = 0.0f;
    float stab = 0.0f;
    float runin = 0.0f;
    float temp = 0.0f;
    float hum = 0.0f;
    float pres = 0.0f;
    float gas_class1 = 0.0f;
    float gas_class2 = 0.0f;

    sig_if_t data;
	/* Check if something should be processed by BSEC */
	if (num_bsec_inputs > 0) {
		/* Set number of outputs to the size of the allocated buffer */
		/* BSEC_NUMBER_OUTPUTS to be defined */
		num_bsec_outputs = BSEC_NUMBER_OUTPUTS;

		/* Perform processing of the data by BSEC
		 Note:
		 The number of outputs you get depends on what you asked for during bsec_update_subscription(). This is
		 handled under bme68x_bsec_update_subscription() function in this example file.
		 The number of actual outputs that are returned is written to num_bsec_outputs. */
		bsec_status = bsec_do_steps(bsec_inputs, num_bsec_inputs, bsec_outputs,
				&num_bsec_outputs);

		/* Iterate through the outputs and extract the relevant ones. */
		for (index = 0; index < num_bsec_outputs; index++) {
			switch (bsec_outputs[index].sensor_id) {
			case BSEC_OUTPUT_IAQ:
				data.iaq = std::make_pair(bsec_outputs[index].signal, bsec_outputs[index].accuracy);
				break;
			case BSEC_OUTPUT_STABILIZATION_STATUS:
				data.stab = static_cast<bool>(bsec_outputs[index].signal);
				break;
			case BSEC_OUTPUT_RUN_IN_STATUS:
				data.run_in = static_cast<bool>(bsec_outputs[index].signal);
				break;
			case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
				data.comp_temp = std::make_pair(bsec_outputs[index].signal, bsec_outputs[index].accuracy);
				break;
			case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
				data.comp_hum = std::make_pair(bsec_outputs[index].signal, bsec_outputs[index].accuracy);
				break;
			case BSEC_OUTPUT_RAW_PRESSURE:
				data.pres = std::make_pair(bsec_outputs[index].signal, bsec_outputs[index].accuracy);
				break;
			case BSEC_OUTPUT_GAS_ESTIMATE_1:
				data.class1 = std::make_pair(bsec_outputs[index].signal, bsec_outputs[index].accuracy);
				break;
			case BSEC_OUTPUT_GAS_ESTIMATE_2:
				data.class2 = std::make_pair(bsec_outputs[index].signal, bsec_outputs[index].accuracy);
				break;
			default:
				continue;
			}

			/* Assume that all the returned timestamps are the same */
			data.timestamp = bsec_outputs[index].time_stamp;

		}

		coap::ux_queue.push(data);
		otCliOutputFormat("iaq: %d, stab: %d, run: %d, temp: %d, hum: %d, pres: %d, gas: %d, gas: %d", (int32_t)(data.iaq.first), (int32_t)(data.stab), (int32_t)(data.run_in), (int32_t)(data.comp_temp.first*1000), (int32_t)(data.comp_hum.first*1000), (int32_t)(data.pres.first*1000), (int32_t)(data.class1.first*1000), (int32_t)(data.class2.first*1000));
	} else otCliOutputFormat("no data");
	return bsec_status;
}

static bsec_library_return_t _proc_data(int64_t currTimeNs, struct bme68x_data data,
		int32_t bsec_process_data) {

	bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR]; /* Temp, Pres, Hum & Gas */
	bsec_library_return_t bsec_status = BSEC_OK;
	uint8_t nInputs = 0;
	/* Checks all the required sensor inputs, required for the BSEC library for the requested outputs */
	if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_HEATSOURCE)) {
		inputs[nInputs].sensor_id = BSEC_INPUT_HEATSOURCE;
		inputs[nInputs].signal = 0;
		inputs[nInputs].time_stamp = currTimeNs;
		nInputs++;
	}
	if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_TEMPERATURE)) {

		inputs[nInputs].sensor_id = BSEC_INPUT_TEMPERATURE;

		inputs[nInputs].signal = data.temperature;
		inputs[nInputs].time_stamp = currTimeNs;
		nInputs++;
	}
	if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_HUMIDITY)) {

		inputs[nInputs].sensor_id = BSEC_INPUT_HUMIDITY;

		inputs[nInputs].signal = data.humidity;
		inputs[nInputs].time_stamp = currTimeNs;
		nInputs++;
	}
	if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_PRESSURE)) {
		inputs[nInputs].sensor_id = BSEC_INPUT_PRESSURE;
		inputs[nInputs].signal = data.pressure;
		inputs[nInputs].time_stamp = currTimeNs;
		nInputs++;
	}
	if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_GASRESISTOR)
			&& (data.status & BME68X_GASM_VALID_MSK)) {
		inputs[nInputs].sensor_id = BSEC_INPUT_GASRESISTOR;
		inputs[nInputs].signal = data.gas_resistance;
		inputs[nInputs].time_stamp = currTimeNs;
		nInputs++;
	}
	if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_PROFILE_PART)
			&& (data.status & BME68X_GASM_VALID_MSK)) {
		inputs[nInputs].sensor_id = BSEC_INPUT_PROFILE_PART;
		inputs[nInputs].signal =
				(opMode == BME68X_FORCED_MODE) ? 0 : data.gas_index;
		inputs[nInputs].time_stamp = currTimeNs;
		nInputs++;
	}

	if (nInputs > 0) {
		/* Processing of the input signals and returning of output samples is performed by bsec_do_steps() */
		bsec_status = _bsec_get_data(inputs, nInputs);

		if (bsec_status != BSEC_OK)
			return bsec_status;
	}
	return BSEC_OK;
}

static uint8_t _get_data(struct bme68x_data *data) {
	if (lastOpMode == BME68X_FORCED_MODE) {
		*data = bme_data[0];
	} else {
		if (nFields) {
			/* iFields spans from 0-2 while nFields spans from
			 0-3, where 0 means that there is no new data
			 */
			*data = bme_data[iFields];
			iFields++;

			/* Limit reading continuously to the last fields read */
			if (iFields >= nFields) {
				iFields = nFields - 1;
				return 0;
			}

			/* Indicate if there is something left to read */
			return nFields - iFields;
		}
	}

	return 0;
}


bsec_library_return_t proc(void (*burtc_cb)(uint32_t)) {
	static uint32_t ctr = 0;

	bsec_library_return_t ret = BSEC_OK;

	ret = bsec_sensor_control(0, &bme_stg);
/*	otCliOutputFormat("Sensor control called with %d delay and error %d %d\n",
			(uint32_t) (bme_stg.next_call / 1000 / 1000), ret,
			sl_sleeptimer_tick_to_ms(sl_sleeptimer_get_tick_count()) * 1000);
*/
	burtc_cb((uint32_t) (bme_stg.next_call / CONST_SCALE_MS_TO_US));

	if (bme_stg.op_mode == BME68X_FORCED_MODE) {
		bme_set_forced(&bme_stg);
	} else if(bme_stg.op_mode == BME68X_PARALLEL_MODE) {
		if (opMode != bme_stg.op_mode)
		{
			bme_set_parallel(&bme_stg);
		}
	}
	else if (bme_stg.op_mode == BME68X_SLEEP_MODE) {

		if (opMode != bme_stg.op_mode) {
			int8_t tmp = bme68x_set_op_mode(BME68X_SLEEP_MODE, &bme);
			if ((tmp == BME68X_OK) && (opMode != BME68X_SLEEP_MODE)) {
				opMode = BME68X_SLEEP_MODE;
			}
		}
	}

	if (bme_stg.trigger_measurement && bme_stg.op_mode != BME68X_SLEEP_MODE) {
		nFields = 0;
		bme68x_get_data(lastOpMode, &bme_data[0], &nFields, &bme);
		iFields = 0;
		if (nFields) {
			do {
				nFieldsLeft = _get_data(&data);
				/* check for valid gas data */
				if (data.status & BME68X_GASM_VALID_MSK) {
					if ((ret = _proc_data((int64_t) (sl_sleeptimer_tick_to_ms(sl_sleeptimer_get_tick_count()) * CONST_SCALE_MS_TO_US), data,
							bme_stg.process_data)) != BSEC_OK) {
						return ret;
					}
				}
			} while (nFieldsLeft);
		}
	}


	ctr++;
	if (ctr >= SAVE_CYCLE_COUNT) {
		uint32_t len = sizeof(bsec_state);
		ret = bsec_get_state(0, bsec_state, sizeof(bsec_state),
				work_buffer, sizeof(work_buffer), &len);
		if (ret == BSEC_OK) {
			//state_save(bsec_state, bsec_state_len);
		}
		ctr = 0;
	}
	return ret;
}

}
