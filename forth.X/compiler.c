#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "forth.h"
#include "code.h"



//uint8_t* code_store;
uint8_t* compiled_code;
//uint8_t* code;
uint8_t* scratchpad_code;


uint8_t *jumps[6];
uint8_t jp = 0;
// uint8_t* define_start;

bool immediate = false;



void code_init() {
   // TODO should compiler use SCRATCHPAD directly?
 	scratchpad_code = code_store;
	compiled_code = code_store + SCRATCHPAD;
}

void compiler_reset() {
    compiled_code = code_store + SCRATCHPAD;
}

void words() {
    struct Dictionary * ptr = dictionary;
    while (ptr != NULL) {
        printf("%s -> %04x\n", ptr->name, ptr->code + SCRATCHPAD);
        ptr = ptr->previous;
    }
}

void compile_stuff(char* source) {
	char* start = source;

	if (trace) {
		printf("compile [%s]\n", source);
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
		/*
			if (c == 0) {
				source++;
				continue;
			}
		 */
		strncpy(instruction, start, len);
		instruction[len] = 0;

        to_upper(instruction, strlen(instruction));

		if (trace) {
			printf("instruction [%s] %i @ %0x\n", instruction, len, process->code);
		}

		if (strcmp(instruction, "STACK") == 0) {
			*process->code++ = STACK;
            
		} else if (strcmp(instruction, "DUP") == 0) {
			*process->code++ = DUP;
            
		} else if (strcmp(instruction, "SWAP") == 0) {
			*process->code++ = SWAP;
            
		} else if (strcmp(instruction, "DROP") == 0) {
			*process->code++ = DROP;
            
        // TODO should end be allowed as instruction?
		} else if(strcmp(instruction, "END") == 0) {
			*process->code++ = END;
            
		} else if(strcmp(instruction, "CR") == 0) {
			*process->code++ = CR;

		} else if(strcmp(instruction, "EMIT") == 0) {
			*process->code++ = EMIT;
            
        } else if(strcmp(instruction, "IF") == 0) {
			*process->code++ = ZBRANCH;
			jumps[jp++] = process->code;
			process->code++;
            
		} else if (strcmp(instruction, "THEN") == 0) {
			uint8_t distance = process->code - jumps[--jp];
			*(jumps[jp]) = distance;
            
		} else if(strcmp(instruction, "BEGIN") == 0) {
			jumps[jp++] = process->code;
            
		} else if (strcmp(instruction, "UNTIL") == 0) {
			*process->code++ = ZBRANCH;
			uint8_t distance = jumps[--jp] - process->code;
			*process->code++ = distance;
            
		} else if(strcmp(instruction, ":") == 0) {
			start = source + 1;
			while (*++source != ' ' && *source != 0) {}
			int len = source - start;
			struct Dictionary* entry = malloc(sizeof(struct Dictionary));
			entry->code = compiled_code - (code_store + SCRATCHPAD);
			entry->name = malloc(len + 1);
			entry->previous = dictionary;
            to_upper(start, len);
			strncpy(entry->name, start, len);
			dictionary = entry;
			scratchpad_code = process->code;
			process->code = compiled_code;
            
            immediate = true;
            
		} else if(strcmp(instruction, ";") == 0) {
			*process->code++ = RETURN;
			compiled_code = process->code;
			process->code = scratchpad_code;
            
            immediate = false;
/*
		} else if(strcmp(instruction, "task") == 0) {
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
			scratchpad_code = code;
			code = compiled_code;
            
            immediate = true;
 */           
		} else if(strcmp(instruction, "TASK") == 0) {
            new_task(5, "new-task");
             
		} else if(strcmp(instruction, "PAUSE") == 0) {
			*process->code++ = YIELD;
                       
		} else if(strcmp(instruction, "MS") == 0) {
			*process->code++ = WAIT;
                       
		} else if(strcmp(instruction, "+") == 0) {
			*process->code++ = ADD;
                       
		} else if(strcmp(instruction, "@") == 0) {
			*process->code++ = READ_MEMORY;
           
		} else if(strcmp(instruction, "!") == 0) {
			*process->code++ = WRITE_MEMORY;

		} else if(strcmp(instruction, "-") == 0) {
			*process->code++ = SUBTRACT;
            
		} else if(strcmp(instruction, "/") == 0) {
			*process->code++ = DIVIDE;
            
		} else if(strcmp(instruction, "*") == 0) {
			*process->code++ = MULTIPLY;
            
		} else if(strcmp(instruction, ">") == 0) {
			*process->code++ = GREATER_THAN;
            
		} else if(strcmp(instruction, "=") == 0) {
			*process->code++ = EQUAL_TO;
            
        } else if(strcmp(instruction, "AND") == 0) {
			*process->code++ = AND;
            
        } else if(strcmp(instruction, "OR") == 0) {
			*process->code++ = OR;
            
        } else if(strcmp(instruction, "XOR") == 0) {
			*process->code++ = XOR;
            
        } else if(strcmp(instruction, "LSHIFT") == 0) {
			*process->code++ = LSHIFT;
            
        } else if(strcmp(instruction, "RSHIFT") == 0) {
			*process->code++ = RSHIFT;
            
		} else if(strcmp(instruction, ".") == 0) {
			*process->code++ = PRINT_TOS;

		} else if(strcmp(instruction, ".HEX") == 0) {
			*process->code++ = PRINT_HEX;

		} else if(strcmp(instruction, "TICKS") == 0) {
			*process->code++ = TICKS;

		} else if(strcmp(instruction, "WORDS") == 0) {
            *process->code++ = WORDS;
            
		} else if(strcmp(instruction, "TASKS") == 0) {
            tasks();
            
		} else if(strcmp(instruction, "_DEBUG") == 0) {
            *process->code++ = DEBUG;

		} else if(strcmp(instruction, "_TRACE") == 0) {
            *process->code++ = TRACE_ON;

		} else if(strcmp(instruction, "_NOTRACE") == 0) {
            *process->code++ = TRACE_OFF;

		} else if(strcmp(instruction, "_RESET") == 0) {
            *process->code++ = RESET;

		} else if(strcmp(instruction, "_CLEAR") == 0) {
            *process->code++ = CLEAR_REGISTERS;          
            
		} else {
           // printf("not built in instruction\n");
           // strncpy(instruction, start, len);
           // instruction[len] = 0;        
			struct Dictionary * ptr = dictionary;
			while (ptr != NULL) {
				if (strcmp(instruction, ptr->name) == 0) {
					if (trace) {
						printf("# run %s at %i\n", ptr->name, ptr->code + SCRATCHPAD);
					}
					*process->code++ = RUN;
					*process->code++ = ptr->code;
					break;
				}
				ptr = ptr->previous;
			}

			if (ptr == NULL) {
                // printf("not word\n");
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
					*process->code++ = LIT;
                    if (trace) {
                        printf("# literal = 0x%09llx @ %0x\n", value, process->code);
                    }
					*process->code++ = value % 0x100;
					*process->code++ = (value / 0x100) % 0x100;
					*process->code++ = (value / 0x10000) % 0x100;
					*process->code++ = (value / 0x1000000) % 0x100;
				} else {
                    printf("# instruction not understood\n");
                    return;
                }
			}
		}
	}
	source++;
}

bool compile(char* source) {
    if (trace) {
        printf("# compile %s\n", source);
    }

    if (trace) {
        printf("# code at %p\n", code_store);
    }
    if (!immediate) {
        memset(code_store, 0, SCRATCHPAD);
        scratchpad_code = code_store;
    }
    if (trace) {
        printf("# scratch at %p\n", scratchpad_code);
    }

    compile_stuff(source);
    if (!immediate) {
        *process->code++ = END;
        process->ip = 0;
        return true;
    } else {
        return false;
    }
}    
    

