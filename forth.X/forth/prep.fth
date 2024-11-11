\ prep




\ 1's complement
: invert ( n - n ) NEGATE 1 - ;



\ source
\ value
\ pos
\ length
: set_bits ( n n n n - n )
    OVER MASK_PATTERN INVERT    \ mask
    LROT LSHIFT                 \ new value
    LROT AND                      \ mask the source VALUE
    OR                           \ combine
;

decimal

0x0 0x012 16 8 set_bits hex.
0xffffff 0x12 16 8 set_bits hex.
0x123456 0xab 12 8 set_bits hex.
0xffffff 0x7 8 5 set_bits hex.
