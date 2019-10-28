#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <GenericTypeDefs.h>

#include "forth.h"
#include "code.h"
#include "uart.h"

uint32_t timer = 0;
uint8_t next_process_id = 0;
bool waiting = true;


void dump_parameter_stack(struct Process*);
void dump_return_stack(struct Process*);
void execute(uint8_t*);
void display_code(uint8_t*);
void dump(void);
void tasks(void);
uint32_t popParameterStack(void);
void wait(uint32_t);
void next_task();
struct Process* new_task(uint8_t, char*);
void tasks();
void words();

void compiler_init(void);
void compiler_reset(void);
bool compiler_compile(char*);
void compiler_dump(void);

uint8_t code_store[2048];

struct Process* processes;

struct Dictionary* dictionary;

struct Process* process;
struct Process* main_process;


int forth_init() {
    compiler_init();
    processes = new_task(3, "MAIN");
    process = main_process = processes;
	uart_transmit_buffer("FORTH v0.1\n\n");
}

void forth_execute(uint8_t* word) {
    printf("> %s\n", word);
    if (compiler_compile(word)) {
        if (trace) {
            dump();
        }
        main_process->ip = 0;
        main_process->next_time_to_run = timer;
    }
}

bool stack_underflow() {
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        return true;
    } else {
        return false;
    }
}

static bool isAccessibleMemory(uint32_t address) {
    if ((address >= 0xBF800000 && address <= 0xBF8FFFFF) ||
            (address >= 0x80000000 && address <= 0x8000FFFF)) 
    {
        return true;
    } 
    else 
    {
        printf("LIMIT %X!", address);
        return false;
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
    
    uint8_t instruction = process->code[process->ip++];    
    if (instruction == 0) {
        printf ("no instruction %0x @ %04x", instruction, process->ip - 1);
        process->ip = 0xffff;
        process->next_time_to_run = 0;
        return;
    }

    if (trace) {
        printf("& execute [%i] %02x@%04x\n", process->id, instruction, process->ip - 1);
    }
    if (debug) {
        dump_parameter_stack(process);
        char *ins;
        switch (instruction) {
            case DUP:
                ins = "DUP";
                break;
                
            case SWAP:
                ins = "SWAP";
                break;
                           
            case WRITE_MEMORY:
                ins = "!";
                break;

            case ADD:
                ins = "+";
                break;
                
            case LIT:
                ins = "PUSH";
                break;
                
            default:
                ins = "?";
        }
        printf(" %s [%i] %02x@%04x\n", ins, process->id, instruction, process->ip - 1);
    }

    bool in_error = false;
    switch (instruction) {
		case DUP:
			if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp];
			process->stack[++(process->sp)] = tos_value;
			break;

		case OVER:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			nos_value = process->stack[process->sp - 1];
			process->stack[++(process->sp)] = nos_value;
			break;

		case DROP:
			if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			process->sp--;
			break;

		case NIP:
			if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
            tos_value = process->stack[process->sp--];
            process->stack[process->sp] = tos_value;
			break;

		case SWAP:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp];
			process->stack[process->sp] = process->stack[process->sp - 1];
			process->stack[process->sp - 1] = tos_value;
			break;

		case TUCK:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp];
            process->stack[++(process->sp)] = tos_value;
            process->stack[process->sp - 1] = process->stack[process->sp - 2];
            process->stack[process->sp - 2] = tos_value;
			break;

		case ROT:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp];
            process->stack[process->sp] = process->stack[process->sp - 1];
            process->stack[process->sp - 1] = process->stack[process->sp - 2];
            process->stack[process->sp - 2] = tos_value;
			break;

		case LIT:
			tos_value = process->code[process->ip++] +
                    process->code[process->ip++] * 0x100 +
                    process->code[process->ip++] * 0x10000 +
                    process->code[process->ip++] * 0x1000000;
			process->stack[++(process->sp)] = tos_value;
			break;

		case PROCESS:
			tos_value = process->code[process->ip++];;
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
            if (process->sp < 0) {
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
            ;
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
            if (trace) {
                printf("& run from %i\n", process->code[process->ip]);
            }
			process->return_stack[++(process->rsp)] = process->ip + 1;
			uint8_t code_at = process->code[process->ip ++];
			process->ip = code_at;
			break;

        case EXECUTE:
            if (stack_underflow()) {
                return;
            }
			tos_value = process->stack[process->sp--];
            if (trace) {
    			printf("# execute from %i\n", tos_value);
            }
			process->return_stack[++(process->rsp)] = process->ip + 1;
			process->ip = tos_value;
            break;

        case INITIATE:
			if (process->sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = process->stack[process->sp--];
            nos_value = process->stack[process->sp--];
            struct Process *run_process = processes;
            do {
                if (run_process->id == tos_value) {
                    run_process->ip = nos_value;
                    run_process->next_time_to_run = timer + 1;
                    if (trace) {
                        printf("& initiate from %04x with %s at %i\n", run_process->ip, run_process->name, run_process->next_time_to_run);
                    }
                    break;
                }
                run_process = run_process->next;
            } while (run_process != NULL);
            if (run_process == NULL) {
                printf("no process with ID %i\n", tos_value);                
            }
            if (trace) {
                tasks();
            }
			break;
            
		case RETURN:
            if (trace) {
                printf("& return ");
                dump_return_stack(process);
                printf("\n");
            }
            if (process->rsp < 0) {
                // copy of END code - TODO refactor
                if (trace) {
                    dump_parameter_stack(process);
                    printf("\n");
                }
                process->ip = 0xffff;
                process->next_time_to_run = 0;
                next_task();
                if (trace) {
                    printf("& end, go to %s\n", process->name);
                }
            } else {
                process->ip = process->return_stack[process->rsp--];
            }
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
            if (trace) printf("address 0x%x\n", tos_value);
            if (isAccessibleMemory(tos_value)) 
            {
                uint32_t *ptr = (uint32_t *) tos_value;
                tos_value = (uint32_t) *ptr;
                if (trace) printf("& value 0x%x\n", tos_value);
                process->stack[++(process->sp)] = tos_value;
            }
            else 
            {
                in_error = true;
            }
            break;

        case WRITE_MEMORY:
            if (process->sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}

            tos_value = process->stack[process->sp--]; // address
            nos_value = process->stack[process->sp--]; // write value
            if (trace) printf("& set address %04X to %04X\n", tos_value, nos_value);
            if (isAccessibleMemory(tos_value)) 
            {
                uint32_t *ptr = (uint32_t *) tos_value;
                *ptr = nos_value;
            }
            else 
            {
                in_error = true;
            }
            break;

        case PRINT_HEX:
            tos_value = process->stack[process->sp--];
            printf("0x%x", tos_value);
            break;
            
        case WORDS:
            compiler_words();
            break;
            
		case STACK:
			dump_parameter_stack(process);
            printf("\n");
            break;

		case DUMP:
            dump();
			break;

		case TRACE_ON:
            trace = true;
            break;

		case TRACE_OFF:
            trace = false;
			break;

		case DEBUG_ON:
            debug = true;
            break;

		case DEBUG_OFF:
            debug = false;
			break;

		case RESET:
            dictionary = NULL;
            compiler_reset();
            struct Process* next = processes;
            do {
                next->sp = -1;
                next->rsp = 0;
                next->ip = 0;
                next = next->next;
            } while (next != NULL);
            code_store[0] = END;
            break;

		case CLEAR_REGISTERS:
            process->sp = 0;
            process->rsp = 0;
            process->ip = 0;
            break;

		case END:
			//dump_parameter_stack(process);
            //printf("\n");
            process->ip = 0xffff;
            process->next_time_to_run = 0;
            next_task();
            if (trace) {
                printf("& end, go to %s\n", process->name);
            }
			break;

		default:
            printf("%s!\n", instruction);
            if (trace) 
            {
                printf("unknown instruction [%i] %02x@%04x\n", process->id, instruction, process->ip - 1);
            }
			break;
    }
    
    if (in_error) {
        in_error = false;
        process->sp = 0;
        process->rsp = 0;
        process->ip = 0;
        process->ip = 0xffff;
        process->next_time_to_run = 0;
        next_task();
        printf(" ABORTED\n");
    }
}

