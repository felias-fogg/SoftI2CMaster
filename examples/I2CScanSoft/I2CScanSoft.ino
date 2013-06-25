// Scan I2C bus for device responses

#define SDA_PORT PORTD
#define SDA_PIN 3
#define SCL_PORT PORTD
#define SCL_PIN 5
// #define I2C_FASTMODE 1


/* Corresponds to A4/A5 - the hardware I2C pins on Arduinos
#define SDA_PORT PORTC
#define SDA_PIN 4
#define SCL_PORT PORTC
#define SCL_PIN 5
#define I2C_FASTMODE 1
*/

#include <SoftI2C.h>


//------------------------------------------------------------------------------
void setup(void) {

  Serial.begin(19200);
  Serial.print("I2C delay counter: ");
  Serial.println(I2C_DELAY_COUNTER);
  Serial.println("Scanning ...");
  i2c_init();
  
  uint8_t add = 0;

  Serial.println("       8-bit 7-bit addr");
  // try read
  do {
    if (i2c_start(add | I2C_READ)) {
      i2c_read(true);
      i2c_stop();
      Serial.print("Read:   0x");
      if (add < 0x0F) Serial.print(0, HEX);
      Serial.print(add, HEX);
      Serial.print("  0x");
      if (add>>1 < 0x0F) Serial.print(0, HEX);
      Serial.println(add>>1, HEX);
    } else i2c_stop();
    add += 2;
  } while (add);

  // try write
  add = 0;
  do {
    if (i2c_start(add | I2C_WRITE)) {
      i2c_stop();
      Serial.print("Write:  0x");    
      if (add < 0x0F) Serial.print(0, HEX);  
      Serial.print(add, HEX);
      Serial.print("  0x");
      if (add>>1 < 0x0F) Serial.print(0, HEX);
      Serial.println(add>>1, HEX);
    } else i2c_stop();
    i2c_stop();
    add += 2;
  } while (add);

  Serial.println("Done");
}
void loop(void){}
