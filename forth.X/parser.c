#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "logger.h"
#include "forth.h"
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
static bool core;
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
        log_info(LOG, "parse: '%S' (%I)", source, len);
        processing = line;
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
    strcpy(text, "");
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
    entry->is_core = core;
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
        log_debug(LOG, "line end %I", ptr - source);
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
    log_debug(LOG, "token '%S' (%I)", token, len);
    
    type = TEXT_AVAILABLE;
}

static void process()
{
    struct Dictionary_Entry entry;

    // to_upper(token, strlen(token));

    if (dictionary_find_entry_for(token, &entry)) {
        instruction = entry.instruction;
        core = entry.is_core;
        flags = entry.flags;
        type = WORD_AVAILABLE;
        log_debug(LOG, "%S word %S, %Z/%X", core ? "core " : "" , token, entry.instruction, entry.flags);
        return;

    } else {
        log_debug(LOG, "parse as number %S", token);

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
        uint32_t entry_base;
        uint8_t len = strlen(token);
        switch (*ptr) {
            case '&':
            case '#':
                entry_base = 10;
                ptr++;
                len--;
                break;
            case '%':
                entry_base = 2;
                ptr++;
                len--;
                break;
            case '$':
                entry_base = 16;
                ptr++;
                len--;
                break;
            case '0':
                if (*(ptr + 1) == 'x' && base_no < 33)
                {
                    ptr += 2;
                    len -= 2;
                    entry_base = 16;
                }
                else 
                {
                    entry_base = base_no;                
                }
                break;
            case '\'':
                number_value = *(ptr + 1);
                return;
            default:
                entry_base = base_no;
                break;
           
     /*       
            
    & ? decimal
    # ? decimal
    % ? binary
    $ ? hexadecimal
    0x ? hexadecimal, if base<33.
    ' ? numeric value (e.g., ASCII code) of next character; an optional ' may be present after the character. 

Here are some examples, with the equivalent decimal number shown after in braces:

-$41 (-65), %1001101 (205), %1001.0001 (145 - a double-precision number), 'A (65), -'a' (-97), &905 (905), $abc (2478), $ABC (2478).

*/
        }
        int i;
        for (i = 0; i < len; i++) {
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
        
            if (digit >= entry_base) {
                type = INVALID_INSTRUCTION;
                return;
            }
        
            number_value = number_value * entry_base + digit;
        }
        number_value *= signed_number;
        return;
    }
}    
