
#define _SUPPRESS_PLIB_WARNING 1 



#include <p32xxxx.h>
#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include "uart.h"

#include <stdio.h>
#include <proc/ppic32mx.h>
#include "logger.h"

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

void console_put(char c) 
{
    uart_transmit_char(c);
}

static int number_to_string(char * dest, uint32_t val, uint8_t radix, uint8_t len)
{
    int pos = len == 0 ? 16 : len;
    memset(dest, '0', pos);
    do {
        int digit = val % radix;
        *(dest + --pos) = digit > 9 ? 'A' + digit - 10 : '0' + digit;
        val = val / radix;
    } while (val > 0);
    
    if (len > 0) return len;
    
    // move number string back to beginning of assigned space
    int i;
    for (i = 0; i < 16 - pos; i++) {
        *(dest + i) = *(dest + i + pos); 
    }
    return 16 - pos;
}

void _console_out(char* message, va_list argptr)
{
    // TODO what happens if buffer is not long enough?
    char msg[256];

    char * out = msg;
    while(*message)
    {
        if (*message == '%')
        {
            message++;
            if (*message == 'S')
            {
                char * val = va_arg(argptr, char*);
                if (val != NULL)
                {
                    while (*val)
                    {
                        *out++ = *val++;
                    }
                }
            } 
            else if (*message >= 'X' && *message <= 'Z')
            {
                int len = *message == 'X' ? 2 : (*message == 'Y' ? 4 : 8);
                uint32_t val = va_arg(argptr, uint32_t);                
                out += number_to_string(out, val, 16, len);
            } 
            else if (*message == 'I')
            {
                uint32_t val = va_arg(argptr, uint32_t);
                out += number_to_string(out, val, 10, 0);
            }
            else
            {
                *out++ = '?';
                *out++ = *message;
                *out++ = '?';
            }
        }
        else
        {
            *out++ = *message;
        }
        message++;
    }
    *out = 0;
 

    uart_transmit_buffer(msg);
}

void console_out(char* message, ...)
{
    va_list argptr;
    va_start(argptr, message);
    _console_out(message, argptr);
    va_end(argptr);
}

static int limit() {
    return read_tail > receive_head ? receive_head + LENGTH : receive_head;
}

uint8_t uart_next_char()
{
    if (read_tail < limit()) 
    {
        char c = uart_buffer[read_tail++ % LENGTH];
        read_tail %=  LENGTH;
        return c;
    }
}

bool uart_has_next_char()
{
    return read_tail >= limit();
}

bool uart_next_line(char *buffer) {
//    char *b = buffer;
    if (!line_available)
    {
        return false;
    }
    else 
    {
        line_available = false;
        int max = limit();
//        if (on_hold) console_out("%I < %I\n", read_tail, limit) ;
        while (read_tail < max) 
        {
            char c = uart_buffer[read_tail % LENGTH];
//            if (on_hold) console_out("%c", c) ;
            read_tail++;
            if (c == '\r' || c == '\n')
            {
                *buffer++ = '\0';
                int search = read_tail;  
                while (search < max) 
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
//            console_out("~RUN/n");
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
//            console_out("~HLD\n");
//            uart_debug();
        }
        
        while (U2STAbits.URXDA) 
        {
            char c = U2RXREG; 
//            if (on_hold) console_out("%02x ", c);
            uart_buffer[receive_head % LENGTH]  = c;
            if (c == CTRL_C)
            {
                process_interrupt(1);
            }
            else if (c == CTRL_D)
            {
                console_out("^D ", c);
                process_interrupt(2);
            }
            else 
            {
                receive_head++;

                if (c == '\r' || c == '\n')
                {
                    line_available = true;
                }
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
//    console_out("%I? %04x/%04x (%I)\n", line_available, read_tail, receive_head, free_space);
    if (free_space < 50)
    {
        console_out ("\n0000   ");
        int i;
        for (i = 0; i < 16; i++) 
        {
            console_out("%Z   ", i);
        }
        console_put(NL);
        for (i = 0; i < LENGTH; i++) 
        {
            if (i % 16 == 0)
            {
                int j;
                for (j = 0; j < 16; j++)
                {
                    char c = uart_buffer[i - 16 + j];
                    if (c < 32) c = ' ';
                    console_put(c);
                }
                console_out("\n%Y  ", i);
            }
            console_out("%X ", uart_buffer[i]);
        }
        console_put(NL);
    }
}
