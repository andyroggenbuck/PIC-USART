/*
 * Andy Roggenbuck
 * ECNS 424
 * IDL 4
 * 
 * This project uses asynchronous serial communication via the PIC18's USART
 * peripheral and the Bulldog PIC++'s USB to serial converter to create a text-
 * based user interface accessible through a serial terminal on a host PC. The 
 * interface provides commands to access many of the features implemented in
 * previous labs: ?L? to display the current illuminance measurement, ?B? to 
 * control the BlinkM LED via I2C, ?D? to display a user-entered byte value on 
 * the Port B LEDs, ?N? and ?O? to control a musical tone played by the 
 * piezoelectric buzzer, ?S? to display a user-entered string on the LCD, and 
 * ?T' to display the current temperature measurement. For any other character, 
 * the program displays a list of the available commands. 

	Default Pinout designations for Bulldog PIC++:

        *********** PIC18F46K80 ***********
                +--------U--------+
       MCLR/RE3	|1              40| RB7  PICKit & LED Bit 7
      POT - RA0	|2              39| RB6  PICKit & LED Bit 6
Light Sens  RA1	|3              38| RB5  LED Bit 5
Analog 2    RA2	|4              37| RB4  LED Bit 4
Analog 3    RA3	|5              36| RB3  LED Bit 3
       VDD Core |6              35| RB2  LED Bit 2
Switch S2   RA5	|7              34| RB1  LED Bit 1
<not used>  RE0	|8              33| RB0  LED Bit 0 & Switch S5
<not used>  RE1	|9              32| VDD
<not used>  RE2	|10             31| GND
            VDD	|11             30| RD7 (Rx2)-> Serial Data OUT*  Bluetooth
            GND	|12             29| RD6 (Tx2)-> Serial Clock OUT* Bluetooth
  Switch S4 RA7	|13             28| RD5 To LCD
  Switch S3 RA6	|14             27| RD4 To LCD
  To LCD    RC0	|15             26| RC7 (Rx1)-> Serial Data OUT*  USB
  To LCD    RC1	|16             25| RC6 (Tx1)-> Serial Clock OUT* USB
  Buzzer    RC2	|17             24| RC5 <Not Used>
  I2C SCLK  RC3	|18             23| RC4 I2C SDA
  To LCD    RD0	|19             22| RD3 To LCD
  To LCD    RD1	|20             21| RD2 To LCD
                +-----------------+
* Note:  RA4 not available since used by VDD Core!
* for configurations of master synch serial port, see 18.3 of PDF
* YES - Rx IS SERIAL OUT!!!!

*/
#include <stdio.h>
#include <stdlib.h>
#include <p18f46K80.h>
#include <delays.h>
#include <timers.h>
#include <usart.h>
#include <i2c.h>
#include <adc.h>
#include <pwm.h>

#pragma config FOSC = INTIO2    /* internal osc enable */
#pragma config FCMEN = OFF      /* Failsafe Clock Monitor Disabled */
#pragma config WDTEN = OFF      /* no Dog */
#pragma config XINST = OFF      /* Extended Instruction Set off */
#pragma config SOSCSEL = DIG    /* Configures PORTC0&1 for digital I/O, for LCD */

/*******************************************************************************
 * Global Data Definitions
 ******************************************************************************/

/* LCD command byte values for use with LCD_command() function */
#define LCD_CURSOR_LINE1    0x87    /* set cursor to line 1 start (position 7) */
#define LCD_CURSOR_LINE2    0xC3    /* set cursor to line 2 start (posotion 3) */
#define LCD_CURSOR_LEFT     0x10    /* move cursor left */
#define LCD_CURSOR_RIGHT    0x14    /* move cursor right */

/* names for elements in RGB_vals array */
enum colors { RED, GREEN, BLUE };

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

int i;                              /* loop variable */

unsigned char input_char;           /* character received via USART */

unsigned char raw_lux;              /* raw A/D value from light sensor */
unsigned int lux;                   /* lux value from light sensor */

/* array holding intensity values of R, G, and B channels for BlinkM LED */
unsigned char RGB_vals[3] = { 0, 0, 0 };
unsigned char color = RED;          /* variable to indicate current LED color */
unsigned char brightness = 0;       /* variable to indicate current LED brightness */
unsigned char brightness_string[4]; /* brightness string received from user */

