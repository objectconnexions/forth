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

DECIMAL


\ buffer descriptor table
CREATE BDT
    1024
    ALLOT

\ calculate the aligned address
0xffffff00 BDT AND 0x100 +
    CONSTANT BDT_START


CREATE 0_RX_EVEN ALIGN 64 ALLOT

CREATE 0_RX_ODD ALIGN 64 ALLOT



CREATE DEV_DESC ALIGN 18 ALLOT

\ Device descriptor
0x12 DEV_DESC C!           \ length 18 bytes
0x01 DEV_DESC 1+ C!        \ type, device descriptor
0x00 DEV_DESC 2+ C!        \ USB release number (2.10)
0x02 DEV_DESC 3 + C!       \ USB release number
0x00 DEV_DESC 4 + C!       \ class
0x00 DEV_DESC 5 + C!       \ subclass
0x00 DEV_DESC 6 + C!       \ protocol
0x08 DEV_DESC 7 + C!       \ max packet size
0xD8 DEV_DESC 8 + C!       \ vendor id
0x04 DEV_DESC 9 + C!       \ vendor id
0x0A DEV_DESC 10 + C!      \ product id
0x00 DEV_DESC 11 + C!      \ product id
0x51 DEV_DESC 12 + C!      \ release number (3.51)
0x03 DEV_DESC 13 + C!      \ release number
0x00 DEV_DESC 14 + C!       \ manufacturer index
0x00 DEV_DESC 15 + C!       \ product index
0x00 DEV_DESC 16 + C!       \ serial number index
0x01 DEV_DESC 17 + C!       \ number of configurations


CREATE CONF_DESC ALIGN 64 ALLOT

0x09 CONF_DESC 0 + C!   \ length
0x02 CONF_DESC 1 + C!   \ CONFIGURATION
25 CONF_DESC 2 + C!   \ total length
0 CONF_DESC 3 + C!     \
1 CONF_DESC 4 + C!     \ number of intefaces
1 CONF_DESC 5 + C!
0 CONF_DESC 6 + C!        \ index to configuration name
0x80 CONF_DESC 7 + C!         \ attribute - bus powered
100 CONF_DESC 8 + C!      \ max power - 200mA


0x09 CONF_DESC 9 + C!       \ length
0x04 CONF_DESC 10 + C!      \ INTERFACE
0x00 CONF_DESC 11 + C!      \ interface number
0x00 CONF_DESC 12 + C!       \ alternate setting
0x01 CONF_DESC 13 + C!     \ number of endpoints
0xff CONF_DESC 14 + C!       \ class
0x00 CONF_DESC 15 + C!       \ subclass
0x00 CONF_DESC 16 + C!      \ protocol
0x00 CONF_DESC 17 + C!      \ name index


0x07 CONF_DESC 18 + C!       \ length
0x05 CONF_DESC 19 + C!      \ ENDPOINT
0x81 CONF_DESC 11 + C!      \ endpoint address
0x03 CONF_DESC 12 + C!       \ interrupt
0x08 CONF_DESC 13 + C!     \ max size
0x0 CONF_DESC 14 + C!       \
0x0A CONF_DESC 15 + C!       \ 10 frames



CREATE INT_DESC ALIGN 64 ALLOT




\ Interface descriptor
0x09 INT_DESC C!           \ length 18 bytes
0x02 INT_DESC 1+ C!        \ type
0x00 INT_DESC 2+ C!        \ interface number
0x00 INT_DESC 3 + C!       \ alternate setting
0x00 INT_DESC 4 + C!       \ number of endpoints
0x03 INT_DESC 5 + C!       \ class
0x01 INT_DESC 6 + C!       \ subclass
0x02 INT_DESC 7 + C!       \ protocol
0x00 INT_DESC 8 + C!       \ name index

VARIABLE USB_STATE
VARIABLE USB_STATUS     \ address set?
VARIABLE USB_ADDRESS
VARIABLE USB_EP0_TX_PACKET
VARIABLE USB_EP0_RX_PACKET

VARIABLE DEBUGGING



\ 1's complement
: invert ( n - n ) NEGATE 1 - ;


\ source
\ value
\ pos
\ length
: set_bits ( n n n n - n )
    OVER MASK_PATTERN INVERT    \ mask
    LROT LSHIFT                 \ new value
    LROT AND                      \ mask the source VALUE
    OR                           \ combine
