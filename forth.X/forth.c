#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <GenericTypeDefs.h>

#include "forth.h"
#include "code.h"
#include "compiler.h"
#include "uart.h"

uint32_t timer = 0;
uint8_t next_process_id = 0;
static bool waiting = true;
static bool echo;

void dump_parameter_stack(struct Process*);
void dump_return_stack(struct Process*);
void execute(uint8_t*);
void display_code(uint8_t*);
void tasks(void);
uint32_t popParameterStack(void);
void wait(uint32_t);
void next_task();
struct Process* new_task(uint8_t, char*);
void tasks(void);
void load_words(void);
void reset(void);

uint8_t dictionary[1024 * 8];
struct Process* processes;
struct Process* process;
struct Process* main_process;
bool in_error;

int forth_init()
{   
    load_words();
    compiler_init();
    processes = new_task(3, "MAIN");
    process = main_process = processes;
	uart_transmit_buffer("FORTH v0.2\n\n");
    echo = true;
}

void forth_execute(uint8_t* line)
{
    if (echo) printf("> %s\n", line);
    if (compiler_compile(line))
    {
        //if (trace) dump();
        main_process->ip = compiler_scratch();  // scratch code is the temporary entry in dictionary
        main_process->next_time_to_run = timer;
    }
}

bool stack_underflow()
{
    if (process->sp < 0)
    {
        printf("stack underflow; aborting\n");
        return true;
    }
    else
    {
        return false;
    }
}

static bool isAccessibleMemory(uint32_t address)
{
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

void forth_run()
{
    char buf[128]; 
    
    if (main_process->ip == 0xffff)
    {
        bool read = uart_next_line(buf);
        if (read)
        {
            //buf[strlen(buf) - 1] = 0;
            if (trace) 
                printf("> read line %s\n", buf);
            
            if (strcmp(buf, "usb") == 0)
            {
      //          sprintf(buf, "OTG = %08x CON = %08x PWRC = %08x\n", U1OTGCON, U1CON, U1PWRC);   
      //          uart_transmit_buffer(buf);    
                
            }
            else if (strncmp(buf, "led ", 4) == 0)
            {
         /*       if (strcmp(buf + 4, "on") == 0) {
                    PORTCbits.RC4 = 1;
                    uart_transmit_buffer("led on\n");
                } else if (strcmp(buf + 4, "off") == 0) {
                    PORTCbits.RC4 = 0;
                    uart_transmit_buffer("led off\n");
                }
         */       
            }
            else if (strcmp(buf, "timer") == 0)
            {
            /*    sprintf(buf, "timer = %04x\n", TMR1);   
                uart_transmit_buffer(buf);    
*/
            }
            else if (strcmp(buf, "ddd") == 0)
            {
                memory_dump(0, 0x10);
                memory_dump(0, 5);
                memory_dump(9, 7);
                memory_dump(8, 5);
                memory_dump(18, 3);
                memory_dump(0, 0x100);
            }
            else if (strcmp(buf, "load") == 0)
            {
                compiler_compile(": ON 0x0bf886220 dup @ 0x01 4 lshift or swap ! ;");
                compiler_compile(": OFF 0x0bf886220 dup @ 0x01 4 lshift 0x03ff xor and swap ! ;");
                compiler_compile(": FLASH on 200 ms off 200 ms ;");
                compiler_compile(": FLASH2 flash flash flash ;");
            
            }
            else if (strcmp(buf, "##") == 0)
            {
                uart_debug();            
            }
            else if (strcmp(buf, "echo") == 0)
            {
                echo = true;   
            }
            else if (strcmp(buf, "noecho") == 0)
            {
                echo = false;
            }
            else
            {                
                forth_execute(buf);
            }
        }
    }
    
    if (waiting)
    {
        next_task();
        if (waiting) {
            return;
        }
    }
    
    uint8_t instruction = process->code[process->ip++];    
    
    if (trace) 
    {
        printf("& execute [%i] %02x@%04x %s\n", process->id, instruction, process->ip - 1, core_words[instruction].word);
    }
    if (debug)
    {
        dump_parameter_stack(process);
        char * word = instruction >= WORD_COUNT ? "UNKNOWN" : core_words[instruction].word;
        printf(" %s [%i] %02x@%04x\n", word, process->id, instruction, process->ip - 1);
    }

    in_error = false;
    
    if (instruction < WORD_COUNT)
    {
        core_words[instruction].function();
    }
    else 
    {
        in_error = true;
        if (trace) 
        {
            printf("unknown instruction [%i] %02x@%04x\n", process->id, instruction, process->ip - 1);
        }
    }
    
    if (in_error)
    {
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

void duplicate() 
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;	
    }
    uint32_t tos_value = process->stack[process->sp];
    process->stack[++(process->sp)] = tos_value;
}

void over()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t nos_value = process->stack[process->sp - 1];
    process->stack[++(process->sp)] = nos_value;
}

