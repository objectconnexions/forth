HEX

0bf809000 CONSTANT ADC
0 CONSTANT AD1CON1
10 CONSTANT AD1CON2
20 CONSTANT AD1CON3
40 CONSTANT AD1CHS
70 CONSTANT ADC1BUF0

: ADC_INIT ( -- )
	ADC 0f BIT_CLR		\ prepare to set up ADC - turn off 

	0
	4 8 SET_BITS		\ at bit 8, data format: 32 bit integer
	7 5	SET_BITS		\ at bit 5, conversion trigger source: auto
						\ bit 2, manual sampling
	ADC !				\ set up AD1CON1

	0					\ Configure ADC voltage reference
						\ and buffer fill modes.
						\ VREF from AVDD and AVSS,
						\ Inputs are not scanned,
						\ Interrupt every sample
	ADC AD1CON2 OR !	\ set up AD1CON2

	0
	01f 8 SET_BITS		\ slowest conversion speed
	0ff 0 SET_BITS	
	ADC AD1CON2 OR !	\ set up AD1CON2
		
	0					\ CHO- input is VREFL (AVss)
	1 10 SET_BITS		\ CH0+ input is AN7 - battery
	ADC AD1CHS OR !		\ set up AD1CHS

	ADC 0f BIT_SET		\ turn on ADC
;

: ADC_SAMPLE ( -- value )
	ADC 1 BIT_SET		\ set sampling flag
	ADC ADC1BUF0 or @ 	\ read buffer
	. CR				\ display

\ TODO wait for DONE flag
;

: ADC_DEBUG ( ) 
	HEX
	CR ADC AD1CON1 OR @ .
	CR ADC AD1CON2 OR @ . CR
	CR ADC AD1CON3 OR @ . CR
	CR ADC AD1CHS OR @ . CR
	DECIMAL
;
