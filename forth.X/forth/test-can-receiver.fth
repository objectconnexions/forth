noecho


0 CONSTANT TX_FIFO1
1 CONSTANT RX_FIFO1

CREATE MSG_FIFOS
    0 
    6 can_sum_fifo_size 
    6 can_sum_fifo_size
    ALLOT			\ create FIFO buffers

: io_setup ( )
    COMMS_LED 2DUP digital_out digital_on

    MNT_LED 2DUP digital_out digital_on
    ERROR_LED 2DUP digital_out digital_off
;

: flash_comms ( )
    COMMS_LED DIGITAL_ON
    30 ms
    COMMS_LED DIGITAL_OFF
;

: can_setup ( )
	MSG_FIFOS can_init

	TX_FIFO1 CAN_TX 6 can_add_fifo			\ Buffer #0: Tx, 3 messages
	RX_FIFO1 CAN_RX 6 can_add_fifo			\ Buffer #1: Rx, 3 messages
	
    0 $7ff can_sid_mask!

    0 $2 can_sid_filter!
\    1 $4 can_sid_filter!

    can_reset_filters                       
    0 0 can_filter_mask!                     \ filter 0 to use filter pattern 0
    0 RX_FIFO1 can_filter_fifo!              \ filters 0 to use RX fifo 1
    0 can_filter_en                          \ enable filters

\    2 can_mode!
    0 can_mode!
    flash_comms
;


: can_write_msg ( u - )
    \ buffer single messages with two bytes in Tx FIFO
	TX_FIFO1		    				    \ Tx FIFO (#0)
	SWAP
	$2   						        	\ SID
	SWAP
	2									    \ Length
	SWAP
	$ffff AND                      			\ Data, byte 0 (bytes 0-3)
	0                                       \ Data, ignored (bytes 4-7)
	SWAP
\	." SEND " .S CR
	can_fifo!

	\ send messages
	TX_FIFO1 can_send
	flash_comms
;

: can_read_msg  ( - )
    \ Read from Rx FIFOs
	." FIFO1 " RX_FIFO1 can_fifo_ready IF ." Ready" ELSE ." Empty" THEN CR CR
	RX_FIFO1 can_fifo@ . ." -> " DROP . . CR
	RX_FIFO1 can_fifo@ . ." -> " DROP . . CR CR
;

: can_test_send ( )
    HEX
    TX_FIFO1 can_debug								\ display Tx buffer
	$FEDC can_write_msg
	$1234 can_write_msg
	TX_FIFO1 can_debug
;

: can_test_recv ( )
    can_read_msg
    can_read_msg
	RX_FIFO1 can_debug							    \ display Rx buffer
;





task+ can_receive

: run_receive ( - )
    COMMS_LED digital_out
    COMMS_LED REG_BIT_SET
    
    can_receive activate 
        begin
            RX_FIFO1 can_fifo_ready 
            IF 
                flash_comms                
                RX_FIFO1 can_fifo@
                
                ." Received " . ." -> " DROP
                2DUP . . CR
                
                DUP 1 AND MNT_LED DIGITAL!
                DUP 8 AND ERROR_LED DIGITAL!
                DROP
\            ELSE ." . " 
            THEN
            200 ms
        again
 ;




: start ( )
    io_setup
    can_setup
    run_receive
;

: errors  (  ) 
    C1TREC @
    DUP DUP
    ." flags " 16 RSHIFT $ff AND hex. CR
    ." tx errors "   8 RSHIFT $fF and . CR
    ." rx errors "   $ff AND .
;



.( FIFO data @) MSG_FIFOS HEX. CR
.( CAN test loaded) CR
echo
