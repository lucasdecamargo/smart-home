#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <inttypes.h>
#include <stdarg.h>
#include <EEPROM.h>

#include "definitions.h"
#include "SmartWindow.h"
#include <Logger.h>

typedef Logger<PubSubClient> Log;

config_t config;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

SmartWindow* sWindow = nullptr;
LimitSwitch* openSens = nullptr;
LimitSwitch* closeSens = nullptr;

void mqttUpdateTopic()
{
  String mqttTopicRoot = String(config.mqttTopicRoot);
  Log::setMQTT(&mqttClient,String(mqttTopicRoot + "/log"));
  mqttClient.subscribe(String(mqttTopicRoot + "/open").c_str());
  mqttClient.subscribe(String(mqttTopicRoot + "/close").c_str());
  mqttClient.subscribe(String(mqttTopicRoot + "/config/read").c_str());
  mqttClient.subscribe(String(mqttTopicRoot + "/config/write").c_str());
  mqttClient.subscribe(String(mqttTopicRoot + "/config/save").c_str());
  mqttClient.subscribe(String(mqttTopicRoot + "/config/load").c_str());
  mqttClient.subscribe(String(mqttTopicRoot + "/config/reset").c_str());
}

String configSerialize(const struct config_t * confObj)
{
  const size_t capacity = JSON_OBJECT_SIZE(CONFIG_SIZE);
  StaticJsonDocument<capacity> doc;

  doc["dirPin"] = confObj->dirPin;
  doc["stepPin"] = confObj->stepPin;
  doc["slpPin"] = confObj->slpPin;
  doc["revSteps"] = confObj->revSteps;
  doc["radius"] = confObj->radius;
  doc["length"] = confObj->length;
  doc["maxSpeed"] = confObj->maxSpeed;
  doc["acc"] = confObj->acc;
  doc["limOpenSwitch"] = confObj->limOpenSwitch;
  doc["limCloseSwitch"] = confObj->limCloseSwitch;
  doc["timeUTC"] = confObj->timeUTC;
  doc["serialOutput"] = confObj->serialOutput;
  doc["mqttTopicRoot"] = confObj->mqttTopicRoot;
  doc["logLevel"] = confObj->logLevel;

  String output = "";

  serializeJson(doc, output);

  return output;
}

void configDeserealize(struct config_t * confObj, String str)
{
  const size_t capacity = JSON_OBJECT_SIZE(CONFIG_SIZE);
  StaticJsonDocument<capacity> doc;

  DeserializationError error = deserializeJson(doc, str);
  if (error)
  {
    Log::error("Failed to deserialize payload message. Error code: " + String(error.c_str()));
    return;
  }

  // Pin changes only take effect after reinitializing the microcontroler
  if(doc["dirPin"])
     confObj->dirPin = doc["dirPin"];
  if(doc["stepPin"])
     confObj->stepPin = doc["stepPin"];
  if(doc["slpPin"])
     confObj->slpPin = doc["slpPin"];

  if(doc["revSteps"])
  {
     confObj->revSteps = doc["revSteps"];
     sWindow->setRevolutionSteps(confObj->revSteps);
  }
  if(doc["radius"])
  {
     confObj->radius = doc["radius"];
     sWindow->setRadius(confObj->radius);
  }
  if(doc["length"])
  {
     confObj->length = doc["length"];
     sWindow->setLength(confObj->length);
  }
  if(doc["maxSpeed"])
  {
     confObj->maxSpeed = doc["maxSpeed"];
     sWindow->setMaxSpeed(confObj->maxSpeed);
  }
  if(doc["acc"])
  {
     confObj->acc = doc["acc"];
     sWindow->setAcceleration(confObj->acc);
  }

  if(doc["limOpenSwitch"])
     confObj->limOpenSwitch = doc["limOpenSwitch"];
  if(doc["limCloseSwitch"])
     confObj->limCloseSwitch = doc["limCloseSwitch"];

  if(doc["timeUTC"])
  {
     confObj->timeUTC = doc["timeUTC"];
     timeClient.setTimeOffset(3600*config.timeUTC);
  }
  if(doc["serialOutput"])
  {
    confObj->serialOutput = doc["serialOutput"];
    if(confObj->serialOutput)
      Log::setSerial(&Serial);
    else
      Log::setSerial(nullptr);
  }
  if(doc["mqttTopicRoot"])
  {
     mqttClient.unsubscribe(String(String(confObj->mqttTopicRoot) + "/#").c_str());
     // confObj->mqttTopicRoot = doc["mqttTopicRoot"];
     strlcpy(confObj->mqttTopicRoot,(const char*)doc["mqttTopicRoot"],DEVICE_ID_MAX_LENGTH);
     mqttUpdateTopic();
  }
  if(doc["logLevel"])
  {
     confObj->logLevel = doc["logLevel"];
     Log::setLevel(confObj->logLevel);
  }
}

 
void setup() {
    Serial.begin(115200);
    Log::setSerial(&Serial);
    Log::setLevel(Log::LOG_LEVEL_INFO);
    Log::info("Reading data from EEPROM.");

    EEPROM.begin(512);    
    EEPROM.get<struct config_t>(0,config);
    // config.limOpenSwitch = 14;
    // config.limCloseSwitch = 12;
    // EEPROM.put<struct config_t>(0,config);
    // EEPROM.commit();

    timeClient.begin();
    timeClient.setTimeOffset(3600*config.timeUTC);
    Log::setNTP(&timeClient);

    setup_wifi();
    mqttClient.setBufferSize(mqttClient. getBufferSize() *3);
    mqttClient.setServer(mqtt_broker, mqtt_broker_port);
    mqttClient.setCallback(mqttCallback);

    Log::info("MQTT Topic: " + String(config.mqttTopicRoot));

    Log::info("Initializing window actuator.");
    sWindow = new SmartWindow(config);

    if(config.limOpenSwitch != 0 && config.limOpenSwitch != 0)
    {
      openSens = new LimitSwitch(config.limOpenSwitch);
      closeSens = new LimitSwitch(config.limCloseSwitch);
      sWindow->setSensor(openSens, closeSens);
    }
}
 
