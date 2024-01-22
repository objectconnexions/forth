noecho

\ lock

HEX

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

2 PORTE 2CONSTANT C1EN

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
	SYS_UNLOCK
	13 CFGCON REG_BIT_CLEAR					\ unlock PPS

	0 PORTF DIGITAL_IN						\ RF0 -> C1RXR
	0x04 0x0bf80fac8 !						\ set RF0 as used by C1RXR

	1 PORTF DIGITAL_OUT						\ RF1 -> C1TX
	0x0c 0x0bf80FC44 !						\ set RPF1R as used by C1TX

	C1EN DIGITAL_OUT			        			\ RE2 -> C1EN
	C1EN REG_BIT_CLEAR						\ Enable transciever

	13 CFGCON REG_BIT_SET					\ lock PPS
	SYS_LOCK
;


: can_mode@ ( - n )
    21 3 C1CON REG_BITS@                    \ read OPMOD (C1CON 23:21)
;

: can_mode! ( n - )
	24 3 C1CON REG_BITS!		    	    \ set REQOP (C1CON 26:24) to mode
	." Mode set " can_mode@ . CR      		\ should be mode - to show it is set
;

\ Create FIFO n:-
\   for TX (flag is 1) or RX (flag is 0)
\   size (number of messages 1-16)
\   FIFO index n (0-31)
: can_add_fifo ( flag n n  - )
	C1FIFOCON0 can_fifo_register
    SWAP 1- OVER                        \ offset from size
	16 5 ROT REG_BITS!	                \ set FSIZE (20:16) with FIFO size
	7 SWAP REG_BIT!						\ set TXEN flag
\ ???	7 C1FIFOCON0 REG_BIT!		\ Set TXEN for TX
;

	\ FIFO buffers
\	3 16 5 C1FIFOCON0 REG_BITS!				\ set FSIZE (20:16) with FIFO size of
\	7 C1FIFOCON0 REG_BIT_CLEAR				\ Clear TXEN for RX

\	1 16 5 C1FIFOCON0 OFFSET + REG_BITS!	    \ set FSIZE (20:16) with FIFO size
\	7 C1FIFOCON0 OFFSET + REG_BIT_SET		\ Set TXEN for TX


\ Create a mask for filtering message, with:
\   mask index (0-3)
\   mask bitmap
: can_fifo_mask ( n n - )
	C1RXM0 OFFSET_REGISTER                   \ address for register n
 	21 11 ROT REG_BITS!						\ Filter uses SID (31:21) as mask
;

\ Add filter for an Rx FIFO
\ 	Mask index (0-3)
\   FIFO index (0-31)
\	Filter index (0-31)
\   SID pattern (0-0x7FF)
: can_fifo_filter ( u u u u - )
                                    \ Derive 8 bits of control data
	5 LSHIFT			            		\ TOS is mask, bits 6:5
	OR		        		    			\ NOS is FIFO index, bits 4:0
                                    \ S: SID filter# control-data

    OVER                            \ Copy the filter index
    8 SWAP                          \ Add literal for size of control bits in register, used later

    4 /MOD                          \ Get register offset and byte offset
    C1FLTCON0 OFFSET_REGISTER       \ Calc register address (8 regs for 32 entries)
    SWAP
                                    \ S: SID filter# control-data 8 addr byte-offset
    2DUP
    2ROT 2ROT
                                    \ S: SID filter# addr byte-offset control-data 8  addr byte-offset

    8 * LROT            		        \ Calc offset within register (4 per cell)
    REG_BITS!
\    CR .( Write to control reg ) 2SWAP SWAP . .  SWAP . . CR

                                    \ S: SID filter# addr byte-offset
    2SWAP SWAP $15 LSHIFT SWAP      \ adjust pattern positon (31:21)
    C1RXF0 OFFSET_REGISTER          \ Filter register for index        
    !                               \ writer pattern to filter register
\    .( Write to filter reg ) SWAP . . CR
    
    8 * 7 +                         \ determine bit to enable
    SWAP REG_BIT_SET                \ set enable in control register 
\    .( enable ) SWAP . . CR
;

\ Set up the CAN peripheral
\   address of the first FIFO buffer
\   mode to set up in (2 for loopback)
: can_init ( addr - )
    can_setup_ports

	\ Set up can module
	15 C1CON REG_BIT_SET					    \ enable CAN module

    4 can_mode!				        			\ configuration mode

.S

	\ Data base address
	0xFFFF AND								\ Physical address
