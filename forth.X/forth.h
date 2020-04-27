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

extern struct Process* process;
extern struct Process* main_process;

extern uint8_t dictionary[];

void start_code(uint32_t);

int forth_init();

void forth_execute(uint8_t*);

void forth_run();

void forth_tasks(uint32_t);

void push(uint32_t);

uint32_t pop(void);

#ifdef	__cplusplus
}
#endif

#endif	/* FORTH_H */

