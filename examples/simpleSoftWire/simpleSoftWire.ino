// -*- c++ -*-
// Simple sketch to read out one register of an I2C device

#define SDA_PORT PORTC
#define SDA_PIN 4 // = A4
#define SCL_PORT PORTC
#define SCL_PIN 5 // = A5
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