;

\ source
\ pos
\ length
: get_bits ( n n n - n )
    OVER MASK_PATTERN            \ mask
    ROT AND                      \ mask the source VALUE
    SWAP RSHIFT                 \ value
;

: +! ( addr - )
    DUP @ 1+ SWAP !
;


: change_order ( n - n )
    DUP ." original " hex. CR
    DUP 24 RSHIFT 0xff AND SWAP
    DUP 16 RSHIFT 0xff AND 8 LSHIFT SWAP
    DUP 8 RSHIFT 0xff AND 16 LSHIFT SWAP
    0xff AND 24 LSHIFT OR OR OR

    DUP ." endian " hex. CR
;


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

: USB_target ( addr - )
    DUP DUP
    5 RSHIFT 0xf AND  .          \ endpoint
    0x10 AND IF ." TX" ELSE ." RX" THEN
    SPACE
    0x08 AND IF ." ODD" ELSE ." EVEN" THEN
;

: BDT_status ( addr  - )
    ." @" DUP hex.
    DUP @ DUP DUP DUP DUP
    16 RSHIFT 0xfff AND .  ." bytes: "             \ byte count
    0x80 AND IF ." HW" ELSE ." SW" THEN SPACE
    0x40 AND IF ." DATA1" ELSE ." DATA0" THEN SPACE
    2 RSHIFT 0x0f AND  hex.            \ bit pattern of PID

    2 RSHIFT 0xfff AND  bin.            \ bit pattern of flags

    4 + ." -> " @ TO_VIRTUAL_ADDR hex.
;

\ descriptor address
: BDT_reset ( addr - )
\      DUP @
\      0 16 9 set_bits               \ 0 COUNT
\      0 2 6 set_bits SWAP !        \ disable all flags
    0 SWAP !
;

: BDT_remove_buffer ( addr - )
      CELL+ 0 SWAP !
;


: BDT_clear_buffer ( addr n - )
    SWAP CELL+ SWAP

        2DUP HEX .S DECIMAL CR
    ERASE
\      2DROP

\        4 + SWAP .S ERASE
\      0 2DUP 2! 2!
;


\ descriptor address
\ buffer addr
: BDT_buffer ( addr addr - )
\      OVER 0 SWAP !                    \ clear first word
    TO_PHYSICAL_ADDR SWAP
    4 + !                             \ address to second word in buffer descriptor
;

\ descriptor address
: BDT_uown ( addr - )
\      DUP DUP

    DUP @
    1 7 1 set_bits
    SWAP  !

\      ." : " USB_target
\      ." - " BDT_status CR
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
: BDT_count ( addr n - )
    OVER @

    SWAP 16 9 set_bits
    SWAP !
;

\ descriptor address
\ count
: BDT_stall ( addr - )
    OVER @
    1 2 1 set_bits
    SWAP !
;

\ descriptor address
\ count
: BDT_unstall ( addr - )
    OVER @
    0 2 1 set_bits
    SWAP !
;

: BDT_PID ( addr - n )
    @ 2 4 get_bits
;

\ buffer size
\ data 0/1
: rx_control ( n n  -  )
HEX
    ." rx prep " .S CR
    0 0 USB_EP0_RX_PACKET @ 2 MOD BDT_entry    \ set up receive for RX data stage
    DUP BDT_reset
    DUP ROT BDT_data
    ." start " .S CR
    DUP ROT 2DUP BDT_count
    .S CR
    SWAP CELL+ @ SWAP BDT_clear_buffer
\    DUP BDT_DTS
    BDT_uown

    USB_EP0_RX_PACKET +!
DECIMAL
;

: rx_control_setup ( -  )
    64 0 rx_control
;

: rx_control_status ( -  )
\      0 1 rx_control
    64 1 rx_control
;

