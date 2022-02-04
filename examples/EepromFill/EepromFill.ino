// Sketch to fill up EEPROM, sent from the Perl script sendfiles.pl
// Sendbytes sends whole files with the size always as the first number
// Receivebytes stores these chunks in EEPROM and stores in the first
// 256 bytes a table of 64 (unsigned long) addresses to these chunks.

#ifdef __AVR_ATmega328P__
/* Corresponds to A4/A5 - the hardware I2C pins on Arduinos */
#define SDA_PORT PORTC
#define SDA_PIN 4
#define SCL_PORT PORTC
#define SCL_PIN 5
#define I2C_FASTMODE 1
#else
#define SDA_PORT PORTB
#define SDA_PIN 0
#define SCL_PORT PORTB
#define SCL_PIN 2
#define I2C_FASTMODE 1
#endif

#define I2C_FASTMODE 1
// #define I2C_TIMEOUT 10 // timeout after 10 msec
// #define I1C_NOINTERRUPT 1 // no interrupts
// #define I2C_CPUFREQ (F_CPU/8) // slow down CPU frequency
#include <SoftI2CMaster.h>

#define EEPROMADDR 0xA6 // set by jumper (A1=1 and A0=1)
#define MAXADDR 0x1FFFF

long unsigned startaddrs[64];
long unsigned addr = 0;
boolean newpage = true;

boolean writeEEPROM(uint8_t byte) {
  if (newpage) {
    // issue a start condition, send device address and write direction bit
    i2c_start_wait(EEPROMADDR | I2C_WRITE | (addr&0x10000 ? 8 : 0));
    newpage = false;
    // send the address
    if (!i2c_write((addr>>8)&0xFF)) return false;
    if (!i2c_write(addr&0xFF)) return false;
  }
  // send data to EEPROM
  if (!i2c_write(byte)) return false;
  addr++;

  if (addr%128 == 0) {
    // if add points to new page, save the old one!
    finishEEPage();
  }
  return true;
}

void finishEEPage(void)
{
  i2c_stop();
  newpage = true;
}

int convHexDigit(char c) 
{
  if (c >= '0' && c <= '9') return (c - '0');
  else if (c >= 'A' && c <= 'F') return(c - 'A' + 10);
  else return (-99);
}

int convHex(char c1, char c2)
{
  int i1 = convHexDigit(c1);
  int i2 = convHexDigit(c2);
  if (i1 < 0 || i2 < 0) return -99;
  else return (i1*16+i2);
}


int readNextByte(void)
{
  char c1 = '\0';
  char c2;
  while (c1 == '\0') {
    while (!Serial.available()) { };
    c1 = Serial.read();
    if (c1 <= ' ') c1 = '\0';
  }
  while (!Serial.available()) { };
  c2 = Serial.read();
  if (c1 == 'X' && c2 == 'X') return -1;
  if (c1 == 'Z' && c2 == 'Z') return -2;

  return convHex(c1,c2);
}



//------------------------------------------------------------------------------

void setup(void)
{
  Serial.begin(19200);
  Serial.println("XXX");
}

void loop(void)
{
  boolean ready = false;
  int byte;
  int chunk = 0;
  int counter;
  
  addr=0;
  for (int i=0; i< 256; i++) writeEEPROM(0xFF);
  Serial.setTimeout(5000);
  startaddrs[chunk++] = addr;
  counter = 0;
  while (!ready) {
    byte = readNextByte();
    if (byte < 0) {
      switch (byte) {
      case -99:
	Serial.print("Num error\n"); 
	while (1) { };
	break;
      case -1:
	finishEEPage();
	Serial.println("ZZZ");
	startaddrs[chunk++] = addr;
	counter = 0;
	break;
      case -2:
	Serial.println("ZZ seen");
	ready = true;
	break;
      }
    } else {
      writeEEPROM((uint8_t)byte);
      counter++;
      if (counter%256 == 0) {
	Serial.println("PAGE");
	counter = 0;
      }
    }
  }
  pinMode(13,OUTPUT);
  digitalWrite(13,HIGH);
  delay(1000);
  digitalWrite(13,LOW);  
  finishEEPage();
  addr = 0; // write start addr table
  for (chunk = 0; chunk < 64; chunk++) {
    writeEEPROM((uint8_t)(startaddrs[chunk]>>24)&0xFF);
    writeEEPROM((uint8_t)(startaddrs[chunk]>>16)&0xFF);
    writeEEPROM((uint8_t)(startaddrs[chunk]>>8)&0xFF);
    writeEEPROM((uint8_t)(startaddrs[chunk])&0xFF);
  }
  while (1) { };
}
