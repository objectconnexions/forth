noecho

variable TEST_NO

: start_tests
    0 TEST_NO ! 
;

: assert_equals ( t n n - )
    TEST_NO !+

    .S CR
    2DUP
    = if
        DROP
        DROP
        \ DROP
    else
        ." Test " TEST_NO ? ." failed - "
        DEPTH 4 = if 
            2SWAP
\            .S CR TYPE CR
            TYPE
    \       2DROP
        THEN
        ." - expected " 
            DUP . ." ($" hex. ." ) but got "
            DUP . ." ($" hex. ." )"  
            CR

       \ DROP
    then

    clear
;

    
echo

