#ifndef AUTOMATION_CLIENT_H
#define AUTOMATION_CLIENT_H

#include <uMQTTBroker.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <Logger.h>

#define TOPIC_MAX_LENGTH 128

template<typename T>
class AutomatedWindow
{
public:
	typedef Logger<T> Log;

	// Weather Limiting Conditions
	// These are conditions for window to be OPEN
	typedef struct
	{
		// Interval Conditions
		// Weather ID
		// Chekout: https://openweathermap.org/weather-conditions
		int wid[2] = {WID_MIN, WID_MAX};
		int temp[2] = {16, 50}; 		// Degree Celcius		
		// Maximal Conditions
		float wind = 5.5; 			// m/s
		int humidity = 40;			// %

		// If true, uses next i forecasts as dicision factor to
		// close window instead of current weather.
		// It still uses current weather info to open the window.
		uint8_t forecast = 1;
	} wlconditions_t;

	typedef struct
	{
		char mqttTopic[TOPIC_MAX_LENGTH];
		char weatherTopic[TOPIC_MAX_LENGTH];
		bool active = true;
	} config_t;

	static const int WID_MIN = 800;
	static const int WID_MAX = 804;

	AutomatedWindow(T * mqttClient, String mqttTopic = "automatedWindow")
		: _mqttClient(mqttClient), _mqttTopic(mqttTopic) {}

	// wpl: weather data payload
	bool decide(String wpl)
	{
		// Parses string
		const size_t capacity = JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(1) + 3*JSON_OBJECT_SIZE(8) + 310;
		DynamicJsonDocument doc(capacity);

		deserializeJson<String>(doc, wpl);
		JsonArray weather = doc["weather"];

		// Stays true only if all OPEN conditions are matched
		bool match = true;
		for(int i = 0; i < (_wlcond.forecast<=2 ? _wlcond.forecast+1 : 2); i++)
		{
			match &= weather[i]["id"] >= _wlcond.wid[0] && weather[i]["id"] <= _wlcond.wid[1];
			match &= weather[i]["temp"] >= _wlcond.temp[0] && weather[i]["temp"] <= _wlcond.temp[1];
			match &= weather[i]["wind"] <= _wlcond.wind;
			match &= weather[i]["humidity"] <= _wlcond.wind;
		}

		char dummy = '\0';
		if(match)
		{
			// Opens Window
			_mqttClient->publish(String(_windowTopic + "/open").c_str(),&dummy,sizeof(dummy));
		}
		else
		{
			// Closes Window
			_mqttClient->publish(String(_windowTopic + "/close").c_str(),&dummy,sizeof(dummy));
		}
	}

	void setConditions(wlconditions_t & cond) { _wlcond = cond; }
	wlconditions_t getConditions() { return _wlcond; }

	void setMqttTopic(String topic) {_mqttTopic = topic;}
	String getMqttTopic() {return _mqttTopic;}

	void setWeatherTopic(String topic) { _weatherTopic = topic; }
	String getWeatherTopic() { return _weatherTopic; }

	void activate() { _active = true; }
	void deactivate(){ _active = false; }
	bool active() { return _active; }

	void setEEPROMAddress(int add) {_eepromAdd = add;}
	int getEEPROMAddress() {return _eepromAdd;}

