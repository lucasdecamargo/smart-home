#include "SmartWindow.h"

/* CLASS LIMIT SWITCH */
LimitSwitch::LimitSwitch(uint8_t pin, bool trigState)
	: _pin(pin), _trigState(trigState)
{
	pinMode(pin, INPUT);
}

bool LimitSwitch::read()
{
	return(_trigState ? !digitalRead(_pin) : digitalRead(_pin));
}
/**********************/

void SmartWindow::setConfig(const struct config_t config)
{
	_length = config.length;
	_inverted = config.inverted;
	setRadius(config.radius);
	setMaxSpeed(config.maxSpeed);
	setAcceleration(config.acc);
	setRevolutionSteps(config.revSteps);
}

SmartWindow::SmartWindow(const struct config_t config)
	: _sensType(LIMIT_SWITCH), _status(IDLE), _length(config.length), _inverted(config.inverted),
	 WindowActuator(config.dirPin, config.stepPin, config.slpPin, config.revSteps)
{
	setRadius(config.radius);
	setMaxSpeed(config.maxSpeed);
	setAcceleration(config.acc);
}

SmartWindow::SmartWindow(uint8_t dirPin, uint8_t stepPin, uint8_t sleepPin, unsigned revolutionSteps)
	: _sensType(LIMIT_SWITCH), _status(IDLE), _length(0.0), _inverted(false),
	 WindowActuator(dirPin, stepPin, sleepPin, revolutionSteps)
{
/*...*/
}


void SmartWindow::open()
{
	if(_sensType == LIMIT_SWITCH)
	{
		if(_limOpenSwitch != nullptr)
		{
			if(_limOpenSwitch->read())
				return;
		}

		move(_inverted ? -1.0*_length : _length);
		_status =  OPENING;
	}
}


void SmartWindow::close()
{
	if(_sensType == LIMIT_SWITCH)
	{
		if(_limCloseSwitch != nullptr)
		{
			if(_limCloseSwitch->read())
				return;
		}

		move(_inverted ? _length : -1.0*_length);
		_status =  CLOSING;
	}
}


bool SmartWindow::run()
{
	if(_sensType == LIMIT_SWITCH && (_limOpenSwitch != nullptr && _limCloseSwitch != nullptr))
	{
		switch(_status)
		{
			case IDLE:
			return WindowActuator::run();
			break;

			case OPENING:
			if(!_limOpenSwitch->read())
			{
				bool ret = WindowActuator::run();
				if(!ret) _status = IDLE;
				return ret;
			}
			else
			{
				stop();
				_status = IDLE;
				return WindowActuator::run();
			}			
			break;

			case CLOSING:
			if(!_limCloseSwitch->read())
			{
				bool ret = WindowActuator::run();
				if(!ret) _status = IDLE;
				return ret;
			}
			else
			{
				stop();
				_status = IDLE;
				return WindowActuator::run();
			}			
			break;
		}
	}

	else return WindowActuator::run();
}


void SmartWindow::setSensor(LimitSwitch * openSens, LimitSwitch * closeSens)
{
	_limOpenSwitch = openSens;
	_limCloseSwitch = closeSens;
	_sensType = LIMIT_SWITCH;
}


SmartWindow::SensorType SmartWindow::getSensorType()
{
	return _sensType;
}


void SmartWindow::setLength(float length)
{
	_length = length;
}


float SmartWindow::getLength()
{
	return _length;
}
