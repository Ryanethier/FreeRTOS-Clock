#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "config.h"
#include "shared.h"

// ---------------------------------------------------------------------------
// NTP Sync Task
//
// On boot: connects to Wi-Fi and queries an NTP server to set the initial time.
// After that: re-syncs every NTP_INTERVAL ms to correct any drift.
// If Wi-Fi is unavailable, falls back to 12:00 and lets the timekeeping
// task carry on.
// ---------------------------------------------------------------------------

static WiFiUDP ntpUDP;
static NTPClient timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET);

static bool connectWiFi() {
    Serial.printf("[ntp] Connecting to Wi-Fi: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 40) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        retries++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[ntp] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        Serial.println("[ntp] WARNING: Wi-Fi failed. Starting at 12:00.");
        return false;
    }
}

static void syncTimeFromNTP() {
    ntpSynced = true;
    timeClient.update();

    if (xSemaphoreTake(timeMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        currentTime.hours   = timeClient.getHours();
        currentTime.minutes = timeClient.getMinutes();
        currentTime.seconds = timeClient.getSeconds();
        xSemaphoreGive(timeMutex);
        Serial.printf("[ntp] Time synced: %02d:%02d:%02d\n",
            currentTime.hours, currentTime.minutes, currentTime.seconds);
    } else {
        Serial.println("[ntp] WARNING: Could not acquire mutex for time update");
    }
}

void taskNTPSync(void *pvParameters) {
    if (connectWiFi()) {
        timeClient.begin();
        syncTimeFromNTP();
    } else {
        if (xSemaphoreTake(timeMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            currentTime = {12, 0, 0};
            xSemaphoreGive(timeMutex);
        }
    }

    // Re-sync periodically
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(NTP_INTERVAL));

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[ntp] Wi-Fi disconnected, attempting reconnect...");
            connectWiFi();
        }

        if (WiFi.status() == WL_CONNECTED) {
            syncTimeFromNTP();
        }
    }
}
