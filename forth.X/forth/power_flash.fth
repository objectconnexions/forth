noecho


\ PORTB 1 DIGITAL_OUT
\ PORTB 1 port_on
\ 250 ms
\ PORTB 1 port_off

variable flash_rate

500 flash_rate !

: pwr_flash ( - )
	450 50 PORTE 1 led_flash
;

\ run flasher on power task
' pwr_flash power initiate



: red_flash ( - )
	450 50 PORTB 1 led_flash
;

: green_flash ( - )
	150 50 PORTC 4 led_flash
;

\ run flasher on power task
\ task test_flashing
\ ' red_flash test_flashing initiate

echo
