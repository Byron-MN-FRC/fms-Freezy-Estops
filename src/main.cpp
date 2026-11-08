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

// Set in platformio.ini
// #define PLC_MODEL_TEAM
// #define PLC_MODEL_FIELD_TABLE
// #define PLC_MODEL_FIELD_HUB

// Set in platformio.ini
// #define CON_WIFI
// #define CON_ETH

#include "GlobalSettings.h"           // Include the GlobalSettings header
#include <Arduino.h>
#include <ArduinoJson.h>
#define FASTLED_INTERNAL        // Suppress build banner
#include <FastLED.h>
#include "StartMatch.h"               // Include the StartMatch header
#include "postStopStatus.h"           // Include the postStopStatus header
#include "WebServerSetup.h"           // Include the WebServerSetup header
#include "BlinkState.h"               // Include the BlinkState header
#include "battery.h"                  // Battery charge
#include "postHubStatus.h"            // Post hub status to API

// Board-specific pin definitions (must be before model headers that use them)
#ifdef ESP32_S3_DEVKITM_1
  #define LEDSTRIP 17             // Pin connected to NeoPixel
#elif defined(ESP32DEV)
  #define LEDSTRIP 4              // Pin connected to NeoPixel
#endif


// Model-specific LED count (must be before model headers that use them)
#ifdef PLC_MODEL_FIELD_TABLE
#define NUM_LEDS 16
#include "Field_stack_lightStatus.h"  // Include the Field_stack_lightStatus header
#endif
#ifdef PLC_MODEL_TEAM
#define NUM_LEDS 24
#include "Team_stack_lightStatus.h"   // Include the Team_stack_lightStatus
#endif
#ifdef PLC_MODEL_FIELD_HUB
//#define NUM_LEDS 92
#define NUM_LEDS 92
#include "Field_hub_lightStatus.h"    // Include the Field_hub_lightStatus
#endif

#define USE_SERIAL Serial

// Define the base URL for the API
// const char* baseUrl = "http://192.168.10.124:8080";
const char* baseUrl = "http://10.0.100.5:8080";

// Define the IP address and DHCP/Static configuration
extern String deviceIP;
extern bool useDHCP;
extern String gatewayIP;
extern String netmask;

// Pins connected to the stop button
#define NUM_BUTTONS 7
// The score table wires and reports only the field stop; the team boards report all seven.
#ifdef PLC_MODEL_FIELD_TABLE
#define NUM_REPORTED_BUTTONS 1
#else
#define NUM_REPORTED_BUTTONS NUM_BUTTONS
#endif

//C:\Users\Capplegate\.platformio\penv\Scripts\platformio.exe  run -e esp32-s3-devkitm-1 -t upload
#ifdef ESP32_S3_DEVKITM_1
  const int stopButtonPins[NUM_BUTTONS] = {33,  //Field stop
                                          34,   //1E stop
                                          35,   //1A stop
                                          36,   //2E stop
                                          37,   //2A stop
                                          38,   //3E stop
                                          39};   //3A stop                                         

  #define START_MATCH_BTN 40
  int g_Brightness = 255;//15;         // 0-255 LED brightness scale
  // int g_PowerLimit = 50000;//900;        // 900mW Power Limit

  //#define ONBOARD_LED 26 //Board does not have
  #define ONBOARD_RGB 21

  //Adafruit_NeoPixel onBoardRGB = Adafruit_NeoPixel(10, ONBOARD_RGB, NEO_GRB + NEO_KHZ800);
#endif // ESP32_S3_DEVKITM_1

//C:\Users\Capplegate\.platformio\penv\Scripts\platformio.exe  run -e esp32dev -t upload
#ifdef ESP32DEV
  const int stopButtonPins[NUM_BUTTONS] = {21,  //Field stop
                                          22,   //1E stop
                                          23,   //1A stop
                                          25,   //2E stop
                                          26,   //2A stop
                                          27,   //3E stop
                                          32};   //3a stop
  #define START_MATCH_BTN 19
  #define ONBOARD_LED 2
#endif // ESP32DEV


//Adafruit_NeoPixel strip = Adafruit_NeoPixel(20, LEDSTRIP, NEO_GRB + NEO_KHZ800);

bool net_connected = false;

void connected(boolean status) {
  net_connected = status;
  #ifdef PLC_MODEL_TEAM
  setAllDSIndicators(status ? CRGB::Green : CRGB::White, true);
  #endif
  #ifdef PLC_MODEL_FIELD_HUB
  setHubLight(status ? CRGB::Orange : CRGB::White, true);
  #endif
  if (status) {
      Serial.println("Network Connected"); 
  }
}

// IP address configuration for network modules
IPAddress local_ip(deviceIP.c_str());
IPAddress gateway(gatewayIP.c_str());
IPAddress subnet(netmask.c_str());
IPAddress primaryDNS("8.8.8.8");
IPAddress secondaryDNS("8.8.4.4");

// Include the appropriate network module based on build configuration
#ifdef CON_WIFI
#include "NetworkWiFi.h"
#elif defined(CON_ETH)
#include "NetworkEthernet.h"
#endif

