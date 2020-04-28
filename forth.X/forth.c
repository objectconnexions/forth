#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <GenericTypeDefs.h>

#include "debug.h"
#include "parser.h"
#include "interpreter.h"
#include "forth.h"
#include "code.h"
// TODO is this still needed?
#include "compiler.h"
#include "dictionary.h"
#include "uart.h"

//#define NEXT_INSTRUCTION process->code[process->ip++]
#define PEEK_DATA process->stack[process->sp]
#define PUSH_DATA(value) (process->stack[++(process->sp)] = (value))
#define POP_DATA process->stack[process->sp--]
#define PUSH_RETURN(address) (process->return_stack[++(process->rsp)] =  (address))

void dump_parameter_stack(struct Process*);
void dump_return_stack(struct Process*);
void execute_next_instruction(void);
void display_code(uint8_t*);
void tasks(void);
uint32_t pop_stack(void);
void wait(uint32_t);
void next_task();
struct Process* new_task(uint8_t, char*);
void tasks(void);
void load_words(void);
void reset(void);
void print_top_of_stack(void);
void lit(void);

void return_to(void);
uint32_t timer = 0;
static bool waiting = true;
static bool echo;

//void (*core_function)(void)
//void (*core_wordsxx[128](void));
struct Process* processes;
struct Process* process;
struct Process* main_process;
uint8_t next_process_id = 0;
bool in_error;

int forth_init()
{   
    echo = true;
    trace = true;
    dictionary_init();
    compiler_init();
    load_words();
    processes = new_task(3, "MAIN");
    process = main_process = processes;
	uart_transmit_buffer("FORTH v0.3\n\n");
}

void forth_execute(uint8_t* line)
{
    if (echo) printf("> %s\n", line);
    parser_input(line);
    if (trace) 
    {
        dump_parameter_stack(process);
        printf("\n");
    }
    
    interpreter_process();
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

static void test_compile(char * input) {
    parser_input(input);
    compiler_compile_new();
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
                dictionary_memory_dump(0, 0x10);
                dictionary_memory_dump(0, 5);
                dictionary_memory_dump(9, 7);
                dictionary_memory_dump(8, 5);
                dictionary_memory_dump(18, 3);
                dictionary_memory_dump(0, 0x100);
            }
            else if (strcmp(buf, "load") == 0)
            {
                test_compile(": ON 0x0bf886220 dup @ 0x01 4 lshift or swap ! ;");
                test_compile(": OFF 0x0bf886220 dup @ 0x01 4 lshift 0x03ff xor and swap ! ;");
                test_compile(": FLASH on 200 ms off 200 ms ;");
                test_compile(": FLASH2 flash flash flash ;");
            
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
    
    execute_next_instruction();
}

void start_code(uint32_t at_address)
{
        process->ip = at_address;
}

void execute_next_instruction()
{
    in_error = false;

    if (process->ip < 0x80) {
        if (trace) 
        {   
            printf("&   C function %08X\n", dictionary_find_core_function(process->ip));
        }
        dictionary_find_core_function(process->ip)();
        return_to();
        
    } else {
//    uint32_t instruction = dictionary_read(process->ip++);
        uint32_t instruction = dictionary_read(process);
        /*
        if ((instruction & 0x80) == 0x80) {
            instruction = instruction * 0x80 | dictionary_read_byte(process->ip++);
        }
         */

        if (trace) 
        {
            char word_name[64];
            dictionary_find_word_for(instruction, word_name);
            printf("& execute #%i ~%04X: %0X: %s\n", process->id, process->ip - (instruction < 0x80 ? 1 : 2), instruction, word_name);
        }
        if (debug)
        {
            dump_parameter_stack(process);
            char word_name[64];
            dictionary_find_word_for(instruction, word_name);
     //       char * word = instruction >= WORD_COUNT ? "UNKNOWN" : word_name;
            printf("& %s [%i] %02X@%04X\n", word_name, process->id, instruction, process->ip - 1);
        }

        uint16_t code_at = dictionary_read(process); 
        PUSH_RETURN(process->ip);
        if (trace) {
            char word_name[64];
            dictionary_find_word_for(instruction, word_name);
            printf("& run %s from ~%04x return to~%04x\n", word_name, code_at, process->ip);
        }
        process->ip = code_at;
    }

    // TODO how do we set up the errors? needs to be part of the process struct
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
    
void push(uint32_t value)
{
    process->stack[++(process->sp)] = value;
}
    
void duplicate() 
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;	
    }
    uint32_t read = PEEK_DATA;
    PUSH_DATA(read);
}

