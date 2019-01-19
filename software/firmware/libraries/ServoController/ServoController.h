#ifndef ServoController_h
#define ServoController_h

#include <Arduino.h>

class ServoController {

public:
	// pos should be in the range 0..180
	virtual void setServo(uint8_t num, int pos) = 0;
	virtual void updateServos() = 0;

};

#endif