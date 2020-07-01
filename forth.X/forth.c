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

//#define NEXT_INSTRUCTION current_process->code[current_process->ip++]
#define PEEK_DATA current_process->stack[current_process->sp]
#define PUSH_DATA(value) (current_process->stack[++(current_process->sp)] = (value))
#define POP_DATA current_process->stack[current_process->sp--]
#define PUSH_RETURN(address) (current_process->return_stack[++(current_process->rsp)] =  (uint32_t) (address))

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
static void load_words(void);
void reset(void);
void print_top_of_stack(void);
static void return_to(void);
static void abort_task(void);
void lit(void);

static void dump_base(void);
    
uint32_t timer = 0;
static bool waiting = true;

static struct Process* processes;
static struct Process* current_process;
static struct Process* idle_process;
static struct Process* power_process;
static struct Process* interpreter_process;
static uint8_t next_process_id = 0;
static bool in_error;

static CODE_INDEX interpreter_code;  // start of interpreter code
static CODE_INDEX idle_code;

int forth_init()
{   
    parser_init();
    compiler_init();
    
    idle_process = new_task(1, "IDLE");
    power_process = new_task(5, "POWER");
    interpreter_process = new_task(5, "INTERPRETER");
    current_process = interpreter_process;
    
    dictionary_init();
    load_words();
	
    uart_transmit_buffer("FORTH v0.3\n\n");        
}

void process_interrupt(uint8_t level)
{
    log_info(LOG, "restarting interpreter %i", level);
    interpreter_process->sp = -1;
    interpreter_process->rsp = -1;
    interpreter_process->ip = interpreter_code;
    interpreter_process->next_time_to_run = 0;
}

uint8_t find_process(char *name) {
    struct Process* next = processes;
    do {
        if (strcmp(name, next->name) == 0) {
            return next->id;
        }
        next = next->next;
    } while (next != NULL);
    return 0xff;
}

bool stack_underflow()
{
    if (current_process->sp < 0)
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
    return true;
    /*
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
     */
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
        CODE_INDEX instruction = dictionary_read_instruction(current_process);
        forth_execute(instruction);

        // TODO how do we set up the errors? needs to be part of the current_process struct
        if (in_error)
        {
            // TODO extract into abort_task() function
            in_error = false;
            abort_task();
            printf(" ABORTED\n");
        }
    }
}

void start_code(CODE_INDEX at_address)
{
    PUSH_RETURN(current_process->ip);
    // TODO this should be the main current_process only
    interpreter_process->ip = at_address;
}

void forth_execute(CODE_INDEX instruction)
{
    if (dictionary_shortcode(instruction))
    {
        dictionary_execute_function(instruction);
    }
    else
    {
        PUSH_RETURN(current_process->ip);
        if (log_level <= TRACE) 
        {
            char word_name[64];
            dictionary_find_word_for(instruction, word_name);
            log_trace(LOG, "run %s jump to ~%04x return to~%04x", word_name, instruction, current_process->ip);
        }
        current_process->ip = instruction;
    }
}

void execute_next_instruction()
{
    in_error = false;
    
    // TODO move location into trace block
    CODE_INDEX location = current_process->ip; // capture first as can't infer how many bytes it was later on
    
    if (location == LAST_ENTRY) {
        return;
    }
    
    CODE_INDEX instruction = dictionary_read_instruction(current_process);
    if (log_level <= TRACE)
    {   
        char word_name[64];
        dictionary_find_word_for(instruction, word_name);
        log_trace(LOG, "execute #%i ~%04X: %0X: %s", current_process->id, location, instruction, word_name);
    }
    forth_execute(instruction);

    // TODO how do we set up the errors? needs to be part of the current_process struct
    if (in_error)
    {
        in_error = false;
        current_process->sp = -1;
        current_process->rsp = -1;
        current_process->ip = LAST_ENTRY;
        current_process->next_time_to_run = 0;
        next_task();
        printf(" ABORTED\n");
    }
}
    
