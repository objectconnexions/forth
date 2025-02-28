noecho
\ USB testing

.( Test USB code loading )
.( ... )
.( )

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
0xEF C,      \ Miscellaneous device
0x00 C,      \ subclass
0x00 C,      \ protocol
0x20 C,      \ max packet size
0xD8 C,      \ vendor id
0x04 C,      \ vendor id
0x0A C,      \ product id
0x00 C,      \ product id
0x01 C,      \ release number (0.01)
0x00 C,      \ release number
0x00 C,       \ manufacturer index
0x00 C,       \ product index
0x00 C,       \ serial number index
0x01 C,       \ number of configurations


CREATE CONF_DESC
0x09 C,             \ length
0x02 C,             \ CONFIGURATION
75   C,             \ total length
0x00 C,             \
0x02 C,             \ number of intefaces
0x01 C,
0x00 C,             \ index to configuration name
0xa0 C,             \ attribute - not self powered
0x32 C,             \ max power - 100mA

\ Interface association
0x08 C,             \ length
0x0b C,             \ INTERFACE ASSOCIATION)
0x00 C,             \ first interface
0x02 C,             \ interface count
0x02 C,             \ class - CDC communication interface
0x02 C,             \ subclass - Abstract Control Model
0x00 C,             \ protocol - none
0x00 C,             \ name index


\ CDC Communication interface
0x09 C,             \ length
0x04 C,             \ INTERFACE
0x00 C,             \ interface number
0x00 C,             \ alternate setting
0x01 C,             \ number of endpoints
0x02 C,             \ class - CDC communication interface
0x02 C,             \ subclass - Abstract Control Model
0x00 C,             \ protocol - none
0x00 C,             \ name index

0x05 C,             \ length
0x24 C,             \ CS_INTERFACE
0x00 C,             \ Function header
0x10 C, 0x01 C,     \ version 1.1

0x05 C,             \ length
0x24 C,             \ CS_INTERFACE
0x01 C,             \ Function call management
0x01 C,             \ handles call management
0x01 C,             \ Data Interface

0x04 C,             \ length
0x24 C,             \ CS_INTERFACE
0x02 C,             \ Function ACM
0x06 C,             \ Device supports the request combination of Send_Break Set_Line_Coding,
                    \ Set_Control_Line_State, Get_Line_Coding, and the
                    \ notification Serial_State

0x05 C,             \ length
0x24 C,             \ CS_INTERFACE
0x06 C,             \ Function union
0x00 C,             \ CDC interface
0x01 C,             \ Data interface

0x07 C,             \ length
0x05 C,             \ ENDPOINT
0x81 C,             \ endpoint address, IN
0x03 C,             \ attribute - interrupt
0x10 C,             \ max size
0x00 C,             \
0x40 C,             \ interval 10ms

\ CDC data interface
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
0x40 C,             \ max size
0x00 C,             \
0x00 C,             \ ignore interval

0x07 C,             \ length
0x05 C,             \ ENDPOINT
0x83 C,             \ endpoint address, IN
0x02 C,             \ attribute - block
0x40 C,             \ max size
0x00 C,             \
0x00 C,             \ ignore interval


VARIABLE USB_STATE
VARIABLE USB_ADDRESS


32 CONSTANT TX_PCKT_LEN
0 CONSTANT RX
1 CONSTANT TX
0 CONSTANT DATA0
1 CONSTANT DATA1
0 CONSTANT EVEN
1 CONSTANT ODD

\ endpoints
0 CONSTANT CONTROL
1 CONSTANT CDC_CON
2 CONSTANT CDC_DATA_RX
3 CONSTANT CDC_DATA_TX

0 DEV_DESC 2CONSTANT ZLP

\ Tables with 4 values for each endpoint, each entry contains 5 cells
\ data pointer (address), length (bytes), block count and odd/even buffer flag for Tx and Rx
CREATE TX_DATA ALIGN 4 5 *  CELLS ALLOT

