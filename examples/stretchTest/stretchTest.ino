// This is a short sketch that stretches the low pulse on an I2C bus
// in order to test the timeout feature.
// Put any Arduino and I2C device, e.g. a memory chip, as usual on a breadboard,
// then use another Arduino and flash this program into it. Connect the
// pin 5 (of PORTD) with the SCL line and verify on the scope that the 
// low period is indeed stretched.

#include <avr/io.h>

#define SCL_PORT PORTD
#define SCL_PIN 5

#define DELAY 8 // strech SCL low for that many milli seconds

#define SCL_DDR       	(_SFR_IO_ADDR(SCL_PORT) - 1)
#define SCL_OUT       	_SFR_IO_ADDR(SCL_PORT)
#define SCL_IN		(_SFR_IO_ADDR(SCL_PORT) - 2)

void initScl(void) {
  asm volatile 
  (" cbi      %[SCLDDR],%[SCLPIN]     ;release SCL \n\t" 
   " cbi      %[SCLOUT],%[SCLPIN]     ;clear SCL output value \n\t" 
   ::  [SCLDDR] "I"  (SCL_DDR), [SCLPIN] "I" (SCL_PIN), [SCLOUT] "I" (SCL_OUT));
}

void grabScl(void) {
  asm volatile 
    ("_L_wait: \n\t"
     " sbic 	%[SCLIN],%[SCLPIN] \n\t"
     " rjmp _L_wait \n\t"
     "  sbi	%[SCLDDR],%[SCLPIN]"
     ::[SCLDDR] "I"  (SCL_DDR), [SCLPIN] "I" (SCL_PIN), [SCLIN] "I" (SCL_IN));
}

void releaseScl(void) {
  asm volatile
    ("  cbi	%[SCLDDR],%[SCLPIN] \n\t"
     :: [SCLDDR] "I"  (SCL_DDR), [SCLPIN] "I" (SCL_PIN));
}

void setup(void)
{
  Serial.begin(19200);
  Serial.println("Intializing ...");
  initScl();
}

void loop(void) {
  grabScl();
  delay(DELAY);
  releaseScl();
}
