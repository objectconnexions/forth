
#define _SUPPRESS_PLIB_WARNING 1 



#include <p32xxxx.h>
#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <string.h>
#include <stdbool.h>
#include "uart.h"

#include <stdio.h>
#include <proc/ppic32mx.h>

#ifdef MX270
    #include <proc/p32mx270f256d.h>
#endif

// Defines
#define SYSCLK 48000000L

// Macros
// Equation to set baud rate from UART reference manual equation 21-1
#define Baud2BRG(desired_baud)      ( (SYSCLK / (16*desired_baud))-1)
#define XON 0x11
#define XOFF 0x13
#define LENGTH 512
#define CTRL_C 3
#define CTRL_D 4

static char uart_buffer[LENGTH];
static volatile uint16_t receive_head;
static volatile uint16_t read_tail;
static volatile bool line_available;
static volatile bool on_hold;

void uart_init() {

#ifdef MX130
    U2RXRbits.U2RXR = 1;    //SET U2RX to RB5
    // TX
//    ANSELAbits.ANSA3 = 0;
    RPA3Rbits.RPA3R = 2;    //SET RA3 to U2TX    
#elif MX270
    // RX
    ANSELAbits.ANSA1 = 0;
    U2RXRbits.U2RXR = 0;    //SET U2RX to RA0
    // TX
    RPC2Rbits.RPC2R = 2;    //SET RB9 to U2TX
#elif MX570
    // RX
    U2RXRbits.U2RXR = 2;    //SET U2RX to RF4
    // TX
    RPF5Rbits.RPF5R = 1;    //SET RF5 to U2TX
#endif

    uart_configure(57600);  // Configure UART2 for a baud rate of 57600
 
    IFS1bits.U2RXIF = 0;
    
    U2MODESET = 0x8000;     // enable UART2
       
    receive_head = 0;
    read_tail = 0;
    line_available = false;
    
    
    /* Add a small delay for the serial terminal
     *  Although the PIC sends out data fine, I've had some issues with serial terminals
     *  being garbled if receiving data too soon after bringing the DTR line low and
     *  starting the PIC's data transmission. This has ony been with higher baud rates ( > 9600) */
    int t;
    for( t=0 ; t < 100000 ; t++);
        
    memset(uart_buffer, 0, LENGTH);

    on_hold = false;
    uart_transmit_char(XON);
   }


/* UART2Configure() sets up the UART2 for the most standard and minimal operation
 *  Enable TX and RX lines, 8 data bits, no parity, 1 stop bit, idle when HIGH
 *
 * Input: Desired Baud Rate
 * Output: Actual Baud Rate from baud control register U2BRG after assignment*/
int uart_configure(int desired_baud) {
    U2MODE = 0;         // disable autobaud, TX and RX enabled only, 8N1, idle=HIGH
    U2STA = 0x1400;     // enable TX and RX, interrupt when half full
    U2BRG = Baud2BRG(desired_baud); // U2BRG = (FPb / (16*baud)) - 1
 
    U2STAbits.URXISEL = 0b00; //! Rx. Interrupt flag bit is set when a char is received
    IPC9bits.U2IP = 7; //! Interrupt priority of 7
    IPC9bits.U2IS = 3; //! Interrupt sub-priority of 0
    IEC1bits.U2RXIE = 1;
    
    // Calculate actual assigned baud rate
    int actual_baud = SYSCLK / (16 * (U2BRG+1));
 
    return actual_baud;
}

int uart_transmit_char(const char c)
{
    while( U2STAbits.UTXBF);    // wait while TX buffer full
    U2TXREG = c;                // send single character to transmit buffer
}

/* SerialTransmit() transmits a string to the UART2 TX pin MSB first
 *
 * Inputs: *buffer = string to transmit
 */
int uart_transmit_buffer(const char *buffer)
{
    unsigned int size = strlen(buffer);
    while( size)
    {
        uart_transmit_char(*buffer);
        buffer++;                   // transmit next character on following loop
        size--;                     // loop until all characters sent (when size = 0)
    }
 
    while( !U2STAbits.TRMT);        // wait for last transmission to finish
    return 0;
}

bool uart_next_line(char *buffer) {
    char *b = buffer;
    if (!line_available)
    {
        return false;
    }
    else 
    {
        line_available = false;
        int limit = read_tail > receive_head ? receive_head + LENGTH : receive_head;
//        if (on_hold) printf("%i < %i\n", read_tail, limit) ;
        while (read_tail < limit) 
        {
            char c = uart_buffer[read_tail % LENGTH];
//            if (on_hold) printf("%c", c) ;
            read_tail++;
            if (c == '\r' || c == '\n')
            {
                *buffer++ = '\0';
                int search = read_tail;  
                while (search < limit) 
                {
                    c = uart_buffer[search % LENGTH];
                    if (c == '\r' || c == '\n') 
                    {
                        line_available = true;
                        break;
                    }
                    search++;
                }
  
                break;
            }
            else
            {
                *buffer++ = c;
            }
        }

        read_tail %=  LENGTH;
        if (on_hold && (read_tail + LENGTH - receive_head) % LENGTH  > 250) {
//            printf("~RUN/n");
            uart_transmit_char(XON);
            on_hold = false;
        }
        return true;
    }    
}


void __ISR(_UART_2_VECTOR, IPL7SOFT) Uart2Handler(void)
{    
    IEC1bits.U2RXIE = 0;
	if(IFS1bits.U2RXIF)
	{
        if (!on_hold && (read_tail + LENGTH - receive_head - 1) % LENGTH < 50) {
            uart_transmit_char(XOFF);
            on_hold = true;
//            printf("~HLD\n");
//            uart_debug();
        }
        
        while (U2STAbits.URXDA) 
        {
            char c = U2RXREG; 
//            if (on_hold) printf("%02x ", c);
            uart_buffer[receive_head % LENGTH]  = c;
            receive_head++;
        
            if (c == '\r' || c == '\n')
            {
                line_available = true;
            }
            else if (c == CTRL_C)
            {
                process_interrupt(1);
            }
            else if (c == CTRL_D)
            {
                process_interrupt(2);
            }
        }
        receive_head %= LENGTH;
        IFS1bits.U2RXIF = 0;
        IEC1bits.U2RXIE = 1;
	}

	// We don't care about TX interrupt
	if (INTGetFlag(INT_U2TX))
	{
        IFS1bits.U2TXIF = 0;
	}
    
}

void uart_debug() {
    int free_space =  (read_tail + LENGTH - receive_head - 1) % LENGTH;
//    printf("%i? %04x/%04x (%i)\n", line_available, read_tail, receive_head, free_space);
    if (free_space < 50)
    {
        printf ("\n0000   ");
        int i;
        for (i = 0; i < 16; i++) 
        {
            printf("%X   ", i);
        }
        printf ("\n");
        for (i = 0; i < LENGTH; i++) 
        {
            if (i % 16 == 0)
            {
                int j;
                for (j = 0; j < 16; j++)
                {
                    char c = uart_buffer[i - 16 + j];
                    if (c < 32) c = ' ';
                    printf("%c", c);
                }
                printf("\n%04X  ", i);
            }
            printf("%02X ", uart_buffer[i]);
        }
        printf("\n");
    }
}
