#ifndef WEATHER_H
#define WEATHER_H

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <Logger.h>

#define CITY_MAX_LENGTH 128
#define TOPIC_MAX_LENGTH 128
#define APIKEY_MAX_LENGTH 64

class Weather
{
public:
	Weather(String apiKey, WiFiClient* wifiClient)
		: _apiKey(apiKey), _wifiClient(wifiClient)
	{
		_err = "";
	}

	String get(String city, unsigned npredictions = 2)
	{
		// Get first current weather data
		String weather = "";
		if(!httpJsonRequest("/data/2.5/weather?q=" + city + "&APPID=" + _apiKey + "&mode=json&units=metric", weather))
			return String(""); // Error occured

		// Get forecast data
		String forecast = "";
		if(npredictions > 0)
		{
			if(!httpJsonRequest("/data/2.5/forecast?q=" + city + "&APPID=" + _apiKey + "&mode=json&units=metric&cnt=" + String(npredictions), forecast))
				return String(""); // Error occured
		}

		// Deserialize current weather data
		DynamicJsonDocument docWeather(JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(13) + 270);
		deserializeJson(docWeather,weather);

		// Parse to output json
		DynamicJsonDocument docOutput(JSON_ARRAY_SIZE(1+npredictions) + JSON_OBJECT_SIZE(1) + (1+npredictions)*JSON_OBJECT_SIZE(8) + 150 + 80*npredictions);
		
		JsonArray w = docOutput.createNestedArray("weather");
		JsonObject w0 = w.createNestedObject();
		w0["id"] = docWeather["weather"][0]["id"];
		w0["main"] = docWeather["weather"][0]["main"];
		w0["description"] = docWeather["weather"][0]["description"];
		w0["temp"] = docWeather["main"]["temp"];
		w0["feels_like"] = docWeather["main"]["feels_like"];
		w0["humidity"] = docWeather["main"]["humidity"];
		w0["wind"] = docWeather["wind"]["speed"];
		w0["dt"] = docWeather["dt"];

		if(npredictions > 0)
		{
			// Deserialize forecast data
			const size_t capacity = (npredictions == 1 ? 2 : npredictions)*JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(npredictions) + (2*npredictions)*JSON_OBJECT_SIZE(1) + (1+npredictions)*JSON_OBJECT_SIZE(2) 
									+ npredictions*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + (2*npredictions)*JSON_OBJECT_SIZE(9) + 370 + (npredictions-1)*50;
			DynamicJsonDocument docForecast(capacity);
			deserializeJson(docForecast,forecast);

			for(unsigned i = 0; i < npredictions; i++)
			{
				JsonObject wi = w.createNestedObject();
				wi["id"] = docForecast["list"][i]["weather"][0]["id"];
				wi["main"] = docForecast["list"][i]["weather"][0]["main"];
				wi["description"] = docForecast["list"][i]["weather"][0]["description"];
				wi["temp"] = docForecast["list"][i]["main"]["temp"];
				wi["feels_like"] = docForecast["list"][i]["main"]["feels_like"];
				wi["humidity"] = docForecast["list"][i]["main"]["humidity"];
				wi["wind"] = docForecast["list"][i]["wind"]["speed"];
				wi["dt"] = docForecast["list"][i]["dt"];
			}
		}

		String output = "";
		serializeJson(docOutput, output);

		return output;
	}

	String err()
	{
		String ret = _err;
		_err = "";
		return ret;
	}

	void setApiKey(String apiKey) {_apiKey = apiKey;}
	String getApiKey() {return _apiKey;}



protected:
	bool httpJsonRequest(const String url, String &output)
	{
	  	// close any connection before send a new request to allow wifiClient make connection to server
		_wifiClient->stop();

		if(_wifiClient->connect(_server,80))
		{
			_wifiClient->println("GET " + url + " HTTP/1.1");
			_wifiClient->println("Host: " + String(_server));
			_wifiClient->println("User-Agent: ArduinoWiFi/1.1");
			_wifiClient->println("Connection: close");
			_wifiClient->println();

			unsigned long timeout = millis();
			while(_wifiClient->available() == 0)
			{
				if(millis() - timeout > 5000)
				{
					_err = "Client timeout (5s).";
					_wifiClient->stop();
					return false;
				}
			}

			char c = 0;
			int jsonend = 0;
			bool startJson = false;
			while(_wifiClient->available())
			{
				c = _wifiClient->read();

				if(c == '{')
				{
					startJson = true;
					jsonend++;
				}

				else if(c == '}')
					jsonend--;

				if(startJson)
					output += c;

				if(jsonend == 0 && startJson)
				{
					startJson = false;
					return true;
				}
			}
		}

		else
		{
			_err = "Connection to server has failed.";
			return false;
		}

		_err = "Something went wrong on trying to request json data. Connection to weather host has might broken.";
		return false;
	}

private:
	String _apiKey;
	WiFiClient* _wifiClient;
	const char* _server = "api.openweathermap.org";

protected:
	String _err;
};


