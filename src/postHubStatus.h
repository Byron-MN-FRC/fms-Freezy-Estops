/*  ______                              ___
   / ____/_______  ___  ____  __  __   /   |  ________  ____  ____ _
  / /_  / ___/ _ \/ _ \/_  / / / / /  / /| | / ___/ _ \/ __ \/ __ `/
 / __/ / /  /  __/  __/ / /_/ /_/ /  / ___ |/ /  /  __/ / / / /_/ /
/_/ __/_/___\___/\___/ /___/\__, /  /_/  |_/_/   \___/_/ /_/\__,_/
   / ____/ ___// /_____  __/____/___
  / __/  \__ \/ __/ __ \/ __ \/ ___/
 / /___ ___/ / /_/ /_/ / /_/ (__  )
/_____//____/\__/\____/ .___/____/
                     /_/
*/
#ifndef POSTBATTERYSTATUS_H
#define POSTBATTERYSTATUS_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "GlobalSettings.h"

extern bool net_connected;
extern String arenaIP;
extern String arenaPort;

/**
 * Sends an HTTP POST request to update the battery status.
 *
 * @param voltage The battery voltage reading.
 * @param percent The battery charge percentage.
 */
void postBatteryStatus(float voltage, float percent) {
    if (net_connected) {
        HTTPClient http;
        http.setTimeout(1000);
        http.setConnectTimeout(1000);

        // Define payload: {"voltage": 12.53, "percent": 100.0}
        JsonDocument payload;
        payload["voltage"] = voltage;
        payload["percent"] = percent;

        // Convert payload to JSON string
        String jsonString;
        serializeJson(payload, jsonString);

        // Configure HTTP POST request
        String url = "http://" + arenaIP + ":" + arenaPort + "/api/freezy/hub_status?alliance=" + allianceColor;
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        // Send the request
        int httpResponseCode = http.POST(jsonString);

        // Handle the response
        if (httpResponseCode > 0) {
            // Serial.printf("Battery status POST successful! HTTP code: %d\n", httpResponseCode);
        } else {
            Serial.printf("Battery status POST failed! Error code: %d\n", httpResponseCode);
        }

        // Close the connection
        http.end();
    }
}

#endif // POSTBATTERYSTATUS_H