	bool subscribe(bool a=false)
	{
		bool ret = true;
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/wid/get").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/wid/set").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/temp/get").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/temp/set").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/wind/get").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/wind/set").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/humidity/get").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/humidity/set").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/forecast/get").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/forecast/set").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/topic/get").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/topic/set").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/activate").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/deactivate").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/save").c_str());
		ret &= _mqttClient->subscribe(String(getMqttTopic() +"/load").c_str());
		if(_active)
			ret &= _mqttClient->subscribe(_weatherTopic.c_str());
		
		if(!ret)
		{
			_err = "subscribe(): failed.";
			Log::error(_err);
		}

		return ret;
	}

	bool unsubscribe(bool a=false)
	{
		bool ret = true;
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/wid/get").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/wid/set").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/temp/get").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/temp/set").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/wind/get").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/wind/set").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/humidity/get").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/humidity/set").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/forecast/get").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/forecast/set").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/topic/get").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/topic/set").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/activate").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/deactivate").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/save").c_str());
		ret &= _mqttClient->unsubscribe(String(getMqttTopic() +"/load").c_str());
		_mqttClient->unsubscribe(_weatherTopic.c_str());

		if(!ret)
		{
			_err = "unsubscribe(): failed.";
			Log::error(_err);
		}

		return ret;
	}

	bool resubcribe()
	{
		if(!unsubscribe())
			return false;
		return subscribe();
	}
	

	bool save(int const address)
	{
		config_t conf;
		conf.active = _active;
		strlcpy(conf.weatherTopic,(const char*)getWeatherTopic().c_str(),TOPIC_MAX_LENGTH);
		strlcpy(conf.mqttTopic,(const char*)getMqttTopic().c_str(),TOPIC_MAX_LENGTH);

		bool ret = true;

		// A fazer: dar um jeito de verificar se isso aqui vai dar certo
		EEPROM.begin(sizeof(wlconditions_t) + sizeof(config_t));
		EEPROM.put<config_t>(address,conf);
		EEPROM.put<wlconditions_t>(address+sizeof(conf),_wlcond);
		ret &= EEPROM.commit();
		EEPROM.end();

		if(!ret)
		{
			_err = "Could not save on EEPROM.";
			Log::error(_err);
		}
		return ret;
	}
	
	bool save(void)
	{
		return save(_eepromAdd);
	}

	bool load(int const address)
	{
		config_t conf;

		bool ret = true;

		// A fazer: dar um jeito de verificar se isso aqui vai dar certo
		EEPROM.begin(sizeof(wlconditions_t) + sizeof(config_t));
		EEPROM.get<config_t>(address,conf);
		EEPROM.get<wlconditions_t>(address+sizeof(conf),_wlcond);
		EEPROM.end();

		if(!ret)
		{
			_err = "Could not read from EEPROM.";
			Log::error(_err);
			return false;
		}

		_active = conf.active;
		setMqttTopic(String(conf.mqttTopic));
		setWeatherTopic(String(conf.weatherTopic));

		return ret;
	}

	bool load(void)
	{
		return load(_eepromAdd);
	}

	bool callback(const char* topic, const char* payload, unsigned int length)
	{
		String stopic = String(topic);
		char msg[length+1];
	    for (int i = 0; i < length; i++) {
	        msg[i] = (char)payload[i];
	    }
	    msg[length] = '\0';

	    return this->callback(stopic, String(msg));
	}

	// Call this method inside your callback functions with raw arguments
	// returns false in case of error
	bool callback(String topic, String payload)
	{
		if(topic == _weatherTopic)
		{
			decide(payload);
		}
		else if(topic == getMqttTopic() +"/wid/get")
		{
			DynamicJsonDocument doc(JSON_OBJECT_SIZE(2));
			doc["min"] = _wlcond.wid[0];
			doc["max"] = _wlcond.wid[1];
			String data = "";
			serializeJson(doc,data);
			if(!_mqttClient->publish(payload.c_str(),data.c_str()))
			{
				_err = "Publish error! Could not publish <" + data + "> to topic <" + payload + ">.";
				Log::error(_err);
				return false;
			}
		}
		else if(topic == getMqttTopic() +"/wid/set")
		{
			DynamicJsonDocument doc(JSON_OBJECT_SIZE(2) + 10);
			deserializeJson(doc, payload);
			if(doc["max"] > WID_MAX || doc["min"] < WID_MIN)
			{
				_err = "WeatherID values out of bonds.";
				Log::error(_err);
				return false;
			}
			_wlcond.wid[0] = doc["min"];
			_wlcond.wid[1] = doc["max"];
		}
		else if(topic == getMqttTopic() +"/temp/get")
		{
			DynamicJsonDocument doc(JSON_OBJECT_SIZE(2));
			doc["min"] = _wlcond.temp[0];
			doc["max"] = _wlcond.temp[1];
			String data = "";
			serializeJson(doc,data);
			if(!_mqttClient->publish(payload.c_str(),data.c_str()))
			{
				_err = "Publish error! Could not publish <" + data + "> to topic <" + payload + ">.";
				Log::error(_err);
				return false;
			}
		}
		else if(topic == getMqttTopic() +"/temp/set")
		{
			DynamicJsonDocument doc(JSON_OBJECT_SIZE(2) + 10);
			deserializeJson(doc, payload);
			_wlcond.temp[0] = doc["min"];
			_wlcond.temp[1] = doc["max"];
		}
		else if(topic == getMqttTopic() +"/wind/get")
		{
			if(!_mqttClient->publish(payload.c_str(),String(_wlcond.wind).c_str()))
			{
				_err = "Publish error! Could not publish <" + String(_wlcond.wind) + "> to topic <" + payload + ">.";
				Log::error(_err);
				return false;
			}
		}
		else if(topic == getMqttTopic() +"/wind/set")
		{
			_wlcond.wind = payload.toFloat();
		}
		else if(topic == getMqttTopic() +"/humidity/get")
		{
			if(!_mqttClient->publish(payload.c_str(),String(_wlcond.humidity).c_str()))
			{
				_err = "Publish error! Could not publish <" + String(_wlcond.humidity) + "> to topic <" + payload + ">.";
				Log::error(_err);
				return false;
			}
		}
		else if(topic == getMqttTopic() +"/humidity/set")
		{
			_wlcond.humidity = payload.toInt();
		}
		else if(topic == getMqttTopic() +"/forecast/get")
		{
			if(!_mqttClient->publish(payload.c_str(),String(_wlcond.forecast).c_str()))
			{
				_err = "Publish error! Could not publish <" + String(_wlcond.forecast) + "> to topic <" + payload + ">.";
				Log::error(_err);
				return false;
			}
		}
		else if(topic == getMqttTopic() +"/forecast/set")
		{
			_wlcond.forecast = payload.toInt();
		}
		else if(topic == getMqttTopic() +"/activate")
		{
			_active = true;
			if(!_mqttClient->subscribe(_weatherTopic.c_str()))
			{
				_err = "Could not subscribe to <" + _weatherTopic; + ">";
				Log::error(_err);
				return false;
			}
		}
		else if(topic == getMqttTopic() +"/deactivate")
		{
			_active = false;
			if(!_mqttClient->unsubscribe(_weatherTopic.c_str()))
			{
				_err = "Could not unsubscribe from <" + _weatherTopic; + ">";
				Log::error(_err);
				return false;
			}
		}
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

	String err()
	{
		String ret = _err;
		_err = "";
		return ret;
	}



protected:
	T * _mqttClient;
	String _err;

private:
	String _mqttTopic;
	String _weatherTopic = "weather";
	String _windowTopic = "smarthome/window";
	int _eepromAdd = 0;
	wlconditions_t _wlcond;
	bool _active = true;
};

#endif
