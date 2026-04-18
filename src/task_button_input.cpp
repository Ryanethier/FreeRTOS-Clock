#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "config.h"
#include "shared.h"

#define DEBOUNCE_MS 200

static unsigned long lastPressHour = 0;
static unsigned long lastPressMin  = 0;

void taskButtonInput(void *pvParameters) {
    pinMode(PIN_BTN_HOUR, INPUT_PULLUP);
    pinMode(PIN_BTN_MIN,  INPUT_PULLUP);

    Serial.println("[buttons] Button task started");

    for (;;) {
        unsigned long now = millis();

        // Check hour button
        if (digitalRead(PIN_BTN_HOUR) == LOW) {
            if (now - lastPressHour > DEBOUNCE_MS) {
                lastPressHour = now;

                if (xSemaphoreTake(timeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    currentTime.hours = (currentTime.hours + 1) % 24;
                    currentTime.seconds = 0;
                    ClockTime snapshot = currentTime;
                    xSemaphoreGive(timeMutex);

                    xQueueOverwrite(timeQueue, &snapshot);
                    Serial.printf("[buttons] Hour incremented to %02d\n", snapshot.hours);
                }
            }
        }

        // Check minute button
        if (digitalRead(PIN_BTN_MIN) == LOW) {
            if (now - lastPressMin > DEBOUNCE_MS) {
                lastPressMin = now;

                if (xSemaphoreTake(timeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    currentTime.minutes = (currentTime.minutes + 1) % 60;
                    currentTime.seconds = 0;
                    ClockTime snapshot = currentTime;
                    xSemaphoreGive(timeMutex);

                    xQueueOverwrite(timeQueue, &snapshot);
                    Serial.printf("[buttons] Minute incremented to %02d\n", snapshot.minutes);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}