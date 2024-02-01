#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "logger.h"
#include "forth.h"
#include "code.h"
#include "parser.h"
#include "dictionary.h"
#include "compiler.h"
#include "uart.h"
#include "util.h"

#define LOG "Compiler"

#define IN_COMPILATION (uint8_t) 1

static void add_literal(uint32_t);
static void add_double_literal(uint64_t);
static void complete_word(bool);

static CODE_INDEX block_start[6];
static uint8_t jp = 0;
static CODE_INDEX block_leave[6];
static uint8_t lp = 0;
static bool has_error;

// Compile state: 1 = in compilation; 0 = not in compilation
uint8_t state;

void compiler_init()
{
}

void compiler_compile_definition()
{
    char buf[128]; 
    char name[32];
    bool read;
    
    // TODO why does this not use add_named_entry()?
    
    has_error = false;
    state = IN_COMPILATION;
    log_trace(LOG, "new word");
    
    enum TYPE type = parser_next_token();
    if (type != WORD_AVAILABLE && type != INVALID_INSTRUCTION) {
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
//                    uart_dispose();
//                    return;
                }
                else
                {
                    log_debug(LOG, "append entry %S", entry.name);
                    dictionary_append_instruction(entry);
                }
                break;
                
            case INVALID_INSTRUCTION:
//                parser_drop_line();
                parser_token_text(name);
                log_error(LOG, "invalid instruction %S", name);
                has_error = true;
                uart_dispose();
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
    if (dictionary_find_entry_for(token, &entry))
    {
        log_error(LOG, "non-unique name %S", token);
        has_error = true;
        return false;
    }
    else
    {
        dictionary_add_entry(token);
        return true;
    }
}

void compiler_suspend()
{
    state = (uint8_t) 0;
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
//        dictionary_append_function(push_literal);
        uint32_t value = pop_stack();
//        dictionary_append_literal(value);
        
        add_literal(value);
        complete_word(true);
    }
}

void compiler_2constant()
{
    if (add_named_entry())
    {
        CELL tos = pop_stack();
        CELL nos = pop_stack();
        add_literal(nos);
        add_literal(tos);
        complete_word(true);
    }
}

