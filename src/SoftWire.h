/*
  SoftWire.h - A Wire compatible wrapper for SoftI2CMaster
  Copyright (c) 2016 Bernhard Nebel.

  This file is part of SoftI2CMaster https://github.com/felias-fogg/SoftI2CMaster.

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

#ifndef _SoftWire_h
#define _SoftWire_h

#include <inttypes.h>
#include "Stream.h"

#ifndef I2C_BUFFER_LENGTH
  #define I2C_BUFFER_LENGTH 32
#endif

// WIRE_HAS_END means Wire has end()
#define WIRE_HAS_END 1

class SoftWire : public Stream // @suppress("Class has a virtual method and non-virtual destructor")
{
private:
  uint8_t rxBuffer[I2C_BUFFER_LENGTH];
  uint8_t rxBufferIndex;
  uint8_t rxBufferLength;
  uint8_t transmitting;
  uint8_t error;
public:
  SoftWire(void);

  void begin(void);
  void end(void);
  void setClock(uint32_t _);

  void beginTransmission(uint8_t address);

  void beginTransmission(int address);
  uint8_t endTransmission(uint8_t sendStop);

  //    This provides backwards compatibility with the original
  //    definition, and expected behaviour, of endTransmission
  //
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

//  using Print::write;
};

extern SoftWire Wire;

#if !defined(USE_SOFTWIRE_H_AS_PLAIN_INCLUDE)
/*
 * The implementation part of the header only library starts here
 */
#include <SoftI2CMaster.h>

  SoftWire::SoftWire(void) {
  }

  void SoftWire::begin(void) {
    rxBufferIndex = 0;
    rxBufferLength = 0;
    error = 0;
    transmitting = false;

    i2c_init();
  }

  void SoftWire::end(void) {
  }

  void SoftWire::setClock(uint32_t _) {
      (void) _;
  }

  void SoftWire::beginTransmission(uint8_t address) {
    if (transmitting) {
      error = (i2c_rep_start((address<<1)|I2C_WRITE) ? 0 : 2);
    } else {
      error = (i2c_start((address<<1)|I2C_WRITE) ? 0 : 2);
    }
    // indicate that we are transmitting
    transmitting = 1;
  }

  void SoftWire::beginTransmission(int address) {
    beginTransmission((uint8_t)address);
  }

  uint8_t SoftWire::endTransmission(uint8_t sendStop)
  {
    uint8_t transError = error;
    if (sendStop) {
      i2c_stop();
      transmitting = 0;
    }
    error = 0;
    return transError;
  }

  //	This provides backwards compatibility with the original
  //	definition, and expected behaviour, of endTransmission
  //
  uint8_t SoftWire::endTransmission(void)
  {
    return endTransmission(true);
  }

  size_t SoftWire::write(uint8_t data) {
    if (i2c_write(data)) {
      return 1;
    } else {
      if (error == 0) error = 3;
      return 0;
    }
  }

  size_t SoftWire::write(const uint8_t *data, size_t quantity) {
    size_t trans = 0;
    for(size_t i = 0; i < quantity; ++i){
      trans += write(data[i]);
    }
    return trans;
  }

  uint8_t SoftWire::requestFrom(uint8_t address, uint8_t quantity,
              uint32_t iaddress, uint8_t isize, uint8_t sendStop) {
    error = 0;
    uint8_t localerror = 0;
    if (isize > 0) {
      // send internal address; this mode allows sending a repeated start to access
      // some devices' internal registers. This function is executed by the hardware
      // TWI module on other processors (for example Due's TWI_IADR and TWI_MMR registers)
      beginTransmission(address);
      // the maximum size of internal address is 3 bytes
      if (isize > 3){
        isize = 3;
      }
      // write internal register address - most significant byte first
      while (isize-- > 0) {
        write((uint8_t)(iaddress >> (isize*8)));
      }
      endTransmission(false);
    }
    // clamp to buffer length
    if(quantity > I2C_BUFFER_LENGTH){
      quantity = I2C_BUFFER_LENGTH;
    }
    if (transmitting) {
      localerror = !i2c_rep_start((address<<1) | I2C_READ);
    } else {
      localerror = !i2c_start((address<<1) | I2C_READ);
    }
    if (error == 0 && localerror) error = 2;
    // perform blocking read into buffer
    for (uint8_t cnt=0; cnt < quantity; cnt++) {
      rxBuffer[cnt] = i2c_read(cnt == quantity-1);
    }
    // set rx buffer iterator vars
    rxBufferIndex = 0;
    rxBufferLength = error ? 0 : quantity;
    if (sendStop) {
      transmitting = 0;
      i2c_stop();
    }
    return rxBufferLength;
  }

  uint8_t SoftWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) {
	return requestFrom((uint8_t)address, (uint8_t)quantity, (uint32_t)0, (uint8_t)0, (uint8_t)sendStop);
  }

  uint8_t SoftWire::requestFrom(int address, int quantity, int sendStop) {
    return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)sendStop);
  }


  uint8_t SoftWire::requestFrom(uint8_t address, uint8_t quantity) {
    return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
  }

  uint8_t SoftWire::requestFrom(int address, int quantity) {
    return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
  }

  int SoftWire::available(void) {
    return rxBufferLength - rxBufferIndex;
  }

  int SoftWire::read(void) {
    int value = -1;
    if(rxBufferIndex < rxBufferLength){
      value = rxBuffer[rxBufferIndex];
      ++rxBufferIndex;
    }
    return value;
  }

  int SoftWire::peek(void) {
    int value = -1;

    if(rxBufferIndex < rxBufferLength){
      value = rxBuffer[rxBufferIndex];
    }
    return value;
  }

  void SoftWire::flush(void) {
  }


// Preinstantiate Objects //////////////////////////////////////////////////////

SoftWire Wire = SoftWire();

#endif // !defined(USE_SOFTWIRE_H_AS_PLAIN_INCLUDE)

#endif // #ifndef _SoftWire_h
#pragma once
