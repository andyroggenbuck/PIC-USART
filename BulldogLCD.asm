;************************************************************************
;* LCD Routines as modified by R. Most for the Bulldog PIC++            *
;/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*
;* Modified 10-7-2018                                                   *
;* Version 1.0 10-7-2018 Adopotion of PORT C0 and C1 for Enable and     *
;*     BJT power on respectively.  Code was also considerably cleaned   *
;*     up with the removal of PORTA manipulations and the cleansing of  *
;*     improper use of "PORTx" with the proper use of "LATx" which      *
;*     caused considerable problems with processor migration and made   *
;*     the code much more transportable.                                *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;************************************************************************

	list p=18f46K80
	#include p18f46K80.inc

#define	LCD_D4		LATD, 0	; LCD data bits
#define	LCD_D5		LATD, 1
#define	LCD_D6		LATD, 2
#define	LCD_D7		LATD, 3

#define	LCD_D4_DIR	TRISD, 0	; LCD data bits
#define	LCD_D5_DIR	TRISD, 1
#define	LCD_D6_DIR	TRISD, 2
#define	LCD_D7_DIR	TRISD, 3

#define	LCD_E		LATC, 0	; LCD E clock <BulldogPIC RM>
#define	LCD_RW		LATD, 5	; LCD read/write line
#define	LCD_RS		LATD, 4	; LCD register select line

#define	LCD_E_DIR	TRISC, 0    ; <BulldogPIC RM>
#define	LCD_RW_DIR	TRISD, 5
#define	LCD_RS_DIR	TRISD, 4

#define	LCD_INS		0
#define	LCD_DATA	1

D_LCD_DATA	UDATA_ACS
COUNTER		res	1
delay		res	1
temp_wr		res	1
temp_rd		res	1
clrcnt		res 1

	GLOBAL	temp_wr

PROG1	CODE

;***************************************************************************

lcd_clr:					; added by R. Most 2/6/2007
	movlw 0xf				; 15
	movwf clrcnt			; counter
	call LCDLine_1			; 1st line
here:
	movlw " "				; blank
	movwf temp_wr			; move into place
	call d_write			;
	decfsz clrcnt			; loop back again
	goto here				;
	call end_line			;

	movlw 0xf				; 15
	movwf clrcnt			; counter
	call LCDLine_2			; 2nd line
here2:
	movlw " "				; blank
	movwf temp_wr			; move into place
	call d_write			;
	decfsz clrcnt			; loop back again
	goto here2				;
	call end_line			;
	return					; done
	GLOBAL lcd_clr

end_line					; added by R. Most 1/25/2007
;	movlw	"\n"			;move data into TXREG
;	movwf	TXREG			;next line
;	btfss	TXSTA,TRMT		;wait for data TX
;	goto	$-2
;	movlw	"\r"			;move data into TXREG
;	movwf	TXREG			;carriage return
;	btfss	TXSTA,TRMT		;wait for data TX
;	goto	$-2

	return
	GLOBAL end_line


LCDLine_2
	movlw	0xC0
	movwf	temp_wr
	rcall	i_write
	return
	GLOBAL	LCDLine_2

d_write

	rcall	LCDBusy
	bsf	STATUS, C
	rcall	LCDWrite
	return
	GLOBAL	d_write

LCDLine_1
	movlw	0x80
	movwf	temp_wr
	rcall	i_write
	return
	GLOBAL	LCDLine_1

	;write instruction
i_write
	rcall	LCDBusy
	bcf	STATUS, C
	rcall	LCDWrite
	return
 	GLOBAL	i_write


rlcd	macro	MYREGISTER
 IF MYREGISTER == 1
	bsf	STATUS, C
	rcall	LCDRead
 ELSE
	bcf	STATUS, C
	rcall	LCDRead
 ENDIF
	endm
;****************************************************************************