void setup_wifi() {
    delay(10);
    Log::info("Connecting to " + String(ssid));
 
    WiFi.begin(ssid, psk);
 
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Log::info("Trying to connect...");
    }

    timeClient.update();
 
    Log::info("WiFi Connected!");
    Log::info("IP address: " + WiFi.localIP().toString());
}
 
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String stopic = String(topic);
    String mqttTopicRoot = String(config.mqttTopicRoot);
    char msg[length+1];
    for (int i = 0; i < length; i++) {
        msg[i] = (char)payload[i];
    }
    msg[length] = '\0';

    Log::info("Received message [" + stopic + "]: " + String(msg));
 
    if(stopic == (String(DEVICE_ID) + "/topic/write"))
    {
      Log::info("Changing topic via device root topic to: " + String(msg));
      //config.mqttTopicRoot = String(msg);
      configDeserealize(&config, "{\"mqttTopicRoot\":\""  + String(msg) + "\"}");
    }
    if(stopic == (String(DEVICE_ID) + "/topic/read"))
    {
      Log::info("Reading and sending topic via device root topic.");
      if(!mqttClient.publish(msg, mqttTopicRoot.c_str()))
        Log::error("Publish error!");
    }
    else if(stopic == (String(DEVICE_ID) + "/reset"))
    {
      Log::info("Reseting config. parameters.");
      config = config_t();
    }

    else if(stopic == (mqttTopicRoot + "/config/read"))
    {
      Log::info("Reading config. parameters.");
      String output = configSerialize(&config);
      // Log::info(String("Sending output: " + output));
      if(!mqttClient.publish(msg, output.c_str()))
        Log::error("Publish error!");
    }
    else if(stopic == (mqttTopicRoot + "/config/write"))
    {
      Log::info("Writing config. parameters.");
      configDeserealize(&config, String(msg));
    }
    else if(stopic == (mqttTopicRoot + "/config/reset"))
    {
      Log::info("Reseting config. parameters.");
      config = config_t();
    }
    else if(stopic == (mqttTopicRoot + "/config/save"))
    {
      Log::info("Saving config. parameters.");
      EEPROM.put<struct config_t>(0,config);
      EEPROM.commit();
    }
    else if(stopic == (mqttTopicRoot + "/config/load"))
    {
      Log::info("Loading config. parameters.");
      EEPROM.get<struct config_t>(0,config);
    }

    else if(stopic == (mqttTopicRoot + "/open"))
    {
      Log::info("Opening window.");
      // OPEN WINDOW // Implementar parâmetros de abrir/fechar janela: acc, vel. etc
      sWindow->open();
    }
    else if(stopic == (mqttTopicRoot + "/close"))
    {
      Log::info("Closing window.");
      // CLOSE WINDOW
      sWindow->close();
    }




    if(strcmp(msg,"on")==0){
        Log::info("Lights on");
    }
    else if(strcmp(msg,"off")==0){
        Log::info("Lights off");
    }
}
 
void mqttReconnect() {
    while (!mqttClient.connected()) {
        Log::info("Reconnecting MQTT.");
        if (!mqttClient.connect(mqtt_id,mqtt_username,mqtt_password)) {
            Log::error("Failed to connect, rc=" + mqttClient.state());
            if(WiFi.status() != WL_CONNECTED)
              Log::warning("WiFi is not connected!");
            Log::info("Connect retry in 5 seconds.");
            delay(5000);
        }
    }
    mqttClient.subscribe(String(DEVICE_ID + String("/topic/write")).c_str());
    mqttClient.subscribe(String(DEVICE_ID + String("/topic/read")).c_str());
    mqttClient.subscribe(String(DEVICE_ID + String("/reset")).c_str());
    mqttUpdateTopic();
    Log::info("MQTT Connected!");
}
 
void loop() {
    if (!mqttClient.connected())
        mqttReconnect();

    mqttClient.loop();

    // Checks if there is some task staked on SmartWindow
    if(sWindow->isRunning())
    {
      Log::info("Starting window operation.");
      String mqttTopicRoot = String(config.mqttTopicRoot);
      // Do not listen to commands!
      mqttClient.unsubscribe(String(mqttTopicRoot + "/open").c_str());
      mqttClient.unsubscribe(String(mqttTopicRoot + "/close").c_str());
      sWindow->enable();
      while(sWindow->run())
      {
        yield();
      }
      sWindow->disable();
      // Reenable listening
      mqttClient.subscribe(String(mqttTopicRoot + "/open").c_str());
      mqttClient.subscribe(String(mqttTopicRoot + "/close").c_str());
      Log::info("Finished window operation.");
    }
    // timeClient.update();
}