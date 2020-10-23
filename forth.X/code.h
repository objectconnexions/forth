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
#include <stdbool.h>   
    
typedef uint32_t CELL;

typedef int32_t SIGNED;
typedef uint32_t UNSIGNED;
    
typedef int64_t SIGNED_DOUBLE;
typedef uint64_t UNSIGNED_DOUBLE;

typedef uint8_t* CODE_INDEX;

struct Process {
    uint8_t id;
    CELL stack[16];
    CELL return_stack[8];
    /*volatile */ 
    CODE_INDEX ip; // instruction pointer
    //TODO change both to unsigned --> then change -1 to 0xffff
    volatile int sp; // stack pointer
    volatile int rsp; // return stack pointer
    
    struct Process* next;
    char *name;
    uint8_t priority;
    uint32_t next_time_to_run;
    bool log;
    bool suspended;
};

void code_init();
    
#ifdef __cplusplus
}
#endif

#endif