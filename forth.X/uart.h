/* 
 * File:   uart.h
 * Author: rcm
 *
 * Created on 09 September 2019, 13:30
 */

#ifndef UART_H
#define	UART_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>

void /*__ISR(_UART_2_VECTOR, IPL7SOFT)*/ Uart2Handler(void);

void _console_out(char*, va_list);

/*
 * Write the specified message to the console (via the UART).
 */
void console_out(char*, ...);

/*
 * Write the specified character the console (via the UART).
 */
void console_put(char);

/*
 * Output the specified number of blank spaces to the console.
 */
void console_pad(uint8_t);

void uart_init(void);

void uart_reset(void);

int uart_configure(int); 

int uart_transmit_buffer(const char *);

int uart_transmit_char(const char);

uint8_t uart_next_char(void);

bool uart_has_next_char(void);

bool uart_next_line(char *);

void uart_dispose(void);

void uart_debug(void);

#ifdef	__cplusplus
}
#endif

#endif	/* UART_H */

