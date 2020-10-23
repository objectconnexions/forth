noecho

\ LCD display, 4-bit parallel

HEX

\ set the port to the LSB of the pattern, then right shift the pattern
: write_lcd_port ( pattern address bit - pattern )
	2 PICK				\ make copy of pattern at bottom of stack - pattern add bit pattern
	1 AND				\ find set bit - add bit pattern flag
	\ .S
	IF
		PORT_ON
	ELSE
		PORT_OFF
	THEN	

	1 RSHIFT			\ adjust pattern for next bit (on next call)
;

\ write nibble to LCD
: write_lcd_nibble ( value - )
	PORTB 3 PORT_ON		\ set clock line high
	
	PORTB 6 write_lcd_port
	PORTB 7 write_lcd_port
	PORTB 8 write_lcd_port
	PORTB 9 write_lcd_port
	DROP
	
	PORTB 3 PORT_OFF		\ bring clock line low to transfer data
;

\ write a byte to the LCD, as two nibbles
: write_lcd_byte ( value - )
	DUP						\ copy of value

	4 RSHIFT				\ use high nibble first
	0f AND
	write_lcd_nibble

	0f AND				\ use low nibble next
	write_lcd_nibble
;

\ turn LCD to control mode
: control_lcd ( )  
	PORTB 0c port_off
;

\ turn LCD to data mode
: data_lcd ( ) 
	PORTB 0c port_on
;

\ clear the LCD display
: clear_lcd ( - )
	control_lcd
	01 write_lcd_byte		\ write clear command
	2 ms
;

\ clear the LCD display
: cursor_at_lcd ( position - )
	control_lcd
	80 + write_lcd_byte		\ write set data position command
;

: write_lcd_string ( string len - )
	swap
	data_lcd
	0						\ create counter
	BEGIN
		2DUP +				\ calc position
		@C write_lcd_byte	\ display char
		1+					\ increment char count

		DUP 3 PICK >= 		\ determine if all characters written
	UNTIL
	DROP 2DROP
;

: init_lcd ( - )
	PORTB C DIGITAL_OUT		\ LCD RS
	PORTB 3 DIGITAL_OUT			\ LCD E
	PORTB 6 DIGITAL_OUT			\ LCD DB0
	PORTB 7 DIGITAL_OUT			\ LCD DB1
	PORTB 8 DIGITAL_OUT			\ LCD DB2
	PORTB 9 DIGITAL_OUT			\ LCD DB3

	control_lcd
	03 write_lcd_nibble		\ ensure in 8 bit mode
	03 write_lcd_nibble
	03 write_lcd_nibble
	02 write_lcd_nibble		\ set to 4 bit interface
	4 ms
	
	0f write_lcd_byte		\ turn on display with cursor
	
;

: test_lcd	
	clear_lcd
	data_lcd
	[char] h write_lcd_byte
	[char] e write_lcd_byte
	[char] l write_lcd_byte
	[char] l write_lcd_byte
	[char] o write_lcd_byte
	20 write_lcd_byte
;

: test_lcd_string
	s" example" 
	write_lcd_string
;

: nnn ( n - c-addr u )
	dup abs 0 	 			\ convert to double
	<# # # [CHAR] . HOLD #S ROT SIGN #>
;



DECIMAL

init_lcd

test_lcd
BL write_lcd_byte
3039 nnn write_lcd_string

DECIMAL

.( added LCD words)
echo
