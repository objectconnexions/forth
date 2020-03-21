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
\	BEGIN
\		SPI2STAT 5 BIT_READ 	\ SPIRBE: RX FIFO Empty bit
\		0<>						\ until buffer full
\	UNTIL
	SPI2BUF @
;

: READ_SPI ( -- value )
	0xff WRITE_SPI
\	1 SPI2BUF !
\	SPI2BUF @
;

: ENABLE_ID_CHIP ( -- )
	PORTB 5 port_off	\ pull B5 low to enable ID
;

: DISABLE_ID_CHIP ( -- )
	PORTB 5 port_on	\ pull B5 high to disable ID
;

: ENABLE_EEPROM_CHIP ( -- )
	PORTA 10 port_off	\ pull A10 low to enable ID
;

: DISABLE_EEPROM_CHIP ( -- )
	PORTA 10 port_on	\ pull A10 high to disable ID
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
\	DISABLE_ID_CHIP
	
	\ set up SPI module
	SPI2CON 15	BIT_CLR		\ turn off SPI
	SPI2CON 5	BIT_SET		\ set to master mode
	SPI2CON 6	BIT_SET		\ set CKP to  clock idle high
	SPI2CON 8	BIT_CLR		\ set CKE to outpu data on active clock
	SPI2CON 9	BIT_CLR		\ set SMP to sample data at middle
	255 SPI2BRG !			\ use FBP/4 clock frequency
	\ READ_SPI				\ clear buffer
	SPI2BUF @
	SPI2CON 16	BIT_CLR		\ Clear the ENHBUF bit
	SPI2STAT 6	BIT_CLR		\ clear SPIROV flag
	SPI2CON 15	BIT_SET		\ enable SPI
;

: READ_ID ( -- )
	ENABLE_ID_CHIP
	0x03 WRITE_SPI			\ read
	0x0fc WRITE_SPI			\ serial number address
	READ_SPI
	READ_SPI
	READ_SPI
	READ_SPI
	READ_SPI
	READ_SPI
	DISABLE_ID_CHIP
	. . . . . . CR
;


: TEST_ID ( -- )
	ENABLE_ID_CHIP
	0x03 WRITE_SPI			\ read
	0x0fa WRITE_SPI			\ serial number address
	READ_SPI
	DISABLE_ID_CHIP

	SPI2STAT @ .HEX CR
 	. CR
	SPI2STAT @ .HEX CR
;

: TEST_EEPROM ( -- )
	WRITE_ENABLE_EEPROM_CHIP
	ENABLE_EEPROM_CHIP
	0x02 WRITE_SPI			\ write mode
	0x00 WRITE_SPI			\ address high byte
	0x00 WRITE_SPI			\ address low byte
	0x12 WRITE_SPI 			\ write 4 bytes
	0x34 WRITE_SPI
	0x56 WRITE_SPI
	0x78 WRITE_SPI
	DISABLE_EEPROM_CHIP
	WRITE_DISABLE_EEPROM_CHIP

	ENABLE_EEPROM_CHIP
	0x03 WRITE_SPI			\ read mode
	0x00 WRITE_SPI			\ address high byte
	0x00 WRITE_SPI			\ address low byte
	READ_SPI
	READ_SPI
	READ_SPI
	READ_SPI
	READ_SPI
	DISABLE_EEPROM_CHIP
	. . . . . CR

;

: DEBUG_SPI ( -- ) 
	SPI2STAT @ .HEX CR
	SPI2BUF @ .HEX CR
	SPI2CON @ .HEX CR
	SPI2CON2 @ .HEX CR
	SPI2BRG @ .HEX CR
;

echo
