
\PORTB 1 DIGITAL_OUT
\PORTB 1 port_on
\250 ms
\PORTB 1 port_off

variable flash_rate

500 flash_rate !

: pwr_flash ( - )
	PORTA 8 DIGITAL_OUT
	begin
		PORTA 8 port_on
		flash_rate @ dup
		ms
		PORTA 8 port_off
		ms
	again
;

\ run flasher on power task
' pwr_flash power initiate
