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


void uart_init();

int uart_configure(int); 

int uart_transmit_buffer(const char *);

int uart_transmit_char(const char);

void uart_receive();


void uart_input_length();

bool uart_read_input(char *);

void uart_resune(void);

#ifdef	__cplusplus
}
#endif

#endif	/* UART_H */

