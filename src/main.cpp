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
// #define PLC_MODEL_DRIVERS_STATION
// #define PLC_MODEL_FIELD_TABLE
#define PLC_MODEL_FIELD_HUB

#include "GlobalSettings.h"           // Include the GlobalSettings header
#include <Arduino.h>
#include <ETH.h>
// #include <WiFi.h>
// #include "WiFiCredentials.h"  // Include the WiFi credentials
#include <ArduinoJson.h>
#define FASTLED_INTERNAL        // Suppress build banner
#include <FastLED.h>
#include "StartMatch.h"               // Include the StartMatch header
#include "postStopStatus.h"           // Include the postStopStatus header
#include "WebServerSetup.h"           // Include the WebServerSetup header
#include "BlinkState.h"               // Include the BlinkState header

#ifdef PLC_MODEL_FIELD_TABLE
#include "Field_stack_lightStatus.h"  // Include the Field_stack_lightStatus header
#endif
#ifdef PLC_MODEL_DRIVERS_STATION
#include "Team_stack_lightStatus.h"   // Include the Team_stack_lightStatus
#endif
#ifdef PLC_MODEL_FIELD_HUB
#include "Field_hub_lightStatus.h"    // Include the Field_hub_lightStatus
#endif


// Blink state variables (defined in BlinkState.h as extern)
boolean ledBlinkState = true;
long lastLedBlinkTime = 0;

#ifndef ETH_PHY_CS
#define ETH_PHY_TYPE     ETH_PHY_W5500
#define ETH_PHY_ADDR     1
#define ETH_PHY_CS       14
#define ETH_PHY_IRQ      10
#define ETH_PHY_RST      9
#define ETH_PHY_SPI_HOST SPI2_HOST
#define ETH_PHY_SPI_SCK  13
#define ETH_PHY_SPI_MISO 12
#define ETH_PHY_SPI_MOSI 11
#endif

#define USE_SERIAL Serial

// Define preferences objects
String g_allianceColor;

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
  #define LEDSTRIP 17             // Pin connected to NeoPixel
  int g_Brightness = 255;//15;         // 0-255 LED brightness scale
  // int g_PowerLimit = 50000;//900;        // 900mW Power Limit

#ifdef PLC_MODEL_DRIVERS_STATION
  #define NUM_LEDS 24            // Number of LEDs in the strip
#endif
#ifdef PLC_MODEL_FIELD_HUB
  #define NUM_LEDS 2             // Number of LEDs in the strip
#endif

  CRGB g_LEDs[NUM_LEDS] = {0};    // Frame buffer for FastLED

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
  #define LEDSTRIP 4           // Pin connected to NeoPixel
  #define ONBOARD_LED 2
#endif // ESP32DEV


//Adafruit_NeoPixel strip = Adafruit_NeoPixel(20, LEDSTRIP, NEO_GRB + NEO_KHZ800);

bool eth_connected = false;

void connected(boolean status) {
  eth_connected = status;
  #ifdef PLC_MODEL_DRIVERS_STATION
  setAllDSIndicators(status ? CRGB::Green : CRGB::White, true);
  #endif
  #ifdef PLC_MODEL_FIELD_HUB
  setHubLight(status ? CRGB::Orange : CRGB::White, true);
  #endif
  if (status) {
      Serial.println("ETH Connected"); 
  }
}

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    // case ARDUINO_EVENT_WIFI_STA_START:
    //   Serial.println("WiFi STA Started");
    //   WiFi.setHostname("Freezy_Red");
    //   break;
    // case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    //   Serial.printf("WiFi STA Got IP: '%s'\n", WiFi.localIP().toString().c_str());
    //   connected(true);
    //   break;
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("Freezy_ScoreTable");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED: 
      Serial.println("ETH Connected"); 
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:    
      Serial.printf("ETH Got IP: '%s'\n", esp_netif_get_desc(info.got_ip.esp_netif)); 
      Serial.println(ETH);
      connected(true);
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      connected(false);
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      connected(false);
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      connected(false);
      break;
    default: break;
  }
}

IPAddress local_ip(deviceIP.c_str());
IPAddress gateway(gatewayIP.c_str());
IPAddress subnet(netmask.c_str());
IPAddress primaryDNS("8.8.8.8");
IPAddress secondaryDNS("8.8.4.4");
// void intiWifi(){
//   WiFi.onEvent(onEvent);
//   //eth_connected = true;
// 	WiFi.mode(WIFI_STA);
// 	WiFi.config(local_ip,gateway,subnet,primaryDNS,secondaryDNS);
// 	WiFi.begin(ssid, password);
// 	USE_SERIAL.print("Connecting to WiFi .. ");
// 	while(WiFi.status() != WL_CONNECTED){
// 		USE_SERIAL.print('.');
// 		delay(1000);
// 	}
// 	//WiFi.reconnect();
// 	Serial.println("Connected to the WiFi network");
//   Serial.print("IP Address: ");
//   Serial.println(WiFi.localIP());
// 	delay(3000);
// }

