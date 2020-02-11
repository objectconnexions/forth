
: INIT_SPI ( -- )
	\ set up chip enable (CE) ports
\	IO PORTA ANSEL RegAddr
\	5 BIT_CLR				\ A5 is digital
\	IO PORTA TRIS RegAddr
\	5 BIT_CLR				\ A5 is output

	IO PORTB +
	5 2dup
	digital_out				\ B5 as digital output
	port_on					\ disable CE for ID chip
	
	IO PORTA +
	9 2dup
	digital_out				\ A9 as digital output
	port_on					\ disable flash

	IO PORTA +
	10 2dup
	XXXdigital_out				\ A10 as digital output
	port_on					\ disable eeprom
\	DISABLE_ID_CHIP
	
	\ set up SPI module
	SPI1CON 15	BIT_CLR		\ turn off SPI
	READ_SPI				\ clear buffer
	0x01 SPI1BRG !			\ us FBP/4 clock frequency
	SPI1CON 16	BIT_CLR		\ Clear the ENHBUF bit
	SPI1STAT 6	BIT_CLR		\ clear SPIROV flag
	SPI1CON 5	BIT_SET		\ set to master mode

	SPI1CON 15	BIT_SET		\ enable SPI
;


: DOUBLE ( n n -- 2n ) DUP + ;

: SQUARE ( n n -- n^2 ) DUP * ;

: INC ( n -- n+1 ) 1 + ;

