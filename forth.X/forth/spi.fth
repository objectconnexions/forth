noecho

0x0bf80fa90 CONSTANT SDI2R
0x03		CONSTANT RPB13
0x0bf80fb8c CONSTANT RPC8R
0x04		CONSTANT SDO2

0x0bf805a00 CONSTANT SPI2CON
0x0bf805a10 CONSTANT SPI2STAT
0x0bf805a20 CONSTANT SPI2BUF
0x0bf805a30 CONSTANT SPI2BRG
0x0bf805a40 CONSTANT SPI2CON2


: WRITE_SPI ( send -- recv )
	SPI2BUF !
	BEGIN
		SPI2STAT 0 BIT_READ 	\ SPIRBF: RX Buffer Full bit
		0<>						\ until buffer full
	UNTIL
	SPI2BUF @
;

: WRITE_DROP_SPI ( send -- )
	WRITE_SPI
	DROP
;

: READ_SPI ( -- value )
	0x0ff WRITE_SPI
\	1 SPI2BUF !
\	SPI2BUF @
;


: CHIP_ENABLE ( address bit -- )
	port_off	\ pull line low to enable ID
;

: CHIP_DISABLE ( address bit -- )
	port_on		\ pull line high to disable ID
;

: ID_CHIP ( -- address bit )
	PORTB 5
;

: EEPROM_CHIP ( -- address bit )
	PORTA 10
;



: ENABLE_ID_CHIP ( -- )
	ID_CHIP CHIP_ENABLE	\ pull B5 low to enable ID
;

: DISABLE_ID_CHIP ( -- )
	ID_CHIP CHIP_DISABLE	\ pull B5 high to disable ID
;

: ENABLE_EEPROM_CHIP ( -- )
	EEPROM_CHIP CHIP_ENABLE   \ pull A10 low to enable ID
;

: DISABLE_EEPROM_CHIP ( -- )
	EEPROM_CHIP CHIP_DISABLE 	\ pull A10 high to disable ID
;

\ ##

: WRITE_ENABLE_EEPROM_CHIP ( -- )
	ENABLE_EEPROM_CHIP
	0x06 WRITE_SPI	
	DISABLE_EEPROM_CHIP
;

: WRITE_DISABLE_EEPROM_CHIP ( -- )
	ENABLE_EEPROM_CHIP
	0x04 WRITE_SPI	
	DISABLE_EEPROM_CHIP
;


\ ##

: INIT_SPI ( -- )
	PORTC 7 2dup
	digital_out				\ A7 (!POWER_SAVE) as digital output
	port_on					\ turn peripheral power on

	RPB13 SDI2R !			\ use RPB13 for SDI
	SDO2 RPC8R !			\ use RPC8 for SDO
	PORTC 8 digital_out
	PORTB 13 digital_in

	PORTB 5 2dup
	digital_out				\ B5 (!CE) as digital output
	port_on					\ disable CE for ID chip
	
	PORTA 9 2dup
	digital_out				\ A9 (!CE) as digital output
	port_on					\ disable flash

	PORTA 10 2dup
	digital_out				\ A10 (!CE) as digital output
	port_on					\ disable eeprom
	
	\ set up SPI module
	0 SPI2CON !				\ reset all
	SPI2CON 15	BIT_CLR		\ turn off SPI
	SPI2CON 5	BIT_SET		\ set to master mode
	SPI2CON 6	BIT_SET		\ set CKP to clock idle high
	SPI2CON 8	BIT_SET		\ set CKE to output data on active-to-idle clock
	SPI2CON 9	BIT_SET		\ set SMP to sample data at end?
	50 SPI2BRG !			\ use FBP/4 clock frequency
	\ READ_SPI				\ clear buffer
	SPI2BUF @
	SPI2CON 16	BIT_CLR		\ Clear the ENHBUF bit
	SPI2STAT 6	BIT_CLR		\ clear SPIROV flag
	SPI2CON 15	BIT_SET		\ enable SPI
;

: READ_ID ( -- )
	ENABLE_ID_CHIP
	0x03 WRITE_DROP_SPI			\ read
	0x0fa WRITE_DROP_SPI		\ serial number address
	READ_SPI .HEX
	READ_SPI .HEX
	READ_SPI .HEX
	READ_SPI .HEX
	READ_SPI .HEX
	READ_SPI .HEX
	DISABLE_ID_CHIP
	CR
;


: TEST_ID ( -- )
	ENABLE_ID_CHIP
	0x03 WRITE_DROP_SPI			\ read
	0x0fa WRITE_DROP_SPI		\ serial number address
	READ_SPI
	DISABLE_ID_CHIP

	SPI2STAT @ .HEX CR
 	. CR
	SPI2STAT @ .HEX CR
;

: TEST_EEPROM ( -- )
	WRITE_ENABLE_EEPROM_CHIP
	ENABLE_EEPROM_CHIP
	0x02 WRITE_DROP_SPI			\ write mode
	0x00 WRITE_DROP_SPI			\ address high byte
	0x00 WRITE_DROP_SPI			\ address low byte

	0x12 WRITE_DROP_SPI 			\ write 4 bytes
	0x34 WRITE_DROP_SPI
	0x56 WRITE_DROP_SPI
	0x78 WRITE_DROP_SPI
	DISABLE_EEPROM_CHIP
	WRITE_DISABLE_EEPROM_CHIP

	ENABLE_EEPROM_CHIP
	0x03 WRITE_DROP_SPI			\ read mode
	0x00 WRITE_DROP_SPI			\ address high byte
	0x00 WRITE_DROP_SPI			\ address low byte

	READ_SPI
	READ_SPI
	READ_SPI
	READ_SPI
	READ_SPI
	DISABLE_EEPROM_CHIP
	. . . . . CR

;


: READ_FROM_ID ( a -- n )
	ENABLE_ID_CHIP
	0x03 WRITE_DROP_SPI			\ read
	WRITE_DROP_SPI				\ serial number address
	READ_SPI
	DISABLE_ID_CHIP
;

: READ_FROM_EEPROM ( a -- n )
	ENABLE_EEPROM_CHIP
	0x03 WRITE_DROP_SPI			\ read
\ need to split address into two bytes
	WRITE_DROP_SPI				\ serial number address
	READ_SPI
	DISABLE_EEPROM_CHIP
;


: DEBUG_SPI ( -- ) 
\	 ." Status=" 
	SPI2STAT @ .HEX CR
\	." Buffer=" 
	SPI2BUF @ .HEX CR
\	." Control=" 
	SPI2CON @ .HEX CR
\	." Control 2=" 
	SPI2CON2 @ .HEX CR
\	." Status=" 
	SPI2BRG @ .HEX CR
;

echo
