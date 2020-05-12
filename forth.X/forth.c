#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <GenericTypeDefs.h>

#include "logger.h"
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

#define LOG "Forth"

void dump_parameter_stack(char *, struct Process*);
void dump_return_stack(char *, struct Process*);
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
void return_to(void);
void lit(void);

static void dump_base(void);
    
uint32_t timer = 0;
static bool waiting = true;

struct Process* processes;
struct Process* process;
struct Process* interpreter_process;
uint8_t next_process_id = 0;
bool in_error;

int forth_init()
{   
    parser_init();
    dictionary_init();
    compiler_init();
    load_words();
    processes = new_task(3, "IDLE");
    new_task(3, "POWER");
    interpreter_process = new_task(3, "INTERPRETER");
    process = interpreter_process;
	uart_transmit_buffer("FORTH v0.3\n\n");
    
    struct Dictionary_Entry entry;
    dictionary_find_entry("INTERACTIVE", &entry);
    interpreter_process->ip = entry.instruction;
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
    compiler_compile();
}

void forth_run()
{
    in_error = false;
    while (true)
    {
        uint32_t instruction = dictionary_read_instruction(process);
        forth_execute(instruction);

        // TODO how do we set up the errors? needs to be part of the process struct
        if (in_error)
        {
            // TODO extract into abort_task() function
            in_error = false;
            process->sp = 0;
            process->rsp = 0;
            process->ip = 0;
            process->ip = LAST_ENTRY;
            process->next_time_to_run = 0;
            next_task();
            printf(" ABORTED\n");
        }
    }
}
//
//static void process_input()
//{
//    char buf[128]; 
//    
////    if (interpreter_process->ip == LAST_ENTRY)
////    {
//        bool read = uart_next_line(buf);
//        if (read)
//        {
//            //buf[strlen(buf) - 1] = 0;
//            if (trace) 
//                log_trace(LOG, "input line: '%s'\n", buf);
//
//            if (strcmp(buf, "usb") == 0)
//            {
//      //          sprintf(buf, "OTG = %08x CON = %08x PWRC = %08x\n", U1OTGCON, U1CON, U1PWRC);   
//      //          uart_transmit_buffer(buf);    
//
//            }
//            else if (strncmp(buf, "led ", 4) == 0)
//            {
//         /*       if (strcmp(buf + 4, "on") == 0) {
//                    PORTCbits.RC4 = 1;
//                    uart_transmit_buffer("led on\n");
//                } else if (strcmp(buf + 4, "off") == 0) {
//                    PORTCbits.RC4 = 0;
//                    uart_transmit_buffer("led off\n");
//                }
//         */       
//            }
//            else if (strcmp(buf, "timer") == 0)
//            {
//            /*    sprintf(buf, "timer = %04x\n", TMR1);   
//                uart_transmit_buffer(buf);    
//*/
//            }
//            else if (strcmp(buf, "ddd") == 0)
//            {
//                dictionary_memory_dump(0, 0x10);
//                dictionary_memory_dump(0, 5);
//                dictionary_memory_dump(9, 7);
//                dictionary_memory_dump(8, 5);
//                dictionary_memory_dump(18, 3);
//                dictionary_memory_dump(0, 0x100);
//            }
//            else if (strcmp(buf, "load") == 0)
//            {
//                test_compile(": ON 0x0bf886220 dup @ 0x01 4 lshift or swap ! ;");
//                test_compile(": OFF 0x0bf886220 dup @ 0x01 4 lshift 0x03ff xor and swap ! ;");
//                test_compile(": FLASH on 200 ms off 200 ms ;");
//                test_compile(": FLASH2 flash flash flash ;");
//
//            }
//            else if (strcmp(buf, "##") == 0)
//            {
//                uart_debug();            
//            }
//            else if (strcmp(buf, "??") == 0)
//            {
//                tasks();            
//            }
//            else if (strcmp(buf, "echo") == 0)
//            {
//                echo = true;   
//            }
//            else if (strcmp(buf, "noecho") == 0)
//            {
//                echo = false;
//            }
//            else
//            {                
////                process_buffer(buf);
//                
//                if (echo) printf("> %s\n", buf);
//                parser_input(buf);
//                if (trace) 
//                {
//                    dump_parameter_stack(process);
//                    printf("\n");
//                }
//
//                interpreter_process();
//
//            }
//        }
//        
////    }
//}
//
void start_code(uint32_t at_address)
{
    PUSH_RETURN(process->ip);
    // TODO this should be the main process only
    interpreter_process->ip = at_address;
}