: reset_usb ( - )
    \ endpoint 0
    0 0 0 BDT_entry DUP
    BDT_reset
    0_RX_EVEN BDT_buffer
    0_RX_EVEN 64 BDT_clear_buffer

    0 0 1 BDT_entry DUP
    BDT_reset
    0_RX_ODD BDT_buffer
    0_RX_ODD 64 BDT_clear_buffer

    0 1 0 BDT_entry DUP
    BDT_reset
    BDT_remove_buffer

    0 1 1 BDT_entry DUP
    BDT_reset
    BDT_remove_buffer

    0 U1ADDR !          \ reset address
    0 USB_ADDRESS !
    0 USB_STATUS !

    0 USB_EP0_TX_PACKET !
    0 USB_EP0_RX_PACKET !
    U1CON 1 REG_BIT_SET

    enable_packet_processing
;


: enable_usb ( )
    U1CON 0 REG_BIT_SET  \ enable USB (USBEN)
;

: disable_usb ( )
    U1CON 0 REG_BIT_CLEAR  \ disable USB (USBEN)
;

: debug_usb ( )
    ." USB"  CR
    ."  OTG IR " U1OTGIR @ HEX. CR
    ."  OTG STAT " U1OTGSTAT @ HEX. CR
    ."  OTG CON " U1OTGCON @ HEX. CR

    ."  USB IR " U1IR @ HEX. CR
    ."  USB EIR " U1EIR @ HEX. CR
    ."  USB STAT " U1STAT @ HEX. CR
    ."  USB CON " U1CON @ HEX. CR
    ."  USB BDT " U1BDTP3 @ HEX. SPACE U1BDTP2 @ HEX. SPACE U1BDTP1 @ HEX. CR
    ."  EP0 " U1EP0 @ HEX.
;


: debug_BD_data ( addr - )
    4 + @ TO_VIRTUAL_ADDR                       \ get buffer adderss for data dump
    DUP ." @" HEX.
    ." ["
    DUP 2@ HEX. HEX. 2@ HEX. HEX.
    ." ]"
;


: debug_BD ( addr  - )
\      ." @" DUP hex.
    DUP @ DUP DUP DUP
    16 RSHIFT 0xfff AND .  ." bytes: "             \ byte count
    0x80 AND IF ." MOD" ELSE ." PRG" THEN SPACE
    0x40 AND IF ." DATA1" ELSE ." DATA0" THEN SPACE
    2 RSHIFT 0xfff AND  bin.                        \ bit pattern of flags

\      4 + ." -> " @ TO_VIRTUAL_ADDR
     ." -> " debug_BD_data
;


: debug_endpoints ( n - )
    DUP DUP ." |  #" . ." RX EVEN "   0 0 BDT_entry debug_BD CR
    DUP DUP ." |  #" . ." RX ODD "   0 1 BDT_entry debug_BD CR
    DUP DUP ." |  #" . ." TX EVEN "   1 0 BDT_entry debug_BD CR
    DUP ." |  #" . ." TX ODD "   1 1 BDT_entry debug_BD CR
;


: debug_recvd ( )
    \ debug details
    U1STAT @  1 LSHIFT

\      ." request "
    ." @" U1ADDR ?
    DUP USB_target

    BDT_START +                                \ buffer descriptor entry address
\      DUP BDT_status

    SPACE debug_BD
\      ." ; data " debug_BD_data
;


: debug_state ( - )
    DEBUGGING @ IF
        ." |   @" U1ADDR ?

        ."  : IR " U1IR @ HEX.
        ."  ; EIR " U1EIR @ HEX.
        ."  ; STAT " U1STAT @ HEX. CR

        \ debug details

        0 debug_endpoints

\          CR
    THEN
;

: status_usb ( )
    ." USB status" CR
    ."  Power " U1PWRC 0 REG_BIT? CR
    ."  State " USB_STATUS ? CR
    ."  Address " U1ADDR ? CR
\      ."  Descriptors " BDT_START HEX. CR
\      ."  Packet # RX/TX " USB_EP0_RX_PACKET ? USB_EP0_TX_PACKET ? CR
    0 debug_endpoints
    CR
;

: clear_usr_ir ( )
    0xff U1OTGIR !          \ clear all interrupt flags
    0xff U1IR !          \ clear all interrupt flags
    0xff U1EIR !          \ clear all interrupt flags
;


\ buffer size
\ buffer address
: tx_control_data ( n addr )
\      ." tx prep "
    0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry DUP             \ set up device descriptor for TX data stage
    BDT_reset
    DUP ROT BDT_buffer
    DUP ROT BDT_count
    DUP 1 BDT_data
