#ifndef APP_SENSOR_H
#define APP_SENSOR_H


#include <iostream>
#include <utility>
#include "bsec_interface.h"

namespace sensor {
// Namespace for wrapping BSEC, bme68x and platform abstractions together

typedef std::pair<float, uint8_t> op_data_t;
typedef struct
{
	op_data_t iaq;
	op_data_t comp_temp;
	op_data_t comp_hum;
	op_data_t pres;
	op_data_t class1;
	op_data_t class2;
	bool stab;
	bool run_in;
	int64_t timestamp;
} sig_if_t; // TODO output data form




/**
 * @brief Performs bsec_sensor_control, bsec_do_steps() etc. To be called on BURTC trigger.
 *
 * @param burtc_cb  Const fp to BURTC setting function, for (uint32_t) ms
 *
 * @return          Returns a value of type bsec_library_return_t (enum)
 */
bsec_library_return_t proc(void (*const burtc_cb)(uint32_t));


}


#endif // APP_SENSOR_H