; *******************************************************************
LCDInit
	bsf		LATC,1      ; <BulldogPIC RM>
	bcf		TRISC,1     ; <BulldogPIC RM>
	bsf		LATC,1      ; <BulldogPIC RM>
	call	Delay30ms

	bcf	LCD_E_DIR		;configure control lines
	bcf	LCD_RW_DIR
	bcf	LCD_RS_DIR

	movlw	0xff			; Wait ~15ms @ 20 MHz
	movwf	COUNTER
lil1
	movlw	0xFF
	movwf	delay
	rcall	DelayXCycles
	decfsz	COUNTER,F
	bra	lil1

	movlw	b'00100000'		;#1 Send control sequence
	movwf	temp_wr
	bcf	STATUS,C
	rcall	LCDWriteNibble

	movlw	0xff			;Wait ~4ms @ 20 MHz
	movwf	COUNTER
lil2
	movlw	0xFF
	movwf	delay
	rcall	DelayXCycles
	decfsz	COUNTER,F
	bra	lil2

	movlw	b'00100000'		;#2 Send control sequence
	movwf	temp_wr
	bcf	STATUS,C
	rcall	LCDWriteNibble

	movlw	0xFF			;Wait ~100us @ 20 MHz
	movwf	delay
	rcall	DelayXCycles

	movlw	b'10000000'		;
	movwf	temp_wr
	bcf	STATUS,C
	rcall	LCDWriteNibble

	movlw	0xFF			;Wait ~100us @ 20 MHz
	movwf	delay
	rcall	DelayXCycles

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;0C--0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	movlw	b'00000000'		;#4 set 4-bit
	movwf	temp_wr
	bcf	STATUS,C
	rcall	LCDWriteNibble
;0x08--08
	movlw	b'11000000'		;#4 set 4-bit
	movwf	temp_wr
	bcf	STATUS,C
	rcall	LCDWriteNibble

	movlw	0xFF			;Wait ~100us @ 20 MHz
	movwf	delay
	rcall	DelayXCycles
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;0x01
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;0x01-0
	movlw	b'00000000'		;#4 set 4-bit
	movwf	temp_wr
	bcf	STATUS,C
	rcall	LCDWriteNibble
;0x01--01
	movlw	b'00010000'		;#4 set 4-bit
	movwf	temp_wr
	bcf	STATUS,C
	rcall	LCDWriteNibble

	movlw	0xFF			;Wait ~100us @ 20 MHz
	movwf	delay
	rcall	DelayXCycles
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	call	LongDelay ;2ms
	call	LongDelay ;2ms
	call	LongDelay ;2ms
	call	LongDelay ;2ms
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;0x02
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;0x0-0
	movlw	b'00000000'		;#4 set 4-bit
	movwf	temp_wr
	bcf	STATUS,C
	rcall	LCDWriteNibble
;0x02
	movlw	b'00100000'		;#4 set 4-bit
	movwf	temp_wr
	bcf	STATUS,C
	rcall	LCDWriteNibble

	movlw	0xFF			;Wait ~100us @ 20 MHz
	movwf	delay
	rcall	DelayXCycles
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	rcall	LCDBusy			;Busy?
	call	LongDelay ;2ms
	call	LongDelay ;2ms
	call	LongDelay ;2ms
	call	LongDelay ;2ms
	call	LongDelay ;2ms
	call	LongDelay ;2ms
	call	LongDelay ;2ms

	movlw	0x4E
	movwf	temp_wr
	rcall	LCDBusy
	bsf	STATUS, C
	rcall	LCDWrite



	movlw	0xFF			;Wait ~100us @ 20 MHz
	movwf	delay
	rcall	DelayXCycles
	return
	GLOBAL	LCDInit
; *******************************************************************


