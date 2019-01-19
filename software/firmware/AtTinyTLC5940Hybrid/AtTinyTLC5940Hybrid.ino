/*
 * Chip AtTiny84A @ 8MHz - build via Arduino IDE
 * 
 * Pins:
 *  2 - Blank Out
 *  3 - XLAT Out
 *  5 - GSCLK Out
 *  12 - Latch LED
 *  13 - XLAT IN (with external 10K pull up resistor)
 * 
 * This code acts as a helper signal generator for using a TLC5940 
 * "led" driver chip to run servos.  It generates the GSCLK pulse
 * and Blank and XLAT pulses.
 * 
 * Data is clocked into the TLS5940 by another microcontroller, eg
 * in the case of the Hexaloop, a NodeMCU ESP8266 chip.
 * 
 * The "other" microcontroller then sends the XLAT pulse to this ATTiny
 * which passes it on the the TLC5940 and fires the BLANK signal at the 
 * right time.
 * 
 * The GSCLK signal is started after the first XLAT pulse so that 
 * the "other" microcontroller has chance to clock in an inital set
 * of servo positions.
 * 
 * This code is heavily based on code found on the internet for 
 * running LEDs on the TLC5940 from an ATTiny.  (hence a lot is commented)
 * 
 * Note also that clocking in new servo positions too frequently causes
 * servo jitter, so you should update the servo positions no more than
 * every 25ms or so.
 * 
 * Robert Longbottom, 2015
 */

#include <avr/io.h>
#include <avr/interrupt.h>

/*------------------------------------------------------------------------
 User configurable values
------------------------------------------------------------------------*/

//#ifndef NUM_TLC5940s
//#define NUM_TLC5940s 2
//#endif


/*------------------------------------------------------------------------
 Standard data types
------------------------------------------------------------------------*/
#include <stdint.h>
typedef uint8_t byte;
typedef int16_t int16;
typedef uint16_t uint16;

/*------------------------------------------------------------------------
 Servo settings
------------------------------------------------------------------------*/
/** The maximum angle of the servo. */
//#define SERVO_MAX_ANGLE     180
/** The 1ms pulse width for zero degrees (0 - 4095). */
//#define SERVO_MIN_WIDTH     140
/** The 2ms pulse width for 180 degrees (0 - 4095). */
//#define SERVO_MAX_WIDTH     490

/*------------------------------------------------------------------------
 Speed of timers
------------------------------------------------------------------------*/
// Timer 0 flips OC0A every (GSCLK_TIM+1) clock cycles
// With a setting of "1" we get a complete clock pulse every 4 CPU cycles
// ie. A 2mHz GSCLK at 8mHz CPU speed
//#define GSCLK_TIM 1
#define GSCLK_TIM 18 // RL, trial and error with an oscilloscope for servo control

// How many CPU cycles in a complete GSCLK pulse
#define GSCLK_CYCLES (2*(GSCLK_TIM+1))

/*------------------------------------------------------------------------
 Chip-specific things

 Most of the pin definitions can't be changed
------------------------------------------------------------------------*/

#if defined(__AVR_ATtiny84__)

// Pin definitions for ATtiny84
#define BLANK_PIN PB0
#define XLAT_PIN PB1
#define GSCLK_PIN PB2      /* Must be OC0A */
#define PIN_LED 1
//#define SERIAL_CLK PA4
//#define SERIAL_OUT PA5
//#define SERIAL_DDR DDRA    /* Where the serial direction bits are */
//#define SERIAL_PORT PORTA  /* Where the serial output pins are */

