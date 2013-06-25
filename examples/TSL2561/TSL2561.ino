// Sketch to explore the luminosity sensor TSL2561 (breakout board by Adafruit)

#define SDA_PORT PORTD
#define SDA_PIN 3
#define SCL_PORT PORTD
#define SCL_PIN 5

#include <SoftI2C.h>

#define ADDR 0x72

//------------------------------------------------------------------------------
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
  Serial.print(low0+(high0<<8));
  Serial.print(F(" / chan1="));
  Serial.println(low1+(high1<<8));
}
