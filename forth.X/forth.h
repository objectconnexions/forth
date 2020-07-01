/* 
 * File:   forth.h
 * Author: rcm
 *
 * Created on 13 September 2019, 08:54
 */

#ifndef FORTH_H
#define	FORTH_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "dictionary.h"

//extern struct Process* process;
//extern struct Process* main_process;
    
extern uint32_t timer;

extern uint8_t dictionary[];

void start_code(CODE_INDEX);

int forth_init();

void forth_execute(CODE_INDEX);

void forth_run();

void forth_tasks(CODE_INDEX);

void push(uint32_t);

uint32_t pop(void);

void process_interrupt(uint8_t);

uint8_t find_process(char *);

#ifdef	__cplusplus
}
#endif

#endif	/* FORTH_H */