;****************************************************************************
;     _    ______________________________
; RS  _>--<______________________________
;     _____
; RW       \_____________________________
;                  __________________
; E   ____________/                  \___
;     _____________                ______
; DB  _____________>--------------<______
;
LCDWriteNibble
	btfss	STATUS, C		; Set the register select
	bcf	LCD_RS
	btfsc	STATUS, C
	bsf	LCD_RS

	bcf	LCD_RW			; Set write mode

	bcf	LCD_D4_DIR		; Set data bits to outputs
	bcf	LCD_D5_DIR
	bcf	LCD_D6_DIR
	bcf	LCD_D7_DIR

	NOP				; Small delay
	NOP

	bsf	LCD_E			; Setup to clock data

	btfss	temp_wr, 7			; Set high nibble
	bcf	LCD_D7
	btfsc	temp_wr, 7
	bsf	LCD_D7
	btfss	temp_wr, 6
	bcf	LCD_D6
	btfsc	temp_wr, 6
	bsf	LCD_D6
	btfss	temp_wr, 5
	bcf	LCD_D5
	btfsc	temp_wr, 5
	bsf	LCD_D5
	btfss	temp_wr, 4
	bcf	LCD_D4
	btfsc	temp_wr, 4
	bsf	LCD_D4

	NOP
	NOP

	bcf	LCD_E			; Send the data

	return
; *******************************************************************


; *******************************************************************
LCDWrite

	rcall	LCDWriteNibble
	swapf	temp_wr,F
	rcall	LCDWriteNibble
	swapf	temp_wr,F

	return

	GLOBAL	LCDWrite
; *******************************************************************



; *******************************************************************
;     _____    _____________________________________________________
; RS  _____>--<_____________________________________________________
;               ____________________________________________________
; RW  _________/
;                  ____________________      ____________________
; E   ____________/                    \____/                    \__
;     _________________                __________                ___
; DB  _________________>--------------<__________>--------------<___
;
LCDRead
	bsf	LCD_D4_DIR		; Set data bits to inputs
	bsf	LCD_D5_DIR
	bsf	LCD_D6_DIR
	bsf	LCD_D7_DIR

	btfss	STATUS, C		; Set the register select
	bcf	LCD_RS
	btfsc	STATUS, C
	bsf	LCD_RS

	bsf	LCD_RW			;Read = 1

	NOP
	NOP

	bsf	LCD_E			; Setup to clock data

	NOP
	NOP
	NOP
	NOP

	btfss	LCD_D7			; Get high nibble
	bcf	temp_rd, 7
	btfsc	LCD_D7
	bsf	temp_rd, 7
	btfss	LCD_D6
	bcf	temp_rd, 6
	btfsc	LCD_D6
	bsf	temp_rd, 6
	btfss	LCD_D5
	bcf	temp_rd, 5
	btfsc	LCD_D5
	bsf	temp_rd, 5
	btfss	LCD_D4
	bcf	temp_rd, 4
	btfsc	LCD_D4
	bsf	temp_rd, 4

	bcf	LCD_E			; Finished reading the data

	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP

	bsf	LCD_E			; Setup to clock data

	NOP
	NOP

	btfss	LCD_D7			; Get low nibble
	bcf	temp_rd, 3
	btfsc	LCD_D7
	bsf	temp_rd, 3
	btfss	LCD_D6
	bcf	temp_rd, 2
	btfsc	LCD_D6
	bsf	temp_rd, 2
	btfss	LCD_D5
	bcf	temp_rd, 1
	btfsc	LCD_D5
	bsf	temp_rd, 1
	btfss	LCD_D4
	bcf	temp_rd, 0
	btfsc	LCD_D4
	bsf	temp_rd, 0

	bcf	LCD_E			; Finished reading the data

FinRd
	return
; *******************************************************************

; *******************************************************************
LCDBusy
	call	LongDelayLast
;	call	LongDelay
	return
					; Check BF
	rlcd	LCD_INS
	btfsc	temp_rd, 7
	bra	LCDBusy
	return

	GLOBAL	LCDBusy
; *******************************************************************

; *******************************************************************
DelayXCycles
	decfsz	delay,F
	bra	DelayXCycles
	return
; *******************************************************************

Delay1ms			;Approxiamtely at 4Mhz
	clrf	delay
Delay_1
	nop
	decfsz	delay
	goto	Delay_1
	return

Delay30ms	;more than 30 at 4 Mhz
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms

	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms

	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	return
LongDelay:
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	return

LongDelayLast
	call	Delay1ms
	call	Delay1ms
	call	Delay1ms
	return

	END