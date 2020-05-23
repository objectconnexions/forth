\ _reset
noecho

0x0bf80f230 CONSTANT SYSKEY

0x0bf880000 CONSTANT IO
0x0bf886000 CONSTANT PORTA
0x0bf886100 CONSTANT PORTB
0x0bf886200 CONSTANT PORTC

0x00 CONSTANT ANSEL
0x10 CONSTANT TRIS
0x20 CONSTANT PORT

4 CONSTANT LED_COMM


: 0= ( n -- flag )
	0 =
;

: 0> ( n -- flag )
	0 >
;

: 0< ( n -- flag )
	0 > NOT
;

: 0<> ( n -- flag )
	0 = NOT
;


: 2DUP ( n1 n2 -- n1 n2 n1 n2 )
	over
	over
;


: SET_BITS ( value value position -- value )
	lshift or
;

\ clear bit at position in register
: BIT_CLR ( addr pos -- ) 
	1 swap lshift		\ determine mask
	swap 
	0x04 or				\ clear register is 4 registers on
	!					\ write to register
;

\ set bit at position in register
: BIT_SET ( addr pos -- ) 
	1 swap lshift		\ determine mask
	swap 
	0x08 or				\ set register is 8 registers on
	!					\ write to register
;

\ set bit at position in register
: BIT_INV ( addr pos -- ) 
	1 swap lshift		\ determine mask
	swap 
	0x0c or				\ set register is 12 registers on
	!					\ write to register
;

: BIT_READ ( addr pos -- val )
	swap @
	swap rshift
	0x01 and
;


echo
