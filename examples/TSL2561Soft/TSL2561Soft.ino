// Sketch to explore the luminosity sensor TSL2561 (breakout board by Adafruit)

#define SDA_PORT PORTD
#define SDA_PIN 3
#define SCL_PORT PORTD
#define SCL_PIN 5

#include <SoftI2CMaster.h>
#include "TSL2561Soft.h"

#define ADDR 0x72

//------------------------------------------------------------------------------
unsigned long computeLux(unsigned long channel0, unsigned long channel1){
  
  /* Make sure the sensor isn't saturated! */
  uint16_t clipThreshold = TSL2561_CLIPPING_402MS;;

  /* Return 0 lux if the sensor is saturated */
  if ((channel0 > clipThreshold) || (channel1 > clipThreshold))
  {
    Serial.println(F("Sensor is saturated"));
    return 32000;
  }

  /* Find the ratio of the channel values (Channel1/Channel0) */
  unsigned long ratio1 = 0;
  if (channel0 != 0) ratio1 = (channel1 << (TSL2561_LUX_RATIOSCALE+1)) / channel0;

  /* round the ratio value */
  unsigned long ratio = (ratio1 + 1) >> 1;

  unsigned int b, m;

  if ((ratio >= 0) && (ratio <= TSL2561_LUX_K1T))
    {b=TSL2561_LUX_B1T; m=TSL2561_LUX_M1T;}
  else if (ratio <= TSL2561_LUX_K2T)
    {b=TSL2561_LUX_B2T; m=TSL2561_LUX_M2T;}
  else if (ratio <= TSL2561_LUX_K3T)
    {b=TSL2561_LUX_B3T; m=TSL2561_LUX_M3T;}
  else if (ratio <= TSL2561_LUX_K4T)
    {b=TSL2561_LUX_B4T; m=TSL2561_LUX_M4T;}
  else if (ratio <= TSL2561_LUX_K5T)
    {b=TSL2561_LUX_B5T; m=TSL2561_LUX_M5T;}
  else if (ratio <= TSL2561_LUX_K6T)
    {b=TSL2561_LUX_B6T; m=TSL2561_LUX_M6T;}
  else if (ratio <= TSL2561_LUX_K7T)
    {b=TSL2561_LUX_B7T; m=TSL2561_LUX_M7T;}
  else if (ratio > TSL2561_LUX_K8T)
    {b=TSL2561_LUX_B8T; m=TSL2561_LUX_M8T;}

  unsigned long temp;
  temp = ((channel0 * b) - (channel1 * m));

  /* Do not allow negative lux value */
  if (temp < 0) temp = 0;

  /* Round lsb (2^(LUX_SCALE-1)) */
  temp += (1 << (TSL2561_LUX_LUXSCALE-1));

  /* Strip off fractional portion */
  uint32_t lux = temp >> TSL2561_LUX_LUXSCALE;

  return lux;
}

void setup(void) {

  Serial.begin(19200);
  Serial.println("Initializing ...");
  i2c_init();

  if (!i2c_start(ADDR | I2C_WRITE)) Serial.println(F("Device does not respond"));
  if (!i2c_write(0x80)) Serial.println(F("Cannot address reg 0"));
  if (!i2c_write(0x03)) Serial.println(F("Cannot wake up"));
  i2c_stop();
}  

void loop (void) {
  unsigned int low0, high0, low1, high1;
  unsigned int chan0, chan1;
  unsigned int lux;

  delay(1000);
  i2c_start(ADDR | I2C_WRITE);
  i2c_write(0x8C);
  i2c_rep_start(ADDR | I2C_READ);
  low0 = i2c_read(false);
  high0 = i2c_read(false);
  low1 = i2c_read(false);
  high1 = i2c_read(true);
  i2c_stop();
  Serial.print(F("Raw values: chan0="));
  Serial.print(chan0=(low0+(high0<<8)));
  Serial.print(F(" / chan1="));
  Serial.println(chan1=(low1+(high1<<8)));
  lux = computeLux(chan0,chan1);
  Serial.print(F("Lux value="));
  Serial.println(lux);
}
