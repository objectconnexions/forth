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

void uart_init();

int uart_configure(int); 

int uart_transmit_buffer(const char *);

int uart_transmit_char(const char);

uint8_t uart_next_char(void);

bool uart_has_next_char(void);

bool uart_next_line(char *);

void uart_debug(void);

#ifdef	__cplusplus
}
#endif

#endif	/* UART_H */

