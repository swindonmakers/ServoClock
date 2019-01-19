#ifndef TLC5940ServoController_h
#define TLC5940ServoController_h

#include <Arduino.h>
#include <Tiny5940.h>
#include <ServoController.h>

class TLC5940ServoController : public ServoController {

public:
	void init(uint8_t pinSIN, uint8_t pinSCLK, uint8_t pinXLAT);
	void setServo(uint8_t num, int pos);
	void updateServos();

private:
	Tiny5940 tlc;
};

#endif