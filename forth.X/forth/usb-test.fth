noecho
\ USB testing


HEX

0bf885040 CONSTANT U1OTGIR
0bf885060 CONSTANT U1OTGSTAT
0bf885070 CONSTANT U1OTGCON

0bf885080 CONSTANT U1PWRC
0bf885200 CONSTANT U1IR
0bf885220 CONSTANT U1EIR
0bf885240 CONSTANT U1STAT
0bf885250 CONSTANT U1CON
0bf885260 CONSTANT U1ADDR
0bf885270 CONSTANT U1BDTP1
0bf8852C0 CONSTANT U1BDTP2
0bf8852D0 CONSTANT U1BDTP3

0bf885300 CONSTANT U1EP0

0bf885280 CONSTANT U1FRML
0bf8852B0 CONSTANT U1SOF

DECIMAL


\ buffer descriptor table
CREATE BDT
    1024
    ALLOT

\ calculate the aligned address
0xffffff00 BDT AND 0x200 +
    CONSTANT BDT_START


CREATE 0_RX_EVEN ALIGN 64 ALLOT

CREATE 0_RX_ODD ALIGN 64 ALLOT

\  CREATE DEV_DESC2 ALIGN 18 ALLOT
\
\  \ TODO fix C,
\  \ Device descriptor
\  \  DEV_DESC
\  \  0x12 C,
\  \  0x01 C,
\  \  0x10 C,
\  \
\  \  DROP
\
\  0x12 DEV_DESC C!           \ length 18 bytes
\  0x01 DEV_DESC 1+ C!        \ type, device descriptor
\  0x10 DEV_DESC 2+ C!        \ USB release number (1.1)
\  0x01 DEV_DESC 3 + C!       \ USB release number
\  \  0x00 DEV_DESC 2+ C!        \ USB release number (2.0)
\  \  0x02 DEV_DESC 3 + C!       \ USB release number
\  0x02 DEV_DESC 4 + C!       \ CDC device
\  0x00 DEV_DESC 5 + C!       \ subclass
\  0x00 DEV_DESC 6 + C!       \ protocol
\  0x40 DEV_DESC 7 + C!       \ max packet size
\  0xD8 DEV_DESC 8 + C!       \ vendor id
\  0x04 DEV_DESC 9 + C!       \ vendor id
\  0x0A DEV_DESC 10 + C!      \ product id
\  0x00 DEV_DESC 11 + C!      \ product id
\  0x51 DEV_DESC 12 + C!      \ release number (3.51)
\  0x03 DEV_DESC 13 + C!      \ release number
\  0x00 DEV_DESC 14 + C!       \ manufacturer index
\  0x00 DEV_DESC 15 + C!       \ product index
\  0x00 DEV_DESC 16 + C!       \ serial number index
\  0x01 DEV_DESC 17 + C!       \ number of configurations


CREATE DEV_DESC

0x12 C,      \ length 18 bytes
0x01 C,      \ type, device descriptor
\  0x10 C,      \ USB release number (1.1)
\  0x01 C,      \ USB release number
0x00 C,      \ USB release number (2.0)
0x02 C,      \ USB release number
0x02 C,      \ CDC device
0x00 C,      \ subclass
0x00 C,      \ protocol
0x40 C,      \ max packet size
0xD8 C,      \ vendor id
0x04 C,      \ vendor id
0x0A C,      \ product id
0x00 C,      \ product id
0x51 C,      \ release number (3.51)
0x03 C,      \ release number
0x00 C,       \ manufacturer index
0x00 C,       \ product index
0x00 C,       \ serial number index
0x01 C,       \ number of configurations



