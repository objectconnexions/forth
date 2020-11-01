#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "logger.h"
#include "debug.h"
#include "forth.h"
#include "code.h"
#include "parser.h"
#include "dictionary.h"
#include "compiler.h"

#define LOG "compiler"

#define IN_COMPILATION 1

static void add_literal(uint32_t);
static void add_double_literal(uint64_t);
static void complete_word(void);

static CODE_INDEX jumps[6];
static uint8_t jp = 0;
static bool has_error;

// Comipile state: 1 = in compilation; 0 = not in compilation
CELL state;

void compiler_init()
{
}

void compiler_compile_definition()
{
    char buf[128]; 
    char name[32];
    bool read;
    
    has_error = false;
    state = IN_COMPILATION;
    log_trace(LOG, "new word");
    
    enum TYPE type = parser_next_token();
    if (type != WORD_AVAILABLE && type != INVALID_INSTRUCTION) {
//        parser_drop_line();
//        has_error = false;
        parser_token_text(name);
        log_error(LOG, "can't compile %S", name);
        return;
    }
    parser_token_text(name);
    log_info(LOG, "new word %S", name);
    dictionary_add_entry(name);
    
    while (true) 
    {
        switch (parser_next_token())
        {
            case SINGLE_NUMBER_AVAILABLE:
                add_literal(parser_token_number());
                break;
                
            case DOUBLE_NUMBER_AVAILABLE:
                add_double_literal(parser_token_number());
                break;
                
            case WORD_AVAILABLE:
                ;
                struct Dictionary_Entry entry;
                parser_token_text(entry.name);
                parser_token_entry(&entry);
                if ((entry.flags & IMMEDIATE) == IMMEDIATE)
                {
                    log_debug(LOG, "run immediate %S", entry.name);
                    forth_execute(entry.instruction);
                }
                else if ((entry.flags & SCRUB) == SCRUB)
                {
                    // TODO will this even occur as the entry won't appear to exist until it is completed
                    log_debug(LOG, "recursive call to %S", entry.name);
//                    parser_drop_line();
                    has_error = true;
                    return;
                }
                else
                {
                    log_debug(LOG, "append entry %S", entry.name);
                    dictionary_append_instruction(entry.instruction);
                }
                break;
                
            case INVALID_INSTRUCTION:
//                parser_drop_line();
                parser_token_text(name);
                log_error(LOG, "invalid instruction %S", name);
                has_error = true;
                return;
                
            case END_LINE:
            case BLANK_LINE:
            case NONE:
                if (state == IN_COMPILATION)
                {
                    read = uart_next_line(buf);
                    if (read)
                    {
                        log_debug(LOG, "input line: '%S'", buf);
                        parser_input(buf);
                    }                
                    break;
                }
                else
                {
                    return;
                }
        }
    }
}

static bool add_named_entry() {
    char token[32];
    struct Dictionary_Entry entry;
    has_error = false;
    state = IN_COMPILATION;

    parser_next_text(token);
    to_upper(token);
    log_debug(LOG, "find named entry %S", token);
    if (dictionary_find_entry(token, &entry))
    {
        log_error(LOG, "non-unique name %S", token);
        has_error = true;
        return false;
    }
    else
    {
        dictionary_add_entry(entry.name);
        return true;
    }
}

void compiler_suspend()
{
    state = 0;
    log_debug(LOG, "suspend compiler");
}

void compiler_resume()
{
    state = IN_COMPILATION;
    log_debug(LOG, "resume compiler");
}
    
void compiler_constant()
{
    if (add_named_entry())
    {
        dictionary_append_function(push_literal);
        uint32_t value = pop_stack();
        dictionary_append_value(value);

        complete_word();
    }
}

void compiler_2constant()
{
    if (add_named_entry())
    {
        CELL value1 = pop_stack();
        CELL value2 = pop_stack();
        dictionary_append_function(push_literal);
        dictionary_append_value(value2);
        dictionary_append_function(push_literal);
        dictionary_append_value(value1);
        
        complete_word();
    }
}

static void add_variable(uint8_t size)
{
    dictionary_append_function(memory_address);
    dictionary_align();
    
    int i;
    for (i = 0; i < size; i++) {
        dictionary_append_byte(0); // space for value
    }

    complete_word();
    
}

void compiler_variable()
{
    if (add_named_entry())
    {
        add_variable(4);
    }
}

void compiler_2variable()
{
    if (add_named_entry())
    {
        add_variable(8);
    }
}

void compiler_end()
{
    complete_word();
}

