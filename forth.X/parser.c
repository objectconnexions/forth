#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "debug.h"
#include "code.h"
#include "dictionary.h"
#include "parser.h"

#define LOG "Parser"

static void parse_next();

static char source[80];
static char * ptr;

static char token[32];
static enum TYPE type;
static uint32_t instruction;
static uint8_t flags;
static uint32_t number_value;


void parser_init()
{
    type = END_LINE;
    ptr = source;
}

void parser_input(char * line)
{
    type = START;
    strcpy(source, line);  
    ptr = source;
    log_info(LOG, "parse: '%s' (%i)", source, strlen(source));
}

void parser_token_text(char * name) {
    strcpy(name, token);
}

enum TYPE parser_next_token()
{
    if (type != END_LINE) {
        parse_next();
    }
    return type;
}

void parser_drop_line()
{
    type = END_LINE;
    *ptr = 0;
}

uint32_t parser_token_number()
{
    return number_value;
}

// TODO rename
void parser_token_entry(struct Dictionary_Entry *entry)
{
    entry->instruction = instruction;
    entry->flags = flags;
}

void parse_next()
{
    char * start;  // the start position within the input source
    struct Dictionary_Entry entry;
    
    while (*ptr == ' ' || *ptr == '\t') {
        ptr++;
    }
    start = ptr;
    if (*ptr == 0) {
        log_debug(LOG, "line end %i", ptr - source);
        type = END_LINE;
        return;
    }
    while (*ptr != 0 && *ptr != ' ' && *ptr != '\t') {
        ptr++;
    }

    int len = ptr - start;
    strncpy(token, start, len);
    token[len] = 0;
    to_upper(token, len);

    log_debug(LOG, "token '%s' (%i)", token, len);
    type = WORD_AVAILABLE;
    if (dictionary_find_entry(token, &entry)) {
        instruction = entry.instruction;
        flags = entry.flags;
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
        

        bool is_number = true;
        int radix = 10;
        int i = 0;
        // TODO determine based number by looking for 0, 0B or 0X first, then confirm every other char is a digit
        if (token[i] == '-')
        {
            i++;
        }
        if (token[i] == '0')
        {
            i++;
            if (token[i] == 'X')
            {
                radix = 16;
                i++;
            }   
            else if (token[i] == 'B')
            {
                radix = 2;
                i++;
            }   
            else if (isdigit(token[i]))
            {
                radix = 8;
                i++;
            }   
            else if (strlen(token) > i)
            {
                is_number = false;
            }
        }
        for (; i < strlen(token); ++i) {
            if (!isdigit(token[i]) && !(radix > 10 && token[i] < ('a' + radix - 10))) {
                is_number = false;
                break;
            }
        }

        if (is_number) {
            int64_t value = strtoll(token, NULL, 0);
            log_debug(LOG, "literal = 0x%09llX", value);
            type = NUMBER_AVAILABLE;
            number_value = value;
            return;
        } else {
            type = INVALID_INSTRUCTION;
            return;
        }
    }
}    
