 VARIABLE TEST
 VARIABLE ACTUAL_SIZE
 CREATE ACTUAL 100 ALLOT
  CREATE EXPECTED 100 ALLOT

0 TEST !

: T{ ( )
	clear
	ACTUAL 100 ERASE
	EXPECTED 100 ERASE
	TEST !+
;

\ Copy n elements of the stack into the memory at the address
: COPY_STACK ( ... n addr - )
    SWAP 0 DO
\        .S
		DUP ROT SWAP !
		CELL+
	LOOP
	DROP	
;

: -> ( ... - )
	DEPTH ACTUAL_SIZE !
	
	DEPTH ACTUAL COPY_STACK
;

: } ( ... - )
	DEPTH ACTUAL_SIZE @ <> IF ." different number of values" .S EXIT THEN

	DEPTH EXPECTED COPY_STACK
	
\	.S CR
	ACTUAL EXPECTED
	\ .S CR
	ACTUAL_SIZE @ 0 DO
	\ .S CR
		2DUP
		@ SWAP @ <> IF 
			2DUP @ SWAP @ SWAP
			." param "  ACTUAL_SIZE @ I -  . 
			." expected " hex. 
			." but was " hex. CR
		THEN
		SWAP CELL+ SWAP CELL+       \ move to next elements
	LOOP
;



T{ 1 2 3 SWAP -> 1 3 2 }    \ valid
T{ 1 2 3 SWAP -> 5 6 }      \ invalid different sizes
T{ 1 2 SWAP -> 5 6 7 }
T{ 1 2 3 SWAP -> 1 2 3 }    \ invalid, one wrong value
T{ 1 2 3 SWAP -> 5 6 7 }    \ invalid, all wrong values

