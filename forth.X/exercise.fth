2 4 +
4 8 * \ add two numbers, multiply two
. CR . CR CR

\ turn on LED
0x0bf886220 dup @
stack CR
0x01 4 lshift
stack CR
or
stack CR
\ !
