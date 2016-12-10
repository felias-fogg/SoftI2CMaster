// -*- c++ -*-
// Simple sketch to read out BMA020 using SoftI2C

// Readout BMA020 chip

// use low processor speed (you have to change the baud rate to 2400!) 
//#define I2C_CPUFREQ (F_CPU/16)
//#define NO_INTERRUPT 1
//#define I2C_FASTMODE 1
//#define I2C_SLOWMODE 1
//#define I2C_TIMEOUT 1000
#define TERMOUT 1

#define SDA_PORT PORTC
#define SDA_PIN 4
#define SCL_PORT PORTC
#define SCL_PIN 5
#include <SoftI2CMaster.h>
#include <avr/io.h>


#define BMAADDR 0x70
#define LEDPIN 13

int xval, yval, zval;

void CPUSlowDown(int fac) {
  // slow down processor by a fac
  switch(fac) {
  case 2:
    CLKPR = _BV(CLKPCE);
    CLKPR = _BV(CLKPS0);
    break;
  case 4:
    CLKPR = _BV(CLKPCE);
    CLKPR = _BV(CLKPS1);
    break;    
  case 8:
    CLKPR = _BV(CLKPCE);
    CLKPR = _BV(CLKPS1) | _BV(CLKPS0);
    break;
  case 16:
    CLKPR = _BV(CLKPCE);
    CLKPR = _BV(CLKPS2);
    break;
  }
}
  

boolean setControlBits(uint8_t cntr)
{
#ifdef TERMOUT
  Serial.println(F("Soft reset"));
#endif
  if (!i2c_start(BMAADDR | I2C_WRITE)) {
    return false;
  }
  if (!i2c_write(0x0A)) {
    return false;
  }
  if (!i2c_write(cntr)) {
    return false;
  }
  i2c_stop();
  return true;
}

boolean initBma(void)
{
  if (!setControlBits(B00000010)) return false;;
  delay(100);
  return true;
}

int readOneVal(boolean last)
{
  uint8_t msb, lsb;
  lsb = i2c_read(false);
  msb = i2c_read(last);
  if (last) i2c_stop();
  return (int)((msb<<8)|lsb)/64;
}

boolean readBma(void)
{
  xval = 0xFFFF;
  yval = 0xFFFF;
  zval = 0xFFFF;
  if (!i2c_start(BMAADDR | I2C_WRITE)) return false;
  if (!i2c_write(0x02)) return false;
  if (!i2c_rep_start(BMAADDR | I2C_READ)) return false;
  xval = readOneVal(false);
  yval = readOneVal(false);
  zval = readOneVal(true);
  return true;
}



//------------------------------------------------------------------------------
void setup(void) {
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  i2c_init();
#if I2C_CPUFREQ != F_CPU
  CPUSlowDown(F_CPU/I2C_CPUFREQ);
#endif
  Serial.begin(19200); // in case of CPU slow down, change to baud rate / FAC!
  if (!initBma()) {
#ifdef TERMOUT
    Serial.println(F("INIT ERROR"));
#else
    while (1);
#endif
  }

}

void loop(void){
  if (!readBma()) {
#ifdef TERMOUT
    Serial.println(F("READ ERROR"));
#else
    while (1);
#endif
  }
#ifdef TERMOUT
  Serial.print(F("X="));
  Serial.print(xval);
  Serial.print(F("  Y="));
  Serial.print(yval);
  Serial.print(F("  Z="));
  Serial.println(zval);
  delay(300);
#else
  digitalWrite(LEDPIN, HIGH);
  delay(500);
  digitalWrite(LEDPIN, LOW);
  delay(500);
  delay(5000);
#endif
}
