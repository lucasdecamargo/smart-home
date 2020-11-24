# Broker

It just implements the MQTT broker library [uMQTTBroker](https://github.com/martin-ger/uMQTTBroker). Mind the connection limitations described in their page! For example, it does not support QoS greater than zero and neither too many clients connected simultaneously.

Configure your connection parameters in `definitions.h`. There you can set your static IP address for the broker as well as your Wi-Fi settings.

This application also implements the Automation Client for the Smart Window. Check out below!

### Dependencies

* [uMQTTBroker](https://github.com/martin-ger/uMQTTBroker)

Before uploading the code to the ESP8266, go to your Arduino Board Configuration and **set IwIP Variant to v1.4 Higher Bandwidth**.



# Automated Window

This service is a decision maker and it subscribes to the weather client and publishes to the smart window depending on the user preferences. It consists of closing/opening the window according to weather conditions. It is encapsulated in `AutomatedClient.h` and therefore can be implemented in other applications as well rather than in the broker specifically.

**Note** that the user must set the conditions for the window to be **open**! If these conditions do not match, then it will call the operation to close the window.

## MQTT API

**First note:** every `/get` topic receives as argument another topic where the response should be published to.

It consists of getters and setters for its parameters as follows. The **standard root topic** is `automatedWindow`.

For the weather ID, check out [this webpage](https://openweathermap.org/weather-conditions)!

Intervals are given in **JSON format** like: `{"min": <value>, "max": <value>}`

1. `/wid/get`
   Returns the current weather ID interval set. Output is JSON.

2. `/wid/set`
   Sets a new weather ID interval. To avoid letting your window open during a thunderstorm, the interval is limited to [800, 804]. Input is JSON. Default: `{"min": 800, "max": 804}`

3. `/temp/get`
   Returns the current temperature interval set. Output is JSON.

4. `/temp/set`
   Sets a new temperature interval. Input is JSON. Default: `{"min": 16, "max": 50}`

5. `/wind/get`
   Returns the current maximum wind speed condition set in m/s.

6. `/wind/set`
   Sets a maximum wind speed condition in m/s. Default: `5.5`.

7. `/humidity/get`
   Returns the current maximum humidity condition set in %.

8. `/humidity/set`
   Sets a maximum humidity condition in %. Default: `40`.

9. `/forecast/get`
   Returns the number of forecasts to consider for taking decision. For example, if it is about to rain in, it closes the window before it even starts to rain.

10. `/forecast/set`
    Sets the number of forecasts to consider for taking decision. Default: `1`.

11. `/topic/get`
    Returns the current root topic.

12. `/topic/set`
    Sets a new root topic. Default: `automatedWindow`.

13. `/activate`

    Activates the automation client. Active by default.

14. `/deactivate`
    Deactivates the automation client. (Watch out!)

15. `/save`
    Saves current configuration parameters.

16. `/load`
    Loads last saved configuration parameters.



## Importing the Automation Client to your Application

Because the automation client was encapsulated from the application, it is easy to implement it in another one. This client could be implemented within the smart window for example.

In order to do that, mind its class constructor:

```c++
template<typename T>
class AutomatedWindow
{
public:
    AutomatedWindow(T * mqttClient, String mqttTopic = "automatedWindow");
    /* ... */
};
```

Note that you should specify the MQTT Client as a template and pass its reference to the constructor.

An usage base example is given below:

```c++
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "AutomationClient.h"

WifiClient mqttWiFiClient;
PubSubClient mqttClient(mqttWiFiClient);
AutomatedWindow<PubSubClient> autoWindow(&mqttClient);

void setup()
{
    connect_to_wifi();
    connect_to_mqtt();
    
    // Load last saved configurations. If no configurations have been yet saved, the application will not work! So for the first use, comment this line, let the standard configurations be given and save them in the EEPROM memory by calling autoWindow.save();
    autoWindow.load();
    
    // Callbacks must be redirected to weatherMQTT.callback. See below
    mqttClient.setCallback(mqttCallback);
}

void connect_to_mqtt()
{
    // Set up and connect...
    
    // Call subscribe for weather client at the end!
    autoWindow.subscribe();
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
	autoWindow.callback(topic,payload,length);
}

void loop()
{
    if (!mqttClient.connected())
        connect_to_mqtt();
    
    mqttClient.loop();
}
```

### Class UML

![](D:\Biblioteca\Documents\Arduino\smart_home\autowindow_uml.svg)