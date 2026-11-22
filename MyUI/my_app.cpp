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

static WiFiClientSecure secureClient;      // einmal anlegen, nicht pro Abfrage
volatile uint32_t lastOkMs = 0;


void connectWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.onEvent([](arduino_event_id_t e, arduino_event_info_t info) {
    if (e == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        Serial.printf("ERROR: WiFi disconnected, reason=%d\n", info.wifi_sta_disconnected.reason);
    }
  });
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


bool httpGetBtcQuoteFromBinance() {
  Serial.println("Query Binance API");
  if (WiFi.status() != WL_CONNECTED) return false;

  secureClient.setInsecure();

  HTTPClient http;
  http.setReuse(true);                     // Verbindung wiederverwenden
  http.setConnectTimeout(5000);
  http.setTimeout(10000);
  http.addHeader("Content-Type", "application/json");

  if (!http.begin(secureClient, API_URL)) return false;

  bool ok = false;
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("---- Response Body ----");
    Serial.println(payload);
    JsonDocument doc;

    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      const char* last = doc["lastPrice"];
      const char* open = doc["openPrice"];
      const char* high = doc["highPrice"];
      const char* low  = doc["lowPrice"];

      if (last && open && high && low) {            // NULL-Schutz
        UiCommandItem item;
        memset(&item, 0, sizeof(item));
        item.uiCommand = 1;

        char currTime[32] = "--";                   // initialisiert
        get_current_time(currTime, sizeof(currTime));

        int32_t nLast = getIntPartFromPrice(last);
        snprintf(item.price, sizeof(item.price), "USD %ld", (long)nLast);
        snprintf(item.timeAt, sizeof(item.timeAt), "%s", currTime);
        item.candleChartData.priceClose = nLast;
        item.candleChartData.priceOpen  = getIntPartFromPrice(open);
        item.candleChartData.priceHight = getIntPartFromPrice(high);
        item.candleChartData.priceLow   = getIntPartFromPrice(low);

        xQueueGenericSend(uiCommandQueue, &item, 0, queueOVERWRITE);
        ok = true;
      } else {
        Serial.println("ERROR: JSON response not as expected");
      }
    }
  } else {
    Serial.printf("ERROR: HTTP GET failed: %d %s\n", httpCode,
                  http.errorToString(httpCode).c_str());
  }

  http.end();                                       // immer, auch bei Fehlern
  return ok;
}


void vTaskBinance(void * notUsed) {
  const TickType_t xDelay = pdMS_TO_TICKS(15000);
  for (;;) {
    connectWiFi();
    if (httpGetBtcQuoteFromBinance()) {
      lastOkMs = millis();
    } else {
      secureClient.stop();                          // sauberer Neustart der TLS-Verbindung
    }
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

    Serial.printf("[%s] up=%lus int_free=%u int_maxblock=%u stackHW=%u rssi=%d lastOk=%lus\n",
        pcTaskGetName(NULL), millis()/1000, ESP.getFreeHeap(),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        uxTaskGetStackHighWaterMark(NULL), WiFi.RSSI(), (millis()-lastOkMs)/1000);

    lv_mem_monitor_t mon; lv_mem_monitor(&mon);
    Serial.printf("lvgl free=%u biggest=%u frag=%u%%\n",
        mon.free_size, mon.free_biggest_size, mon.frag_pct);
}


void user_loop(void) {
    //Serial.println("myApp user_loop");

    static uint32_t lastDbg = 0;
    if (millis() - lastDbg > 10000) {
        lastDbg = millis();
        esp_debug();
    }

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
// esp_reset_reason()
