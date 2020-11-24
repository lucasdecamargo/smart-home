#ifndef WINDOW_ACTUATOR_H
#define WINDOW_ACTUATOR_H

#include <Arduino.h>
#include <AccelStepper.h>

class Driver
{
public:

	const static bool UNIT_DEGREE = 0;
	const static bool UNIT_RADIAN = 1;
	

	Driver(uint8_t dirPin, uint8_t stepPin, unsigned revolutionSteps = 200);
	Driver(uint8_t dirPin, uint8_t stepPin, uint8_t sleepPin, unsigned revolutionSteps = 200);
	Driver(uint8_t dirPin, uint8_t stepPin, uint8_t sleepPin, uint8_t enablePin,
			uint8_t resetPin, unsigned revolutionSteps = 200);

	// Enable pins and turn on the driver if enablePin is set.
	void enable();
	// Disable pin, disables driver and puts it to sleep if pins are set.
	void disable();

	void setDegree();
	void setRadian();
	bool getUnit();

	void setMaxSpeed(float speed);
	void setAcceleration(float acc);
	float getMaxSpeed();
	float getAcceleration();

	void setRevolutionSteps(unsigned revSteps);
	unsigned getRevolutionSteps();

	void setSleepPin(uint8_t pin);
	void setEnablePin(uint8_t pin);
	void setResetPin(uint8_t pin);
	uint8_t getSleepPin();
	uint8_t getEnablePin();
	uint8_t getResetPin();

	void rotate(float angle);

	/// Poll the motor and step it if a step is due, implementing
  /// accelerations and decelerations to acheive the target position. You must call this as
  /// frequently as possible, but at least once per minimum step time interval,
  /// preferably in your main loop. Note that each call to run() will make at most one step, and then only when a step is due,
  /// based on the current speed and the time since the last step.
  /// \return true if the motor is still running to the target position.
	bool run();
	/// Checks to see if the motor is currently running to a target
  /// \return true if the speed is not zero or not at the target position
	bool isRunning();
	bool blockingRun(); 	// Blocking!

	void stop();

private:
	AccelStepper _driver;
	unsigned _revolutionSteps;
	uint8_t _enablePin = 0xFF;
	uint8_t _resetPin = 0xFF;
	uint8_t _sleepPin = 0xFF;
	bool _unit = UNIT_DEGREE; 			// false = Degree, true = Radian
	float _acceleration = 1.8;
};


class WindowActuator : public Driver
{
public:
	WindowActuator(uint8_t dirPin, uint8_t stepPin, unsigned revolutionSteps = 200);
	WindowActuator(uint8_t dirPin, uint8_t stepPin, uint8_t sleepPin, unsigned revolutionSteps = 200);

	void setRadius(float radius); // radius given in mm
	float getRadius();

	void move(float distance); // distance given in mm

private:
	float _radius = 20;

};

#endif