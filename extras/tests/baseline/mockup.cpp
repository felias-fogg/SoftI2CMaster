
#include <Arduino.h>
#include "mockup.h"

bool i2c_init(void)
{
  return true;
}

bool i2c_start(uint8_t addr)
{
  return true;
}

bool i2c_start_wait(uint8_t addr)
{
  return true;
}

bool i2c_rep_start(uint8_t addr)
{
  return true;
}

bool i2c_write(uint8_t data)
{
  return true;
}

byte i2c_read(bool last)
{
  return 0xA1;
}

void i2c_stop()
{
}
