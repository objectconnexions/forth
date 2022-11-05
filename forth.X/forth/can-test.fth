noecho

lock

CREATE TEST_FIFO 6 4 4 * * ALLOT			\ create FIFO buffers

: can_test_setup ( )
	TEST_FIFO can_init

	1 2 0 can_add_fifo						\ Buffer #0: Tx, 2 messages
	0 3 1 can_add_fifo						\ Buffer #1: Rx, 3 messages

    $7ff 0 can_fifo_mask                    \ Mask #0: include all bits
    $146 0 1 0 can_fifo_filter              \ Filter #0, using mask 0 and SID of 146 to Rx FIFO (#1)

    2 can_set_mode
;

: can_test_write ( - )
    \ buffer two messages in Tx FIFO
	0									    \ Tx FIFO (#0)
	$6543210 $DCBA987       				\ Data, bytes 0-3 and 4 - 7
	8									    \ Length
	$146						        	\ SID
	can_write

	0 $789abcd $12345  8  $64  can_write

    \ send messages
	0 can_send
;

: can_test_read  ( - )
    \ Read from Rx FIFO
	1 can_read_ready IF ." Ready" ELSE ." Empty" THEN CR
	1 can_read . ." -> " DROP . . CR
	1 can_read . ." -> " DROP . . CR
;

: can_test ( )
    HEX
    0 can_debug								\ display Tx buffer
	can_test_write
	0 can_debug

    can_test_read
	1 can_debug							    \ display Rx buffer
;


: .can_mask ( n - ) C1RXM0 OFFSET_REGISTER @ 21 RSHIFT bin. ;
: .can_filter ( n - ) C1RXF0 OFFSET_REGISTER @ 21 RSHIFT bin. ;
: .can_control ( n - )  DUP 4 MOD C1FLTCON0 OFFSET_REGISTER @
                    4 / 8 * RSHIFT bin. ;

\ TODO explore using short words to set up filter
\ index# value 
: can_mask ( u u - ) 21 LSHIFT SWAP C1RXM0 OFFSET_REGISTER .S ! ;
: can_filter ( u u - ) 21 LSHIFT SWAP C1RXF0 OFFSET_REGISTER .S ! ;
: can_control ( u u - )  
    SWAP DUP SWAP
    4 / 8 * LSHIFT .S
    SWAP 4 MOD C1FLTCON0 OFFSET_REGISTER .S !
;

.( FIFO data @) TEST_FIFO HEX. CR
.( CAN test loaded) CR
echo
