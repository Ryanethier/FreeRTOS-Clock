#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// ---------------------------------------------------------------------------
// Shared ClockTime struct — included by all task files
// ---------------------------------------------------------------------------

struct ClockTime {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

// Shared across all tasks (defined in main.cpp)
extern ClockTime currentTime;
extern SemaphoreHandle_t timeMutex;
extern QueueHandle_t timeQueue;
extern bool ntpSynced;