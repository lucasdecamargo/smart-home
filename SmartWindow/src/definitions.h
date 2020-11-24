#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <PubSubClient.h>
#include <Logger.h>

/* ************************************************************************* 
 * MAKE CHANGES HERE IF NEEDED!! 
 * ************************************************************************* */
// WiFi Settings
#define ssid "myWiFi"
#define psk "myWiFiPassword"
// MQTT Settings
#define mqtt_broker "192.168.0.80"
#define mqtt_broker_port 1883
#define mqtt_id "SmartWindow01"  // DO NEVER USE DUPLICATED ID ON BROKER!
#define mqtt_username ""
#define mqtt_password ""
/* ************************************************************************* */



#define DEVICE_ID "SWALPHA01\0"
#define DEVICE_ID_MAX_LENGTH 128
#define CONFIG_SIZE 15

/* THIS ARE THE DEAFULT VALUES! CHANGE IT IF YOU WANT BUT WATCH OUT! */
typedef struct config_t
{
  uint8_t dirPin = 4;
  uint8_t stepPin = 5;
  uint8_t slpPin = 16;
  bool inverted = false;

  unsigned revSteps = 200;

  float radius = 6.35943935;
  float length = 500;
  float maxSpeed = 1080;
  float acc = 360;

  uint8_t limOpenSwitch = 0;
  uint8_t limCloseSwitch = 0;

  int8_t timeUTC = -3;

  bool serialOutput = true;

  char mqttTopicRoot[DEVICE_ID_MAX_LENGTH] = DEVICE_ID;

  uint8_t logLevel = Logger<PubSubClient>::LOG_LEVEL_INFO;
};

#endif
