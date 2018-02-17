# SoftI2CMaster


## Why another I2C library?

The standard I2C library for the Arduino is the
[Wire Library](http://arduino.cc/en/Reference/Wire). While this
library is sufficient most of the time when you want to communicate
with devices, there are situations when it is not applicable:
* the I2C pins SDA/SCL are in use already for other purposes,
* the code shall run on an ATtiny processor with 1 MHz on arbitrary pins,
* you are short on memory (flash and RAM), or
* you do not want to use the implicitly enabled pull-up resistors
  because your devices are run with 3 volts.

I adapted [Peter Fleury's I2C software
library](http://homepage.hispeed.ch/peterfleury/avr-software.html)
that is written in AVR assembler, extremely light weight (just under 500
byte in flash) and very
fast. Even on an ATtiny running with 1MHz, one can still operate the
bus with 33 kHz, which implies that you can drive slave devices that
use the SMBus protocol (which timeout if the the bus frequency is
below 10 kHz).

If you want a solution running on an ARM MCU (Due, Zero, Teensy 3.x), you want to use pins
on port H or above on an ATmega256, or you want to use many different I2C buses,
this library is not the right solution for you. In these cases, another
bit-banging I2C library written in pure C++ could perhaps help you: [SlowSoftI2CMaster](https://github.com/felias-fogg/SlowSoftI2CMaster).

## Features

This library has the following features:
* supports only master mode
* compatible with all 8-bit AVR MCUs
* no bus arbitration (i.e., only one master allowed on bus)
* clock stretching (by slaves) supported
* timeout on clock stretching
* timeout on ACK polling for busy devices (new!)
* internal MCU pullup resistors can be used (new!)
* can make use of almost any pin (except for pins on port H and above on large ATmegas)
* very lightweight (roughly 500 bytes of flash and 0 byte of RAM, except for call stack)
* it is not interrupt-driven
* very fast (standard and fast mode on ATmega328, 33 kHz on ATtiny
with 1 MHz CPU clock)
* Optional <code>Wire</code> library compatible interface
* GPL license


## Installation

Just download the
[Zip-File from github](https://github.com/felias-fogg/SoftI2CMaster),
uncompress, rename the directory to <code>SoftI2CMaster</code> and move it into
the <code>libraries</code> folder. In case you have not installed a library
before, consult the the respective [help page](http://arduino.cc/en/Guide/Libraries).

## Importing the library

In order to use the library, you have to import it using the include
statement:

    #include <SoftI2CMaster.h>

In the program text *before* the include statement, some compile-time
parameters have to be specified, such as which pins are used for the
data (SDA) and clock (SCL) lines. These pins have to be specified in a
way so that
[port manipulation commands](http://www.arduino.cc/en/Reference/PortManipulation)
can be used. Instead of specifying the number of the digital pin
(0-19), the *port* (PORTB, PORTC, PORTD) and the *port pin* has to be
specified. The mapping is explained
[here](http://www.arduino.cc/en/Reference/PortManipulation). For
example, if you want to use *digital pin 2* for *SCL* and *digital pin 14*
(= analog pin 0) for *SDA*, you have to specify *port pin 2 of PORTD* for
SCL and *port pin 0 of PORTC* for SDA:

    #define SCL_PIN 2
    #define SCL_PORT PORTD
    #define SDA_PIN 0
    #define SDA_PORT PORTC
    #include <SoftI2CMaster.h>


## Configuration

There are a few other constants that you can define in order to
control the behavior of the library. You have to specify them before
the <code>include</code> statement so that they can take effect. Note
that this different from the usual form of libraries! This library is
always compiled with your sketch and therefore the <code>defines</code>
need to be specfied before the inclusion of the library!

    #define I2C_HARDWARE 1
Although this is basically a bit-banging library, there is the
possibility to use the hardware support for I2C, if you happen
to run this library on an MCU such as the ATmega328 that implements this. If this constant is
set to 1, then the hardware registers are used (and you have to
use the standard SDA and SCL pins).

    #define I2C_PULLUP 1
With this definition you enable the internal pullup resistores of the
MCU. Note that the internal pullups have around 50k&#x2126;, which may be
too high. This slows down the bus speed somewhat.
Furthermore, when switching from <code>HIGH</code> to
<code>LOW</code> (or the other way around), the bus lines will temporarily in a high impedance
state. With low I2C frequencies, things will
probably work. However, be careful when using this option and better check with a
scope that things work out.

    #define I2C_TIMEOUT ...
Since slave devices can stretch the low period of the clock
indefinitely, they can lock up the MCU. In order to avoid this, one
can define <code>I2C_TIMEOUT</code>. Specify the number of
milliseconds after which the I2C functions will time out. Possible
values are 0 (no time out) to 10000 (i.e., 10 seconds). Enabling this
option slows done the bus speed somewhat.

    #define I2C_MAXWAIT ...
When waiting for a busy device, one may use the function
<code>i2c_start_wait(addr)</code> (see below), which sends start
commands until the device responds with an <code>ACK</code>. If the
value of this constant is different from 0, then it specifies the
maximal number of start commands to be sent. Default value is 1000.

    #define I2C_NOINTERRUPT 1
With this definition you disable interrupts between issuing a start
condition and terminating the transfer with a stop condition. Usually,
this is not necessary. However, if you have an SMbus device that can
timeout, one may want to use this feature. Note however, that in this
case interrupts become unconditionally disabled when calling
<code>i2c_start(...)</code> und unconditionally enabled after calling <code>i2c_stop()</code>.

    #define I2C_CPUFREQ ...
If you are changing the CPU frequency dynamically using the clock
prescale register CLKPR and intend to call the I2C functions with a
frequency different from F_CPU, then define this constant with the
correct frequency. For instance, if you used a prescale factor of 8,
then the following definition would be adequate: <code>#define I2C_CPUFREQ (F_CPU/8)</code>

    #define I2C_FASTMODE 1
The *standard I2C bus frequency* is 100kHz. Often, however, devices permit for faster transfers up to 400kHz. If you want to allow for the higher frequency, then the above definition should be used.

    #define I2C_SLOWMODE 1
In case you want to slow down the clock frequency to less than 25kHz, you can use this
definition (in this case, do not define
<code>I2C_FASTMODE</code>!). This can help to make the communication
more reliable.

I have measured the maximal bus frequency under different processor
speeds. The results are displayed in the following
table. The left value is with <code>I2C_TIMEOUT</code> and
<code>I2C_PULLUP</code> disabled. The right value is the bus
frequency with both options enabled. Note also that there is a high
clock jitter (roughly 10-15%) because the clock is implemented by delay
loops. This is not a problem for the I2C bus, though. However, the
throughput might be lower than one would expect from the numbers in
the table. 

<table>
<tr> <th></th> <th colspan="2" align="center">1 MHz</th> <th
colspan="2" align="center">2 MHz</th> <th colspan="2" align="center">4
MHz</th> <th colspan="2" align="center">8 MHz</th>
<th colspan="2" align="center">16 MHz</th> </tr>

<tr>
<td align="left">Slow mode (kHz) </td><td align="right"> 23 </td><td align="right">21 </td><td
align="right"> 24 </td><td align="right">22 </td><td align="right"> 24 </td><td align="right">23</td><td
align="right"> 24 </td><td align="right">23</td><td align="right"> 24 </td><td align="right">23</td></tr>

<tr><td align="left">Standard mode (kHz) </td><td align="right">
45 </td><td align="right">33</td><td align="right"> 90 </td><td align="right">72 </td><td align="right"> 95 </td><td align="right">83</td><td
align="right"> 95 </td><td align="right">89</td><td align="right"> 90 </td><td align="right">88</td></tr>

<tr><td align="left">Fast mode (kHz) </td><td align="right">
45 </td><td align="right">33</td><td align="right"> 90 </td><td align="right">72</td><td align="right"> 180 </td><td align="right">140</td><td
align="right"> 370 </td><td align="right">290</td><td align="right"> 370 </td><td align="right">330</td></tr>
</table>

## Interface

The following functions are provided by the library:

    i2c_init()
Initialize the I2C system. Must be called once in
<code>setup</code>. Will return <code>false</code> if SDA or SCL is on
a low level, which means that the bus is locked. Otherwise returns
<code>true</code>. 

    i2c_start(addr)
Initiates a transfer to the slave device with the 8-bit I2C address
*<code>addr</code>*. Note that this library uses the 8-bit addressing
scheme different from the 7-bit scheme in the Wire library.
In addition the least significant bit of *<code>addr</code>* must be specified as
<code>I2C_WRITE</code> (=0) or <code>I2C_READ</code> (=1). Returns
<code>true</code> if the addressed device replies with an
<code>ACK</code>. Otherwise <code>false</code> is returned. 

    i2c_start_wait(addr)
Similar to the <code>i2c_start</code> function. However, it tries
repeatedly to start the transfer until the device sends an
acknowledge. It will timeout after <code>I2C_MAXWAIT</code> failed
attempts to contact the device (if this value is different from 0). By
default, this value is 1000.

    i2c_rep_start(addr)
Sends a repeated start condition, i.e., it starts a new transfer
without sending first a stop condition. Same return value as
<code>i2c_start()</code>. 

    i2c_stop()
Sends a stop condition and thereby releases the bus. No return value.

    i2c_write(byte)
Sends a byte to the previously addressed device. Returns
<code>true</code> if the device replies with an ACK, otherwise <code>false</code>.

    i2c_read(last)
Requests to receive a byte from the slave device. If <code>last</code>
is <code>true</code>, then a <code>NAK</code> is sent after receiving
the byte finishing the read transfer sequence. The function returns
the received byte.

## Example

As a small example, let us consider reading one register from an I2C
device, with an address space < 256 (i.e. one byte for addressing) 

	// Simple sketch to read out one register of an I2C device
	#define SDA_PORT PORTC
	#define SDA_PIN 4 // = A4
	#define SCL_PORT PORTC
	#define SCL_PIN 5 // = A5
	#include <SoftI2CMaster.h>

	#define I2C_7BITADDR 0x68 // DS1307
	#define MEMLOC 0x0A 

	void setup(void) {
	    Serial.begin(57600);
	    if (!i2c_init()) // Initialize everything and check for bus lockup
            Serial.println("I2C init failed");
	}

	void loop(void){
	    if (!i2c_start((I2C_7BITADDR<<1)|I2C_WRITE)) { // start transfer
	        Serial.println("I2C device busy");
	        return;
	    }
	    i2c_write(MEMLOC); // send memory address
	    i2c_rep_start((I2C_7BITADDR<<1)|I2C_READ); // restart for reading
	    byte val = i2c_read(true); // read one byte and send NAK to terminate
	    i2c_stop(); // send stop condition
	    Serial.println(val);
	    delay(1000);
	}

## I2CShell

In the example directory, you find a much more elaborate example:
<code>I2CShell</code>. This sketch can be used to interact with I2C
devices similar in the way you can use the Bus Pirate. For example,
you can type:

    [ 0xAE 0 0 [ 0xAF r:5 ]

This will address the I2C device under the (8-bit) address in write
mode, set the reading register to 0, then opens the same device again
in read mode and read 5 registers. A complete documentation of this
program can be found in the
[I2CShell example folder](https://github.com/felias-fogg/SoftI2CMaster/tree/master/examples/I2CShell).

## Alternative Interface

Meanwhile, I have written a wrapper around SoftI2CMaster that emulates
the [Wire library](http://arduino.cc/en/Reference/Wire)
(master mode 
only). It is another C++-header file called <code>SoftWire.h</code>,
which you need to include instead of
<code>SoftI2CMaster.h</code>. The ports and pins have to be specified
as described above. After the include statement you need to
create a <code>SoftWire</code> instance:

    #define SDA_PORT ...
    ...
    #include <SoftWire.h>
    SoftWire Wire = SoftWire();
    ...
    setup() {
        Wire.begin()
    ...
    }

This interface sacrifices some of the advantages of the original
library, in particular its small footprint, but comes handy if you
need a replacement of the original *Wire* library. The following section
sketches the memory footprint of different I2C libraries.

## Memory requirements

In order to measure the memory requirements of the different
libraries, I wrote a baseline sketch, which contains all necessary I2C
calls for reading and writing an EEPROM device, and compiled it
against a library with empty functions. Then all the other libraries
were used. For the Wire-like libraries, I had to rewrite the sketch,
but it has the same functionality. The memory requirements differ
somewhat from ATmega to ATtiny, but the overall picture is
similar. The take-home message is: If you are short on memory (flash
or RAM), it
makes sense to use the SoftI2CMaster library. 

<table align="right">
<tr><td colspan="10" align="center">ATmega328</td></tr>
<tr><th>Library</th><th align="center">SoftI2C-</th><th align="center">SoftI2C-</th><th
align="center">SoftI2C-</th><th align="center">Soft-</th><th
align="center">SlowSoft-</th><th align="center">SlowSoft-</th><th
align="center">USI-</th><th align="center">Tiny-</th><th
align="center">Wire</th></tr>
<tr><th></th><th align="center">Master</th><th align="center">Master</th><th
align="center">Master</th><th align="center">Wire</th><th
align="center">I2CMaster</th><th align="center">Wire</th><th
align="center">Wire</th><th align="center">Wire</th><th
align="center"></th></tr>
<tr><th>Option</th><th align="center"></th><td align="center">Pullup+Timeout</td><td
align="center">Hardware</td><th align="center"></th><th
align="center"></th><th align="center"></th><th
align="center"></th><th align="center"></th><th
align="center"></th></tr>
<tr><td>Flash</td><td align="right">482</td><td align="right">564</td><td
align="right">434</td><td align="right">1066</td><td
align="right">974</td><td align="right">1556</td><td
align="center">-</td><td align="center">-</td><td
align="right">1972</td></tr>
<tr><td>RAM</td><td align="right">0</td><td align="right">0</td><td
align="right">0</td><td align="right">66</td><td
align="right">4</td><td align="right">70</td><td
align="center">-</td><td align="center">-</td><td
align="right">210</td></tr>
</table>

<table align="right">
<tr><td colspan="10" align="center">ATtiny85</td></tr>
<tr><th>Library</th><th align="center">SoftI2C-</th><th align="center">SoftI2C-</th><th
align="center">SoftI2C-</th><th align="center">Soft-</th><th
align="center">SlowSoft-</th><th align="center">SlowSoft-</th><th
align="center">USI-</th><th align="center">Tiny-</th><th
align="center">Wire</th></tr>
<tr><th></th><th align="center">Master</th><th align="center">Master</th><th
align="center">Master</th><th align="center">Wire</th><th
align="center">I2CMaster</th><th align="center">Wire</th><th
align="center">Wire</th><th align="center">Wire</th><th
align="center"></th></tr>
<tr><th>Option</th><th align="center"></th><td align="center">Pullup+Timeout</td><td
align="center">Hardware</td><th align="center"></th><th
align="center"></th><th align="center"></th><th
align="center"></th><th align="center"></th><th
align="center"></th></tr>
<tr><td>Flash</td><td align="right">428</td><td align="right">510</td><td
align="center">-</td><td align="right">1002</td><td
align="right">732</td><td align="right">1292</td><td
align="right">1108</td><td align="right">1834</td><td
align="center">-</td></tr>
<tr><td>RAM</td><td align="right">0</td><td align="right">0</td><td
align="center">-</td><td align="right">66</td><td
align="right">4</td><td align="right">70</td><td
align="right">45</td><td align="right">86</td><td
align="center">-</td></tr>
</table>


## Shortcomings

The entire code had to be included in the header file, because the communication ports in the code need to be determined at compile time. This implies that this header file should only be included once per project (usually in the sketch). 

Another shortcoming is that one cannot use ports H and above on an ATmega256. The reason is that these ports are not directly addressable. 

Finally, as mentioned, the code runs only on AVR MCUs (because it uses
assembler). If you want to use a software I2C library on the ARM
platform, you could use
https://github.com/felias-fogg/SlowSoftI2CMaster, which uses only C++
code. Because of this, it is much slower, but on a Genuino/Arduino
Zero, the I2C bus runs with roughly 100kHz. There is also a Wire-like wrapper available for this library: https://github.com/felias-fogg/SlowSoftWire.