void over()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t nos_value = process->stack[process->sp - 1];
    PUSH_DATA(nos_value);
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
    uint32_t tos_value = PEEK_DATA;
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
//    uint32_t tos_value = dictionary_read_word(process->ip);
    uint32_t tos_value = dictionary_read(process);
    //process->ip += 4;
    
//    NEXT_INSTRUCTION +
//            NEXT_INSTRUCTION * 0x100 +
//            NEXT_INSTRUCTION * 0x10000 +
//            NEXT_INSTRUCTION * 0x1000000;
    // process->stack[++(process->sp)] = tos_value;
    PUSH_DATA(tos_value);
}
/*
void exec_process()
{
   uint32_t tos_value = process->code[process->ip++];
   process->stack[++(process->sp)] = tos_value;
}
*/
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
        uint16_t pos = process->ip;
//        uint8_t relative = dictionary_read_byte(process->ip);
        uint8_t relative = dictionary_read(process);
//        int relative = process->code[process->ip];
        if (relative > 128) {
            relative = relative - 256;
        }
        if (trace) {
            printf("jump %i\n", relative);
        }
        process->ip = pos + relative;
    } else {
        process->ip++;
    }
}

void branch()
{
    uint16_t pos = process->ip;
//    uint8_t relative = dictionary_read_byte(process->ip);
    uint8_t relative = dictionary_read(process);
 //   uint8_t relative = process->code[process->ip];
    if (relative > 128) {
        relative = relative - 256;
    }
    if (trace) {
        printf("jump %i\n", relative);
    }
    process->ip = pos + relative;
}
/*
void run()
{
    
    if (dictionary_is_core_word(process->ip))
    {
        uint32_t address = dictionary_read_long(process->ip);
        process->ip += 4;
//        uint32_t address = process->code[process->ip ++] * 0x1000000 +
//        process->code[process->ip ++] * 0x10000 +
//        process->code[process->ip ++] * 0x100 +
//        process->code[process->ip ++];

        if (trace) printf("& core word at %08X\n", address);
        ((CORE_FUNC) address)();
    }
    else
    {
        // TODO make this the default case - so no RUN is needed!
        process->return_stack[++(process->rsp)] = process->ip + 2;
        uint16_t code_at = dictionary_read_word(process->ip);
        process->ip += 2;
//        uint16_t code_at = process->code[process->ip ++] * 0x100 + process->code[process->ip ++];
        process->ip = code_at;
        if (trace) {
            char word_name[64];
            find_word_for(code_at, word_name);
            printf("& run %s from %04x\n", word_name, code_at);
        }
    }
}
*/
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
    compiler_constant();

    /*
    
    // index of value within dictionary
//    uint16_t nos_value = dictionary_read_word(process->ip);
//    process->ip += 2;
    uint16_t nos_value = dictionary_read(process);
//    uint32_t tos_value = dictionary_read_long(nos_value);
    uint32_t tos_value = dictionary_read_at(nos_value);
    process->stack[++(process->sp)] = tos_value;
     */
}

