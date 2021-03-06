/* 
 * File:   parser.h
 * Author: rcm
 *
 * Created on 04 April 2020, 11:30
 */

#ifndef PARSER_H
#define	PARSER_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stdint.h>
#include "dictionary.h"

enum TYPE 
{
    NONE,
    BLANK_LINE,
    END_LINE,
    INVALID_INSTRUCTION,
    START,
    TEXT_AVAILABLE,
    WORD_AVAILABLE,
    SINGLE_NUMBER_AVAILABLE,
    DOUBLE_NUMBER_AVAILABLE,
    PROCESS_AVAILABLE,
};

void parser_init(void);

void parser_input(char *);

enum TYPE parser_next_text(char *);
    
enum TYPE parser_next_token(void);
 
uint64_t parser_token_number(void);

void parser_token_entry(struct Dictionary_Entry *);
 
void parser_token_text(char *); 
 
void parser_drop_line(void);

uint16_t code_for(char *);


#ifdef	__cplusplus
}
#endif

#endif	/* PARSER_H */