void drop() 
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    process->sp--;
}

void nip() 
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    process->stack[process->sp] = tos_value;
}

void swap()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp];
    process->stack[process->sp] = process->stack[process->sp - 1];
    process->stack[process->sp - 1] = tos_value;
}

void tuck() 
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp];
    process->stack[++(process->sp)] = tos_value;
    process->stack[process->sp - 1] = process->stack[process->sp - 2];
    process->stack[process->sp - 2] = tos_value;
}

void rot() 
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp];
    process->stack[process->sp] = process->stack[process->sp - 1];
    process->stack[process->sp - 1] = process->stack[process->sp - 2];
    process->stack[process->sp - 2] = tos_value;
}

void lit()
{
    uint32_t tos_value = process->code[process->ip++] +
            process->code[process->ip++] * 0x100 +
            process->code[process->ip++] * 0x10000 +
            process->code[process->ip++] * 0x1000000;
    process->stack[++(process->sp)] = tos_value;
}

void exec_process()
{
   uint32_t tos_value = process->code[process->ip++];
   process->stack[++(process->sp)] = tos_value;
}

void add()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    tos_value = nos_value + tos_value;
    process->stack[++(process->sp)] = tos_value;
}

void subtract()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    tos_value = nos_value - tos_value;
    process->stack[++process->sp] = tos_value;
}

void divide()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    tos_value = nos_value / tos_value;
    process->stack[++(process->sp)] = tos_value;
}

void muliply() 
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    tos_value = nos_value * tos_value;
    process->stack[++(process->sp)] = tos_value;
}

void greater_than()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    tos_value = nos_value > tos_value ? 1 : 0;
    process->stack[++(process->sp)] = tos_value;
}

void equal_to()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    tos_value = nos_value == tos_value ? 1 : 0;
    process->stack[++(process->sp)] = tos_value;
}

void and() 
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    tos_value = nos_value &tos_value;
    process->stack[++(process->sp)] = tos_value;
}

void or()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    tos_value = nos_value | tos_value;
    process->stack[++(process->sp)] = tos_value;
}

void xor()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    tos_value = nos_value ^ tos_value;
    process->stack[++(process->sp)] = tos_value;
}

void not()
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    tos_value = tos_value > 0 ? 0 : 1;
    process->stack[++(process->sp)] = tos_value;
}

void left_shift()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    tos_value = nos_value << tos_value;
    process->stack[++(process->sp)] = tos_value;
}

void right_shift()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    tos_value = nos_value >> tos_value;
    process->stack[++(process->sp)] = tos_value;
}

void ticks()
{
    process->stack[++(process->sp)] = timer;
}

void yield()
{
    wait(0);
}

void wait_for()
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    wait(tos_value);
}

void zero_branch()
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
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
}

void branch()
{
    uint8_t relative = process->code[process->ip];
    if (relative > 128) {
        relative = relative - 256;
    }
    if (trace) {
        printf("jump %i\n", relative);
    }
    process->ip = process->ip + relative;
}

void run()
{
    // TODO make this the default case - so no RUN is needed!
    process->return_stack[++(process->rsp)] = process->ip + 2;
    uint16_t code_at = process->code[process->ip ++] * 0x100 + process->code[process->ip ++];
    process->ip = code_at;
    if (trace) {
        char word_name[64];
        find_word_for(code_at, word_name);
        printf("& run %s from %04x\n", word_name, code_at);
    }
}

