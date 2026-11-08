#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <Arduino.h>

// Declare the allianceColor variable
// One of:  Red, Blue, Field
String allianceColor = "Field";
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
String deviceIP = "10.0.100.26";  // Field

String gatewayIP = "10.0.100.3";

String netmask = "255.255.255.0";

// Timeout, connect and read, for every HTTP call to the arena. On the field LAN the arena answers in a few
// milliseconds, so anything longer is a lost packet, and lwIP would otherwise wait ~3 s to retransmit a lost SYN.
// Every call blocks loop() and the stop buttons are only read between calls, so this is also the worst-case delay
// before a press is noticed. A call that times out is simply retried on the next loop.
#define ARENA_HTTP_TIMEOUT_MS 300

#endif // GLOBALSETTINGS_H