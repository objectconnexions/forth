#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "forth.h"
#include "code.h"
#include "parser.h"
#include "dictionary.h"
#include "compiler.h"


static void add_literal(uint64_t);
static void complete_word(void);

/*
enum immediate {
    INTERACTIVE,
    EOL_COMMENT,
    PAREN_COMMENT,
    WORD,
    DEFINITION,
    COMPILE_ERROR,
    CONSTANT,
    VARIABLE,
    TASK,
    TICK,
    RUN_TASK
};
 * */
/*
static uint16_t constants;
static uint16_t variables;

static uint8_t immediate = INTERACTIVE;
static uint8_t previous = INTERACTIVE;
 */

static CODE_INDEX jumps[6];
static uint8_t jp = 0;

//static uint8_t ZBRANCH, BRANCH, RETURN, PROCESS, CONST, VAR, LIT, RUN, END, PRINT_TEXT;

void compiler_init()
{
     
//    ZBRANCH = code_for("__ZBRANCH");
//    BRANCH = code_for("__BRANCH");
//    PROCESS = code_for("__PROCESS");
//    VAR = code_for("__VAR");
//    CONST = code_for("__CONSTANT");
//    LIT = code_for("__LIT");
//    RUN = code_for("__RUN");
//    END = code_for("END");
//    RETURN = code_for("__RETURN");
//    PRINT_TEXT = code_for(".\"");
}

void compiler_compile_new()
{
    char name[32];
    
    if (trace) printf("+ compiling new word\n");
    
    if (parser_next_token() != INVALID_INSTRUCTION) {
        parser_drop_line();
        parser_token_text(name);
        printf("!compile failed on %s\n", name);
    }
    parser_token_text(name);
    printf("# new word %s\n", name);
    dictionary_add_entry(name);
    
    while (true) 
    {
        switch (parser_next_token())
        {
            case NUMBER_AVAILABLE:
                add_literal(parser_token_number());
                break;
            case WORD_AVAILABLE:
                // TODO check for immediate!
                
                parser_token_text(name);
                printf("+ append word %s\n", name);
                if (strcmp(name, ";") == 0) 
                {
                    complete_word();
                    return;
                }
                
                uint32_t word = parser_token_word();
                dictionary_append_value(word);
                break;
            case INVALID_INSTRUCTION:
                parser_drop_line();
                return;
            case END_LINE:
                return;
        }
    }
}


void compiler_constant()
{
    char name[32];
    
    if (parser_next_token() != INVALID_INSTRUCTION) {
        parser_drop_line();
        parser_token_text(name);
        printf("!constant failed on %s\n", name);
    }
    
    parser_token_text(name);
    dictionary_add_entry(name);
    
    dictionary_append_value(LIT);
    uint32_t value = pop_stack();
    dictionary_append_value(value);
}


void compiler_variable()
{
    char name[32];
    
    if (parser_next_token() != INVALID_INSTRUCTION) {
        parser_drop_line();
        parser_token_text(name);
        printf("!variable failed on %s\n", name);
    }
    
    parser_token_text(name);
    dictionary_add_entry(name);
    
    dictionary_append_value(LIT);
//    dictionary_append_value_address(1);
    dictionary_append_value(RETURN);

    dictionary_append_byte(0); // space for value
    dictionary_append_byte(0); // space for value
    dictionary_append_byte(0); // space for value
    dictionary_append_byte(0); // space for value
}

void compiler_if()
{
    dictionary_append_value(ZBRANCH);
    jumps[jp++] = dictionary_offset();  // compilation;
    dictionary_append_value(0);  
}

void compiler_begin()
{
    jumps[jp++] = dictionary_offset();
}
    
void compiler_then()
{
    uint8_t distance = dictionary_offset() - jumps[--jp];
    dictionary_write_byte(jumps[jp], distance);
}

void compiler_again()
{
    dictionary_append_value(BRANCH);
    uint8_t distance = jumps[--jp] - dictionary_offset();
    dictionary_append_byte(distance);
}    

void compiler_until()
{
    dictionary_append_value(ZBRANCH);
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
                process->ip = 0xffff;
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
                    if (code != 0xffff) 
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
                            if (code != 0xffff) 
                            {
                                *compilation++ = CONST;
                                *compilation++ = code / 0x100;
                                *compilation++ = code % 0x100;
                                
                            } else {
                                code = find_dictionay_entry(instruction, variables);
                                if (code != 0xffff)
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
    dictionary_append_value(RETURN);
    dictionary_append_value(END); // TODO check this stop the runner from failing with instruction 0;
    dictionary_end_entry();
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
        previous_entry = 0xffff;  // first entry; code to indicate no more previous
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
        previous_entry = 0xffff;  // first entry; code to indicate no more previous
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
    dictionary_append_value(LIT);
    if (trace)
    {
        printf("+ literal = 0x%09llx\n", value);
    }
    dictionary_append_value(value);
}



