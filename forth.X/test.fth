: Y 1 + 2 + 3 + ;
: X DUP * ;
: Z Y X ;
: TEST1 Z DUP Z . CR . CR ;

: IFF IF 222 . CR THEN ;

: test1
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
		0  = 
		STACK
	UNTIL
	DROP
	CR
;

: test 
      5 loop ;
      
      
      
: count
	BEGIN
		1 - DUP
		0  =
		STACK
	UNTIL
	DROP
	CR
;
      