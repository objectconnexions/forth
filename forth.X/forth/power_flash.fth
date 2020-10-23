\  noecho

\ double constants PWR_LED and ACT_LED must be declared before running this

2variable flash_rate
450 50 flash_rate 2!

\ 2variable act_led
\ 2variable pwr_led

\ PORTE 1 pwr_led 2!		\ for MX270 board
\ PORTE 1 act_led 2!		\ for MX270 board

\ PORTB 14 pwr_led 2!			\ for MX130 breadboard
\ PORTB 15 act_led 2!			\ for MX130 breadboard

pwr_led DIGITAL_OUT
act_led DIGITAL_OUT

: pwr_flash ( - )
	450 50 pwr_led led_flash
;
\ run flasher on power task
' pwr_flash power initiate


: active_flash ( - )
	150 50 act_led led_flash
;
\ run flasher on another task
\ task test_flashing
\ ' active_flash test_flashing initiate

.( added power flash)
echo
