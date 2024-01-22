noecho

\ double constants PWR_LED and ACT_LED must be declared before running this

2variable flash_rate
task+ power
task+ flasher

: pwr_flash ( - )
    pwr_led DIGITAL_OUT				\ enable LEDs
    power activate 300 120 pwr_led led_flash
;

: active_flash ( - )
    act_led DIGITAL_OUT
    900 50 flash_rate 2!
	flasher activate flash_rate act_led var_led_flash
;

\ list new tasks with:
\   tasks

\ start with
: main ( )
    pwr_flash
    active_flash
;

.( Added power flash) CR
echo
