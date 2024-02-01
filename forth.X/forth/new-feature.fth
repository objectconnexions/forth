
: test_do ( )
   \ ." DO Loop" CR CR
   
    CR CR
    
\    TRACE CR
    10 0 DO 
\    TRACE 
        I . CR
    LOOP
\    10 12 0 DO .S TRACE 1+ DUP . CR LOOP
    
    CR 
    
    6 0 DO 
        5 0 DO 0 . LOOP
        CR
    LOOP
    
    CR
    
 \   clear
 \   TRACE
 \   CR
    30 0 DO I . CR 2 LOOP+

;



: test_while ( )
   \ ." WHILE Loop" CR CR
   
    
    10 BEGIN DUP 0 > WHILE 1- DUP . REPEAT 
;


: test_leave ( )
    CR
    1   
    30 0 DO
        1+
        I .
        .S trace CR
        LEAVE
        ." -"
    LOOP
    .S TRACE CR
;

: test_IF_leave ( )
    1   
    30 0 DO
        1+
        I .
        
        I 12 > IF
            I LEAVE
        THEN
        
        I DUP  . .
        
        LEAVE
        
    2 LOOP+ 
;

: test_if ( )
    TRUE
    IF
        12 .
    ELSE
        24 .
    THEN
;


