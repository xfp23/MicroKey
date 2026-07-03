[中文](./readme.zh.md)

# Microkey — Bare-Metal Button Driver

A lightweight, event-driven button (key) debounce and short/long-press
detection driver for bare-metal MCUs. No dynamic memory, no OS dependency.
Designed to be driven from a periodic GPIO scan (main loop or low-priority
task) plus a 1ms timer tick (main loop or timer interrupt).

## Features

- Debounce filtering with a configurable sample threshold (`deboun_tick`)
- Short-press / long-press classification (`hold_short_tick`)
- Simple 5-state state machine, safe against redundant/duplicate calls
  (returns `MICROKEY_BUSY` instead of corrupting state)
- One button = one `Microkey_obj_t` instance, so it scales to N buttons
  trivially (array of objects)

## File Layout

| File | Purpose |
|---|---|
| `Microkey_types.h` | Types, enums, config struct, object struct |
| `Microkey.h` | Public API declarations |
| `Microkey.c` | Public API implementation |
| `example.c` | Minimal usage example (single button on a GPIO) |

## Data Flow / State Machine

```
IDLE --(Microkey_Set() called deboun_tick times)--> TRIGGER
TRIGGER --(Microkey_Timerhandler(), every tick)--> tick++
TRIGGER --(Microkey_Clear() called)--> RELEASE
RELEASE --(Microkey_Timerhandler(), one-shot)--> classify SHORT/LONG --> CATCH
CATCH --(Microkey_GetKeyTouchSta() called)--> CLEAR, returns touch_type
CLEAR --(Microkey_Timerhandler())--> tick/touch_type reset --> IDLE
```

## Typical Integration

```c
// GPIO polling context (main loop, ~1ms or faster)
if (read_gpio_pressed())
{
    Microkey_Set(&key1);
}
else
{
    Microkey_Clear(&key1);
}

// Timer ISR or 1ms tick source
Microkey_Timerhandler(&key1);

// Anywhere, whenever you want to check for a completed press event
Microkey_TouchSta_t sta = Microkey_GetKeyTouchSta(&key1);
if (sta == MICROKEY_TOUCH_SHORT)
{
    // handle short press
}
else if (sta == MICROKEY_TOUCH_LONG)
{
    // handle long press
}
```

See `example.c` for a complete, compilable-style example.

## API Reference

### `Microkey_Status_t Microkey_Init(Microkey_obj_t *obj, Microkey_Conf_t *conf)`

Zero-initializes `obj` and copies `*conf` into it.

- **Params**
  - `obj` — pointer to the button object to initialize
  - `conf` — pointer to a filled-in configuration (`deboun_tick`,
    `hold_short_tick`, `hold_long_tick`)
- **Returns** `MICROKEY_OK`, or `MICROKEY_PARAM_ERR` if `obj`/`conf` is NULL
- **Notes**
  - Must be called once, before any other API, for each button object.
  - Relies on `MICROKEY_EVENT_IDLE == 0` and `MICROKEY_TOUCH_NONE == 0`
    (guaranteed by the current enum declaration order in
    `Microkey_types.h`); do not reorder those enums.

### `Microkey_Status_t Microkey_Set(Microkey_obj_t *obj)`

Call once per scan cycle while the raw GPIO input indicates "pressed".
Increments an internal debounce counter; once it reaches `conf.deboun_tick`
the button transitions `IDLE -> TRIGGER`.

- **Returns**
  - `MICROKEY_OK` on a normal call
  - `MICROKEY_BUSY` if the button is not currently `IDLE` (i.e. a press is
    already in progress or pending consumption)
  - `MICROKEY_PARAM_ERR` if `obj` is NULL

### `Microkey_Status_t Microkey_Clear(Microkey_obj_t *obj)`

Call once per scan cycle while the raw GPIO input indicates "released".

- If the button is `IDLE`, resets the debounce counter (cancels an
  in-progress debounce sequence) and returns `MICROKEY_OK`.
- If the button is `TRIGGER`, transitions it to `RELEASE` (starts the
  short/long classification on the next timer tick).
- Otherwise returns `MICROKEY_BUSY`.
- **Returns** `MICROKEY_PARAM_ERR` if `obj` is NULL.

### `Microkey_Status_t Microkey_Timerhandler(Microkey_obj_t *obj)`

Must be called periodically at a fixed tick rate (1ms is the assumed unit
throughout the driver, based on `hold_short_tick`/`hold_long_tick` being
expressed in ticks). Drives the internal timing/classification logic.

- **Returns** `MICROKEY_OK`, or `MICROKEY_PARAM_ERR` if `obj` is NULL

### `Microkey_TouchSta_t Microkey_GetKeyTouchSta(Microkey_obj_t *obj)`

Reads and consumes the classification result of a completed press/release
cycle.

- **Returns**
  - `MICROKEY_TOUCH_SHORT` or `MICROKEY_TOUCH_LONG` if a result is ready
    (button was in `CATCH` state); consuming it moves the button toward
    `CLEAR -> IDLE`
  - `MICROKEY_TOUCH_NONE` if no result is ready yet, or if `obj` is NULL
- **Important**: this function *must* eventually be called after every
  completed press, or the button object will remain latched in `CATCH`
  and `Microkey_Set()` will keep returning `MICROKEY_BUSY` forever for
  that button (no built-in timeout).

## Types

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
    MICROKEY_TOUCH_CONTINUE, // reserved, not implemented yet
} Microkey_TouchSta_t;

typedef struct
{
    uint32_t deboun_tick;     // number of consecutive Set() calls to confirm a press
    uint32_t hold_short_tick; // tick threshold above which a press is "long"
    uint32_t hold_long_tick;  // reserved, not used yet
} Microkey_Conf_t;
```

## Known Limitations

- `hold_long_tick` and `MICROKEY_TOUCH_CONTINUE` are reserved fields for a
  future "long-press auto-repeat" feature and currently have no effect.
- There is no timeout/self-recovery if `Microkey_GetKeyTouchSta()` is never
  called after a press completes; the caller is responsible for always
  draining a completed event.
- Release-edge debouncing is not symmetric with press-edge debouncing;
  in practice this is largely masked by the state machine's "one active
  transition at a time" design, but very noisy release edges are not
  independently filtered.

## License

Add your license of choice here.