\  CREATE CONF_DESC ALIGN 64 ALLOT
\
\  0x09 CONF_DESC 0 + C!   \ length
\  0x02 CONF_DESC 1 + C!   \ CONFIGURATION
\  48 CONF_DESC 2 + C!   \ total length
\  0 CONF_DESC 3 + C!     \
\  2 CONF_DESC 4 + C!          \ number of intefaces
\  1 CONF_DESC 5 + C!
\  0 CONF_DESC 6 + C!        \ index to configuration name
\  0xC0 CONF_DESC 7 + C!         \ attribute - self powered
\  0x32 CONF_DESC 8 + C!      \ max power - 100mA
\
\
\  \ CDC Communication interface
\  0x09 CONF_DESC 9 + C!       \ length
\  0x04 CONF_DESC 10 + C!      \ INTERFACE
\  0x00 CONF_DESC 11 + C!      \ interface number
\  0x00 CONF_DESC 12 + C!       \ alternate setting
\  0x01 CONF_DESC 13 + C!     \ number of endpoints
\  0x02 CONF_DESC 14 + C!       \ class - CDC communication
\  0x02 CONF_DESC 15 + C!       \ subclass
\  0x01 CONF_DESC 16 + C!      \ protocol
\  0x00 CONF_DESC 17 + C!      \ name index
\
\  0x07 CONF_DESC 18 + C!       \ length
\  0x05 CONF_DESC 19 + C!      \ ENDPOINT
\  0x81 CONF_DESC 20 + C!      \ endpoint address, IN
\  0x03 CONF_DESC 21 + C!       \ attribute - interrupt
\  0x08 CONF_DESC 22 + C!     \ max size
\  0x0 CONF_DESC 23 + C!       \
\  0x0a CONF_DESC 24 + C!       \ interval 10ms
\
\
\  \ data interface
\  0x09 CONF_DESC 25 + C!       \ length
\  0x04 CONF_DESC 26 + C!      \ INTERFACE
\  0x01 CONF_DESC 27 + C!      \ interface number
\  0x00 CONF_DESC 28 + C!       \ alternate setting
\  0x02 CONF_DESC 29 + C!     \ number of endpoints
\  0x0a CONF_DESC 30 + C!       \ class - CDC communication
\  0x00 CONF_DESC 31 + C!       \ subclass
\  0x00 CONF_DESC 32 + C!      \ protocol
\  0x00 CONF_DESC 33 + C!      \ name index
\
\  0x07 CONF_DESC 34 + C!       \ length
\  0x05 CONF_DESC 35 + C!      \ ENDPOINT
\  0x02 CONF_DESC 36 + C!      \ endpoint address, OUT
\  0x02 CONF_DESC 37 + C!       \ attribute - block
\  32 CONF_DESC 38 + C!     \ max size
\  0 CONF_DESC 39 + C!       \
\  0 CONF_DESC 40 + C!       \ ignore interval
\
\  0x07 CONF_DESC 41 + C!       \ length
\  0x05 CONF_DESC 42 + C!      \ ENDPOINT
\  0x82 CONF_DESC 43 + C!      \ endpoint address, IN
\  0x02 CONF_DESC 44 + C!       \ attribute - block
\  32 CONF_DESC 45 + C!     \ max size
\  0 CONF_DESC 46 + C!       \
\  0x00 CONF_DESC 47 + C!       \ ignore interval
\
\
\
\  \  0x07 CONF_DESC 39 + C!       \ length
\  \  0x05 CONF_DESC 40 + C!      \ ENDPOINT
\  \  0x81 CONF_DESC 41 + C!      \ endpoint address, IN
\  \  0x03 CONF_DESC 42 + C!       \ attribute - interrupt
\  \  32 CONF_DESC 43 + C!     \ max size
\  \  0 CONF_DESC 44 + C!       \
\  \  0x0a CONF_DESC 45 + C!       \ interval 10ms




CREATE CONF_DESC

0x09 C,             \ length
0x02 C,             \ CONFIGURATION
67   C,             \ total length
0x00 C,             \
0x02 C,             \ number of intefaces
0x01 C,
0x00 C,             \ index to configuration name
0xC0 C,             \ attribute - self powered
0x32 C,             \ max power - 100mA


\ CDC Communication interface
0x09 C,             \ length
0x04 C,             \ INTERFACE
0x00 C,             \ interface number
0x00 C,             \ alternate setting
0x01 C,             \ number of endpoints
0x02 C,             \ class - CDC communication
0x02 C,             \ subclass
0x01 C,             \ protocol
0x00 C,             \ name index



0x05 C,             \ length
0x24 C,             \ CS_INTERFACE
0x00 C,             \ Function header
0x10 C, 0x01 C,     \

0x04 C,             \ length
0x24 C,             \ CS_INTERFACE
0x02 C,             \ Function ACM
0x02 C,             \

0x05 C,             \ length
0x24 C,             \ CS_INTERFACE
0x06 C,             \ Function union
0x00 C,             \ CDC interface
0x01 C,             \ Data interface

0x05 C,             \ length
0x24 C,             \ CS_INTERFACE
0x01 C,             \ Function call management
0x00 C,
0x01 C,             \  Data Interface


0x07 C,             \ length
0x05 C,             \ ENDPOINT
0x81 C,             \ endpoint address, IN
0x03 C,             \ attribute - interrupt
0x08 C,             \ max size
0x00 C,             \
0x0a C,             \ interval 10ms


\ data interface
0x09 C,             \ length
0x04 C,             \ INTERFACE
0x01 C,             \ interface number
0x00 C,             \ alternate setting
0x02 C,             \ number of endpoints
0x0a C,             \ class - CDC communication
0x00 C,             \ subclass
0x00 C,             \ protocol
0x00 C,             \ name index

0x07 C,             \ length
0x05 C,             \ ENDPOINT
0x02 C,             \ endpoint address, OUT
0x02 C,             \ attribute - block
0x20 C,             \ max size
0x00 C,             \
0x00 C,             \ ignore interval

0x07 C,             \ length
0x05 C,             \ ENDPOINT
0x82 C,             \ endpoint address, IN
0x02 C,             \ attribute - block
0x20 C,             \ max size
0x00 C,             \
0x00 C,             \ ignore interval



\  0x07 CONF_DESC 39 + C!       \ length
\  0x05 CONF_DESC 40 + C!      \ ENDPOINT
\  0x81 CONF_DESC 41 + C!      \ endpoint address, IN
\  0x03 CONF_DESC 42 + C!       \ attribute - interrupt
\  32 CONF_DESC 43 + C!     \ max size
\  0 CONF_DESC 44 + C!       \
\  0x0a CONF_DESC 45 + C!       \ interval 10ms




