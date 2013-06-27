SoftI2CMaster
=============

Software I2C Arduino library

This is a very fast and very light-weight software I2C-master library
written in assembler. It is based on Peter Fleury's I2C software I2C
library: http://homepage.hispeed.ch/peterfleury/avr-software.html

It assumes a single master and does not support bus arbitration. It
allows for clock stretching by slave devices and also supports
timeouts for clock stretching between 1 and 10000 milli seconds.

Even on 1MHz systems, you can get a transfer speed of 40 kbit/sec.