HEX 


\ Peripheral clock is at 48MHz

0e disable_int
0 T3CON !			\ disable TIMER 2

0070 T3CON !		\ 1:256 prescale => 187.5kHz
927C PR3 !			\ Count down from 37500, for delay of 0.2 seconds
0 TMR3 !			\ reset counter
8070 T3CON !		\ enable

TMR3 ? 				\ READ Timer


1 log

0c 2 0 priority_int
0e clear_int
0e enable_int

