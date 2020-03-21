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
  
extern bool trace;
extern bool debug;

extern struct Process* process;
extern struct Process* main_process;

extern uint8_t dictionary[];

int forth_init();

void forth_execute(uint8_t*);

void forth_run();

void forth_tasks(uint32_t ticks);

void memory_dump(uint16_t start, uint16_t size);


#ifdef	__cplusplus
}
#endif

#endif	/* FORTH_H */

