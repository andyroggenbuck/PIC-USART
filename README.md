# USART User Interface for PIC Development Board

This project was done as a lab assignment in the Advanced Digital Design course at Ferris State University. It uses asynchronous serial communication to provide a text-based user interface through which a user can utilize the devices on the Bulldog PIC++ development board. The development board was designed and built by Professor Robert Most at Ferris State University, featuring a PIC18F46K80 microcontroller and a variety of additional devices.

The project was programmed in C using MPLAB X and compiled using Microchip's C18 compiler. Functions from the C18 compiler library were used to configure and use the PIC18's USART peripheral. The development board features a USB to serial converter, which receives serial data from the PIC18 via USART and provides USB connectivity to a host PC. The user interface is accessed via a serial terminal on the host PC.

The project performs these functions:

* Display the current illuminance measurement from a light sensor
  * Analog light sensor read as an 8-bit value by the PIC18's A/D converter
* Change the color of an RGB LED
  * BlinkM smart LED controlled via I2C
* Display a user-entered byte value in binary on a row of eight LEDs
  * Eight GPIO pins used to control LEDs
* Turn a piezoelectric buzzer on or off
  * Buzzer driven by a 488Hz square wave generated by one of the PIC18's Capture/Compare/PWM modules in PWM mode
* Display a user-entered text string on an LCD
  * LCD containing SPLC782 LCD driver interfaced to PIC18 via 4-bit parallel interface
  * LCD functions accessed via pre-written assembly functions called from main C program
* Display the current temperature measurement from a temperature sensor
  * TC74 temperature sensor accessed via I2C
