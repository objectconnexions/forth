
 2VARIABLE LED
 VARIABLE LED_PATTERN

\ probably only need one or two leds flashing

: led_show_pattern

    LED 2@ DIGITAL_OUT
    LED 2@ REG_BIT_SET
    
	5 0 DO
\	BEGIN
		LED_PATTERN @
		30 0 DO 
			DUP $01 AND		    \ extract bit 
			LED 2@ REG_BIT!		\ turn LED on or off
			1 RSHIFT		    \ set up for next bit
			100 ms
		LOOP 
		DROP
\	AGAIN
	LOOP
;

MNT_LED LED 2!
$ff00ff00 LED_PATTERN !

HEX
LED 2@ hex. hex. CR
LED_PATTERN @ hex. CR

\ TASK+ FLASH
\ : FLASH ACTIVATE led_show_pattern ;