// Setup function
void setup() {
  Serial.begin(115200);
  delay(5000);

  // Initialize the LED strip
  setupLEDs();

  // Initialize the start match button
  pinMode(START_MATCH_BTN, INPUT_PULLDOWN);

   // Initialize the stop buttons
  for (int i = 0; i < NUM_BUTTONS; i++) {
      pinMode(stopButtonPins[i], INPUT_PULLDOWN);
  } 
  
  // Initialize preferences
  preferences.begin("settings", false);

  // Load IP address and DHCP/Static configuration from preferences
  deviceIP = preferences.getString("deviceIP", "");
  useDHCP = preferences.getBool("useDHCP", true);
  allianceColor = preferences.getString("allianceColor", "Red");

  // Initialize the network.
  initNetwork();

  // Set up the web server
  setupWebServer();

  // Battery charge checker
  #ifdef ONBATTERY
  initBattery();
  #endif

  Serial.println("Battery Monitor");
  Serial.println("Initializing...");

}


// The stop states most recently acknowledged by the arena. A change is re-sent every loop until the POST succeeds,
// so a timed-out or lost request cannot drop a press. Zero at boot differs from a released e-stop (true), so the
// initial state is sent as soon as the network is up instead of being recorded and never posted.
bool lastSentStopStates[NUM_BUTTONS];

void processButtonStates() {
    // Check if the start match button is pressed
    if (digitalRead(START_MATCH_BTN) == HIGH) {
        Serial.println("Start match button pressed!");
        startMatchPost();
    }

    // Read the stop buttons with the same wire sense as before: the A-stops (indices 2, 4 and 6) report LOW as
    // true; the field stop and E-stops report LOW as false. See the input enum in the arena's plc/plc.go.
    bool stopButtonStates[NUM_BUTTONS];
    bool stateChanged = false;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        bool low = digitalRead(stopButtonPins[i]) == LOW;
        stopButtonStates[i] = (i == 2 || i == 4 || i == 6) ? low : !low;
        if (i < NUM_REPORTED_BUTTONS && stopButtonStates[i] != lastSentStopStates[i]) {
            stateChanged = true;
        }
    }
    if (!stateChanged) {
        return;
    }

    Serial.println("processButtonStates() statechanged=true");
    bool acknowledged;
    #ifdef PLC_MODEL_FIELD_TABLE
    acknowledged = postSingleStopStatus(0, stopButtonStates[0]);
    #else
    acknowledged = postAllStopStatus(stopButtonStates);
    #endif
    if (acknowledged) {
        memcpy(lastSentStopStates, stopButtonStates, sizeof(stopButtonStates));
    }
}

static unsigned long lastPrint = 0;
static unsigned long lastBatteryCheck = 0;
// Main loop
// Because we are using HTTP requests synchronously, there is a limit to what we can do with LED blinking and other
// effects.  
// The loop runs fast, relying on each method to handle it's state changes accordingly.
void loop() {
    long currentMillis = millis();
    #ifdef PLC_MODEL_TEAM
    processButtonStates();
    #endif
    #ifdef PLC_MODEL_FIELD_TABLE
    processButtonStates();
    #endif

    // Check alliance status
    #ifdef PLC_MODEL_FIELD_HUB
    updateHub_lightStatus();
    #endif 
    #ifdef PLC_MODEL_FIELD_TABLE
      getField_stack_lightStatus();
    #endif
    #ifdef PLC_MODEL_TEAM
    updateTeam_stack_lightStatus();
    #endif

    // print the IP address every 5 seconds
    if (currentMillis - lastPrint >= 5000) {
        lastPrint = currentMillis;
        deviceIP = preferences.getString("deviceIP", "");
        Serial.printf("Preferences IP Address: %s\n", deviceIP.c_str());
        useDHCP = preferences.getBool("useDHCP", true);
        printCurrentIP();
    }


    
    // int heartbeat_LED = 0;
    // // Use a case statement to set the g_LEDs color based on the heartbeat variable
    // switch (heartbeatState) {
    //     case 0:
    //         g_LEDs[heartbeat_LED] = CRGB::Black;
    //         break;
    //     case 1:
    //         g_LEDs[heartbeat_LED] = CRGB::White; 
    //         break;
    //     case 2:
    //         g_LEDs[heartbeat_LED] = CRGB::Orange;
    //         break;
    //     default:
    //         g_LEDs[heartbeat_LED] = CRGB::Red;
    //         break;
    // }
    
    // Serial.printf("show");
    // FastLED.show(g_Brightness); //  Show and delay
    // delay(50);

    // Read battery voltage
    #ifdef ONBATTERY
    if (currentMillis - lastBatteryCheck >= 5000) {
      lastBatteryCheck = currentMillis;
      float voltage = readBatteryVoltage();
      float percentage = calculatePercentage(voltage);

      // Post battery status to API
      postBatteryStatus(voltage, percentage);

      // Print to Serial Monitor
      Serial.print("Battery Voltage: ");
      Serial.print(voltage, 2);
      Serial.print("V | Charge: ");
      Serial.print(percentage, 1);
      Serial.print("%");
      Serial.println("");
    }
    #endif
}
