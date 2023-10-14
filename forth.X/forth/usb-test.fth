\ UART
\ noecho

HEX

0bf806210 CONSTANT USB_STA

0bf885040 CONSTANT U1OTGIR
0bf885060 CONSTANT U1OTGSTAT
0bf885070 CONSTANT U1OTGCON

0bf885080 CONSTANT U1PWRC
0bf885200 CONSTANT U1IR
0bf885240 CONSTANT U1STAT
0bf885250 CONSTANT U1CON

:  test_usb ( -  )
	begin
		USB_STA @
		." USB status " . CR
		2000 ms
	again
;

: debug_usb ( ) 
    ." USB"
    CR
    ."  OTG IR " U1OTGIR @ HEX. CR
    ."  OTG STAT " U1OTGSTAT @ HEX. CR
    ."  OTG CON " U1OTGCON @ HEX. CR

    ."  USB IR " U1IR @ HEX. CR
    ."  USB STAT " U1STAT @ HEX. CR
    ."  USB CON " U1CON @ HEX. CR
;

: clear_usr_ir ( )
    0xff U1OTGIR !          \ clear all interrupt flags
;


1 0 U1PWRC REG_BIT!     \ turn on USB module (0)

0x80 U1OTGCON !         \ Full speed - pull up D+ (7)
                        \ VBUS not powered (3)
                        \ OTGEN off (2)
                        \ No VBUS charge/discharge (1-0)
debug_usb


\ task usb
\ ' test_usb usb initiate
