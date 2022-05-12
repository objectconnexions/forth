
: button_task

	begin
		DIO_1 CN_STATUS
		IF
			DIO_1 REG_BIT_READ
			." CHANGED to " . CR	
			DIO_1 CN_CLEAR
		THEN
		300 ms
	again
;

task check_button
' button_task check_button initiate