void execute_word() 
{
    if (stack_underflow()) {
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    if (trace) {
        printf("# execute from %i\n", tos_value);
    }
    process->return_stack[++(process->rsp)] = process->ip + 1;
    process->ip = tos_value;
}
 
void initiate()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
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
}

void return_to()
{
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
}

void print_top_of_stack()
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    printf("%i ", tos_value);
}

void print_line()
{
    printf("\n");
}

void emit()
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    printf("%c", tos_value);
}

inline void constant() 
{
    // index of value within dictionary
    uint32_t nos_value = dictionary[process->ip++] * 0x100 +
            dictionary[process->ip++]; 
    uint32_t tos_value = dictionary[nos_value++] +
            dictionary[nos_value++] * 0x100 +
            dictionary[nos_value++] * 0x10000 +
            dictionary[nos_value++] * 0x1000000;
    process->stack[++(process->sp)] = tos_value;
}

void variable()
{
    // address in memory
    uint32_t tos_value = dictionary[process->ip++] * 0x100 +
            dictionary[process->ip++]; 
    process->stack[++(process->sp)] = (uint32_t) dictionary + tos_value;            
}

void read_memory()
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        in_error = true;
        return;
    }

    uint32_t tos_value = process->stack[process->sp--];
    if (trace) printf("& address 0x%x\n", tos_value);
    if (isAccessibleMemory(tos_value)) 
    {
        uint32_t *ptr = (uint32_t *) tos_value;
        uint32_t tos_value = (uint32_t) *ptr;
        if (trace) printf("& value 0x%x\n", tos_value);
        process->stack[++(process->sp)] = tos_value;
    }
    else 
    {
        in_error = true;
    }
}

void write_memory()
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        in_error = true;
        return;
    }

    uint32_t tos_value = process->stack[process->sp--]; // address
    uint32_t nos_value = process->stack[process->sp--]; // write value
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
}

void print_hex()
{
    uint32_t tos_value = process->stack[process->sp--];
    printf("0x%X", tos_value);
}

inline void print_cr() 
{
    printf("\n");
}

void stack()
{
    dump_parameter_stack(process);
    printf("\n");
}

inline void trace_on()
{
    trace = true;
}

inline void trace_off()
{
    trace = false;
}

inline void debug_on()
{
    debug = true;
}

inline void debug_off()
{
    debug = false;
}

void reset() {
    compiler_reset();
    struct Process* next = processes;
    do {
        next->sp = -1;
        next->rsp = 0;
        next->ip = 0;
        next = next->next;
    } while (next != NULL);
}

void clear_registers()
{
    process->sp = 0;
    process->rsp = 0;
    process->ip = 0;
}

void end()
{
    process->ip = 0xffff;
    process->next_time_to_run = 0;
    next_task();
    if (trace) {
        printf("& end, go to %s\n", process->name);
    }
}

void wait(uint32_t wait_time) {
    process->next_time_to_run = timer + wait_time;
    next_task();
}

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
				printf("| %x ]", p->return_stack[i]);
			} else {
				printf("%x ", p->return_stack[i]);
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
    nextProcess->code = dictionary; // TODO remove code from process - all processes use the same code
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
        printf("Task #%i%s %s (P%i) @%04X, next %i  ", 
                p->id, p == process ? "*" : "", p->name, p->priority,  
                p->ip, p->next_time_to_run);
        dump_return_stack(p);
        printf(" ");
        dump_parameter_stack(p);
        printf("\n");
        p = p->next;
    } while (p != NULL);
}

void memory_dump(uint16_t start, uint16_t size) {
    int i, j;
    bool new_line;
    int from = start - (start % 16);
    int end = start + size;
    int to = end / 16 * 16 + (end % 16 == 0 ? 0 : 16);
    
	printf ("\n%04x   ", start);
	for (i = 0; i < 16; i++) {
		printf("%X   ", i);
	}
	printf ("\n");
    
	for (i = from; i < to; i++) {
        bool new_line = i % 16 == 0;
		if (new_line) {
			printf("\n%04X %s", i, i == start ? "[" : " ");
		}
		printf("%02X%s", dictionary[i], (!new_line && i == start - 1) ? "[" : (i == end - 1 ? "]" : " "));
        
        if (i % 16 == 15) {
            for (j = 0; j < 16; j++)
            {
                if (j == 8) printf(" ");
                char c = dictionary[i - 15 + j];
                if (c < 32) c = '.';
                printf("%c", c);
            }
        }
	}
	printf("\n\n");

    tasks();
}

