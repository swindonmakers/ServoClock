#include "TLC5940ServoController.h"

void TLC5940ServoController::init(uint8_t pinSIN, uint8_t pinSCLK, uint8_t pinXLAT) {
	tlc.init(pinSIN, pinSCLK, pinXLAT);
}

void TLC5940ServoController::setServo(uint8_t num, int pos) {
	tlc.setServo(num, pos);
}

void TLC5940ServoController::updateServos(){
	tlc.update();
}
