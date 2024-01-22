\ noecho

DECIMAL

$0bf809000 CONSTANT AD1CON1
$0bf809010 CONSTANT AD1CON2
$0bf809020 CONSTANT AD1CON3
$0bf809040 CONSTANT AD1CHS
$0bf809050 CONSTANT AD1CSSL

$0bf809070 CONSTANT ADC1BUF0
\  +10 for BUF1, +20 for BUF2 etc


: ADC_INIT ( -- )
	15 AD1CON1 REG_BIT_CLEAR			\ prepare to set up ADC, ON (15:15) to 0 - turn off 
	4 8 3 AD1CON1 REG_BITS!				\ set data format, FORM (10:8) to 4 - 32 bit integer
	7 5 3 AD1CON1 REG_BITS!				\ set conversion trigger, SSRC (7:5) to 7, internal counter - auto convert
	2 AD1CON1 REG_BIT_CLEAR				\ reset ASAM (2:2), manual sampling

	0 AD1CON2 !							\ set voltage references, VCFG (7:5) to 0, AVDD~AVSS
										\ and buffer fill modes.
										\ reset CSCNA (10:10), Inputs are not scanned,
										\ SMPI (5:2) to 0, Interrupt every sample
	
	$01f 8 5 AD1CON3 REG_BITS!			\ Auto sample time, SAMC (12:8) to 31 TAD
	$0ff 0 8 AD1CON3 REG_BITS!			\ Conversion clock, ADCS (7:0) to 255, the slowest conversion speed

	23 AD1CHS REG_BIT_CLEAR				\ Negative input select, CHONA (23:23) to 0, input is VREFL (AVss)	

	15 AD1CON1 REG_BIT_SET				\ turn on ADC
;

\ eg ADC_!
: ADC_INPUT ( bit port -- )

\	2DUP 
	.S CR
	ANSELA REG_BY_OFFSET REG_BIT_SET   \ set ADC input
	.S CR
\	DROP ADC_SELECT					    \ input is AN6 (A1)
;

: ADC_SAMPLE ( channel -- value )
	16 4 AD1CHS REG_BITS!				\ Positive input select, CH0SA (19:16) 
	
	1 AD1CON1 REG_BIT_SET				\ set sampling flag, SAMP (1:1)
                                        \ TODO wait for DONE flag
	50 MS
	ADC1BUF0 @ 						    \ read value
;

: ADC_DEBUG ( ) 
	HEX
	CR ." CON1 " AD1CON1 ?
	CR ." CON2 " AD1CON2 ?
	CR ." CON3 " AD1CON3 ?
	CR ." CHS " AD1CHS ?
	DECIMAL
;
