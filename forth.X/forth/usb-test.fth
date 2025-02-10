noecho
\ USB testing

.( Test USB code loading )

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
0x20 C,      \ max packet size
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


VARIABLE USB_STATE
VARIABLE USB_ADDRESS
VARIABLE USB_EP0_TX_PACKET
VARIABLE USB_EP0_RX_PACKET


32 CONSTANT TX_PCKT_LEN
VARIABLE 0_TX_DATA
VARIABLE 0_TX_COUNT
VARIABLE 0_TX_LEN

VARIABLE DEBUGGING

\ USB States:-
0 CONSTANT DETACHED
1 CONSTANT DEFAULT
2 CONSTANT ADDRESSED
3 CONSTANT CONFIGURED



CREATE 1_TX_EVEN ALIGN 32 ALLOT

CREATE 1_TX_ODD ALIGN 32 ALLOT

VARIABLE USB_EP1_TX_PACKET


CREATE 2_RX_EVEN ALIGN 32 ALLOT

CREATE 2_RX_ODD ALIGN 32 ALLOT

CREATE 2_TX_EVEN ALIGN 32 ALLOT

CREATE 2_TX_ODD ALIGN 32 ALLOT

VARIABLE USB_EP2_TX_PACKET
VARIABLE USB_EP2_RX_PACKET




\
: .HEXS ( )
    HEX .S DECIMAL
;





\ Re-enable packet processing after setup token received
: enable_packet_processing ( )
    U1CON 5 REG_BIT_CLEAR
;

\ Get the BDT entry address for specified endpoint
\ - endpoint
\ - rx (0/1)
\ - even (0/1)
: BDT_entry ( n n n - addr)
    ROT 1 LSHIFT
    ROT + 1 LSHIFT
    SWAP + 3 LSHIFT
    BDT_START +
;

\ Get the virtual address for a BDT buffer
: BDT_buffer_address ( addr - addr )
    CELL+ @  TO_VIRTUAL_ADDR
;

\ ???
\ - descriptor address
: BDT_pid ( addr - )
    @ 16 9 get_bits
;

\ Reset the data count and flags for BDT
\ - descriptor address
: BDT_reset ( addr - )
\      DUP @
\      0 16 9 set_bits               \ 0 COUNT
\      0 2 6 set_bits SWAP !        \ disable all flags
    0 SWAP !
;


\ Set up the the buffer for a BDT to read/write to
\ - descriptor address
\ - buffer addr
: BDT_buffer ( addr addr - )
\      OVER 0 SWAP !         ADDRESSED           \ clear first word
    TO_PHYSICAL_ADDR SWAP
    4 + !                             \ address to second word in buffer descriptor
;

\ Set the BDT to be owned by the module, rather than the software
\ - descriptor address
: BDT_uown ( addr - )
    DUP @
    1 7 1 set_bits
    SWAP  !
;


\ Set the DTS flag for the BDT
\ - descriptor address
: BDT_DTS ( addr - )
    DUP @
    1 3 1 set_bits              \ DTS - data toggle sync - flag
    SWAP  !
;

\ Set the data number (1 or 0) for the BDT
\ - descriptor address
\ - data (0/1)
: BDT_data ( addr n - )
    OVER @
    SWAP 6 1 set_bits
    SWAP  !
;

\ Set the expected data count for the BDT
\ - descriptor address
\ - count
: BDT_count_expected ( addr n - )
    OVER @
    SWAP 16 9 set_bits
    SWAP !
;

\ Read the actual data count for the BDT
\ - descriptor address
\ - count
: BDT_count_actual ( addr - n )
    @ 16 9 get_bits
;

\ Set the Stall flag for the BDT
\ - descriptor address
: BDT_stall ( addr - )
    DUP @
    1 2 1 set_bits
    SWAP !
;

\ Clear the Stall flag for the BDT
\ - descriptor address
: BDT_unstall ( addr - )
    DUP @
    0 2 1 set_bits
    SWAP !
