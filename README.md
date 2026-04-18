# FreeRTOS Digital Clock

An ESP32-based digital clock built with FreeRTOS for an Operating Systems final project. Time is synced automatically over Wi-Fi using NTP, with push buttons for manual adjustment as a fallback.

## Hardware

| Component | Part | Notes |
|---|---|---|
| Microcontroller | ESP32 DevKit | |
| Display | TM1637 4-digit 7-segment | CLK → GPIO 22, DIO → GPIO 23 |
| Hour button | Momentary push button | GPIO 4 → GND |
| Minute button | Momentary push button | GPIO 5 → GND |
| Buzzer | Passive buzzer (optional) | GPIO 21 |

All GPIO assignments can be changed in `include/config.h`.

## Wiring Diagram

```
ESP32 DevKit
                    ┌─────────────┐
              3.3V ─┤             ├─ GPIO 22 ──── TM1637 CLK
               GND ─┤             ├─ GPIO 23 ──── TM1637 DIO
                    │             ├─ GPIO 18 ──┬─ Hour Button ── GND
                    │             ├─ GPIO 19 ──┴─ Min Button  ── GND
                    │             ├─ GPIO 21 ──── Buzzer (+)  ── GND
                    └─────────────┘

TM1637 module: VCC → 3.3V, GND → GND
```

## Software Architecture

The firmware uses four FreeRTOS tasks:

| Task | Priority | Responsibility |
|---|---|---|
| `taskTimekeeping` | 4 (highest) | Increments time every second using `vTaskDelayUntil()`, pushes to display queue |
| `taskNTPSync` | 3 | Connects to Wi-Fi on boot, syncs time from NTP, re-syncs hourly |
| `taskButtonInput` | 2 | Debounced button polling, adjusts time via mutex |
| `taskDisplay` | 1 (lowest) | Reads from display queue, updates TM1637 |

Time is stored in a shared `ClockTime` struct protected by a FreeRTOS mutex. The timekeeping task posts snapshots to a queue so the display task never blocks the tick.

### Setting the Time Manually

If Wi-Fi is unavailable, use the buttons:
- **Hour button:** increments the hour
- **Minute button:** increments the minute and resets seconds to 0

## Project Structure

```
freertos-clock/
├── include/
│   └── config.h            # local config (gitignored)
├── src/
│   ├── main.cpp            
│   ├── task_timekeeping.cpp
│   ├── task_display.cpp
│   ├── task_ntp_sync.cpp
│   └── task_button_input.cpp
├── platformio.ini          # PlatformIO project config and dependencies
└── .gitignore
```

## Dependencies

Managed automatically by PlatformIO (defined in `platformio.ini`):

- [TM1637 Display](https://github.com/avishorp/TM1637) by avishorp
- [NTPClient](https://github.com/arduino-libraries/NTPClient) by Arduino Libraries

## License

MIT
