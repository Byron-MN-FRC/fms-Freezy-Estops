#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <Arduino.h>

// Declare the allianceColor variable
// One of:  Red, Blue, Field
String allianceColor = "Red";
// String allianceColor = "Blue";

// Declare the arenaIP variable
String arenaIP = "10.0.100.5";

// Declare the arenaPort variable
String arenaPort = "8080";

// Declare the useDHCP variable
bool useDHCP = false;

// Declare the deviceIP variable
// String deviceIP = "10.0.100.22";  // Red
// String deviceIP = "10.0.100.23"; // Blue
String deviceIP = "10.0.100.24";  // Red Hub

String gatewayIP = "10.0.100.3";

String netmask = "255.255.255.0";

#endif // GLOBALSETTINGS_H