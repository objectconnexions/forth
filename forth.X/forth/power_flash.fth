\ noecho

\ double constants PWR_LED and ACT_LED must be declared before running this

2variable flash_rate

task test_flashing				\ run flasher on another task

: pwr_flash ( - )
	300 120 pwr_led led_flash
;

: start_power
    pwr_led DIGITAL_OUT				\ enable LEDs
    ' pwr_flash power initiate		\ run flasher on power task
;

: active_flash ( - )
	flash_rate act_led var_led_flash
;

: start_active
    900 50 flash_rate 2!
    act_led DIGITAL_OUT

    ' active_flash test_flashing initiate
;

\ list new tasks with:
\ tasks

start_power_flash

.( Added power flash) CR
echo
