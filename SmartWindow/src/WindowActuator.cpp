#include "WindowActuator.h"

// % Driver

Driver::Driver(uint8_t dirPin, uint8_t stepPin, unsigned revolutionSteps)
  : _driver(AccelStepper::MotorInterfaceType::DRIVER, stepPin, dirPin)
{
  _revolutionSteps = revolutionSteps;
}

Driver::Driver(uint8_t dirPin, uint8_t stepPin, uint8_t sleepPin, unsigned revolutionSteps)
  : Driver(dirPin, stepPin, revolutionSteps)
{
  _sleepPin = sleepPin;
  pinMode(_sleepPin, OUTPUT);
  digitalWrite(_sleepPin, LOW);
}

Driver::Driver(uint8_t dirPin, uint8_t stepPin, uint8_t sleepPin, uint8_t enablePin,
  uint8_t resetPin, unsigned revolutionSteps)
    : Driver(dirPin, stepPin, revolutionSteps)
{
  _enablePin = enablePin;
  _resetPin = resetPin;
  _sleepPin = sleepPin;

  if(_sleepPin != 0xFF)
  {
    pinMode(_sleepPin, OUTPUT);
    digitalWrite(_sleepPin, LOW);
  }
}


void Driver::enable()
{
  _driver.enableOutputs();
  _driver.setPinsInverted(false,false,false);
  _driver.setEnablePin(_enablePin);

  if(_resetPin != 0xFF)
  {
    pinMode(_resetPin, OUTPUT);
    digitalWrite(_resetPin, HIGH); 
  }

  if(_sleepPin != 0xFF)
  {
    digitalWrite(_sleepPin, HIGH);
    delay(2);
  }
}


void Driver::disable()
{
  if(_resetPin != 0xFF)
    digitalWrite(_resetPin, LOW);
  if(_sleepPin != 0xFF)
    digitalWrite(_sleepPin, LOW);
  
  _driver.disableOutputs();
}


void Driver::setDegree()
{
  _unit = UNIT_DEGREE;
}

void Driver::setRadian()
{
  _unit = UNIT_RADIAN;
}

bool Driver::getUnit()
{
  return _unit;
}


void Driver::setMaxSpeed(float speed)
{
  if(_unit == UNIT_DEGREE)
    return _driver.setMaxSpeed(_revolutionSteps/360.0 * speed);
  else
    return _driver.setMaxSpeed(_revolutionSteps/(2*PI) * speed);
}


void Driver::setAcceleration(float acc)
{
  _acceleration = abs(acc);

  if(_unit == UNIT_DEGREE)
    return _driver.setAcceleration(_revolutionSteps/360.0 * _acceleration);
  else
    return _driver.setAcceleration(_revolutionSteps/(2*PI) * _acceleration);
}


float Driver::getMaxSpeed()
{
  return _driver.maxSpeed();
}


float Driver::getAcceleration()
{
  return _acceleration;
}


void Driver::setRevolutionSteps(unsigned revSteps)
{
  _revolutionSteps = revSteps;
}

unsigned Driver::getRevolutionSteps()
{
  return _revolutionSteps;
}


void Driver::setSleepPin(uint8_t pin)
{
  _sleepPin = pin;
}

void Driver::setEnablePin(uint8_t pin)
{
  _enablePin = pin;
}

void Driver::setResetPin(uint8_t pin)
{
  _resetPin = pin;
}

uint8_t Driver::getSleepPin()
{
  return _sleepPin;
}

uint8_t Driver::getEnablePin()
{
  return _enablePin;
}

uint8_t Driver::getResetPin()
{
  return _resetPin;
}



void Driver::rotate(float angle)
{
  if(_unit == UNIT_DEGREE)
    return _driver.move(_revolutionSteps/360.0 * angle);
  else
    return _driver.move(_revolutionSteps/(2.0*PI) * angle);
}


bool Driver::blockingRun()  // Blocking
{
  return _driver.runSpeedToPosition();
}



bool Driver::run()
{
  return _driver.run();
}

bool Driver::isRunning()
{
  return _driver.isRunning();
}


void Driver::stop()
{
  return _driver.stop();
}



// % WindowActuator

WindowActuator::WindowActuator(uint8_t dirPin, uint8_t stepPin, unsigned revolutionSteps)
  : Driver(dirPin, stepPin, revolutionSteps)
{

}

WindowActuator::WindowActuator(uint8_t dirPin, uint8_t stepPin, 
  uint8_t sleepPin, unsigned revolutionSteps)
  : Driver(dirPin, stepPin, sleepPin, revolutionSteps)
{

}


void WindowActuator::setRadius(float radius)
{
  _radius = abs(radius);
}

float WindowActuator::getRadius()
{
  return _radius;
}


void WindowActuator::move(float distance)
{
  if(this->getUnit() == UNIT_DEGREE)
  {
    this->setRadian();
    this->rotate(distance*1.0/_radius);
    this->setDegree();
  }
  else
    return this->rotate(distance*1.0/_radius);
}
