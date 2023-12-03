' pwr_flash power initiate		\ run flasher on power task

task test_flashing				\ run flasher on another task
900 50 flash_rate 2!
act_led DIGITAL_OUT
' active_flash test_flashing initiate