\ CREATE INT_DESC ALIGN 64 ALLOT
\
\
\
\
\ \ Interface descriptor
\ 0x09 INT_DESC C!           \ length 18 bytes
\ 0x02 INT_DESC 1+ C!        \ type
\ 0x00 INT_DESC 2+ C!        \ interface number
\ 0x00 INT_DESC 3 + C!       \ alternate setting
\ 0x00 INT_DESC 4 + C!       \ number of endpoints
\ 0x03 INT_DESC 5 + C!       \ class
\ 0x01 INT_DESC 6 + C!       \ subclass
\ 0x02 INT_DESC 7 + C!       \ protocol
\ 0x00 INT_DESC 8 + C!       \ name index

VARIABLE USB_STATE
VARIABLE USB_ADDRESS
VARIABLE USB_EP0_TX_PACKET
VARIABLE USB_EP0_RX_PACKET


64 CONSTANT TX_BUFF_LEN
VARIABLE 0_TX_DATA
VARIABLE 0_TX_END

VARIABLE DEBUGGING

\ USB States:-
0 CONSTANT DETACHED
1 CONSTANT DEFAULT
2 CONSTANT ADDRESSED
3 CONSTANT CONFIGURED







\  \ 1's complement
\  : invert ( n - n ) NEGATE 1 - ;
\
\
\  \ source
\  \ value
\  \ pos
\  \ length
\  : set_bits ( n n n n - n )
\      OVER MASK_PATTERN INVERT    \ mask
\      LROT LSHIFT                 \ new value
\      LROT AND                      \ mask the source VALUE
\      OR                           \ combine
\  ;
\
\  \ source
\  \ pos
\  \ length
\  : get_bits ( n n n - n )
\      OVER MASK_PATTERN            \ mask
\      ROT AND                      \ mask the source VALUE
\      SWAP RSHIFT                 \ value
\  ;
\
\  : +! ( addr - )
\      DUP @ 1+ SWAP !
\  ;
\
\
: .HEXS ( )
    HEX .S DECIMAL
;









\  : change_order ( n - n )
\      DUP ." original " hex. CR
\      DUP 24 RSHIFT 0xff AND SWAP
\      DUP 16 RSHIFT 0xff AND 8 LSHIFT SWAP
\      DUP 8 RSHIFT 0xff AND 16 LSHIFT SWAP
\      0xff AND 24 LSHIFT OR OR OR
\
\      DUP ." endian " hex. CR
\  ;


: enable_packet_processing ( )
    U1CON 5 REG_BIT_CLEAR
;


\ endpoint
\ rx (0/1)
\ even (0/1)
: BDT_entry ( n n n - addr)
    ROT 1 LSHIFT
    ROT + 1 LSHIFT
    SWAP + 3 LSHIFT
    BDT_START +
;


: BDT_buffer_address ( addr - addr )
    CELL+ @  TO_VIRTUAL_ADDR
;


\ descriptor address
: BDT_pid ( addr - )
    @ 16 9 get_bits
;

\  : BDT_status ( addr  - )
\      ." @" DUP hex.
\      DUP @ DUP DUP DUP DUP
\      16 RSHIFT 0xfff AND .  ." bytes: "             \ byte count
\      0x80 AND IF ." HW" ELSE ." SW" THEN SPACE
\      0x40 AND IF ." DATA1" ELSE ." DATA0" THEN SPACE
\      2 RSHIFT 0x0f AND  hex.            \ bit pattern of PID
\
\      2 RSHIFT 0xfff AND  bin.            \ bit pattern of flags
\
\  \      4 + ." -> " @ TO_VIRTUAL_ADDR hex.
\      BDT_buffer_address ." -> " hex.
\  ;

\ descriptor address
: BDT_reset ( addr - )
\      DUP @
\      0 16 9 set_bits               \ 0 COUNT
\      0 2 6 set_bits SWAP !        \ disable all flags
    0 SWAP !
;


\ descriptor address
\ buffer addr
: BDT_buffer ( addr addr - )
\      OVER 0 SWAP !         ADDRESSED           \ clear first word
    TO_PHYSICAL_ADDR SWAP
    4 + !                             \ address to second word in buffer descriptor
;

\ descriptor address
: BDT_uown ( addr - )
    DUP @
    1 7 1 set_bits
    SWAP  !
;


\ descriptor address
: BDT_DTS ( addr - )
    DUP @
    1 3 1 set_bits              \ DTS - data toggle sync - flag
    SWAP  !
;

\ descriptor address
\ data (0/1)
: BDT_data ( addr n - )
    OVER @
    SWAP 6 1 set_bits
    SWAP  !
;

\ descriptor address
\ count
: BDT_count_expected ( addr n - )
    OVER @
    SWAP 16 9 set_bits
    SWAP !
;

\ descriptor address
\ count
: BDT_count_actual ( addr - n )
    @ 16 9 get_bits
;

\ descriptor address
: BDT_stall ( addr - )
    DUP @
    1 2 1 set_bits
    SWAP !
;

\ descriptor address
: BDT_unstall ( addr - )
    DUP @
    0 2 1 set_bits
    SWAP !
;

\ descriptor address
: BDT_disable_DMA ( addr - )
    DUP @
\      1 4 1 set_bits
    0 4 1 set_bits
    SWAP !
;

\ descriptor address
: BDT_PID ( addr - n )
    @ 2 4 get_bits
