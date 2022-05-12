\ Temperature logger

: read-temp ( -- temp ) 
		ADC_SAMPLE				\ take 5 samples over a quarter of a second
		50 ms ADC_SAMPLE +
		50 ms ADC_SAMPLE +
		50 ms ADC_SAMPLE +
		50 ms ADC_SAMPLE +
	
		5 /						\ calculate the average
		
		." Avg " .S
		
		3000 *					\ base voltage reading on 3V
		1023 /					\ divide by full 10 bit range

		." Voltage " .S CR
		
		500 -					\ adjust to 0 degrees at 0.5V
;

: log-temp ( ) 
	ADC_INIT
  	6 ADC_SELECT				\ sensor on AN6
  	
\	BEGIN
		read-temp		
		." Temp " .S CR			\ Temp in 0.1 degrees
\	AGAIN
	

;