/* DO NOT use the same WiFiClient instance for both MQTTClient and WeatherClient.
 * Use two instances e.g.:
 * WifiClient mqttWiFiClient;
 * WifiClient httpWiFiClient;
 * 
 * PubSubClient mqttClient(mqttWiFiClient);
 * WeatherMQTT weatherMQTT(..., httpWiFiClient); */
template<typename T>
class WeatherMQTT: public Weather
{
public:
	typedef Logger<T> Log;

	typedef struct
	{
		char city[CITY_MAX_LENGTH];
		char mqttTopic[TOPIC_MAX_LENGTH];
		char apiKey[APIKEY_MAX_LENGTH];
		unsigned npredictions;
		unsigned long period;
		unsigned long lastConnectionTime;
	} args_t;

	WeatherMQTT(String apiKey, WiFiClient * wifiClient, T * mqttClient, String mqttTopic = "weather")
		: _wifiClient(wifiClient), _mqttClient(mqttClient), _mqttTopic(mqttTopic), Weather(apiKey, wifiClient)
	{
		// ...
	}

	void setCity(String city) {_city = city;}
	String getCity() {return _city;}

	void setMqttTopic(String topic) {_mqttTopic = topic;}
	String getMqttTopic() {return _mqttTopic;}

	void setPeriod(unsigned long period) {_period = period*1000; _lastConnectionTime = _period;}
	unsigned long getPeriod() {return _period;}

	// Specifies the number of the n following forecast data
	void setnPredictions(unsigned val) {_npredictions = val;}
	unsigned getnPredictions(void) {return _npredictions;}

