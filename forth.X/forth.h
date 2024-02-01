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

extern bool trace_code;
    
extern uint32_t base_no;

extern uint8_t dictionary[];

extern struct Process* current_process;

void forth_trace(bool);

void start_code(CODE_INDEX);

int forth_init();

void forth_execute(CODE_INDEX);

void forth_run();

void forth_interrupt(uint8_t);

void forth_tasks(CODE_INDEX);

void forth_abort(void);

void push(UNSIGNED);

void push_double(SIGNED_DOUBLE);

uint32_t pop_stack(void);

void process_abort(uint8_t);

uint8_t find_process(char *);

void wait(uint32_t);

void nop(void);

void push_literal(void);

void push_double_literal(void);

void data_address(void);

void process_address(void);

void branch(void);

void zero_branch(void);

void do_unloop(void);

void yield(void);
void return_to(void);
void interpreter_run(void);
void print_string(void);
void s_string(void);
void c_string(void);

void do_loop_begin(void);
void do_loop_add_step_and_check(void);
void do_loop_increment_and_check(void);



void example(void);

extern const struct CORE_ENTRY core_funcs[];

#ifdef	__cplusplus
}
#endif

#endif	/* FORTH_H */

