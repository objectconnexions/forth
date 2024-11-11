\ debug usb

\ Descriptor table
BDT_START 16 + DUP hex. @ hex.

U1EP0 @ bin.

0_RX_EVEN 18 dump
0_TX_EVEN 18 dump
