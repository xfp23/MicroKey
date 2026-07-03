[EN](./readme.md)

# Microkey — 裸机按键驱动

一个轻量级、基于事件驱动的按键去抖 + 短按/长按检测驱动，适用于裸机 MCU。
不依赖动态内存、不依赖操作系统。设计上由"周期性 GPIO 扫描（主循环或低优先级任务）
+ 1ms 定时器 tick（主循环或定时器中断）"两部分共同驱动。

## 特性

- 基于可配置采样阈值（`deboun_tick`）的去抖过滤
- 短按 / 长按分类（依据 `hold_short_tick`）
- 简洁的 5 状态状态机，对重复/冗余调用安全
  （返回 `MICROKEY_BUSY` 而不是破坏内部状态）
- 一个按键对应一个 `Microkey_obj_t` 实例，扩展到 N 个按键只需要一个对象数组

## 文件结构

| 文件 | 作用 |
|---|---|
| `Microkey_types.h` | 类型、枚举、配置结构体、对象结构体 |
| `Microkey.h` | 对外 API 声明 |
| `Microkey.c` | 对外 API 实现 |
| `example.c` | 最小可用示例（单个 GPIO 按键） |

## 数据流 / 状态机

```
IDLE --(Microkey_Set() 被连续调用 deboun_tick 次)--> TRIGGER
TRIGGER --(Microkey_Timerhandler()，每个 tick)--> tick++
TRIGGER --(调用 Microkey_Clear())--> RELEASE
RELEASE --(Microkey_Timerhandler()，一次性判定)--> 判定 SHORT/LONG --> CATCH
CATCH --(调用 Microkey_GetKeyTouchSta())--> CLEAR，返回 touch_type
CLEAR --(Microkey_Timerhandler())--> tick/touch_type 清零 --> IDLE
```

## 典型集成方式

```c
// GPIO 轮询上下文（主循环，采样周期建议 <= 1ms）
if (read_gpio_pressed())
{
    Microkey_Set(&key1);
}
else
{
    Microkey_Clear(&key1);
}

// 定时器中断或 1ms tick 来源
Microkey_Timerhandler(&key1);

// 在任意位置检查是否有已完成的按键事件
Microkey_TouchSta_t sta = Microkey_GetKeyTouchSta(&key1);
if (sta == MICROKEY_TOUCH_SHORT)
{
    // 处理短按
}
else if (sta == MICROKEY_TOUCH_LONG)
{
    // 处理长按
}
```

完整可编译风格的示例见 `example.c`。

## API 参考

### `Microkey_Status_t Microkey_Init(Microkey_obj_t *obj, Microkey_Conf_t *conf)`

将 `obj` 清零，并把 `*conf` 拷贝进去。

- **参数**
  - `obj` — 待初始化的按键对象指针
  - `conf` — 已填好的配置（`deboun_tick`、`hold_short_tick`、`hold_long_tick`）
- **返回值** 成功返回 `MICROKEY_OK`；`obj`/`conf` 为 NULL 时返回 `MICROKEY_PARAM_ERR`
- **注意事项**
  - 每个按键对象在使用其它 API 之前，必须先调用一次本函数完成初始化。
  - 依赖 `MICROKEY_EVENT_IDLE == 0` 和 `MICROKEY_TOUCH_NONE == 0`
    （由 `Microkey_types.h` 当前的枚举声明顺序保证），请勿调整这两个枚举的声明顺序。

### `Microkey_Status_t Microkey_Set(Microkey_obj_t *obj)`

在原始 GPIO 输入表示"按下"时，每个扫描周期调用一次。内部去抖计数递增，
达到 `conf.deboun_tick` 后按键状态从 `IDLE` 转为 `TRIGGER`。

- **返回值**
  - 正常调用返回 `MICROKEY_OK`
  - 按键当前不处于 `IDLE`（正在按下过程中或事件尚未被消费）时返回 `MICROKEY_BUSY`
  - `obj` 为 NULL 时返回 `MICROKEY_PARAM_ERR`

### `Microkey_Status_t Microkey_Clear(Microkey_obj_t *obj)`

在原始 GPIO 输入表示"释放"时，每个扫描周期调用一次。

- 若按键处于 `IDLE`，清零去抖计数（取消一次尚未完成的去抖过程），返回 `MICROKEY_OK`。
- 若按键处于 `TRIGGER`，转为 `RELEASE`（在下一次定时器 tick 中开始短/长按判定）。
- 其余状态下返回 `MICROKEY_BUSY`。
- **返回值** `obj` 为 NULL 时返回 `MICROKEY_PARAM_ERR`。

### `Microkey_Status_t Microkey_Timerhandler(Microkey_obj_t *obj)`

必须以固定周期（驱动内部以 1ms 为假设单位，`hold_short_tick`/`hold_long_tick`
均以 tick 数表示）被周期性调用，驱动内部计时与判定逻辑。

- **返回值** 正常返回 `MICROKEY_OK`；`obj` 为 NULL 时返回 `MICROKEY_PARAM_ERR`

### `Microkey_TouchSta_t Microkey_GetKeyTouchSta(Microkey_obj_t *obj)`

读取并消费一次完整按下/释放周期的判定结果。

- **返回值**
  - 若结果已就绪（按键处于 `CATCH` 状态），返回 `MICROKEY_TOUCH_SHORT` 或
    `MICROKEY_TOUCH_LONG`；消费后按键状态会流转到 `CLEAR -> IDLE`
  - 若结果尚未就绪，或 `obj` 为 NULL，返回 `MICROKEY_TOUCH_NONE`
- **重要**：每次按键完成后，**必须**最终调用一次本函数，否则该按键对象会
  永久停留在 `CATCH` 状态，此后 `Microkey_Set()` 会一直返回 `MICROKEY_BUSY`
  （驱动内部没有超时保护机制）。

## 类型定义

```c
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
    MICROKEY_TOUCH_CONTINUE, // 预留，尚未实现
} Microkey_TouchSta_t;

typedef struct
{
    uint32_t deboun_tick;     // 确认一次按下所需的连续 Set() 调用次数
    uint32_t hold_short_tick; // 判定为“长按”的 tick 阈值
    uint32_t hold_long_tick;  // 预留字段，暂未使用
} Microkey_Conf_t;
```

## 已知限制

- `hold_long_tick` 和 `MICROKEY_TOUCH_CONTINUE` 是为未来"长按自动连发"功能
  预留的字段，目前不产生任何效果。
- 若按键完成后从未调用 `Microkey_GetKeyTouchSta()`，驱动没有超时自恢复机制；
  调用方需要自行保证每次事件都被及时消费。
- 释放沿的去抖与按下沿并不完全对称；实际中大部分场景会被状态机
  "同一时刻只有一个有效状态转换"的特性掩盖，但对特别抖动剧烈的释放沿
  没有独立的去抖过滤。

## 许可证

请在此处填写你选择的开源许可证。