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

extern struct Process* current_process;

void start_code(CODE_INDEX);

int forth_init();

void forth_execute(CODE_INDEX);

void forth_run();

void forth_tasks(CODE_INDEX);

void push(uint32_t);

uint32_t pop(void);

void process_interrupt(uint8_t);

uint8_t find_process(char *);



void nop(void);

void push_literal(void);

void memory_address(void);

void branch(void);

void zero_branch(void);

void return_to(void);
void interpreter_run(void);
void print_string(void);
void s_string(void);
void c_string(void);
void data_address(void);

#ifdef	__cplusplus
}
#endif

#endif	/* FORTH_H */