;

\ Clear the DMA flag for the BDT
\ - descriptor address
: BDT_disable_DMA ( addr - )
    DUP @
\      1 4 1 set_bits
    0 4 1 set_bits
    SWAP !
;

\ Get the PID from the BDT
\ - descriptor address
: BDT_PID ( addr - n )
    @ 2 4 get_bits
;

\ Clear the biffer from the BDT - does not change the content of the buffer,
\ just unlinks it from the table (see next word).
\ - descriptor address
: BDT_remove_buffer ( addr - )
    DUP 0 BDT_count_expected         \ reset count to zero
    CELL+ 0 SWAP !         \ clear address
;

\ Clears buffer pointed to by the BDT
\ - descriptor address
\ - buffer size
: BDT_clear_buffer ( addr n - )
    SWAP BDT_buffer_address SWAP
    ERASE
;

\ Lookup the address of the next DBT entry from the U1STAT register. Only
\ valid when the token available interrupt is set.
: token_processing_address ( - addr )
    U1STAT @ 1 LSHIFT BDT_START +
;


: usb_reset ( - )
    \ endpoint 0
    0 0 0 BDT_entry DUP DUP   \ DUP
    BDT_reset
    BDT_disable_DMA
    0_RX_EVEN BDT_buffer

    0 0 1 BDT_entry DUP DUP    \ DUP
    BDT_reset
    BDT_disable_DMA
    0_RX_ODD BDT_buffer

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
    U1CON 1 REG_BIT_SET     \ reset ping pong buffer to even
    U1CON 1 REG_BIT_CLEAR
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

    ." -> " debug_BD_data
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

    SWAP 8 * RSHIFT 0xff AND
;

\ endpoint
\ buffer size
\ data 0/1
: rx_control ( n n n  -  )
\   ( debug ) HEX
    ROT 0 USB_EP0_RX_PACKET @ 2 MOD BDT_entry    \ set up receive for RX data stage
    DUP BDT_reset
    DUP BDT_disable_DMA
    DUP ROT BDT_data
\    ( debug ) ." start " .HEXS CR
    DUP ROT 2DUP BDT_count_expected
\    ( debug ) .HEXS CR
    BDT_clear_buffer
\    DUP BDT_DTS

    DEBUGGING @ IF
        ( debug ) .HEXS CR
        ( debug ) ." prep " DUP debug_BD CR
    THEN

    DUP
    BDT_uown
    ( debug ) ." prep " debug_BD CR

    USB_EP0_RX_PACKET +!
;

: rx_control_setup ( -  )
    0 64 0 rx_control
;

: rx_control_status ( -  )
\      0 1 rx_control
    0 64 1 rx_control
;


: tx_control_BD ( -- addr )
    0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry    \ set up device descriptor for TX data stage
\    ( debug ) ." prep " DUP debug_BDT_target cr
    DUP BDT_reset
    USB_EP0_TX_PACKET +!
;


