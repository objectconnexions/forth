noecho

\ set up the mask pattern
\   - mask (0-7FF)
\   - mask no (0-4)
: can_sid_mask ( u u - ) 21 LSHIFT SWAP C1RXM0 OFFSET_REGISTER ! ;

\ print the mask pattern
\   - mask no (0-4)
: .can_sid_mask ( u - ) C1RXM0 OFFSET_REGISTER @ 21 RSHIFT hex. ;

\ set up the filter pattern
\   - filter pattern (0-7FF)
\   - filter no (0-15)
: can_sid_filter ( u u - ) 21 LSHIFT SWAP C1RXF0 OFFSET_REGISTER ! ;

\ print the filter pattern
\   - filter no (0-15)
: .can_sid_filter ( u - ) C1RXF0 OFFSET_REGISTER @ 21 RSHIFT hex. ;



\ C1FLTCON0: address  and bits for filter control register for filer number
: can_c1fltconn ( u - a u ) 
    DUP
    4 mod 8 *                     \ bit 0, 8, 16, 24
    SWAP 4 / $10 * C1FLTCON0 +    \ in reg C1RXM0-3
    SWAP
;


\ Disable the filter for the given number (0-15)
: can_filter_dis ( u - )  can_c1fltconn 7 + SWAP REG_BIT_CLEAR ;

\ Enable the filter for the given number (0-15)
: can_filter_en ( u - )  can_c1fltconn 7 + SWAP REG_BIT_SET ;

\ Is filter n (0-15) enabled?
: .can_filter_enabled ( u - )  can_c1fltconn 7 + SWAP REG_BIT@
                IF ." enabled" THEN ;

\ set the mask used for the filter
\   - filter number (0-15)
\   - mask number (0-4)
: can_filter_mask ( u u - ) can_c1fltconn 5 + 2 ROT REG_BITS! ;

: .can_filter_mask ( u - ) can_c1fltconn 5 + 2 ROT REG_BITS@ bin. ;

\ set the filter used for the filter
\   - filter number (0-15)
\   - fifo number  (0-15)
: can_filter_fifo ( u u - ) can_c1fltconn 5 ROT REG_BITS! ;

: .can_filter_fifo ( u - ) can_c1fltconn 5 ROT REG_BITS@ bin. ;


: can_sum_fifo_size ( u u - u )
    4 4 * * +

;




0 CONSTANT TX1_FIFO
1 CONSTANT RX1_FIFO
\ CREATE TEST_FIFOS 6 4 4 * * ALLOT			\ create FIFO buffers
CREATE TEST_FIFOS
    0 
    2 can_sum_fifo_size
    3 can_sum_fifo_size
    ALLOT			\ create FIFO buffers

: can_test_setup ( )
	TEST_FIFOS can_init

	CAN_TX 2 TX1_FIFO can_add_fifo						\ Buffer #0: Tx, 2 messages
	CAN_RX 3 RX1_FIFO can_add_fifo						\ Buffer #1: Rx, 3 messages


\    $7ff 0 can_fifo_mask                    \ Mask #0: include all bits
\    $146 0 1 0 can_fifo_filter              \ Filter #0, using mask 0 and SID of 146 to Rx FIFO (#1)

    $7ff 0 can_sid_mask
    $146 0 can_sid_filter
    $64 1 can_sid_filter

    0 C1FLTCON0 !                           \ clear filters 0-3
    0 0 can_filter_mask                     \ filters 0 and 1 to use filter pattern 0
    0 1 can_filter_mask
    RX1_FIFO 0 can_filter_fifo          \ filters 0 and 1 to use RX1 fifo
    RX1_FIFO 1 can_filter_fifo
    0 can_filter_en                         \ enable both filters
    1 can_filter_en
    
    2 can_mode!
;

: can_test_write ( - )
    \ buffer two messages in Tx FIFO
	TX1_FIFO		    				    \ Tx FIFO (#0)
	$6543210 $DCBA987       				\ Data, bytes 0-3 and 4 - 7
	8									    \ Length
	$146						        	\ SID
	can_write

	TX1_FIFO $789abcd $12345  8  $64  can_write

	\ send messages
	TX1_FIFO can_send
;

: can_test_read  ( - )
    \ Read from Rx FIFO
	RX1_FIFO can_read_ready IF ." Ready" ELSE ." Empty" THEN CR
	RX1_FIFO can_read . ." -> " DROP . . CR
	RX1_FIFO can_read . ." -> " DROP . . CR
;

: can_test ( )
    HEX
    TX1_FIFO can_debug								\ display Tx buffer
	can_test_write
	TX1_FIFO can_debug

    can_test_read
	RX1_FIFO can_debug							    \ display Rx buffer
;




.( FIFO data @) TEST_FIFOS HEX. CR
.( CAN test loaded) CR
echo
