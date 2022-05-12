noecho
\ Test interrupts

DECIMAL

46 CONSTANT irq
34 CONSTANT vector

: CN_ENABLE ( bit port -- )
	CNENA REG_BY_OFFSET
						\ CHANGE NOTIFICATION ENABLE register
	reg_bit_set			\ set to enabled
;

: CN_DISABLE ( bit port -- )
	\ CHANGE NOTIFICATION DISABLE register
	CNENA REG_BY_OFFSET reg_bit_clear
;

: CN_STATUS ( bit port -- value )
	CNSTATA REG_BY_OFFSET reg_bit_read
;

: CN_CLEAR ( bit port -- value )
	CNSTATA REG_BY_OFFSET reg_bit_clear
;

: CN_START ( port -- )
	15 swap
	CNCONA REG_BY_OFFSET reg_bit_set
;

: CN_STOP ( port -- )
	15 swap
	CNCONA REG_BY_OFFSET reg_bit_clear
;

\ read bit and the change notification -- reading bit clears the flag
: check_cn ( ) 
	DIO_1 CN_STATUS .
	DIO_1 reg_bit_read .
	CR
;



DIO_1 DIGITAL_IN		\ PB on pin 11 as input
DIO_1 PULL_UP
DIO_1 CN_ENABLE
DIO_1 CN_CLEAR

PORTB CN_START

\ f CNCONa REG_BIT_SET	\ enable CN (for port b)

DIO_1 CN_STATUS .



\ show the interrupt flag being toggled - Address is for IFS1
\ bf881040 @ .
\ e bff881040 reg_bit_clear
\ press button
\ bf881040 @ .



: test_int  ( )
	." INT processed"
	.S
	check_cn
	
	irq ICLR
\	IRET
;

\ set up interrupt
1 5 vector IPRI
vector ' test_int IREG
irq ICLR
irq IEN

2 LOG

echo