\    DUP BDT_DTS
    DUP BDT_uown

    USB_EP0_TX_PACKET +!
;

: setup_token ( addr -  )
    DUP @ 0xffff AND 0x0680 = IF              \ check request is GET_DESCRIPTOR
        ." GET_DESCRIPTOR: "

        DUP @ 24 RSHIFT

        DUP 0x01 = IF                            \ device descriptor
            ." DEVICE" CR

            USB_STATE 0= IF
                1 USB_STATE !           \  attached
            THEN

            18 DEV_DESC tx_control_data DROP
\              64 DEV_DESC tx_control_data
            rx_control_status
            rx_control_setup
\              CR status_usb
            DROP 0
        THEN

        DUP 0x02 = IF                               \ configuration descriptor
            ." CONFIGURATION" CR

\              OVER 4 + @
\              16 RSHIFT 0xffff AND
\              DUP ." size " hex. CR

            9 CONF_DESC tx_control_data DROP

\              0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry DUP DUP DUP DUP   \ set up configuration descriptor TX for data stage
\              BDT_reset
\              CONF_DESC BDT_buffer
\              1 BDT_data
\  \            ROT .S BDT_count
\                9 BDT_count
\              BDT_uown

            0 0 USB_EP0_RX_PACKET @ 2 MOD BDT_entry DUP DUP DUP DUP    \ set up device descriptor RX for data stage
            BDT_reset
            0 BDT_count
            1 BDT_data
            BDT_DTS
            BDT_uown


            OVER 4 + @
            16 RSHIFT 0xffff AND
            DUP ." size " hex. CR

            0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry
            SWAP BDT_count

\              USB_EP0_TX_PACKET +!
            USB_EP0_RX_PACKET +!

            DROP 0
        THEN


        DUP 0x00 <> IF
            \ for all others, a zero length packet
            ." OTHER " DUP . OVER @ 24 RSHIFT . CR
            ." error request " OVER 2@ hex. hex. CR
\              0

            0 DEV_DESC tx_control_data DROP

\              0 1 USB_EP0_TX_PACKET @ 2 MOD BDT_entry DUP DUP DUP DUP    \ set up device descriptor
\              BDT_reset
\              DEV_DESC BDT_buffer
\              0 BDT_count
\              1 BDT_data
\              BDT_uown
\
\              USB_EP0_TX_PACKET +!

        THEN

        DROP                    \ descriptor no
    THEN


    DUP @ 0xffff AND 0x0500 = IF             \ check request is device standard SET_ADDRESS
        ." SET_ADDRESS "

        reset_usb

        1 USB_STATUS !
        DUP @ 16 RSHIFT  USB_ADDRESS !
        USB_ADDRESS ? CR

\          CR status_usb

\          rx_control_setup
        rx_control_setup
        0 DEV_DESC tx_control_data           \ prepare for ZLP response
        DROP

        ticks
\          0x08 U1IR !        \ clear interrupt

        enable_packet_processing

        USB_ADDRESS @ 0x7F AND
        U1ADDR !
        2 USB_STATUS !

        ticks swap - CR ." set " U1ADDR ? ." in ms " . CR

\          CR status_usb




\          ." >> " OVER HEx. CR
\          U1IR @ hex. U1STAT 1 LSHIFT @ hex. CR
\
\  \          DUP 64 DUMP
\
\
\
\  \          1000 ms
\
\          rx_control_setup
\
\          0 DEV_DESC tx_control_data           \ prepare for ZLP response
\  \             DUP BDT_stall
\  \
\  \            3 ms
\  \
\  \            BDT_unstall
\  \          DROP
\          ." => " hex. CR
\
\          ticks
\
\          0x08 U1IR !        \ clear interrupt
\          U1IR @ hex. U1STAT 1 LSHIFT @ hex. CR
\
\  \          40 ms
\  \          USB_ADDRESS @ 0x7F AND
\  \          U1ADDR !
\
\  \          rx_control_setup
\
\
\          \          BEGIN
\  \              U1IR @ 0x08 AND
\  \              0x08 =
\  \          UNTIL
\          enable_packet_processing
\  \          3 ms
\
\          U1IR @ hex. U1STAT 1 LSHIFT @ hex. CR
\
\          USB_ADDRESS @ 0x7F AND
\          U1ADDR !
\
\          ticks swap - CR ." set " U1ADDR ? ." in ms " . CR
\
\          2 USB_STATUS !
\
\  \          rx_control_setup
\          CR
\
\          0x08 U1IR !        \ clear interrupt
\          U1IR @ hex. U1STAT 1 LSHIFT @ hex. CR
\

    THEN


    DUP @ 0xffff AND 0x0900 = IF             \ check request is device standard GET_INTERFACE
        ." SET_CONFIGURATION "
        DUP @ . CR

        0 DEV_DESC tx_control_data DROP


        0 0 USB_EP0_RX_PACKET @ 2 MOD BDT_entry DUP DUP DUP DUP    \ set up RX for next set up stage
        BDT_reset
        0 BDT_count
        1 BDT_data
        BDT_DTS
        BDT_uown