void initHardwareTimers()
{
 // Timer 0 -> 2MHz GSCLK output on OC0A
 TCCR0A=0; TCCR0B=0;   // Stop the timer
 TIMSK0 = 0;           // No interrupts from this timer
 TCNT0 = 0;            // Counter starts at 0
 OCR0A = GSCLK_TIM;    // Timer restart value
 TCCR0A = _BV(WGM01)   // CTC mode
         |_BV(COM0A0); // Toggle OC0A output on every timer restart
 TCCR0B = _BV(CS00);   // Start timer, no prescale

 // Timer 1 -> Generate an interrupt every 4096 GSCLK pulses
 TCCR1A=0; TCCR1B = 0; // Stop the timer
 TCNT1 = 0;            // Counter starts at 0
 OCR1A = 0;            // Where the interrupt happens during the count
 ICR1 = GSCLK_CYCLES*4; // There's four clock cycles for every CSCLK and 4096 per PWM cycle
 TIMSK1 = _BV(OCIE1A); // Interrupt every time the timer passes zero
 TCCR1A = 0;           // No outputs on chip pins
 TCCR1C = 0;           // Just in case...
 TCCR1B = _BV(WGM12)   // Start timer, CTC mode
         |_BV(WGM13)   // Start timer, CTC mode
         |_BV(CS12)    // div1024 prescale
         |_BV(CS10);   // div1024 prescale
}

#elif defined (__AVR_ATtiny85__)

// Pin definitions for ATtiny85
#define BLANK_PIN PB3
#define XLAT_PIN PB4
#define GSCLK_PIN PB0      /* Must be OC0A */
//#define SERIAL_CLK PB2
//#define SERIAL_OUT PB1
//#define SERIAL_DDR DDRB    /* Where the serial direction bits are */
//#define SERIAL_PORT PORTB  /* Where the serial output pins are */

void initHardwareTimers()
{
 // Timer 0 -> 2MHz GSCLK output on OC0A
 TCCR0A=0; TCCR0B=0;   // Stop the timer
 TIMSK = 0;            // No interrupts from this timer
 TCNT0 = 0;            // Counter starts at 0
 OCR0A = GSCLK_TIM;    // Timer restart value
 TCCR0A = _BV(WGM01)   // CTC mode
         |_BV(COM0A0); // Toggle OC0A output on every timer restart
 TCCR0B = _BV(CS00);   // Start timer, no prescale

 // Timer 1 -> Generate an interrupt every 4096 GSCLK pulses
 TCCR1=0;  GTCCR=0;    // Stop the timer
 TCNT1 = 0;            // Counter starts at 0
 OCR1A = 0;            // Where the interrupt happens during the count
 OCR1C = GSCLK_CYCLES; // Prescaler is 4096...total clocks is 4096*GSCLK_CYCLES
 TIMSK = _BV(OCIE1A);  // Interrupt every time the timer passes zero
 TCCR1 = _BV(CTC1)     // Start timer, CTC mode
         |_BV(CS10)    // Prescale=4096
         |_BV(CS12)
         |_BV(CS13);
}

#else
 unsupported_chip;
#endif

/*------------------------------------------------------------------------
 Interrupt handler - BLANK and XLAT are sent here
------------------------------------------------------------------------*/
// Set this flag whenever you want an XLAT bit to be sent
static bool doXLAT = false;

// Interrupt called at the end of each PWM cycle  - do BLANK/XLAT here
ISR(TIM1_COMPA_vect) // nb. *NOT*   "TIMER1_COMPA_vect"
{
 if (doXLAT) {
   PORTB |= _BV(BLANK_PIN)|_BV(XLAT_PIN);
   doXLAT = false;
   PINB = _BV(XLAT_PIN);      // XLAT low
 }
 else {
   PORTB |= _BV(BLANK_PIN);
 }
 PINB = _BV(BLANK_PIN);       // BLANK low
}