\ len
\ data0/1
\ start
: tx_send_data   ( n n' addr )
    \ set up device descriptor for TX data stage
    0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry     \ get address of next buffer descriptor
    USB_EP0_TX_PACKET +!        (  data addr, len, data no, BD addr )

\      .HEXS CR
    DUP BDT_reset
    TUCK SWAP BDT_buffer
    TUCK SWAP BDT_data
    TUCK SWAP BDT_count_expected
    DUP BDT_uown
    ( debug ) ." prep b "  debug_BD CR
;


\ data address
: tx_send ( -- )

    0_TX_LEN @                         \ total length to send
    0_TX_COUNT @ TX_PCKT_LEN *          \ length already sent
    2DUP SWAP . ." ~ " .
    2DUP - ."  (length " . ." ) >> "
    ( total, sent )

    >= IF
        ." packet #" 0_TX_COUNT @ . ." ,"
\          ." calc " .HEXS CRS

        \ transfer length - packet size or the remaining length if smaller
        0_TX_LEN @
        0_TX_COUNT @ TX_PCKT_LEN *
        -                   \ remaining length to send
        TX_PCKT_LEN MIN
\        ( debug) ." size " DUP . CR

        \ data0 or 1
        0_TX_COUNT @ 1+ 2 MOD                \ data0 or 1
\        ( debug) SPACE SPACE ." data" DUP . CR

        \ data address
        0_TX_COUNT @ TX_PCKT_LEN *          \ length of data already sent
        0_TX_DATA @  +                      \ -> data addr, within buffer
\        ( debug) ." data @ " DUP HEX. CR

        \ transaction count for next tx
        0_TX_COUNT +!

\       ." => " .HEXS CR
        tx_send_data
    ELSE
\       ." drop start and len " CR
\          2DROP
\        DROP
        ." no prep " CR
    THEN

\    .HEXS CR
;


\ endpoint number
\ buffer length
\ buffer address
: tx_send_data ( n n' addr -- )
    ." Send new" CR
    0_TX_DATA !             \ new buffer is the start of data
    0_TX_LEN !
    DROP                            \ endpoint not being used yet
    0 0_TX_COUNT !
    tx_send
    tx_send
;


\ endpoint number
: tx_send_next ( n  -- )
    ." Next " CR
    tx_send

    DROP                            \ endpoint not being used yet
;


\ buffer size
\ buffer address
: tx_control_data ( n addr -- addr )
    0 LROT tx_send_data

    \ TODO resolve this
    0   \ added as each tc_control call drops the duplicated address
;


: setup_cdc ( -- )
    ." setup CDC"

    0x0 U1EP0 CELL+ !            \ Disbale Tx/Rx for endpoint 1
    0x0 U1EP0 2 CELLS + !            \ Disbale Tx/Rx for endpoint 2

    \ endpoint 1 - CDC control
    1 1 0 BDT_entry DUP
    BDT_reset
    1_TX_EVEN BDT_buffer

    1 1 1 BDT_entry DUP
    BDT_reset
    1_TX_ODD BDT_buffer

    1 USB_EP1_TX_PACKET !


    \ endpoint 2 - CDC data
    2 0 0 BDT_entry DUP DUP   \ DUP
    BDT_reset
    BDT_disable_DMA
    2_RX_EVEN BDT_buffer

    2 0 1 BDT_entry DUP DUP    \ DUP
    BDT_reset
    BDT_disable_DMA
    2_RX_ODD BDT_buffer

    2 1 0 BDT_entry DUP
    BDT_reset
    1_TX_EVEN BDT_buffer

    2 1 1 BDT_entry DUP
    BDT_reset
    2_TX_ODD BDT_buffer

    0 USB_EP2_TX_PACKET !
    0 USB_EP2_RX_PACKET !


    \ enable endpoints
    0x04 U1EP0 CELL+ !           \ Enable Tx for endpoint 1
    0x1D U1EP0 2 CELLS + !           \ Enable Tx/Rx for endpoint 2

    \ assign buffers
;



: processing_descriptor ( -- addr addr' n )
    token_processing_address
    DUP BDT_buffer_address          ( descriptor, buffer )
    OVER BDT_PID                    ( descriptor, buffer, PID )
;

\   descriptor, buffer, PID
: process_default_token ( addr addr n -- )
    \ SETUP packet
    DUP 13 = IF
        SPACE SPACE ." > SETUP - "

        \ check request is GET_DESCRIPTOR fpr DEVICE
        OVER @ 0x01000680 = IF
            ." GET_DESCRIPTOR: DEVICE" CR
\              enable_packet_processing

            rx_control_status
              rx_control_setup

            18 DEV_DESC tx_control_data DROP
\              ." sending " .hEXS CR

            enable_packet_processing

\              debug_state
        THEN

        \ check request is device standard SET_ADDRESS
        OVER @ 0xffff AND 0x0500 = IF
            SPACE SPACE ." SET_ADDRESS "

            OVER @  16 RSHIFT  USB_ADDRESS !
            USB_ADDRESS ? CR

\            enable_packet_processing

\              ." pause... " 2000 ms ." continue " CR
            0 DEV_DESC tx_control_data DROP      \ prepare for ZLP response

            enable_packet_processing

            2 ms

            USB_ADDRESS @ 0x7F AND U1ADDR !     \ set up address of device
            rx_control_setup
            ." > address set " U1ADDR ? CR
            ADDRESSED USB_STATE !

\              debug_state
        THEN
    THEN


    \ IN packet
    \ device to host (IN) transaction
    DUP 9 = IF
        SPACE SPACE ." < IN" CR
        0 tx_send_next
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

\ descriptor, buffer, PID
: process_addressed_token ( addr addr n -- )

    \ SETUP packet
    DUP 13 = IF
        SPACE SPACE ." > SETUP - "

        \ check request is GET_DESCRIPTOR for DEVICE
        OVER @ 0x01000680 = IF
            ." GET_DESCRIPTOR: DEV" CR
\              ." pause... " 500 ms ." continue " CR

\              0 debug_BDs

            rx_control_status
            18 DEV_DESC tx_control_data DROP
              rx_control_setup
\              rx_control_status

\            ( debug )
\              CR 0 debug_BDs

            enable_packet_processing
        THEN


        \ check request is GET_DESCRIPTOR for CONFIGURATION
        OVER @ 0x02000680 = IF
            ." GET_DESCRIPTOR: CONF" CR

            OVER 6 BD_READ_BYTE    ." READ " DUP . CR      \ read size

            0 SWAP CONF_DESC tx_send_data

            rx_control_setup
            rx_control_status

            enable_packet_processing
        THEN


        \ check request is SET CONFIGURATION
        OVER @ 0x010900 = IF
            OVER 2 BD_READ_BYTE
            ." SET_CONF " . CR

            0 DEV_DESC tx_control_data DROP      \ prepare for ZLP response

            rx_control_status

            rx_control_setup   \ for next command

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

            rx_control_setup \ ???
           rx_control_status

           enable_packet_processing
        THEN


    THEN


    \ IN packet
    DUP 9 = IF
        SPACE SPACE ." < IN" CR                         \ device to host (IN) transaction
        0 tx_send_next
\              rx_control_status
    THEN

    \ OUT packet
    DUP 1 = IF                              \ host to device (OUT)
        SPACE SPACE ." > OUT" CR
\            rx_control_setup
    THEN

    DROP    \ PID
    DROP    \ buffer
    DROP    \ descriptor
;



\ descriptor, buffer, PID
: process_configured_token ( addr addr n -- )

    \ SETUP packet
    DUP 13 = IF
        SPACE SPACE ." > SETUP - "

        \ check request is LIN_CODING for CDC
        OVER @ 0x00002021 = IF
            ." LINE_CODING: DEV" CR

            rx_control_status
            rx_control_setup
            enable_packet_processing
        THEN



    THEN


    \ IN packet
    DUP 9 = IF
        SPACE SPACE ." < IN" CR                         \ device to host (IN) transaction
        0 tx_send_next
\              rx_control_status
    THEN

    \ OUT packet
    DUP 1 = IF                              \ host to device (OUT)
        SPACE SPACE ." > OUT" CR
\            rx_control_setup
    THEN

    DROP    \ PID
    DROP    \ buffer
    DROP    \ descriptor
;


: debug_IR ( -- )
    ." {IR="  U1IR @ hex. ." } "
;

: handle_token ( - )
    ." token  " debug_IR  ." STAT=" U1STAT @ HEX. CR

    processing_descriptor           ( descriptor, buffer, PID )

    DUP 0 = IF
        SPACE SPACE ." <++ NO PID " debug_IR

        DROP    \ PID
        DROP    \ buffer
        DROP    \ descriptor

        0x08 U1IR !        \ clear interrupt

        exit
    THEN

    \ IN packet
    \ device to host (IN) transaction
    DUP 9 = IF
        SPACE SPACE ." <++ IN " debug_IR

        DROP    \ PID
        DROP    \ buffer
        DROP    \ descriptor

        CR     token_processing_address debug_BD

        0 tx_send_next


        ." | " U1CON @ HEX. CR
        0 debug_BDs

        0x08 U1IR !        \ clear interrupt
        debug_IR

        exit
    THEN





    ." <++ "
    DUP 13 = IF ." SETUP"
    ELSE
        DUP 1 = IF ." OUT"
        ELSE ." PID=" DUP . THEN
    THEN
    debug_IR


    USB_STATE @ CONFIGURED = IF
        ." CONFIGURED " CR SPACE SPACE debug_recvd CR
        process_configured_token
    THEN

    USB_STATE @ ADDRESSED = IF
        ." /ADDRESSED " CR SPACE SPACE debug_recvd CR
        process_addressed_token
   THEN

    USB_STATE @ DEFAULT = IF
        ." /DEFAULT " CR SPACE SPACE debug_recvd CR
        process_default_token
    THEN


    ." | " U1CON @ HEX. CR
    0 debug_BDs

    0x08 U1IR !        \ clear interrupt

    DEBUGGING @ IF
        debug_state
        U1IR @
        ." >> TOKEN processed 0x" hex.
        ."  --> " debug_recvd CR
    THEN
;


: handle_reset ( -- )
    ." <++ USB reset " debug_IR CR
    debug_state

    0 U1ADDR !          \ reset address
    0 USB_ADDRESS !
    DEFAULT USB_STATE !

    0 USB_EP0_TX_PACKET !
    0 USB_EP0_RX_PACKET !
    U1CON 1 REG_BIT_SET     \ reset ping pong buffer to even
    U1CON 1 REG_BIT_CLEAR
    enable_packet_processing


    rx_control_setup
\      rx_control_setup
    0x01 U1IR !        \ clear interrupt


    DEBUGGING @ IF
        debug_state
        ." >> USB reset complete"
    THEN

   debug_IR
    ." ------"
    CR
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
            ." <++ DETACHED" CR ." ++>" CR
            DETACHED USB_STATE !
            0x01 U1OTGIR !      \ reset interrpt
        THEN


		U1IR @
\    		DUP HEX.

        DUP
        0x04 AND IF            \ SOF received
            0x04 U1IR !        \ clear interrupt
        THEN


        DUP
        0x01 AND IF
            handle_reset
            ." ++>" CR CR

            DROP U1IR @
        THEN

        DUP
        0x08 AND IF
            handle_token
            ." ++>" CR CR
        THEN

        DUP
        0x80 AND IF
            ." !!stalled" CR
            0x80 U1IR !        \ clear interrupt
        THEN
        DUP
        0x20 AND IF
            ." <++ RESUME detected" CR ." ++>" CR
            0x20 U1IR !        \ clear interrupt
        THEN

        0x10 AND IF
            ." <++ IDLE condition" CR ." ++>"  CR
            0x10 U1IR !        \ clear interrupt
        THEN

        U1EIR @ 0> IF
            ." !!error raised " U1EIR @ hex. CR
            0xff U1EIR !

            ABORT
        THEN

        DEPTH 0> IF
            ." !!stack not zero: " .HEXS CR
            clear
        THEN

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

    0x0D U1EP0 !            \ Enable Tx/Rx for endpoint 0
;


task usb_t

: run  ( )  usb_t activate handle_usb ;



\ start USB
usb_init
\  debug_usb
\  1 DEBUGGING !
run
\ usb_enable



echo
