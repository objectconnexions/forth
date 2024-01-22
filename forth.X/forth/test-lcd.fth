
lcd_init

\ test lcd with direct character writing

lcd_clear
lcd_data
char h lcd_byte!
char e lcd_byte!
char l lcd_byte!
char l lcd_byte!
char o lcd_byte!
20 lcd_byte!

1 2 lcd_position

\ test lcd string

s" example" lcd_append_string


\ test lcd number

lcd_append_space 3039 lcd_append_decimal

