#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "forth.h"
#include "code.h"
#include "compiler.h"

static void compile(char*);
static uint16_t find_word(char *);
static uint16_t find_dictionay_entry(char *, uint16_t);
static void add_constant_entry(char *);
static void add_variable_entry(char *);
static void add_word_entry(char *);
static void add_literal(uint64_t);
static void complete_word(void);
static void list_dictionary_entries(uint16_t);
static uint8_t code_for(char * instruction);

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

static uint16_t words; // last non-scratch entry in dictionary
static uint16_t constants;
static uint16_t variables;
static uint16_t next_entry;  // next free space in dictionary for a new entry

static uint8_t* compilation;
static uint8_t *jumps[6];
static uint8_t jp = 0;
static uint8_t immediate = INTERACTIVE;
static uint8_t previous = INTERACTIVE;

static uint8_t ZBRANCH, BRANCH, RETURN, PROCESS, CONST, VAR, LIT, RUN, END, PRINT_TEXT;

void compiler_init()
{
 	compilation = dictionary;
    compiler_reset();
    
    ZBRANCH = code_for("__ZBRANCH");
    BRANCH = code_for("__BRANCH");
    PROCESS = code_for("__PROCESS");
    VAR = code_for("__VAR");
    CONST = code_for("__CONSTANT");
    LIT = code_for("__LIT");
    RUN = code_for("__RUN");
    END = code_for("END");
    RETURN = code_for("__RETURN");
    PRINT_TEXT = code_for(".\"");
}

void compiler_reset()
{
    words = constants = variables = 0xffff;
    next_entry = 0;
    
    // TODO use same size as in forth.c
    memset(dictionary, 0, 1024 * 8);   
    // TODO free memory and reset pointers: constants, variables
}

static uint8_t code_for(char * instruction)
{
    int i;
    for (i = 0; i < WORD_COUNT; i++) {
        if (core_words[i].word != NULL && strcmp(instruction, core_words[i].word) == 0) {
            return i;
        }
    }
    return 0xff;
}

uint16_t compiler_scratch() 
{
    return next_entry;
}

bool compiler_compile(char* source)
{
    uint8_t * start;
    
    if (trace) {
        printf("# input: %s\n", source);
    }
    if (immediate == INTERACTIVE) {
        start = dictionary + next_entry;
        compilation = start;
    }
    if (trace) {
        printf("# compilation at %04x\n", compilation - dictionary);
    }
    compile(source);
    if (immediate == INTERACTIVE) {
        *compilation++ = END;
        if (trace) memory_dump(next_entry, compilation - dictionary - next_entry - 1) ; //start - dictionary, compilation - start);        
        return true;
    } else {
        return false;
    }
}    

void compiler_words() {
    int i;
    for (i = 0; i < WORD_COUNT; i++) {
        printf("%s (%02X)\n", core_words[i].word, i);
    }
    printf("\n");
    
    list_dictionary_entries(words);
}

void compiler_dump() {
    printf("next entry @%04x\n", next_entry);
    printf("words %04x\n", words);
    list_dictionary_entries(words);

    printf("\nconstants @%04x\n", constants);
    list_dictionary_entries(constants);

    printf("\nvariables @%04x\n", variables);
    list_dictionary_entries(variables);
 }

