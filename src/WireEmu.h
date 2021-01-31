/*
  EmuWire.h - A Wire compatible wrapper for SoftI2CMaster
  Copyright (c) 2020 Bernhard Nebel.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _WireEmu_h
#define _WireEmu_h

#include <inttypes.h>
#include "Stream.h"

// buffer size
#define BUF_LEN 32

// constants for reading & writing
#define I2C_READ    1
#define I2C_WRITE   0


// WIRE_HAS_END means Wire has end()
#define WIRE_HAS_END 1

class TwoWire : public Stream
{
private:
  uint8_t rxBuffer[BUF_LEN];
  uint8_t rxBufferIndex;
  uint8_t rxBufferLength;
  uint8_t transmitting;
  uint8_t error;
public:
  TwoWire(void) {
  }

  void begin(void);
  void end(void);
  void setClock(uint32_t _);
  void beginTransmission(uint8_t address);
  void beginTransmission(int address);
  uint8_t endTransmission(uint8_t sendStop);
  uint8_t endTransmission(void);
  size_t write(uint8_t data);
  size_t write(const uint8_t *data, size_t quantity);
  uint8_t requestFrom(uint8_t address, uint8_t quantity,
		      uint32_t iaddress, uint8_t isize, uint8_t sendStop);
  uint8_t requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop);
  uint8_t requestFrom(int address, int quantity, int sendStop);
  uint8_t requestFrom(uint8_t address, uint8_t quantity);
  uint8_t requestFrom(int address, int quantity);
  int available(void);
  int read(void);
  int peek(void);
  void flush(void);
  inline size_t write(unsigned long n) { return write((uint8_t)n); }
  inline size_t write(long n) { return write((uint8_t)n); }
  inline size_t write(unsigned int n) { return write((uint8_t)n); }
  inline size_t write(int n) { return write((uint8_t)n); }
  using Print::write;
};

extern TwoWire Wire;
bool i2c_init(void);
bool i2c_start(uint8_t addr);
bool i2c_start_wait(uint8_t addr);
bool i2c_rep_start(uint8_t addr);
void i2c_stop(void) asm("ass_i2c_stop");
bool i2c_write(uint8_t value) asm("ass_i2c_write");
uint8_t i2c_read(bool last);

#endif
