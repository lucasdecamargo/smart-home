# Smart Home

Welcome to my smart home project using the microcontroller NodeMCU ESP8266!
This implementation is for demonstration purposes and its main concept is that the broker runs on an ESP8266 as well as the services, thanks to the one who has written the [uMQTTBroker](https://github.com/martin-ger/uMQTTBroker)! Check out his repository to see the limitations on running a broker on the ESP controller.

In each subfolder you will find the broker and my so far implemented services. Descriptions are given in their directory.



### Common Dependencies

They all run on a **NodeMCU ESP8266** microcontroller, so you need to pre-configure your Arduino environment. I use the board config `NodeMCU 1.0 (ESP-12E Module)`.

Make sure to install the following Arduino libraries:

* [Logger](https://github.com/lucasdecamargo/arduino-logger)
  * During the development I wrote a common logger class to use in all services and broker. It is completely encapsulated, so I made a whole new library for it. Download and export the ZIP in your Arduino library folder.
* [NTPClient](https://github.com/arduino-libraries/NTPClient)
* [PubSubClient](https://github.com/knolleary/pubsubclient) (except for the broker)
* [ArduinoJson](https://arduinojson.org/)

