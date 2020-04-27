#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "forth.h"
#include "parser.h"
#include "interpreter.h"

// TODO make static
void interpret_number(uint32_t);
void interpret_code(uint32_t);


void interpreter_process() {
    char instruction[32];
    
    while (true) 
    {
        switch (parser_next_token())
        {
            case NUMBER_AVAILABLE:
                interpret_number(parser_token_number());
                break;
            case WORD_AVAILABLE:
                interpret_code(parser_token_word());
                break;
            case INVALID_INSTRUCTION:
                parser_token_text(instruction);
                printf("%s!\n", instruction);
                parser_drop_line();
                return;
            case END_LINE:
                return;
        }
    }
}

void interpret_number(uint32_t value)
{
    if (trace) printf("@ push 0x%04X\n", value);
    push(value);
}

void interpret_code(uint32_t code)
{
    if (trace) printf("@ execute 0x%04X\n", code);
    start_code(code);
    
    // TODO do this in the main process
    execute_next_instruction();
}
