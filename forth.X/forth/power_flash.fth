noecho


\ PORTB 1 DIGITAL_OUT
\ PORTB 1 port_on
\ 250 ms
\ PORTB 1 port_off

2variable flash_rate
450 50 flash_rate 2!

2variable act_led
2variable pwr_led

\ PORTE 1 pwr_led 2!		\ for MX270 board
\ PORTE 1 act_led 2!		\ for MX270 board

 PORTB 4 pwr_led 2!			\ for MX130 breadboard
 PORTA 4 act_led 2!			\ for MX130 breadboard

: pwr_flash ( - )
	450 50 pwr_led 2@ led_flash
;
\ run flasher on power task
' pwr_flash power initiate


: active_flash ( - )
	150 50 act_led 2@ led_flash
;
\ run flasher on another task
\ task test_flashing
\ ' active_flash test_flashing initiate

.( added power flash)
echo