unsigned char byte_string[3];       /* hex byte string received from user */
unsigned char byte;                 /* byte value received from user */

unsigned char LCD_string[17];       /* user-entered string to be displayed on LCD */

/* variables for reading TC74 temperature sensor */
int temperature;        /* temperature value */
char read_data = 0;     /* byte received from sensor */
unsigned char cmd = 1;  /* Select TC74 Config Reg. */

/*******************************************************************************
 * Global Function Prototypes
 ******************************************************************************/

/* LCD functions defined in BulldogLCD.asm */
extern void LCDInit(void);      /* initialize LCD */
extern void LCDLine_1(void);    /* set LCD cursor to line 1 */
extern void LCDLine_2(void);    /* set LCD cursor to line 2 */
extern void lcd_clr(void);      /* clear LCD */
extern void i_write(void);      /* send primitive command to LCD */
extern void d_write(void);      /* send ASCII character to LCD */
extern unsigned int temp_wr;    /* used to pass parameters to LCD functions */

void LCD_command( char command );   /* send a command to the LCD */

/* get new Lux value from light sensor & print it via USART */
void print_lux( void );

/* accept color & brightness from user & update BlinkM LED */
void color_change( void );

/* write new RGB values to BlinkM LED */
void blinkm_update( void );

/* accept byte value in hexadecimal and display it on Port B LEDs */
void LED_byte( void );

/* accept string to display on LCD */
void LCD_display_string( void );

/* print temperature in degrees Celsius */
void print_temperature( void );

/* read temperature from TC74 via I2C */
void read_temperature( void );

/* turn musical note on */
void note_on( void );

/* turn musical note off */
void note_off( void );

/*******************************************************************************
 * Main
 ******************************************************************************/

void main( void )
{
    OSCCON = 0x60;  /* set to 8 MHz base clock internal (default clock) */
    TRISB = 0;      /* PORT B all outputs for LEDs */
    LATB = 0;       /* All outputs off initially for safety */
    TRISA = 0xFF;   /* PORT A all inputs */
    
    /* configure I2C */
    OpenI2C( MASTER, SLEW_OFF );
    /* I2C baud rate:
     * 100 KHz for TC-74
     * baud = Fosc/(4*(SSPADD + 1))
     */ 
    SSPADD = 9;
    
    /* initialize BlinkM to 'off' state */
    StartI2C();                 /* generate I2C start condition */
    while( SSPCON2bits.SEN );   /* wait for start */
    while( WriteI2C(0x12) );    /* Send BlinkM address (Write; LSB=0) */
    while( WriteI2C('o') );     /* stop script */
    while( WriteI2C('n') );     /* go to RGB value now */
    while( WriteI2C(0) );       /* red value */
    while( WriteI2C(0) );       /* green value */
    while( WriteI2C(0) );       /* blue value */
    StopI2C();                  /* generate I2C stop condition */

    /* for analog / digital configurations, see reg. 23-8 pp. 356 of the pdf */
    ANCON0 = 0x0F;  /* AN0, AN1, AN2, AN3 configured as analog */
    ANCON1 = 0x00;  /* All other channels are digital */
    
    /* initialize and clear LCD */
    LCDInit();
    lcd_clr();
    
    /* configure ADC:
     * A/D clock = Fosc/64
     * Tacq = 20 Tad
     * Left justified result
     * Ch AD1 input (light sensor)
     * references Vdd and Vss
     */
    OpenADC( ADC_FOSC_64 & 
             ADC_20_TAD & 
             ADC_LEFT_JUST,
             ADC_CH1 & 
             ADC_REF_VDD_VDD & 
             ADC_REF_VDD_VSS, 0 );
    
    /* initialize USART */
    Open1USART (USART_TX_INT_OFF &
                USART_RX_INT_OFF &
            	USART_ASYNCH_MODE &
            	USART_EIGHT_BIT &
            	USART_CONT_RX &
            	USART_BRGH_LOW, 12);    /* baud rate = 9600 @ 8MHz Fosc */
    
    /* buzzer pin set to input (buzzer off) */
    TRISCbits.TRISC2 = 1;
    /* set PWM period: 0xFF = 488Hz with Fosc = 8MHz and timer prescaler = 16 */
    PR2 = 0xFF;
    /* set PWM duty: 0x80 = 50% duty */
    CCPR2L = 0x80;
    /* configure CCP2 for PWM mode */
    CCP2CON = 0x0F;
    /* open Timer2 for PWM, with prescaler = 16 */
    OpenTimer2( TIMER_INT_OFF & T2_PS_1_16 & T2_POST_1_1 );
    
    /* display initial prompt via USART */
    putrs1USART( (const far rom char *)"ECNS-424 In Depth Lab #4, by Andy Roggenbuck\r\n" );

    while(1)
    {
        /* prompt user for input */
        putrs1USART( (const far rom char *)"Waiting for a command...\r\n\n");
        
        while( !DataRdy1USART() );      /* wait for input character */
        input_char = getc1USART();      /* get input character */

        /* respond to input character */
        switch( input_char )
        {
            case 'L':
            case 'l':
                /* print lux value */
                print_lux();
                break;
            case 'B':
            case 'b':
                /* accept color & brightness and update BlinkM */
                color_change();
                break;
            case 'D':
            case 'd':
                /* accept byte value and display it on LEDs.
                 * Then print "Byte x on LEDs" */
                LED_byte();
                break;
            case 'N':
            case 'n':
                /* turn musical note on */
                note_on();
                break;
            case 'O':
            case 'o':
                /* turn musical note off */
                note_off();
                break;
            case 'S':
            case 's':
                /* accept string & display it on LCD */
                LCD_display_string();
                break;
            case 'T':
            case 't':
                /* print temperature value in degrees Celsius */
                print_temperature();
                break;
            default:
                /* print list of acceptable commands */
                putrs1USART( (const far rom char *)
                        "Available commands:\r\n\n"
                        "L - Display current illuminance value in Lux\r\n"
                        "B - Enter new color and intensity for BlinkM LED\r\n"
                        "D - Enter byte value to be displayed on Port B LEDs\r\n"
                        "N - Turn note on\r\n"
                        "O - Turn note off\r\n"
                        "S - Enter string to be displayed on LCD\r\n"
                        "T - Display current temperature value in degrees Celsius\r\n\n" );
                break;
        }
    }
}

