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
#define LEDPIN 1 // LEd to report result

#include <TinyWire.h>

void setup() {
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  delay(1000);
  digitalWrite(LEDPIN, LOW);
  delay(1000);
  TinyWire.begin();
}

void loop() {
  byte i;
  // writing 10 bytes
  TinyWire.beginTransmission(MEMADDR7B);
  for (i=0; i < ADDRLEN; i++) 
    TinyWire.send(0);
  for (i=0; i<MEMLEN; i++)
    TinyWire.send(DATA);
  if (TinyWire.endTransmission() != 0) error();

  // setting register addres (waiting for an ACK)
  
  while (true) {
    TinyWire.beginTransmission(MEMADDR7B);
    for (i=0; i < ADDRLEN; i++) 
      TinyWire.send(0);
    if (TinyWire.endTransmission() == 0) break;
  }
  if (TinyWire.requestFrom(MEMADDR7B, MEMLEN) != MEMLEN) error();
  i = 0;
  while (TinyWire.available()) {
    if (TinyWire.read() != DATA) error();
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
    
