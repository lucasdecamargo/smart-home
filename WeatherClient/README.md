# Weather Station

Every smart home you think must include some kind of weather station. Weather and forecast are really important decision factors to take actions inside a house. In order to develop a demonstrator model, I wrote this station service that can be attached to other applications within the ESP8266 or even in one single controller.

Weather data are periodically collected from [OpenWeather](https://openweathermap.org/) through its API. In order for that to work, you must create an account and generate your free API key for the HTTP requests. Once you have the key, you can set it by using the MQTT API that I will present below.

In short, the weather client is fully defined in `weather.h` and wrapped in the `.ino` file as an application.

**Mind** changing `definitions.h` before uploading the code to your ESP8266.

Check out [this webpage](https://openweathermap.org/weather-conditions) as well!

## MQTT API

**First note:** every `/get` topic receives as argument another topic where the response should be published to. For the output JSON format, check out the topic below.

It consists of getters and setters for its parameters as follows. The **standard root topic** is `weather`.

1. `city/get`
   Returns the current set city from where weather data is collected and its country code as `<city>,<country code>`, *e.g.* `Berlin,DE`.
2. `city/set`
   Sets city from where weather data is collected and its country code as `<city>,<country code>`, *e.g.* `Berlin,DE`.
3. `npredictions/get`
   Returns the number of predictions/forecast data.
4. `npredictions/set`
   Sets the number of predictions/forecast data. Maximum value tested is two.
5. `topic/get`
   Returns the current root topic.
6. `topic/set`
   Sets a new root topic.
7. `apiKey/get`
   Returns the current API key from [OpenWeather](https://openweathermap.org/).
8. `apiKey/set`
   Sets an API key from [OpenWeather](https://openweathermap.org/). Consider creating an account and generating one. Otherwise you wont get any response from the weather server.
9. `period/get`
   Returns the current data request period in seconds.
10. `period/set`
    Sets a new data request period in seconds. Consider that OpenWeather lets you make a maximum of 1,000,000 calls per month with a free account! This implies in 60 calls per minute.
11. `save`
    Saves current configuration parameters in the static EEPROM memory.
12. `load`
    Loads last saved configuration parameters from the static EEPROM memory.

## Output JSON Format

The client will periodically publish a JSON data as below for `npredictions=2`.

```json
{
	"weather":
	[
		{
			"id": 801,
			"main": "Cloudy",
			"description": "Pigs are falling from sky! They might want to rule this world...",
			"temp": 23.33,
			"feels_like": 25.66,
			"humidity": 50,
			"wind": 0.21,
			"dt": 1603167280
		},

		{
			"id": 600,
			"main": "Raining",
			"description": "It will rain cows!",
			"temp": 33.33,
			"feels_like": 45.66,
			"humidity": 50,
			"wind": 0.21,
			"dt": 1603168000
		},

		{
			"id": 600,
			"main": "Cloudy",
			"description": "Aliens will be seen on sky!",
			"temp": 23.33,
			"feels_like": 25.66,
			"humidity": 50,
			"wind": 0.21,
			"dt": 1603169000
		}
	]
}
```



## Importing the Weather Client to your Application

Because the weather client was encapsulated from the application, it is easy to implement it in another one. This client could be implemented within the smart window for example.

In order to do that, mind its class constructor:

```c++
template<typename T>
class WeatherMQTT: public Weather
{
public:
    WeatherMQTT(String apiKey, WiFiClient * wifiClient, T * mqttClient, String mqttTopic = "weather");
    /* ... */
};
```

Note that you should specify the MQTT Client as a template and pass its reference to the constructor. Also note that it also requires a Wi-Fi Client for the HTTP requests, but you should not use the same client as the MQTT! So make sure to declare two instances:

```c++
WifiClient mqttWiFiClient;
WifiClient httpWiFiClient;

PubSubClient mqttClient(mqttWiFiClient);
WeatherMQTT<PubSubClient> weatherMQTT(..., &httpWiFiClient);
```

An usage base example is also given below:

```c++
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "weather.h"

WifiClient mqttWiFiClient;
WifiClient httpWiFiClient;

PubSubClient mqttClient(mqttWiFiClient);
WeatherMQTT<PubSubClient> weatherMQTT("API_Key_from_OpenWeather", &httpWiFiClient, &mqttClient);

void setup()
{
    connect_to_wifi();
    connect_to_mqtt();
    
    // Load last saved configurations. If no configurations have been yet saved, the application will not work! So for the first use, comment this line, let the standard configurations be given and save them in the EEPROM memory by calling weatherMQTT.save();
    weatherMQTT.load();
    
    // Callbacks must be redirected to weatherMQTT.callback. See below
    mqttClient.setCallback(mqttCallback);
}

void connect_to_mqtt()
{
    // Set up and connect...
    
    // Call subscribe for weather client at the end!
    weatherMQTT.subscribe();
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
	weatherMQTT.callback(topic,payload,length);
}

void loop()
{
    if (!mqttClient.connected())
        connect_to_mqtt();
    
    mqttClient.loop();
    // This must be periodically called inside the loop!
    weatherMQTT.run();
}
```

### Class UML

![](D:\Biblioteca\Documents\Arduino\smart_home\weather_uml.svg)

