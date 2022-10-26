# USART User Interface for PIC Development Board

This project uses asynchronous serial communication to provide a text-based user interface through which a user can utilize the devices on the Bulldog PIC++ development board, which was designed and built by Professor Robert Most at Ferris State University. This project was done as a lab assignment in the Advanced Digital Design course at Ferris State University.

The hardware used in this project is shown in the block diagram below.

</br>
<p align="center">
  <img src="https://github.com/andyroggenbuck/PIC-USART/blob/master/Images/IDL4%20-%20Hardware%20Block%20Diagram.png" width=100%>
</p>
</br>

The project was programmed in C using MPLAB X and compiled using Microchip's C18 compiler. Functions from the C18 compiler library were used to configure and use peripherals of the PIC18. The PIC18 uses USART to send serial data to a USB to serial converter, which provides USB connectivity to a host PC. The user interface is accessed via a serial terminal on the host PC.

After initializations, the program prompts the user for an input command and then polls the DataRdy1USART() function to wait until a character is received. A switch case statement takes the necessary action when a valid command is received. The available commands are as follows:
</br>
</br>
### L - Display the current illuminance measurement from the light sensor
The voltage from the analog light sensor is read as an 8-bit value by the PIC18's A/D converter, converted to an illuminance value in lux, and printed via USART to the user.
</br>
</br>
### B - Enter new color and intensity for the BlinkM LED
The user is prompted to enter a color and brightness level from 0-255. The user's input strings are converted to numerical values for the R, G, and B channels of the LED and sent to the LED via I2C.
</br>
</br>
### D - Enter a byte value to be displayed on a row of eight discrete LEDs
The user is prompted to enter a hexadecimal value from 00-FF, which is converted to an 8-bit integer and written to the GPIO register controlling the LEDs.
</br>
</br>
### N - Turn piezoelectric buzzer on
During initialization, one of the PIC18's Capture/Compare/PWM modules is configured in PWM mode to produce a 488Hz waveform on the output pin driving the buzzer. The PWM module runs continuously, and the buzzer is turned on or off by setting its GPIO pin as an output or input. The 'N' command sets the pin as an output to enable the buzzer.
</br>
</br>
### O - Turn piezoelectric buzzer off
The 'O' command sets the buzzer's GPIO pin as an input to disable the tone.
</br>
</br>
### S - Enter string to be displayed on LCD
The user is prompted to enter a message (16 characters max), which is stored in an array and then written to the LCD. To access the LCD, the course instructor provided an assembly file containing subroutines which are called as functions from this C program.
</br>
</br>
### T - Display the current temperature in degrees Celsius
A new temperature value is read from the temperature sensor via I2C and displayed to the user via USART.
