/**
 * @file example.c
 * @brief Minimal usage example for the Microkey button driver.
 *
 * This example assumes:
 *  - A GPIO read function `BSP_GPIO_ReadKey()` that returns 1 when the
 *    button is physically pressed, 0 otherwise.
 *  - A 1ms system tick source (e.g. SysTick_Handler on Cortex-M) that
 *    calls `Microkey_Timerhandler()`.
 *  - A main loop that polls the GPIO frequently (ideally every 1ms, or
 *    at least fast enough relative to `deboun_tick`).
 */

#include "Microkey.h"

/* ---- Replace with your actual BSP/HAL GPIO read ---- */
extern int BSP_GPIO_ReadKey(void);   /* returns 1 = pressed, 0 = released */
extern void BSP_Delay_ms(uint32_t ms);

static Microkey_obj_t s_key1;

static void Key1_ShortPressHandler(void)
{
    /* TODO: your short-press action */
}

static void Key1_LongPressHandler(void)
{
    /* TODO: your long-press action */
}

/**
 * @brief Call this once at startup.
 */
void App_Key_Init(void)
{
    Microkey_Conf_t conf = {
        .deboun_tick = 20,      /* 20 consecutive "pressed" samples to confirm a press */
        .hold_short_tick = 800, /* > 800 ticks (ms) held => classified as long press */
        .hold_long_tick = 0,    /* reserved, not used yet */
    };

    Microkey_Init(&s_key1, &conf);
}

/**
 * @brief Call this from a 1ms timer interrupt (e.g. SysTick_Handler).
 */
void App_Key_1ms_Tick(void)
{
    Microkey_Timerhandler(&s_key1);
}

/**
 * @brief Call this frequently from the main loop (GPIO scan).
 */
void App_Key_Scan(void)
{
    if (BSP_GPIO_ReadKey())
    {
        Microkey_Set(&s_key1);
    }
    else
    {
        Microkey_Clear(&s_key1);
    }
}

/**
 * @brief Call this from the main loop to drain completed key events.
 */
void App_Key_Process(void)
{
    Microkey_TouchSta_t sta = Microkey_GetKeyTouchSta(&s_key1);

    switch (sta)
    {
    case MICROKEY_TOUCH_SHORT:
        Key1_ShortPressHandler();
        break;

    case MICROKEY_TOUCH_LONG:
        Key1_LongPressHandler();
        break;

    case MICROKEY_TOUCH_NONE:
    default:
        /* nothing ready yet */
        break;
    }
}

int main(void)
{
    App_Key_Init();

    /* In a real project, App_Key_1ms_Tick() would be called from a
     * hardware timer interrupt configured for a 1ms period instead
     * of being simulated here. */

    for (;;)
    {
        App_Key_Scan();
        App_Key_Process();

        BSP_Delay_ms(1);
        App_Key_1ms_Tick();
    }
}