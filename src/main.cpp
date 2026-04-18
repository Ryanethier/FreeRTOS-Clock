#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "config.h"
#include "shared.h"

// ---------------------------------------------------------------------------
// Global definitions (declared extern in shared.h)
// ---------------------------------------------------------------------------
ClockTime currentTime = {0, 0, 0};
SemaphoreHandle_t timeMutex;
QueueHandle_t timeQueue;

// ---------------------------------------------------------------------------
// Task prototypes
// ---------------------------------------------------------------------------
void taskTimekeeping(void *pvParameters);
void taskDisplay(void *pvParameters);
void taskNTPSync(void *pvParameters);

// ---------------------------------------------------------------------------
// Setup & loop
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    Serial.println("[boot] FreeRTOS Digital Clock starting...");

    // Create synchronization primitives
    timeMutex = xSemaphoreCreateMutex();
    timeQueue  = xQueueCreate(1, sizeof(ClockTime));

    if (timeMutex == NULL || timeQueue == NULL) {
        Serial.println("[boot] ERROR: Failed to create RTOS primitives. Halting.");
        while (true) {}
    }

    // Create tasks
    xTaskCreate(taskNTPSync,     "NTPSync",     4096, NULL, 3, NULL);
    xTaskCreate(taskTimekeeping, "Timekeeping", 2048, NULL, 2, NULL);
    xTaskCreate(taskDisplay,     "Display",     2048, NULL, 1, NULL);

    Serial.println("[boot] All tasks created. Scheduler running.");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}