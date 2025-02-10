: LOOP1

	begin

		1500 ms
		
		." OK" CR

	again
;


\ LOOP


\ loops printing "OK"--Ctrl-C will lose the input and so no more entries will be seen!



: loop2 ( )

	10 0 DO I . LOOP

;


: recurse ( n - )

	dup . CR
	1+
	recurse

;
