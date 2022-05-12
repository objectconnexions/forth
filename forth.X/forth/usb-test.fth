\ UART
\ noecho

HEX

0bf806210 CONSTANT USB_STA

:  test_usb ( -  )
	begin
		USB_STA @
		." USB status " . CR
		2000 ms
	again
;

task usb
' test_usb usb initiate
