# I2CShell

I2CShell allows you to interact with I2C devices from the console,
similar to what you can do with the Bus Pirate. You
can type in a line such as

    [ 0x10 5 [ 0x11 r:3 ]
This will address the I2C device under (8-bit) address 0x10,
corresponding to 7-bit address 0x08 in writing direction, and
send 0x05 to the device. After that the device is again addressed
with a repeated start in reading direction and 3 bytes are requested.
The output could look like as follows:

    Start: 0x10 (0x08!) + ACK
    Write: 0x05 + ACK
    Rep. start: 0x11 (0x08?) + ACK
    Read: 0x10 + ACK
    Read: 0x5A + ACK
    Read: 0x98 + NAK
    Stop


## Installation and configuration

<code>I2CShell</code> is part of the bit-banging I2C library
[SoftI2CMaster](https://github.com/felias-fogg/SoftI2CMaster). After
downloading this library, you find I2CShell as one of the examples in
the <code>examples</code> folder.

Before you use the sketch, you should probably adjust some of the
compile-time constants in the sketch. 

    USEEEPROM
If 1 (the default), macros are stored in EEPROM so that they survive
program exits. If 0, the EEPROM contents is not touched.

    I2C_HARDWARE
If 1 (default), the I2C hardware support is used instead of the
bit-banging version of the I2C library. This gives a little bit more
flexibility (concerning clock frequency and pullup resistors),
but restricts you to use the standard I2C pins SDA and SCL.

    I2C_TIMEOUT
If 0, I2C operations do not time out. The default value is 100
(milliseconds). It is safe to leave it this way.

    I2C_PULLUP
If 1 (the default), then the MCU internal pullups are activated. This
is safe as long as you do not connect 3 Volt devices that get
destroyed by higher voltage.

Finally, there are the four constants to determine the I2C pins when
you use the bit-banging version of the I2C library (i.e. I2C_HARDWARE
is 0). These pins have to be specified in a
way so that
[port manipulation commands](http://www.arduino.cc/en/Reference/PortManipulation)
can be used. Instead of specifying the number of the digital pin
(0-19), the *port* (PORTB, PORTC, PORTD) and the *port pin* has to be
specified. The mapping is explained
[here](http://www.arduino.cc/en/Reference/PortManipulation). For
example, if you want to use *digital pin 2* for *SCL* and *digital pin 14*
(= analog pin 0) for *SDA*, you have to specify *port pin 2 of PORTD* for
SCL and *port pin 0 of PORTC* for SDA. 

    #define SCL_PIN 2
    #define SCL_PORT PORTD
    #define SDA_PIN 0
    #define SDA_PORT PORTC


Note that all the compile time constants have to be placed before the
inclusion of the <code>SoftI2CMaster</code> library.


## Commands

You can use I2CShell either with Arduino's serial monitor or with
an ordinary terminal program (e.g. <code>screen</code> under
Unix). Baudrate is 115200, 8 bits, 1 stop bit, no parity bit.

In the following _\<digit\>_ denotes a single decimal digit and *\<number\>*
denotes a non-negative number in the usual notation, i.e., 17, 0x11,
0X11, 0o21, or 0b10001
can all be written in order to refer to the decimal number
seventeen. As a means to make life easier when converting between 8-bit
and 7-bit addresses, one can postfix a number with <code>!</code> or
<code>?</code> in order to signify the write, resp. read direction
together with a 7-bit address. This means that, e.g. 0x10 can also be
written as 0x08! and 0x11 as 0x08?. In general, the input is case
insensitive. 

There are a number of commands that one can use to interact with the
program in order to configure it and to access the I2C bus:

* <code>H</code> gives a short help screen.
* <code>S</code> scans the I2C bus for devices. Reports under which
  addresses the program receives an ACK.
* <code>T</code> prints last execution trace again.
* <code>T\<number\></code> prints 20 commands of the last execution trace
  starting at command \<number\> (numbering starts at command 0).
* <code>\<digit\>=...</code> defines a macro under the single digit
  identifier \<digit\>. These macros can be used later inside I2C command
  strings. 
* <code>L</code> lists all macros.
* <code>L\<digit\></code> lists the macro with identifier \<digit\>.  
* <code>P</code> shows status of pullup resistors.
* <code>P\<digit\></code> enables the pullups (1) or disables them
  (0). Works only if the hardware interface has been enabled (I2C_HARDWARE
  must be 1).
* <code>F</code> reports the I2C clock frequency.
* <code>F\<number\></code> sets the I2C clock frequency to \<number\>
  kHz. Works only if the hardware  interface has been enabled.

## I2C interaction

You can write one line (of 80 characters) of I2C commands that is 
sent to the I2C bus after you finish the line with the \<Return\>
key. The results are recorded and then presented in the terminal
window, as shown above. There exist the following commands:

* <code>[</code> Issue a start condition; this is needed in order to claim access to the bus and is always followed by an I2C device address.
* <code>{</code> Issue start conditions until the addressed device responds with an ACK. The maximum number of repetitions is 255.
* <code>]</code> Issue a stop condition, which releases the bus.
* <code>\<number\></code> Send one byte to the I2C bus; the receipt of
  the byte will be acknowledged by the receiving device with an ACK.
* <code>R</code> Read one byte from the addressed I2C device. The NAK
  for the last byte will be automatically generated.
* <code>&</code> Wait for a microsecond.
* <code>%</code> Wait for a millisecond.
* <code>:\<number\></code> Repeat the previous command \<number\>
  times. This is not allowed for start conditions and previous
  repetitions.
* <code>(\<digit\>)</code> Execute macro.

Between commands, one can  put commas and blanks in order to make the line
more readable. The only point when you have to use such separators is
when two numbers are adjacent. 

## Edit commands

When using a terminal window, there are also a number of edit commands
one can use:

* <code>Ctrl-A</code> or <code>UP arrow</code> Go to start of line.
* <code>Ctrl-E</code> or <code>DOWN arrow</code> Go to end on line.
* <code>Ctrl-B</code> or <code>LEFT arrow</code> Go one character
backward.
* <code>Ctrl-F</code> or <code>RIGHT arrow</code> Go one character
forward.
* <code>Ctrl-H</code> or <code>DELETE</code> Delete last character.
* <code>Ctrl-D</code> Delete character under cursor.
* <code>Ctrl-K</code> Kill rest of line.
* <code>Ctrl-P</code> If typed before anything else is written, the
  last line is retrieved and can be edited.
* <code>$\<digit\></code> Inserts the macro \<digit\> at this point
  into the line.
