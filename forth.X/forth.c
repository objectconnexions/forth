#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include "forth.h"
#include "code.h"

uint32_t timer = 0;
bool waiting = true;


void dump_stack(struct Process*);
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

void code_init(void);
void compiler_reset(void);
bool compile(char*);


uint8_t code_store[2048];

struct Process* processes;

struct Dictionary* dictionary;




// TODO replace with ????
uint8_t* execute_code;

int forth_init() {
    code_init();

    execute_code = code_store;
    
    processes = new_task(3, "main");
    process = processes;
    process->code = execute_code; // WAS scratchpad_code]

	uart_transmit("FORTH v0.1\n\n");
    
    // compile some basic words
    //memset(code_store, 0, SCRATCHPAD);
 //   compile(": ON 0x0bf886220 dup @ 0x01 4 lshift or ! ;", code_store);
 //   compile(": OFF 0x0bf886220 dup @ 0x01 4 lshift 0x03ff xor and ! ;", code_store);
}

void forth_execute(uint8_t* word) {
    if (trace) {
        printf("execute %s\n", word);
    }
    if (compile(word)) {
        debug();
        execute(code_store);
        execute_code = code_store;
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
        return;
    }

    if (trace) {
        printf("& execute @%i [%x|%x]\n", process->ip - 1, instruction, process->code[process->ip - 1]);
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
            execute_code = code_store;
            compiler_reset();
        //    compiled_code = code_store + SCRATCHPAD;
            process->sp = 0;
            process->rsp = 0;
            process->ip = 0;
            execute_code[process->ip] = END;
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