static void compile(char* source) {
	char* start = source;

	if (trace) {
		printf("# compile [%s]\n", source);
	}

	while (true) {
		while (*source == ' ' || *source == '\t') {
			source++;
			start = source;
		}
		if (*source == 0 /* || *source == '\n' */) {
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
			printf("# instruction [%s] %i @ %0x\n", instruction, len, compilation - dictionary);
		}

                  
		if (immediate == TASK) {
            immediate = INTERACTIVE;
            new_task(5, instruction);
            printf("# new task %s\n", instruction);
          
		} else if (immediate == WORD) {
            immediate = DEFINITION;
            add_word_entry(instruction);
            printf("# new word %s\n", instruction);
          
		} else if (immediate == COMPILE_ERROR) {
            if (strcmp(instruction, ";") == 0) {
                immediate = INTERACTIVE;
                printf("COMPILE!\n");
            }
          
		} else if (immediate == CONSTANT) {
            immediate = INTERACTIVE;
            add_constant_entry(instruction);
            printf("# new constant %s\n", instruction);
          
		} else if (immediate == VARIABLE) {
            immediate = INTERACTIVE;
            add_variable_entry(instruction);
            printf("# new variable %s\n", instruction);
          
		} else if (immediate == TICK) {
            immediate = INTERACTIVE;
            uint16_t word = find_word(instruction);
            main_process->stack[++(main_process->sp)] = word;
            printf("# xt %s @ %04x onto stack\n", instruction, word);

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
                /* if (immediate != WORD) {
                    printf("Error: invalid place for comment\n");
                    *compilation++ = END;  
                    immediate = INTERACTIVE;
                } else {
                    immediate = PAREN_COMMENT;
                } */
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
                            //printf("# check process %s against %s\n", process->name, instruction);
                            if (strcmp(instruction, process->name) == 0) {
                                if (trace) {
                                    printf("# process %s found\n", process->name);
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
                                            printf("# literal = 0x%09llx @ %04x\n", value, compilation - dictionary);
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

/*
 Returns the address of the code for for the specified word.
 */
static uint16_t find_dictionay_entry(char *search_name, uint16_t start) 
{
    uint16_t entry;
    uint8_t *code;
    char name[32];    
    
    entry = start;
    while (entry != 0xffff) {
        code = dictionary + entry;
        entry = *code++ * 0x100 + *code++;
        uint8_t len = *code++ & 0x1f;
        int i;
        for (i = 0; i < len; i++) {
            name[i] = *code++;
        }
        name[i] = 0;
        // if (trace) printf("previous %04x, %s\n", entry, name);

        if (strcmp(name, search_name) == 0) {
            if (trace) {
                printf("# entry %s found at %04x\n", name, code - dictionary);
            }
            // the next byte is the start of the code
            return code - dictionary;
        }
    }
    return 0xffff;
}

/*
 Return the address of the dictionary entry for the code at the specified address
 */
void find_word_for(uint16_t offset, char *name) {
    uint16_t entry;
    uint8_t *code;

    strcpy(name, "Unknown!");    
    
    entry = words;
    while (entry != 0xffff) {
        code = dictionary + entry;
        entry = *code++ * 0x100 + *code++;
        uint8_t len = *code++ & 0x1f;
        if (dictionary + offset == code + len) {
            strncpy(name, code, len);
            name[len] = 0;
            break;
        }
    }
}

static uint16_t find_word(char *instruction) 
{
    return find_dictionay_entry(instruction, words);
}

static void add_word_entry(char *name)
{
    int len = strlen(name);
    compilation = dictionary + next_entry;              

    // previous entry
    uint16_t previous_entry;
    if (next_entry == 0) {
        previous_entry = 0xffff;  // first entry; code to indicate no more previous
    } else {
        previous_entry = words;
    }
    if (trace) printf("previous_entry %0x %x %x, %i\n", previous_entry, previous_entry / 0x100,  previous_entry % 0x100, len & 0x1f);
    *compilation++ = previous_entry / 0x100;
    *compilation++ = previous_entry % 0x100;
    *compilation++ = len & 0x1f;

    strcpy(compilation, name);
 //   *(compilation + len) = 0;
    compilation += len;
    if (trace) {
        printf("# compiling word, code at %04X\n", compilation - dictionary);
    }
}

static void complete_word() 
{
    *compilation++ = RETURN;
    *compilation = END; // TODO check this stop the runner from failing with instruction 0;
    words = next_entry;
    next_entry = compilation - dictionary;
    immediate = INTERACTIVE;
}

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

static void add_literal(uint64_t value)
{
    *compilation++ = LIT;
    if (trace)
    {
        printf("# literal = 0x%09llx @ %0x\n", value, compilation);
    }
    *compilation++ = value % 0x100;
    *compilation++ = (value / 0x100) % 0x100;
    *compilation++ = (value / 0x10000) % 0x100;
    *compilation++ = (value / 0x1000000) % 0x100;
}

/*
 * List the entries in the dictionary starting at index position
 */
static void list_dictionary_entries(uint16_t index)
{
    uint16_t entry;
    uint8_t * code;
    char name[32];

	if (index == 0xffff) {
		printf("No entries\n");
	}
    
    entry = index;
    while (entry != 0xffff)
    {
        printf("@%04x ", entry);
        code = dictionary + entry;
        entry = *code++ * 0x100 + *code++;
        uint8_t len = *code++ & 0x1f;
        if (trace) printf("[%0x - %i]  ", entry, len);

        int i;
        for (i = 0; i < len; i++) {
            name[i] = *code++;
        }
        name[i] = 0;
        printf("%s => @%04x\n", name, code - dictionary);
    }
}
