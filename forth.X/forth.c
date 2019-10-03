#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include "forth.h"


#define LIT 0x1
#define DUP 0x2
#define SWAP 0x3
#define DROP 0x4

#define ADD 0x10
#define SUBTRACT 0x11
#define DIVIDE 0x12
#define MULTIPLY 0x13
#define GREATER_THAN 0x15
#define EQUAL_TO 0x16
#define AND 0x17
#define OR 0x18
#define XOR 0x19
#define LSHIFT 0x1a
#define RSHIFT 0x1b

#define PRINT_TOS 0xc8
#define PRINT_HEX 0xc9 
#define CR 0xca
#define EMIT 0xcb

#define READ_MEMORY 0xc0
#define WRITE_MEMORY 0xc1

#define RUN 0xe0
#define RETURN 0xe1
#define ZBRANCH 0xe2
#define BRANCH 0xe3

#define YIELD 0xe4
#define WAIT 0xe5

#define WORDS 0xf0
#define STACK 0xf1
#define DEBUG 0xf2
#define TRACE_ON 0xf3
#define TRACE_OFF 0xf4
#define RESET 0xf5
#define CLEAR_REGISTERS 0xf6
#define TICKS 0xf7
#define END 0xff


#define SCRATCHPAD 64

uint32_t timer = 0;

struct Dictionary {
	struct Dictionary* previous;
	char* name;
	//uint16_t code;
    uint8_t *code;
};

uint8_t code_store[2048];

//uint8_t* code_store;
uint8_t* compiled_code;
//uint8_t* code;
uint8_t* scratchpad_code;

struct Process {
    uint32_t stack[16];
    uint32_t return_stack[8];
    //int16_t* stack;
    //uint16_t* return_stack;
    uint16_t ip; // instruction pointer
    //TODO change both to unsigned
    int sp; // stack pointer
    int rsp; // return stack pointer
    
    struct Process* next;
    char *name;
    uint8_t priority;
    uint32_t next_time_to_run;
    uint8_t* code;
};

struct Process* processes;
struct Process* process;

struct Dictionary* dictionary;

bool immediate = false;
bool trace = true;
bool waiting = true;


void dump_stack(struct Process*);
void compile(char*, uint8_t*);
void execute(uint8_t*);
void display_code(uint8_t*);
void debug(void);
void tasks(void);
uint32_t popParameterStack(void);
void wait(uint32_t);
void next_task();
struct Process* new_task(uint8_t, char*);
void tasks();
void words();


