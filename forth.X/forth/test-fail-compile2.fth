
\ test for how compile and interpreter failures are handled 
\ when loading a file

HEX


: INIT_SPI ( -- )
	1 2 +
;

.s INIT_SPI cr .S words

: FAILS  ()
	2 3 +
	DUP +
	SWAP
;

: FAIL2 ( ) 
	2 DOUBLE
	. CR
;


: DOUBLE ( n n -- 2n ) DUP + ;

: SQUARE ( n n -- n^2 ) DUP * ;

: INC ( n -- n+1 ) 1 + ;

: FINAL  ( ) 
	FIFO ?
;

: CAN 


	CREATE FIFO 64 ALLOT			\ create FIFO buffers
	C1FIFOBA !						\ set up address register

	C1RXM0
	D B 7FF	REG_BITS_WRITE			\ Set SID (31:21) to all set
;


: can_init ( - )
	15 C1CON REG_BIT_SET			\ enable CAN module
	
	C1CON 4 3 18 REG_BIT_WRITE
	
	3 18 MASK						\ mask for clearing REQOP (C1CON 26:24)
	DUP
	C1CON REG_CLR OR !				\ clear REQOP
	4 18 LSHIFT 					\ mask for setting REQOP
	DUP
	C1CON REG_SET OR !				\ set REQOP - configuration mode
									\ ( clear-mask set-mask )
	BEGIN
		2DUP						\ copy masks
		SWAP
		C1CON @ AND					\ read OPMD
		=							\ repeat until matches mask
	UNTIL
	2DROP							\ clear stack 

	
	C1FIFOCON0 DUP					\ FIFO0 CON register
	11 5 3 REG_BITS_WRITE			\ set FSIZE (20:16) with FIFO size
	7 SWAP REG_BIT_WRITE					\ Set TXEN

	C1FIFOCON0 1+ DUP				\ FIFO1 CON register
	11 5 3 REG_BITS_WRITE			\ set FSIZE (20:16) with FIFO size
	7 SWAP REG_BIT_WRITE					\ Set TXEN

	CREATE FIFO 64 ALLOT			\ create FIFO buffers
	C1FIFOBA !						\ set up address register

	C1RXM0
	D B 7FF	REG_BITS_WRITE			\ Set SID (31:21) to all set
;


