
noecho

S" offset register" T+

T{ PORTB                         ->  $0bf886100 }
T{ 2 PORTB OFFSET_REGISTER       ->  $0bf886120 }

T{ PORTA LAT REGISTER           -> $0bf886030 } 
T{ PORTC ANSEL REGISTER         -> $0bf886200 } 


s" masks" T+

T{ 0 MASK_BIT   -> $01 }
T{ 2 MASK_BIT   -> $04 }
T{ 4 MASK_BIT   -> $010 }
T{ 5 MASK_BIT   -> $020 }
T{ 6 MASK_BIT   -> $040 }

T{ 31 MASK_BIT  -> $80000000 }

T{ 4 4 MASK_PATTERN     ->  $0f0 } 
T{ 4 8 MASK_PATTERN     ->  $0f00 } 
T{ 2 8 MASK_PATTERN     ->  $0300 } 





\ T{ 3 PORTB OFFSET_REGISTER  ->  PORTB $30 + }

\ TODO add constant for ANSELB

s" Read/write register bit" T+

0 ANSELB ! 
T{ ANSELB 1 REG_BIT@                -> 0 }
T{ ANSELB 8 REG_BIT@                -> 0 }

$102 ANSELB !
T{ ANSELB 1 REG_BIT@                -> 1 }
T{ ANSELB 8 REG_BIT@                -> 1 }

0 ANSELB ! 
T{ 1 ANSELB 4 REG_BIT!  ANSELB @    -> $10 }
T{ 1 ANSELB 8 REG_BIT!  ANSELB @    -> $110 }
T{ 0 ANSELB 4 REG_BIT!  ANSELB @    -> $100 }


s" Set/clear/invert register bit" T+

0 ANSELB ! 

T{ ANSELB 1 REG_BIT_SET    ANSELB @   -> $002 }
T{ ANSELB 8 REG_BIT_SET    ANSELB @   -> $102 }
T{ ANSELB 2 REG_BIT_SET    ANSELB @   -> $106 }

T{ ANSELB 5 REG_BIT_CLEAR  ANSELB @   -> $106 }
T{ ANSELB 1 REG_BIT_CLEAR  ANSELB @   -> $104 }
T{ ANSELB 8 REG_BIT_CLEAR  ANSELB @   -> $004 }

T{ ANSELB 9 REG_BIT_INVERT ANSELB @   -> $204 }
T{ ANSELB 6 REG_BIT_INVERT ANSELB @   -> $244 }
T{ ANSELB 2 REG_BIT_INVERT ANSELB @   -> $240 }



s" Read/write register bits" T+

$530F ANSELB ! 
T{ ANSELB 0 4 REG_BITS@                -> $0F }
T{ ANSELB 4 4 REG_BITS@                -> $00 }
T{ ANSELB 8 4 REG_BITS@                -> $03 }
T{ ANSELB 12 4 REG_BITS@               -> $05 }

0 ANSELB ! 
T{ $18 ANSELB 4 8 REG_BITS!  ANSELB @    -> $0180 }
T{ $0F ANSELB 2 4 REG_BITS!  ANSELB @    -> $01BC }


s" port set up" T+

$FFF ANSELB !
0 TRISB ! 

T{ PORTB 5 DIGITAL_IN   ANSELB @ TRISB @   -> $FDF $020 } 
T{ PORTB 8 DIGITAL_IN   ANSELB @ TRISB @   -> $EDF $120 } 

$FFF ANSELB !
$FFF TRISB ! 
$FFF PORTB PORT REGISTER ! 


T{ PORTB 5 DIGITAL_OUT   ANSELB @ TRISB @ PORTB PORT REGISTER @  
    -> $FDF $FDF $FDF } 
T{ PORTB 8 DIGITAL_OUT   ANSELB @ TRISB @ PORTB PORT REGISTER @  
    -> $EDF $EDF $EDF } 

echo
