
HEX

\ Calculate register address for an IRQ
: ~INT_FLAG_ADDR ( addr irq -- addr bit )
	DUP				\ addr irq irq --
	20 /			\ addr irq reg_no -- register no - 1 or 0
	10 *			\ addr irq offset -- address offset
	rot + 			\ irq addr -- new address
	swap			\ addr irq --
	20 mod			\ addr bit -- bit used for the flag 
	swap			\ bit addr
;

\ Enable an interrupt by IQR
: ENABLE_INT ( irq -- )
	IEC0 swap ~INT_FLAG_ADDR		\ address for specified flag
	REG_BIT_SET					\ set flag
;

\ Disable an interrupt by IQR
: DISABLE_INT ( irq -- )
	IEC0 swap ~INT_FLAG_ADDR		\ address for specified flag
	REG_BIT_CLEAR				\ reset flag
;

\ 1 on the stack if enabled; 0 if disabled.
: IS_ENABLED_INT ( irq -- flag )
	IEC0 swap ~INT_FLAG_ADDR
	REG_BIT_READ
;

: STATUS_INT ( irq -- flag )
	IFS0 swap ~INT_FLAG_ADDR
	REG_BIT_READ
;

: CLEAR_INT ( irq -- )
	IFS0 swap ~INT_FLAG_ADDR
	REG_BIT_CLEAR
;


: PRIORITY_INT ( vector priority subpriority -- )
					\ byte value for combined priority and subpriority
	swap
	2 LSHIFT +		\ vector priority --
	
	
					\ determine address and byte 
	swap					
	DUP				\ pri vec vec --
	4 / 10 *		\ calculate register offset
	IPC0 +			\ pri vec addr -- add to address to locate register for setting
	rot	rot			\ addr pri vec --
	
	4 mod			\ addr pri byte -- byte within register
	8 * 			\ addr pri shift -- shift amount
	
	swap over		\ address shift pri shift --
	lshift			\ addr shift regval -- (as part of word)
	
	swap
	1F swap			\ addr regval mask shift
	lshift			\ addr regval mask -- create the mask for the byte within the register
	
	rot	tuck		\ regval addr mask addr --
	REG_CLR + !		\ clear byte
	REG_SET + !		\ set new byte
;



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



DIO_1 DIGITAL_IN		\ PB on pin 11 as input
DIO_1 PULL_UP
DIO_1 CN_ENABLE
DIO_1 CN_CLEAR

f CNCONB REG_BIT_SET	\ enable CN (for port b)
\ 15 PORTB CNCONB REG_BY_OFFSET reg_bit_set
\ PORTB CNCON + 15 BIT_SET	\ enable change notification


DIO_1 CN_STATUS .
\ DIO_1 CNSTAT + 11 BIT_READ


\ show the interrupt flag being toggled - Address is for IFS1
\ bf881040 @ .
\ e bff881040 reg_bit_clear
\ press button
\ bf881040 @ .