/* Function: print_lux
 * Input/Output: none
 * 
 * Gets new lux value from light sensor and prints it via USART
 */
void print_lux( void )
{
    /* get new A/D value from light sensor */
    ConvertADC();               /* start A/D conversion */
    while( BusyADC() );         /* wait for conversion to finish */
    raw_lux = ADRESH;           /* store 8-bit result */
    
    /* convert light sensor A/D value to Lux */
    lux = (unsigned int) ((float) raw_lux * 5 / 0.00224 / 256);
    
    /* print lux value via USART */
    fprintf( _H_USART, "%d Lux\r\n\n", lux );
}

/* Function: color_change
 * Input/Output: none
 * 
 * Prompts user for color and brightness values, updates BlinkM LED accordingly
 */
void color_change( void )
{
    /* prompt user for color */
    putrs1USART( (const far rom char *)
            "Choose color:\r\n\n"
            "R - Red\r\n"
            "G - Green\r\n"
            "B - Blue\r\n\n");
    
    while( !DataRdy1USART() );      /* wait for input character */
    input_char = getc1USART();      /* get input character */
    
    /* echo input character to USART */
    fprintf( _H_USART, "%c\r\n\n", input_char );
    
    /* set color according to input character */
    switch ( input_char )
    {
        case 'R':
        case 'r':
            color = RED;            /* set color to red */
            break;
        case 'G':
        case 'g':
            color = GREEN;          /* set color to green */
            break;
        case 'B':
        case 'b':
            color = BLUE;           /* set color to blue */
            break;
        default:
            break;
    }
    
    /* initialize brightness_string to all 0s */
    for( i = 0; i < 3; i++ )
    {
        brightness_string[i] = '0';
    }
    brightness_string[4] = NULL;
    
    /* prompt user for brightness value */
    putrs1USART( (const far rom char *)
            "Enter brightness value from 000-255:\r\n\n");
    
    /* get string via USART */
    for( i = 0; i < 3; i++ )
    {
        while( !DataRdy1USART() );      /* wait for input character */
        input_char = getc1USART();      /* get input character */
        
        /* store character in LCD_string */
        brightness_string[i] = input_char;
        /* echo character to USART */
        putc1USART( input_char );
    }
    
    /* line break */
    putrs1USART( (const far rom char *) "\r\n\n" );
    
    brightness = atoi( brightness_string ); /* set brightness from string */
    
    blinkm_update();                    /* update BlinkM with new values */
}

