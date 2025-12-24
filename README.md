# STM32 Toaster Oven Controller (Bare-Metal C + FSM + Interrupts)

A simulated toaster-oven controller implemented on the **STM32F411RE Nucleo** in **C**. The firmware uses an **event-driven superloop** with a centralized **finite state machine** to manage oven modes (BAKE/TOAST/BROIL), knob-based setpoint changes via **ADC**, and live status output on an **OLED**. Input is handled with **timer interrupts** (GPIO button events + tick timing) and an **ADC interrupt** with windowing to reduce jitter.

## Features
- **Oven modes:** BAKE / TOAST / BROIL
- **Centralized FSM:** `SETUP -> SELECTOR_CHANGE_PENDING -> COOKING -> RESET_PENDING`
- **Event-driven superloop:** main loop runs `runOvenSM()` only when event flags are set (ADC update, button event, or timer tick)
- **Interrupt-driven I/O:**
  - **TIM3:** debounced button-event capture (`ButtonsCheckEvents()`)
  - **TIM4:** free-running tick counter + periodic tick flag
  - **ADC IRQ:** latches ADC samples and triggers stable updates
- **ADC smoothing/windowing:** 20-sample moving average + deadband threshold to avoid noisy knob updates
- **OLED UI:** live mode, time, temperature, and selector arrows (time vs temp)
- **LED progress bar:** updates during COOKING to show remaining time

## Controls (Nucleo I/O Shield Buttons)
- **Button 3 (short press):** cycle modes (BAKE -> TOAST -> BROIL -> BAKE)
- **Button 3 (long press):** toggle selector (time vs temperature) in BAKE mode
- **Button 4 (press in SETUP):** start cooking
- **Button 4 (press during COOKING):** enter reset-pending; short release resumes, long release resets to initial setpoints

> Note: Exact button mapping depends on the course shield/button library events (BUTTON_EVENT_3DOWN/UP, BUTTON_EVENT_4DOWN/UP).

## Architecture Overview
### 1) Event-driven Superloop
The main loop polls lightweight flags and only calls the state machine when something happened:
- `hasNewADC` (stable knob update available)
- `buttonsEvents` (debounced press/release event captured)
- `TimerTick` (periodic tick from TIM4)

This keeps the control logic centralized and avoids doing heavy work inside interrupts.

### 2) Finite State Machine (FSM)
`runOvenSM()` owns the full behavior:
- **SETUP:** user sets time/temperature (ADC) and selects mode/selector (buttons)
- **SELECTOR_CHANGE_PENDING:** measures press duration to decide short vs long press behavior
- **COOKING:** decrements time, updates LEDs, refreshes OLED
- **RESET_PENDING:** short/long press on Button 4 decides resume vs reset

### 3) Interrupts (Lightweight ISRs)
- **TIM4 ISR:** increments a free-running tick counter and sets `TimerTick`
- **TIM3 ISR:** captures debounced button events into `buttonsEvents`
- **ADC IRQ:** stores ADC samples into a circular window buffer and sets `hasNewADC` only when a stable change is detected

### 4) ADC Windowing / Jitter Reduction
The ADC interrupt keeps a 20-sample window and computes the average. The firmware only publishes a new value if the average changes by at least a small threshold (deadband), preventing jitter when the knob is barely touched.

## Build / Run
This project is designed for the STM32F411RE Nucleo with course-provided libraries (Buttons, Timers, Oled, Adc).
Typical workflow:
1. Open the project in your STM32 toolchain/IDE (e.g., STM32CubeIDE if applicable for your setup).
2. Build and flash to the Nucleo.
3. Use the knob (ADC) and buttons to set mode/time/temp; OLED shows live status.

## Files
- `main.c` (or equivalent): FSM, OLED rendering, interrupt handlers, superloop
- Support libraries: `Buttons.h`, `Timers.h`, `Oled.h`, `Adc.h`, `Leds.h` (course/board-specific)
