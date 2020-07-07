
: Y ( -- n ) 1 + 2 + 3 + ;
: X ( n -- n doubls the number ) DUP * ;
: Z Y X ; \ n n -- n multiple numbers
: TEST0 Z DUP Z . CR . CR ;

: IFF IF 222 . CR THEN ;


: 0= ( n -- flag )
	0 =
;

: test1 ( -- )
  IF
    22 . CR
 	IF
		44 . CR
    THEN
  THEN
;

: test2
  DUP *
  . CR
;

: test3 SWAP 3 DUP + * ;


: loop
	BEGIN
		CR  DUP  .
		1 - DUP
		DUP .
		DUP test2
		0= 
		.S
	UNTIL
	DROP
	CR
;

: test 
      5 loop ;
      
      
      
: count
	BEGIN
		1 - DUP
		0=
		.S
	UNTIL
	DROP
	CR
;
      
