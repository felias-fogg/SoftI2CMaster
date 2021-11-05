// -*- c++ -*-
// Simple sketch to read out one register of an I2C device

#define I2C_TIMEOUT 1000
#define I2C_PULLUP 1

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

#include <SoftWire.h>

#define I2C_7BITADDR 0x68 // DS1307
#define MEMLOC 0x0A
#define ADDRLEN 1

SoftWire Wire = SoftWire();

void setup(void) {
  Serial.begin(57600);
  Wire.begin();
}

void loop(void){
  Wire.beginTransmission(I2C_7BITADDR);
  for (byte i=1; i<ADDRLEN; i++) Wire.write(0x00);
  Wire.write(MEMLOC);
  Wire.endTransmission(false);
  Wire.requestFrom(I2C_7BITADDR,1);
  byte val = Wire.read();
  Serial.println(val);
  delay(1000);
}
