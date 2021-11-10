// Sketch to explore 24AA1024 using SoftI2C

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
#define I1C_NOINTERRUPT 1 // no interrupts
// #define I2C_CPUFREQ (F_CPU/8) // slow down CPU frequency
#include <SoftWire.h>

SoftWire Wire = SoftWire();

#define EEPROMADDR 0xA6 // set by jumper (A0 and A1 = High)
#define MAXADDR 0x1FFFF
#define MAXTESTADDR 0x003FF

void CPUSlowDown(void) {
  // slow down processor by a factor of 8
  CLKPR = _BV(CLKPCE);
  CLKPR = _BV(CLKPS1) | _BV(CLKPS0);
}
  

//------------------------------------------------------------------------------
/*
 * read one byte from address
 */
boolean readEEPROM(unsigned long address, uint8_t *byte) {
  // issue a start condition, send device address and write direction bit
  uint8_t i2caddr = (EEPROMADDR >> 1) | (address&0x10000 ? 4 : 0);
  uint8_t end;
  Wire.beginTransmission(i2caddr);
  Wire.write((address>>8)&0xFF);
  Wire.write(address&0xFF);
  end = Wire.endTransmission(false);
  Wire.requestFrom(i2caddr,1);
  *byte = Wire.read();
  return (end == 0);
}
//------------------------------------------------------------------------------
/* 
 *burst read 
 */
boolean readBurstEEPROM(unsigned long start, unsigned long stop) {
  // does not handle the transition from 0x0FFFF to 0x10000
  // since we only use it for performance evaluation, we do not care!
  unsigned long addr = start;
  uint8_t byte;
  uint8_t i2caddr = (EEPROMADDR >> 1) | (start&0x10000 ? 4 : 0);
  uint8_t end;

  Wire.beginTransmission(i2caddr);
  Wire.write((start>>8)&0xFF);
  Wire.write(start&0xFF);
  end = Wire.endTransmission(false);
  
  while (addr < stop) {
    addr += 32;
    Wire.requestFrom(i2caddr,32,(addr >= stop));
    while (Wire.available()) byte = Wire.read();
  }
  return (end == 0);
}


//------------------------------------------------------------------------------

/*
 * write 1 byte to 'address' in eeprom
 */
boolean writeEEPROM(long unsigned address, uint8_t byte) {

  uint8_t end;
  uint8_t i2caddr = (EEPROMADDR >> 1) | (address&0x10000 ? 4 : 0);
  Wire.beginTransmission(i2caddr);
  Wire.write((address>>8)&0xFF);
  Wire.write(address&0xFF);
  Wire.write(byte);
  end = Wire.endTransmission(true);

  delay(6);
  return (end == 0);
}

//------------------------------------------------------------------------------
/*
 * delete eeprom
 */
boolean deleteEEPROM(long unsigned from, unsigned long to, uint8_t byte, 
		     boolean poll) {

  unsigned long tempto, i;
  uint8_t i2caddr;
  uint8_t end = 0;
  
  while (from <= to) {
    tempto = ((from/128)+1)*128-1;
    if (tempto > to) tempto = to;
    i2caddr = (EEPROMADDR >> 1) | (from&0x10000 ? 4 : 0);
    Wire.beginTransmission(i2caddr);
    Wire.write((from>>8)&0xFF);
    Wire.write(from&0xFF);
    
    // send data to EEPROM
    for (i=from; i<=tempto; i++)
      Wire.write(byte);
    // issue a stop condition
    end |= Wire.endTransmission();

    // wait for ack again
    if (!poll) delay(6);

    from = tempto+1;
  }
  return (end == 0);
}