\          USB_EP0_TX_PACKET +!
        USB_EP0_RX_PACKET +!

    THEN


    DUP @ 0xffff AND 0x0A81 = IF             \ check request is device standard GET_INTERFACE
        ." GET_INTERFACE " DUP hex.

        \ not expected yet

        64 DEV_DESC tx_control_data DROP
    THEN

    DROP                    \ address


;

: token_packet ( - )
    ." +> TOKEN "
    debug_recvd
    CR

\      .S CR

\      \ debug details
\      ." recieved " U1ADDR ? U1STAT @  1 LSHIFT  USB_target
\
\      U1STAT @ 1 LSHIFT BDT_START + DUP              \ buffer descriptor entry address
\      DUP BDT_status
\
\      4 + @ TO_VIRTUAL_ADDR                       \ get buffer adderss
\      ." ; data " DUP 2@ HEX. HEX. CR


        U1STAT @ 1 LSHIFT BDT_START +            \ buffer descriptor entry address
        DUP
        4 + @ TO_VIRTUAL_ADDR                       \ get buffer adderss

        OVER BDT_PID
\      OVER BDT_PID

    \ Reserved packet
    DUP 0 = IF
        ." Reserved - " OVER BDT_status CR
\          OVER 32 DUMP
\          status_usb

    THEN


    \ SETUP packet
    DUP 13 = IF
        ." > SETUP - "

        OVER setup_token

        DROP 0                                     \ drop & restore PID


        enable_packet_processing
\          U1CON 5 REG_BIT_CLEAR                   \ clear PKTDIS - packet transfer disable
    THEN


    \ IN packet
    DUP 9 = IF
        ." < IN" CR                                   \ device to host (IN) transaction

\          0 1 tx_control_data DROP                         \ status transaction
\        rx_control_status

        DROP 0                                      \ drop & restore PID
    THEN


    \ OUT packet
    DUP 1 = IF
        ." > OUT" CR

        DROP                                       \ drop PID
\          OVER BDT_reset
\          OVER 64 BDT_count
\          OVER BDT_uown                                 \ reset the incoming buffer
\  \        OVER 1 BDT_data

            rx_control_status
\          \ afer status, prepare for new setup stage
\          0 0 USB_EP0_RX_PACKET @ 2 MOD BDT_entry DUP DUP DUP        \ set up receive for RX setup stage
\          BDT_reset
\          0 BDT_count
\          BDT_DTS
\          BDT_uown
\
\
\          USB_EP0_RX_PACKET +!

        0                                           \ restore PID

        DROP 0
    THEN

    \ unrecognised packet
    DUP 0 <> IF
           ." > unknown request, PID " DUP . ." :" CR
            U1STAT @ 1 LSHIFT DUP hex. CR
            BDT_START + 4 + @ TO_VIRTUAL_ADDR 64 DUMP
    THEN
    DROP


    DROP
    DROP

    U1EIR @ 0> IF
        ." error raised " U1EIR @ hex. CR
         0xff U1EIR !
    THEN

    debug_state
    ." >> TOKEN processed" CR CR

    0x08 U1IR !        \ clear interrupt
;


:  handle_usb ( -  )
    0xff U1IR !        \ clear interrupt
    CR ." USB ready" CR
    begin
        U1EIR @ 0> IF
            ." error " U1EIR @ hex. CR
            0xff U1EIR !
        THEN

		U1IR @


