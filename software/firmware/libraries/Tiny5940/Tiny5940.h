#ifndef Tiny5940_H
#define Tiny5940_H

#include <Arduino.h>

#define NUM_TLCS 2
#define TLC_CHANNEL_TYPE    uint8_t

/** The maximum angle of the servo. */
#define SERVO_MAX_ANGLE     180
/** The 1ms pulse width for zero degrees (0 - 4095). */
#define SERVO_MIN_WIDTH     60
/** The 2ms pulse width for 180 degrees (0 - 4095). */
#define SERVO_MAX_WIDTH     255

#define UPDATE_RATE_LIMIT 25

#define pulse_pin(pin) digitalWrite(pin, HIGH); digitalWrite(pin, LOW);

class Tiny5940 {
private:
	uint8_t _pinSIN = D0;
	uint8_t _pinSCLK = D1;
	uint8_t _pinXLAT = D2;
	uint8_t tlc_GSData[NUM_TLCS * 24];
	long lastUpdate = 0;
	uint16_t angleToVal(uint8_t angle);
	void tlc_shift8_init(void);
	void tlc_shift8(uint8_t byte);

public:
	void init(uint8_t pinSIN, uint8_t pinSCLK, uint8_t pinXLAT);
	void set(TLC_CHANNEL_TYPE channel, uint16_t value);
	void setAll(uint16_t value);
	void setServo(byte channel, uint8_t angle);
	void setAllServo(uint8_t angle);
	bool update();
};

#endif