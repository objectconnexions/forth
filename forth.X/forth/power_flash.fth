noecho


\ PORTB 1 DIGITAL_OUT
\ PORTB 1 port_on
\ 250 ms
\ PORTB 1 port_off

2variable flash_rate
450 50 flash_rate 2!

2variable active_led
2variable power_led

\ PORTE 1 power_led 2!		\ for MX270 board
\ PORTE 1 active_led 2!		\ for MX270 board

 PORTB 4 power_led 2!		\ for MX130 breadboard
 PORTA 4 active_led 2!		\ for MX130 breadboard

: pwr_flash ( - )
	450 50 power_led 2@ led_flash
;
\ run flasher on power task
' pwr_flash power initiate


: active_flash ( - )
	150 50 active_led 2@ led_flash
;
\ run flasher on another task
\ task test_flashing
\ ' active_flash test_flashing initiate

.( added power flash)
echo
