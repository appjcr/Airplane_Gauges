/**
 * @file flow_gauge.h
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
extern float flow_value;
extern float remail_value;
extern float flow_used_value;
extern int32_t time_to_empty_value;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void flow_gauge(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

