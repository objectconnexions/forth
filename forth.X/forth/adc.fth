0x0bf809000 CONSTANT ADC
0x0bf886220 CONSTANT PORTE
4 CONSTANT LED

: ADC_INIT ( -- )
	ADC 15 BIT_CLR		\ prepare to set up ADC - turn off 

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
	ADC 0x10 OR !		\ set up AD1CON2

	0
	0x01f 8 SET_BITS	\ slowest conversion speed
	0x0ff 0 SET_BITS	
	ADC 0x20 OR !		\ set up AD1CON2
		
	0					\ CHO- input is VREFL (AVss)
	7 16 SET_BITS		\ CH0+ input is AN7 - battery
	ADC 0x40 OR !		\ set up AD1CHS

	ADC 15 BIT_SET		\ turn on ADC
;

: ADC_SAMPLE ( -- value )
	ADC 1 BIT_SET		\ set sampling flag
	ADC 0x70 or @ 		\ read buffer
	. CR				\ display

\ TODO wait for DONE flag
;

: ADC_DEBUG ( ) 
	ADC @ .HEX CR
	ADC 0x10 OR @ .HEX CR
	ADC 0x20 OR @ .HEX CR
	ADC 0x40 OR @ .HEX CR
;