int forth_init() {
//	code_store = malloc(sizeof(uint8_t) * 2048);
//	stack = malloc(2048);
//	return_stack = malloc(sizeof(uint16_t) * 64);]
    
    // TODO should compiler use SCRATCHPAD directly?
 	scratchpad_code = code_store;
	compiled_code = code_store + SCRATCHPAD;
	//code = scratchpad_code;

    processes = new_task(3, "main");
    process = processes;
    process->code = scratchpad_code;

	uart_transmit("FORTH v0.1\n\n");
	
    /*
    char word[SCRATCHPAD];
	while(1) {
		printf( "> " );
		if (fgets(word, SCRATCHPAD, stdin) != NULL ) {
			word[strlen(word) - 1] = 0;
			if (strcmp(word, "exit") == 0) {
				return 0;
			}
			if (strcmp(word, ">") == 0) {
				dump_stack();
				continue;
			}
			if (strcmp(word, "debug") == 0) {
				debug();
				continue;
			}
			if (strcmp(word, "trace") == 0) {
				trace = true;
				continue;
			}
			if (strcmp(word, "traceoff") == 0) {
				trace = false;;
				continue;
			}
			if (strcmp(word, "reset") == 0) {
				dictionary = NULL;
				//	next_new_word = 0;
				scratchpad_code = code_store;
				compiled_code = code_store + SCRATCHPAD;
				sp = 0;
				rsp = 0;
				ip = 0;
				continue;
			}
			if (strcmp(word, "clear") == 0) {
				sp = 0;
				rsp = 0;
				ip = 0;
				continue;
			}

			if (strncmp(word, "include ", 8) == 0) {
				FILE *stream;
				char *line = NULL;
				size_t len = 0;
				ssize_t read;
				char filepath[20];
				strcpy(filepath, word + 8);
				len = strlen(filepath);
				if (strcspn(filepath, ".") == len) {
					strcat(filepath, ".fth");
				}
				stream = fopen(filepath, "r");
				if (stream == NULL) {
					printf ("failed to open file %s", filepath);
					continue;
				}

				memset(code_store, 0, SCRATCHPAD);
				while ((read = getline(&line, &len, stream)) != -1) {
					printf("Retrieved line of length %zu :\n", read);
					if (*(line + read -1) == '\n') {
						read--;
						*(line + read) = 0;
					}
					if ( read == 0) {
						continue;
					}
					printf("'%s'\n", line);

					compile(line, code_store);
				}

				free(line);
				fclose(stream);

				ip = 0;
				execute(code_store);
				dump_stack();

				continue;
			}

			if (strcmp(word, "") == 0) {
				continue;
			}
			memset(code_store, 0, SCRATCHPAD);
			scratchpad_code = code_store;
			compile(word, code_store);
			printf("entered '%s'\n", word);
			ip = 0;
			execute(code_store);
			scratchpad_code = code_store;
			code = scratchpad_code;
			dump_stack();
		}
	}


	/*





  uint8_t* program = code_store;



  // DUP + 3 * . CR
  int i = 0;

  program[i++] = LIT;
  program[i++] = 10;
  program[i++] = DUP;
  program[i++] = PLUS;
  program[i++] = LIT;
  program[i++] = 3;
  program[i++] = MULTIPLY;
  program[i++] = DOT;
  program[i++] = CR;
  program[i++] = END;


  //stack[0] = 11;
  //  stack[1] = 2;
  //  sp = 1;

  //  char* source = ": DOUBLE DUP + ; 3 DOUBLE .\nCR\nEND";
  //  char* source = "0 IF 10 ELSE 1 IF 33 THEN ELSE THEN . 2 3 > STACK IF 40 ELSE 50 THEN . CR END";
  char* source = "CR 99 2 3 STACK + CR . CR CR END";

  compile(source, code_store);
  display_code(code_store);

  program = code_store;
	 */
    
 //   tasks();
 //    debug();
    
    
    // compile some basic words
    memset(code_store, 0, SCRATCHPAD);
    scratchpad_code = code_store;
    compile(": ON 0x0bf886220 dup @ 0x01 4 lshift or ! ;", code_store);
    compile(": OFF 0x0bf886220 dup @ 0x01 4 lshift 0x03ff xor and ! ;", code_store);
}

void forth_execute(uint8_t* word) {
    if (trace) {
        printf("# input %s\n", word);
    }
    if (!immediate) {
        memset(code_store, 0, SCRATCHPAD);
        scratchpad_code = code_store;
    }
    compile(word, code_store);
    
    if (!immediate) {
        *process->code++ = END;
        process->ip = 0;
    //    debug();
        execute(code_store);
        scratchpad_code = code_store;
   //     code = scratchpad_code;
    //    dump_stack(process);
    }
}

