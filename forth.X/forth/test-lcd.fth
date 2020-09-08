
	0x03 write_lcd_nibble		\ ensure in 8 bit mode
	0x03 write_lcd_nibble
	0x03 write_lcd_nibble
	0x02 write_lcd_nibble		\ set to 4 bit interface
	4 ms
	
	0x0f write_lcd_byte