\          U1CON 5 REG_BIT@ IF
\              ." PKT_DIS set" CR
\          THEN

		DUP
        0x01 AND IF
            ." +> USB reset ("  U1IR @ hex. ." )" CR
\              ." address " U1ADDR ? CR
\              status_usb
            reset_usb

            rx_control_setup

            0x01 U1IR !        \ clear interrupt

            DROP
            0

\              ." >> USB reset complete" CR
            debug_state
            CR
        THEN

        DUP
        0x08 AND IF
\              USB_STATUS @ 1 = IF
\                  USB_ADDRESS @ U1ADDR !
\                  2 USB_STATUS !
\                  ." address set: " U1ADDR ? CR
\
\
\
\                  ." process transaction " U1ADDR ? U1STAT @
\                          1 LSHIFT  USB_target
\                  U1STAT @ 1 LSHIFT BDT_START +              \ buffer descriptor entry address
\                  DUP BDT_status
\                  DUP 4 + @ TO_VIRTUAL_ADDR                       \ get buffer adderss
\                  ." ; data " 2@ HEX. HEX. CR
\                  BDT_PID
\                  ." PID " . CR
\
\
\
\                  rx_control_setup
\
\  \                  enable_packet_processing                   \ clear PKTDIS - packet transfer disable
\
\              ELSE
\                  ." +> TOKEN READY" CR
                token_packet
\              THEN
        THEN


        DUP
        0x04 AND IF
\              U1IR @ hex. CR
\              ." SOF received" CR
            0x04 U1IR !        \ clear interrupt
        THEN

        DUP
        0x20 AND IF
\              U1IR @ hex. CR
            ." +> RESUME detected" CR
            0x20 U1IR !        \ clear interrupt
        THEN

\          DUP
        0x10 AND IF
\              U1IR @ hex. CR
            ." +> IDLE condition" CR
            0x10 U1IR !        \ clear interrupt
        THEN

\          0x08 AND NOT IF
\              1 ms
\          THEN

        100 ms
\		U1IR @ hex. SPACE
	again
;



: init_usb ( - )
    U1CON 0 REG_BIT_CLEAR       \ disable USB (USBEN)
    U1PWRC 0 REG_BIT_CLEAR     \ turn off USB module (0)
    100 ms


    U1PWRC 0 REG_BIT_SET     \ turn on USB module (0)


\      0x80 U1OTGCON !         \ Full speed - pull up D+ (7)
    0x0 U1OTGCON !         \ Full speed - pull up D+ (7)
                            \ VBUS not powered (3)
                            \ OTGEN pull up/down controlled  by software ???? (2)
                            \ No VBUS charge/discharge (1-0)


    \ set up BDT address registers
    BDT_START
    TO_PHYSICAL_ADDR DUP DUP
    24 RSHIFT 0xff AND U1BDTP3 !
    16 RSHIFT 0xff AND U1BDTP2 !
    8 RSHIFT 0xff AND U1BDTP1 !

    0x0 U1EP0 !            \ Disbale Tx/Rx for endpoint 0

    reset_usb

    0 USB_STATE !           \  initial

    0x0D U1EP0 !            \ Enable Tx/Rx for endpoint 0

    status_usb

    rx_control_setup

\      \ endpoint 0
\      0 0 0 BDT_entry DUP DUP DUP
\      BDT_reset
\      0_RX_EVEN BDT_buffer
\      64 BDT_count
\      BDT_uown
\
\      0 0 1 BDT_entry DUP DUP DUP
\      BDT_reset
\      0_RX_ODD  BDT_buffer
\      64 BDT_count
\      DUP     1 BDT_data
\      BDT_uown
\
\      0 1 0 BDT_entry
\      BDT_reset
\
\      0 1 1 BDT_entry
\      BDT_reset
\
\      0x0D U1EP0 !            \ Enable Tx/Rx for endpoint 0

\      0x80 U1OTGCON !         \ Full speed - pull up D+ (7)
                            \ VBUS not powered (3)
                            \ OTGEN pull up/down controlled  by software ???? (2)
                            \ No VBUS charge/discharge (1-0)

\      0xff U1IR !       \ clear all interrupts
\
\      0 USB_STATUS !

;




\ debug_usb


task usb_t
: run  ( )
    usb_t activate handle_usb
;


init_usb
status_usb
1 DEBUGGING !
run

echo
