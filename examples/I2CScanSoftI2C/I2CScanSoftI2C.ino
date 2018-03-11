// -*-c++-*-
// Scan I2C bus for device responses


#define I2C_TIMEOUT 0
#define I2C_NOINTERRUPT 0
#define I2C_FASTMODE 0
#define FAC 1
#define I2C_CPUFREQ (F_CPU/FAC)

/* Corresponds to A4/A5 - the hardware I2C pins on Arduinos */
/* Adjust to your own liking */
#define SDA_PORT PORTC
#define SDA_PIN 4
#define SCL_PORT PORTC
#define SCL_PIN 5
#define I2C_FASTMODE 0


#include <SoftI2CMaster.h>
#include <avr/io.h>

//------------------------------------------------------------------------------
void CPUSlowDown(int fac) {
  // slow down processor by a fac
    CLKPR = _BV(CLKPCE);
    CLKPR = _BV(CLKPS1) | _BV(CLKPS0);
}
  


void setup(void) {
#if FAC != 1
  CPUSlowDown(FAC);
#endif

  Serial.begin(57600); // change baudrate to 2400 on terminal when low CPU freq!
  Serial.println(F("Intializing ..."));
  Serial.print("I2C delay counter: ");
  Serial.println(I2C_DELAY_COUNTER);
  if (!i2c_init()) 
    Serial.println(F("Initialization error. SDA or SCL are low"));
  else
    Serial.println(F("...done"));
}

void loop(void)
{
  uint8_t add = 0;
  int found = false;
  Serial.println("Scanning ...");

  Serial.println("       8-bit 7-bit addr");
  // try read
  do {
    delay(100);
    if (i2c_start(add | I2C_READ)) {
      found = true;
      i2c_read(true);
      i2c_stop();
      Serial.print("Read:   0x");
      if (add < 0x0F) Serial.print(0, HEX);
      Serial.print(add+I2C_READ, HEX);
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
      found = true;
      i2c_stop();
      Serial.print("Write:  0x");    
      if (add < 0x0F) Serial.print(0, HEX);  
      Serial.print(add+I2C_WRITE, HEX);
      Serial.print("  0x");
      if (add>>1 < 0x0F) Serial.print(0, HEX);
      Serial.println(add>>1, HEX);
    } else i2c_stop();
    i2c_stop();
    add += 2;
  } while (add);
  if (!found) Serial.println(F("No I2C device found."));
  Serial.println("Done\n\n");
  delay(1000/FAC);
}
