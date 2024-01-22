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

: -> ( ? - )
	DEPTH ACTUAL_SIZE !
	DEPTH 0 > IF ACTUAL ! THEN
	DEPTH 0 > IF ACTUAL ! THEN
	DEPTH 0 > IF ACTUAL ! THEN
	DEPTH 0 > IF ACTUAL ! THEN
	clear
	
;

: } ( ? - )
	DEPTH ACTUAL_SIZE <> IF ." different number of values" .S EXIT THEN

	DEPTH 0 > IF EXPECTED ! THEN
	DEPTH 0 > IF EXPECTED ! THEN
	DEPTH 0 > IF EXPECTED ! THEN
	DEPTH 0 > IF EXPECTED ! THEN
	clear

	0
	
	DUP
	DUP ACTUAL @ EXPECTED @
	<> IF DUP DUP ." param " . ." expected " EXPECTED ? ." but was " ACTUAL ? ELSE DROP THEN
\	<> IF DUP DUP ." param " . ." expected " EXPECTED ? ." but was " ACTUAL ? ELSE DROP THEN
\	<> IF DUP DUP ." param " . ." expected " EXPECTED ? ." but was " ACTUAL ? ELSE DROP THEN
\	<> IF DUP DUP ." param " . ." expected " EXPECTED ? ." but was " ACTUAL ? ELSE DROP THEN

;



T{ 1 2 3 SWAP -> 1 3 2 }
T{ 1 2 3 SWAP -> 1 2 3 }


