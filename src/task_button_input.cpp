#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "config.h"
#include "shared.h"

// ---------------------------------------------------------------------------
// Button Input Task
//
// Polls the hour and minute buttons with software debounce.
// When a button is pressed, acquires the time mutex and adjusts the time.
// This lets the user manually correct the time if Wi-Fi is unavailable.
// ---------------------------------------------------------------------------

#define DEBOUNCE_MS 50
#define LONG_PRESS_MS 500  // Hold to fast-increment

static unsigned long lastPressHour = 0;
static unsigned long lastPressMin  = 0;

static bool isButtonPressed(int pin, unsigned long &lastPress) {
    if (digitalRead(pin) == LOW) {
        unsigned long now = millis();
        if (now - lastPress > DEBOUNCE_MS) {
            lastPress = now;
            return true;
        }
    }
    return false;
}

void taskButtonInput(void *pvParameters) {
    pinMode(PIN_BTN_HOUR, INPUT_PULLUP);
    pinMode(PIN_BTN_MIN,  INPUT_PULLUP);

    for (;;) {
        bool hourPressed = isButtonPressed(PIN_BTN_HOUR, lastPressHour);
        bool minPressed  = isButtonPressed(PIN_BTN_MIN,  lastPressMin);

        if (hourPressed || minPressed) {
            if (xSemaphoreTake(timeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                if (hourPressed) {
                    currentTime.hours = (currentTime.hours + 1) % 24;
                    Serial.printf("[buttons] Hour set to %02d\n", currentTime.hours);
                }
                if (minPressed) {
                    currentTime.minutes = (currentTime.minutes + 1) % 60;
                    currentTime.seconds = 0;  // Reset seconds on minute change
                    Serial.printf("[buttons] Minute set to %02d\n", currentTime.minutes);
                }
                xSemaphoreGive(timeMutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // Poll at ~100Hz
    }
}