void forth_execute(uint32_t instruction)
{
    if (dictionary_shortcode(instruction))
    {
        dictionary_execute_function(instruction);
    }
    else
    {
        PUSH_RETURN(process->ip);
        if (log_level <= TRACE) 
        {
            char word_name[64];
            dictionary_find_word_for(instruction, word_name);
            log_trace(LOG, "run %s jump to ~%04x return to~%04x", word_name, instruction, process->ip);
        }
        process->ip = instruction;
    }
}

void execute_next_instruction()
{
    in_error = false;
    
    // TODO move location into trace block
    uint32_t location = process->ip; // capture first as can't infer how many bytes it was later on
    
    if (location == LAST_ENTRY) {
        return;
    }
    
    uint32_t instruction = dictionary_read_instruction(process);
    if (log_level <= TRACE)
    {   
        char word_name[64];
        dictionary_find_word_for(instruction, word_name);
        log_trace(LOG, "execute #%i ~%04X: %0X: %s", process->id, location, instruction, word_name);
    }
    forth_execute(instruction);

    // TODO how do we set up the errors? needs to be part of the process struct
    if (in_error)
    {
        in_error = false;
        process->sp = 0;
        process->rsp = 0;
        process->ip = 0;
        process->ip = LAST_ENTRY;
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
    uint32_t tos_value = dictionary_read(process);
    PUSH_DATA(tos_value);
}

/*
 * Using the address in the dictionary work out the corresponding memory address
 */
void address()
{
    PUSH_DATA(dictionary_data_address(process->ip));
    return_to();
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
    uint32_t time = process->stack[process->sp--];
    wait(time);
}

void branch()
{
    uint16_t pos = process->ip;
    int32_t relative = dictionary_read_byte(process);
    relative = relative - 0x80;
    process->ip = pos + relative;
}

void zero_branch()
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t offset = process->stack[process->sp--];
    log_trace(LOG, "zbranch for %i -> %s", offset, (offset == 0 ? "zero" : "non-zero"));
    if (offset == 0) {
        branch();
    } else {
        process->ip++;
    }
}

void execute_word() 
{
    if (stack_underflow()) {
        return;
    }
    uint32_t instruction = process->stack[process->sp--];
    log_debug(LOG, "execute from %i", instruction);
    process->return_stack[++(process->rsp)] = process->ip + 1;
    process->ip = instruction;
}
 
void initiate()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t id = process->stack[process->sp--];
    uint32_t instruction = process->stack[process->sp--];
    struct Process *run_process = processes;
    do {
        if (run_process->id == id) {
            run_process->ip = instruction;
            run_process->next_time_to_run = timer + 1;
            log_debug(LOG, "initiate from %04x with %s at %i", run_process->ip, run_process->name, run_process->next_time_to_run);
            break;
        }
        run_process = run_process->next;
    } while (run_process != NULL);
    if (run_process == NULL) {
        printf("no process with ID %i\n", id);                
    }
//    if (trace) {
//        tasks();
//    }
}

