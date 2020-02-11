0x0bf80f000 CONSTANT OSCCON
0x0bf800200 CONSTANT RTCCON
0x0bf800220 CONSTANT RTCTIME
0x0bf800230 CONSTANT RTCDATE

: clock_init ( -- ) 
	OSCCON 1 BIT_SET		\ SOSCEN - turn secondary oscillator on
\	OSCCON 22 BIT_SET		\ SOSCRDY - make secondary oscillator ready

	5 ms
	OSCCON @ 0x4000 and .hex cr \ is ready?	
;

: rtcc_set ( time date -- )
	0x0aa996655 SYSKEY !	\ write unlock sequence
	0x0556699aa SYSKEY !
	RTCCON 3 BIT_SET		\ make write enable

	RTCCON 15 BIT_CLR		\ turn RTCC off 
	
	5 ms

	RTCCON @ 0x4 and .HEX CR
	
	RTCDATE !				\ store date
	RTCTIME !				\ store time
	RTCCON 15 BIT_SET		\ turn RTCC on
;

: rtcc_date ( -- n  get date as BCD yymmdd ) 
	RTCDATE @ 8 rshift
;

: rtcc_time ( -- n  get date as BCD yymmdd ) 
	RTCTIME @ 8 rshift
;

: rtcc_show ( -- )
\	RTCDATE @ .HEX CR
\	RTCTIME @ 0x100 / .HEX CR
	." Date" rtcc_date .HEX CR
	." Time" rtcc_time .HEX CR
;

: rtcc_test ( -- )
	clock_init
	0x09304500 0x20190308	rtcc_set
	rtcc_read
;
