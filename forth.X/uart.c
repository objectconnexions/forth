
#define _SUPPRESS_PLIB_WARNING 1 



#include <p32xxxx.h>
#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <string.h>
#include <stdbool.h>
#include "uart.h"

#include <stdio.h>
#include <proc/ppic32mx.h>
#include <proc/p32mx270f256d.h>

// Defines
#define SYSCLK 48000000L

// Macros
// Equation to set baud rate from UART reference manual equation 21-1
#define Baud2BRG(desired_baud)      ( (SYSCLK / (16*desired_baud))-1)
#define XON 0x11
#define XOFF 0x13
#define LENGTH 512

// static int uart_len;
static char uart_buffer[LENGTH];
//static char *uart_received;
//static char *uart_read;
static int receive_index;
static int read_index;
//static bool uart_read;
static bool line_available;
static bool on_hold;

void uart_init() {
    // RX
    ANSELAbits.ANSA1 = 0;
    U2RXRbits.U2RXR = 0;    //SET RX to RA0
    
    // TX
    RPC2Rbits.RPC2R = 2;    //SET RB9 to TX
 
    uart_configure(57600);  // Configure UART2 for a baud rate of 57600
 
    IFS1bits.U2RXIF = 0;
    
    U2MODESET = 0x8000;     // enable UART2
       
    receive_index = 0;
    read_index = 0;
    on_hold = false;
    line_available = false;
    
    
    /* Add a small delay for the serial terminal
     *  Although the PIC sends out data fine, I've had some issues with serial terminals
     *  being garbled if receiving data too soon after bringing the DTR line low and
     *  starting the PIC's data transmission. This has ony been with higher baud rates ( > 9600) */
    int t;
    for( t=0 ; t < 100000 ; t++);
    
    uart_transmit_char(XON);
    
    memset(uart_buffer, 0, LENGTH);
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
} // END UART2Configure()

int uart_transmit_char(const char c)
{
    while( U2STAbits.UTXBF);    // wait while TX buffer full
    U2TXREG = c;                // send single character to transmit buffer
}

/* SerialTransmit() transmits a string to the UART2 TX pin MSB first
 *
 * Inputs: *buffer = string to transmit */
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

bool uart_read_input(char *buffer) {
    if (!line_available)
    {
        return false;
        
    }
    else 
    {
        line_available = false;
        int limit = read_index > receive_index ? receive_index + LENGTH : receive_index;
        while (read_index <= limit) 
        {
            char c = uart_buffer[read_index % LENGTH];
            *buffer++ = c;
            read_index++;
            if (c == '\r' || c == '\n')
            {
                *buffer = '\0';

                int search = read_index;  
                while (search <= limit) 
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
        }
        read_index = read_index % LENGTH;
        return true;
    }    
}


void __ISR(_UART_2_VECTOR, IPL7SOFT) Uart2Handler(void)
{    
    IEC1bits.U2RXIE = 0;
	if(IFS1bits.U2RXIF)
	{
        while (U2STAbits.URXDA) 
        {
            char c = U2RXREG; 
            uart_buffer[receive_index % LENGTH]  = c;
            receive_index = (receive_index + 1) % LENGTH;
        
            if (c == '\r' || c == '\n')
            {
                line_available = true;
            }
        }
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
    printf("%i? %i/%i\n", line_available, read_index, receive_index);
}