/* 
 * File:   dictionary.h
 * Author: rcm
 *
 * Created on 21 April 2020, 11:02
 */

#ifndef DICTIONARY_H
#define	DICTIONARY_H

#ifdef	__cplusplus
extern "C" {
#endif

//#include <stdint.h>
#include <stdbool.h>
#include "code.h"

#define LAST_ENTRY NULL
    // 0xffff
    
typedef void (*CORE_FUNC)(void);


#define SCRUB 1
#define IMMEDIATE 2
#define OTHER 4
    
struct Dictionary_Entry {
    char *name;
    uint8_t flags;
    CODE_INDEX instruction;
};

void dictionary_init(void);

void dictionary_reset(void);

uint32_t dictionary_unused(void);

void dictionary_allot(int32_t);

bool dictionary_is_core_word(CODE_INDEX);

void dictionary_add_entry(char *);

void dictionary_end_entry(void);

void dictionary_insert_internal_instruction(uint8_t, CORE_FUNC);

void dictionary_add_core_word(char *, CORE_FUNC, bool);

CORE_FUNC dictionary_find_core_function(uint16_t);

bool dictionary_find_entry(char *, struct Dictionary_Entry *);

uint64_t dictionary_read(struct Process *);

CODE_INDEX dictionary_read_instruction(struct Process *);

uint8_t dictionary_read_next_byte(struct Process *);

void dictionary_find_word_for(CODE_INDEX, char*);

void dictionary_debug_summary(CODE_INDEX);
        
void dictionary_debug_entry(CODE_INDEX);

CODE_INDEX dictionary_data_address(CODE_INDEX);

void dictionary_memory_dump(CODE_INDEX, uint16_t);

void dictionary_align(void);

uint8_t dictionary_read_byte(CODE_INDEX);

void dictionary_write_byte(CODE_INDEX, uint8_t);

void dictionary_words(void);

void dictionary_append_byte(uint8_t);

void dictionary_append_value(uint64_t);

void dictionary_append_instruction(CODE_INDEX);

void dictionary_append_function(CORE_FUNC);

CODE_INDEX dictionary_offset(void);

void dictionary_debug(void);

bool dictionary_shortcode(CODE_INDEX);

void dictionary_execute_function(CODE_INDEX);

void dictionary_lock(void);

void dictionary_mark_internal(void);

CODE_INDEX dictionary_here(void);

void compiler_suspend(void);

void compiler_resume(void);

CODE_INDEX dictionary_pad(void);

void dictionary_restart(CODE_INDEX);

#ifdef	__cplusplus
}
#endif

#endif	/* DICTIONARY_H */