/* Function: blinkm_update
 * Input/Output: none
 * 
 * Updates values in RGB_vals[] based on currently selected color and brightness.
 * Then updates BlinkM LED based on values in RGB_vals[]
 */
void blinkm_update( void )
{
    /* set currently selected color to brightness value from potentiometer */
    RGB_vals[ color ] = brightness;

    /* set the other two colors to zero (off) */
    RGB_vals[ (color + 1) % 3 ] = 0;
    RGB_vals[ (color + 2) % 3 ] = 0;
        
    StartI2C();                         /* generate I2C start condition */
    while( SSPCON2bits.SEN );           /* wait for start */
    while( WriteI2C(0x12) );            /* Send BlinkM address (Write; LSB=0) */
    while( WriteI2C('n') );             /* go to RGB value now */
    while( WriteI2C(RGB_vals[RED]) );   /* red value */
    while( WriteI2C(RGB_vals[GREEN]) ); /* green value */
    while( WriteI2C(RGB_vals[BLUE]) );  /* blue value */
    StopI2C();                          /* generate I2C stop condition */
}

/* Function: LED_byte
 * Input/Output: none
 * 
 * Accept hexadecimal byte value from user & display it on Port B LEDs 
 */
void LED_byte( void )
{
    /* prompt user for brightness value */
    putrs1USART( (const far rom char *)
            "Enter hexadecimal value from 00-FF:\r\n\n");
    
    /* get string via USART */
    for( i = 0; i < 2; i++ )
    {
        while( !DataRdy1USART() );      /* wait for input character */
        input_char = getc1USART();      /* get input character */
        
        /* store character in LCD_string */
        byte_string[i] = input_char;
        /* echo character to USART */
        putc1USART( input_char );
    }
    
    /* line break */
    putrs1USART( (const far rom char *) "\r\n\n" );
    
    byte = 0;                           /* clear byte value */
    
    /* convert hex string to byte value */
    for (i = 0; i < 2; i++ )
    {
        switch ( byte_string[i] )
        {
            case '0':
                byte |= (char) (0x00 << (4 * (1 - i)));
                break;
            case '1':
                byte |= (char) (0x01 << (4 * (1 - i)));
                break;
            case '2':
                byte |= (char) (0x02 << (4 * (1 - i)));
                break;
            case '3':
                byte |= (char) (0x03 << (4 * (1 - i)));
                break;
            case '4':
                byte |= (char) (0x04 << (4 * (1 - i)));
                break;
            case '5':
                byte |= (char) (0x05 << (4 * (1 - i)));
                break;
            case '6':
                byte |= (char) (0x06 << (4 * (1 - i)));
                break;
            case '7':
                byte |= (char) (0x07 << (4 * (1 - i)));
                break;
            case '8':
                byte |= (char) (0x08 << (4 * (1 - i)));
                break;
            case '9':
                byte |= (char) (0x09 << (4 * (1 - i)));
                break;
            case 'A':
            case 'a':
                byte |= (char) (0x0A << (4 * (1 - i)));
                break;
            case 'B':
            case 'b':
                byte |= (char) (0x0B << (4 * (1 - i)));
                break;
            case 'C':
            case 'c':
                byte |= (char) (0x0C << (4 * (1 - i)));
                break;
            case 'D':
            case 'd':
                byte |= (char) (0x0D << (4 * (1 - i)));
                break;
            case 'E':
            case 'e':
                byte |= (char) (0x0E << (4 * (1 - i)));
                break;
            case 'F':
            case 'f':
                byte |= (char) (0x0F << (4 * (1 - i)));
                break;
            default:
                break;
        }
    }
    
    LATB = byte;
    
    /* print command confirmation via USART */
    fprintf( _H_USART, "Byte 0x%c%c on LEDs\r\n\n", (char)byte_string[0], (char)byte_string[1] );
}

/* Function: note_on
 * Input/Output: none
 * 
 * Turn on musical note by setting buzzer pin as output
 */
