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

#include <stdint.h>
#include "code.h"

#define LAST_ENTRY 0xffff

typedef uint16_t CODE_INDEX;
    
typedef void (*CORE_FUNC)(void);


#define SCRUB 1
#define IMMEDIATE 2
#define OTHER 4
    
struct Dictionary_Entry {
    char *name;
    uint8_t flags;
    uint32_t instruction;
};

void dictionary_init(void);

void dictionary_reset(void);

bool dictionary_is_core_word(CODE_INDEX);

void dictionary_add_entry(char *);

void dictionary_end_entry(void);

void dictionary_insert_internal_instruction(uint8_t, CORE_FUNC);

void dictionary_add_core_word(char *, CORE_FUNC, bool);

CORE_FUNC dictionary_find_core_function(uint16_t);

bool dictionary_find_entry(char *, struct Dictionary_Entry *);

uint32_t dictionary_read(struct Process*);

uint8_t dictionary_read_byte(struct Process *);

void dictionary_find_word_for(CODE_INDEX, char*);

void dictionary_debug_summary(uint32_t);
        
void dictionary_debug_entry(CODE_INDEX);

uint32_t dictionary_data_address(CODE_INDEX);

void dictionary_memory_dump(CODE_INDEX, uint16_t);

void dictionary_write_byte(CODE_INDEX, uint8_t);

void dictionary_words(void);

void dictionary_append_byte(uint8_t);

void dictionary_append_value(uint32_t);

void dictionary_append_instruction(FORTH_WORD);

CODE_INDEX dictionary_offset(void);

void dictionary_debug(void);

bool dictionary_shortcode(CODE_INDEX);

void dictionary_execute_function(CODE_INDEX);


#ifdef	__cplusplus
}
#endif

#endif	/* DICTIONARY_H */

