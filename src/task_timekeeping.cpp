#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "shared.h"

// ---------------------------------------------------------------------------
// Timekeeping Task
//
// Runs every 1 second using vTaskDelayUntil() for precision.
// Increments the shared time struct and pushes a snapshot to the display queue.
// Runs at priority 2 so it is never delayed by display work.
// ---------------------------------------------------------------------------

void taskTimekeeping(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(1000);

    while (!ntpSynced) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    Serial.println("[timekeeping] NTP Synced, starting clock");

    for (;;) {
        vTaskDelayUntil(&lastWakeTime, interval);

        if (xSemaphoreTake(timeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            currentTime.seconds++;
            if (currentTime.seconds >= 60) {
                currentTime.seconds = 0;
                currentTime.minutes++;
            }
            if (currentTime.minutes >= 60) {
                currentTime.minutes = 0;
                currentTime.hours++;
            }
            if (currentTime.hours >= 24) {
                currentTime.hours = 0;
            }

            ClockTime snapshot = currentTime;
            xSemaphoreGive(timeMutex);

            xQueueOverwrite(timeQueue, &snapshot);
        } else {
            Serial.println("[timekeeping] WARNING: Could not acquire mutex");
        }
    }
}
