noecho

\ double constants PWR_LED and ACT_LED must be declared before running this

2variable flash_rate
900 50 flash_rate 2!

pwr_led DIGITAL_OUT				\ enable LEDs
act_led DIGITAL_OUT

: pwr_flash ( - )
	50 300 pwr_led led_flash
;

' pwr_flash power initiate		\ run flasher on power task

: active_flash ( - )
	flash_rate act_led var_led_flash
;

task test_flashing				\ run flasher on another task
' active_flash test_flashing initiate

\ list new tasks with:
\ tasks

.( Added power flash)
echo
