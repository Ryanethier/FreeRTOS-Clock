#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <TM1637Display.h>
#include "config.h"
#include "shared.h"

// ---------------------------------------------------------------------------
// Display Task
//
// Blocks on the time queue waiting for updates from the timekeeping task.
// Formats the time and writes it to the TM1637 7-segment display.
// Runs at priority 1 (lowest) so it never delays a timekeeping tick.
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

    ClockTime received;
    for (;;) {
        if (xQueueReceive(timeQueue, &received, portMAX_DELAY) == pdTRUE) {
            int timeValue = received.hours * 100 + received.minutes;
            display.showNumberDecEx(timeValue, 0b01000000, true);
            Serial.printf("[display] Time: %02d:%02d:%02d\n",
                received.hours, received.minutes, received.seconds);
        }
    }
}
