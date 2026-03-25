#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "config.h"

// ---------------------------------------------------------------------------
// Shared state
// ---------------------------------------------------------------------------

// Current time struct, protected by timeMutex
struct ClockTime {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

static ClockTime currentTime = {0, 0, 0};
static SemaphoreHandle_t timeMutex;

// Queue for sending time snapshots from timekeeping -> display task
static QueueHandle_t timeQueue;

// ---------------------------------------------------------------------------
// Task prototypes (implemented in src/tasks/)
// ---------------------------------------------------------------------------

void taskTimekeeping(void *pvParameters);
void taskDisplay(void *pvParameters);
void taskButtonInput(void *pvParameters);
void taskNTPSync(void *pvParameters);

// ---------------------------------------------------------------------------
// Setup & loop
// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    Serial.println("[boot] FreeRTOS Digital Clock starting...");

    // Create synchronization primitives
    timeMutex = xSemaphoreCreateMutex();
    timeQueue = xQueueCreate(5, sizeof(ClockTime));

    if (timeMutex == NULL || timeQueue == NULL) {
        Serial.println("[boot] ERROR: Failed to create RTOS primitives. Halting.");
        while (true) {}
    }

    // Create tasks
    // Higher number = higher priority in FreeRTOS
    xTaskCreate(taskNTPSync,      "NTPSync",      4096, NULL, 3, NULL);
    xTaskCreate(taskTimekeeping,  "Timekeeping",  2048, NULL, 4, NULL);  // Highest priority
    xTaskCreate(taskButtonInput,  "ButtonInput",  2048, NULL, 2, NULL);
    xTaskCreate(taskDisplay,      "Display",      2048, NULL, 1, NULL);  // Lowest priority

    Serial.println("[boot] All tasks created. Scheduler running.");
    // FreeRTOS scheduler starts automatically on ESP32 after setup() returns
}

void loop() {
    // Nothing here — all work is done inside FreeRTOS tasks
    vTaskDelay(portMAX_DELAY);
}
