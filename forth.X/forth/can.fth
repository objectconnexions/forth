noecho

\ lock

HEX

\ PIC32MX Registers
0bf88b000 CONSTANT C1CON
0bf88b010 CONSTANT C1CFG
0bf88b020 CONSTANT C1INT
0bf88b030 CONSTANT C1VEC
0bf88b040 CONSTANT C1TREC
0bf88b050 CONSTANT C1FSTAT
0bf88b060 CONSTANT C1RXOVF
0bf88b070 CONSTANT C1TMR
0bf88b080 CONSTANT C1RXM0
0bf88b090 CONSTANT C1RXM1
0bf88b0a0 CONSTANT C1RXM2
0bf88b0b0 CONSTANT C1RXM3
0bf88b0c0 CONSTANT C1FLTCON0
0bf88b0d0 CONSTANT C1FLTCON1
0bf88b0e0 CONSTANT C1FLTCON2
0bf88b0f0 CONSTANT C1FLTCON3
0bf88b140 CONSTANT C1RXF0
0bf88b340 CONSTANT C1FIFOBA
0bf88b350 CONSTANT C1FIFOCON0
0bf88b360 CONSTANT C1FIFOINT0
0bf88b370 CONSTANT C1FIFOUA0
0bf88b380 CONSTANT C1FIFOC0

PORTE 2 2CONSTANT CAN1EN

1 CONSTANT CAN_TX 
0 CONSTANt CAN_RX


DECIMAL


\ Calculate the address of the CAN register for the specified FIFO number
\ Each register set is 0x40 bytes apart
\   FIFO index (0-31)
\   Base register address
: can_fifo_register ( n addr - addr )
	SWAP $40 * +          	     	\ reg for the fifo
;

: can_setup_ports ( - )
    PPS_UNLOCK
    
	PORTF 0 DIGITAL_IN						\ RF0 input (RX)
	0x04 $0bf80fac8 !						\   for use by C1RXR

	PORTF 1 DIGITAL_OUT						\ RF1 output (TX)
	0x0c $0bf80FC44 !						\   for use by C1TX

	CAN1EN DIGITAL_OUT			        	\ RE2 output for CAN 1 transceiver
	CAN1EN DIGITAL_OFF  					\ Enable transceiver
	
	PPS_LOCK
;


: can_mode@ ( - n )
    C1CON 21 3 REG_BITS@                    \ read OPMOD (C1CON 23:21)
;

\ Set can mode
\ - 0 normal
\ - 2 loopback
\ - 4 configuration
: can_mode! ( n - )
	C1CON 24 3 REG_BITS!		    	    \ set REQOP (C1CON 26:24) to mode
	." Mode set " can_mode@ . CR      		\ should be mode - to show it is set
;


: can_sum_fifo_size ( u u - u )   4 4 * * + ;

\ Create FIFO n:-
\   FIFO index n (0-31)
\   for TX (flag is 1) or RX (flag is 0)
\   size (number of messages 1-16)
: can_add_fifo ( u t u - )
	ROT C1FIFOCON0 can_fifo_register
    SWAP 1- OVER                        \ offset from size
	16 5 REG_BITS!	                    \ set FSIZE (20:16) with FIFO size
	7 REG_BIT!						    \ set TXEN flag
;

\ Add message to a FIFO
\ 	Two words (8 bytes) of data
\   Data size in bytes (0-8)
\   SID
\   FIFO number (0-31)
: can_fifo! ( n n n x1 x2 - )
    2SWAP SWAP
    4 PICK                                      \ Copy of FIFO index
	C1FIFOUA0 can_fifo_register @				\ get FIFO 1 (TX fifo) current msg address
	TO_VIRTUAL_ADDR					        	\ convert to physical address
\ ." use buffer " .S CR
	TUCK !									    \ write bytes to successive words: SID;
	CELL+ TUCK !						            	\ Length;
	CELL+ TUCK !							        \ Bytes 0-3; and
	CELL+ !								        	\ Bytes 4-7

	C1FIFOCON0 can_fifo_register
	13 REG_BIT_SET						\ Set UINC bit - fifo increments pointer
	
