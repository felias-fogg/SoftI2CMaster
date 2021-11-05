// -*- c++ -*-
/* Write MEMLEN bytes of EEPROM and then read it back.
 * If successful, light a LED with slow blinks, otherwise blink very hecticly.
 * The baseline uses almost empty functions, the other
 * implementations are measured against it
 */

#define MEMADDR7B 0x57 // 7-bit addr of memory chip
#define ADDRLEN 2 // length of internal mem addr
#define MEMLEN 10 // the number of bytes to be written and to be read
#define LEDPIN 1 // LED to report result

#include "SlowSoftI2CMaster.h"

SlowSoftI2CMaster si = SlowSoftI2CMaster(4, 5, true);

void setup() {
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  delay(1000);
  digitalWrite(LEDPIN, LOW);
  delay(1000);
  if (!si.i2c_init()) error();
}

void loop() {
  byte i;
  if (!si.i2c_start(MEMADDR7B<<1 | I2C_WRITE)) error();
  for (i=0; i < ADDRLEN; i++) 
    if (!si.i2c_write(0)) error();
  for (i=0; i<MEMLEN; i++)
    if (!si.i2c_write(0xA1));
  si.i2c_stop();
  if (!si.i2c_start_wait((MEMADDR7B<<1 | I2C_WRITE))) error();
  for (i=0; i < ADDRLEN; i++) 
    if (!si.i2c_write(0)) error();
  si.i2c_stop();
  if (!si.i2c_rep_start((MEMADDR7B<<1 | I2C_READ))) error();
  for (i=0; i < MEMLEN-1; i++) 
    if (si.i2c_read(false) != 0xA1) error();
  if (si.i2c_read(true) != 0xA1) error();
  si.i2c_stop();
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
    
