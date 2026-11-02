/**
 * @file Microkey_types.h
 * @author https://github.com/xfp23
 * @brief Micro系列 Microkey 按键事件分发驱动模型
 * @version 0.2
 * @date 2026-07-03
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

typedef enum
{
    MICROKEY_STATE_IDLE,
    MICROKEY_STATE_DEBOUNCE_ON,
    MICROKEY_STATE_PRESSED,
    MICROKEY_STATE_LONG,
    MICROKEY_STATE_DEBOUNCE_OFF,
} MicroKeyState_t;

/**
 * @brief 回调函数类型
 */
typedef void (*Microkey_Callback_t)(void);

/**
 * @brief 按键配置
 * @note 所有时间都由 Microkey_Tickhandler 计时, 与 Microkey_Timerhandler 的调用周期无关
 */
typedef struct
{
    uint32_t freq_hz;         // Microkey_Tickhandler 的调度频率 (Hz), 范围 1~1000
    uint32_t deboun_tick;     // 消抖时间 (ms), 按下和松开共用
    uint32_t hold_short_tick; // 短按阈值 (ms): 按住 >= 该值, 松开后算短按
    uint32_t hold_long_tick;  // 长按阈值 (ms): 按住 >= 该值, 无需松开直接触发长按
    Microkey_Callback_t short_callback;
    Microkey_Callback_t long_callback;

} Microkey_Conf_t;

typedef struct
{
    Microkey_Conf_t conf;

    volatile MicroKeyState_t state;

    volatile uint32_t tick;     // 按住计时 (Tickhandler 计数)
    volatile uint32_t debounce; // 消抖计时 (Tickhandler 计数)

    uint32_t deboun_cnt;        // Init 时由 ms 换算出的 tick 数
    uint32_t hold_short_cnt;
    uint32_t hold_long_cnt;

    volatile bool level;        // 当前IO状态(Set/Clear修改)

    volatile bool short_event;
    volatile bool long_event;

    volatile uint32_t event_hold_tick;

    volatile bool long_trigged;
    volatile bool short_trigged;

} Microkey_obj_t;

#ifdef __cplusplus
}
#endif

#endif