void setupTeamLeds() {
  // The 12v stack light strip that has 3-LEDs per position.
  FastLED.addLeds<WS2811, LEDSTRIP, BRG>(g_LEDs, NUM_LEDS);               // Add our LED strip to the FastLED library
  FastLED.setTemperature(Tungsten100W);
  // The test black 
  // FastLED.addLeds<WS2812B, LEDSTRIP, GRB>(g_LEDs, NUM_LEDS);               // Add our LED strip to the FastLED library
	FastLED.setBrightness(g_Brightness);
  //set_max_power_indicator_LED(LED_BUILTIN);                               // Light the builtin LED if we power throttle
  // FastLED.setMaxPowerInMilliWatts(g_PowerLimit);                          // Set the power limit, above which brightness will be throttled
  
}


// Setup function
void setup() {
  Serial.begin(115200);
  delay(5000);

  // Initialize the LED strip
  setupTeamLeds();

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
    g_allianceColor = preferences.getString("allianceColor", "Red");

  #ifdef ESP32DEV
    // Connect to the WiFi network
    intiWifi();
    pinMode(ONBOARD_LED, OUTPUT);
  #endif // ESP32 

  #ifdef ESP32_S3_DEVKITM_1
    Network.onEvent(onEvent);
    // Initialize Ethernet with DHCP or Static IP
    if (useDHCP) {
        ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);
    } else {
        IPAddress localIP;
        if (localIP.fromString(deviceIP)) {
          Serial.println("Setting static IP address.");
          if ( !ETH.config(local_ip,gateway,subnet,primaryDNS,secondaryDNS) ) {
            Serial.println("Error setting ethernet static ip.");
          }
          ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);
          if ( !ETH.config(local_ip,gateway,subnet,primaryDNS,secondaryDNS) ) {
            Serial.println("Error setting ethernet static ip.");
          }
        } else {
            Serial.println("Invalid static IP address. Falling back to DHCP.");
            ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);
        }
    }

    // // Wait for Ethernet to connect
    // while (!eth_connected) {
    //     delay(100);
    // }
    // Print the IP address
    Serial.print("init - IP Address: ");
    Serial.println(ETH.localIP());


  #endif // ESP32


  // Set up the web server
  setupWebServer();

}


unsigned long currentMillis;
static unsigned long lastPrint = 0;
static unsigned long lastStatusCheck = 0;
void processButtonStates() {
    // FastLED.clear(); // Clear the LED strip

    // Check if the start match button is pressed
    if (digitalRead(START_MATCH_BTN) == HIGH) {
        Serial.println("Start match button pressed!");
        startMatchPost();
    }

    // Create an array to store the states of the stop buttons
    bool stopButtonStates[NUM_BUTTONS];
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
      if (i == 2 || i == 4 || i == 6)
      {
        stopButtonStates[i] = false;
        if (digitalRead(stopButtonPins[i]) == LOW)
        {
          stopButtonStates[i] = true;
        }
      }
      else
      {
        // Invert the ESTOPs  only
        stopButtonStates[i] = true;
        if (digitalRead(stopButtonPins[i]) == LOW)
        {
          stopButtonStates[i] = false;
        }
      }
    }

  // Call the postAllStopStatus method with the array
  postAllStopStatus(stopButtonStates);


/*     // Check if the stop buttons are pressed
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (digitalRead(stopButtonPins[i]) == HIGH) {
            stopButtonPressed[i] = true;
            if (stopButtonPressed[i]) {
                Serial.printf("Stop button %d pressed!\n", i);
            }
            postSingleStopStatus(i, false);
        } else{
            stopButtonPressed[i] = false;
            postSingleStopStatus(i, true);
        }
    }
     */  
}

// Main loop
void loop() {
    currentMillis = millis();
    #ifdef PLC_MODEL_DRIVERS_STATION
    processButtonStates();
    #endif

    // Check alliance status every 500ms
    if (currentMillis - lastStatusCheck >= 500) {
        #ifdef PLC_MODEL_FIELD_HUB
        updateHub_lightStatus();
        #endif 
        #ifdef PLC_MODEL_FIELD_TABLE
         getField_stack_lightStatus();
        #endif
        #ifdef PLC_MODEL_DRIVERS_STATION
        updateTeam_stack_lightStatus();
        #endif
        lastStatusCheck = currentMillis;  
    }
    // print the IP address every 5 seconds
    if (currentMillis - lastPrint >= 5000) {
        lastPrint = currentMillis;
        deviceIP = preferences.getString("deviceIP", "");
        Serial.printf("Preferences IP Address: %s\n", deviceIP.c_str());
        useDHCP = preferences.getBool("useDHCP", true);
        #ifdef ESP32DEV
          Serial.printf("Current WiFi IP Address: %s\n", WiFi.localIP().toString().c_str());
          digitalWrite(ONBOARD_LED, !digitalRead(ONBOARD_LED));
        #endif
        #ifdef ESP32_S3_DEVKITM_1
          Serial.printf("Current Wired IP Address: %s\n", ETH.localIP().toString().c_str());
          
        #endif
        
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
    
    Serial.printf("show");
    FastLED.show(g_Brightness); //  Show and delay
    delay(200);
}
