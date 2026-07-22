#ifndef MICROKEY_H
#define MICROKEY_H

/**
 * @file Microkey.h
 * @author https://xfp23.github.io
 * @brief Micro series Microkey button event dispatch driver model
 * @version 0.1
 * @date 2026-07-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "Microkey_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Initialize a button object
 * 
 * @param obj Button object
 * @param conf Button configuration
 * @return Microkey_Status_t Operation status
 */
extern Microkey_Status_t Microkey_Init(Microkey_obj_t *obj, Microkey_Conf_t *conf);

/**
 * @brief Feed a "pressed" sample into the button state machine.
 *        Call this every scan cycle while the GPIO reads "pressed".
 * 
 * @param obj Button object
 * @return Microkey_Status_t Operation status
 */
extern Microkey_Status_t Microkey_Set(Microkey_obj_t *obj);

/**
 * @brief Feed a "released" sample into the button state machine.
 *        Call this every scan cycle while the GPIO reads "released".
 * 
 * @param obj Button object
 * @return Microkey_Status_t Operation status
 */
extern Microkey_Status_t Microkey_Clear(Microkey_obj_t *obj);

/**
 * @brief Timer-driven tick handler, must be called periodically (e.g. every 1ms)
 * 
 * @param obj Button object
 * @return Microkey_Status_t Operation status
 */
extern Microkey_Status_t Microkey_Timerhandler(Microkey_obj_t *obj);

/**
 * @brief Get and consume the latched touch result (short/long press).
 *        Calling this resets the button back toward IDLE.
 * 
 * @param obj Button object
 * @return Microkey_TouchSta_t Touch type result
 */
extern Microkey_TouchSta_t Microkey_GetKeyTouchSta(Microkey_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif