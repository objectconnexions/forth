noecho

\ Add filter for an Rx FIFO
\ 	Mask index (0-3)
\   FIFO index (0-31)
\	Filter index (0-31)
\   SID pattern (0-0x7FF)
\ : can_fifo_filter ( n n n n - )

HEX

\ test data
\ $1a8 $0 1 0

CR .S CR
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
\    REG_BITS!
    CR .( Write to control reg ) 2SWAP SWAP . .  SWAP . . CR

                                    \ S: SID filter# addr byte-offset
    2SWAP SWAP $15 LSHIFT SWAP      \ adjust pattern positon (31:21)
    C1RXF0 OFFSET_REGISTER          \ Filter register for index        
\    !                              \ writer pattern to filter register
    .( Write to filter reg ) SWAP . . CR
    
    8 * 7 +                         \ determine bit to enable
\   SWAP REG_BIT_SET                \ set enable in control register 
    .( enable ) SWAP . . CR


CR
    .S CR

echo
