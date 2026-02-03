#ifndef NETWORK_ETHERNET_H
#define NETWORK_ETHERNET_H

#include <ETH.h>

// Ethernet PHY configuration for W5500
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

// Forward declaration - must be defined before including this header
extern void connected(boolean status);

// IP address configuration - must be defined before including this header
extern IPAddress local_ip;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress primaryDNS;
extern IPAddress secondaryDNS;

// External variables from GlobalSettings
extern String deviceIP;
extern bool useDHCP;

// Handle states for Ethernet connection
void onNetworkEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.printf("ETH Got IP");
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
    default:
      break;
  }
}

void initNetwork() {
  Network.onEvent(onNetworkEvent);

  // Initialize Ethernet with DHCP or Static IP
  if (useDHCP) {
    ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST,
              ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);
  } else {
    IPAddress localIP;
    if (localIP.fromString(deviceIP)) {
      Serial.println("Setting static IP address.");
      if (!ETH.config(local_ip, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("Error setting ethernet static ip.");
      }
      ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST,
                ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);
      if (!ETH.config(local_ip, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("Error setting ethernet static ip.");
      }
    } else {
      Serial.println("Invalid static IP address. Falling back to DHCP.");
      ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST,
                ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);
    }
  }
}

void printCurrentIP() {
  Serial.printf("Current Wired IP Address: %s\n", ETH.localIP().toString().c_str());
}

#endif // NETWORK_ETHERNET_H
