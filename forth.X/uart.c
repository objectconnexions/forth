
#define _SUPPRESS_PLIB_WARNING 1 



#include <p32xxxx.h>
#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <string.h>
#include <stdbool.h>
#include "uart.h"

// Defines
#define SYSCLK 48000000L

// Macros
// Equation to set baud rate from UART reference manual equation 21-1
#define Baud2BRG(desired_baud)      ( (SYSCLK / (16*desired_baud))-1)

#define LENGTH 100

int uart_len;
char uart_buffer[LENGTH];
char *uart_read_ptr;
bool uart_read;

void uart_init() {
    // RX
    ANSELAbits.ANSA1 = 0;
    U2RXRbits.U2RXR = 0;    //SET RX to RA0

    // TX
    RPC2Rbits.RPC2R = 2;    //SET RB9 to TX
 
    uart_configure(57600);  // Configure UART2 for a baud rate of 57600
    U2MODESET = 0x8000;     // enable UART2
 
    uart_read_ptr = uart_buffer;
    uart_len = 0;
    uart_read = 1;
    
    
    /* Add a small delay for the serial terminal
     *  Although the PIC sends out data fine, I've had some issues with serial terminals
     *  being garbled if receiving data too soon after bringing the DTR line low and
     *  starting the PIC's data transmission. This has ony been with higher baud rates ( > 9600) */
    int t;
    for( t=0 ; t < 100000 ; t++);
}


/* UART2Configure() sets up the UART2 for the most standard and minimal operation
 *  Enable TX and RX lines, 8 data bits, no parity, 1 stop bit, idle when HIGH
 *
 * Input: Desired Baud Rate
 * Output: Actual Baud Rate from baud control register U2BRG after assignment*/
int uart_configure(int desired_baud) {
 
    U2MODE = 0;         // disable autobaud, TX and RX enabled only, 8N1, idle=HIGH
    U2STA = 0x1400;     // enable TX and RX
    U2BRG = Baud2BRG(desired_baud); // U2BRG = (FPb / (16*baud)) - 1
 
    // Calculate actual assigned baud rate
    int actual_baud = SYSCLK / (16 * (U2BRG+1));
 
    return actual_baud;
} // END UART2Configure()
 
/* SerialTransmit() transmits a string to the UART2 TX pin MSB first
 *
 * Inputs: *buffer = string to transmit */
int uart_transmit(const char *buffer)
{
    unsigned int size = strlen(buffer);
    while( size)
    {
        while( U2STAbits.UTXBF);    // wait while TX buffer full
        U2TXREG = *buffer;          // send single character to transmit buffer
 
        buffer++;                   // transmit next character on following loop
        size--;                     // loop until all characters sent (when size = 0)
    }
 
    while( !U2STAbits.TRMT);        // wait for last transmission to finish
 
    return 0;
}
 
/* SerialReceive() is a blocking function that waits for data on
 *  the UART2 RX buffer and then stores all incoming data into *buffer
 *
 * Note that when a carriage return '\r' is received, a nul character
 *  is appended signifying the strings end
 *
 * Inputs:  *buffer = Character array/pointer to store received data into
 *          max_size = number of bytes allocated to this pointer
 * Outputs: Number of characters received */
void uart_receive()
{
    while(U2STAbits.URXDA) 
    {
        /*
        if (uart_len + 1 >= LENGTH)
        {
            *uart_read_ptr = '\0';
            uart_read = false;
            return;
        }
        */
    
        *uart_read_ptr = U2RXREG;          // empty contents of RX buffer into *buffer pointer
/*        if(*uart_read_ptr == '\n') {        // drop new lines
            continue;
        }
  */
        uart_len++;
        if(*uart_read_ptr == '\r' || *uart_read_ptr == '\n') {
            *uart_read_ptr = '\0';
            uart_read = false;
            return;
        }
        uart_read_ptr++;
    }

    
    
    
    
    
  //  unsigned int num_char = 0;
 
    /* Wait for and store incoming data until either a carriage return is received
     *   or the number of received characters (num_chars) exceeds max_size */
    /*
    while(num_char < max_size)
    {
        while( !U2STAbits.URXDA);   // wait until data available in RX buffer
        *buffer = U2RXREG;          // empty contents of RX buffer into *buffer pointer
 PORTBbits.RB1 = 1;
        // insert nul character to indicate end of string
        if( *buffer == '\r'){
            *buffer = '\0';     
            break;
        }
 
        buffer++;
        num_char++;
    }
 */
//    return num_char;
} // END SerialReceive()



int uart_input_length() {
    return uart_read ? 0 : uart_len;
}

void uart_read_input(char *buffer) {
    uart_read_ptr = uart_buffer;
    strcpy(buffer, uart_read_ptr);
    
    uart_len = 0;
    uart_read = 1;
}