void forth_run() {
    uint32_t tos_value;
	uint32_t nos_value;
    
    if (waiting) {
        next_task();
        if (waiting) {
            return;
        }
    }

    
    
//	while(1) {
  /*  
    if (process->ip == 0xffff) {
        return;
    }
*/
    uint8_t instruction = process->code[process->ip++];
/*
    if (trace) {
        printf ("%0x ", instruction);
    }
  */  
    if (instruction == 0) {
        printf ("no instruction %0x @ %04x", instruction, process->ip);
        process->ip = 0xffff;
        return;
    }

    if (trace) {
        printf("& execute @%i [%x|%x]\n", process->ip - 1, instruction, process->code[process->ip]);
    }

    switch (instruction) {
		case DUP:
			if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp];
			process->stack[++(process->sp)] = tos_value;
			break;

		case DROP:
			if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			process->sp--;
			break;

		case SWAP:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp];
			process->stack[process->sp] = process->stack[process->sp -1];
			process->stack[process->sp - 1] = tos_value;
			break;

		case LIT:
			tos_value = process->code[process->ip++] +
                    process->code[process->ip++] * 0x100 +
                    process->code[process->ip++] * 0x10000 +
                    process->code[process->ip++] * 0x1000000;
			process->stack[++(process->sp)] = tos_value;
			break;

		case ADD:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			nos_value = process->stack[process->sp--];
			tos_value = nos_value + tos_value;
			process->stack[++(process->sp)] = tos_value;
			break;

		case SUBTRACT:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			nos_value = process->stack[process->sp--];
			tos_value = nos_value - tos_value;
			process->stack[++process->sp] = tos_value;
			break;

		case DIVIDE:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			nos_value = process->stack[process->sp--];
			tos_value = nos_value / tos_value;
			process->stack[++(process->sp)] = tos_value;
			break;

		case MULTIPLY:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			nos_value = process->stack[process->sp--];
			tos_value = nos_value * tos_value;
			process->stack[++(process->sp)] = tos_value;
			break;

		case GREATER_THAN:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			nos_value = process->stack[process->sp--];
			tos_value = nos_value > tos_value ? 1 : 01;
			process->stack[++(process->sp)] = tos_value;
			break;

		case EQUAL_TO:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			nos_value = process->stack[process->sp--];
			tos_value = nos_value == tos_value ? 1 : 0;
			process->stack[++(process->sp)] = tos_value;
			break;

		case AND:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			nos_value = process->stack[process->sp--];
			tos_value = nos_value &tos_value;
			process->stack[++(process->sp)] = tos_value;
			break;

		case OR:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			nos_value = process->stack[process->sp--];
			tos_value = nos_value | tos_value;
			process->stack[++(process->sp)] = tos_value;
			break;
            
		case XOR:
			if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			nos_value = process->stack[process->sp--];
			tos_value = nos_value ^ tos_value;
			process->stack[++(process->sp)] = tos_value;
			break;

        case LSHIFT:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			nos_value = process->stack[process->sp--];
			tos_value = nos_value << tos_value;
			process->stack[++(process->sp)] = tos_value;
			break;

        case RSHIFT:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			nos_value = process->stack[process->sp--];
			tos_value = nos_value >> tos_value;
			process->stack[++(process->sp)] = tos_value;
			break;

        case TICKS:
			process->stack[++(process->sp)] = timer;
			break;

        case YIELD:
            wait(0);
			break;

        case WAIT:
            if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
            wait(tos_value);
			break;
            
		case ZBRANCH:
			if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			if (trace) {
				printf("zbranch for %i -> %s\n", tos_value, (tos_value == 0 ? "zero" : "non-zero"));
			}
			if (tos_value == 0) {
				int relative = process->code[process->ip];
				if (relative > 128) {
					relative = relative - 256;
				}
				if (trace) {
					printf("jump %i\n", relative);
				}
				process->ip = process->ip + relative;
			} else {
				process->ip++;
			}
			break;

		case BRANCH:
			if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			int relative = process->code[process->ip];
			if (relative > 128) {
				relative = relative - 256;
			}
			if (trace) {
				printf("jump %i\n", relative);
			}
			process->ip = process->ip + relative;
			break;

		case RUN:
			printf("run from %i\n", process->code[process->ip] + SCRATCHPAD);
			process->return_stack[++(process->rsp)] = process->ip + 1;
			uint8_t code_at = process->code[process->ip ++];
			process->ip = code_at + SCRATCHPAD;
			break;

		case RETURN:
			process->ip = process->return_stack[process->rsp--];
			break;

		case PRINT_TOS:
			if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
			printf("%i ", tos_value);
			break;

		case CR:
			printf("\n");
			break;

		case EMIT:
			if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
            tos_value = process->stack[process->sp--];
			printf("%c", tos_value);
			break;

        case READ_MEMORY:
            if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}

            tos_value = process->stack[process->sp--];                        
            printf("address 0x%x\n", tos_value);
            uint32_t *ptr = (uint32_t *) tos_value;
            tos_value = (uint32_t) *ptr;
            printf("value 0x%x\n", tos_value);
            process->stack[++(process->sp)] = tos_value;
            break;

        case WRITE_MEMORY:
            if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}

            tos_value = process->stack[process->sp--];
            nos_value = process->stack[process->sp--];
            printf("set 0x%x to 0x%x\n", nos_value, tos_value);
            ptr = (uint32_t *) nos_value;
            *ptr = tos_value;
            break;

        case PRINT_HEX:
            tos_value = process->stack[process->sp--];
            printf("0x%x", tos_value);
            break;
            
        case WORDS:
            words();
            break;
            
		case STACK:
			dump_stack(process);
			break;

		case DEBUG:
            debug();
			break;

		case TRACE_ON:
            trace = true;
            break;

		case TRACE_OFF:
            trace = false;;
			break;

		case RESET:
            dictionary = NULL;
            //	next_new_word = 0;
            scratchpad_code = code_store;
            compiled_code = code_store + SCRATCHPAD;
            process->sp = 0;
            process->rsp = 0;
            process->ip = 0;
            scratchpad_code[process->ip] = END;
            break;

		case CLEAR_REGISTERS:
            process->sp = 0;
            process->rsp = 0;
            process->ip = 0;
            break;

		case END:
			dump_stack(process);
            process->ip = 0xffff;
            next_task();
//			return;
			break;

		default:
			printf("unknown instruction %i", instruction);
			return;
			break;
    }
//	}
    
}

