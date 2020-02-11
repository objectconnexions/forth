0x0bf80fa90 CONSTANT SDI2R
0x3			CONSTANT RPB13
0x0bf80fb8c CONSTANT RPC8R
0x4			CONSTANT SDO2

0x0bf805a00 CONSTANT SPI2CON
0x0bf805a10 CONSTANT SPI2STAT
0x0bf805a20 CONSTANT SPI2BUF
0x0bf805a30 CONSTANT SPI2BRG
0x0bf805a40 CONSTANT SPI2CON2


: READ_SPI ( -- value )
	1 SPI2BUF !
	SPI2BUF @
;

: WRITE_SPI ( value -- )
	SPI2BUF !
;

: ENABLE_ID_CHIP ( -- )
	IO PORTB + PORT +
	5 BIT_CLR				\ low for enabled
;

: DISABLE_ID_CHIP ( -- )
	IO PORTB + PORT +
	5 BIT_SET				\ high for disabled
;

: INIT_SPI ( -- )
	IO PORTC +
	7 2dup
	digital_out				\ A7 (!POWER_SAVE) as digital output
	port_on					\ turn peripheral power on


	RPB13 SDI2R !			\ use RPB13 for SDI
	SDO2 RPC8R !			\ use RPC8 for SDO

	IO PORTB +
	5 2dup
	digital_out				\ B5 (!CE) as digital output
	port_on					\ disable CE for ID chip
	
	IO PORTA +
	9 2dup
	digital_out				\ A9 (!CE) as digital output
	port_on					\ disable flash

	IO PORTA +
	10 2dup
	digital_out				\ A10 (!CE) as digital output
	port_on					\ disable eeprom
\	DISABLE_ID_CHIP
	
	\ set up SPI module
	SPI2CON 15	BIT_CLR		\ turn off SPI
	READ_SPI				\ clear buffer
	10 SPI2BRG !			\ us FBP/4 clock frequency
	SPI2CON 16	BIT_CLR		\ Clear the ENHBUF bit
	SPI2STAT 6	BIT_CLR		\ clear SPIROV flag
	SPI2CON 5	BIT_SET		\ set to master mode

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


: TEST_SPI ( -- )
	ENABLE_ID_CHIP
	0x03 WRITE_SPI			\ read
	0x0fa WRITE_SPI			\ serial number address
	READ_SPI
	DISABLE_ID_CHIP
 	. CR
	SPI2STAT @ .HEX CR
;


: DEBUG_SPI ( -- ) 
	SPI2CON @ .HEX CR
	SPI2STAT @ .HEX CR
	SPI2BUF @ .HEX CR
	SPI2BRG @ .HEX CR
	SPI2CON2 @ .HEX CR
;
