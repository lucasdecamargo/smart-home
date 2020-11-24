# Smart Window

This service was made for a window actuator that receives commands via MQTT to open, close or setting up operational parameters. Note that here a linear actuator was considered for sliding windows that are not common in many countries.

It cooperates with another service called `AutomatedWindow` that bridges information coming from the weather station to the window through a decision maker that closes or opens the windows based on the external weather or forecast.

Mind changing `definitions.h` before uploading the code to your ESP8266.

### Dependencies

Besides the dependencies listed before in the root folder, we have:

* [AccelStepper](http://www.airspayce.com/mikem/arduino/AccelStepper/index.html)
  * [Download](http://www.airspayce.com/mikem/arduino/AccelStepper/AccelStepper-1.61.zip)

### Hardware

Written for using with a [Nema 17](https://www.stepper-motor.cn/product/hybridsteppermotor/nema17steppermotor.html) step motor and a [A4988](https://www.pololu.com/product/1182) driver. The connections are shown below, where the STEP pin is connected to D1 (GPIO 5) and DIR is connected to D2 (GPIO 4). Pins can however be changed in the configurations shown next. Here is an illustration of the electric circuit:

<img src="D:\Biblioteca\Documents\Arduino\smart_home\circuit.png" style="zoom:55%;" />

#### Using Limit Sensor

In order to identify if the window is open or closed I implemented the use of limit sensor, that can be a switch for example. You may use two pull up switches like below.

![](D:\Biblioteca\Documents\Arduino\smart_home\sensor.jpg)

#### The Actuator

The actuator consists on a linear guide built with the step motor, a belt and pulleys. The model is shown below. Two parameters are important for the configuration: **linear length and pulley radius**. They both can be set via the MQTT API presented next. The window is supposed to move as the belt runs along the linear guide. A point to point connection can then be made by using some sort of line for example. Note this is by far not the best design but it was easy to build and it serves pretty enough for a demonstrator.

<img src="D:\Biblioteca\Documents\Arduino\smart_home\actuator.png" style="zoom:60%;" />

### Usage

User Wi-Fi and MQTT configurations are defined in file `definitions.h`. There you will find:

```c++
// WiFi Settings
#define ssid "myWiFi"
#define psk "myWiFiPassword"
// MQTT Settings
#define mqtt_broker "192.168.0.80"
#define mqtt_broker_port 1883
#define mqtt_id "SmartWindow01"  // DO NEVER USE DUPLICATED ID ON BROKER!
#define mqtt_username ""
#define mqtt_password ""
```

 The base actuator API is defined by the UML below:

![](D:\Biblioteca\Documents\Arduino\smart_home\uml.png)

## MQTT API

**First note:** every `/read` topic receives as argument another topic where the response should be published to. For the configuration JSON format, check out the topic below.

A base topic is always set for pre-configurations. Note that `SWALPHA01` will always be set as configuration topic. If more smart windows are to be connected, make sure to change `DEVICE_ID` under `definitions.h`.

1. `SWALPHA01/topic/read`
   Returns the current root topic other than `SWALPHA01`.
2. `SWALPHA01/topic/write`
   Receives the new root topic as argument.
3. `SWALPHA01/reset`
   Resets all configurations parameters.

After defining the new root topic, the API is given. Consider including the root topic before every command.

1. `/log`
   Log messages destination.
2. `/open`
   Opens the window. No parameters needed.
3. `/close`
   Closes the window. No parameters needed.
4. `/config/read`
   Returns current configurations in JSON format.
5. `/config/write`
   Receives new configuration parameters in JSON format. Not all parameters must be set. You can also just write a new acceleration for example. **Note:** changing pins will only take effect after saving configurations and reinitializing the microcontroller.
6. `/config/save`
   Saves the current configurations in the static memory that are loaded in every initialization.
7. `/config/load`
   Loads last saved configuration parameters.
8. `/config/reset`
   Resets all configuration parameters to their default value.

## Configuration JSON Format

As an example, default parameters are listed below in the JSON format. Units are given in degrees, millimetres and seconds. Speed and acceleration are related to the rotor, for example, acceleration is equal to $360 º/s^2$. Limit switches set to zero means that no switch is used for both closing and opening the window. Changing pins as well as the limit switches will only take effect after saving the new configurations and reinitializing the microcontroller.

```json
{
   "dirPin":4,
   "stepPin":5,
   "slpPin":16,
   "revSteps":200,
   "radius":6.359439,
   "length":500,
   "maxSpeed":1080,
   "acc":360,
   "limOpenSwitch":0,
   "limCloseSwitch":0,
   "timeUTC":-3,
   "serialOutput":true,
   "mqttTopicRoot":"SWALPHA01",
   "logLevel":3
}
```