void note_on( void )
{
    TRISCbits.TRISC2 = 0;
    putrs1USART( (const far rom char *) "Note on\r\n\n" );
}

/* Function: note_off
 * Input/Output: none
 * 
 * Turn off musical note by setting buzzer pin as input
 */
void note_off( void )
{
    TRISCbits.TRISC2 = 1;
    putrs1USART( (const far rom char *) "Note off\r\n\n" );
}

/* Function: LCD_display_string
 * Input/Output: none
 * 
 * Prompts user to input string, displays string on LCD
 */
void LCD_display_string( void )
{
    /* initialize LCD_string to all spaces */
    for( i = 0; i < 16; i++ )
    {
        LCD_string[i] = ' ';
    }
    LCD_string[16] = NULL;
    
    /* prompt user to input string */
    putrs1USART( (const far rom char *)
            "Enter string (16 characters max):\r\n\n");
    
    /* get string via USART */
    for( i = 0; i < 16; i++ )
    {
        while( !DataRdy1USART() );      /* wait for input character */
        input_char = getc1USART();      /* get input character */
        
        /* exit loop if 'enter' pressed */
        if( input_char == '\r' )
        {
            break;
        }
        
        /* store character in LCD_string */
        LCD_string[i] = input_char;
        /* echo character to USART */
        putc1USART( input_char );
    }
    
    /* display LCD_string on LCD */
    LCDLine_1();                    /* set LCD cursor to line 1 */
    for( i = 0; i < 16; i++ )
    {
        temp_wr = LCD_string[i];
        d_write();                  /* write next character to display */
    }
    
    putrs1USART( (const far rom char *) "\r\n\n" );
}

/* Function: print_temperature
 * Input/Output: none
 * 
 * Gets temperature value from TC74 temp sensor and prints it via USART
 */
void print_temperature( void )
{
    /* read new temperature value from TC74 */
    read_temperature();
    
    /* print temperature value via USART */
    fprintf(_H_USART, "Temperature: %d%cC\r\n\n", temperature, (unsigned char)248 );
}

/* Function: read_temperature
 * Input/Output: none
 * 
 * Reads new temperature value from TC74 temperature sensor and stores it in
 * temperature variable
 */
void read_temperature( void )
{
    cmd = 1;                /* command: read config register from TC74 */
    
    /* check TC74 config register until it shows new temp value is ready */
    while( cmd == 1 )
    {
        StartI2C();             /* generate I2C start condition */
        while(SSPCON2bits.SEN); /* wait for start */

        while(WriteI2C(0x9A));  /* Send TC74 address (Write; LSB=0) */

        while(WriteI2C(cmd));   /* Select TC74 config register */

        IdleI2C();              /* idle i2c */
        RestartI2C();           /* Restart i2c */
        IdleI2C();              /* idle i2c */

        while(WriteI2C(0x9B));  /* Send TC74 address (Read; LSB=1) */

        read_data = ReadI2C();  /* Read config register from TC74 */

        NotAckI2C();            /* NACK */
        IdleI2C();              /* idle i2c */
        StopI2C();              /* stop i2c */

        /* if TC74 temp value is ready, set cmd to 0 */
        cmd = (read_data & 0x40) ? 0 : 1 ;
    }
    
    StartI2C();             /* generate I2C start condition */
    while(SSPCON2bits.SEN); /* wait for start */

    while(WriteI2C(0x9A));  /* Send TC74 address (Write; LSB=0) */
    
    while(WriteI2C(cmd));   /* Select TC74 temp register */

    IdleI2C();              /* idle i2c */
    RestartI2C();           /* Restart i2c */
    IdleI2C();              /* idle i2c */

    while(WriteI2C(0x9B));  /* Send TC74 address (Read; LSB=1) */

    read_data = ReadI2C();  /* Read temp register from TC74 */

    NotAckI2C();            /* NACK */
    IdleI2C();              /* idle i2c */
    StopI2C();              /* stop i2c */
    
    /* store temperature value */
    temperature = read_data;
}

/* Function: LCD_command
 * Input: 8-bit primitive command value to be sent to LCD
 * Output: value sent to LCD via i_write function
 * 
 * This is just used to send primitive commands to LCD with a single line of code
 */
void LCD_command( char command )
{
    /* send command to LCD */
    temp_wr = command;
    i_write();
}