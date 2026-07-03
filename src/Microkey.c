/**
 * @file Microkey.c
 * @author https://github.com/xfp23
 * @brief Micro系列 Micorkey 按键事件分发驱动模型
 * @version 0.1
 * @date 2026-07-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "Microkey.h"
#include "string.h"
#include "stdlib.h"


#define CHECK_PARAM(x)                 \
    do                                 \
    {                                  \
        if (!x)                        \
        {                              \
            return MICROKEY_PARAM_ERR; \
        }                              \
    } while (0)

Microkey_Status_t Microkey_Init(Microkey_obj_t *obj, Microkey_Conf_t *conf)
{
    CHECK_PARAM(obj);
    CHECK_PARAM(conf);

    memset(obj, 0, sizeof(Microkey_obj_t));
    obj->conf = *(conf);

    return MICROKEY_OK;
}

Microkey_Status_t Microkey_Set(Microkey_obj_t *obj)
{
    CHECK_PARAM(obj);

    if (obj->event != MICROKEY_EVENT_IDLE)
    {
        return MICROKEY_BUSY;
    }

    if (++obj->deboun_count >= obj->conf.deboun_tick)
    {
        obj->event = MICROKEY_EVENT_TRIGGER; // 第一次触发
        obj->deboun_count = 0;
    }

    return MICROKEY_OK;
}

/**
 * @brief 按键清除
 *
 * @param obj 按键对象
 * @return Microkey_Status_t 操作状态
 */
Microkey_Status_t Microkey_Clear(Microkey_obj_t *obj)
{
    CHECK_PARAM(obj);

    if (obj->event == MICROKEY_EVENT_IDLE)
    {
        obj->deboun_count = 0;
        return MICROKEY_OK;
    }

    if (obj->event != MICROKEY_EVENT_TRIGGER)
    {
        return MICROKEY_BUSY;
    }

    obj->event = MICROKEY_EVENT_RELEASE;
    return MICROKEY_OK;
}

/**
 * @brief 定时驱动 1ms
 *
 * @param obj
 * @return Microkey_Status_t
 */
Microkey_Status_t Microkey_Timerhandler(Microkey_obj_t *obj)
{
    CHECK_PARAM(obj);

    switch (obj->event)
    {
    case MICROKEY_EVENT_TRIGGER:
        obj->tick++;
        break;
    case MICROKEY_EVENT_RELEASE: // 释放开始掐表
        obj->touch_type = MICROKEY_TOUCH_SHORT;

        if (obj->tick > obj->conf.hold_short_tick)
        {
            obj->touch_type = MICROKEY_TOUCH_LONG;
        }

        obj->event = MICROKEY_EVENT_CATCH; // 清除
        break;

    case MICROKEY_EVENT_CLEAR:
        obj->tick = 0;
        obj->touch_type = MICROKEY_TOUCH_NONE;
        obj->event = MICROKEY_EVENT_IDLE;
        break;

    case MICROKEY_EVENT_IDLE:
        // 空闲
        break;
    default:
        break;
    }

    return MICROKEY_OK;
}

Microkey_TouchSta_t Microkey_GetKeyTouchSta(Microkey_obj_t *obj)
{
    if (obj == NULL)
    {
        return MICROKEY_TOUCH_NONE;
    }

    if (obj->event != MICROKEY_EVENT_CATCH)
    {
        return MICROKEY_TOUCH_NONE;
    }

    obj->event = MICROKEY_EVENT_CLEAR;

    return obj->touch_type;
}
