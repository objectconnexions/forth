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

enum TYPE 
{
    START,
    WORD_AVAILABLE,
    NUMBER_AVAILABLE,
    END_LINE,
    INVALID_INSTRUCTION
};

void parser_input(char* source);

enum TYPE parser_next_token(void);
 
uint32_t parser_token_number(void);

uint32_t parser_token_word(void);
 
void parser_token_text(char *); 
/*
uint32_t parser_token_text(void); 
 */

void parser_drop_line(void);

uint16_t code_for(char *);


#ifdef	__cplusplus
}
#endif

#endif	/* PARSER_H */