\	.S CR
;

: can_fifo_ready ( n - flag )
	C1FIFOINT0 can_fifo_register
	0 REG_BIT@							\ confirm ready with data (RXNEMPTYIF)
;

\ Read the next message from the specified FIFO, data on stack is:-
\   - SID
\   - length (bytes) of message
\   - data, bytes 4-7
\   - data, bytes 0-3
: can_fifo@ ( n - x1 x2 n n )
    DUP can_fifo_ready IF
        DUP
		C1FIFOUA0 can_fifo_register @ TO_VIRTUAL_ADDR	\ get RX FIFO address

		DUP @ 0x7FF AND						        \ read register 1, the SID
		LROT CELL+ DUP @ 0xF AND  		    	    \ read register 2, Length

        LROT CELL+ DUP @							\ register 3, read data, bytes 0-3
		LROT CELL+ @						        \ register 4, read data, bytes 4-7

		LROT C1FIFOCON0 can_fifo_register
		13 REG_BIT_SET				                \ setting UINC tells fifo to increment pointer

		2SWAP 								        \ order CELLS and bring length/SID to top
        SWAP                                        \ order length and SID

	ELSE
		DROP
		0 0 0 0
	THEN
;


\ set up the mask pattern
\   - mask no (0-4)
\   - mask (0-7FF)
: can_sid_mask! ( u u - ) 21 LSHIFT SWAP C1RXM0 OFFSET_REGISTER ! ;

\   - mask no (0-4)
: can_sid_mask@ ( u - ) C1RXM0 OFFSET_REGISTER @ 21 RSHIFT ;

\ print the mask pattern
\   - mask no (0-4)
: .can_sid_mask ( u - ) can_sid_mask@ hex. ;

\ set up the filter pattern
\   - filter no (0-15)
\   - filter pattern (0-7FF)
: can_sid_filter! ( u u - ) 21 LSHIFT SWAP C1RXF0 OFFSET_REGISTER ! ;

\   - filter no (0-15)
: can_sid_filter@ ( u - ) C1RXF0 OFFSET_REGISTER @ 21 RSHIFT ;

\ print the filter pattern
\   - filter no (0-15)
: .can_sid_filter ( u - ) can_sid_filter@ hex. ;



\ C1FLTCON0: address  and bits for filter control register for filer number
\   filter number
\   flag bit
: can_filter_register ( u u - a-addr u )
    SWAP
    DUP
    4 mod 8 *                     \ bit 0, 8, 16, 24
    SWAP 4 / $10 * C1FLTCON0 +    \ in reg C1RXM0-3
    SWAP
    ROT +
;

: can_reset_filters ( )
    C1FLTCON0 0 OVER !                      \ clear filters 0-3
    CELL+ 0 OVER !                          \ clear filters 4-7
    CELL+ 0 OVER !                          \ clear filters 8-11
    CELL+ 0 SWAP !                          \ clear filters 12-15
;

\ Disable the filter for the given number (0-15)
: can_filter_dis ( u - )  7 can_filter_register REG_BIT_CLEAR ;

\ Enable the filter for the given number (0-15)
: can_filter_en ( u - )  7 can_filter_register REG_BIT_SET ;

\ Is filter n (0-15) enabled?
: .can_filter_enabled ( u - )  
    7 can_filter_register REG_BIT@  IF ." enabled " ELSE ." disabled " THEN ;

\ set the mask used for the filter
\   - filter number (0-15)
\   - mask number (0-4)
: can_filter_mask! ( u u - ) SWAP 5 can_filter_register 2 REG_BITS! ;

: can_filter_mask@ ( u - u ) 5 can_filter_register 2 REG_BITS@ ;

: .can_filter_mask ( u - ) can_filter_mask@ hex. ;

