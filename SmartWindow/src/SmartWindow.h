#ifndef SMART_WINDOW_H
#define SMART_WINDOW_H

#include <Arduino.h>
#include "WindowActuator.h"
#include "definitions.h"

class LimitSwitch
{
public:
	LimitSwitch(uint8_t pin, bool trigState = true);

	bool read();

private:
	uint8_t _pin;
	bool _trigState;
};


class SmartWindow: public WindowActuator
{
public:
	SmartWindow(const struct config_t config);
	SmartWindow(uint8_t dirPin, uint8_t stepPin, uint8_t sleepPin = 0xFF, unsigned revolutionSteps = 200);

	void open();
	void close();

	bool run();

	enum SensorType {LIMIT_SWITCH};
	enum Status {IDLE, OPENING, CLOSING};

	void setSensor(LimitSwitch * openSens, LimitSwitch * closeSens);
	SensorType getSensorType();

	void setLength(float length);
	float getLength();

	void setConfig(const struct config_t config);

private:
	SensorType _sensType;
	Status _status;
	LimitSwitch * _limOpenSwitch = nullptr;
	LimitSwitch * _limCloseSwitch = nullptr;
	float _length;
	bool _inverted;
};

#endif