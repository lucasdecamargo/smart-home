#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <Logger.h>
#include "weather.h"
#include "definitions.h"


typedef Logger<PubSubClient> Log;

WiFiClient client;
PubSubClient mqttClient(client);
WiFiClient httpClient;

int status = WL_IDLE_STATUS;

// Get an API Key on https://openweathermap.org/ 
WeatherMQTT<PubSubClient> weatherService("your_API_key_from_OpenWeatherMap", &httpClient, &mqttClient);

void setup() {
  Serial.begin(115200);
  Log::setLevel(Log::LOG_LEVEL_INFO);
  Log::setSerial(&Serial);
  Log::setPrefix("Weather");

  WiFi.begin(ssid,psk);

  Log::info("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Log::info("WiFi Connected");
  printWiFiStatus();

  weatherService.load();

  mqttClient.setBufferSize(weatherService.minBufferSize());
	mqttClient.setServer(mqtt_broker, mqtt_broker_port);
	mqttClient.setCallback(mqttCallback);
}

void mqttReconnect() {
    while (!mqttClient.connected())
    {
        Log::info("Reconnecting MQTT.");
        if (!mqttClient.connect(mqtt_id,mqtt_username,mqtt_password)) {
            Log::error(String("Failed to connect to MQTT, rc=") + String(mqttClient.state()));
            if(WiFi.status() != WL_CONNECTED)
              Log::error("WiFi is not connected!");
            Log::info("Connect retry in 5 seconds.");
            delay(5000);
        }
    }
    Log::setMQTT(&mqttClient);

    weatherService.subscribe();
    Log::info("MQTT Connected!");
}

void loop() { 
	if (!mqttClient.connected())
	  mqttReconnect();

  mqttClient.loop();
  weatherService.run();
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
	weatherService.callback(topic,payload,length);
}

// print Wifi status
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Log::info(String("SSID: ") + WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Log::info(String("IP Address: ") + ip.toString());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Log::info(String("Signal strength (RSSI): ") + String(rssi) + " dBm");
}
