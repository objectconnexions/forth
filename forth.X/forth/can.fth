noecho

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


040 CONSTANT OFFSET

0146 CONSTANT SID


DECIMAL


CREATE FIFO 6 4 4 * * ALIGN ALLOT		\ create FIFO buffers

: can_init ( - )
	SYS_UNLOCK
	13 CFGCON REG_BIT_CLEAR				\ unlock PPS
	
	\ Set up ports
\	0 PORTF DIGITAL_IN					\ RF0 = C1RX
	0x04 0x0bf80fac8 !					\ set RF0 as used by C1RXR
	0 PORTF TRISA REG_BY_OFFSET			\ offset to register
	REG_BIT_CLEAR						\ set to output

\	1 PORTF DIGITAL_OUT					\ RF1 = C1TX
	0x0c 0x0bf80FC44 !					\ set RPF1R as used by C1TX
	1 PORTF TRISA REG_BY_OFFSET			\ offset to register
	REG_BIT_CLEAR						\ set to input
	
	C1EN DIGITAL_OUT					\ RE2 = C1EN
	C1EN REG_BIT_CLEAR					\ Enable transciever
	
	13 CFGCON REG_BIT_SET				\ lock PPS
	SYS_LOCK
	
			
	\ Set up can module		
	15 C1CON REG_BIT_SET				\ enable CAN module
	4 24 3 C1CON REG_BITS!				\ set REQOP (C1CON 26:24) to configuration
	21 3 C1CON REG_BITS?				\ should be 4 - to show it is set

	\ Bit rate	
	15 C1CFG REG_BIT_SET				\ set SEG2PHTS -- freely programmable
	2 16 3 C1CFG REG_BITS!				\ SEG2PH (18:16) -- 2xTQ
	2 11 3 C1CFG REG_BITS!				\ SEG1PH (13:11) -- 2xTQ
	2 8 3 C1CFG REG_BITS!				\ PRSEG (10:8) -- 2xTQ
	14 C1CFG REG_BIT_SET				\ set SAM -- sample three times
	2 6 2 C1CFG REG_BITS!				\ SJW (7:6) -- length 3xTQ
	3 0 5 C1CFG REG_BITS!				\ BRP (5:0) -- (2 x 4)/FSYS



	\ FIFO buffers
	3 16 5 C1FIFOCON0 REG_BITS!			\ set FSIZE (20:16) with FIFO size of 
	7 C1FIFOCON0 REG_BIT_CLEAR			\ Clear TXEN for RX

	1 16 5 C1FIFOCON0 OFFSET + REG_BITS!	\ set FSIZE (20:16) with FIFO size
	7 C1FIFOCON0 OFFSET + REG_BIT_SET	\ Set TXEN for TX

	FIFO
	ALIGN								\ align to boundary -- should this be needed, as we created it aligned??
	0xFFFF AND							\ Physical address
	." ADDR " ..
	C1FIFOBA !							\ set up address register

	..
	
	0x07FF 21 11 C1RXM0 REG_BITS!		\ Set SID (31:21) to all set
	
	0 0 5 C1FLTCON0 REG_BITS!			\ Filter 0 for FIFO 0 - FSEL0 (4:0)
	0 5 2 C1FLTCON0 REG_BITS!			\ Filter 0 uses mask 0 - MSEL0 (6:5)
	SID 21 11 C1RXF0 REG_BITS!			\ Filter matches against SID (146) - SID (31:21)
	7 C1FLTCON0 REG_BIT_SET				\ Enable filter 0 - FLTEN0 (7)
	
	2 24 3 C1CON REG_BITS!				\ set REQOP (C1CON 26:24) to loop-back mode
	21 3 C1CON REG_BITS?				\ should be 2 - to show it is set
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
	." MEMORY " C1FIFOBA ? CR
	
	DECIMAL
;


: can_write ( - )
	C1FIFOUA0 OFFSET + @					\ get FIFO 1 (TX fifo) current msg address
	0x80000000 OR
	.S
	4 + SID OVER !						\ write bytes to successive bytes
	4 + 2 OVER !
	4 + 12 OVER !
	4 + 34 SWAP !
	
	C1FIFOCON0 OFFSET +
	13 OVER REG_BIT_SET					\ Set UINC bit - fifo increments pointer
	3 SWAP REG_BIT_SET					\ Set data sent TXREQ
;

: can_read ( - ) 
	0 C1FIFOINT0 REG_BIT?				\ confirm ready with data
	
	C1FIFOUA0 @							\ get RX FIFO address
	0x80000000 OR
	
	.S
	
	DUP @ . 
	4 + DUP @ .
	4 + DUP @ .
	4 + @ .
	
	13 C1FIFOCON0 REG_BIT_SET			\ setting UINC tells fifo to increment pointer
;

echo