void return_to()
{
    char buf[64];
    if (log_level <= TRACE) 
    {
        dump_return_stack(buf, process);
        log_trace(LOG, "return %s", buf);
    }
    if (process->rsp < 0) {
        // copy of END code - TODO refactor
        if (log_level <= TRACE) 
        {
            dump_parameter_stack(buf, process);
            log_trace(LOG, "  %s", buf);
        }
        process->ip = LAST_ENTRY;
        process->next_time_to_run = 0;
        next_task();
        log_trace(LOG, "end, go to %s", process->name);
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
    uint32_t ch = process->stack[process->sp--];
    printf("%c", ch);
}

void read_memory()
{
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        in_error = true;
        return;
    }

    uint32_t address = process->stack[process->sp--];
    log_trace(LOG, "address 0x%x", address);
    if (isAccessibleMemory(address)) 
    {
        uint32_t *ptr = (uint32_t *) address;
        uint32_t value = (uint32_t) *ptr;
        log_debug(LOG, "value at %0X = 0x%x", address, value);
        process->stack[++(process->sp)] = value;
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

    uint32_t address = process->stack[process->sp--]; // address
    uint32_t value = process->stack[process->sp--]; // write value
    log_debug(LOG, "set address %04X to %04X", address, value);
    if (isAccessibleMemory(address)) 
    {
        uint32_t *ptr = (uint32_t *) address;
        *ptr = value;
    }
    else 
    {
        in_error = true;
    }
}

void print_hex()
{
    uint32_t value = process->stack[process->sp--];
    printf("0x%X", value);
}

inline void print_cr() 
{
    printf("\n");
}

void stack()
{
    char buf[64];
    dump_parameter_stack(buf, process);
    printf("%s\n", buf);
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
        if (next != interpreter_process)
        {
            next->sp = -1;
            next->rsp = 0;
            next->ip = LAST_ENTRY;
            next = next->next;
        }
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
    process->ip = LAST_ENTRY;
    process->next_time_to_run = 0;
    next_task();
    log_trace(LOG, "end, go to %s", process->name);
}

void wait(uint32_t wait_time) {
    process->next_time_to_run = timer + wait_time;
    next_task();
}

uint32_t pop_stack() {
    if (process->sp < 0) {
        printf("stack underflow; aborting\n");
        // TODO need to use exception or some other way of dropping out
        return;
    }
    return process->stack[process->sp--];
}

void dump_return_stack(char *buf, struct Process *p) {
	if (p->rsp >= 0) {
		uint8_t len = sprintf(buf, "[ ");
        int i;
		for (i = 0; i <= p->rsp; i++) {
            // TODO use snprintf and check length written to avoid overflow
			if (i == p->rsp) {
				len += sprintf(buf + len, "| %x ]", p->return_stack[i]);
			} else {
				len += sprintf(buf + len, "%x ", p->return_stack[i]);
			}
		}
	} else {
		sprintf(buf, "[ ]");
	}
}

void dump_parameter_stack(char *buf, struct Process *p) {
	if (p->sp >= 0) {
		uint8_t len = sprintf(buf, "< ");
        int i;
		for (i = 0; i <= p->sp; i++) {
            // TODO use snprintf and check length written to avoid overflow
			if (i == p->sp) {
				len += sprintf(buf + len, "| %i >", p->stack[i]);
			} else {
				len += sprintf(buf + len, "%i ", p->stack[i]);
			}
		}
	} else {
		sprintf(buf, "< >");
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
    
    nextProcess->ip = LAST_ENTRY;
    
    log_debug(LOG, "new task %s (P%i)", nextProcess->name, priority);
    
    return nextProcess;
}

/*
 * Find the next task to execute. This is the highest priority task that has a next run
 * time that is less than the current time. If such a process exists then the waiting
 * flag is cleared.
 */
void next_task()
{
    waiting = true;
    struct Process* next = processes;
    do {
        if (next->ip != LAST_ENTRY && next->next_time_to_run <= timer) {
            process = next;
            waiting = false;
            log_trace(LOG, "switch to process %s\n", process->name);
            break;
        }
        next = next->next;
    } while (next != NULL);
}

void tasks()
{
    struct Process* p = processes;
    printf("Time %i\n", timer);
    do {
        char buf[64];
        dump_return_stack(buf, process);
        printf("Task #%i%s %s (P%i) ~%04X, next %i  %s ", 
                p->id, p == process ? "*" : "", p->name, p->priority,  
                p->ip, p->next_time_to_run, buf);
        dump_parameter_stack(buf, p);
        printf("%s\n", buf);
        p = p->next;
    } while (p != NULL);
}

void dump()
{
    if (process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    CELL length = process->stack[process->sp--];
    CELL code_index = process->stack[process->sp--];
    dictionary_memory_dump(code_index, length);
}

static void dump_base()
{
    dictionary_memory_dump(0, 0x200);
}

void aborted() 
{
    // TODO kill process
    printf("invalid execution\n");
}

/*
 * Push the address of the word in the dictionary (the execution token) that 
 * matched the next token on the input 
 */
void tick()
{
    struct Dictionary_Entry entry;
    char name[32];
    parser_next_token();
    parser_token_text(name);    
    
    dictionary_find_entry(name, &entry);
    
    if (entry.instruction == LAST_ENTRY) 
    {
        printf("No entry %s!\n", entry.name);
        aborted();
    }
    else
    {
        PUSH_DATA(entry.instruction);
    }
}

void debug_word()
{ 
    dictionary_debug_entry(pop_stack());
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
    
    dictionary_append_instruction(LIT);
    dictionary_append_value(name[0]);
}

static void set_log_level() 
{
    log_level = process->stack[process->sp--];
}

void load_words()
{
    // Internal codes
//    dictionary_insert_internal_instruction("__PROCESS", exec_process);
    dictionary_insert_internal_instruction(LIT, lit);
    dictionary_insert_internal_instruction(ADDR, address);
    dictionary_insert_internal_instruction(BRANCH, branch);
    dictionary_insert_internal_instruction(ZBRANCH, zero_branch);
    dictionary_insert_internal_instruction(RETURN, return_to);
    dictionary_insert_internal_instruction(PROCESS_LINE, interpreter_run);
    
    
    // words with short codes
    dictionary_add_core_word("DUP", duplicate, false);
    dictionary_add_core_word("SWAP", swap, false);
    dictionary_add_core_word("WORDS", dictionary_words, false);
    dictionary_add_core_word("DUMP", dump, false);

    dictionary_add_core_word("END", end, false);
//        
//    dictionary_add_core_word("__RUN", run, false);
//    dictionary_add_core_word("__PROCESS", exec_process, false);
//    dictionary_add_core_word("__LIT", lit, false);
//    dictionary_add_core_word("__VAR", variable, false);
//    dictionary_add_core_word("__ZBRANCH", zero_branch, false);
//    dictionary_add_core_word("__BRANCH", branch, false);
//    dictionary_add_core_word("__RETURN", return_to, false);
    
    dictionary_add_core_word("OVER", over, false);
    dictionary_add_core_word("DROP", drop, false);
    dictionary_add_core_word("NIP", nip, false);
    dictionary_add_core_word("ROT", rot, false);
    dictionary_add_core_word("TUCK", tuck, false);

    dictionary_add_core_word("+", add, false);
    dictionary_add_core_word("-", subtract, false);
    dictionary_add_core_word("/", divide, false);
    dictionary_add_core_word("*", muliply, false);
    dictionary_add_core_word(">", greater_than, false);
    dictionary_add_core_word("=", equal_to, false);
    dictionary_add_core_word("AND", and, false);
    dictionary_add_core_word("OR", or, false);
    dictionary_add_core_word("XOR", xor, false);
    dictionary_add_core_word("NOT", not, false);
    dictionary_add_core_word("LSHIFT", left_shift, false);
    dictionary_add_core_word("RSHIFT", right_shift, false);
    dictionary_add_core_word(".", print_top_of_stack, false);
    dictionary_add_core_word(".HEX", print_hex, false);
    dictionary_add_core_word("CR", print_cr, false);
    dictionary_add_core_word("EMIT", emit, false);
    dictionary_add_core_word("@", read_memory, false);
    dictionary_add_core_word("!", write_memory, false);
    dictionary_add_core_word("EXECUTE", execute_word, false);
    dictionary_add_core_word("INITIATE", initiate, false);
    dictionary_add_core_word("PAUSE", yield, false);
    dictionary_add_core_word("MS", wait_for, false);
    dictionary_add_core_word(".S", stack, false);
    dictionary_add_core_word("TICKS", ticks, false);
    

    dictionary_add_core_word("'", tick, false);

    
    dictionary_add_core_word("CHAR", push_char, false);

    dictionary_add_core_word("VARIABLE", compiler_variable, false);
    dictionary_add_core_word("CONSTANT", compiler_constant, false);

    dictionary_add_core_word("DBG", debug_word, false);
    dictionary_add_core_word("TASKS", tasks, false);
    dictionary_add_core_word("_DUMP", dump_base, false);
    dictionary_add_core_word("_DEBUG", debug_on, false);
    dictionary_add_core_word("_NODEBUG", debug_off, false);
    dictionary_add_core_word("_RESET", reset, false);
    dictionary_add_core_word("_CLEAR", clear_registers, false);

    
    // Immediate words
    dictionary_add_core_word("\\", compiler_eol_comment, true);
    dictionary_add_core_word("(", compiler_inline_comment, true);
    dictionary_add_core_word("IF", compiler_if, true);
    dictionary_add_core_word("THEN", compiler_then, true);
    dictionary_add_core_word("ELSE", compiler_else, true);
    dictionary_add_core_word("BEGIN", compiler_begin, true);
    dictionary_add_core_word("AGAIN", compiler_again, true);
    dictionary_add_core_word("UNTIL", compiler_until, true);
    dictionary_add_core_word(":", compiler_compile, false);
    dictionary_add_core_word(";", compiler_end ,true);
    dictionary_add_core_word("[CHAR]", compile_char, true);
    
    dictionary_add_core_word("LOG", set_log_level, false);
    dictionary_add_core_word("DICT", dictionary_debug, false);
    
    
    dictionary_add_entry("INTERACTIVE");
    compiler_begin();
    dictionary_append_instruction(PROCESS_LINE);
    compiler_again();
    dictionary_end_entry();
    
        
    dictionary_add_entry("TEST");
    dictionary_append_instruction(1);
    dictionary_append_instruction(2);
    dictionary_append_instruction(1);
    dictionary_append_instruction(0x13);
    dictionary_append_instruction(12);
    dictionary_end_entry();

}