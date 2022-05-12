PWR_LED DIGITAL_OUT
PWR_LED REG_IT_SET



: test_int 
	DIO_1 DIGITAL_IN			\ PB on pin 11 as input
	DIO_1 PULL_UP
	DIO_1 REG_BIT_READ DROP		\ clear interrupt
	
	15 PORTB CNCON REG_BIT_SET	\ enable change notification

\	DIO_1 CNSTAT REG_BIT_CLEAR




	5 IPC0 or 80 + !		\ set int priorities
	\ 400000 IEC1 or !	
	14 IEC1 reg_bit_set		\ enable int

	DIO_1 CNEN REG_BIT_SET


	\ DIO_1 CNSTAT REG_BIT_READ
	
;