void push(uint32_t value)
{
    current_process->stack[++(current_process->sp)] = value;
}
    
void duplicate() 
{
    if (current_process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;	
    }
    uint32_t read = PEEK_DATA;
    PUSH_DATA(read);
}

void over()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t nos_value = current_process->stack[current_process->sp - 1];
    PUSH_DATA(nos_value);
}

void drop() 
{
    if (current_process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    current_process->sp--;
}

void nip() 
{
    if (current_process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    current_process->stack[current_process->sp] = tos_value;
}

void swap()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = PEEK_DATA;
    current_process->stack[current_process->sp] = current_process->stack[current_process->sp - 1];
    current_process->stack[current_process->sp - 1] = tos_value;
}

void tuck() 
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp];
    current_process->stack[++(current_process->sp)] = tos_value;
    current_process->stack[current_process->sp - 1] = current_process->stack[current_process->sp - 2];
    current_process->stack[current_process->sp - 2] = tos_value;
}

void rot() 
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    CELL tos_value = current_process->stack[current_process->sp];
    current_process->stack[current_process->sp] = current_process->stack[current_process->sp - 1];
    current_process->stack[current_process->sp - 1] = current_process->stack[current_process->sp - 2];
    current_process->stack[current_process->sp - 2] = tos_value;
}

void lit()
{
    uint32_t tos_value = dictionary_read(current_process);
    PUSH_DATA(tos_value);
}

/*
 * Using the address in the dictionary work out the corresponding memory address
 */
void memory_address()
{
    PUSH_DATA((CELL) dictionary_data_address(current_process->ip));
    return_to();
}

/*
void exec_process()
{
   uint32_t tos_value = current_process->code[current_process->ip++];
   current_process->stack[++(current_process->sp)] = tos_value;
}
*/
void add()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    uint32_t nos_value = current_process->stack[current_process->sp--];
    tos_value = nos_value + tos_value;
    current_process->stack[++(current_process->sp)] = tos_value;
}

void subtract()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    uint32_t nos_value = current_process->stack[current_process->sp--];
    tos_value = nos_value - tos_value;
    current_process->stack[++current_process->sp] = tos_value;
}

void divide()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    uint32_t nos_value = current_process->stack[current_process->sp--];
    tos_value = nos_value / tos_value;
    current_process->stack[++(current_process->sp)] = tos_value;
}

void muliply() 
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    uint32_t nos_value = current_process->stack[current_process->sp--];
    tos_value = nos_value * tos_value;
    current_process->stack[++(current_process->sp)] = tos_value;
}

void greater_than()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    uint32_t nos_value = current_process->stack[current_process->sp--];
    tos_value = nos_value > tos_value ? 1 : 0;
    current_process->stack[++(current_process->sp)] = tos_value;
}

void equal_to()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    uint32_t nos_value = current_process->stack[current_process->sp--];
    tos_value = nos_value == tos_value ? 1 : 0;
    current_process->stack[++(current_process->sp)] = tos_value;
}

void and() 
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    uint32_t nos_value = current_process->stack[current_process->sp--];
    tos_value = nos_value &tos_value;
    current_process->stack[++(current_process->sp)] = tos_value;
}

void or()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    uint32_t nos_value = current_process->stack[current_process->sp--];
    tos_value = nos_value | tos_value;
    current_process->stack[++(current_process->sp)] = tos_value;
}

void xor()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    uint32_t nos_value = current_process->stack[current_process->sp--];
    tos_value = nos_value ^ tos_value;
    current_process->stack[++(current_process->sp)] = tos_value;
}

void not()
{
    if (current_process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    tos_value = tos_value > 0 ? 0 : 1;
    current_process->stack[++(current_process->sp)] = tos_value;
}

void left_shift()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    uint32_t nos_value = current_process->stack[current_process->sp--];
    tos_value = nos_value << tos_value;
    current_process->stack[++(current_process->sp)] = tos_value;
}

