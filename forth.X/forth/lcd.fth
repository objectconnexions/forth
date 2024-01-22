
noecho

\ LCD display, 4-bit parallel

HEX

\ set the port to the LSB of the pattern, then right shift the pattern
: lcd_port! ( pattern address bit - pattern )
	2 PICK					\ make copy of pattern at bottom of stack - pattern add bit pattern
	1 AND					\ find set bit - add bit pattern flag
	\ .S
	IF
		REG_BIT_SET
	ELSE
		REG_BIT_CLEAR
	THEN	

	1 RSHIFT				\ adjust pattern for next bit (on next call)
;


\ write nibble to LCD
: lcd_nibble! ( value - )
	LCD_EN REG_BIT_SET			\ set clock line high
	
	LCD_D0 lcd_port!
	LCD_D1 lcd_port!
	LCD_D2 lcd_port!
	LCD_D3 lcd_port!
	DROP
	
	LCD_EN REG_BIT_CLEAR		\ bring clock line low to transfer data
;

\ write a byte to the LCD, as two nibbles
: lcd_byte! ( value - )
	DUP						\ copy of value

	4 RSHIFT				\ use high nibble first
	0f AND
	lcd_nibble!

	0f AND					\ use low nibble next
	lcd_nibble!
	
	1 ms					\ brief delay
;

\ turn LCD to control mode
: lcd_control ( )  
	LCD_RS REG_BIT_CLEAR
;

\ turn LCD to data mode
: lcd_data ( ) 
	LCD_RS REG_BIT_SET
;

: lcd_init ( - )
	LCD_RS DIGITAL_OUT
	LCD_EN DIGITAL_OUT
	LCD_D0 DIGITAL_OUT
	LCD_D1 DIGITAL_OUT
	LCD_D2 DIGITAL_OUT
	LCD_D3 DIGITAL_OUT

	lcd_control
	03 lcd_nibble!		\ ensure in 8 bit mode
	03 lcd_nibble!
	03 lcd_nibble!
	02 lcd_nibble!		\ set to 4 bit interface
	4 ms
	
	0C lcd_byte!		\ turn on display without cursor
;

\ clear the LCD display
: lcd_clear ( - )
	lcd_control
	01 lcd_byte!		\ write clear command
	2 ms
;

: lcd_cursor_on_block ( )
	lcd_control
	0D lcd_byte!
;

: lcd_cursor_on_line ( )
	lcd_control
	0E lcd_byte!
;

: lcd_cursor_off ( )
	lcd_control
	0C lcd_byte!
;

\ clear the LCD display
: lcd_position ( line position - )
	lcd_control
	swap 40 * +
	80 OR lcd_byte!		\ write set data position command
;

: lcd_append_string ( string len - )
	swap
	lcd_data
	0						\ create counter
	BEGIN
		2DUP +				\ calc position
		C@ lcd_byte!	        \ display char
		1+					\ increment char count

		DUP 3 PICK >= 		\ determine if all characters written
	UNTIL
	DROP 2DROP
;

: lcd_append_space ( )
	lcd_data
	BL lcd_byte!
;

: lcd_append_decimal ( n - c-addr u )
	dup abs 0 	 			\ convert to double
	<# # # [CHAR] . HOLD #S ROT SIGN #>
	lcd_append_string
;


DECIMAL

.( added LCD words)
echo
