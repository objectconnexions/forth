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

//#include "dictionary.h"
    
    
typedef uint32_t CELL;

typedef uint8_t* CODE_INDEX;

enum codes {
    NOP,

    LIT, 
    ADDR,
    BRANCH, 
    ZBRANCH,
    RETURN, 
    PROCESS_LINE,
    
    PRINT_STRING,
    S_STRING,
    C_STRING,
    
    PROCESS
};

CODE_INDEX memory_test;

struct Process {
    uint8_t id;
    uint32_t stack[16];
    uint32_t return_stack[8];
    /*volatile */ 
    CODE_INDEX ip; // instruction pointer
    //TODO change both to unsigned --> then change -1 to 0xffff
    volatile int sp; // stack pointer
    volatile int rsp; // return stack pointer
    
    struct Process* next;
    char *name;
    uint8_t priority;
    uint32_t next_time_to_run;
};

void code_init();
    
#ifdef __cplusplus
}
#endif

#endif