void right_shift()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    uint32_t nos_value = current_process->stack[current_process->sp--];
    tos_value = nos_value >> tos_value;
    current_process->stack[++(current_process->sp)] = tos_value;
}

void ticks()
{
    current_process->stack[++(current_process->sp)] = timer;
}

void yield()
{
    wait(0);
}

void wait_for()
{
    if (current_process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t time = current_process->stack[current_process->sp--];
    wait(time);
}

void branch()
{
    CODE_INDEX pos = current_process->ip;
    int8_t relative = dictionary_read_byte(current_process);
    current_process->ip = pos + relative;
}

void zero_branch()
{
    if (current_process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t offset = current_process->stack[current_process->sp--];
    log_trace(LOG, "zbranch for %i -> %s", offset, (offset == 0 ? "zero" : "non-zero"));
    if (offset == 0) {
        branch();
    } else {
        current_process->ip++;
    }
}

void execute_word() 
{
    if (stack_underflow()) {
        return;
    }
    CODE_INDEX instruction = (CODE_INDEX) current_process->stack[current_process->sp--];
    log_debug(LOG, "execute from %i", instruction);
    current_process->return_stack[++(current_process->rsp)] = (CELL) current_process->ip + 1;
    current_process->ip = instruction;
}
 
static void initiate()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    CELL id = current_process->stack[current_process->sp--];
    CODE_INDEX instruction = (CODE_INDEX) current_process->stack[current_process->sp--];
    struct Process *run_process = processes;
    do {
        if (run_process->id == id) {
            run_process->ip = instruction;
            run_process->next_time_to_run = timer + 1;
            log_info(LOG, "initiate from %04x with %s at %i", run_process->ip, run_process->name, run_process->next_time_to_run);
            break;
        }
        run_process = run_process->next;
    } while (run_process != NULL);
    if (run_process == NULL) {
        log_error(LOG, "no current_process with ID %i", id);                
    }
//    if (trace) {
//        tasks();
//    }
}

static void return_to()
{
    char buf[64];
    if (log_level <= TRACE) 
    {
        dump_return_stack(buf, current_process);
        log_trace(LOG, "return %s", buf);
    }
    if (current_process->rsp < 0) {
        // copy of END code - TODO refactor
        if (log_level <= TRACE) 
        {
            dump_parameter_stack(buf, current_process);
            log_trace(LOG, "  %s", buf);
        }
        current_process->ip = LAST_ENTRY;
        current_process->next_time_to_run = 0;
        next_task();
        log_trace(LOG, "end, go to %s", current_process->name);
    } else {
        current_process->ip = (CODE_INDEX) current_process->return_stack[current_process->rsp--];
    }
}

void print_top_of_stack()
{
    if (current_process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t tos_value = current_process->stack[current_process->sp--];
    printf("%i ", tos_value);
}

void print_line()
{
    printf("\n");
}

void emit()
{
    if (current_process->sp < 0) {
        printf("stack underflow; aborting\n");
        return;
    }
    uint32_t ch = current_process->stack[current_process->sp--];
    printf("%c", ch);
}

void read_memory()
{
    if (current_process->sp < 0) {
        printf("stack underflow; aborting\n");
        in_error = true;
        return;
    }

    uint32_t address = current_process->stack[current_process->sp--];
    log_trace(LOG, "address 0x%x", address);
    if (isAccessibleMemory(address)) 
    {
        uint32_t *ptr = (uint32_t *) address;
        uint32_t value = (uint32_t) *ptr;
        log_debug(LOG, "value at %0X = 0x%x", address, value);
        current_process->stack[++(current_process->sp)] = value;
    }
    else 
    {
        in_error = true;
    }
}

void write_memory()
{
    if (current_process->sp < 0) {
        printf("stack underflow; aborting\n");
        in_error = true;
        return;
    }

    uint32_t address = current_process->stack[current_process->sp--]; // address
    uint32_t value = current_process->stack[current_process->sp--]; // write value
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
    uint32_t value = current_process->stack[current_process->sp--];
    printf("0x%X", value);
}

inline void print_cr() 
{
    printf("\n");
}

void stack()
{
    char buf[64];
    dump_parameter_stack(buf, current_process);
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
    log_info(LOG, "reseting memory");
    struct Process* next = processes;
    do {
        next->sp = -1;
        next->rsp = -1;
//        if (next != interpreter_process && next != idle_process)
//        {
            next->ip = LAST_ENTRY;
//        }
        next = next->next;
    } while (next != NULL);
    idle_process->ip = idle_code;
    interpreter_process->ip = interpreter_code;
    
    
    dictionary_reset();
    tasks();
    dictionary_debug();
    dump_base();
    
    //load_words();
    //dump_base();
}

void clear_registers()
{
    current_process->sp = -1;
    current_process->rsp = -1;
    current_process->ip = LAST_ENTRY;
}

void end()
{
    current_process->ip = LAST_ENTRY;
    current_process->next_time_to_run = 0;
    next_task();
    log_trace(LOG, "end, go to %s", current_process->name);
}

void wait(uint32_t wait_time) {
    current_process->next_time_to_run = timer + wait_time;
    next_task();
}

uint32_t pop_stack()
{
    if (current_process->sp < 0) {
        printf("stack underflow; aborting\n");
        // TODO need to use exception or some other way of dropping out
        return;
    }
    return current_process->stack[current_process->sp--];
}

void dump_return_stack(char *buf, struct Process *p)
{
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

void dump_parameter_stack(char *buf, struct Process *p)
{
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

void to_upper(char *string, int len)
{
    int i;
    for (i = 0; i <= len; i++) {
        if (*string >= 97 && *string <= 122) {
            *string = *string - 32;
        }
        string++;
    }
}

struct Process* new_task(uint8_t priority, char *name)
{
    struct Process* new_process;
    new_process = malloc(sizeof(struct Process));

    if (processes == NULL) 
    {
        processes = new_process;
    }
    else
    {
        struct Process* last_process = processes;
        while (last_process->next != NULL) {
            last_process = last_process->next;
        }
        last_process->next = new_process;
    }
    
    new_process->id = next_process_id++;
    new_process->sp = -1;
    new_process->rsp = -1;
    new_process->ip = LAST_ENTRY;
    new_process->priority = priority;
    new_process->next_time_to_run = 0;
    new_process->name = malloc(sizeof(strlen(name) + 1));
    strcpy(new_process->name, name);
    
    
    log_info(LOG, "new task %s (P%i)", new_process->name, priority);
    
    return new_process;
}

static void add_task()
{
    char name[32];
    parser_next_token();
    parser_token_text(name);    
    new_task(5, name);
    log_info("add task %s", name);
}

static void task_priority()
{
    CELL priority = current_process->stack[current_process->sp--];
    current_process->priority = priority;
    log_info(LOG, "process priority %i now on %s", priority, current_process->name);
}
/*
 * Find the next task to execute. This is the highest priority task that has a next run
 * time that is less than the current time. If such a current_process exists then the waiting
 * flag is cleared.
 */
void next_task()
{
    uint8_t highest_priority = 0;
    
    waiting = true;
    struct Process* next = processes;
    do {
        if (next->ip != LAST_ENTRY && timer >= next->next_time_to_run && next->priority >= highest_priority) {
            highest_priority = next->priority;
            current_process = next;
            waiting = false;
        }
        next = next->next;
    } while (next != NULL);
    if (!waiting) {
        log_trace(LOG, "switch to current_process %s", current_process->name);
    }
}

void tasks()
{
    struct Process* p = processes;
    printf("Time %i\n", timer);
    do {
        char buf[64];
        dump_return_stack(buf, current_process);
        printf("Task #%i%s %s (P%i) %04X, next %i  %s ", 
                p->id, p == current_process ? "*" : "", p->name, p->priority,  
                p->ip, p->next_time_to_run, buf);
        dump_parameter_stack(buf, p);
        printf("%s\n", buf);
        p = p->next;
    } while (p != NULL);
}

void dump()
{
    if (current_process->sp < 1) {
        printf("stack underflow; aborting\n");
        return;
    }
    CELL length = current_process->stack[current_process->sp--];
    CODE_INDEX code_index = (CODE_INDEX) current_process->stack[current_process->sp--];
    dictionary_memory_dump(code_index, length);
}

static void dump_base()
{
    dictionary_memory_dump(0, 0x200);
}

static void abort_task() 
{
    log_debug("abort task %s", current_process->name);
    current_process->sp = -1;
    current_process->rsp = -1;
    current_process->ip = 0;
    current_process->ip = (current_process == interpreter_process) ? interpreter_code : (CODE_INDEX) LAST_ENTRY;
    current_process->next_time_to_run = 0;
    next_task();
}

/*
 * Push the address of the word in the dictionary (the execution token) that 
 * matched the next token on the input 
 */
static void tick()
{
    struct Dictionary_Entry entry;
    char name[32];
    parser_next_token();
    parser_token_text(name);    
    
    dictionary_find_entry(name, &entry);
    
    if (entry.instruction == LAST_ENTRY) 
    {
        printf("No entry %s!\n", entry.name);
        abort_task();
    }
    else
    {
        PUSH_DATA((CELL) entry.instruction);
    }
}

static void debug_word()
{ 
    dictionary_debug_entry((CODE_INDEX) pop_stack());
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
    
    dictionary_append_instruction((CODE_INDEX) LIT);
    dictionary_append_value(name[0]);
}

static void set_log_level() 
{
    log_level = current_process->stack[current_process->sp--];
}

static void question_dup() 
{
    if (PEEK_DATA != 0) 
    {
        duplicate();
    }
}

/*
 Push the depth of the stack (the number of items on it) onto the stack.
 */
static void depth() 
{
    PUSH_DATA(current_process->sp);
}

/* 
 Place a copy of the item at the specified level (from TOS) on to the top of the stack.
 */
static void pick()
{
    uint8_t level = POP_DATA - 1;
    PUSH_DATA(current_process->stack[current_process->sp - level]);
}

/*
 Remove the top cell pair from the stack.
 */
static void two_drop()
{
   current_process->sp -= 2;
}

/*
 Make a copy the top cell pair onto TOS.
 */
static void two_dup()
{
    uint32_t tos = current_process->stack[current_process->sp];
    uint32_t nos = current_process->stack[current_process->sp - 1];
    PUSH_DATA(nos);
    PUSH_DATA(tos);
}

/*
 * In a two cell pair stack copy the bottom pair to the TOS.
 */
static void two_over() {
    PUSH_DATA(current_process->stack[current_process->sp - 3]);  
    PUSH_DATA(current_process->stack[current_process->sp - 3]);  
}

/*
 * In a two cell pair stack copy the bottom pair to the TOS.
 */
static void two_rot() {
    
    CELL tos = current_process->stack[current_process->sp];
    CELL nos = current_process->stack[current_process->sp - 1];
    
    current_process->stack[current_process->sp] = current_process->stack[current_process->sp - 2];
    current_process->stack[current_process->sp - 1] = current_process->stack[current_process->sp - 3];
    current_process->stack[current_process->sp - 2] = current_process->stack[current_process->sp - 4];
    current_process->stack[current_process->sp - 3] = current_process->stack[current_process->sp - 5];
    
    current_process->stack[current_process->sp - 4] = tos;
    current_process->stack[current_process->sp - 5] = nos;
}

/*
 * In a two cell pair stack copy the bottom pair to the TOS.
 */
static void two_swap() {
    CELL tos = current_process->stack[current_process->sp - 0];
    CELL nos = current_process->stack[current_process->sp - 1];
    
    current_process->stack[current_process->sp - 0] = current_process->stack[current_process->sp - 2];
    current_process->stack[current_process->sp - 1] = current_process->stack[current_process->sp - 3];
    
    current_process->stack[current_process->sp - 2] = tos;
    current_process->stack[current_process->sp - 3] = nos;
}

static void load_words()
{
    log_info(LOG, "load words");
    
    // Internal codes
//    dictionary_insert_internal_instruction("__PROCESS", exec_process);
    dictionary_insert_internal_instruction(LIT, lit);
    dictionary_insert_internal_instruction(ADDR, memory_address);
    dictionary_insert_internal_instruction(BRANCH, branch);
    dictionary_insert_internal_instruction(ZBRANCH, zero_branch);
    dictionary_insert_internal_instruction(RETURN, return_to);
    dictionary_insert_internal_instruction(PROCESS_LINE, interpreter_run);
    
    dictionary_add_core_word("END", end, false);
    
    // words with short codes
    dictionary_add_core_word("?DUP", question_dup, false);
    dictionary_add_core_word("DEPTH", depth, false);
    dictionary_add_core_word("DROP", drop, false);
    dictionary_add_core_word("DUP", duplicate, false);
    dictionary_add_core_word("NIP", nip, false);
    dictionary_add_core_word("OVER", over, false);
    dictionary_add_core_word("PICK", pick, false);
    dictionary_add_core_word("ROT", rot, false);
    dictionary_add_core_word("SWAP", swap, false);
    dictionary_add_core_word("TUCK", tuck, false);

    dictionary_add_core_word("2DROP", two_drop, false);
    dictionary_add_core_word("2DUP", two_dup, false);
    dictionary_add_core_word("2OVER", two_over, false);
    dictionary_add_core_word("2ROT", two_rot, false);
    dictionary_add_core_word("2SWAP", two_swap, false);

    
    dictionary_add_core_word("DUMP", dump, false);
    dictionary_add_core_word("WORDS", dictionary_words, false);
//        
//    dictionary_add_core_word("__RUN", run, false);
//    dictionary_add_core_word("__PROCESS", exec_process, false);
//    dictionary_add_core_word("__LIT", lit, false);
//    dictionary_add_core_word("__VAR", variable, false);
//    dictionary_add_core_word("__ZBRANCH", zero_branch, false);
//    dictionary_add_core_word("__BRANCH", branch, false);
//    dictionary_add_core_word("__RETURN", return_to, false);
    

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
    dictionary_add_core_word("TASK", add_task, false);
    dictionary_add_core_word("PRIORITY", task_priority, false);

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
    
    dictionary_add_core_word("DRESET", dictionary_reset, false);
    dictionary_add_core_word("LOCK", dictionary_lock, false);
    dictionary_add_core_word("ABORT", abort_task, false);


    // create loop with process instruction
    dictionary_add_entry("_INTERACTIVE");
    compiler_begin();
    dictionary_append_instruction((CODE_INDEX) PROCESS_LINE);
    compiler_again();
    dictionary_end_entry();
    
    // create loop with pause instruction
    dictionary_add_entry("_IDLE");
    compiler_begin();
    struct Dictionary_Entry entry;
    dictionary_find_entry("PAUSE", &entry);
    dictionary_append_instruction(entry.instruction);
    compiler_again();
    dictionary_end_entry();

    
    
        
 //   struct Dictionary_Entry entry;
    dictionary_find_entry("_INTERACTIVE", &entry);
    interpreter_code = entry.instruction;
    interpreter_process->ip = interpreter_code;
    dictionary_debug_entry(entry.instruction);

    dictionary_find_entry("_IDLE", &entry);
    idle_code = entry.instruction;
    idle_process->ip = idle_code;
    dictionary_debug_entry(entry.instruction);

    dictionary_lock();
}