void dump() {
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    memory_dump(nos_value, tos_value);
}

void dump_base() {
    compiler_dump();
    memory_dump(0, 0x200);
}

void load_words()
{
    int i = 0;
    core_words[i++] = (struct Word) { "END", end, "" };
        
    core_words[i++] = (struct Word) { "__RUN", run, "" };
    core_words[i++] = (struct Word) { "__PROCESS", exec_process, "" };
    core_words[i++] = (struct Word) { "__LIT", lit, "Duplicate top of stack" };
    core_words[i++] = (struct Word) { "__VAR", variable, "" };
    core_words[i++] = (struct Word) { "__CONSTANT", constant, "" };
    core_words[i++] = (struct Word) { "__ZBRANCH", zero_branch, "" };
    core_words[i++] = (struct Word) { "__BRANCH", branch, "" };
    core_words[i++] = (struct Word) { "__RETURN", return_to, "" };
    
    core_words[i++] = (struct Word) { "OVER", over, "?? of stack" };
    core_words[i++] = (struct Word) { "DROP", drop, "?? of stack" };
    core_words[i++] = (struct Word) { "NIP", nip, "?? of stack" };
    core_words[i++] = (struct Word) { "SWAP", swap, "swap top two element of stack" };
    core_words[i++] = (struct Word) { "DUP", duplicate, "Duplicate top of stack" };
    core_words[i++] = (struct Word) { "ROT", rot, "Duplicate top of stack"};
    core_words[i++] = (struct Word) { "TUCK", tuck, "Duplicate top of stack" };

    core_words[i++] = (struct Word) { "+", add, "" };
    core_words[i++] = (struct Word) { "-", subtract, "" };
    core_words[i++] = (struct Word) { "/", divide, "" };
    core_words[i++] = (struct Word) { "*", muliply, "" };
    core_words[i++] = (struct Word) { ">", greater_than, "" };
    core_words[i++] = (struct Word) { "=", equal_to, "" };
    core_words[i++] = (struct Word) { "AND", and, "" };
    core_words[i++] = (struct Word) { "OR", or, "" };
    core_words[i++] = (struct Word) { "XOR", xor, "" };
    core_words[i++] = (struct Word) { "NOT", not, "" };
    core_words[i++] = (struct Word) { "LSHIFT", left_shift, "" };
    core_words[i++] = (struct Word) { "RSHIFT", right_shift, "" };
    core_words[i++] = (struct Word) { ".", print_top_of_stack, "" };
    core_words[i++] = (struct Word) { ".HEX", print_hex, "" };
    core_words[i++] = (struct Word) { "CR", print_cr, "" };
    core_words[i++] = (struct Word) { "EMIT", emit, "" };
    core_words[i++] = (struct Word) { "@", read_memory, "" };
    core_words[i++] = (struct Word) { "!", write_memory, "" };
    core_words[i++] = (struct Word) { "EXECUTE", execute_word, "" };
    core_words[i++] = (struct Word) { "INITIATE", initiate, "" };
    core_words[i++] = (struct Word) { "PAUSE", yield, "" };
    core_words[i++] = (struct Word) { "MS", wait_for, "" };
    core_words[i++] = (struct Word) { "WORDS", compiler_words, "" };
    core_words[i++] = (struct Word) { ".S", stack, "" };
    core_words[i++] = (struct Word) { "DUMP", dump, "" };
    core_words[i++] = (struct Word) { "_DUMP", dump_base, "" };
    core_words[i++] = (struct Word) { "_TRACE", trace_on, "" };
    core_words[i++] = (struct Word) { "_NOTRACE", trace_off, "" };
    core_words[i++] = (struct Word) { "_DEBUG", debug_on, "" };
    core_words[i++] = (struct Word) { "_NODEBUG", debug_off, "" };
    core_words[i++] = (struct Word) { "_RESET", reset, "" };
    core_words[i++] = (struct Word) { "_CLEAR", clear_registers, "" };
    core_words[i++] = (struct Word) { "TICKS", ticks, "" };
}