VARIABLE DEBUGGING

\ USB States:-
0 CONSTANT DETACHED
1 CONSTANT DEFAULT
2 CONSTANT ADDRESSED
3 CONSTANT CONFIGURED



CREATE 1_TX_EVEN ALIGN 16 ALLOT

CREATE 1_TX_ODD ALIGN 16 ALLOT

CREATE 2_RX_EVEN ALIGN 64 ALLOT

CREATE 2_RX_ODD ALIGN 64 ALLOT

CREATE 3_TX_EVEN ALIGN 64 ALLOT

CREATE 3_TX_ODD ALIGN 64 ALLOT



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
: BDT_entry ( n n n - addr )
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
: BDT_data01 ( addr n - )
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



\ address for the data to send for the specified endpoint
\ - endpoint
: tx_data_data ( n -- n addr )
    DUP 5 * CELLS TX_DATA +
;

\ address for the length of data to send for the specified endpoint
\ - endpoint
: tx_data_len ( n -- n addr )
    DUP 5 * 1+ CELLS TX_DATA +
;

\ address for the count of data sent for the specified endpoint
\ - endpoint
: tx_data_pkt ( n -- n addr )
    DUP 5 * 2+ CELLS TX_DATA +
;

\ address for the tx odd/even buffer flag for the specified endpoint
\ - endpoint
: tx_data_odd_even ( n -- addr )
    5 * 3 + CELLS TX_DATA +
;

\ address for the rx odd/even buffer flag for the specified endpoint
\ - endpoint
: rx_data_odd_even ( n -- addr )
    5 * 4 + CELLS TX_DATA +
;



: usb_reset ( - )
    \ endpoint 0
    CONTROL RX EVEN BDT_entry DUP DUP

    BDT_reset
    BDT_disable_DMA
    0_RX_EVEN BDT_buffer

    CONTROL RX ODD BDT_entry DUP DUP
    BDT_reset
    BDT_disable_DMA
    0_RX_ODD BDT_buffer

    CONTROL TX EVEN BDT_entry DUP
    BDT_reset
    BDT_remove_buffer

    CONTROL TX ODD BDT_entry DUP
    BDT_reset
    BDT_remove_buffer

    0 U1ADDR !          \ reset address
    0 USB_ADDRESS !
    DEFAULT USB_STATE !

    EVEN CONTROL tx_data_odd_even !
    EVEN CONTROL rx_data_odd_even !
    U1CON 1 REG_BIT_SET     \ reset ping pong buffer to even
    U1CON 1 REG_BIT_CLEAR
;


: usb_enable ( )
    U1CON 0 REG_BIT_SET  \ enable USB (USBEN)
;

: usb_disable ( )
    U1CON 0 REG_BIT_CLEAR  \ disable USB (USBEN)
;

\ the enpoint number for the specified BDT address
\ - BDT address
: endpoint ( addr -- n )
    BDT_START - 32 /
;

\ display the BDT entry and the buffer it points to
: debug_BDT_target ( addr - )
    DUP DUP
\      DUP HEX.
\      BDT_START - 32 /
    endpoint
    ." EP#" DUP .   ." /" 0x10 * U1EP0 + @ HEX.                   \ endpoint
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
    token_processing_address endpoint     \ BDT_START - 8 MOD
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

\ get the next odd/even sequence for the endpoint
\ - endpoint
\ > 0 or 1
: rx_odd_even ( n -- n )
    rx_data_odd_even
    DUP @ 2 MOD
    SWAP +!
;

\ endpoint
\ buffer size
\ data 0/1
: rx_control ( n n n  -  )
    ROT
    RX OVER rx_odd_even BDT_entry       \ set up device descriptor for RX data stage

\      2 PICK DUP rx_data_odd_even @ 2 MOD RX SWAP BDT_entry    \ get BD for RX data stage
\          ." == rx buffer " .hexs CR
    DUP BDT_reset
    DUP BDT_disable_DMA
    DUP ROT BDT_data01
    DUP ROT 2DUP BDT_count_expected