\    TO_PHYSICAL
	DUP ." ADDR " HEX. CR
	C1FIFOBA !								\ set up address register

	\ Bit rate
	15 C1CFG REG_BIT_SET    				\ set SEG2PHTS -- freely programmable
	2 16 3 C1CFG REG_BITS!					\ SEG2PH (18:16) -- 2xTQ
	2 11 3 C1CFG REG_BITS!					\ SEG1PH (13:11) -- 2xTQ
	2 8 3 C1CFG REG_BITS!					\ PRSEG (10:8) -- 2xTQ
	14 C1CFG REG_BIT_SET		       		\ set SAM -- sample three times
	2 6 2 C1CFG REG_BITS!					\ SJW (7:6) -- length 3xTQ
	3 0 5 C1CFG REG_BITS!					\ BRP (5:0) -- (2 x 4)/FSYS
;

: can_status ( )
	HEX
	CR
	." TRAN EN " C1EN REG_BIT? CR
	." CAN EN " 15 C1CON REG_BIT? CR
	." MODE " 21 3 C1CON REG_BITS? CR
	." C1CFG " C1CFG ? CR

	." SEG1PH " 11 3 C1CFG REG_BITS?	CR
	." SEG2PH " 16 3 C1CFG REG_BITS? CR
	." PRSEG " 8 3 C1CFG REG_BITS? CR

	." SJW " 6 2 C1CFG REG_BITS?	CR
	." BRP " 0 5 C1CFG REG_BITS?	CR
	." MEMORY " C1FIFOBA @ U. CR
	." MEMORY " C1FIFOUA0 @ TO_PHYSICAL U.  CR

	DECIMAL
;


\ Add message to a FIFO
\   SID
\   Data size in bytes (0-8)
\ 	Two words (8 bytes) of data
\   FIFO number (0-31)
: can_write ( n x1 x2 n n - )
    4 PICK                                       \ Copy of FIFO index
	C1FIFOUA0 can_fifo_register @				\ get FIFO 1 (TX fifo) current msg address
	TO_PHYSICAL							        	\ convert to physical address

	TUCK !									    \ write bytes to successive words: SID;
	CELL+ TUCK !						            	\ Length;
	CELL+ TUCK !							        \ Bytes 0-3; and
	CELL+ !								        	\ Bytes 4-7

	C1FIFOCON0 can_fifo_register
	13 SWAP REG_BIT_SET						\ Set UINC bit - fifo increments pointer
;

: can_send ( n - )
	C1FIFOCON0 can_fifo_register
	3 SWAP REG_BIT_SET						\ Set data sent TXREQ
;

: can_read_ready ( n - flag )
	C1FIFOINT0 can_fifo_register
	0 SWAP REG_BIT@							\ confirm ready with data (RXNEMPTYIF)
;

\ Read the next message from the specified FIFO, data on stack is:-
\   - SID
\   - length (bytes) of message
\   - data, bytes 4-7
\   - data, bytes 0-3
: can_read ( n - x1 x2 n n )
    DUP can_read_ready IF
        DUP
		C1FIFOUA0 can_fifo_register @ TO_PHYSICAL	\ get RX FIFO address

		DUP @ 0x7FF AND						\ read register 1, the SID
		LROT
		CELL+ DUP @ 0xF AND  		    	\ read register 2, Length
        LROT

		CELL+ DUP @							\ register 3, read data, bytes 0-3
		LROT
		CELL+ @						        \ register 4, read data, bytes 4-7
		LROT

		C1FIFOCON0 can_fifo_register
		13 SWAP REG_BIT_SET				    \ setting UINC tells fifo to increment pointer

		2SWAP 								\ order CELLS and bring length/SID to top
        SWAP                                \ order length and SID

	ELSE
		DROP
		0 0 0 0
	THEN
;

: can_read_debug ( n - )
	DUP C1FIFOINT0 can_fifo_register
	." Ready "
	0 SWAP REG_BIT? CR				    \ confirm ready with data (RXNEMPTYIF)

	C1FIFOUA0 can_fifo_register @								\ get RX FIFO address
	TO_PHYSICAL

\	.S CR

	DUP @ 									\ read register 1
	DUP 0x7FF AND ." SID " . CR				\ SID
	16 RSHIFT ." TIME " . CR 				\ Time stamp

	CELL+ DUP @								\ read register 2
	0xF AND ." LEN " . CR					\ Length

	CR

	." DATA1 " CELL+ DUP ? CR				\ read register 3
											\ Data, bytes 0-3
	." DATA2 " CELL+ ?						\ read register 4
;

: can_debug ( n - )
	CR ." CAN FIFO #" DUP .
	C1FIFOUA0 can_fifo_register @
	TO_PHYSICAL
	\ TODO find size of FIFO from reg. limit to 32 (max fifo size?)
	32 DUMP
;


.( Set up CAN driver-PIC32MX570 board ) CR
echo
