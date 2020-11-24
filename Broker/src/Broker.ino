#include <ESP8266WiFi.h>
#include <uMQTTBroker.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Logger.h>
#include "AutomationClient.h"
#include "myBroker.h"
#include "definitions.h"

myMQTTBroker myBroker(mqtt_broker_port,mqtt_max_subscriptions,mqtt_max_retained_topics);
typedef Logger<myMQTTBroker> Log;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

AutomatedWindow<myMQTTBroker> autoWindow(&myBroker);

void setup_wifi()
{
	// Set your Static IP address
	IPAddress local_IP;
	// Set your Gateway IP address
	IPAddress gateway;
	IPAddress subnet;

  local_IP.fromString(mqtt_broker);
  gateway.fromString(mqtt_gateway);
  subnet.fromString(mqtt_subnet);

	// Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Log::error("STA Failed to configure!");
  }

	Log::info("Connecting to " + String(ssid));

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, psk);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Log::info("Trying to connect...");
	}

	timeClient.update();

	Log::info("WiFi Connected!");
	Log::info("IP address: " + WiFi.localIP().toString());
}

void callback(const char* topic, const char* payload, unsigned int length)
{
  autoWindow.callback(topic,payload,length);
}

void setup()
{
  Serial.begin(115200);
  Log::setLevel(Log::LOG_LEVEL_INFO);
  Log::setSerial(&Serial);
  Log::setPrefix("Broker");

  // Load automated window configs
  autoWindow.load();

  // Start WiFi
  setup_wifi();

  timeClient.begin();
  timeClient.setTimeOffset(3600*-3);
  Log::setNTP(&timeClient);

  // Start the broker
  Log::info("Starting MQTT broker");
  myBroker.init();
  Log::setMQTT(&myBroker,"log");
  Log::info("The MQTT broker is alive!");

  Log::info("Setting up the Automated Window Client...");
  myBroker.set_callback(callback);
  autoWindow.subscribe();
  Log::info("Ready!");
}

void loop()
{
  // do anything here
  delay(1000);
}
