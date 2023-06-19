#ifndef APP_SENSOR_H
#define APP_SENSOR_H




namespace sensor {
// Namespace for wrapping BSEC, bme68x and platform abstractions together

typedef struct
{

} output; // TODO output data form



/**
 * @brief Performs bsec_sensor_control, bsec_do_steps() etc. To be called on BURTC trigger.
 *
 * @param burtc_cb  Const fp to BURTC setting function, for (uint32_t) ms
 *
 * @param ts        Const fp to get system timestamp in ms
 *
 * @return          Returns a value of type bsec_library_return_t (enum)
 */
bsec_library_return_t proc(void (*const burtc_cb)(uint32_t), uint32_t (*const ts)(void));
output get(void);

}


#endif // APP_SENSOR_H
