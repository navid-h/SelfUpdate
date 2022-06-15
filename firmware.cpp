#include <Arduino.h>
#line 1 "C:\\Users\\Navid.H\\Documents\\Arduino\\AutoHttpUpdate\\AutoHttpUpdate.ino"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

#define WIFI_SSID "Ang-IoT"
#define WIFI_PASS "qwerty2580"
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

const char* fimwareInfo = "https://raw.githubusercontent.com/navid-h/SelfUpdate/main/version-esp32.json";
#define URL_fw_Bin "https://github.com/navid-h/SelfUpdate/raw/main/firmware.bin"

const char* actualVersion = "1.0.0";
void firmwareUpdate();

#line 18 "C:\\Users\\Navid.H\\Documents\\Arduino\\AutoHttpUpdate\\AutoHttpUpdate.ino"
void setup();
#line 29 "C:\\Users\\Navid.H\\Documents\\Arduino\\AutoHttpUpdate\\AutoHttpUpdate.ino"
void loop();
#line 79 "C:\\Users\\Navid.H\\Documents\\Arduino\\AutoHttpUpdate\\AutoHttpUpdate.ino"
void firmwareUpdate(void);
#line 18 "C:\\Users\\Navid.H\\Documents\\Arduino\\AutoHttpUpdate\\AutoHttpUpdate.ino"
void setup() {
  Serial.begin(9600);
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void loop() {
  delay(10);
  static uint32_t checkTime;
  if (millis() - checkTime > 10000) {
    checkTime = millis();
    WiFiClientSecure *client = new WiFiClientSecure;  
    firmwareUpdate();  
    HTTPClient http;
    client->setInsecure();

    Serial.print("[HTTP] begin...\n");    
    http.begin(*client, fimwareInfo); //HTTP
    
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        // Parse the payload in order to get version and url        
        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
          Serial.print(F("deserializeJson() failed with code "));
          Serial.println(err.f_str());
        }
        
        String version = doc["version"].as<String>();
        String firmware_url = doc["raw_url"].as<String>();

        Serial.println(version);
        Serial.println(firmware_url);
        if(!version.equals(actualVersion)) {
          Serial.println("DO UPDATE!!!");
        }
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}
  void firmwareUpdate(void) {
   WiFiClientSecure client;
client.setInsecure();
  httpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}

