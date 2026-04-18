#include <Arduino.h>
#include <TM1637Display.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define CLK 22
#define DIO 23
#define WIFI_SSID     "The Thornton"
#define WIFI_PASSWORD "BrandyAliceRyan502!"
#define NTP_OFFSET    -18000  // UTC-6 Chicago

TM1637Display display(CLK, DIO);
QueueHandle_t timeQueue;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", NTP_OFFSET);

struct ClockTime {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

ClockTime currentTime = {0, 0, 0};

void taskTimekeeping(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(1000);

    for (;;) {
        vTaskDelayUntil(&lastWakeTime, interval);

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

        xQueueOverwrite(timeQueue, &currentTime);
    }
}

void taskDisplay(void *pvParameters) {
    display.setBrightness(4);
    display.clear();

    ClockTime received;
    for (;;) {
        if (xQueueReceive(timeQueue, &received, portMAX_DELAY) == pdTRUE) {
            int timeValue = received.hours * 100 + received.minutes;
            display.showNumberDecEx(timeValue, 0b01000000, true);
            Serial.printf("Time: %02d:%02d:%02d\n", received.hours, received.minutes, received.seconds);
        }
    }
}

void taskNTPSync(void *pvParameters) {
    Serial.println("[ntp] Connecting to Wi-Fi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 60) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        retries++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[ntp] Connected! IP: %s\n", WiFi.localIP().toString().c_str());

        timeClient.begin();
        timeClient.update();

        currentTime.hours   = timeClient.getHours();
        currentTime.minutes = timeClient.getMinutes();
        currentTime.seconds = timeClient.getSeconds();

        Serial.printf("[ntp] Time synced: %02d:%02d:%02d\n", 
            currentTime.hours, currentTime.minutes, currentTime.seconds);
    } else {
        Serial.println("[ntp] Wi-Fi failed, starting at 12:00");
        currentTime = {12, 0, 0};
    }

    // Re-sync every hour
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(3600000));
        if (WiFi.status() == WL_CONNECTED) {
            timeClient.update();
            currentTime.hours   = timeClient.getHours();
            currentTime.minutes = timeClient.getMinutes();
            currentTime.seconds = timeClient.getSeconds();
            Serial.printf("[ntp] Re-synced: %02d:%02d:%02d\n",
                currentTime.hours, currentTime.minutes, currentTime.seconds);
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("[boot] Starting clock...");

    timeQueue = xQueueCreate(1, sizeof(ClockTime));

    // NTP runs first at highest priority to set time before ticking starts
    xTaskCreate(taskNTPSync,     "NTPSync",     4096, NULL, 3, NULL);
    xTaskCreate(taskTimekeeping, "Timekeeping", 2048, NULL, 2, NULL);
    xTaskCreate(taskDisplay,     "Display",     2048, NULL, 1, NULL);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}