/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef _CODE_H    /* Guard against multiple inclusion */
#define _CODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef uint32_t FORTH_WORD;

typedef uint32_t CELL;

enum codes {
    NOP,

    LIT, 
    ADDR,
    BRANCH, 
    ZBRANCH,
    RETURN, 
    PROCESS_LINE,
    
    PROCESS
};

struct Process {
    uint8_t id;
    uint32_t stack[16];
    uint32_t return_stack[8];
    /*volatile */ uint16_t ip; // instruction pointer
    //TODO change both to unsigned --> then change -1 to 0xffff
    volatile int sp; // stack pointer
    volatile int rsp; // return stack pointer
    
    struct Process* next;
    char *name;
    uint8_t priority;
    uint32_t next_time_to_run;
//    uint8_t* code;  // TODO remove
};

extern struct Process* processes;

void code_init();

    
#ifdef __cplusplus
}
#endif

#endif