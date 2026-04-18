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
// If Wi-Fi is unavailable, logs a warning and lets the timekeeping task
// carry on with whatever time it has (or 00:00:00 if never synced).
// ---------------------------------------------------------------------------

static WiFiUDP ntpUDP;
static NTPClient timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET);

static bool connectWiFi() {
    Serial.printf("[ntp] Connecting to Wi-Fi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 40) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        retries++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[ntp] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        Serial.println("[ntp] WARNING: Wi-Fi connection failed. Clock will run without NTP sync.");
        return false;
    }
}

static void syncTimeFromNTP() {
    timeClient.update();

    uint8_t h = timeClient.getHours();
    uint8_t m = timeClient.getMinutes();
    uint8_t s = timeClient.getSeconds();

    if (xSemaphoreTake(timeMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        currentTime.hours   = h;
        currentTime.minutes = m;
        currentTime.seconds = s;
        xSemaphoreGive(timeMutex);
        Serial.printf("[ntp] Time synced: %02d:%02d:%02d\n", h, m, s);
    } else {
        Serial.println("[ntp] WARNING: Could not acquire mutex for time update");
    }
}

void taskNTPSync(void *pvParameters) {
    if (connectWiFi()) {
        timeClient.begin();
        syncTimeFromNTP();
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
