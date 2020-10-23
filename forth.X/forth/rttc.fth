noecho

HEX
0bf800200 CONSTANT RTCCON
0bf800220 CONSTANT RTCTIME
0bf800230 CONSTANT RTCDATE

: rtcc_init ( -- ) 
	OSCCON 1 BIT_SET		\ SOSCEN - turn secondary oscillator on
	OSCCON 16 BIT_SET		\ SOSCRDY - make secondary oscillator ready

	5 ms
\	OSCCON @ 4000 and . cr 	\ is ready?
	OSCCON 16 BIT_READ cr . \ is ready?		
;

: rtcc_set ( time date -- )
	0aa996655 SYSKEY !		\ write unlock sequence
	0556699aa SYSKEY !
	RTCCON 3 BIT_SET		\ make write enable

	RTCCON f BIT_CLR		\ turn RTCC off 
	
	5 ms

	\ RTCCON @ 4 and . CR
	
	8 LSHIFT
	RTCDATE !				\ store date
	8 LSHIFT
	RTCTIME !				\ store time
	RTCCON f BIT_SET		\ turn RTCC on
;

: rtcc_date ( -- n  get date as BCD yymmdd ) 
	RTCDATE @ 8 RSHIFT
;

: rtcc_time ( -- n  get date as BCD yymmdd ) 
	RTCTIME @ 8 RSHIFT
;

: rtcc_show ( -- )
	DECIMAL
	CR ." Date " rtcc_date .
	CR ." Time " rtcc_time .
;

: rtcc_test ( -- )
	rtcc_init
	hex
	093045 200308 rtcc_set
	decimal
	rtcc_show
;

DECIMAL

.( added RTTC words)
echo
