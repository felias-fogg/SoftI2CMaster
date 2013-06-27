// reads out the MLX90614 infrared thermometer

#include <Arduino.h>
#define SDA_PORT PORTD
#define SDA_PIN 3
#define SCL_PORT PORTD
#define SCL_PIN 5
#include <SoftI2CMaster.h>



#define DEVICE (0x5A<<1)



void setup(){
#if (__AVR_ARCH__  == 5) // means ATMEGA 
	Serial.begin(19200);
	Serial.println("Setup...");
#endif
	i2c_init();
}

void loop(){
    int dev = 0x5A<<1;
    int data_low = 0;
    int data_high = 0;
    int pec = 0;
    
    i2c_start(dev+I2C_WRITE);
    i2c_write(0x07);
    // read
    i2c_rep_start(dev+I2C_READ);
    data_low = i2c_read(false); //Read 1 byte and then send ack
    data_high = i2c_read(false); //Read 1 byte and then send ack
    pec = i2c_read(true);
    i2c_stop();
    
    //This converts high and low bytes together and processes temperature, MSB is a error bit and is ignored for temps
    double tempFactor = 0.02; // 0.02 degrees per LSB (measurement resolution of the MLX90614)
    double tempData = 0x0000; // zero out the data
    int frac; // data past the decimal point
    
    // This masks off the error bit of the high byte, then moves it left 8 bits and adds the low byte.
    tempData = (double)(((data_high & 0x007F) << 8) + data_low);
    tempData = (tempData * tempFactor)-0.01;
    
    float celcius = tempData - 273.15;
    float fahrenheit = (celcius*1.8) + 32;

#if (__AVR_ARCH__  == 5) // means ATMEGA 
    Serial.print("Celcius: ");
    Serial.println(celcius);

    Serial.print("Fahrenheit: ");
    Serial.println(fahrenheit);
#endif

    delay(1000); // wait a second before printing again
}