	bool subscribe()
	{
		bool ret = true;
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/city/get").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/city/set").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/npredictions/get").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/npredictions/set").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/topic/get").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/topic/set").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/apiKey/get").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/apiKey/set").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/period/get").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/period/set").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/save").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/load").c_str());

		if(!ret)
		{
			_err = "Failed to subscribe to weather related topic.";
			Log::error(_err);
		}

		return ret;
	}

	bool unsubscribe()
	{
		bool ret = true;
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/city/get").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/city/set").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/npredictions/get").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/npredictions/set").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/topic/get").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/topic/set").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/apiKey/get").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/apiKey/set").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/period/get").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/period/set").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/save").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/load").c_str());

		if(!ret)
		{
			_err = "Failed to unsubscribe from weather related topics.";
			Log::error(_err);
		}
		
		return ret;
	}

	bool resubscribe()
	{
		if(!unsubscribe())
			return false;
		return subscribe();
	}

	void setEEPROMAddress(int add) {_eepromAdd = add;}
	int getEEPROMAddress() {return _eepromAdd;}


	// Returns the last saved address. You still need to begin EEPROM and commit it!
	// Begin recommended: EEPROM.begin(sizeof(args_t));
	// To commit (after calling save): EEPROM.commit();
	int save(int const address)
	{
		args_t s;
		EEPROM.begin(sizeof(args_t));

		if(EEPROM.length() < sizeof(s))
		{
			_err = "EEPROM length is too small. Please initialize it with EEPROM.begin(sizeof(s_t))";
			Log::error(_err);
			return 0;
		}

		strlcpy(s.city,(const char*)getCity().c_str(),CITY_MAX_LENGTH);
		strlcpy(s.mqttTopic,(const char*)getMqttTopic().c_str(),TOPIC_MAX_LENGTH);
		strlcpy(s.apiKey,(const char*)getApiKey().c_str(),APIKEY_MAX_LENGTH);
		s.npredictions = getnPredictions();
		s.period = getPeriod();
		s.lastConnectionTime = _lastConnectionTime;

		EEPROM.put<args_t>(address,s);
		EEPROM.commit();
		EEPROM.end();

		return address + sizeof(s) -1;
	}

	int save(void)
	{
		return save(_eepromAdd);
	}

	bool load(int const address)
	{
		args_t s;
		EEPROM.begin(sizeof(args_t));

		if(EEPROM.length() < sizeof(s))
		{
			_err = "EEPROM length is too small. Please initialize it with EEPROM.begin(sizeof(s_t))";
			Log::error(_err);
			return false;
		}

		EEPROM.get<args_t>(address,s);
		EEPROM.end();

		setCity(String(s.city));
		setMqttTopic(String(s.mqttTopic));
		setApiKey(String(s.apiKey));
		setnPredictions(s.npredictions);
		_period = s.period; // setPeriod multiplies it by 1000!
		_lastConnectionTime = s.lastConnectionTime;


		return true;
	}

	bool load(void)
	{
		return load(_eepromAdd);
	}

	// Call this method inside your callback functions with raw arguments
	// returns false in case of error
	bool callback(String topic, String payload)
	{
		if(topic == getMqttTopic() +"/city/get")
		{
			if(!_mqttClient->publish(payload.c_str(),getCity().c_str()))
			{
				_err = "Publish error! Could not publish <" + getCity() + "> to topic <" + payload + ">.";
				Log::error(_err);
				return false;
			}
		}
		else if(topic == getMqttTopic() +"/city/set")
			setCity(payload);

		else if(topic == getMqttTopic() +"/npredictions/get")
		{
			if(!_mqttClient->publish(payload.c_str(),String(getnPredictions()).c_str()))
			{
				_err = "Publish error! Could not publish <" + String(getnPredictions()) + "> to topic <" + payload + ">.";
				Log::error(_err);
				return false;
			}
		}
		else if(topic == getMqttTopic() +"/npredictions/set")
			setnPredictions(payload.toInt());

		else if(topic == getMqttTopic() +"/topic/get")
		{
			if(!_mqttClient->publish(payload.c_str(),getMqttTopic().c_str()))
			{
				_err = "Publish error! Could not publish <" + getMqttTopic() + "> to topic <" + payload + ">.";
				Log::error(_err);
				return false;
			}
		}
		else if(topic == getMqttTopic() +"/topic/set")
		{
			if(!unsubscribe())
				return false;

			String hold = getMqttTopic();

			setMqttTopic(payload);

			if(!subscribe())
			{
				_err = "Given topic is might unvalid.";
				Log::error(_err);
				setMqttTopic(hold);
				if(!subscribe())
				{
					_err = "Could not resubscribe to MQTT topic. New given topic was discarded.";
					Log::error(_err);
				}
				return false;
			}
		}

		else if(topic == getMqttTopic() +"/period/get")
		{
			if(!_mqttClient->publish(payload.c_str(),String(getPeriod()).c_str()))
			{
				_err = "Publish error! Could not publish <" + String(getPeriod()) + "> to topic <" + payload + ">.";
				Log::error(_err);
				return false;
			}
		}
		else if(topic == getMqttTopic() +"/period/set")
			setPeriod((unsigned long)payload.toInt());

		else if(topic == getMqttTopic() +"/apiKey/get")
		{
			if(!_mqttClient->publish(payload.c_str(),getApiKey().c_str()))
			{
				_err = "Publish error! Could not publish <" + getApiKey() + "> to topic <" + payload + ">.";
				Log::error(_err);
				return false;
			}
		}
		else if(topic == getMqttTopic() +"/apiKey/set")
			setApiKey(payload);

		else if(topic == getMqttTopic() +"/save")
		{
			if(!save(getEEPROMAddress()))
				return false;
		}
		else if(topic == getMqttTopic() +"/load")
		{
			if(!load(getEEPROMAddress()))
				return false;
		}

		return true;
	}

	bool callback(char* topic, byte* payload, unsigned int length)
	{
		String stopic = String(topic);
		char msg[length+1];
	    for (int i = 0; i < length; i++) {
	        msg[i] = (char)payload[i];
	    }
	    msg[length] = '\0';

	    return this->callback(stopic, String(msg));
	}

	uint16_t minBufferSize() {return _minMqttBuff + _buffSizeInc*_npredictions;}

	// Call this method inside your loop
	bool run()
	{
		if(millis() - _lastConnectionTime > _period)
		{
			_lastConnectionTime = millis();
			
			String payload = get(_city, _npredictions);
			
			if(payload == "")
			{
				Log::error(_err);
				return false;
			}

			if(!_mqttClient->publish(_mqttTopic.c_str(), payload.c_str(), true)) // true -> retained
			{
				_err = "Publish failed. Check if the MQTT Client buffer size matches the minimum required for your given npredictions, given by WeatherClient::minBufferSize().";
				Log::error(_err);
				return false;
			}
		}

		return true;
	}

private:
	WiFiClient * _wifiClient;
	T * _mqttClient;
	String _city;
	String _mqttTopic;
	unsigned _npredictions = 2;
	unsigned long _period = 60*1000; // 60 seg = 1 min
	unsigned long _lastConnectionTime = 60*1000;
	const uint16_t _minMqttBuff = 280;
	const uint16_t _buffSizeInc = 242;
	int _eepromAdd = 0;
};


#endif
