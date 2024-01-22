


: button ( - )
  DIO_1 2dup digital_in
  ." button init "
  2dup reg_bit@ .
  
  begin
    2dup reg_bit@
    if
      50 ms
      2dup reg_bit@
      not if
        \ do action
        ." pressed"
        300 ms
      then
    then
    
    200 ms
  again
;

task+ scan

: scan_button ( )
    scan activate button
;
