// -*- c++ -*-
// Simple sketch to read out one register of an I2C device
#define I2C_TIMEOUT 1000
#define I2C_PULLUP 1

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

#include <SoftI2CMaster.h>

#define I2C_7BITADDR 0x68 // DS1307
#define MEMLOC 0x0A
#define ADDRLEN 1 // address length, usually 1 or 2 bytes

void setup(void) {
    Serial.begin(115200);
    Serial.println(F("START " __FILE__ " from " __DATE__));

    if (!i2c_init()) {
        Serial.println("I2C init failed");
    }
}

void loop(void) {
    if (!i2c_start_wait((I2C_7BITADDR << 1) | I2C_WRITE)) {
        Serial.println("I2C device busy");
        delay(1000);
        return;
    }
    for (byte i = 1; i < ADDRLEN; i++) {
        i2c_write(0x00);
    }
    i2c_write(MEMLOC);
    i2c_rep_start((I2C_7BITADDR << 1) | I2C_READ);
    byte val = i2c_read(true);
    i2c_stop();
    Serial.println(val);
    delay(1000);
}
