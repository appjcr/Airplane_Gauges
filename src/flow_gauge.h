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
extern int32_t flow_value;
extern int32_t remail_value;
extern int32_t flow_used_value;
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

