\ _reset
noecho

: DIGITAL_IN ( port bit -- )
	swap
	2dup				\ copy params
	ANSEL +				\ determine ANSEL reg address
	swap bit_clr		\ clear analogue flag
	
	2dup
	TRIS +				\ TRIS register
	swap bit_set		\ set to input
;

: DIGITAL_OUT ( port bit -- )
	swap
	2dup				\ copy params
	ANSEL +				\ determine ANSEL reg address
	swap bit_clr		\ clear analogue flag
	
	2dup
	TRIS +				\ TRIS register
	swap bit_clr		\ set to output

	PORT +				\ PORT register
	swap bit_clr		\ set to off
;

: PORT_ON ( port bit -- )
	swap
	PORT +				\ PORT register
	swap bit_set		\ set to on
;

: PORT_OFF ( port bit -- )
	swap
	PORT +				\ PORT register
	swap bit_clr		\ set to on
;

: PORT_TOGGLE ( port bit -- )
	swap
	PORT +				\ PORT register
	swap bit_inv		\ set to on
;
 
: PORT_STATE ( port bit -- flag )
	swap
	PORT +
	swap bit_read
;

echo

