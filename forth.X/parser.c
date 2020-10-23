#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "forth.h"
#include "debug.h"
#include "code.h"
#include "dictionary.h"
#include "parser.h"

#define LOG "Parser"

static void parse_next(void);
static void process(void);

static char source[80];
static char * ptr;

static char token[32];
static enum TYPE type;
static CODE_INDEX instruction;
static uint8_t flags;
static UNSIGNED_DOUBLE number_value;


void parser_init()
{
    type = NONE;
    ptr = source;
}

void parser_input(char * line)
{
    uint8_t len = strlen(line);
    if (len > 0) {
        strcpy(source, line);  
        type = START;
        ptr = source;
        log_info(LOG, "parse: '%s' (%i)", source, len);
    } else {
        log_info(LOG, "blank line");
        type = BLANK_LINE;
    }
}

void parser_token_text(char * name)
{
    strcpy(name, token);
}

/*
 Get the next token in its original raw text form
 */
enum TYPE parser_next_text(char * text)
{
    if (type != NONE) {
        parse_next();
        if (type != END_LINE) {
            strcpy(text, token);
        }
    }
    return type;
}

enum TYPE parser_next_token()
{
    if (type != NONE && type != BLANK_LINE) {
        parse_next();
        if (type != END_LINE) {
            process();
        }
    }
    return type;
}

void parser_drop_line()
{
    type = NONE;
    *ptr = 0;
}

uint64_t parser_token_number()
{
    return number_value;
}

// TODO rename
void parser_token_entry(struct Dictionary_Entry *entry)
{
    entry->instruction = instruction;
    entry->flags = flags;
}

static void parse_next()
{
    char * start;  // the start position within the input source
    
    while (*ptr == ' ' || *ptr == '\t') {
        ptr++;
    }
    start = ptr;
    if (*ptr == 0) {
        log_debug(LOG, "line end %i", ptr - source);
        type = END_LINE;
//        type = NONE;
        return;
    }
    while (*ptr != 0 && *ptr != ' ' && *ptr != '\t') {
        ptr++;
    }

    int len = ptr - start;
    strncpy(token, start, len);
    token[len] = 0;
    log_debug(LOG, "token '%s' (%i)", token, len);
    
    type = TEXT_AVAILABLE;
}

static void process()
{
    struct Dictionary_Entry entry;

    to_upper(token, strlen(token));

    if (dictionary_find_entry(token, &entry)) {
        instruction = entry.instruction;
        flags = entry.flags;
        type = WORD_AVAILABLE;
        log_debug(LOG, "word 0x%0X %0X", entry.instruction, entry.flags);
        return;

    } else {
        log_debug(LOG, "not a word %s", token);

        uint8_t process = find_process(token);
        if (process != 0xff) {
            type = PROCESS_AVAILABLE;
            number_value = process;
            return;
        }

        number_value = 0;
        uint64_t signed_number = 1;
        type = SINGLE_NUMBER_AVAILABLE;
        char * ptr = token;
        int i;
        for (i = 0; i < strlen(token); i++) {
            char c = *ptr++;
            
            uint64_t digit = 0;
            if (c >= '0' && c <= '9')
            {
                digit = c - '0';
            } 
            else if (c >= 'a' && c <= 'z')
            {
                digit = c - 'a' + 10;
            }
            else if (c >= 'A' && c <= 'Z')
            {
                digit = c - 'A' + 10;
            }
            else if (i == 0 && c == '-')
            {
                signed_number = -1;
                continue;
            }
            else if (c == ',' || c == '.' || c == '+' || c == '-' || c == '/' || c == ':')
            {
                type = DOUBLE_NUMBER_AVAILABLE;
                continue;
            }
            else
            {
                type = INVALID_INSTRUCTION;
                return;
            }
        
            if (digit >= base_no) {
                type = INVALID_INSTRUCTION;
                return;
            }
        
            number_value = number_value * base_no + digit;
        }
        number_value *= signed_number;
        return;
    }
}    
