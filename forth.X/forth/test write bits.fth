
noecho

\ write bits at position in register
\ : REG_BITS! ( value pos len address -- )

HEX

%00010010 C 8 10002000

	SWAP 2SWAP						\ ( addr len val pos)
	SWAP ROT					    \ ( - pos val len )
	0 MASK							\ ( - - val mask ) generate a mask for the bits within the register
	
	DUP ROT 					    \ ( - pos mask mask val )
	AND								\ ( - pos mask masked-value )
	LROT						    \ ( - masked-value pos mask )
	OVER LSHIFT						\ ( - - pos clear-out ) calculate the clear mask register change value 

	LROT    						\ ( - pos clear-out masked-value pos )
	LSHIFT							\ ( - pos clear-out value-out) calculate the register clear value 
	
	
	ROT SWAP OVER					\ ( clear-out addr val-out addr )
	2SWAP
	.S CR
	
	REG_CLR OR SWAP .( CLR ) . .( -> ) . CR
	REG_SET OR SWAP .( SET ) . .( -> ) . CR
	
\	REG_CLR OR !					\ clear register section bits
\	REG_SET OR !					\ write register section
\ ;


echo
