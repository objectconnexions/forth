#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "forth.h"
#include "code.h"
#include "compiler.h"

//#define MEMORY 4192
//#define WIDTH 2

static void compile(char*);
static uint16_t find_word(char *);
static uint16_t find_dictionay_entry(char *, uint16_t);
static void add_constant_entry(char *);
static void add_variable_entry(char *);
static void add_word_entry(char *);
static void add_literal(uint64_t);
static void complete_word(void);
static void list_dictionary_entries(uint16_t);


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

void compiler_init() {
 	compilation = dictionary;
    compiler_reset();
}

void compiler_reset() {
    words = constants = variables = 0xffff;
    next_entry = 0;
    
    *dictionary = 0xff;
    *(dictionary + 1) = 0xff;
    
    // TODO use same size as in forth.c
    memset(dictionary, 0, 1024 * 8);
    // TODO free memory and reset pointers: constants, variables
}

uint16_t compiler_scratch() {
    return next_entry;
}

bool compiler_compile(char* source) {
    if (trace) {
        printf("# input: %s\n", source);
    }
    if (immediate == INTERACTIVE) {
        compilation = dictionary + next_entry;
    }
    if (trace) {
        printf("# compilation at %04x\n", compilation - dictionary);
    }
    compile(source);
    if (immediate == INTERACTIVE) {
        *compilation++ = END;
        return true;
    } else {
        return false;
    }
}    

void compiler_words() {
    list_dictionary_entries(words);
   /* 
    uint16_t entry;
    uint8_t * code;
    char name[32];

	if (words == 0xffff) {
		printf("No new words\n");
	}
    
    entry = words;
    while (entry != 0xffff) {
        if (trace) printf("@%x -- ", entry);
        code = dictionary + entry;
        entry = *code++ * 0x100 + *code++;
        uint8_t len = *code++ & 0x1f;
        if (trace) printf("%0x - %i  ", entry, len);

        int i;
        for (i = 0; i < len; i++) {
            name[i] = *code++;
        }
        name[i] = 0;
        printf("%s @%04X\n", name, entry + 3 + len);
    }
    * */
}