;


\ descriptor address
: BDT_remove_buffer ( addr - )
    DUP 0 BDT_count_expected         \ reset count to zero
    CELL+ 0 SWAP !         \ clear address
;


\ descriptor address
\ buffer size
: BDT_clear_buffer ( addr n - )
    SWAP BDT_buffer_address SWAP
\      SWAP CELL+ SWAP
\    ( debug ) ." clear @" 2DUP SWAP HEX. . CR
    ERASE
;


: token_processing_address ( - addr )
    U1STAT @ 1 LSHIFT BDT_START +
;


: usb_reset ( - )
    \ endpoint 0
    0 0 0 BDT_entry DUP DUP   \ DUP
    BDT_reset
    BDT_disable_DMA
    0_RX_EVEN BDT_buffer
\      64 BDT_clear_buffer

    0 0 1 BDT_entry DUP DUP    \ DUP
    BDT_reset
    BDT_disable_DMA
    0_RX_ODD BDT_buffer
\      64 BDT_clear_buffer

    0 1 0 BDT_entry DUP
    BDT_reset
    BDT_remove_buffer

    0 1 1 BDT_entry DUP
    BDT_reset
    BDT_remove_buffer

    0 U1ADDR !          \ reset address
    0 USB_ADDRESS !
    DEFAULT USB_STATE !

    0 USB_EP0_TX_PACKET !
    0 USB_EP0_RX_PACKET !
    U1CON 1 REG_BIT_SET
\      5 ms
    U1CON 1 REG_BIT_CLEAR

\      enable_packet_processing
;


: usb_enable ( )
    U1CON 0 REG_BIT_SET  \ enable USB (USBEN)
;

: usb_disable ( )
    U1CON 0 REG_BIT_CLEAR  \ disable USB (USBEN)
;


: debug_BDT_target ( addr - )
    DUP DUP
    BDT_START - 32 /
\      5 RSHIFT 0xf AND
    ." #" DUP .   ." EP/" CELLS U1EP0 + @ HEX.                   \ endpoint
    0x10 AND IF ." TX" ELSE ." RX" THEN         \ direction
    SPACE
    0x08 AND IF ." ODD" ELSE ." EVEN" THEN      \ ping-pong position
;

: debug_BD_data ( addr - )
    DUP BDT_count_actual
    SWAP BDT_buffer_address
    DUP ." @" HEX.
    ." ["
    SWAP 0
    \ TODO add DUP? to allow this to be simplified
    2DUP <> IF
        DO DUP i + C@ HEX. LOOP
    ELSE
        2DROP
    THEN
    ." ]"
    DROP \ address
;


: debug_BD ( addr  - )
    ." | "
    DUP debug_BDT_target SPACE
    DUP @
    DUP 16 RSHIFT 0xfff AND .  ." bytes: "         \ byte count
    DUP 0x80 AND IF ." MOD" ELSE ." PRG" THEN SPACE
    DUP 0x40 AND IF ." DATA1" ELSE ." DATA0" THEN SPACE
    2 RSHIFT 0xfff AND  bin.                        \ bit pattern of flags
\      DEBUGGING @ IF
        ." -> " debug_BD_data
\      ELSE
\          DROP
\      THEN
;


: debug_BDs ( n - )
    DUP 0 0 BDT_entry debug_BD CR
    DUP 0 1 BDT_entry debug_BD CR
    DUP 1 0 BDT_entry debug_BD CR
    1 1 BDT_entry debug_BD CR
;


\ list the Descriptors addresses for a specified endpoint
: debug_BDT ( n - )
    DUP 0 0 BDT_entry   DUP hex. SPACE ." -> "     debug_BDT_target CR
    DUP 0 1 BDT_entry   DUP hex. SPACE ." -> "     debug_BDT_target CR
    DUP 1 0 BDT_entry   DUP hex. SPACE ." -> "     debug_BDT_target CR
    1 1 BDT_entry   DUP hex. SPACE ." -> "     debug_BDT_target CR


\      DUP debug_BDT_target hex.   SPACE ." -> @"   DUP 0 0 BDT_entry CR
\      DUP debug_BDT_target hex.   SPACE ." -> @"   DUP 0 1 BDT_entry CR
\      DUP debug_BDT_target hex.   SPACE ." -> @"   DUP 1 0 BDT_entry CR
\      DUP debug_BDT_target hex.   SPACE ." -> @"   1 1 BDT_entry CR
;


: debug_recvd ( )
    ." @" U1ADDR ?
    token_processing_address
    debug_BD
;

: debug_reg ( )
    ." USB"  CR
    SPACE SPACE ." PWR " U1PWRC @ HEX. CR

    SPACE SPACE ." OTG IR " U1OTGIR @ HEX. CR
    SPACE SPACE ." OTG STAT " U1OTGSTAT @ HEX. CR
    SPACE SPACE ." OTG CON " U1OTGCON @ HEX. CR

    SPACE SPACE ." USB IR " U1IR @ HEX. CR
    SPACE SPACE ." USB EIR " U1EIR @ HEX. CR
    SPACE SPACE ." USB STAT " U1STAT @ HEX. CR
    SPACE SPACE ." USB CON " U1CON @ HEX. CR
    SPACE SPACE ." USB ADDR " U1ADDR @ HEX. CR
    SPACE SPACE ." USB BDT " U1BDTP3 @ HEX.

    SPACE U1BDTP2 @ HEX. SPACE U1BDTP1 @ HEX. CR
    SPACE SPACE ." EP0 " U1EP0 @ HEX.
