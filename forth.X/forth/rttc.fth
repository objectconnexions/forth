noecho

HEX
0bf800200 CONSTANT RTCCON
0bf800220 CONSTANT RTCTIME
0bf800230 CONSTANT RTCDATE

: rtcc_init ( -- ) 
	1 OSCCON REG_BIT_SET		\ SOSCEN - turn secondary oscillator on
	16 OSCCON  REG_BIT_SET		\ SOSCRDY - make secondary oscillator ready

	5 ms
\	OSCCON @ 4000 and . cr 	\ is ready?
	16 OSCCON REG_BIT@ cr . 	\ is ready?		
;

: rtcc_set ( time date -- )
	SYS_UNLOCK
	3 RTCCON REG_BIT_SET		\ make write enable
	f RTCCON REG_BIT_CLEAR		\ turn RTCC off 
	SYS_LOCK
	
	5 ms

	8 LSHIFT
	RTCDATE !				\ store date
	8 LSHIFT
	RTCTIME !				\ store time
	f RTCCON REG_BIT_SET		\ turn RTCC on
;

: rtcc_date ( -- n  get date as BCD yymmdd ) 
	RTCDATE @ 8 RSHIFT
;

: rtcc_time ( -- n  get date as BCD yymmdd ) 
	RTCTIME @ 8 RSHIFT
;

: rtcc_show ( -- )
	HEX
	CR ." Date " rtcc_date .
	CR ." Time " rtcc_time .
;

: rtcc_test ( -- )
	rtcc_init
	hex
	093045 200308 rtcc_set
	rtcc_show
	decimal
;

DECIMAL

.( added RTTC words)
echo