static void add_variable(uint8_t size)
{
    dictionary_append_function(data_address);
    dictionary_align();
    dictionary_allot(size);
    complete_word(false);
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

void compiler_create_data()
{
    if (add_named_entry())
    {
        dictionary_append_function(data_address);
        dictionary_align();
    }
}

static void add_literal(uint32_t value)
{
    log_debug(LOG, "literal = %Z", value);
    if ((value & 0xC0000000) == 0)
    {
        dictionary_append_literal(value);   
    }
    else 
    {
        dictionary_append_function(push_literal);
        dictionary_append_literal(value);   
    }
}

static void add_double_literal(uint64_t value)
{
    add_literal(value >> 32);
    add_literal(value & 0xFFFFFFFF);
}



void compiler_task()
{
    if (add_named_entry())
    {
        dictionary_append_function(process_address);
        uint32_t addr = (uint32_t) dictionary_aligned(dictionary_here());
        dictionary_append_cell(addr + 4);
        dictionary_align();
        dictionary_allot(4);
        complete_word(false);
    }
}


void compiler_end()
{
    complete_word(true);
}

void compiler_if()
{
    // zbranch offset instruction, over main block
    block_start[jp++] = dictionary_offset();
    dictionary_append_literal(ZERO_BRANCH);
}

/*
 * Add a jump instruction at the specified location that take you to the current
 * dictionary location.
 */
static void update_branch_distance(bool forward, CODE_INDEX start)
{
    uint16_t jump = dictionary_offset() - start;
    if (forward)
    {
        jump -= 4;
    }
    dictionary_write_byte(start + 1, (jump >> 8) & 0xFF );
    dictionary_write_byte(start + 0, jump & 0xFF );
}
    
// TODO these need to check if bounds are exceeded (> 128 or < -127)
void compiler_then()
{
    // zbranch (for if) or branch (for else) offset over respective block
    CODE_INDEX start = block_start[--jp];
    update_branch_distance(true, start);
}

void compiler_else()
{
    // zbranch offset distance, over main block
    CODE_INDEX start = block_start[--jp];
    update_branch_distance(false, start);
 
    // branch over else block
    block_start[jp++] = dictionary_offset();
    dictionary_append_literal(BRANCH);
}

void compiler_do()
{
    dictionary_append_function(do_loop_begin);
    block_start[jp++] = dictionary_offset();
}

void compiler_leave()
{
    dictionary_append_function(do_unloop);
    block_leave[lp++] = dictionary_offset();
    dictionary_append_literal(BRANCH);
}

static void loop()
{
    while (lp > 0)
    {
        CODE_INDEX start = block_leave[--lp];
        update_branch_distance(false, start);
    }
    uint16_t distance = block_start[--jp] - dictionary_offset() - 4;
    dictionary_append_literal(ZERO_BRANCH | distance);
}

void compiler_loop()
{
    dictionary_append_function(do_loop_increment_and_check);
    loop();
    
}

void compiler_loop_plus()
{
    dictionary_append_function(do_loop_add_step_and_check);
    loop();
}

void compiler_begin()
{
    block_start[jp++] = dictionary_offset();
}

void compiler_again()
{
    uint16_t distance = block_start[--jp] - dictionary_offset() - 4;
    dictionary_append_literal(BRANCH | distance);
}    

void compiler_until()
{
    uint16_t distance = block_start[--jp] - dictionary_offset() - 4;
    dictionary_append_literal(ZERO_BRANCH | distance);
}

void compiler_while()
{
    block_start[jp++] = dictionary_offset();
    dictionary_append_literal(ZERO_BRANCH);
}

void compiler_repeat()
{
    CODE_INDEX start = block_start[--jp];
    uint16_t distance_to_repeat = dictionary_offset() - start;
    dictionary_write_byte(start + 1, (distance_to_repeat >> 8) & 0xFF );
    dictionary_write_byte(start + 0, distance_to_repeat & 0xFF );
    
    uint16_t distance_to_begin = block_start[--jp] - dictionary_offset() - 4;
    dictionary_append_literal(BRANCH | distance_to_begin);
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
    }
    while(text[strlen(text) - (size_t) 1] != ')');
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
    }
    while(!end);
}

void compiler_char()
{
    char name[32];
    parser_next_text(name);    
    
    dictionary_append_function(push_literal);
    dictionary_append_literal(name[0]);
}

/*
 * Lays down the characters from the input stream until-the next quote (") character-and places the 
 * string length before the first character.
 */
int compiler_write_string()
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
    }
    while(text[len - 1] != '"');
    len--;




// TODO move to dictionary    
    CODE_INDEX lengthAt = dictionary_here();
    CODE_INDEX pos = lengthAt;
//    dictionary_write_byte(0);
    log_debug(LOG, " write at %Z", pos);
    pos++;
    
    int bytes = 0;
    int i;
    for (i = 0; i < len; i++) {
        char c = text[i];
        if (c == '\\')
        {
            c = text[++i];
            switch (c)
            {
                case 'n':
                   c = '\n';
                    break;
                case 'r':
                    c = '\r';
                    break;
                case 't':
                    c = '\t';
                    break;
                case '\\':
                    c = '\\';
                    break;
                default:
                   c = c;
            }
        }
        dictionary_write_byte(pos++, c);
        bytes++;
    }
    log_debug(LOG, " string length %I at %Z", bytes, lengthAt);
    dictionary_write_byte(lengthAt, bytes);
    return bytes;
}

void compiler_add_string()
{
    dictionary_allot(compiler_write_string() + 1);
}

void compiler_print_string()
{
    dictionary_append_function(print_string);
    compiler_add_string();    
}

void compiler_c_string()
{
    dictionary_append_function(c_string);
    compiler_add_string();
}

void compiler_s_string()
{
    dictionary_append_function(s_string);
    compiler_add_string();
}

static void complete_word(bool with_return) 
{
    if (has_error) {
        dictionary_abort_entry();
        // TODO store name when starting and use to add context to message
        console_out("Compile failed, entry not added!\n");
    }
    else
    {
        if (with_return)
        {
            dictionary_append_function(return_to);
        }
        dictionary_end_entry();
    }
    state = (uint8_t) 0;
}