void variable()
{
    compiler_variable();
    // address in memory
//    uint32_t tos_value = dictionary_read_word(process->ip++);
//    process->ip += 2;
    
    // TODO rename tos to address
//    uint32_t tos_value = dictionary_read(process);
//    process->stack[++(process->sp)] = (uint32_t) dictionary_memory_address(tos_value);
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
    dictionary_reset();
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

uint32_t pop_stack() {
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
        printf("Task #%i%s %s (P%i) ~%04X, next %i  ", 
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
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = process->stack[process->sp--];
    uint32_t nos_value = process->stack[process->sp--];
    dictionary_memory_dump(nos_value, tos_value);
    tasks();
}

void dump_base() {
  //  dictionary_dump();
    dictionary_memory_dump(0, 0x200);
    tasks();
}

void define() {
    compiler_compile_new();
}

void aborted() 
{
    printf("invalid execution\n");
}

/*
 push the address of the word in the dictionary that matched the next token on the input 
 */
void tick()
{
    // TODO get next word and find XT
}

void debug_word()
{
    
    dictionary_debug_entry(pop_stack());

    /*
    char name[32];
    CODE_INDEX addr, offset;
    
    offset = pop_stack();
//    
//    offset = dictionary_entry_after(offset);
//    end = dictionary
//    
//    
//    
    
    dictionary_find_word_for(offset, name);
    printf("Word '%s' at %04X\n", name, offset);
//    
//    offset = 
//    
    uint32_t value;
    value = dictionary_read(&offset);
    printf("  Next word at %04X\n", value);
    
    offset += strlen(name) + 1;
    
    int i;
    for (i = 0; i < 10; i++) {
        addr = offset;
        value = dictionary_read(&offset);
        if (value == LIT) {
            value = dictionary_read(&offset);
            printf("  %04X %i\n", addr, value);
        } else {
            dictionary_find_word_for(value, name);
            printf("  %04X %s\n", addr, name);
        }
    }
     * */
}

static void push_char()
{
    char name[32];
    parser_next_token();
    parser_token_text(name);    

    PUSH_DATA(name[0]);
}

// TODO move to compiler.c
static void compile_char()
{
    char name[32];
    parser_next_token();
    parser_token_text(name);    
    
    dictionary_append_value(LIT);
    dictionary_append_value(name[0]);
}

void load_words()
{
    
 //   dictionary_insert_internal_instruction(RUN, run);
//    dictionary_insert_internal_instruction("__PROCESS", exec_process);
    dictionary_insert_internal_instruction(LIT, lit);
//    dictionary_insert_internal_instruction("__VAR", variable);
//    dictionary_insert_internal_instruction("__CONSTANT", constant);
//    dictionary_insert_internal_instruction("__ZBRANCH", zero_branch);
//    dictionary_insert_internal_instruction("__BRANCH", branch);
    dictionary_insert_internal_instruction(RETURN, return_to);
    
    
    
    dictionary_add_core_word("DUP", duplicate);
    dictionary_add_core_word("SWAP", swap);
    dictionary_add_core_word("WORDS", dictionary_words);
    dictionary_add_core_word("DUMP", dump);

    dictionary_add_core_word("END", end);
//        
//    dictionary_add_core_word("__RUN", run);
//    dictionary_add_core_word("__PROCESS", exec_process);
//    dictionary_add_core_word("__LIT", lit);
//    dictionary_add_core_word("__VAR", variable);
    dictionary_add_core_word("VARIABLE", compiler_variable);
    dictionary_add_core_word("CONSTANT", compiler_constant);
//    dictionary_add_core_word("__ZBRANCH", zero_branch);
//    dictionary_add_core_word("__BRANCH", branch);
//    dictionary_add_core_word("__RETURN", return_to);
    
    dictionary_add_core_word("OVER", over);
    dictionary_add_core_word("DROP", drop);
    dictionary_add_core_word("NIP", nip);
    dictionary_add_core_word("ROT", rot);
    dictionary_add_core_word("TUCK", tuck);

    dictionary_add_core_word("+", add);
    dictionary_add_core_word("-", subtract);
    dictionary_add_core_word("/", divide);
    dictionary_add_core_word("*", muliply);
    dictionary_add_core_word(">", greater_than);
    dictionary_add_core_word("=", equal_to);
    dictionary_add_core_word("AND", and);
    dictionary_add_core_word("OR", or);
    dictionary_add_core_word("XOR", xor);
    dictionary_add_core_word("NOT", not);
    dictionary_add_core_word("LSHIFT", left_shift);
    dictionary_add_core_word("RSHIFT", right_shift);
    dictionary_add_core_word(".", print_top_of_stack);
    dictionary_add_core_word(".HEX", print_hex);
    dictionary_add_core_word("CR", print_cr);
    dictionary_add_core_word("EMIT", emit);
    dictionary_add_core_word("@", read_memory);
    dictionary_add_core_word("!", write_memory);
    dictionary_add_core_word("EXECUTE", execute_word);
    dictionary_add_core_word("INITIATE", initiate);
    dictionary_add_core_word("PAUSE", yield);
    dictionary_add_core_word("MS", wait_for);
    dictionary_add_core_word(".S", stack);
    dictionary_add_core_word("DUMP", dump);
    dictionary_add_core_word("TICKS", ticks);
    
    dictionary_add_core_word(":", define);
    dictionary_add_core_word(";", aborted);
    
    dictionary_add_core_word("_DUMP", dump_base);
    dictionary_add_core_word("_TRACE", trace_on);
    dictionary_add_core_word("_NOTRACE", trace_off);
    dictionary_add_core_word("_DEBUG", debug_on);
    dictionary_add_core_word("_NODEBUG", debug_off);
    dictionary_add_core_word("_RESET", reset);
    dictionary_add_core_word("_CLEAR", clear_registers);

    dictionary_add_core_word("\\", compiler_eol_comment);
    dictionary_add_core_word("(", compiler_inline_comment);
    dictionary_add_core_word("IF", compiler_if);
    dictionary_add_core_word("THEN", compiler_then);
    dictionary_add_core_word("BEGIN", compiler_begin);
    dictionary_add_core_word("AGAIN", compiler_again);
    dictionary_add_core_word("UNTIL", compiler_until);

    dictionary_add_core_word("'", tick);
    dictionary_add_core_word("DBG", debug_word);

    
     dictionary_add_core_word("CHAR", push_char);
     dictionary_add_core_word("[CHAR]", compile_char);
}