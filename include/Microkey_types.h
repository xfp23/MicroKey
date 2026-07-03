/**
 * @file Microkey_types.h
 * @author https://github.com/xfp23
 * @brief Micro系列 Micorkey 按键事件分发驱动模型
 * @version 0.1
 * @date 2026-07-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef MICROKEY_TYPES_H
#define MICROKEY_TYPES_H

#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    MICROKEY_OK,
    MICROKEY_PARAM_ERR,
    MICROKEY_BUSY,
} Microkey_Status_t;

typedef enum
{
    MICROKEY_TOUCH_NONE,
    MICROKEY_TOUCH_SHORT,
    MICROKEY_TOUCH_LONG,
    MICROKEY_TOUCH_CONTINUE, // 后续再说
} Microkey_TouchSta_t;       // 触发类型

typedef enum
{
    MICROKEY_EVENT_IDLE,
    MICROKEY_EVENT_TRIGGER, // 触发
    MICROKEY_EVENT_RELEASE, // 释放
    MICROKEY_EVENT_CATCH,   // 等待获取值
    MICROKEY_EVENT_CLEAR,
} Microkey_Event_t;

/**
 * @brief 回调函数返回
 *
 */
// 按键扫描频率
// 可以是IO,也可以是中断
typedef struct
{
    // uint32_t scan_freq_hz;
    uint32_t deboun_tick;
    uint32_t hold_short_tick; // 短按保持滴答
    uint32_t hold_long_tick;  // 长按保持滴答
} Microkey_Conf_t;

typedef struct
{
    volatile uint32_t tick;
    // volatile float ms; // 最多也就调试用，为了精度，我不使用时间，而是直接使用定时的滴答心跳
    Microkey_Conf_t conf;
    volatile Microkey_TouchSta_t touch_type;
    // volatile bool triggered;
    volatile Microkey_Event_t event;
    volatile uint32_t deboun_count;
} Microkey_obj_t;

#ifdef __cplusplus
}
#endif

#endif
