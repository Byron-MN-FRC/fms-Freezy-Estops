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
#ifndef FIELDSTACKLIGHTSTATUS_H
#define FIELDSTACKLIGHTSTATUS_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#define FASTLED_INTERNAL        // Suppress build banner
#include <FastLED.h>
#include "GlobalSettings.h"
#include "ColorUtils.h"
#include "BlinkState.h"

//extern Adafruit_NeoPixel strip;

extern const char* baseUrl;
extern bool net_connected;
extern String allianceColor;
extern String arenaIP;
extern String arenaPort;

CRGB g_LEDs[NUM_LEDS] = {0};


void setupLEDs() {
  // The 12v stack light strip that has 3-LEDs per position.
  // LEDSTRIP macro must be defined before including this header
  FastLED.addLeds<WS2811, LEDSTRIP, BRG>(g_LEDs, NUM_LEDS);
  FastLED.setTemperature(Tungsten100W);
}

/*
 Field Stack Lights:
 The light configuration used for the stack lights is BRG, where each Led is a set of 3 physical LEDs.
 
 L4: 3,4,11,12
 L3: 2,5,10,13
 L2: 1,6,9,14
 L1: 0,7,8,15
 reference: https://fms-manual.readthedocs.io/en/latest/fms-whitepaper/fms-whitepaper.html#field-status-indicators
*/
int ledMatrix[4][4] = {
    {3, 4, 11, 12}, // L4
    {2, 5, 10, 13}, // L3
    {1, 6, 9, 14},  // L2
    {0, 7, 8, 15},  // L1
};

/**
 * @brief Sets the color of the field stack light LEDs.
 *
 * @param layerN The stack light layer (1-n)
 * @param rgbColor The color to set the LEDs to if the status is true.
 * @param blank true if we want to blink
 */
void setFieldIndicator(int layerN, CRGB rgbColor, boolean blink)
{
    if (layerN < 1 || layerN > sizeof(ledMatrix[0])) {
        Serial.printf("setFieldIndicator: invalid values:  layerN=%i", layerN);
    }

    // Blink clock advances once per render pass, not here.
    if(blink && !ledBlinkState) {
        rgbColor=CRGB::Black;
    }

    // Serial.print("setFieldIndicator:");
    for (int i = 0; i < sizeof(ledMatrix[0][0]); i++)
    {
        // Serial.printf("(%i,%i,%i,%s)\n", dsN-1, layerN-1, i, color);
        g_LEDs[ledMatrix[layerN - 1][i]] = rgbColor;
    }
    // Serial.println("setFieldIndicator: return");
}

const CRGB RED_COLOR = CRGB(255, 0, 0);
const CRGB BLUE_COLOR = CRGB(0, 0, 255);
const CRGB ORANGE_COLOR = CRGB(250, 100, 0);
const CRGB GREEN_COLOR = CRGB(0, 255, 0);
const CRGB WHITE_COLOR = CRGB(20, 20, 20);
int heartbeatState = 0;
long int hartBeatTck = 0;
long int currentTime = 0;

// On-time and off-time of a blinking layer. Only knob for blink speed.
#define FIELD_BLINK_RATE_MS 500
// Redraw period. Independent of the 200 ms poll so blink rate is not quantized to it.
#define FIELD_RENDER_INTERVAL_MS 20

// Last values received from the arena.
bool redStackLightStatus = false;
bool blueStackLightStatus = false;
bool orangeStackLightStatus = false;
bool greenStackLightStatus = false;
bool greenStackLightBlink = false;
unsigned long lastRenderMS = 0;

// Redraws the stack light from the cached arena state. Called every loop() so the blink
// period is FIELD_BLINK_RATE_MS rather than the poll interval.
void renderFieldStackLight() {
    unsigned long now = millis();
    if (now - lastRenderMS < FIELD_RENDER_INTERVAL_MS) {
        return;
    }
    lastRenderMS = now;

    updateBlinkState(FIELD_BLINK_RATE_MS);
    setFieldIndicator(4, (redStackLightStatus ? RED_COLOR : CRGB::Black), false);
    setFieldIndicator(3, (blueStackLightStatus ? BLUE_COLOR : CRGB::Black), false);
    setFieldIndicator(2, (orangeStackLightStatus ? ORANGE_COLOR : CRGB::Black), false);
    setFieldIndicator(1, (greenStackLightStatus ? GREEN_COLOR : CRGB::Black), greenStackLightBlink);
    FastLED.show();
}

void getField_stack_lightStatus() {
    long int currentTime = millis();
    renderFieldStackLight();
    if (net_connected) {
        if(currentTime > hartBeatTck){  //Get the status every 200ms
            hartBeatTck = currentTime + 200;
            
            HTTPClient http;
            http.setTimeout(ARENA_HTTP_TIMEOUT_MS);
            http.setConnectTimeout(ARENA_HTTP_TIMEOUT_MS);
            String url = "http://" + arenaIP + ":" + arenaPort + "/api/freezy/field_stack_light?alliance=" + allianceColor;;
            http.begin(url);
            int httpResponseCode = http.GET();

            if (httpResponseCode > 0) {
                String response = http.getString();
                // The body is not printed: at 5 Hz it floods the serial log, and with no USB host attached each
                // write can block for HWCDC's tx timeout, which is time the buttons are not being read.

                // Parse and print JSON data
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, response);
                if (error) {
                    Serial.print("deserializeJson() failed: ");
                    Serial.println(error.f_str());
                    return;
                }

                // Cache the field_stack_light status; renderFieldStackLight() draws it.
                redStackLightStatus = doc["redStackLight"].is<bool>() ? doc["redStackLight"].as<bool>() : false;
                blueStackLightStatus = doc["blueStackLight"].is<bool>() ? doc["blueStackLight"].as<bool>() : false;
                orangeStackLightStatus = doc["orangeStackLight"].is<bool>() ? doc["orangeStackLight"].as<bool>() : false;
                greenStackLightStatus = doc["greenStackLight"].is<bool>() ? doc["greenStackLight"].as<bool>() : false;
                greenStackLightBlink = doc["greenStackLightBlink"].is<bool>() ? doc["greenStackLightBlink"].as<bool>() : false;

                

                // Print the JSON data
                //serializeJsonPretty(doc, Serial);
                //heartbeat = !heartbeat; // Toggle heartbeat
                //setLEDColor(1, 1, heartbeat, WHITE_COLOR); // White
                if (heartbeatState == 0) {
                    heartbeatState = 1;
                } else {
                    heartbeatState = 0;
                }
            } else {
                Serial.printf("GET request failed! Error code: %d\n", httpResponseCode);
                //heartbeat = !heartbeat; // Toggle heartbeat
                //setLEDColor(1, 1, heartbeat, ORANGE_COLOR); // Orange
                if (heartbeatState == 0) {
                    heartbeatState = 2;
                } else {
                    heartbeatState = 0;
                }
            }
            http.end();
        } //Get the status every 200ms

    } else {
        Serial.println("Network not connected! [FSL]");
        //heartbeat = !heartbeat; // Toggle heartbeat
        //setLEDColor(1, 1, heartbeat, ORANGE_COLOR); // Red
        if (heartbeatState == 0) {
            heartbeatState = 3;
        } else {
            heartbeatState = 0;
        }
    }
}

#endif // FIELDSTACKLIGHTSTATUS_H