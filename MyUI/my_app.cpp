#include <Arduino.h>

#include  <WiFi.h> 
#include  <HTTPClient.h> 
#include  <WiFiClientSecure.h> 
#include  <ArduinoJson.h>

#include <time.h>

#include "src/ui/ui.h"

#include "../env.h"
#include "my_app.h"
#include "candle.h"

const char* WIFI_SSID = WIFI_SSID_VALUE;
const char* WIFI_PASS = WIFI_PASSWORD_VALUE;

const char* API_URL = "https://api.binance.com/api/v3/ticker/tradingDay?symbol=BTCUSDT";

QueueHandle_t uiCommandQueue;


void connectWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int retryCount = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    retryCount++;

    if (retryCount > 60) {
      Serial.println();
      Serial.println("Wi-Fi connection failed.");
      return;
    }
  }

  Serial.println();
  Serial.println("Wi-Fi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void initNTP() {
    // Zurich timezone (CET/CEST), automatic daylight saving
    const char* timezoneZurich = "CET-1CEST,M3.5.0/2,M10.5.0/3";
    const char* timezone = timezoneZurich;

    // Initialize NTP servers
    configTzTime(timezone,
                 "pool.ntp.org",
                 "time.google.com",
                 "time.cloudflare.com");

    Serial.println("NTP initialized");
}


void get_first_part(const char *input, unsigned char char_separator, char *output)
{
    const char *dot = strchr(input, char_separator);

    if (dot != NULL) {
        size_t len = dot - input;
        strncpy(output, input, len);
        output[len] = '\0';
    } else {
        strcpy(output, input);
    }
}


void get_current_time(char *output, int output_size) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        strftime(output, output_size,
                 "%d.%m.%Y %H:%M:%S",
                 &timeinfo);
    } else {
        Serial.println("Failed to get time");
    }
}


int32_t getIntPartFromPrice(const char *pszPrice) {
    if (pszPrice == NULL) return 0;
    char szPrice[32];
    get_first_part(pszPrice, '.', szPrice);
    return atoi(szPrice);
}


void httpGetBtcQuoteFromBinance() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi is not connected.");
    return;
  }

  bool executeQueueSend = true;

  Serial.println();
  Serial.println("Starting HTTP GET request...");
  Serial.print("URL: ");
  Serial.println(API_URL);
  Serial.println();

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  bool beginResult = http.begin(client, API_URL);

  if (!beginResult) {
    Serial.println("HTTPClient begin failed.");
    return;
  }

  http.setTimeout(10000);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.GET();

  Serial.print("HTTP status code: ");
  Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();

    Serial.println("---- Response Body ----");
    Serial.println(payload);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("Response JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    const char* btcPriceUsd = doc["lastPrice"];
    char btcPriceRounded[50];
    get_first_part(btcPriceUsd, '.', btcPriceRounded);

    Serial.printf("BTC USD price: %s", btcPriceRounded);
    Serial.println("");

    char currTime[32];
    get_current_time(currTime, sizeof(currTime));

    UiCommandItem uiCommandItem;
    uiCommandItem.uiCommand = 1;
    snprintf(uiCommandItem.price, sizeof(uiCommandItem.price), "USD %s", btcPriceRounded);
    snprintf(uiCommandItem.timeAt, sizeof(uiCommandItem.timeAt), "%s", currTime);

    const char* pszPrice = doc["lastPrice"];
    if (pszPrice == NULL) {
        executeQueueSend = false;
    }
    int32_t nPrice = getIntPartFromPrice(pszPrice);
    uiCommandItem.candleChartData.priceClose = nPrice;      // store the actual price in the closing field

    pszPrice = doc["openPrice"];
    if (pszPrice == NULL) {
        executeQueueSend = false;
    }
    nPrice = getIntPartFromPrice(pszPrice);
    uiCommandItem.candleChartData.priceOpen = nPrice;

    pszPrice = doc["highPrice"];
    if (pszPrice == NULL) {
        executeQueueSend = false;
    }
    nPrice = getIntPartFromPrice(pszPrice);
    uiCommandItem.candleChartData.priceHight = nPrice;

    pszPrice = doc["lowPrice"];
    if (pszPrice == NULL) {
        executeQueueSend = false;
    }
    nPrice = getIntPartFromPrice(pszPrice);
    uiCommandItem.candleChartData.priceLow = nPrice;

    // overwrite a possible existing queue entry
    if (executeQueueSend) {
        xQueueGenericSend(uiCommandQueue, &uiCommandItem, 0, queueOVERWRITE);
    }

  } else {
    Serial.print("HTTP POST failed. Error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}


void vTaskBinance(void * notUsed) {
    const TickType_t xDelay = 15000 / portTICK_PERIOD_MS;
    for( ;; ) {
        connectWiFi();
        Serial.println("Query Binance API");
        httpGetBtcQuoteFromBinance();
        vTaskDelay(xDelay);
    }
}


void user_pre_ui_setup(void) {
    Serial.println("myApp pre_ui_setup");
    connectWiFi();
    initNTP();
}


void user_setup(void) {
    Serial.println("myApp user_setup");

    uiCommandQueue = xQueueCreate(1, sizeof(UiCommandItem));

    xTaskCreate(vTaskBinance,       /* Function that implements the task. */
                "vTaskBinance",     /* Text name for the task. */
                16*1024,            /* Stack size in words, not bytes. */
                (void *) 0,         /* Parameter passed into the task. */
                tskIDLE_PRIORITY,   /* Priority at which the task is created. */
                NULL); 
}


void esp_debug() {
    Serial.printf("heap free=%u minfree=%u maxblock=%u stackHW=%u\n",
        ESP.getFreeHeap(), ESP.getMinFreeHeap(),
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
        uxTaskGetStackHighWaterMark(NULL));

    lv_mem_monitor_t mon; lv_mem_monitor(&mon);
    Serial.printf("lvgl free=%u biggest=%u frag=%u%%\n",
        mon.free_size, mon.free_biggest_size, mon.frag_pct);
}


void user_loop(void) {
    Serial.println("myApp user_loop");

    esp_debug();

    UiCommandItem receivedItem;

    if (xQueueReceive(uiCommandQueue, &receivedItem, 0) == pdTRUE) {
        // Item received successfully
        Serial.println("UI queue consumer...");
        Serial.println(receivedItem.uiCommand);
        Serial.println(receivedItem.price);
        Serial.println(receivedItem.timeAt);

        char buf[64];
        snprintf(buf, sizeof(buf), "%s", receivedItem.price);
        if (ui_Label1 != NULL) {
            lv_label_set_text(ui_Label1, buf);
        }

        snprintf(buf, sizeof(buf), "%s", receivedItem.timeAt);
        if (ui_Label2 != NULL) {
            lv_label_set_text(ui_Label2, buf);
        }

        candle_stick_day_daw(ui_CanvasCandleDayChart, &(receivedItem.candleChartData));
    }
}

// ESP.restart()
