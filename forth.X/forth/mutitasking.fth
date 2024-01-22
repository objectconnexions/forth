: timer ticks 1000 ms ticks .S CR swap .S CR - . CR ;

task+ flashing

: testp ." example printing" CR ;

: test_proc flashing activate begin 65 emit 2000 ms again ;
