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
#ifndef TEAMSTACKLIGHTSTATUS_H
#define TEAMSTACKLIGHTSTATUS_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#define FASTLED_INTERNAL // Suppress build banner
#include <FastLED.h>
#include "GlobalSettings.h"
#include "ColorUtils.h"
#include "BlinkState.h"

// extern Adafruit_NeoPixel strip;

extern const char *baseUrl;
extern bool net_connected;
extern String allianceColor;
extern String arenaIP;
extern String arenaPort;

extern CRGB g_LEDs[]; // Declare the LED array

/*
 Drivers Station Stack Lights:
 The light configuration used for the stack lights is BRG, where each Led is a set of 3 physical LEDs.
 This means for each alliance there are 24x3 = 72 physical LED's

 From the bottom up, we 2 levels: 1 and 2
 The lights are wired from DS2 -> DS3 -> DS1
 DS2:
    L2 = 1,2,5,6
    L1 = 0,3,4,7
 DS3:
    L2 = 9,10,13,14
    L1 = 8,11,12,15
 DS1:
    L2 = 17,18,21,22
    L1 = 16,19,20,23

 The lights are as follows:
 L2: Blue/Red
    off: Connection est. to robot
    solid: Robot enabled
    flash: no connection to robot or bypassed
 L1: Amber
    off: Estop not pressed/disabled
    solid: Estop pressed/enabled
    flash: Astop pressed/enabled during autonomous period
 reference: https://fms-manual.readthedocs.io/en/latest/fms-whitepaper/fms-whitepaper.html#field-status-indicators
*/
int ledMatrix[3][2][4] = {
    { // DS1
    {17, 18, 21, 22}, // L2
    {16, 19, 20, 23} // L1
    },
    {// DS2
     {1, 2, 5, 6},
     {0, 3, 4, 7}
    },
    {// DS3
     {9, 10, 13, 14},
     {8, 11, 12, 15}
    },
};

/**
 * @brief Sets the color of the drivers station stack light LEDs.
 *
 * @param dsN The drivers station number (1-3)
 * @param layerN The stack light layer (1-n)
 * @param rgbColor The color to set the LEDs to if the status is true.
 * @param blank true if we want to blink
 */
void setDSIndicator(int dsN, int layerN, CRGB rgbColor, boolean blink)
{
    if (dsN < 1 || dsN > sizeof(ledMatrix[0]) || layerN < 1 || layerN > sizeof(ledMatrix[0][0])) {
        Serial.printf("setDSIndicator: invalid values: dsN=%i, layerN=%i", dsN, layerN);
    }

    updateBlinkState(500);

    if(blink && !ledBlinkState) {
        rgbColor=CRGB::Black;
    }

    // Serial.print("setDSIndicator:");
    for (int i = 0; i < sizeof(ledMatrix[0][0][0]); i++)
    {
        // Serial.printf("(%i,%i,%i,%s)\n", dsN-1, layerN-1, i, color);
        g_LEDs[ledMatrix[dsN - 1][layerN - 1][i]] = rgbColor;
    }
    FastLED.show();
    // Serial.println("setDSIndicator: return");
}

void setAllDSIndicators(CRGB color, boolean blink)
{
    // Serial.println("setAllDSIndicators");
    for (int i = 1; i <= 3; i++)
    {
        setDSIndicator(i, 1, color, blink);
        setDSIndicator(i, 2, color, blink);
    }
}

// Example payload from FMS:
// {
// 	"red": [
// 		{
// 			"lightStates": [
// 				{
// 					"color": "black",
// 					"blink": false
//					"rgb": {
//						"r": 0,
//						"g": 0,
//						"b": 0
//					},
// 				},
// 				{
// 					"color": "red",
// 					"blink": true
// 				}
// 			]
// 		},
static long lastPollTimeMS;
static const int pollIntervalMS = 200;
/*
  Retrieves the team stack light status from FMS and sets the LED data 
  Stack blinks WHITE while not connected to FMS.
  Stack blinks RED when we fail to get the light status from FMS.
  Once connected, it retrieves the state directly from FMS
 */
void updateTeam_stack_lightStatus()
{
    long currentTime = millis();
    if(currentTime-lastPollTimeMS >= pollIntervalMS) {
        lastPollTimeMS = currentTime;

        // long int currentTime = millis();
        if (net_connected)
        {
            
            HTTPClient http;
            String url = "http://" + arenaIP + ":" + arenaPort + "/api/freezy/team_stack_light?alliance=" + allianceColor;;
            http.setTimeout(1000);
            http.setConnectTimeout(1000);
            http.begin(url);
            int httpResponseCode = http.GET();

            if (httpResponseCode == HTTP_CODE_OK)
            {
                String response = http.getString();
                // Serial.printf("GET request successful! HTTP code: %d\n", httpResponseCode);
                // Serial.println("Response:");
                // Serial.println(response);

                // Parse and print JSON data
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, response);
                if (error)
                {
                    Serial.print("deserializeJson() failed: ");
                    Serial.println(error.f_str());
                    // Blink the error condition
                    setAllDSIndicators(CRGB::Red, true);
                    return;
                }
                String allianceColorLower = allianceColor;
                allianceColorLower.toLowerCase();

                // Return an array of objects, 1 per driver station
                JsonArray teamLightStates = doc[allianceColorLower];
                // Serial.printf(" Team Light States: %i\n", teamLightStates.size());
                for (int i = 0; i < teamLightStates.size(); i++)
                {
                    // Each driver station has a lightStates array (3 of them)
                    JsonObject dsState = teamLightStates[i];
                    JsonArray lightStates = dsState["lightStates"];
                    // Serial.printf("   Light States: %i\n", lightStates.size());
                    // Set the lights for this driver station
                    for (int j = 0; j < lightStates.size(); j++)
                    {
                        bool blink = lightStates[j]["blink"].as<bool>();
                        String colorString = lightStates[j]["color"].as<String>();
                        CRGB color;
                        if (colorString.length() != 0) {
                            color = toRGBColor(colorString);
                        } else {
                            // Fallback if no color string, use the RGB value
                            JsonObject colorRGB = lightStates[j]["rgb"];
                            color = CRGB(colorRGB["r"], colorRGB["g"], colorRGB["b"]);
                        }
                        setDSIndicator(i + 1, j + 1, color, blink);
                    }
                }
            } else {
                Serial.printf("GET request failed! HTTP code: %d\n", httpResponseCode);
                // Blink the error condition
                setAllDSIndicators(CRGB::Red, true);
            }
        }
        else
        {
            // Blink the error condition
            setAllDSIndicators(CRGB::Red, true);
        }
    } // Delay
}

#endif // TEAMSTACKLIGHTSTATUS_H