// TODO remove
void forth_tasks(uint32_t ticks) {
    /*
    int i;
    for (i = 0; i < task_count; i++) {
        struct Task task = tasks[i];

        if (ticks >= task.next) {
     //       printf("task from %i\n", task.code);
//            return_stack[++rsp] = ip + 1;
//            uint8_t code_at = code[ip ++];
//            ip = code_at + SCRATCHPAD;
   //         execute(task.code);
            
            task.next += task.interval;
        }

    }
*/
}

void words() {
    struct Dictionary * ptr = dictionary;
    while (ptr != NULL) {
        printf("%s -> %04x\n", ptr->name, ptr->code + SCRATCHPAD);
        ptr = ptr->previous;
    }
}

void wait(uint32_t wait_time) {
    process->next_time_to_run = timer + wait_time;
    next_task();
}

/*
 * Find the next task to execute. This is the highest priority task that has a next run
 * time that is less than the current time. If such a process exists then the waiting
 * flag is cleared.
 */
void next_task() {
    waiting = true;
    struct Process* next = processes;
    do {
        if (next->ip != 0xffff && next->next_time_to_run <= timer) {
            process = next;
            waiting = false;
            break;
        }
        next = next->next;
    } while (next != NULL);
}

void execute(uint8_t* code) {
    processes->code = code;
}

uint32_t popParameterStack() {
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        // TODO need to use exception or some other way of dropping out
        return;
    }
    return process->stack[process->sp];
}

void dump_stack(struct Process *p) {
	if (p->sp >= 0) {
		printf("< ");
        int i;
		for (i = 0; i <= p->sp; i++) {
			if (i == p->sp) {
				printf("| %i >", p->stack[i]);
			} else {
				printf("%i ", p->stack[i]);
			}
		}
	} else {
		printf("< >");
	}
	printf("\n");
}



uint8_t *jumps[6];
uint8_t jp = 0;
// uint8_t* define_start;


void to_upper(char *string, int len) {
    int i;
    for (i = 0; i <= len; i++) {
        if (*string >= 97 && *string <= 122) {
            *string = *string - 32;
        }
        string++;
    }
}

struct Process* new_task(uint8_t priority, char *name) {
    struct Process* nextProcess;
    nextProcess = malloc(sizeof(struct Process));

    if (processes != NULL) {
        struct Process* lastProcess = processes;
        while (lastProcess->next != NULL) {
            lastProcess = lastProcess->next;
        }
        lastProcess->next = nextProcess;
    }
    
    nextProcess->sp = -1;
    nextProcess->priority = priority;
    nextProcess->next_time_to_run = timer;
    nextProcess->name = name;
    
    nextProcess->ip = 0xffff;
    
    return nextProcess;
}

void compile(char* source, uint8_t* ignore) {
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
			printf("instruction [%s] %i\n", instruction, len);
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
            printf("not built in instruction\n");
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
                printf("not word\n");
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
                        printf("# literal = 0x%09llx\n", value);
                    }
					*process->code++ = value % 0x100;
					*process->code++ = (value / 0x100) % 0x100;
					*process->code++ = (value / 0x10000) % 0x100;
					*process->code++ = (value / 0x1000000) % 0x100;
				} else {
                    printf("instruction not understood\n");
                    return;
                }
			}
		}
	}
	source++;
}

void tasks() {
    int i = 1;
    struct Process* p = processes;
    do {
        printf("Task #%i%s %s - P%i, %i\n", i++, p == process ? "(running) " : "",
                p->name, p->priority, p->next_time_to_run);
        printf("  @%0x", p->ip);
        printf("  ");
        dump_stack(p);
        printf("\n");
        p = p->next;
    } while (p != NULL);
}

void debug() {
    int i;

	if (dictionary == NULL) {
		printf("No new words\n");
	} else {
        words();
	}

/*
	printf ("\n");
	for (i = 0; i < SCRATCHPAD; i++) {
		printf("[%03i] ", code_store[i]);

		if (i % 16 == 15) {
			printf("\n");
		}

	}
	printf("\n");
*/
	printf ("\n     ");
	for (i = 0; i < 16; i++) {
		printf("%3x ", i);
	}
	printf ("\n");
	for (i = 0; i < 512; i++) {
		if (i % 16 == 0) {
			printf("\n %5x  ", i);
		}
		printf("%3x ", code_store[i]);

	}
	printf("\n");

/*    for (i = 0; i < task_count; i++) {
        struct Task task = tasks[i];
//        printf("task#%i - %i, %i - %04x\n", i + 1, task.interval, task.next, task.code);
    }

	printf ("\n");
 */ 
}

void display_code(uint8_t* code) {

}