;

: debug_state ( - )
\      DEBUGGING @ IF
        ." |   @" U1ADDR ?

        USB_STATE @
        DUP DETACHED = IF
            ." DETACHED"
        THEN
        DUP DEFAULT = IF
            ." DEFAULT"
        THEN
        DUP ADDRESSED = IF
            ." ADDRESSED"
        THEN
        DUP CONFIGURED = IF
            ." CONFIGURED"
        THEN
        DROP SPACE


        ."  : IR " U1IR @ HEX.
        ."  ; EIR " U1EIR @ HEX.
        ."  ; STAT " U1STAT @ HEX.
        ."  ; CON " U1CON @ HEX.
        CR
        ." | "
        token_processing_address BDT_START - 8 MOD
        ." endpoint " DUP  . CR
        debug_BDs
\      THEN
;

: debug_usb ( )
    ." USB status" CR
    ."  Power " U1PWRC 0 REG_BIT? CR
    ."  State " USB_STATE ? CR
    ."  Address " U1ADDR ? CR
    ."  Descriptors " BDT_START HEX. CR
    0 debug_BDT
    1 debug_BDT
    2 debug_BDT
    CR
;

: debug_clear_ir ( )
    0xff U1OTGIR !          \ clear all interrupt flags
    0xff U1IR !             \ clear all interrupt flags
    0xff U1EIR !            \ clear all interrupt flags
;


: BD_READ_BYTE ( addr n -- n )
    4 /MOD    \ 2DUP SWAP . .
    4 *
    ROT
    +       \ DUP HEX.
    @
\              ." -> " DUP HEX.

    SWAP 8 * RSHIFT 0xff AND
\      DUP HEX.

\      SWAP DROP
;

\ buffer size
\ data 0/1
: rx_control ( n n  -  )
 \   ( debug ) HEX
    0 0 USB_EP0_RX_PACKET @ 2 MOD BDT_entry    \ set up receive for RX data stage
    DUP BDT_reset
    DUP BDT_disable_DMA
    DUP ROT BDT_data
\    ( debug ) ." start " .HEXS CR
    DUP ROT 2DUP BDT_count_expected
\    ( debug ) .HEXS CR
    BDT_clear_buffer
\    DUP BDT_DTS

    DEBUGGING @ IF
        ( debug ) ." prep " DUP debug_BD CR
    THEN

    BDT_uown

    USB_EP0_RX_PACKET +!
;

: rx_control_setup ( -  )
    64 0 rx_control
;

: rx_control_status ( -  )
\      0 1 rx_control
    64 1 rx_control
;


: tx_control_BD ( -- addr )
    0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry    \ set up device descriptor for TX data stage
\    ( debug ) ." prep " DUP debug_BDT_target cr
    DUP BDT_reset
    USB_EP0_TX_PACKET +!
;

\ buffer size
\ buffer address
: tx_control_data ( n addr -- addr )
    0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry    \ set up device descriptor for TX data stage
\    ( debug ) ." prep " DUP debug_BDT_target cr
    DUP BDT_reset
\      .HEXS CR
    DUP ROT BDT_buffer
    DUP ROT BDT_count_expected
    DUP 1 BDT_data
\    DUP BDT_DTS

    DEBUGGING @ IF
        ( debug ) ." prep " DUP debug_BD CR
    THEN

    DUP BDT_uown

    USB_EP0_TX_PACKET +!
;


\ data address
: tx_send ( addr -- )

    DUP 0_TX_END @ TUCK
    <= IF
        -

        1
        DUP TX_BUFF_LEN > IF DROP TX_BUFF_LEN THEN

        0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry
        USB_EP0_TX_PACKET +!

        DUP BDT_reset
        DUP BDT_count_expected
        DUP SWAP BDT_data
        DUP BDT_buffer
        BDT_uown
    THEN
;


\ 2 LOG

\ data address
\  : tx_send ( addr -- )
\
\      DUP 0_TX_END @ TUCK   \       ( start end start end )
\      <=
\      IF
\          -             \          ( start len )
\
\          1                                                   \ data number  TODO calculate
\          DUP TX_BUFF_LEN > IF DROP TX_BUFF_LEN THEN      \ limit lenth to max packet size
\
\      \    \ set up device descriptor for TX data stage
\          0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry     \ get address of next buffer descriptor
\          USB_EP0_TX_PACKET +!                        \                 ( data no, packet len, data addres, buffer descriptor )
\      \        ( debug ) ." prep a " DUP debug_BDT_target cr
\
\          DUP BDT_reset
\          DUP BDT_count_expected
\          DUP SWAP BDT_data
\          DUP BDT_buffer
\      \        ( debug ) ." prep b " DUP debug_BD CR
\          BDT_uown
\
\      THEN
\  ;


