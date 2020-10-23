#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "uart.h"
#include "debug.h"
#include "forth.h"
#include "parser.h"
#include "interpreter.h"
#include "compiler.h"

#define LOG "Interpreter"

static void interpret_single_number(uint32_t);
static void interpret_double_number(uint64_t);
static void interpret_code(CODE_INDEX);

static bool echo = true;


static void test_compile(char *code)
{
    parser_input(code);
    compiler_compile_definition();
}

void interpreter_run() {
    char buf[128]; 
    char instruction[32];
    struct Dictionary_Entry entry;
    bool read;
    
    switch (parser_next_token())
    {
        case SINGLE_NUMBER_AVAILABLE:
            interpret_single_number(parser_token_number());
            break;

        case DOUBLE_NUMBER_AVAILABLE:
            interpret_double_number(parser_token_number());
            break;

        case WORD_AVAILABLE:
            parser_token_entry(&entry);
            interpret_code(entry.instruction);
            break;

        case PROCESS_AVAILABLE:
            interpret_single_number(parser_token_number());
            break;

        case INVALID_INSTRUCTION:
            parser_token_text(instruction);
            printf("%s!\n> ", instruction);
            parser_drop_line();
            forth_abort();
            break;

        case END_LINE:
            if (echo) printf(" ok\n> ");
            parser_drop_line();
            break;

        case BLANK_LINE:
            if (echo) printf("\n> ");
            parser_drop_line();
            break;
            
        case NONE:
            
            
            read = uart_next_line(buf);
            if (read)
            {
                
                log_debug(LOG, "input line: '%s'", buf);

                if (strcmp(buf, "ddd") == 0)
                {
                    dictionary_memory_dump(0, 0x250);
                }
                else if (strcmp(buf, "eee") == 0)
                {
                    dictionary_debug();
                }
                else if (strcmp(buf, "load") == 0)
                {
                    test_compile("TEST 1 2 3 + + . CR ;");
                    test_compile("ON 0x0bf886220 dup @ 0x01 4 lshift or swap ! ;");
                    test_compile("OFF 0x0bf886220 dup @ 0x01 4 lshift 0x03ff xor and swap ! ;");
                    test_compile("FLASH on 200 ms off 200 ms ;");
                    test_compile("FLASH2 flash flash flash ;");

                }
                else if (strcmp(buf, "##") == 0)
                {
                    uart_debug();            
                }
                else if (strcmp(buf, "ttt") == 0)
                {
                    tasks();            
                }
                else if (strcmp(buf, "echo") == 0)
                {
                    echo = true;   
                }
                else if (strcmp(buf, "noecho") == 0)
                {
                    echo = false;
                }
                else
                {                
                    if (echo) printf("%s ", buf);
                    parser_input(buf);
                }
            }
            else
            {
//                if (log_level <= DEBUG) 
//                {
//                    dump_parameter_stack(process);
//                    printf("\n");
//                }
                    
                    
                wait(250);
            }

            break;
    }
}

static void interpret_single_number(uint32_t value)
{
    log_debug(LOG, "push 0x%04X", value);
    push(value);
}

static void interpret_double_number(uint64_t value)
{
    log_debug(LOG, "push 0x%08X", value);
    push_double(value);
}

static void interpret_code(CODE_INDEX code)
{
    log_debug(LOG, "execute 0x%04X", code);
    forth_execute(code);
}
