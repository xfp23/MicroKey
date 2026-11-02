/**
 * @file Microkey.c
 * @author https://github.com/xfp23
 * @brief Micro系列 Microkey 按键事件分发驱动模型
 * @version 0.2
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
        if (!(x))                      \
        {                              \
            return MICROKEY_PARAM_ERR; \
        }                              \
    } while (0)

/* ms -> Tickhandler 计数值, 向上取整 */
static uint32_t Microkey_MsToCnt(uint32_t ms, uint32_t freq_hz)
{
    return (uint32_t)(((uint64_t)ms * freq_hz + 999U) / 1000U);
}

Microkey_Status_t Microkey_Init(Microkey_obj_t *obj, Microkey_Conf_t *conf)
{
    CHECK_PARAM(obj);
    CHECK_PARAM(conf);

    if (conf->freq_hz == 0 || conf->freq_hz > 1000)
    {
        return MICROKEY_PARAM_ERR;
    }

    memset(obj, 0, sizeof(Microkey_obj_t));
    obj->conf = *(conf);

    obj->deboun_cnt     = Microkey_MsToCnt(conf->deboun_tick, conf->freq_hz);
    obj->hold_short_cnt = Microkey_MsToCnt(conf->hold_short_tick, conf->freq_hz);
    obj->hold_long_cnt  = Microkey_MsToCnt(conf->hold_long_tick, conf->freq_hz);

    return MICROKEY_OK;
}

Microkey_Status_t Microkey_Set(Microkey_obj_t *obj)
{
    CHECK_PARAM(obj);

    obj->level = true;

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

    obj->level = false;

    return MICROKEY_OK;
}

/**
 * @brief 计时驱动 (调度频率 = conf.freq_hz)
 *        只负责计数: 消抖状态累加 debounce, 按住状态累加 tick
 *        不做任何状态切换和回调
 *
 * @param obj
 * @return Microkey_Status_t
 */
void Microkey_Tickhandler(Microkey_obj_t *obj)
{
    CHECK_PARAM(obj);

    switch (obj->state)
    {
    case MICROKEY_STATE_DEBOUNCE_ON:
    case MICROKEY_STATE_DEBOUNCE_OFF:
        if (obj->debounce < UINT32_MAX)
        {
            obj->debounce++;
        }
        break;

    case MICROKEY_STATE_PRESSED:
    case MICROKEY_STATE_LONG:
        if (obj->tick < UINT32_MAX)
        {
            obj->tick++;
        }
        break;

    case MICROKEY_STATE_IDLE:
    default:
        break;
    }

//    return MICROKEY_OK;
}

/**
 * @brief 扫描驱动: 根据电平和 Tickhandler 的计数值切换状态, 自身不计时
 *        调用周期不限, 越快响应越及时
 *
 * @param obj
 * @return Microkey_Status_t
 */
Microkey_Status_t Microkey_Timerhandler(Microkey_obj_t *obj)
{
    CHECK_PARAM(obj);

    switch (obj->state)
    {
    case MICROKEY_STATE_IDLE:
        if (obj->level)
        {
            obj->debounce = 0;
            obj->tick = 0;
            obj->long_trigged = false;
            obj->state = MICROKEY_STATE_DEBOUNCE_ON;
        }
        break;

    case MICROKEY_STATE_DEBOUNCE_ON: // 按下消抖
        if (!obj->level)
        {
            obj->state = MICROKEY_STATE_IDLE;
            break;
        }
        if (obj->debounce >= obj->deboun_cnt) // 消抖完成, 从此刻开始计按住时间
        {
            obj->tick = 0;
            obj->state = MICROKEY_STATE_PRESSED;
        }
        break;

    case MICROKEY_STATE_PRESSED: // 按住中
        if (!obj->level)
        {
            obj->debounce = 0;
            obj->state = MICROKEY_STATE_DEBOUNCE_OFF;
            break;
        }

        if (obj->tick >= obj->hold_long_cnt) // 到达长按阈值, 无需松开
        {
            obj->state = MICROKEY_STATE_LONG;
            obj->long_trigged = true;
            obj->event_hold_tick = 0;

            if (obj->conf.long_callback) // 边沿触发
            {
                obj->conf.long_callback();
            }
        }
        break;

    case MICROKEY_STATE_LONG: // 长按保持中, 等待松开
        if (!obj->level)
        {
            obj->debounce = 0;
            obj->state = MICROKEY_STATE_DEBOUNCE_OFF;
        }
        break;

    case MICROKEY_STATE_DEBOUNCE_OFF: // 松开消抖
        if (obj->level)               // 消抖期间又被按下, 回到原状态继续
        {
            obj->state = obj->long_trigged ? MICROKEY_STATE_LONG
                                           : MICROKEY_STATE_PRESSED;
            break;
        }

        if (obj->debounce >= obj->deboun_cnt) // 松开确认
        {
            // 按住时间在 [short, long) 之间松开才算短按
            if (!obj->long_trigged &&
                obj->tick >= obj->hold_short_cnt &&
                obj->tick < obj->hold_long_cnt)
            {
                obj->short_event = true;

                if (obj->conf.short_callback)
                {
                    obj->conf.short_callback(); // 边沿触发回调
                }
            }

            obj->state = MICROKEY_STATE_IDLE;
        }
        break;

    default:
        obj->state = MICROKEY_STATE_IDLE;
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

    if (obj->state == MICROKEY_STATE_LONG)
    {
        return MICROKEY_TOUCH_LONG;
    }

    if (obj->short_event)
    {
        obj->short_event = false;
        return MICROKEY_TOUCH_SHORT;
    }

    return MICROKEY_TOUCH_NONE;
}

/**
 * @brief 重置按键所有状态
 *
 * @param obj
 */
void Microkey_Reset(Microkey_obj_t *obj)
{
    if (!obj)
    {
        return;
    }

    obj->state = MICROKEY_STATE_IDLE; // 复位状态机
    obj->short_event = false;
    obj->long_event = false;
    obj->long_trigged = false;
    obj->short_trigged = false;
    obj->tick = 0;
    obj->debounce = 0;
    obj->event_hold_tick = 0;
}

bool Microkey_IsBusy(Microkey_obj_t *obj)
{
    if (!obj)
    {
        return true;
    }

    if (obj->state != MICROKEY_STATE_IDLE || obj->long_event || obj->short_event)
    {
        return true;
    }

    return false;
}
