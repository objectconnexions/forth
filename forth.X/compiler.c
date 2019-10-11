#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "forth.h"
#include "code.h"

static uint8_t* find_word(char *);

enum immediate {
    INTERACTIVE,
    WORD,
    TASK,
    TICK,
    RUN_TASK
};


uint8_t* compiled_code;
uint8_t* compilation;

uint8_t *jumps[6];
uint8_t jp = 0;

uint8_t immediate = INTERACTIVE;



void compiler_init() {
   // TODO should compiler use SCRATCHPAD directly?
 	compilation = code_store;
	compiled_code = code_store + SCRATCHPAD;
}

void compiler_reset() {
    compiled_code = code_store + SCRATCHPAD;
}

void compiler_words() {
    struct Dictionary * ptr = dictionary;
    while (ptr != NULL) {
        printf("%s @%04X\n", ptr->name, ptr->code + SCRATCHPAD);
        ptr = ptr->previous;
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
          
		} else if (immediate == TICK) {
            immediate = INTERACTIVE;
            uint32_t word = (uint32_t) find_word(instruction);
            process->stack[++(process->sp)] = word;
            printf("# xt %s @ %04x onto stack\n", instruction, word);
            
        } else {
            if (strcmp(instruction, "STACK") == 0) {
                *compilation++ = STACK;

            } else if (strcmp(instruction, "DUP") == 0) {
                *compilation++ = DUP;

            } else if (strcmp(instruction, "SWAP") == 0) {
                *compilation++ = SWAP;

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

            } else if (strcmp(instruction, "UNTIL") == 0) {
                *compilation++ = ZBRANCH;
                uint8_t distance = jumps[--jp] - compilation;
                *compilation++ = distance;

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
                    printf("# compilation moved to %i\n", compilation - code_store);
                }


                immediate = WORD;

            } else if (strcmp(instruction, ";") == 0) {
                *compilation++ = RETURN;

                compiled_code = compilation;
        //		process->code = compilation;

                compilation = code_store;

                immediate = INTERACTIVE;

                /*
            } else if (strcmp(instruction, "task") == 0) {
                start = source + 1;
                while (*++source != ' ' && *source != 0) {}
                int len = source - start;

                struct Task entry; // = malloc(sizeof(struct Task));
    //			entry->code = compiled_code - (code_store + SCRATCHPAD);
    //			entry->name = malloc(len + 1);
    //			entry->previous = dictionary;
    //            to_upper(start, len);
    //			strncpy(entry->name, start, len);
                tasks[task_count++] = entry;
                compilation = code;
                code = compiled_code;

                immediate = true;
     */           
            } else if (strcmp(instruction, "TASK") == 0) {
                immediate = TASK;

            } else if (strcmp(instruction, "'") == 0) {
                immediate = TICK;

            } else if (strcmp(instruction, "PAUSE") == 0) {
                *compilation++ = YIELD;

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

            } else if (strcmp(instruction, "_DEBUG") == 0) {
                *compilation++ = DEBUG;

            } else if (strcmp(instruction, "_TRACE") == 0) {
                *compilation++ = TRACE_ON;

            } else if (strcmp(instruction, "_NOTRACE") == 0) {
                *compilation++ = TRACE_OFF;

            } else if (strcmp(instruction, "_RESET") == 0) {
                *compilation++ = RESET;

            } else if (strcmp(instruction, "_CLEAR") == 0) {
                *compilation++ = CLEAR_REGISTERS;          

            } else {
                uint8_t* code = find_word(instruction);
                if (code != NULL) {
                    *compilation++ = RUN;
                    *compilation++ = code;
                    
                } else {
                    //printf("not a word\n");
                    struct Process * ptr = processes;
                    do {
                        //printf("# check process %s against %s\n", ptr->name, instruction);
                        if (strcmp(instruction, ptr->name) == 0) {
                            if (trace) {
                                printf("# process %s found\n", ptr->name);
                            }
                            *compilation++ = PROCESS;
                            *compilation++ = ptr->id;
                            break;
                        }
                        
                        ptr = ptr->next;
                      //  ptr = NULL;
                    } while (ptr != NULL);

                    if (ptr == NULL) {
                        //printf("not a process\n");
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
                            printf("# instruction not understood\n");
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
    struct Dictionary * ptr = dictionary;
    while (ptr != NULL) {
        if (strcmp(instruction, ptr->name) == 0) {
            if (trace) {
                printf("# word %s found at %04x\n", ptr->name, ptr->code + SCRATCHPAD);
            }
            return ptr->code + SCRATCHPAD;
        }
        ptr = ptr->previous;
    }
    return NULL;
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
        process->ip = 0;
        return true;
    } else {
        return false;
    }
}    
    

