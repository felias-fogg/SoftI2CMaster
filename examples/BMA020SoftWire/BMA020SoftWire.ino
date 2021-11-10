// -*- c++ -*-
// Simple sketch to read out BMA020 using SoftWire

// Readout BMA020 chip

#ifdef __AVR_ATmega328P__
/* Corresponds to A4/A5 - the hardware I2C pins on Arduinos */
#define SDA_PORT PORTC
#define SDA_PIN 4
#define SCL_PORT PORTC
#define SCL_PIN 5
#define I2C_FASTMODE 1
#else
#define SDA_PORT PORTB
#define SDA_PIN 0
#define SCL_PORT PORTB
#define SCL_PIN 2
#define I2C_FASTMODE 1
#endif

#define I2C_TIMEOUT 100
//#define I2C_FASTMODE 1

#include <SoftWire.h>
#include <avr/io.h>


SoftWire Wire = SoftWire();

#define BMAADDR 0x38

int xval, yval, zval;


boolean setControlBits(uint8_t cntr)
{
  Wire.beginTransmission(BMAADDR);
  Wire.write(0x0A);
  Wire.write(cntr);
  return (Wire.endTransmission() == 0);
}

boolean initBma(void)
{
  if (!setControlBits(B00000010)) return false;
  delay(100);
  return true;
}

int readOneVal(void)
{
  uint8_t msb, lsb;
  lsb = Wire.read();
  msb = Wire.read();
  return (int)((msb<<8)|lsb)/64;
}

boolean readBma(void)
{
  xval = 0xFFFF;
  yval = 0xFFFF;
  zval = 0xFFFF;
  Wire.beginTransmission(BMAADDR);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(BMAADDR, 6, true);
  xval = readOneVal();
  yval = readOneVal();
  zval = readOneVal();
  return true;
}



//------------------------------------------------------------------------------
void setup(void) {
  Serial.begin(19200);
  Wire.begin();
  if (!initBma()) {
    Serial.println(F("INIT ERROR"));
    while (1);
  }

}

void loop(void){
  if (!readBma()) {
    Serial.println(F("READ ERROR"));
    while (1);
  }
  Serial.print(F("X="));
  Serial.print(xval);
  Serial.print(F("  Y="));
  Serial.print(yval);
  Serial.print(F("  Z="));
  Serial.println(zval);
  delay(300);
}
