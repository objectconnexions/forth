\ This code should fail - to check out how the sysem handles things

\ this code does not compile as the words are not avail

: led_flash ( on off addr port - )
	\ on and off duration are one double (timing);
	\ addr and port are another (location)

    2DUP NO_DIGITAL_OUT
	begin
        2DUP NO_REG_BIT_SET

		2SWAP	\ bring timings to top
		2DUP	
        ms		\ delay for first period (on) - in top of double
		drop	\ drop the other half
		
		2SWAP 	\ return location to top
        2DUP NO_REG_BIT_CLEAR

		2SWAP	\ bring timings to top
		2DUP	
		drop	\ drop the first half ro expose the off time
        ms		\ delay for second period

		2SWAP	\ bring location back to top
		
		NO_FINAL_WORD
	again
;

