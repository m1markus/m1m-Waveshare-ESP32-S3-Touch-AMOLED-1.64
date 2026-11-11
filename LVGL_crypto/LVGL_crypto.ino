 #include  <WiFi.h> 
 #include  <HTTPClient.h> 
 #include  <WiFiClientSecure.h> 
 #include  <ArduinoJson.h>

 #include "../env.h"

const char* WIFI_SSID = WIFI_SSID_VALUE;
const char* WIFI_PASS = WIFI_PASSWORD_VALUE;

const char* API_URL = "https://api.binance.com/api/v3/ticker/price?symbol=BTCUSDT";

void connectWiFi()
{
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

    if (retryCount > 40) {
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

void parseHttpbinResponseHttpBin(const String& payload)
{
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("Response JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* url = doc["url"];

  Serial.println("---- Response Summary ----");
  Serial.print("url: ");
  Serial.println(url);
}

void httpGetFromHttpBinAsJson()
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi is not connected.");
    return;
  }

  //String jsonData = createPostJson();

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

  } else {
    Serial.print("HTTP POST failed. Error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

void myFreeRtosInfo()
{
  Serial.print("FreeRTOS version: ");
  Serial.println(tskKERNEL_VERSION_NUMBER);

  Serial.print("Number of CPU cores: ");
  Serial.println(portNUM_PROCESSORS);

  Serial.print("Current task: ");
  Serial.println(pcTaskGetName(NULL));
}


void setup()
{
  Serial.begin(115200);
  Serial.println("setup() begin");

  myFreeRtosInfo();

  connectWiFi();

  httpGetFromHttpBinAsJson();

  Serial.println("func: setup() done");
}

void loop()
{
  /*
  Serial.println("");
  Serial.print("Running task: ");
  Serial.println(pcTaskGetName(NULL));

  Serial.print("Current core: ");
  Serial.println(xPortGetCoreID());

  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());
  */

  delay(2000);
}

/*

from: https://note.com/ya_ikegami/n/n58273d2d4a14?hl=en

GET https://httpbin.org/anything

response:

{
  "args": {},
  "headers": {
    "Accept": "application/json",
    "Accept-Encoding": "gzip, deflate, br, zstd",
    "Accept-Language": "de-CH,de-DE;q=0.9,de;q=0.8,en-US;q=0.7,en;q=0.6",
    "Host": "httpbin.org",
    "Priority": "u=1, i",
    "Referer": "https://httpbin.org/",
    "Sec-Ch-Ua": "\"Not:A-Brand\";v=\"8\", \"Chromium\";v=\"156\", \"Google Chrome\";v=\"156\"",
    "Sec-Ch-Ua-Mobile": "?0",
    "Sec-Ch-Ua-Platform": "\"macOS\"",
    "Sec-Fetch-Dest": "empty",
    "Sec-Fetch-Mode": "cors",
    "Sec-Fetch-Site": "same-origin",
    "User-Agent": "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/156.0.0.0 Safari/537.36"
  },
  "method": "GET",
  "origin": "2a02:1210:5203:7900:291f:c388:10d2:edfd",
  "url": "https://httpbin.org/anything",
  "data": "",
  "files": {},
  "form": {},
  "json": null
}


  //StaticJsonDocument<192> doc;
  DynamicJsonDocument doc(192); //For ESP32/ESP8266 you'll mainly use dynamic.

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("Response JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* url = doc["url"];

  JsonObject json = doc["json"];

  if (json.isNull()) {
    Serial.println("The response does not contain a JSON object named 'json'.");
    return;
  }

  const char* device = json["device"];
  const char* sensor = json["sensor"];

*/

