
\ setup CN for PORTB
PORTB CNCON OR   15 REG_BIT_SET 
\ set up button as input with pull up
PB1 2DUP DIGITAL_IN PULL_UP
\ PB1 CNSTAT REGISTER REG_BIT@ .
\ set up pin for CN int
PB1 SWAP CNEN REGISTER SWAP REG_BIT_SET
   
: read_button ( )
    time 1000 / .  ." - "
    
    \ TODO REGISTER need to do swapping itselt - probably need two methods
    PB1 SWAP LAT REGISTER SWAP REG_BIT@ .
    PB1 SWAP CNSTAT REGISTER SWAP REG_BIT@ .

    PB1 DIGITAL@ .
    CR
;

: clear_button
   PB1 SWAP LAT REGISTER SWAP REG_BIT_SET
   PB1 SWAP CNSTAT REGISTER SWAP REG_BIT_CLEAR   
 
;
