
	0x03 lcd_nibble!		\ ensure in 8 bit mode
	0x03 lcd_nibble!
	0x03 lcd_nibble!
	0x02 lcd_nibble!		\ set to 4 bit interface
	4 ms
	
	0x0f lcd_byte!