\ set the filter used for the filter
\   - filter number (0-15)
\   - fifo number  (0-15)
: can_filter_fifo! ( u u - ) SWAP 0 can_filter_register 5 REG_BITS! ;

: can_filter_fifo@ ( u - u ) 0 can_filter_register 5 REG_BITS@ ;

: .can_filter_fifo ( u - ) can_filter_fifo@ hex. ;

: .can_filter ( u - ) 
    dup .
    dup can_fifo_ready if ." RDY " else ."     " then
    dup C1FIFOUA0 can_fifo_register @ TO_VIRTUAL_ADDR	hex.
    dup .can_filter_enabled
    dup can_filter_mask@ .can_sid_mask
    can_filter_fifo@ .can_sid_filter
;

\ Set up the CAN peripheral
\   address of the first FIFO buffer
: can_init ( addr - )
    can_setup_ports

	\ Set up can module
	C1CON 15 REG_BIT_SET					    \ enable CAN module

    4 can_mode!				        			\ configuration mode

    \ Data base-address
\	0xFFFF AND								\ Physical address
    TO_PHYSICAL_ADDR
	DUP ." ADDR " HEX. CR
	C1FIFOBA !								\ set up address register

	\ Bit rate
	C1CFG 15 REG_BIT_SET    				\ set SEG2PHTS -- freely programmable
	2 C1CFG 16 3 REG_BITS!					\ SEG2PH (18:16) -- 2xTQ
	2 C1CFG 11 3 REG_BITS!					\ SEG1PH (13:11) -- 2xTQ
	2 C1CFG 8 3 REG_BITS!					\ PRSEG (10:8) -- 2xTQ
	C1CFG 14 REG_BIT_SET		       		\ set SAM -- sample three times
	2 C1CFG 6 2 REG_BITS!					\ SJW (7:6) -- length 3xTQ
	3 C1CFG 0 5 REG_BITS!					\ BRP (5:0) -- (2 x 4)/FSYS
;

: .can_status ( )
	HEX
	CR
	." TRAN EN " CAN1EN REG_BIT? CR
	." CAN EN " C1CON 15 REG_BIT? CR
	." MODE " C1CON 21 3 REG_BITS? CR   
	." C1CFG " C1CFG ? CR

	." SEG1PH " C1CFG 11 3 REG_BITS?	CR
	." SEG2PH " C1CFG 16 3 REG_BITS? CR
	." PRSEG " C1CFG 8 3 REG_BITS? CR

	." SJW " C1CFG 6 2 REG_BITS?	CR
	." BRP " C1CFG 0 5 REG_BITS?	CR
	." MEMORY " C1FIFOBA @ U. CR
	." MEMORY " C1FIFOUA0 @ TO_VIRTUAL_ADDR U.  CR

	DECIMAL
;



: can_send ( n - )
	C1FIFOCON0 can_fifo_register
	3 REG_BIT_SET						\ Set data sent TXREQ
;

: can_read_debug ( n - )
	DUP C1FIFOINT0 can_fifo_register
	." Ready "
	0 REG_BIT? CR				    \ confirm ready with data (RXNEMPTYIF)

	C1FIFOUA0 can_fifo_register @			\ get RX FIFO address
    TO_VIRTUAL_ADDR

	DUP @ 									\ read register 1
	DUP 0x7FF AND ." SID " . CR				\ SID
	16 RSHIFT ." TIME " . CR 				\ Time stamp

	CELL+ DUP @								\ read register 2
	0xF AND ." LEN " . CR					\ Length

	." DATA1 " CELL+ DUP ? CR				\ read register 3
											\ Data, bytes 0-3
	." DATA2 " CELL+ ?						\ read register 4
;

: can_debug ( n - )
	CR ." CAN FIFO #" DUP .
	C1FIFOUA0 can_fifo_register @ TO_VIRTUAL_ADDR
	\ TODO find size of FIFO from reg. limit to 32 (max fifo size?)
	DUP hex.
	32 DUMP
;


.( Set up CAN driver-PIC32MX570 board ) CR
echo
