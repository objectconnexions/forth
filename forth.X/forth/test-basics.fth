noecho


start_tests

s" add" 
    4 5 6 + +
    15 assert_equals


s" 4+4 mask"
    4 3 MASK
    $0f0 assert_equals


s" 4 mask bit"
    0 MASK_BIT
    $01 assert_equals


s" 4 mask bit"
    2 MASK_BIT
    $08 assert_equals

s" 4 mask bit"
    4 MASK_BIT
    $010 assert_equals

s" 4 mask bit"
    4 MASK_BIT
    $020 assert_equals

s" 4 mask bit"
    31 MASK_BIT
    $80000000 assert_equals


0 PORTB OFFSET_REGISTER
    PORTB assert_equals

3 PORTB OFFSET_REGISTER
    PORTB $30 + assert_equals
    
echo

