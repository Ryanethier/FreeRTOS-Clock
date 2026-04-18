#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <TM1637Display.h>
#include "config.h"

#include "shared.h"

// ---------------------------------------------------------------------------
// Display Task
//
// Blocks on the time queue waiting for updates from the timekeeping task.
// Formats the time and writes it to the TM1637 7-segment display.
// Runs at the lowest priority so it never delays a timekeeping tick.
// ---------------------------------------------------------------------------

static TM1637Display display(PIN_TM1637_CLK, PIN_TM1637_DIO);

void taskDisplay(void *pvParameters) {
    display.setBrightness(DISPLAY_BRIGHTNESS);
    display.clear();

    // Show dashes on startup while waiting for NTP sync
    const uint8_t dashes[] = {
        SEG_G, SEG_G, SEG_G, SEG_G
    };
    display.setSegments(dashes);

    struct ClockTime received;

    for (;;) {
        // Block indefinitely until a new time arrives
        if (xQueueReceive(timeQueue, &received, portMAX_DELAY) == pdTRUE) {
            uint8_t h = received.hours;
            uint8_t m = received.minutes;

            if (!HOUR_FORMAT_24) {
                // Convert to 12-hour format
                if (h == 0) h = 12;
                else if (h > 12) h -= 12;
            }

            // Display as HHMM with colon on
            int timeValue = h * 100 + m;
            display.showNumberDecEx(timeValue, 0b01000000, true);  // 0b01000000 = colon on
        }
    }
}
