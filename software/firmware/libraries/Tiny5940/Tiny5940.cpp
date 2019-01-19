#include "Tiny5940.h"

void Tiny5940::init(uint8_t pinSIN, uint8_t pinSCLK, uint8_t pinXLAT)
{
	_pinSIN = pinSIN;
	_pinSCLK = pinSCLK;
	_pinXLAT = pinXLAT;

	pinMode(_pinXLAT, OUTPUT);
	digitalWrite(_pinXLAT, HIGH);
	tlc_shift8_init();
}

void Tiny5940::set(TLC_CHANNEL_TYPE channel, uint16_t value)
{
    TLC_CHANNEL_TYPE index8 = (NUM_TLCS * 16 - 1) - channel;
    uint8_t *index12p = tlc_GSData + ((((uint16_t)index8) * 3) >> 1);
    if (index8 & 1) { // starts in the middle
                      // first 4 bits intact | 4 top bits of value
        *index12p = (*index12p & 0xF0) | (value >> 8);
                      // 8 lower bits of value
        *(++index12p) = value & 0xFF;
    } else { // starts clean
                      // 8 upper bits of value
        *(index12p++) = value >> 4;
                      // 4 lower bits of value | last 4 bits intact
        *index12p = ((uint8_t)(value << 4)) | (*index12p & 0xF);
    }
}

void Tiny5940::setAll(uint16_t value)
{
    uint8_t firstByte = value >> 4;
    uint8_t secondByte = (value << 4) | (value >> 8);
    uint8_t *p = tlc_GSData;
    while (p < tlc_GSData + NUM_TLCS * 24) {
        *p++ = firstByte;
        *p++ = secondByte;
        *p++ = (uint8_t)value;
    }
}
 
void Tiny5940::setServo(byte channel, uint8_t angle)
{
    set(channel, angleToVal(angle));
}

void Tiny5940::setAllServo(uint8_t angle)
{
  setAll(angleToVal(angle));
}

/* 
 * Clock out the current servo settings and latch 
 * Takes approx 750us
 */

bool Tiny5940::update()
{
  // Limit update rate of servos or we end up latching in new
  // values before the old values are set
  if (millis() - lastUpdate < UPDATE_RATE_LIMIT)
    return false;

  // Clock out data
  pulse_pin(_pinSCLK);
  uint8_t *p = tlc_GSData;
  while (p < tlc_GSData + NUM_TLCS * 24) {
    tlc_shift8(*p++);
  }
  pulse_pin(_pinSCLK);
    
  // Pulse XLAT low to tell Attiny that it should
  // pulse XLAT at the end of the GSCLK cycle
  digitalWrite(_pinXLAT, LOW);
  delayMicroseconds(10); // small delay because ESP runs quicker than ATTiny
  digitalWrite(_pinXLAT, HIGH);

  lastUpdate = millis();

  return true;
}


/** Converts and angle (0 - SERVO_MAX_ANGLE) to the inverted tlc channel value
    (4095 - 0). */
uint16_t Tiny5940::angleToVal(uint8_t angle)
{
    return 4095 - SERVO_MIN_WIDTH - (
            ((uint16_t)(angle) * (uint16_t)(SERVO_MAX_WIDTH - SERVO_MIN_WIDTH))
            / SERVO_MAX_ANGLE);
}

void Tiny5940::tlc_shift8_init(void)
{
	pinMode(_pinSIN, OUTPUT);
	pinMode(_pinSCLK, OUTPUT);
	digitalWrite(_pinSCLK, LOW);
}

/** Shifts a byte out, MSB first */
void Tiny5940::tlc_shift8(uint8_t byte)
{
    for (uint8_t bit = 0x80; bit; bit >>= 1) {
        if (bit & byte) {
			digitalWrite(_pinSIN, HIGH);
        } else {
			digitalWrite(_pinSIN, LOW);
        }
        
		pulse_pin(_pinSCLK);
    }
}
