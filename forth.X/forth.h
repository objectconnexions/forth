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
    
#define LIT 0x1
#define PROCESS 0x2
#define DUP 0x3
#define SWAP 0x4
#define DROP 0x5
#define NIP 0x6
#define TUCK 0x7
#define OVER 0x8
#define ROT 0x9

#define ADD 0x10
#define SUBTRACT 0x11
#define DIVIDE 0x12
#define MULTIPLY 0x13
#define GREATER_THAN 0x15
#define EQUAL_TO 0x16
#define AND 0x17
#define OR 0x18
#define XOR 0x19
#define LSHIFT 0x1a
#define RSHIFT 0x1b

#define PRINT_TOS 0xc8
#define PRINT_HEX 0xc9 
#define CR 0xca
#define EMIT 0xcb

#define READ_MEMORY 0xc0
#define WRITE_MEMORY 0xc1

#define RUN 0xe0
#define CONST 0xe1
#define VAR 0xe2
#define RETURN 0xe3
#define ZBRANCH 0xe4
#define BRANCH 0xe5

#define EXECUTE 0xe6
#define INITIATE 0xe7
#define YIELD 0xe8
#define WAIT 0xe9

#define WORDS 0xf0
#define STACK 0xf1
#define DUMP 0xf2
#define TRACE_ON 0xf3
#define TRACE_OFF 0xf4
#define DEBUG_ON 0xf8
#define DEBUG_OFF 0xf9
#define RESET 0xf5
#define CLEAR_REGISTERS 0xf6
#define TICKS 0xf7
#define END 0xff

    
  
extern bool trace;
extern bool debug;

extern struct Process* process;
extern struct Process* main_process;

extern uint8_t dictionary[];

int forth_init();

void forth_execute(uint8_t*);

void forth_run();

void forth_tasks(uint32_t ticks);

#ifdef	__cplusplus
}
#endif

#endif	/* FORTH_H */

