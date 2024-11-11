


: button ( - )
  MNT_LED digital_out
  
  PB1 
  2dup digital_in
  2dup pull_up
  ." button init "
  2dup digital? CR
  
  begin
    2dup digital@
    not if
      50 ms
      2dup digital@
      not if
        \ do action
        ." pressed"
        300 ms
      then
    then
    
    MNT_LED digital_invert
    200 ms
  again
;

task+ scan

variable BUTTON_COUNT

\ check if task can run every 1ms
: scanner ( )
  MNT_LED digital_out
  
  ticks 1000 mod
  
  1
  begin
    1- DUP   NOT IF 
        MNT_LED digital_invert   
        DROP 250
        
        over
        ticks swap - . CR
    THEN
    1 ms
  again
;

\ check if task can run every 1ms
: button_scanner ( )
  MNT_LED digital_out
  
  PB1 digital_in
  PB1 pull_up

  1
  begin
    PB1 digital@
    SWAP
    OVER
    <> IF 
        MNT_LED digital_invert 
        BUTTON_COUNT !+
        TICKS . CR
    ELSE 
        1 ms
    THEN
  again
;

: scan_button ( )
    scan activate button
;

: scan_IO ( )
    scan activate button_scanner
;