/*------------------------------------------------------------------------
 The TLC5940 controller
------------------------------------------------------------------------*/
class Tiny5940 {
/*
 typedef byte pwm_index_type;
 byte pwmData[24*NUM_TLC5940s];
 void sendByte(byte b) {
   // Put the byte in the USI data register
   USIDR = b;
   // Send it
#define tick USICR = _BV(USIWM0)|_BV(USITC)
#define tock USICR = _BV(USIWM0)|_BV(USITC)|_BV(USICLK);
   tick;  tock;  tick;  tock;
   tick;  tock;  tick;  tock;
   tick;  tock;  tick;  tock;
   tick;  tock;  tick;  tock;
 }
 */
public:
 void init() {
   cli();            // The system may already have interrupts running on the timers, disable them
   DDRB |= _BV(BLANK_PIN)|_BV(XLAT_PIN)|_BV(GSCLK_PIN);  // All port B pins as outputs
   PORTB |= _BV(BLANK_PIN);                              // BLANK pin high (stop the TLC5940, disable all output)
   initHardwareTimers();
   //setAll(0);
   PINB = _BV(XLAT_PIN);
   PINB = _BV(XLAT_PIN);
   //update();
   sei();
 }
 /*
 void setAll(uint16 value) {
   for (pwm_index_type i=0; i<16*NUM_TLC5940s; ++i) {
     set(i,value);
   }
 }
 void set(pwm_index_type channel, uint16 value) {
   if (channel < (16*NUM_TLC5940s)) {
     // Pointer to a pair of packed PWM values
     byte *pwmPair = pwmData+((channel>>1)*3);
     if (channel&1) {
       // Replace high part of pair
       pwmPair[1] = (pwmPair[1]&0x0f) | ((value<<4)&0xf0);
       pwmPair[2] = value >> 4;
     }
     else {
       // Replace low part of pair
       pwmPair[0] = value&0xff;
       pwmPair[1] = (pwmPair[1]&0xf0) | ((value>>8)&0x0f);
     }
   }
 }
*/
/** Converts and angle (0 - SERVO_MAX_ANGLE) to the inverted tlc channel value
    (4095 - 0). */
/*    
uint16_t angleToVal(uint8_t angle)
{
    return 4095 - SERVO_MIN_WIDTH - (
            ((uint16_t)(angle) * (uint16_t)(SERVO_MAX_WIDTH - SERVO_MIN_WIDTH))
            / SERVO_MAX_ANGLE);
}
 */
 /** Sets a servo on channel to angle.
    \param channel which channel to set
    \param angle (0 - SERVO_MAX_ANGLE) */
    /*
void setServo(byte channel, uint8_t angle)
{
    set(channel, angleToVal(angle));
}

void setAllServo(uint8_t angle)
{
  setAll(angleToVal(angle));
}

 
 bool update() {
   // Still busy sending the previous values?
   if (doXLAT) {
     // Yes, return immediately
     return false;
   }

   // Set up USI to send the PWMdata
   byte sOut = _BV(SERIAL_CLK)|_BV(SERIAL_OUT);
   SERIAL_DDR |= sOut;     // Pins are outputs
   SERIAL_PORT &= ~sOut;   // Outputs start low
   USICR = _BV(USIWM0);    // Three wire mode

   // Send it...
   pwm_index_type numBytes = sizeof(pwmData);
   byte *pwm = pwmData+numBytes;   // High byte first...
   while (numBytes > 0) {
     sendByte(*--pwm);
     --numBytes;
   }

   // Tell the interrupt routine there's data to be latched
   doXLAT = true;

   return true;
 }
*/
};


// Create a Tiny5940 for people to use
Tiny5940 tlc;


volatile bool active = false;
volatile bool initialized = false;


ISR (PCINT0_vect)
{
  if ((PINA & (1 << PA0)) == 1)
  {
    // Low to high change
  }
  else
  {
    // High to low change
    // trigger XLAT to latch in current data (sent by another device)
    doXLAT = true;

    // activate timers to start sending pulses
    active = true;
  }
}

void setup() {

  // use PCINT0 to trigger XLAT and initial start
  DDRA &= ~(1 << PA0); // PA0 as input
  PORTA |= (1 << PORTA0); // Turn on pull up
  GIMSK |= (1 << PCIE0); // enable interrupts on PCINT0
  PCMSK0 = 0; // disable all PCINTS
  PCMSK0 |= (1 << PCINT0); // just enable interrupts on PCINT0 = PA0

  // Active Led
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // Interrupts on
  sei();
}

void loop() {
  if (active) {
    if (!initialized) {
      tlc.init();
      initialized = true;
    }
    if (doXLAT) {
      digitalWrite(PIN_LED, HIGH);
    } else {
      digitalWrite(PIN_LED, LOW);
    }
  }
}
