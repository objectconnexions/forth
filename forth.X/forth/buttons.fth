


: button ( - )
  PORTA 1 2dup digital_in
  ." button init "
  2dup port_read .
  
  begin
    2dup port_read
    if
      50 ms
      2dup port_read
      if
        \ do action
        ." pressed"
        300ms
      then
    then
    
    200 ms
  again
;

task scan
' button initiate scan