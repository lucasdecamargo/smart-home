#ifndef DEFINITIONS_H
#define DEFINITIONS_H

/* ************************************************************************* 
 * MAKE CHANGES HERE IF NEEDED!! 
 * ************************************************************************* */
// WiFi Settings
#define ssid "myWiFi"
#define psk "myWiFiPassword"
// MQTT Settings
#define mqtt_broker "192.168.0.80"  // Static IP Address
#define mqtt_gateway "192.168.0.1"  // Static Gateway IP Address
#define mqtt_subnet "255.255.255.0" // Subnet
#define mqtt_broker_port 1883
#define mqtt_max_subscriptions 10000
#define mqtt_max_retained_topics 30
/* ************************************************************************* */

#endif
