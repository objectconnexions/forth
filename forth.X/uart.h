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

int uart_transmit(const char *);

void uart_receive();


int uart_input_length();

void uart_read_input(char *);



#ifdef	__cplusplus
}
#endif

#endif	/* UART_H */

