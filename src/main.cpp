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
#include <Arduino.h>
#include <ETH.h>
#include <WiFi.h>
#include "WiFiCredentials.h"  // Include the WiFi credentials
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "StartMatch.h"               // Include the StartMatch header
#include "postStopStatus.h"           // Include the postStopStatus header
#include "Field_stack_lightStatus.h"  // Include the Field_stack_lightStatus header
#include "WebServerSetup.h"           // Include the WebServerSetup header
#include "GlobalSettings.h"           // Include the GlobalSettings header


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
const char* baseUrl = "http://192.168.10.124:8080";
//const char* baseUrl = "http://10.0.100.5:8080";

// Define the IP address and DHCP/Static configuration
extern String deviceIP;
extern bool useDHCP;

// Pins connected to the stop button
#define NUM_BUTTONS 7

//C:\Users\Capplegate\.platformio\penv\Scripts\platformio.exe  run -e esp32-s3-devkitm-1 -t upload
#ifdef ESP32_S3_DEVKITM_1
  const int stopButtonPins[NUM_BUTTONS] = {34,  //Field stop
                                          46,   //1E stop
                                          17,   //1A stop
                                          16,   //2E stop
                                          18,   //2A stop
                                          15,   //3E stop
                                          3};   //3A stop
                                                      
  #define START_MATCH_BTN 33
  #define LEDSTRIP 47             // Pin connected to NeoPixel
  //#define ONBOARD_LED 26 //Board does not have
  #define ONBOARD_RGB 21
  Adafruit_NeoPixel onBoardRGB = Adafruit_NeoPixel(1, ONBOARD_RGB, NEO_GRB + NEO_KHZ800);
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

Adafruit_NeoPixel strip = Adafruit_NeoPixel(10, LEDSTRIP, NEO_GRB + NEO_KHZ800);

bool eth_connected = false;

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("WiFi STA Started");
      WiFi.setHostname("Freezy_Red");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.printf("WiFi STA Got IP: '%s'\n", WiFi.localIP().toString().c_str());
      eth_connected = true;
      break;
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
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default: break;
  }
}

IPAddress local_ip(192,168,10,220);
IPAddress gateway(192,168,10,1);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8,8,8,8);
IPAddress secondaryDNS(8,8,4,4);
void intiWifi(){
  WiFi.onEvent(onEvent);
  //eth_connected = true;
	WiFi.mode(WIFI_STA);
	WiFi.config(local_ip,gateway,subnet,primaryDNS,secondaryDNS);
	WiFi.begin(ssid, password);
	USE_SERIAL.print("Connecting to WiFi .. ");
	while(WiFi.status() != WL_CONNECTED){
		USE_SERIAL.print('.');
		delay(1000);
	}
	//WiFi.reconnect();
	Serial.println("Connected to the WiFi network");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
	delay(3000);
}


// Setup function
void setup() {
  Serial.begin(115200);
  delay(5000);


  // Initialize the NeoPixel strip
  strip.begin();
  strip.setBrightness(20);
  strip.show(); // Initialize all pixels to 'off'

  // Initialize the start match button
  pinMode(START_MATCH_BTN, INPUT_PULLUP);


   // Initialize the stop buttons
  for (int i = 0; i < NUM_BUTTONS; i++) {
      pinMode(stopButtonPins[i], INPUT);
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
          // THis is not working Need to fix
            ETH.config(localIP);
            ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);
        } else {
            Serial.println("Invalid static IP address. Falling back to DHCP.");
            ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);
        }
    }

    // Wait for Ethernet to connect
    while (!eth_connected) {
        delay(100);
    }
    // Print the IP address
    Serial.print("init - IP Address: ");
    Serial.println(ETH.localIP());

    onBoardRGB.begin();
    onBoardRGB.setBrightness(50);
    onBoardRGB.show();
  #endif // ESP32


  // Set up the web server
  setupWebServer();

}

// Main loop
void loop() {
  static unsigned long lastStatusCheck = 0;
  static unsigned long lastPrint = 0;
    unsigned long currentMillis = millis();

    // Check if the start match button is pressed
    if (digitalRead(START_MATCH_BTN) == LOW) {
        Serial.println("Start match button pressed!");
        startMatchPost();
    }

    // Create an array to store the states of the stop buttons
  bool stopButtonStates[NUM_BUTTONS];
  for (int i = 0; i < NUM_BUTTONS; i++) {
    stopButtonStates[i] = !digitalRead(stopButtonPins[i]);
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
    // Check alliance status every 500ms
    if (currentMillis - lastStatusCheck >= 500) {
        getField_stack_lightStatus();
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
          uint32_t currentColor = onBoardRGB.getPixelColor(1);
          onBoardRGB.setPixelColor(1, currentColor == onBoardRGB.Color(255, 0, 0) ? onBoardRGB.Color(0, 255, 0) : onBoardRGB.Color(255, 0, 0)); // Toggle between red and green
          onBoardRGB.show();
        #endif
        
    }
        strip.show();
        delay(500);
}
