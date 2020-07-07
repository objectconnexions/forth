noecho


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




: led_flash ( on off addr port - )
	\ on and off duration are one double (timing);
	\ addr and port are another (location)

    2DUP DIGITAL_OUT
	begin
        2DUP port_on

		2SWAP	\ bring timings to top
		2DUP	
        ms		\ delay for first period (on) - in top of double
		drop	\ drop the other half
		
		2SWAP 	\ return location to top
        2DUP port_off

		2SWAP	\ bring timings to top
		2DUP	
		drop	\ drop the first half ro expose the off time
        ms		\ delay for second period

		2SWAP	\ bring location back to top
	again
;

: red_flash ( - )
	450 50 PORTB 1 led_flash
;

: green_flash ( - )
	150 50 PORTC 4 led_flash
;

\ run flasher on power task
task test_flashing
' red_flash test_flashing initiate

echo
