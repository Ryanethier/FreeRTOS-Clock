# FreeRTOS Digital Clock

An ESP32-based digital clock built with FreeRTOS for an Operating Systems final project. Time is synced automatically over Wi-Fi using NTP, with push buttons for manual adjustment as a fallback.

## Hardware

| Component | Part | Notes |
|---|---|---|
| Microcontroller | ESP32 DevKit (any variant) | |
| Display | TM1637 4-digit 7-segment | CLK → GPIO 22, DIO → GPIO 23 |
| Hour button | Momentary push button | GPIO 18 → GND |
| Minute button | Momentary push button | GPIO 19 → GND |
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

## Getting Started

### 1. Install Prerequisites

- [VS Code](https://code.visualstudio.com/)
- [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode) for VS Code

PlatformIO will automatically download the ESP32 toolchain and all libraries the first time you build.

### 2. Clone the Repo

```bash
git clone https://github.com/your-username/freertos-clock.git
cd freertos-clock
```

### 3. Configure Wi-Fi and Settings

Copy the example config and fill in your details:

```bash
cp include/config.h.example include/config.h
```

Then open `include/config.h` and set:

```c
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
#define NTP_OFFSET    -21600   // UTC offset in seconds
                               // UTC-6 = -21600  (Chicago)
                               // UTC-5 = -18000  (New York)
                               // UTC+0 =  0      (London)
                               // UTC+1 =  3600   (Paris)
```

> **Note:** `config.h` is listed in `.gitignore` so your Wi-Fi credentials will never be committed to GitHub.

### 4. Build and Flash

1. Open the project folder in VS Code
2. Click the **PlatformIO: Build** button (checkmark icon in the bottom toolbar)
3. Connect your ESP32 over USB
4. Click **PlatformIO: Upload** (right arrow icon)
5. Open the Serial Monitor (plug icon) at 115200 baud to see boot logs

On first boot the clock will show `----` while connecting to Wi-Fi, then switch to the current time once NTP sync completes.

### 5. Setting the Time Manually

If Wi-Fi is unavailable, use the buttons:
- **Hour button:** increments the hour
- **Minute button:** increments the minute and resets seconds to 0

## Project Structure

```
freertos-clock/
├── include/
│   ├── config.h.example    # Copy to config.h and fill in your credentials
│   └── config.h            # Your local config (gitignored)
├── src/
│   ├── main.cpp            # Setup, task creation, shared state
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
