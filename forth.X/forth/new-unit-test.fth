noecho

4 16 * CONSTANT STACK_SIZE

VARIABLE TEST
VARIABLE ACTUAL_SIZE
CREATE ACTUAL STACK_SIZE ALLOT
CREATE EXPECTED STACK_SIZE ALLOT

\ 0 TEST !

: T+ ( addr - )
    CR TYPE CR
    0 TEST !
;

: T{ ( )
	clear
	ACTUAL STACK_SIZE ERASE
	EXPECTED STACK_SIZE ERASE
	TEST !+
;

\ Copy n elements of the stack into the memory at the address
: COPY_STACK ( ... n addr - )
    SWAP 0 DO
		DUP ROT SWAP !
		CELL+
	LOOP
	DROP	
;

: -> ( ... - )
	DEPTH ACTUAL_SIZE !
	
	DEPTH ACTUAL COPY_STACK
;

\ Check if there are difference between ACTUAL and EXPECTED elements 
: IS_DIFFERENT ( ) 
	FALSE

    ACTUAL EXPECTED
	ACTUAL_SIZE @ 0 DO
\	    ." read from " .S CR
		OVER @ OVER @ <> IF 
\	        ." diff " .S
            ROT DROP
	        TRUE 
	        LROT
\	        .S CR
	        LEAVE
		THEN
		SWAP CELL+ SWAP CELL+       \ move to next elements
	LOOP
    DROP DROP

 \   .S CR
;

\ Display differences between ACTUAL and EXPECTED elements 
: DISPLAY_DIFFERENCES ( ) 
    ACTUAL EXPECTED
    ACTUAL_SIZE @ 0 DO
 \       .S CR
	    2DUP
	    @ SWAP @ <> IF 
		    2DUP @ SWAP @ SWAP
		    ."  -  param "  ACTUAL_SIZE @ I -  . 
		    ." expected " hex. 
		    ." but was " hex. CR
	    THEN
	    SWAP CELL+ SWAP CELL+       \ move to next elements
    LOOP
;

: } ( ... - )
	DEPTH ACTUAL_SIZE @ <> IF 
	    ." Test " TEST ? ." , different number of values " .S CR EXIT 
    THEN
	DEPTH EXPECTED COPY_STACK
    IS_DIFFERENT IF 
        ." Test " TEST ? ." failed" CR
        DISPLAY_DIFFERENCES	
    THEN
;

echo




noecho

s" valid" T+

T{ 1 2 3 SWAP -> 1 3 2 }    \ valid
T{ 1 2 3 SWAP -> 5 6 }      \ invalid different sizes
T{ 1 2 SWAP -> 5 6 7 }

s" invalid" T+

T{ 1 2 3 SWAP -> 1 2 3 }    \ invalid, one wrong value
T{ 1 2 3 SWAP -> 5 6 7 }    \ invalid, all wrong values
T{ 1 2 3 SWAP -> 1 3 2 }    \ valid

echo
