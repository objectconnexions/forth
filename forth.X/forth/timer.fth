variable timer_offset
variable timer_elapsed

: timer_stopped ( - f )
	timer_offset @ 0=
;

: timer_time ( - u )
	timer_stopped if 
		timer_elapsed @ 
	else
		ticks 1000 /
		timer_offset @ -
	then
;

: timer_start ( - )
	timer_stopped if
		ticks 1000 /
		timer_elapsed @ -		
		timer_offset !
	then
;

: timer_stop ( - )
	timer_time
	timer_elapsed !
	0 timer_offset !
;

: timer_reset ( - )
	0 timer_elapsed !
	0 timer_offset !
;

: timer_result ( u - caddr u )
	timer_time 
	60 /MOD						\ split into seconds and minutes
	60 /MOD						\ split into minutes and hours
	24 /MOD						\ split into hours and seconds
	100 * +						\ combine into single number
	100 * +
	100 * +
	0 .S <# # # [CHAR] : HOLD # # [CHAR] : HOLD # # BL HOLD #S #>
 ;

timer_reset

