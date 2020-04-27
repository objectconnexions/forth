#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "debug.h"
#include "code.h"
#include "dictionary.h"
#include "parser.h"

static void parse_next();

static char source[80];
static char * ptr;

static char token[32];
static enum TYPE type;
static uint32_t instruction;
static uint32_t number_value;


void parser_input(char * line)
{
    type = START;
    strcpy(source, line);  
    ptr = source;
    if (trace) {
        printf("~ parse: %s (%i)\n", source, strlen(source));
    }
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
uint32_t parser_token_word()
{
    return instruction;
}

void parse_next()
{
    char * start;  // the start position within the input source

    while (*ptr == ' ' || *ptr == '\t') {
        ptr++;
    }
    start = ptr;
    if (*ptr == 0) {
        if (trace) {
            printf("~ line end %i\n", ptr - source);
        }
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

    if (trace) {
        printf("~ token [%s] %i\n", token, len);
    }
    uint16_t code = dictionary_code_for(token); 
    if (code != CODE_END) {
        type = WORD_AVAILABLE;
        instruction = code;
        if (trace) printf("~ word 0x%0X\n", code);
        return;

    } else {
        if (trace) printf("~ not a word %s\n", token);

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
            if (trace) {
                printf("~ literal = 0x%09llX\n", value);
            }
            type = NUMBER_AVAILABLE;
            number_value = value;
            return;
        } else {
            type = INVALID_INSTRUCTION;
            return;
        }
    }
}    
