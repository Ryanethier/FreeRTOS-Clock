#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "config.h"

#include "shared.h"

// ---------------------------------------------------------------------------
// Timekeeping Task
//
// Runs every 1 second using vTaskDelayUntil() for precision.
// Increments the shared time struct and pushes a snapshot to the display queue.
// This task runs at the highest priority so it is never delayed by display
// or input work.
// ---------------------------------------------------------------------------

void taskTimekeeping(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(1000);

    for (;;) {
        vTaskDelayUntil(&lastWakeTime, interval);

        if (xSemaphoreTake(timeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            // Increment time
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

            // Push snapshot to display queue (don't block if queue is full)
            ClockTime snapshot = currentTime;
            xSemaphoreGive(timeMutex);

            xQueueOverwrite(timeQueue, &snapshot);

        } else {
            Serial.println("[timekeeping] WARNING: Could not acquire mutex");
        }
    }
}