void compiler_if()
{
    dictionary_append_function(zero_branch);
    jumps[jp++] = dictionary_offset();
    dictionary_append_byte(0);  
}

void compiler_begin()
{
    jumps[jp++] = dictionary_offset();
}
    
// TODO these need to check if bounds are exceeded (> 128 or < -127)
void compiler_then()
{
    uint8_t distance = dictionary_offset() - jumps[--jp];
    dictionary_write_byte(jumps[jp], distance);
}

void compiler_else()
{
    dictionary_append_function(branch);
    CODE_INDEX jump_offset = dictionary_offset();
    dictionary_append_byte(0);  

    uint8_t distance = dictionary_offset() - jumps[--jp];
    dictionary_write_byte(jumps[jp], distance);
    
    jumps[jp++] = jump_offset;
}

void compiler_again()
{
    dictionary_append_function(branch);
    uint8_t distance = jumps[--jp] - dictionary_offset();
    dictionary_append_byte(distance);
}    

void compiler_until()
{
    dictionary_append_function(zero_branch);
    uint8_t distance = jumps[--jp] - dictionary_offset();
    dictionary_append_byte(distance);
}
                
void compiler_eol_comment()
{
    parser_drop_line();
}
              
void compiler_inline_comment()
{
    char text[80];
    
    do
    {
        parser_next_text(text);
    } while(text[strlen(text) - 1] != ')');
}
        
/* 
 * print all characters until the next closing bracket ()) so comment is displayed during compilation.
 */
void compiler_print_comment()
{
    char text[80];
    bool end;
    
    do
    {
        parser_next_text(text);
        end = text[strlen(text) - 1] == ')';
        if (end)
        {
            text[strlen(text) - 1] = 0;
            console_out(text);            
        } 
        else
        {
            console_out(text);
            console_put(SPACE);
        }
    } while(!end);
    console_put(NL);
}

void compiler_char()
{
    char name[32];
    parser_next_text(name);    
    
    dictionary_append_function(push_literal);
    dictionary_append_value(name[0]);
}

/*
 * Lays down the characters from the input stream until-the next quote (") character-and places the 
 * string length before the first character.
 */
void compiler_compile_string()
{
    char text[80];
    uint8_t len;
    
    len = 0;
    do
    {
        if (len > 0)
        {
            text[len++] = ' ';
        }
        if (parser_next_text(text + len) == END_LINE) 
//        if (parser_next_text(text + len) == NONE) 
        {
            log_error(LOG, "no end quote %S", text);
            has_error = true;
            return;
        }
        len = strlen(text);
        log_debug(LOG, "%I string %S", len, text);
    } while(text[len - 1] != '"');
    len--;
    
    CODE_INDEX lengthAt = dictionary_here();
    dictionary_append_byte(0);
    
    int len2 = 0;
    int i;
    for (i = 0; i < len; i++) {
        char c = text[i];
        if (c == '\\')
        {
            char c = text[++i];
            switch (c)
            {
                case 'n':
                    dictionary_append_byte('\n');
                    break;
                case 'r':
                    dictionary_append_byte('\r');
                    break;
                case 't':
                    dictionary_append_byte('\t');
                    break;
                case '\\':
                    dictionary_append_byte('\\');
                    break;
                default:
                   dictionary_append_byte(c);
            }
        }
        else 
        {
           dictionary_append_byte(c);
        }
        len2++;
    }
    dictionary_write_byte(lengthAt, len2);
}

void compiler_print_string()
{
    dictionary_append_function(print_string);
    compiler_compile_string();
}

void compiler_c_string()
{
    dictionary_append_function(c_string);
    compiler_compile_string();
}

void compiler_s_string()
{
    dictionary_append_function(s_string);
    compiler_compile_string();
}

void compiler_create_data()
{
    char name[32];
    parser_next_text(name);
    to_upper(name);
    dictionary_add_entry(name);
    dictionary_append_function(data_address);
}

static void complete_word() 
{
    if (has_error) {
        console_out("Compile failed!\n");
    }
    else
    {
        dictionary_append_function(return_to);
        dictionary_end_entry();
    }
    state = 0;
}

static void add_literal(uint32_t value)
{
    dictionary_append_function(push_literal);
    log_trace(LOG, "literal = %Z", value);
    dictionary_append_value(value);
}

static void add_double_literal(uint64_t value)
{
    dictionary_append_function(push_double_literal);
    log_trace(LOG, "literal = %W", value);
    dictionary_append_value(value);
}

