/**
 * @file fuel_gauge.h
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
extern lv_obj_t * screen_gauges;
extern int32_t Fuel_L_value;
extern int32_t Fuel_R_value;


/**********************
 * GLOBAL PROTOTYPES
 **********************/
void fuel_gaugeL(int gaugeL_timer_value);
void fuel_gaugeR(int gaugeR_timer_value);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

