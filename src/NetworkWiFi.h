#ifndef NETWORK_WIFI_H
#define NETWORK_WIFI_H

#include <WiFi.h>
#include "WiFiCredentials.h"

// Forward declaration - must be defined before including this header
extern void connected(boolean status);

// IP address configuration - must be defined before including this header
extern IPAddress local_ip;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress primaryDNS;
extern IPAddress secondaryDNS;

// Handle states for WiFi connection
void onNetworkEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("WiFi STA Started");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.printf("WiFi STA Got IP: '%s'\n", WiFi.localIP().toString().c_str());
      connected(true);
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi STA Disconnected");
      connected(false);
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      Serial.println("WiFi STA Stop");
      connected(false);
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("WiFi STA Lost IP");
      connected(false);
      break;
    default:
      break;
  }
}

void initNetwork() {
  WiFi.onEvent(onNetworkEvent);
  WiFi.mode(WIFI_STA);
  WiFi.config(local_ip, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi .. ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.printf("Connected! IP: %s, Signal: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  delay(3000);
}

void printCurrentIP() {
  Serial.printf("Current WiFi IP Address: %s\n", WiFi.localIP().toString().c_str());
}

#endif // NETWORK_WIFI_H