//------------------------------------------------------------------------------
boolean performanceTest() {
  unsigned long eeaddr;
  unsigned long startmicros, endmicros;
  int avgtime;
  boolean OK = true;
  uint8_t byte;

  Serial.println(F("\nPerformance test:"));
  
  Serial.println(F("Sequential reads ..."));
  startmicros = micros();
  OK &= readBurstEEPROM(0,MAXTESTADDR);
  endmicros = micros();
  Serial.print(F("Time: "));
  avgtime = (endmicros-startmicros)/(MAXTESTADDR+1);
  Serial.print(avgtime);
  Serial.println(F(" micro secs/byte"));
  if (!OK) {
    Serial.println(F("An error occured"));
    return false;
  }
  
  Serial.println(F("Random reads ..."));
  startmicros = micros();
  for (eeaddr = 0; eeaddr <= MAXTESTADDR; eeaddr++) 
    OK &= readEEPROM(eeaddr,&byte);
  endmicros = micros();
  Serial.print(F("Time: "));
  avgtime = (endmicros-startmicros)/(MAXTESTADDR+1);
  Serial.print(avgtime);
  Serial.println(F(" micro secs/byte"));
  if (!OK) {
    Serial.println(F("An error occured"));
    return false;
  }

  Serial.println(F("Random writes ..."));
  startmicros = micros();
  for (eeaddr = 0; eeaddr <= MAXTESTADDR; eeaddr++) 
    OK &= writeEEPROM(eeaddr,0x55);
  endmicros = micros();
  Serial.print(F("Time: "));
  avgtime = (endmicros-startmicros)/(MAXTESTADDR+1);
  Serial.print(avgtime);
  Serial.println(F(" micro secs/byte"));
  if (!OK) {
    Serial.println(F("An error occured"));
    return false;
  }
  
  Serial.println(F("Page writes with wait ..."));
  startmicros = micros();
  OK &= deleteEEPROM(0,MAXTESTADDR,0xFF,false);
  endmicros = micros();
  Serial.print(F("Time: "));
  avgtime = (endmicros-startmicros)/(MAXTESTADDR+1);
  Serial.print(avgtime);
  Serial.println(F(" micro secs/byte"));
  if (!OK) {
    Serial.println(F("An error occured"));
    return false;
  }

  Serial.println(F("Page writes with poll ..."));
  startmicros = micros();
  OK &= deleteEEPROM(0,MAXTESTADDR,0xFF,true);
  endmicros = micros();
  Serial.print(F("Time: "));
  avgtime = (endmicros-startmicros)/(MAXTESTADDR+1);
  Serial.print(avgtime);
  Serial.println(F(" micro secs/byte"));
  if (!OK) {
    Serial.println(F("An error occured"));
    return false;
  }
  
  return true;
}
//------------------------------------------------------------------------------

unsigned long parseHex() {
  unsigned long result = 0L;
  char inp = '\0';
  byte num = 0;

  while (inp != '\r' && inp != '#') {
    while (!Serial.available());
    inp = Serial.read();
    if (inp == '\r' || inp == '#') break;
    if (inp >= 'a' && inp <= 'f')
      inp = inp -'a' + 'A';
    if ((inp >= '0' && inp <= '9') ||
	(inp >= 'A' && inp <= 'F')) {
      Serial.print(inp);
      if (inp >= '0' && inp <= '9') num = inp - '0';
      else num = inp - 'A' + 10;
      result = result * 16;
      result = result + num;
    }
  }
  Serial.println();
  return result;
}

void help (void) {
  Serial.println();
  Serial.println(F("r - read byte from address"));
  Serial.println(F("w - write byte to address"));
  Serial.println(F("d - delete from start address to end address"));  
  Serial.println(F("l - list memory range"));
  Serial.println(F("p - test performance"));
  Serial.println(F("h - help message"));
  Serial.println(F("Finish all numeric inputs with '#'"));
}

//------------------------------------------------------------------------------



void setup(void) {
#if I2C_CPUFREQ == (F_CPU/8)
  CPUSlowDown();
#endif

  Serial.begin(115200);
  Serial.println(F("\n\nTest program for EEPROM 24AA1025 (SoftWire)"));
  help();
}


void loop(void) {
  char cmd;
  uint8_t byte;
  boolean noterror;
  unsigned long addr, toaddr;

  while (!Serial.available());
  cmd = Serial.read();
  switch (cmd) {
  case 'r': Serial.print(F("Read from addr: "));
    addr = parseHex();
    Serial.println(F("Reading..."));
    noterror = readEEPROM(addr,&byte);
    Serial.print(addr,HEX);
    Serial.print(F(": "));
    if (byte < 0x10) Serial.print("0");
    Serial.println(byte,HEX);
    if (!noterror) Serial.println(F("Error while reading"));
    break;
  case 'w':
    Serial.print(F("Write to addr: "));
    addr = parseHex();
    Serial.print(F("Value: "));
    byte = parseHex();
    Serial.println(F("Writing..."));
    noterror = writeEEPROM(addr,byte);
    if (!noterror) Serial.println(F("Error while reading"));
    break;
  case 'd':
    Serial.print(F("Delete from addr: "));
    addr = parseHex();
    Serial.print(F("to addr: "));
    toaddr = parseHex();
    Serial.print(F("Value: "));
    byte = parseHex();
    Serial.print(F("Deleting ... "));
    noterror = deleteEEPROM(addr,toaddr,byte,false);
    Serial.println(F("...done"));
    if (!noterror) Serial.println(F("Error while deleting"));
    break;
  case 'l':
    Serial.print(F("List from addr: "));
    addr = parseHex();
    Serial.print(F("to addr: "));
    toaddr = parseHex();
    while (addr <= toaddr) {
      noterror = readEEPROM(addr,&byte);
      Serial.print(addr,HEX);
      Serial.print(F(": "));
      if (byte < 0x10) Serial.print("0");
      Serial.println(byte,HEX);
      if (!noterror) Serial.println(F("Error while reading"));
      addr++;
    }
    break;
  case 'p': 
    noterror = performanceTest();
    if (!noterror) Serial.println(F("Error while executing performance test"));
    break;
  case 'h':
    help();
    break;
  case '\r':
  case '\n':
  case ' ':
    break;
  default:
    Serial.println(F("Unknown command"));
    Serial.println();
    help();
  }
}