void compiler_dump() {
    printf("next entry %04x\n", next_entry);
    printf("\nwords %04x\n", words);
    list_dictionary_entries(words);

    printf("\nconstants %04x\n", constants);
    list_dictionary_entries(constants);

    printf("\nvariables %04x\n", variables);
    list_dictionary_entries(variables);
    
    
    
    /*
    uint16_t entry;
    uint8_t * code;
    char name[32];

    
	if (constants == 0xffff) {
		printf("No constants\n");
	} 
    
//        code = dictionary + constants;
        while (entry != 0xffff) {
            //if (trace) 
                printf("@%x -- ", entry);
            code = dictionary + entry;
            entry = *code++ * 0x100 + *code++;
            uint8_t len = *code++ & 0x1f;
            //if (trace) 
                printf("%0x - %i  ", entry, len);
            int i;
            for (i = 0; i < len; i++) {
                name[i] = *code++;
            }
            name[i] = 0;
            printf("%s @%04X\n", name, entry + 3 + len);
        }
    
    
    
    /*
    struct Constant *constant = constants;
    while (constant != NULL) {
        printf("Constant %s: %i\n", 
                constant->name, constant->value);
        constant = constant->next;
    }

    struct Variable *variable = variables;
    while (variable != NULL) {
        printf("Constant %s: %i\n", 
                variable->name, variable->address);
        variable = variable->next;
    }
    */
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


		char instruction[16];
		int len = source - start;
		strncpy(instruction, start, len);
		instruction[len] = 0;
        to_upper(instruction, strlen(instruction));

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
            if (strcmp(instruction, "STACK") == 0) {
                *compilation++ = STACK;

            } else if (strcmp(instruction, "DUP") == 0) {
                *compilation++ = DUP;

            } else if (strcmp(instruction, "SWAP") == 0) {
                *compilation++ = SWAP;

            } else if (strcmp(instruction, "NIP") == 0) {
                *compilation++ = NIP;

            } else if (strcmp(instruction, "TUCK") == 0) {
                *compilation++ = TUCK;

            } else if (strcmp(instruction, "OVER") == 0) {
                *compilation++ = OVER;

            } else if (strcmp(instruction, "ROT") == 0) {
                *compilation++ = ROT;

            } else if (strcmp(instruction, "DROP") == 0) {
                *compilation++ = DROP;

            // TODO should end be allowed as instruction?
            } else if (strcmp(instruction, "END") == 0) {
                *compilation++ = END;

            } else if (strcmp(instruction, "CR") == 0) {
                *compilation++ = CR;

            } else if (strcmp(instruction, "EMIT") == 0) {
                *compilation++ = EMIT;

            } else if (strcmp(instruction, "IF") == 0) {
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

            } else if (strcmp(instruction, "PAUSE") == 0) {
                *compilation++ = YIELD;

            } else if (strcmp(instruction, "HALT") == 0) {
                process->ip = 0xffff;
                process->next_time_to_run = 0;

            } else if (strcmp(instruction, "MS") == 0) {
                *compilation++ = WAIT;

            } else if (strcmp(instruction, "+") == 0) {
                *compilation++ = ADD;

            } else if (strcmp(instruction, "@") == 0) {
                *compilation++ = READ_MEMORY;

            } else if (strcmp(instruction, "!") == 0) {
                *compilation++ = WRITE_MEMORY;

            } else if (strcmp(instruction, "-") == 0) {
                *compilation++ = SUBTRACT;

            } else if (strcmp(instruction, "/") == 0) {
                *compilation++ = DIVIDE;

            } else if (strcmp(instruction, "*") == 0) {
                *compilation++ = MULTIPLY;

            } else if (strcmp(instruction, ">") == 0) {
                *compilation++ = GREATER_THAN;

            } else if (strcmp(instruction, "=") == 0) {
                *compilation++ = EQUAL_TO;

            } else if (strcmp(instruction, "AND") == 0) {
                *compilation++ = AND;

            } else if (strcmp(instruction, "OR") == 0) {
                *compilation++ = OR;

            } else if (strcmp(instruction, "XOR") == 0) {
                *compilation++ = XOR;

            } else if (strcmp(instruction, "LSHIFT") == 0) {
                *compilation++ = LSHIFT;

            } else if (strcmp(instruction, "RSHIFT") == 0) {
                *compilation++ = RSHIFT;

            } else if (strcmp(instruction, ".") == 0) {
                *compilation++ = PRINT_TOS;

            } else if (strcmp(instruction, ".HEX") == 0) {
                *compilation++ = PRINT_HEX;

            } else if (strcmp(instruction, "TICKS") == 0) {
                *compilation++ = TICKS;

            } else if (strcmp(instruction, "WORDS") == 0) {
                *compilation++ = WORDS;

            } else if (strcmp(instruction, "TASKS") == 0) {
                tasks();

            } else if (strcmp(instruction, "`") == 0) {
                immediate = RUN_TASK;

            } else if (strcmp(instruction, "EXECUTE") == 0) {
                *compilation++ = EXECUTE;

            } else if (strcmp(instruction, "INITIATE") == 0) {
                *compilation++ = INITIATE;

            } else if (strcmp(instruction, "_DUMP") == 0) {
                *compilation++ = DUMP;

            } else if (strcmp(instruction, "_TRACE") == 0) {
                *compilation++ = TRACE_ON;

            } else if (strcmp(instruction, "_NOTRACE") == 0) {
                *compilation++ = TRACE_OFF;

            } else if (strcmp(instruction, "_DEBUG") == 0) {
                *compilation++ = DEBUG_ON;

            } else if (strcmp(instruction, "_NODEBUG") == 0) {
                *compilation++ = DEBUG_OFF;

            } else if (strcmp(instruction, "_RESET") == 0) {
                *compilation++ = RESET;

            } else if (strcmp(instruction, "_CLEAR") == 0) {
                *compilation++ = CLEAR_REGISTERS;          

            } else {
                // words
                uint16_t code = find_word(instruction);
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
                                int i;
                                for (i = 0; i < strlen(instruction); ++i) {
                                    if (!isdigit(instruction[i]) && i == 2 && !(instruction[i] == 'X' || instruction[i] == 'B')) {
                                        is_number = false;
                                        break;
                                    }
                                }
                                if (is_number) {
                                    int64_t value = strtoll(instruction, NULL, 0);
                                    *compilation++ = LIT;
                                    if (trace) {
                                        printf("# literal = 0x%09llx @ %0x\n", value, compilation);
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
	source++;
}

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
        if (trace) printf("previous %x, %s\n", entry, name);

        if (strcmp(name, search_name) == 0) {
            if (trace) {
                printf("# word %s found at %04x\n", name, code);
            }
            // the next byte is the start of the code
            return code - dictionary;
        }
    }
    return 0xffff;
}

static uint16_t find_word(char *instruction) 
{
    return find_dictionay_entry(instruction, words);
    
    /*
    uint16_t entry;
    uint8_t *code;
    char name[32];    
    
    entry = words;
    //code = dictionary + words;
    while (entry != 0xffff) {
    //    if (next_entry > code_store) {
   //     do {
        code = dictionary + entry;
        entry = *code++ * 0x100 + *code++;
        uint8_t len = *code++ & 0x1f;
        int i;
        for (i = 0; i < len; i++) {
            name[i] = *code++;
        }
        name[i] = 0;
        if (trace) printf("previous %x, %s\n", entry, name);

        if (strcmp(instruction, name) == 0) {
            if (trace) {
                printf("# word %s found at %04x\n", name, code);
            }
            // the next byte is the start of the code
            return code - dictionary;
        }
            //entry = code_store + previous_entry;
     //   } while (previous_entry != 0xffff);
    }
    return 0xffff;
     */
}

static void add_word_entry(char *name)
{
//    start = source + 1;
//    while (*++source != ' ' && *source != 0) {}
//    int len = source - start;
//    to_upper(start, len);
    int len = strlen(name);
    compilation = dictionary + next_entry;              

    // previous entry
    uint16_t offset;
    if (next_entry == 0) {
        offset = 0xffff;  // first entry; code to indicate no more previous
    } else {
        offset = words;
    }
    if (trace) printf("offset %0x %x %x, %i\n", offset, offset / 0x100,  offset % 0x100, len & 0x1f);
    *compilation++ = offset / 0x100;
    *compilation++ = offset % 0x100;
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
    /*
    uint8_t *data_at = (uint8_t *) (dictionary + next_entry + 3 + len);
    // TODO does this provide enough space?
    // move value to data section
    *data_at++ = *(compilation - 4);
    *data_at++ = *(compilation - 3);
    *data_at++ = *(compilation - 2);
    *data_at++ = *(compilation - 1);
    *data_at = END;
    */
    
    uint8_t data[4];
    data[0] = *(compilation - 4);
    data[1] = *(compilation - 3);
    data[2] = *(compilation - 2);
    data[3] = *(compilation - 1);
    
    compilation = dictionary + next_entry;
    
    uint16_t offset;
    if (next_entry == 0) {
        offset = 0xffff;  // first entry; code to indicate no more previous
    } else {
        offset = constants;
    }
    //if (trace) 
        printf("offset %0x %x %x, %i\n", offset, offset / 0x100,  offset % 0x100, len & 0x1f);
    *compilation++ = offset / 0x100;
    *compilation++ = offset % 0x100;
    *compilation++ = len & 0x1f;
   
    strcpy(compilation, name);
    compilation += len;
    
    /*  this does work here because the value is not yet on the stack - until the code is run!
    uint32_t value = process->stack[process->sp--];
    printf("constant value %x\n", value);
    *compilation++ = value % 0x100;
    *compilation++ = (value / 0x100) % 0x100;
    *compilation++ = (value / 0x10000) % 0x100;
    *compilation++ = (value / 0x1000000) % 0x100;
     */
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
    
    uint16_t offset;
    if (next_entry == 0) {
        offset = 0xffff;  // first entry; code to indicate no more previous
    } else {
        offset = variables;
    }
    //if (trace) 
        printf("offset %0x %x %x, %i\n", offset, offset / 0x100,  offset % 0x100, len & 0x1f);
    *compilation++ = offset / 0x100;
    *compilation++ = offset % 0x100;
    *compilation++ = len & 0x1f;
   
    strcpy(compilation, name);
    compilation += len + 4;
  
    variables = next_entry;
    next_entry += len + 7;



    /*
    struct Variable *variable = malloc(sizeof(struct Variable));
    variable->name = malloc(sizeof(struct Constant));
    strcpy(variable->name, name);
    variable->address = (uint32_t) malloc(sizeof(uint32_t));

    if (trace) {
        printf("# variable %s at %04p\n", variable->name, variable->address);
    }

    if (variables == NULL) {
        variables = variable;
    } else {
        variable->next = variables;
        variables = variable;
    }
     * */
}

static void add_literal(uint64_t value)
{
    *compilation++ = LIT;
    if (trace) {
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
    while (entry != 0xffff) {
        if (trace) printf("@%x -- ", entry);
        code = dictionary + entry;
        entry = *code++ * 0x100 + *code++;
        uint8_t len = *code++ & 0x1f;
        if (trace) printf("%0x - %i  ", entry, len);

        int i;
        for (i = 0; i < len; i++) {
            name[i] = *code++;
        }
        name[i] = 0;
        printf("%s @%04X\n", name, code - dictionary);
    }
}
