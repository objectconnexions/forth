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

#define BASE_ENTRY NULL
    
typedef void (*CORE_FUNC)(void);

#define CELL_SIZE 4

#define SCRUB (uint8_t) 1
#define IMMEDIATE (uint8_t) 2
#define OTHER (uint8_t) 4
    
struct Dictionary_Entry {
    CODE_INDEX start;           // starting address of entry
    CODE_INDEX end;             // ending address of entry
    char name[32];               // entry's name
    uint8_t flags;
    INSTRUCTION instruction;     // address of entry's executable code
    bool is_core;                   // core function, rather than user created
};
    
struct CORE_ENTRY {
    char *name;
    CORE_FUNC function;
    bool immediate;
};
     
//void dictionary_init(void);
void dictionary_init(CODE_INDEX *, CODE_INDEX *);

void dictionary_init_done(void);

void dictionary_master_reset(void);

void dictionary_purge(struct Dictionary_Entry *);

/*
 * Removes the specified entry and all subsequent entries from 
 * the dictionary.
 */
void dictionary_truncate_at(struct Dictionary_Entry *);

uint32_t dictionary_unused(void);

void dictionary_allot(int32_t);

bool dictionary_is_core_word(CODE_INDEX);

void dictionary_abort_entry(void);

CODE_INDEX dictionary_add_entry(char *);

void dictionary_end_entry(void);

//void dictionary_insert_internal_instruction(uint8_t, CORE_FUNC);

//CODE_INDEX dictionary_add_core_word(char *, CORE_FUNC, bool);

CORE_FUNC dictionary_find_core_function(uint16_t);

bool dictionary_find_entry_with(CODE_INDEX, struct Dictionary_Entry *);

bool dictionary_find_entry_for(char *, struct Dictionary_Entry *);

uint32_t dictionary_read(struct Process *);

CODE_INDEX dictionary_read_instruction(struct Process *);

uint8_t dictionary_read_next_byte(struct Process *);

/*
 *  Return the name of the dictionary entry for the memory at the specified address
 */
bool dictionary_find_word_for(CODE_INDEX, char *);

void dictionary_debug_summary(CODE_INDEX);
        
void dictionary_debug_entry(struct Dictionary_Entry * );

void dictionary_memory_dump(CODE_INDEX, uint16_t);

/*
 * Write details to the console about the the specified command in memory
 * 
 *   <address> <data>  <name> <value|address|function>
 * 
 * Where
 * 
 *   1. address - is the the memory address
 *   2. data - is the data held in memory
 *   3. name - is the word or behaviour that is represented 
 *   4. the final items refer to the data that is being used, the word address 
 *      that is being called, or the function being invoked
 * 
 * The return value indicated whether this instruction is calling out to another
 * word, return from a call, or continuing to work within the same word (+1, -1
 * and 0) respectively.
 */
int8_t dictionary_print_instruction(CODE_INDEX *);

CODE_INDEX dictionary_aligned(CODE_INDEX);

void dictionary_align(void);

uint8_t dictionary_read_byte(CODE_INDEX);

void dictionary_write_byte(CODE_INDEX, BYTE);

void dictionary_words(void);

void dictionary_append_byte(BYTE);

void dictionary_append_cell(CELL);

void dictionary_append_instruction(struct Dictionary_Entry);

void dictionary_append_function(CORE_FUNC);

void dictionary_append_literal(uint32_t);

void dictionary_append_string(char const *);

CODE_INDEX dictionary_offset(void);

void dictionary_debug(void);

void dictionary_debug2(void);

void dictionary_debug_all(void);

//bool dictionary_shortcode(CODE_INDEX);
//
//void dictionary_execute_function(CODE_INDEX);
//
//void dictionary_lock(void);
//
//void dictionary_unlock(void);

void dictionary_mark_internal(void);

CODE_INDEX dictionary_here(void);

void compiler_suspend(void);

void compiler_resume(void);

CODE_INDEX dictionary_pad(void);

int strcicmp(char const *, char const *);

void dictionary_move_to_flash(void);

//void dictionary_move_to_proxy(void);

#ifdef	__cplusplus
}
#endif

#endif	/* DICTIONARY_H */