\ endpoint number
\ buffer length
\ buffer address
: tx_send_data ( n n' addr -- )
    DUP DUP 0_TX_DATA !             \ new buffer is the start of data
    ROT + 0_TX_END !                \ end of data is at length after start
    tx_send

    DROP                            \ endpoint not being used yet
;


\ endpoint number
: tx_send_next ( n  -- )
    TX_BUFF_LEN                   \ next data point is buffer length on
    0_TX_DATA @ +
    DUP 0_TX_DATA !
    tx_send

    DROP                            \ endpoint not being used yet
;


: processing_descriptor ( -- addr addr' n )
    token_processing_address
    DUP BDT_buffer_address          ( descriptor, buffer )
    OVER BDT_PID                    ( descriptor, buffer, PID )
;


: process_default_token ( -- )
    processing_descriptor           ( descriptor, buffer, PID )

    \ SETUP packet
    DUP 13 = IF
        SPACE SPACE ." > SETUP - "

        \ check request is GET_DESCRIPTOR fpr DEVICE
        OVER @ 0x01000680 = IF
            ." GET_DESCRIPTOR: DEVICE" CR
            enable_packet_processing

            18 DEV_DESC tx_control_data DROP
            rx_control_status
            rx_control_setup
        THEN

        \ check request is device standard SET_ADDRESS
        OVER @ 0xffff AND 0x0500 = IF
            SPACE SPACE ." SET_ADDRESS "

            OVER @  16 RSHIFT  USB_ADDRESS !
            USB_ADDRESS ? CR

            enable_packet_processing

\              ." pause... " 2000 ms ." continue " CR
            0 DEV_DESC tx_control_data DROP      \ prepare for ZLP response

            2 ms

            USB_ADDRESS @ 0x7F AND U1ADDR !     \ set up address of device
            rx_control_setup
\              ." > address set " U1ADDR ? CR
            ADDRESSED USB_STATE !
        THEN
    THEN


    \ IN packet
    \ device to host (IN) transaction
    DUP 9 = IF
        SPACE SPACE ." < IN" CR
        tx_send_next
    THEN

    \ OUT packet
    \ host to device (OUT) transaction
    DUP 1 = IF
        SPACE SPACE ." < OUT" CR
    THEN


    DROP    \ PID
    DROP    \ buffer
    DROP    \ descriptor
;


: process_addressed_token ( -- )
    processing_descriptor           ( descriptor, buffer, PID )

    \ SETUP packet
    DUP 13 = IF
        SPACE SPACE ." > SETUP - "

        \ check request is GET_DESCRIPTOR for DEVICE
        OVER @ 0x01000680 = IF
            ." GET_DESCRIPTOR: DEV" CR
\              ." pause... " 500 ms ." continue " CR

\              0 debug_BDs

            18 DEV_DESC tx_control_data
\                    ( debug ) ." prep " DUP debug_BD CR
            DROP
\              rx_control_status
\              rx_control_setup
            rx_control_status

\            ( debug )
\              CR 0 debug_BDs

            enable_packet_processing
        THEN


        \ check request is GET_DESCRIPTOR for CONFIGURATION
        OVER @ 0x02000680 = IF
            ." GET_DESCRIPTOR: CONF" CR

            OVER 6 BD_READ_BYTE    DUP .       \ read size

            0 SWAP  tx_send_data

\              \ TODO convert to word that sends multiple segments of buffer
\              DUP 64 > IF
\                  64 CONF_DESC tx_control_data DROP
\                  64 -
\                  CONF_DESC 64 + tx_control_data 0 BDT_data
\              ELSE
\                  CONF_DESC tx_control_data DROP
\              THEN


            rx_control_status

            enable_packet_processing
        THEN


        \ check request is SET CONFIGURATION
        OVER @ 0x010900 = IF
            OVER 2 BD_READ_BYTE
            ." SET_CONF " . CR
\              OVER 6 BD_READ_BYTE DUP .

\              CONF_DESC tx_control_data DROP
             0 DEV_DESC tx_control_data DROP      \ prepare for ZLP response

            rx_control_status

            CONFIGURED USB_STATE !

            enable_packet_processing
        THEN


        \ check request is DEVICE QUALIFIER
        OVER @ 0x06000680 = IF
            ." DEV_QUALIFIER " CR

\              0 DEV_DESC tx_control_data      \ prepare for ZLP response


            tx_control_BD
            DUP BDT_stall
            BDT_uown

           rx_control_status

            debug_state

\              rx_control_status

            enable_packet_processing
        THEN


    THEN


    \ IN packet
    DUP 9 = IF
        SPACE SPACE ." < IN" CR                         \ device to host (IN) transaction
        tx_send_next
\              rx_control_status
    THEN

    \ OUT packet
    DUP 1 = IF                              \ host to device (OUT)
        SPACE SPACE ." > OUT" CR
          rx_control_setup
    THEN

    DROP    \ PID
    DROP    \ buffer
    DROP    \ descriptor
;


: process_token ( - )
    ." ("  U1IR @ hex. ." ) "
    ." +> PROCESS "


\      token_processing_address $10 AND IF
\
\      THEN




    USB_STATE @ CONFIGURED = IF
        ." CONFIGURED " CR SPACE SPACE debug_recvd CR
\          process_configured_token
        ." NO action" CR
    THEN

    USB_STATE @ ADDRESSED = IF
        ." ADDRESSED " CR SPACE SPACE debug_recvd CR
        process_addressed_token
\        ." NO action" CR
    THEN

    USB_STATE @ DEFAULT = IF
        ." DEFAULT " CR SPACE SPACE debug_recvd CR
        process_default_token
    THEN

    0x08 U1IR !        \ clear interrupt

    DEBUGGING @ IF
        debug_state
        U1IR @
        ." >> TOKEN processed 0x" hex.
        ."  --> " debug_recvd CR
    THEN
    CR
;


: process_reset ( -- )
    DEBUGGING @ IF
        ." !! Prior to reset" CR debug_state CR
    THEN

    ." +> USB reset ("  U1IR @ hex. ." )" CR

    BEGIN U1IR @ 0x08 AND
    WHILE
        SPACE SPACE ." drop " token_processing_address debug_BD CR
        0x08 U1IR !        \ clear interrupt
    REPEAT

    usb_reset
    rx_control_setup
\      rx_control_setup
    0x01 U1IR !        \ clear interrupt


    DEBUGGING @ IF
        debug_state
        ." >> USB reset complete"
    THEN
    CR CR
;


:  handle_usb ( -  )
    CR CR CR ." ----------------------------------------------" CR
    0xff U1IR !        \ clear interrupt
    CR ." USB ready" CR

    begin
        U1EIR @ 0> IF
            ." error " U1EIR @ hex. CR
            0xff U1EIR !
        THEN

        \ recognise detached state
        U1OTGIR @ 0x01 AND IF
            ." +> DETACHED" CR
            DETACHED USB_STATE !
            0x01 U1OTGIR ! \ reset interrpt
        THEN


		U1IR @
\  		DUP HEX.

		DUP
        0x01 AND IF
            process_reset
\              0xff U1EIR !

            \ TODO is this needed?
\              DROP 0
        THEN

        DUP
        0x08 AND IF
            process_token
        THEN


        DUP
        0x04 AND IF            \ SOF received
            0x04 U1IR !        \ clear interrupt
        THEN

        DUP
        0x20 AND IF
            ." +> RESUME detected" CR CR
            0x20 U1IR !        \ clear interrupt
        THEN

        0x10 AND IF
            ." +> IDLE condition" CR CR
            0x10 U1IR !        \ clear interrupt
        THEN


        U1EIR @ 0> IF
            ." error raised " U1EIR @ hex. CR
            0xff U1EIR !

            ABORT
        THEN

        DEPTH 0> IF
            ." !stack not zero: " .HEXS CR
            clear
        THEN

\          0x08 AND NOT IF
\              1 ms
\          THEN

        10 ms
	again
;



: usb_init ( - )
    U1CON 0 REG_BIT_CLEAR       \ disable USB (USBEN)
    U1PWRC 0 REG_BIT_CLEAR     \ turn off USB module (0)
    100 ms

    U1PWRC 0 REG_BIT_SET     \ turn on USB module (0)

    0x0 U1OTGCON !         \ Full speed - pull up D+ (7)
                            \ VBUS not powered (3)
                            \ OTGEN pull up/down controlled  by software ???? (2)
                            \ No VBUS charge/discharge (1-0)

    0x0 U1EP0 !            \ Disbale Tx/Rx for endpoint 0

    DETACHED USB_STATE !

    \ set up BDT address registers
    BDT_START TO_PHYSICAL_ADDR DUP DUP
        24 RSHIFT 0xff AND U1BDTP3 !
        16 RSHIFT 0xff AND U1BDTP2 !
        8 RSHIFT 0xff AND U1BDTP1 !

    usb_reset

    rx_control_setup
\      rx_control_setup

    0x0D U1EP0 !            \ Enable Tx/Rx for endpoint 0
;

\  1 log
\ echo


task usb_t

: run  ( )  usb_t activate handle_usb ;



\ start USB
usb_init
\  debug_usb
\  1 DEBUGGING !
run
\ usb_enable




\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
\
















\  : setup_token ( addr -  )
\      DUP @ 0xffff AND 0x0680 = IF              \ check request is GET_DESCRIPTOR
\          ." GET_DESCRIPTOR: "
\
\          DUP @ 24 RSHIFT
\
\          DUP 0x01 = IF                            \ device descriptor
\              ." DEVICE" CR
\
\              USB_STATE 0= IF
\                  1 USB_STATE !           \  attached
\              THEN
\
\              18 DEV_DESC tx_control_data DROP
\  \              64 DEV_DESC tx_control_data
\              rx_control_status
\  \              rx_control_setup
\  \              CR status_usb
\              DROP 0
\          THEN
\
\          DUP 0x02 = IF                               \ configuration descriptor
\              ." CONFIGURATION" CR
\
\  \              OVER 4 + @
\  \              16 RSHIFT 0xffff AND
\  \              DUP ." size " hex. CR
\
\              9 CONF_DESC tx_control_data DROP
\
\  \              0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry DUP DUP DUP DUP   \ set up configuration descriptor TX for data stage
\  \              BDT_reset
\  \              CONF_DESC BDT_buffer
\  \              1 BDT_data
\  \  \            ROT .S BDT_count_expected
\  \                9 BDT_count_expected
\  \              BDT_uown
\
\              0 0 USB_EP0_RX_PACKET @ 2 MOD BDT_entry DUP DUP DUP DUP    \ set up device descriptor RX for data stage
\              BDT_reset
\              0 BDT_count_expected
\              1 BDT_data
\              BDT_DTS
\              BDT_uown
\
\
\              OVER 4 + @
\              16 RSHIFT 0xffff AND
\              DUP ." size " hex. CR
\
\              0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry
\              SWAP BDT_count_expected
\
\  \              USB_EP0_TX_PACKET +!
\              USB_EP0_RX_PACKET +!
\
\              DROP 0
\          THEN
\
\
\          DUP 0x00 <> IF
\              \ for all others, a zero length packet
\              ." OTHER " DUP . OVER @ 24 RSHIFT . CR
\              ." error request " OVER 2@ hex. hex. CR
\  \              0
\
\              0 DEV_DESC tx_control_data DROP
\
\  \              0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry DUP DUP DUP DUP    \ set up device descriptor
\  \              BDT_reset
\  \              DEV_DESC BDT_buffer
\  \              0 BDT_count_expected
\  \              1 BDT_data
\  \              BDT_uown
\  \
\  \              USB_EP0_TX_PACKET +!
\
\          THEN
\
\          DROP                    \ descriptor no
\      THEN
\
\
\      DUP @ 0xffff AND 0x0500 = IF             \ check request is device standard SET_ADDRESS
\          ." SET_ADDRESS "
\
\  \          reset_usb
\          0 USB_EP0_TX_PACKET !
\          0 USB_EP0_RX_PACKET !
\          U1CON 1 REG_BIT_SET
\
\  \          ADDRESSED USB_STATE !
\          DUP @  16 RSHIFT  USB_ADDRESS !
\          USB_ADDRESS ? CR
\  \          CR status_usb
\
\  \          rx_control_setup
\          rx_control_setup
\          0 DEV_DESC tx_control_data           \ prepare for ZLP response
\          DROP
\
\          ticks
\  \          0x08 U1IR !        \ clear interrupt
\
\          enable_packet_processing
\
\          USB_ADDRESS @ 0x7F AND
\          U1ADDR !
\          ADDRESSED USB_STATE !
\
\          ticks swap - CR ." set " U1ADDR ? ." in ms " . CR
\
\  \          CR status_usb
\
\
\
\
\
\
\  \          ." >> " OVER HEx. CR
\  \          U1IR @ hex. U1STAT 1 LSHIFT @ hex. CR
\  \
\  \  \          DUP 64 DUMP
\  \
\  \
\  \
\  \  \          1000 ms
\  \
\  \          rx_control_setup
\  \
\  \          0 DEV_DESC tx_control_data           \ prepare for ZLP response
\  \  \             DUP BDT_stall
\  \  \
\  \  \            3 ms
\  \  \
\  \  \            BDT_unstall
\  \  \          DROP
\  \          ." => " hex. CR
\  \
\  \          ticks
\  \
\  \          0x08 U1IR !        \ clear interrupt
\  \          U1IR @ hex. U1STAT 1 LSHIFT @ hex. CR
\  \
\  \  \          40 ms
\  \  \          USB_ADDRESS @ 0x7F AND
\  \  \          U1ADDR !
\  \
\  \  \          rx_control_setup
\  \
\  \
\  \          \          BEGIN
\  \  \              U1IR @ 0x08 AND
\  \  \              0x08 =
\  \  \          UNTIL
\  \          enable_packet_processing
\  \  \          3 ms
\  \
\  \          U1IR @ hex. U1STAT 1 LSHIFT @ hex. CR
\  \
\  \          USB_ADDRESS @ 0x7F AND
\  \          U1ADDR !
\  \
\  \          ticks swap - CR ." set " U1ADDR ? ." in ms " . CR
\  \
\  \          2 USB_STATUS !
\  \
\  \  \          rx_control_setup
\  \          CR
\  \
\  \          0x08 U1IR !        \ clear interrupt
\  \          U1IR @ hex. U1STAT 1 LSHIFT @ hex. CR
\  \
\
\      THEN
\
\
\      DUP @ 0xffff AND 0x0900 = IF             \ check request is device standard GET_INTERFACE
\          ." SET_CONFIGURATION "
\          DUP @ . CR
\
\          0 DEV_DESC tx_control_data DROP
\
\  \          rx_control_setup
\  \          0 DEV_DESC tx_control_data           \ prepare for ZLP response
\  \
\  \          0 0 USB_EP0_RX_PACKET @ 2 MOD BDT_entry DUP DUP DUP DUP    \ set up RX for next set up stage
\  \          BDT_reset
\  \          0 BDT_count_expected
\  \          1 BDT_data
\  \          BDT_DTS
\  \          BDT_uown
\  \
\  \  \          USB_EP0_TX_PACKET +!
\  \          USB_EP0_RX_PACKET +!
\
\      THEN
\
\
\      DUP @ 0xffff AND 0x0A81 = IF             \ check request is device standard GET_INTERFACE
\          ." GET_INTERFACE " DUP hex.
\          \ not expected yet
\          64 DEV_DESC tx_control_data DROP
\      THEN
\
\      DROP                    \ address
\
\
\  ;




echo
