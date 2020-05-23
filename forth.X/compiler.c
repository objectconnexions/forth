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

static void add_literal(uint64_t);
static void complete_word(void);

static CODE_INDEX jumps[6];
static uint8_t jp = 0;
static bool compiling;
static bool has_error;

void compiler_init()
{
}

void compiler_compile()
{
    char buf[128]; 
    char name[32];
    bool read;
    
    has_error = true;
    compiling = true;
    log_trace(LOG, "has_error new word");
    
    enum TYPE type = parser_next_token();
    if (type != WORD_AVAILABLE && type != INVALID_INSTRUCTION) {
//        parser_drop_line();
        has_error = false;
        parser_token_text(name);
        log_error(LOG, "can't compile %s", name);
        return;
    }
    parser_token_text(name);
    log_info(LOG, "new word %s", name);
    dictionary_add_entry(name);
    
    while (true) 
    {
        switch (parser_next_token())
        {
            case NUMBER_AVAILABLE:
                add_literal(parser_token_number());
                break;
                
            case WORD_AVAILABLE:
                parser_token_text(name);
                struct Dictionary_Entry entry;
                parser_token_entry(&entry);
                if ((entry.flags & IMMEDIATE) == IMMEDIATE)
                {
                    log_debug(LOG, "run immediate %s", name);
                    forth_execute(entry.instruction);
                }
                else if ((entry.flags & SCRUB) == SCRUB)
                {
                    // TODO will this even occur as the entry won't appear to exist until it is completed
                    log_debug(LOG, "recursive call to %s", name);
//                    parser_drop_line();
                    has_error = false;
                    return;
                }
                else
                {
                    log_debug(LOG, "append entry %s", name);
                    dictionary_append_instruction(entry.instruction);
                }
                break;
                
            case INVALID_INSTRUCTION:
//                parser_drop_line();
                parser_token_text(name);
                log_error(LOG, "invalid instruction %s", name);
                has_error = false;
                return;
                
            case END_LINE:
                if (compiling)
                {
                    read = uart_next_line(buf);
                    if (read)
                    {
                        log_debug(LOG, "input line: '%s'", buf);
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

void compiler_constant()
{
    char name[32];
    has_error = true;
    compiling = true;

    if (parser_next_token() != INVALID_INSTRUCTION) {
//        parser_drop_line();
        has_error = false;
        parser_token_text(name);
        log_error(LOG, "non-unique name %s", name);
    }
    
    parser_token_text(name);
    dictionary_add_entry(name);
    
    dictionary_append_instruction(LIT);
    uint32_t value = pop_stack();
    dictionary_append_value(value);
    
    complete_word();

}

void compiler_variable()
{
    char name[32];
    has_error = true;
    compiling = true;
    
    if (parser_next_token() != INVALID_INSTRUCTION) {
//        parser_drop_line();
        has_error = false;
        parser_token_text(name);
        log_error(LOG, "non-unique name %s", name);
    }
    
    parser_token_text(name);
    dictionary_add_entry(name);
    
    dictionary_append_instruction(ADDR);
    dictionary_align();

    dictionary_append_byte(0); // space for value
    dictionary_append_byte(0); // space for value
    dictionary_append_byte(0); // space for value
    dictionary_append_byte(0); // space for value
    
    complete_word();
}

void compiler_end()
{
    complete_word();
}

void compiler_if()
{
    dictionary_append_instruction(ZBRANCH);
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
    dictionary_append_instruction(BRANCH);
    CODE_INDEX jump_offset = dictionary_offset();
    dictionary_append_byte(0);  

    uint8_t distance = dictionary_offset() - jumps[--jp];
    dictionary_write_byte(jumps[jp], distance);
    
    jumps[jp++] = jump_offset;
}

void compiler_again()
{
    dictionary_append_instruction(BRANCH);
    uint8_t distance = jumps[--jp] - dictionary_offset();
    dictionary_append_byte(distance);
}    

void compiler_until()
{
    dictionary_append_instruction(ZBRANCH);
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
        parser_next_token();
        parser_token_text(text);
    } while(text[strlen(text) - 1] != ')');
}




/*
static void compile(char* source) {
	char* start = source;

	if (trace) {
		printf("+ compile [%s]\n", source);
	}

	while (true) {
		while (*source == ' ' || *source == '\t') {
			source++;
			start = source;
		}
		if (*source == 0) {
            if (immediate == EOL_COMMENT) {
                immediate = previous;
            }
            if (immediate == COMPILE_ERROR) {
                immediate = previous;
            }
			return;
		}

		while (*source != 0 && *source != ' ' && *source != '\t') {
			source++;
		}


		char instruction[64];
		int len = source - start;
		strncpy(instruction, start, len);
		instruction[len] = 0;
        to_upper(instruction, len);

		if (trace) {
			printf("+ instruction [%s] %i @ %0x\n", instruction, len, compilation - dictionary);
		}

                  
		if (immediate == TASK) {
            immediate = INTERACTIVE;
            new_task(5, instruction);
            printf("+ new task %s\n", instruction);
          
		} else if (immediate == WORD) {
            immediate = DEFINITION;
            dictionary_add_entry(instruction);
            printf("+ new word %s\n", instruction);
          
		} else if (immediate == COMPILE_ERROR) {
            if (strcmp(instruction, ";") == 0) {
                immediate = INTERACTIVE;
                printf("COMPILE!\n");
            }
          
		} else if (immediate == CONSTANT) {
            immediate = INTERACTIVE;
            add_constant_entry(instruction);
            printf("+ new constant %s\n", instruction);
          
		} else if (immediate == VARIABLE) {
            immediate = INTERACTIVE;
            add_variable_entry(instruction);
            printf("+ new variable %s\n", instruction);
          
		} else if (immediate == TICK) {
            immediate = INTERACTIVE;
            uint16_t word = find_word(instruction);
            main_process->stack[++(main_process->sp)] = word;
            printf("+ xt %s @ %04x onto stack\n", instruction, word);

        } else if (immediate == PAREN_COMMENT && strcmp(instruction, ")") == 0) {
                immediate = previous;

        } else if (immediate != PAREN_COMMENT && immediate != EOL_COMMENT) {
            
            if (strcmp(instruction, "IF") == 0) {
                *compilation++ = ZBRANCH;
                jumps[jp++] = compilation;
                compilation++;

            } else if (strcmp(instruction, "THEN") == 0) {
                uint8_t distance = compilation - jumps[--jp];
                *(jumps[jp]) = distance;

            } else if (strcmp(instruction, "BEGIN") == 0) {
                jumps[jp++] = compilation;

            } else if (strcmp(instruction, "AGAIN") == 0) {
                *compilation++ = BRANCH;
                uint8_t distance = jumps[--jp] - compilation;
                *compilation++ = distance;

            } else if (strcmp(instruction, "UNTIL") == 0) {
                *compilation++ = ZBRANCH;
                uint8_t distance = jumps[--jp] - compilation;
                *compilation++ = distance;

            } else if (strcmp(instruction, "CONSTANT") == 0) {
                immediate = CONSTANT;

            } else if (strcmp(instruction, "VARIABLE") == 0) {
                immediate = VARIABLE;

            } else if (strcmp(instruction, "\\") == 0) {
                previous = immediate;
                immediate = EOL_COMMENT;

            } else if (strcmp(instruction, "(") == 0) {
                // TODO any reason not to have comments in interactive code?
                previous = immediate;
                immediate = PAREN_COMMENT;
                
            } else if (strcmp(instruction, ":") == 0) {
                immediate = WORD;

            } else if (strcmp(instruction, ";") == 0) {
                complete_word();

            } else if (strcmp(instruction, "TASK") == 0) {
                immediate = TASK;

            } else if (strcmp(instruction, "'") == 0) {
                immediate = TICK;

            }
            else if (strcmp(instruction, "HALT") == 0)
            {
                process->ip = CODE_END;
                process->next_time_to_run = 0;

            }
            else if (strcmp(instruction, "TASKS") == 0)
            {
                tasks();

            }
            else if (strcmp(instruction, "`") == 0)
            {
                immediate = RUN_TASK;
            } 
            else 
            {
                uint8_t code = code_for(instruction);                
                if (code != 0Xff) 
                {
                    *compilation++ = code;
                } 
                else 
                {
                    // defined words
                    uint16_t code = find_dictionay_entry(instruction, words); //find_word(instruction);
                    if (code != CODE_END) 
                    {
                        *compilation++ = RUN;
                        *compilation++ = code / 0x100;
                        *compilation++ = code % 0x100;
                        
                    }
                    else
                    {
                        // Processes
                        //printf("not a word\n");
                        struct Process * process = processes;
                        do {
                            //printf("+ check process %s against %s\n", process->name, instruction);
                            if (strcmp(instruction, process->name) == 0) {
                                if (trace) {
                                    printf("+ process %s found\n", process->name);
                                }
                                *compilation++ = PROCESS;
                                *compilation++ = process->id;
                                break;
                            }
                            
                            process = process->next;
                        } while (process != NULL);
                        
                        if (process == NULL) 
                        {
                            code = find_dictionay_entry(instruction, constants);
                            if (code != CODE_END) 
                            {
                                *compilation++ = CONST;
                                *compilation++ = code / 0x100;
                                *compilation++ = code % 0x100;
                                
                            } else {
                                code = find_dictionay_entry(instruction, variables);
                                if (code != CODE_END)
                                {
                                    *compilation++ = VAR;
                                    *compilation++ = code / 0x100;
                                    *compilation++ = code % 0x100;
                                    
                                }
                                else {
                                    bool is_number = true;
                                    int radix = 10;
                                    int i = 0;
                                    // TODO determine based number by looking for 0, 0B or 0X first, then confirm every other char is a digit
                                    if (instruction[i] == '-')
                                    {
                                        i++;
                                    }
                                    if (instruction[i] == '0')
                                    {
                                        i++;
                                        if (instruction[i] == 'X')
                                        {
                                            radix = 16;
                                            i++;
                                        }   
                                        else if (instruction[i] == 'B')
                                        {
                                            radix = 2;
                                            i++;
                                        }   
                                        else if (isdigit(instruction[i]))
                                        {
                                            radix = 8;
                                            i++;
                                        }   
                                        else if (strlen(instruction) > i)
                                        {
                                            is_number = false;
                                        }
                                    }
                                    for (; i < strlen(instruction); ++i) {
                                        if (!isdigit(instruction[i]) && !(radix > 10 && instruction[i] < ('a' + radix - 10))) {
                                            is_number = false;
                                            break;
                                        }
                                    }

                                    if (is_number) {
                                        int64_t value = strtoll(instruction, NULL, 0);
                                        *compilation++ = LIT;
                                        if (trace) {
                                            printf("+ literal = 0x%09llx @ %04x\n", value, compilation - dictionary);
                                        }
                                        *compilation++ = value % 0x100;
                                        *compilation++ = (value / 0x100) % 0x100;
                                        *compilation++ = (value / 0x10000) % 0x100;
                                        *compilation++ = (value / 0x1000000) % 0x100;
                                    } else {
                                        printf("%s!\n", instruction);
                                        previous = immediate;
                                        immediate = COMPILE_ERROR;
                                        return;
                                    }
                                    
                                    
                                }
                                
                            }
                        }
                    }
                }
            }
        }
	}
	source++;
}
*/

static void complete_word() 
{
    if (has_error) {
        dictionary_append_value(RETURN);
        dictionary_end_entry();
    }
    compiling = false;
}


/*
static void add_constant_entry(char *name)
{
    uint8_t len = strlen(name);
    uint8_t data[4];
    data[0] = *(compilation - 4);
    data[1] = *(compilation - 3);
    data[2] = *(compilation - 2);
    data[3] = *(compilation - 1);
    
    compilation = dictionary + next_entry;
    
    uint16_t previous_entry;
    if (next_entry == 0) {
        previous_entry = CODE_END;  // first entry; code to indicate no more previous
    } else {
        previous_entry = constants;
    }
    if (trace) printf("previous_entry %04x %02x_%02x, x%i\n", previous_entry, previous_entry / 0x100,  
            previous_entry % 0x100, len & 0x1f);
    *compilation++ = previous_entry / 0x100;
    *compilation++ = previous_entry % 0x100;
    *compilation++ = len & 0x1f;
   
    strncpy(compilation, name, len & 0x1f);
    compilation += len & 0x1f;
    *compilation++ = data[0];
    *compilation++ = data[1];
    *compilation++ = data[2];
    *compilation++ = data[3];
    
    constants = next_entry;
    next_entry += len + 7;
}

static void add_variable_entry(char *name)
{
    uint8_t len = strlen(name);    
    compilation = dictionary + next_entry;
    
    uint16_t previous_entry;
    if (next_entry == 0) {
        previous_entry = CODE_END;  // first entry; code to indicate no more previous
    } else {
        previous_entry = variables;
    }
    if (trace) printf("previous_entry %0x %x %x, %i\n", previous_entry, previous_entry / 0x100,  
            previous_entry % 0x100, len & 0x1f);
    *compilation++ = previous_entry / 0x100;
    *compilation++ = previous_entry % 0x100;
    *compilation++ = len & 0x1f;
   
    strcpy(compilation, name);
    compilation += len + 4;
  
    variables = next_entry;
    next_entry += len + 7;
}
*/

static void add_literal(uint64_t value)
{
    dictionary_append_instruction(LIT);
    log_trace(LOG, "literal = 0x%09llx", value);
    dictionary_append_value(value);
}



