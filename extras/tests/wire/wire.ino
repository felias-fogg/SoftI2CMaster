// -*- c++ -*-
/* Write MEMLEN bytes of EEPROM and then read it back.
 * If successful, light a LED with slow blinks, otherwise blink very hecticly.
 * The baseline uses almost empty functions, the other
 * implementations are measured against it
 */

#define MEMADDR7B 0x57 // 7-bit addr of memory chip
#define DATA 0xA1 // can be changed
#define ADDRLEN 2 // length of internal mem addr
#define MEMLEN 10 // the number of bytes to be written and to be read
#define LEDPIN 13 // LEd to report result

#include <Wire.h>

void setup() {
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  delay(1000);
  digitalWrite(LEDPIN, LOW);
  delay(1000);
  Wire.begin();
}

void loop() {
  byte i;
  // writing 10 bytes
  Wire.beginTransmission(MEMADDR7B);
  for (i=0; i < ADDRLEN; i++) 
    Wire.write(0);
  for (i=0; i<MEMLEN; i++)
    Wire.write(DATA);
  if (Wire.endTransmission() != 0) error();

  // setting register addres (waiting for an ACK)
  
  while (true) {
    Wire.beginTransmission(MEMADDR7B);
    for (i=0; i < ADDRLEN; i++) 
      Wire.write(0);
    if (Wire.endTransmission() == 0) break;
  }
  if (Wire.requestFrom(MEMADDR7B, MEMLEN) != MEMLEN) error();
  i = 0;
  while (Wire.available()) {
    if (Wire.read() != DATA) error();
  }
  digitalWrite(LEDPIN, HIGH);
  delay(2000);
  digitalWrite(LEDPIN, LOW);
  delay(2000);
}

void error()
{
  while(true) {
    digitalWrite(LEDPIN, HIGH);
    delay(200);
    digitalWrite(LEDPIN, LOW);
    delay(200);
  }
}
    
