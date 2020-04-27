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

#define CODE_END 0xffff

typedef uint16_t CODE_INDEX;
    
//typedef void (*core_function)(void);
typedef void (*CORE_FUNC)(void);


void dictionary_init(void);

void dictionary_reset(void);

bool dictionary_is_core_word(CODE_INDEX);

void dictionary_add_entry(char *);

void dictionary_end_entry(void);

void dictionary_insert_internal_instruction(uint8_t, CORE_FUNC);

void dictionary_add_core_word(char *, CORE_FUNC);

CORE_FUNC dictionary_core_word_function(uint16_t);

CODE_INDEX dictionary_code_for(char * instruction);
/*
uint8_t dictionary_read_byte(uint16_t);

uint16_t dictionary_read_word(uint16_t);

uint32_t dictionary_read_long(uint16_t);
 */

uint32_t dictionary_read(struct Process*);

uint32_t dictionary_read_at(CODE_INDEX);

void dictionary_find_word_for(CODE_INDEX, char*);

uint32_t dictionary_memory_address(CODE_INDEX);

void dictionary_memory_dump(CODE_INDEX, uint16_t);

void dictionary_words(void);
/*
 * 
void dictionary_append_byte(uint8_t);

void dictionary_append_word(uint16_t);
                  
void dictionary_append_long(uint32_t);
 */

void dictionary_append(uint32_t);

#ifdef	__cplusplus
}
#endif

#endif	/* DICTIONARY_H */

