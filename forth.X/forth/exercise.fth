\ turn on LED
1 PORTB digital_out
1 PORTB port_on


\ read status reg
ENABLE_ID_CHIP
0x05 WRITE_SPI			\ read
READ_SPI
DISABLE_ID_CHIP
SPI1STAT @ .HEX
. CR

CR

ENABLE_ID_CHIP
0x03 WRITE_SPI			\ read
0x0fa WRITE_SPI			\ serial number address
READ_SPI 	. CR
DISABLE_ID_CHIP
SPI1STAT @ .HEX CR

PORTB 1 port_off
