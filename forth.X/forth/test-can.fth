noecho




0 CONSTANT TX1_FIFO
1 CONSTANT RX1_FIFO
2 CONSTANT RX2_FIFO

\ CREATE TEST_FIFOS 6 4 4 * * ALLOT			\ create FIFO buffers
CREATE TEST_FIFOS
    0 
    4 can_sum_fifo_size
    6 can_sum_fifo_size 
    6 can_sum_fifo_size
    ALLOT			\ create FIFO buffers

: can_test_setup ( )
	TEST_FIFOS can_init

	TX1_FIFO CAN_TX 4 can_add_fifo			\ Buffer #0: Tx, 4 messages
	RX1_FIFO CAN_RX 6 can_add_fifo			\ Buffer #1: Rx, 3 messages
	RX1_FIFO CAN_RX 6 can_add_fifo			\ Buffer #3: Rx, 3 messages
	
    0 $7ff can_sid_mask!

    0 $146 can_sid_filter!
    1 $64 can_sid_filter!
    2 $12 can_sid_filter!

    can_reset_filters                       
    0 0 can_filter_mask!                     \ filters 0 and 1 to use filter pattern 0
    1 0 can_filter_mask!
    2 1 can_filter_mask!
    0 RX1_FIFO can_filter_fifo!              \ filters 0 and 1 to use RX1 fifo
    1 RX1_FIFO can_filter_fifo!
    2 RX2_FIFO can_filter_fifo!
    0 can_filter_en                         \ enable filters
    1 can_filter_en
    2 can_filter_en
    
    2 can_mode!
;

: can_test_write ( - )
    \ buffer two messages in Tx FIFO
	TX1_FIFO		    				    \ Tx FIFO (#0)
	$146						        	\ SID
	8									    \ Length
	$6543210 $DCBA987       				\ Data, bytes 0-3 and 4 - 7
	can_fifo!

	TX1_FIFO $64 8  $789abcd $12345       can_fifo!

	TX1_FIFO $12 8  $deadbeef $cafebabe   can_fifo!

	\ send messages
	TX1_FIFO can_send
	
	." 3 messages sent"
;

: can_test_read  ( - )
    \ Read from Rx FIFOs
	." FIFO1 " RX1_FIFO can_fifo_ready IF ." Ready" ELSE ." Empty" THEN CR CR
	RX1_FIFO can_fifo@ . ." -> " DROP . . CR
	RX1_FIFO can_fifo@ . ." -> " DROP . . CR CR

    ." FIFO2 " RX2_FIFO can_fifo_ready IF ." Ready" ELSE ." Empty" THEN CR CR
	RX2_FIFO can_fifo@ . ." -> " DROP . . CR
	RX2_FIFO can_fifo@ . ." -> " DROP . . CR
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