void wait(uint32_t wait_time) {
    process->next_time_to_run = timer + wait_time;
    next_task();
}
/*
 * void execute(uint8_t* code) {
    processes->code = code;
}
*/
uint32_t popParameterStack() {
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        // TODO need to use exception or some other way of dropping out
        return;
    }
    return process->stack[process->sp];
}

void dump_return_stack(struct Process *p) {
	if (p->rsp >= 0) {
		printf("[ ");
        int i;
		for (i = 0; i <= p->rsp; i++) {
			if (i == p->rsp) {
				printf("| %i ]", p->return_stack[i]);
			} else {
				printf("%i ", p->return_stack[i]);
			}
		}
	} else {
		printf("[ ]");
	}
}

void dump_parameter_stack(struct Process *p) {
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
}



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
    
    nextProcess->id = next_process_id++;
    nextProcess->sp = -1;
    nextProcess->rsp = -1;
    nextProcess->priority = priority;
    nextProcess->next_time_to_run = 0;
    nextProcess->name = malloc(sizeof(strlen(name) + 1));
    nextProcess->code = code_store; // TODO remove code from process - all processes use the same code
    strcpy(nextProcess->name, name);
    
    nextProcess->ip = 0xffff;
    
    if (trace) {
        printf("# new task %s (P%i)\n", nextProcess->name, priority);
    }
     
    return nextProcess;
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
            if (trace) {
                printf("& next process %s\n", process->name);
            }
            break;
        }
        next = next->next;
    } while (next != NULL);
}

void tasks() {
    struct Process* p = processes;
    printf("Time %i\n", timer);
    do {
        printf("Task #%i%s %s (P%i) @%04x, next %i  ", 
                p->id, p == process ? "*" : "", p->name, p->priority,  
                p->ip, p->next_time_to_run);
        dump_return_stack(p);
        printf(" ");
        dump_parameter_stack(p);
        printf("\n");
        p = p->next;
    } while (p != NULL);
}

void dump() {
    int i;

	if (dictionary == NULL) {
		printf("No new words\n");
	} else {
        compiler_words();
	}

    compiler_dump();
    
	printf ("\n0000   ");
	for (i = 0; i < 16; i++) {
		printf("%X   ", i);
	}
	printf ("\n");
	for (i = 0; i < 512; i++) {
		if (i % 16 == 0) {
			printf("\n%04X  ", i);
		}
		printf("%02X ", code_store[i]);
	}
	printf("\n");

    tasks();
}
/*
void display_code(uint8_t* code) {

}
*/