\    ( debug ) ." clear " .HEXS CR
    BDT_clear_buffer

    DUP
    BDT_uown
    ( debug ) SPACE SPACE ." prep " debug_BD CR

\      rx_data_odd_even +!
\      USB_EP0_RX_PACKET +!
;

: rx_control_setup ( -  )
    CONTROL 64 DATA0 rx_control
;

: rx_control_status ( -  )
\      0 1 rx_control
    CONTROL 64 DATA1 rx_control
;



\ get the next odd/even sequence for the endpoint
\ - endpoint
\ > 0 or 1
: tx_odd_even ( n -- n )
    tx_data_odd_even
    DUP @ 2 MOD
    SWAP +!
;

: tx_control_BD ( -- addr )
    CONTROL TX CONTROL tx_odd_even BDT_entry    \ set up device descriptor for TX data stage
    DUP BDT_reset
;


\ - len
\ - data0/1
\ - start of buffer content
\ - endpoint
: tx_send_packet   ( n n' addr n -- )
\      ." == send packet to ep#" DUP . SPACE .hexs CR
    \ set up device descriptor for TX data stage
    TX OVER tx_odd_even BDT_entry    \ set up device descriptor for TX data stage

    DUP BDT_reset
    TUCK SWAP BDT_buffer
    TUCK SWAP BDT_data01
    TUCK SWAP BDT_count_expected
    DUP BDT_uown
    ( debug ) SPACE SPACE ." prep b "  debug_BD CR
;


\ table offset for endpoint
: tx_send ( n -- )
    tx_data_pkt @ TX_PCKT_LEN *         \ length already sent
    SWAP tx_data_len @                    \ total length to send
    ROT       ( ep length sent )

\      ." tx send " .HEXS CR
            ( ep, length, sent )

    >= IF
\          SPACE SPACE ." packet " DUP tx_data_pkt @ . ." for EP#" . ." ,"
\          ." packet #" 0_TX_COUNT @ . ." ,"
\          ." calc " .HEXS CRS

        \ transfer length - packet size or the remaining length if smaller
        tx_data_pkt @ TX_PCKT_LEN *
        SWAP tx_data_len @
        ROT
        ( ep, length, sent )
        -                   \ remaining length to send
        ( ep remaining )
        TX_PCKT_LEN MIN
\        ( debug) ." size " DUP . CR

        SWAP tx_data_pkt @
        ( size, ep, packet# )


        \ data0 or 1
        DUP 1+ 2 MOD                \ data0 or 1
                ( size, ep, packet#, data0/1 )
        LROT SWAP
                ( size, data0/1, packet#, ep )

        \ data address
        tx_data_data @ ROT TX_PCKT_LEN * +          \ -> data addr, within buffer
                        ( size, data0/1, buffer addr, ep )

        \ transaction count for next tx
        SWAP tx_data_pkt +!
                ( length, data0/1, buffer start )
        tx_send_packet

    ELSE
        DROP  \ the endpoint
\          ." no more data " CR
    THEN

\      .HEXS CR
;


\ endpoint number
\ buffer length
\ buffer address
: tx_send_data ( n n' addr -- )
    ROT
\      CR ." Send FIRST on EP#" DUP . CR
    tx_data_pkt 0 SWAP !      \ reset count
    tx_data_data SWAP LROT !    \ set the start address in the buffer
    tx_data_len SWAP LROT !     \ set the length of the buffer

    DUP
    tx_send
\      ." Send SECOND on EP#" DUP . CR
    tx_send
;


\ endpoint number
: tx_send_next ( n  -- )
\      CR ." Send NEXT on EP#" DUP . CR
    tx_send
;

\ transmit data in the specified buffer to the control endpoint
\ - buffer size
\ - buffer address
: tx_control_data ( n addr -- addr )
    CONTROL LROT tx_send_data
;

\ transmit data in the specified buffer to the data endpoint
\ - buffer size
\ - buffer address
\  : tx_cdc_data ( n addr -- addr )
\      CDC_DATA_RX LROT tx_send_data
\  ;


: setup_cdc ( -- )
    ." setup CDC" .HEXS CR

    0x0 U1EP0 0x10 + !            \ Disbale Tx/Rx for endpoint 1
    0x0 U1EP0 0x20 + !            \ Disbale Tx/Rx for endpoint 2

    \ endpoint 1 - CDC control
    CDC_CON TX EVEN BDT_entry DUP
    BDT_reset
    1_TX_EVEN BDT_buffer

    CDC_CON TX ODD BDT_entry DUP
    BDT_reset
    1_TX_ODD BDT_buffer

    EVEN CDC_CON tx_data_odd_even !


    \ endpoint 2 - CDC data
    CDC_DATA_RX RX EVEN     BDT_entry DUP DUP
    BDT_reset
    BDT_disable_DMA
    2_RX_EVEN BDT_buffer

    CDC_DATA_RX RX ODD BDT_entry DUP DUP
    BDT_reset
    BDT_disable_DMA
    2_RX_ODD BDT_buffer

    CDC_DATA_TX TX EVEN BDT_entry DUP
    BDT_reset
    1_TX_EVEN BDT_buffer

    CDC_DATA_TX TX ODD BDT_entry DUP
    BDT_reset
    3_TX_ODD BDT_buffer

    EVEN CDC_DATA_RX rx_data_odd_even !
    EVEN CDC_DATA_TX tx_data_odd_even !


    \ enable endpoints
    0x04 U1EP0 0x10 + !       \ Enable Tx for endpoint 1
    0x08 U1EP0 0x20 + !       \ Enable Rx for endpoint 2
    0x04 U1EP0 0x30 + !       \ Enable Tx for endpoint 3

    3_TX_EVEN DUP $1234567 SWAP ! $8900 SWAP CELL+ !  \ example data
    3_TX_ODD $000A4B4F SWAP !

\      CDC_DATA_TX 8 3_TX_EVEN tx_send_data

\      CDC_DATA_RX 64 DATA0 rx_control
\      CDC_DATA_RX 64 DATA0 rx_control

\      CDC_CON 8 3_TX_EVEN tx_send_data
    CDC_DATA_RX 64 DATA0 rx_control

    ." setup done " .HEXS CR
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

        \ check request is GET_DESCRIPTOR for DEVICE
        OVER @ 0x01000680 = IF
            ." GET_DESCRIPTOR: DEVICE" CR
\      .HEXS CR
            rx_control_status
            rx_control_setup
\      .HEXS CR
\              18 DEV_DESC tx_control_data
                CONTROL 18 DEV_DESC tx_send_data

            ." descriptor sent " .hexs CR
            enable_packet_processing
        THEN

        \ check request is device standard SET_ADDRESS
        OVER @ 0xffff AND 0x0500 = IF
            SPACE SPACE ." SET_ADDRESS "

            OVER @  16 RSHIFT  USB_ADDRESS !
            USB_ADDRESS ? CR

\            enable_packet_processing

\              ." pause... " 2000 ms ." continue " CR
            0 DEV_DESC tx_control_data      \ prepare for ZLP response

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

            rx_control_status
            18 DEV_DESC tx_control_data
            rx_control_setup

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

            0 DEV_DESC tx_control_data      \ prepare for ZLP response
            rx_control_status
            rx_control_setup                \ for next command

            CONFIGURED USB_STATE !

            enable_packet_processing
        THEN


        \ check request is DEVICE QUALIFIER
        OVER @ 0x06000680 = IF
            ." DEV_QUALIFIER " CR
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
    THEN

    \ OUT packet
    DUP 1 = IF                              \ host to device (OUT)
        SPACE SPACE ." > OUT" CR
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

        \ check request is SET_LINE_CODING for CDC
        OVER @ 0x00002021 = IF
            ." SET_LINE_CODING: DEV" CR
            CONTROL ZLP tx_send_data
            rx_control_status
            rx_control_setup
            enable_packet_processing
        THEN


        \ check request is SET_LINE_CONTROL_STATE for CDC
        OVER @ 0x0ffff AND 0x2221 = IF
            ." SET_LINE_CONTROL_STATE: DEV" CR
            CONTROL ZLP tx_send_data
            rx_control_status
            rx_control_setup
            enable_packet_processing
        THEN
    THEN


    \ IN packet
    DUP 9 = IF
        SPACE SPACE ." < IN" CR                         \ device to host (IN) transaction
        0 tx_send_next
    THEN

    \ OUT packet
    DUP 1 = IF                              \ host to device (OUT)
        SPACE SPACE ." > OUT" CR
        2 PICK endpoint 2 = IF               \ for endpoint 2 - RX
            OVER 6 BD_READ_BYTE    ." READ " DUP . CR      \ read size
            ." => "
                    DO DUP i + C@ HEX. LOOP
                    CR
                    0
            \ TODO copy data to TX buffer for echo
\              OVER 6 BD_READ_BYTE
\              OVER SWAP
\              3_TX_ODD SWAP MOVE
            CDC_DATA_RX 64 DATA0 rx_control
            CDC_DATA_TX 3 3_TX_ODD tx_send_data
        THEN

    THEN

    DROP    \ PID
    DROP    \ buffer
    DROP    \ descriptor
;


: debug_IR ( -- )
    ." {IR="  U1IR @ hex. ." } "
;

: handle_token ( - )
\      ." token  " debug_IR  ." STAT=" U1STAT @ HEX. CR

    processing_descriptor           ( descriptor, buffer, PID )

    DUP 0 = IF
        ." <++ NO PID " debug_IR

        DROP    \ PID
        DROP    \ buffer
        DROP    \ descriptor

        0x08 U1IR !        \ clear interrupt
        exit
    THEN

    \ IN packet
    \ device to host (IN) transaction
    DUP 9 = IF
        ." <++ IN " debug_IR

        DROP    \ PID
        DROP    \ buffer
        DROP    \ descriptor

        CR     SPACE SPACE token_processing_address debug_BD

        0 tx_send_next
        0x08 U1IR !        \ clear interrupt
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
        ." CONFIGURED STATE" CR SPACE SPACE debug_recvd CR
        process_configured_token
    THEN

    USB_STATE @ ADDRESSED = IF
        ." /ADDRESSED STATE" CR SPACE SPACE debug_recvd CR
        process_addressed_token
   THEN

    USB_STATE @ DEFAULT = IF
        ." /DEFAULT STATE" CR SPACE SPACE debug_recvd CR
        process_default_token
    THEN

    0x08 U1IR !        \ clear interrupt
;


: handle_reset ( -- )
    ." <++ USB reset " debug_IR CR
    debug_state

    0 U1ADDR !          \ reset address
    0 USB_ADDRESS !
    DEFAULT USB_STATE !

    EVEN CONTROL tx_data_odd_even !
    EVEN CONTROL rx_data_odd_even !
    U1CON 1 REG_BIT_SET     \ reset ping pong buffer to even
    U1CON 1 REG_BIT_CLEAR
    enable_packet_processing


    rx_control_setup
    0x01 U1IR !        \ clear interrupt

\      debug_IR
\      ." ------"
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
            ." <++ DETACHED STATE" CR ." ++>" CR
            DETACHED USB_STATE !
            0x01 U1OTGIR !      \ reset interrpt
        THEN


		U1IR @

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
    setup_cdc

    rx_control_setup

    0x0D U1EP0 !            \ Enable Tx/Rx for endpoint 0
;

task usb_t

: run  ( )  usb_t activate handle_usb ;

.( Running )
CR
\ start USB
usb_init
\  debug_usb
\  1 DEBUGGING !
run
\ usb_enable



echo
