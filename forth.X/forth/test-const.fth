
: TEST_CONST ( port register -- register )
	PORTA -				\ offset to register
	+ 					\ adjust port address
;

