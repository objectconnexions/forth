#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "forth.h"
#include "code.h"

static void compile(char*);
static uint8_t* find_word(char *);
static void new_constant(char *);
static void new_variable(char *);

struct Constant {
    char *name;
    uint32_t value;
    struct Constant *next;
};

struct Variable {
    char *name;
    uint32_t address;
    struct Variable *next;
};

enum immediate {
    INTERACTIVE,
    EOL_COMMENT,
    PAREN_COMMENT,
    WORD,
    CONSTANT,
    VARIABLE,
    TASK,
    TICK,
    RUN_TASK
};

static uint8_t* compiled_code;
static uint8_t* compilation;
static uint8_t *jumps[6];
static uint8_t jp = 0;
static uint8_t immediate = INTERACTIVE;
static uint8_t previous = INTERACTIVE;

struct Constant *constants;
struct Variable *variables;


void compiler_init() {
   // TODO should compiler use SCRATCHPAD directly?
 	compilation = code_store;
	compiled_code = code_store + SCRATCHPAD;
}

void compiler_reset() {
    compiled_code = code_store + SCRATCHPAD;
    
    // TODO free memory and reset pointers: constants, variables
}

bool compiler_compile(char* source) {
    if (trace) {
        printf("# input: %s\n", source);
    }
    if (immediate == INTERACTIVE) {
        memset(code_store, 0, SCRATCHPAD);
        compilation = code_store;
    }
    if (trace) {
        printf("# compilation at %04x\n", compilation - code_store);
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
    struct Dictionary * entry = dictionary;
    while (entry != NULL) {
        printf("%s @%04X\n", entry->name, entry->code + SCRATCHPAD);
        entry = entry->previous;
    }
}

void compiler_dump() {
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
			printf("# instruction [%s] %i @ %0x\n", instruction, len, compilation - code_store);
		}

                  
		if (immediate == TASK) {
            immediate = INTERACTIVE;
            new_task(5, instruction);
            printf("# new task %s\n", instruction);
          
		} else if (immediate == CONSTANT) {
            immediate = INTERACTIVE;
            new_constant(instruction);
            printf("# new constant %s\n", instruction);
          
		} else if (immediate == VARIABLE) {
            immediate = INTERACTIVE;
            new_variable(instruction);
            printf("# new variable %s\n", instruction);
          
		} else if (immediate == TICK) {
            immediate = INTERACTIVE;
            uint32_t word = (uint32_t) find_word(instruction);
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
                start = source + 1;
                while (*++source != ' ' && *source != 0) {}
                int len = source - start;
                struct Dictionary* entry = malloc(sizeof(struct Dictionary));
                entry->code = (uint8_t *) (compiled_code - (code_store + SCRATCHPAD));
                entry->name = malloc(len + 1);
                to_upper(start, len);
                strncpy(entry->name, start, len);
                entry->previous = dictionary;
                dictionary = entry;
                compilation = compiled_code;
                if (trace) {
                    printf("# compilng word at %04X\n", compilation - code_store);
                }
                immediate = WORD;

            } else if (strcmp(instruction, ";") == 0) {
                *compilation++ = RETURN;
                compiled_code = compilation;
                compilation = code_store;
                immediate = INTERACTIVE;

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

            } else if (strcmp(instruction, "_NOTDEBUG") == 0) {
                *compilation++ = DEBUG_OFF;

            } else if (strcmp(instruction, "_RESET") == 0) {
                *compilation++ = RESET;

            } else if (strcmp(instruction, "_CLEAR") == 0) {
                *compilation++ = CLEAR_REGISTERS;          

            } else {
                // words
                uint8_t* code = find_word(instruction);
                if (code != NULL) {
                    *compilation++ = RUN;
                    *compilation++ = code;
                    
                } else {
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
                    
                  
                    // Constants
                    struct Constant *constant = constants;
                    if (process == NULL && constant != NULL) {
                        do {
                            if (strcmp(constant->name, instruction) == 0) {
                                *compilation++ = LIT;
                                *compilation++ = constant->value % 0x100;
                                *compilation++ = (constant->value / 0x100) % 0x100;
                                *compilation++ = (constant->value / 0x10000) % 0x100;
                                *compilation++ = (constant->value / 0x1000000) % 0x100;
                                break;
                            }
                            constant = constant->next;
                        } while (constant != NULL);
                    }
                  
                    // Variables
                    struct Variable *variable = variables;
                    if (constant == NULL && variable != NULL) {
                        do {
                            if (strcmp(variable->name, instruction) == 0) {
                                *compilation++ = LIT;
                                *compilation++ = variable->address % 0x100;
                                *compilation++ = (variable->address / 0x100) % 0x100;
                                *compilation++ = (variable->address / 0x10000) % 0x100;
                                *compilation++ = (variable->address / 0x1000000) % 0x100;
                                break;
                            }
                            variable = variable->next;
                        } while (variable != NULL);
                    }
                    
                    if (constant == NULL && variable == NULL) {
                        // numbers/literals
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
                            printf("# %s not understood\n", instruction);
                            return;
                        }
                    }
                }
            }
        }
	}
	source++;
}

static uint8_t* find_word(char *instruction) {
    struct Dictionary * entry = dictionary;
    while (entry != NULL) {
        if (strcmp(instruction, entry->name) == 0) {
            if (trace) {
                printf("# word %s found at %04x\n", entry->name, entry->code + SCRATCHPAD);
            }
            return entry->code + SCRATCHPAD;
        }
        entry = entry->previous;
    }
    return NULL;
}

static void new_constant(char *name) {
    struct Constant *constant = malloc(sizeof(struct Constant));
    constant->name = malloc(sizeof(struct Constant));
    strcpy(constant->name, name);
    compilation--;
    constant->value = (*compilation-- * 0x1000000) +
                    (*compilation-- * 0x10000) +
                    (*compilation-- * 0x100) +
                    *compilation--;
    
    if (constants == NULL) {
        constants = constant;
    } else {
        constant->next = constants;
        constants = constant;
    }
}

static void new_variable(char *name) {
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
}
