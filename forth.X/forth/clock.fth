noecho
\ clock.fth
\
\ run language.def, rtcc.fth, lcd.fth first
\
\

HEX

: clock_init ( )
	rtcc_init
	hex
	093045 200308 rtcc_set
	2 ms
	rtcc_show
	decimal
				
	init_lcd
;

: '/' ( - ) 
	[CHAR] / HOLD 
;

: ':' ( - ) 
	[CHAR] : HOLD 
;

: clock_display_time ( )
	40 cursor_at_lcd
	0 rtcc_date <# # # '/' # # #> write_lcd_string
	S" --" write_lcd_string
	0 rtcc_time <# # # ':' # # ':' # # #> write_lcd_string	
;

: clock_display ( )
	0 cursor_at_lcd
	S" Date: " write_lcd_string
	S" Time: " write_lcd_string
	clock_display_time 
;

: clock_update 
	begin 
		clock_display_time 
		250 MS
	again
;

\ clock_init
\ task clock
\ ' clock_update clock initiate

DECIMAL

.( added CLOCK words)
echo
