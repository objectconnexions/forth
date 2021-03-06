


: PORT_ON ( port bit -- )
	swap
	PORT +				\ port register
	swap
	BIT_SET				\ low for disabled
;
: PORT_OFF ( port bit -- )
	swap
	PORT +				\ port register
	swap
	BIT_CLR				\ low for disabled
;

: mask ( pos size -- mask )
\ creates a bit mask for the specified number of bits at the 
\ postion (zero indexed)

	1 swap lshift
	1 -
	swap lshift
	0x0ffff xor
;

: setBits ( value pos size -- value ) 
	tuck

	mask
	swap



	4 3 mask
	889 and
	6 4 lshift
	or
;

: RegAddr  ( base port register -- addr )
	+ +
;

: RegGet ( mask addr -- n )
	@
	AND
;

: RegSetOn  ( value mask addr -- )
	tuck @		 		\ read current state of port ( addr bit port-state )
	swap
    0x01 swap lshift	
;
\ IO PORTA PORT register

\ position value address regSet

\ position value mask



: DOUBLE ( n n -- 2n ) DUP + ;

: SQUARE ( n n -- n^2 ) DUP * ;

: INC ( n -- n+1 ) 1 + ;

: ON ( -- )
    0x0bf886220 dup @
    0x01 4 lshift
    or
    swap !
;

: OFF ( -- ) 
	0x0bf886220 dup @
	0x01 4 lshift
	0x03ff xor 
	and 
	swap ! 
;

\ : +! ( n address --  - add n to address )
\ 	DUP @ + ! \ need to swap next on stack!
\ ;




: portOn ( bit addr -- )
	tuck @		 		\ read current state of port ( addr bit port-state )
	swap
    0x01 swap lshift	\ use bit to determine mask ( addr mask )
	or					\ use mask to update port state ( addr port-state )
	swap !					\ write new state of port ( )
;

: portOff ( bit addr -- )
	tuck @		 		\ read current state of port ( addr bit port-state )
	swap
    0x01 swap lshift	\ use bit to determine mask ( addr mask )
	0x03ff xor 
	and 				\ use mask to update port state ( addr port-state )
	swap !					\ write new state of port ( )
;

: onComm ( -- )
	LED_COMM PORTE portOn
;

: offComm ( -- )
	LED_COMM PORTE portOff
;

: flashComm
	begin
		onComm 200 ms offComm 200 ms
	again
;

: toggleError ( -- )
	1 1 ( bit no ) lshift
	IO PORTB PORT or or
	0x0c or \ invert bit
	!
;

: flashError
	begin
		toggleError 100 ms toggleError 100 ms
	again
;



