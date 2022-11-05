#ifdef __XC32
#include <xc.h>          /* Defines special funciton registers, CP0 regs  */
#endif

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <GenericTypeDefs.h>
//#include <proc/p32mx270f256d.h>
#include <cp0defs.h>
#include <plib.h>

#include "logger.h"
#include "parser.h"
#include "interpreter.h"
#include "forth.h"
#include "code.h"
// TODO is this still needed?
#include "compiler.h"
#include "dictionary.h"
#include "uart.h"
#include "timer.h"

#define PEEK_DATA current_process->stack[current_process->sp]
#define PUSH_DATA(value) push(value)
#define POP_DATA pop_stack() 
#define POP_2DATA pop_double()
#define PUSH_RETURN(address) (current_process->return_stack[++(current_process->rsp)] =  (uint32_t) (address))

#define BASE_HEX 16
#define BASE_DEC 10
#define BASE_OCT 8
#define BASE_BIN 2

#define LOG "Forth"

void dump_parameter_stack(char *, struct Process*);
void dump_return_stack(char *, struct Process*);
//void execute_next_instruction(void);
void display_code(uint8_t*);
CELL pop_stack(void);
void next_task();
static struct Process* new_task(uint8_t, char*);
static void load_words(void);
void reset(void);
static void print_top_of_stack(void);
static void abort_task(struct Process*);
void lit(void);

static void dump_base(void);
static void read_memory(void);
static void read_char(void);
static void two_drop(void);
static void type(void);
static void debug_print_stack(void);
static void print_task(struct Process*);
static void tasks(void);
    
volatile uint32_t timer = 0;
uint32_t base_no = BASE_DEC;
static bool waiting = true;
static bool in_error;
bool trace_code = false;

struct Process* current_process;
struct Process* return_to_process;
static struct Process* processes;
static struct Process* idle_process;
static struct Process* power_process;
static struct Process* interpreter_process;
static struct Process* interrupt_process;
static uint8_t next_process_id = 0;

static CODE_INDEX interpreter_code;  // start of interpreter code
static CODE_INDEX idle_code;
static CODE_INDEX int_return_code;
static CODE_INDEX interrupts[43];


bool debug = false;


int forth_init()
{   
    parser_init();
    compiler_init();
    
    interrupt_process = new_task(10, "IRQ");
    interrupt_process->log = true;
    interpreter_process = new_task(5, "INTERP");
    interpreter_process->log = true;
    interpreter_process->suspended = false;
    current_process = interpreter_process;
    current_process->log = true;
    idle_process = new_task(1, "IDLE");
    idle_process->suspended = false;
    idle_process->log = false;
    power_process = new_task(5, "POWER");
    power_process->log = false;

    uart_transmit_buffer("FORTH v0.4\n\n");        
    
    dictionary_init();
    load_words();
    dictionary_init_done();
    
    log_info(LOG, "loaded initial words");
    
//    tasks();
}

void forth_trace(bool trace) {
    trace_code = debug && trace;
}

void process_abort(uint8_t level)
{
    log_info(LOG, "restarting interpreter %I", level);
    abort_task(interpreter_process);   
}

uint8_t find_process(char *name) {
    struct Process* next = processes;
    do {
        if (strcicmp(name, next->name) == 0) {
            return next->id;
        }
        next = next->next;
    } while (next != NULL);
    return 0xff;
}
    
static void push_char()
{
    char text[32];
    parser_next_text(text);    

    PUSH_DATA(text[0]);
}

static void push_blank()
{
    PUSH_DATA(0x20);
}

void push(CELL value)
{
     // PUSH_DATA(value);
    (current_process->stack[++(current_process->sp)] = (value));
}

void push_double(SIGNED_DOUBLE value)
{
    PUSH_DATA(value & 0xffffffff);
    PUSH_DATA(value >> 32);
} 

void push_unsigned_double(UNSIGNED_DOUBLE value)
{
    PUSH_DATA(value & 0xffffffff);
    PUSH_DATA(value >> 32);
} 

SIGNED_DOUBLE pop_double()
{
    return (((SIGNED_DOUBLE) current_process->stack[current_process->sp--]) << 32)
            + current_process->stack[current_process->sp--];
}

bool stack_underflow()
{
    if (current_process->sp < -1)
    {
        console_out("stack underflow; aborting\n");
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * Check for valid addresses in REGISTERs or RAM
 */
static bool is_accessible_memory(uint32_t address)
{
    if ((address >= 0xBF800000 && address <= 0xBF8FFFFF) ||
            (address >= 0xA0000000 && address <= 0xA000FFFF) ||
            (address >= 0x80000000 && address <= 0x8000FFFF)) 
    {
        if (address % 4 != 0)
        {
            console_out("NON-ALIGNED %Z!", address); // reading from non-aligned address causes PIC exception
            return false;        
        }
        else 
        {
            return true;
        }
    } 
    else
    {
        console_out("MEMORY LIMIT %Z!", address);
        return false;
    }
}

static void test_compile(char * input) {
    parser_input(input);
    compiler_compile_definition();
}

void forth_run()
{
    uint8_t level = 0;
    
    in_error = false;
    while (true)
    {
            
//    processes = NULL;

    /*    
//        if (trace_code) {
//    //        char word_name[64];
//    //        dictionary_find_word_for(instruction, word_name);
//    //        log_trace(LOG, "execute #%I %Z: %S", current_process->id, instruction, word_name);
//            
//            char buf[100];
//            dump_parameter_stack(buf, current_process);
//            console_out("%S  %Z ", buf, current_process->ip);
//
//            dictionary_print_instruction(current_process->ip);
//            
//            console_put(NL);
//        }
        */
        
    
         if (trace_code) {
    //        char word_name[64];
    //        dictionary_find_word_for(instruction, word_name);
    //        log_trace(LOG, "execute #%I %Z: %S", current_process->id, instruction, word_name);
            if (current_process->ip == interpreter_code + 1) {
                trace_code = false;
            } else {
                char buf[100];
                uint32_t restore_to = base_no;
                base_no = BASE_HEX;
                dump_parameter_stack(buf, current_process);
                console_out("%S  ", buf);
                dump_return_stack(buf, current_process);
                base_no = restore_to; 
                console_out("%S\n", buf);
                console_pad(8);

                CODE_INDEX ip = current_process->ip;
                console_pad(level * 2);
                int8_t change = dictionary_print_instruction(&ip);
                if (change == 1)
                {
                    console_put(NL);
                    console_pad(level * 2);
                    console_out(">>");
                }
                else if (change == -1)
                {
                    struct Dictionary_Entry e;
                    dictionary_find_entry_with(ip, &e);
                    console_put(NL);
                    console_pad(level * 2);
                    console_out("<< (%S)", e.name);
                }
                level += change;
            }
            console_put(NL);
        }
    

        CODE_INDEX instruction = dictionary_read_instruction(current_process);
        forth_execute(instruction);

        // TOD check this is how is still works. Think not!
        // TODO how do we set up the errors? needs to be part of the current_process struct
        if (in_error)
        {
            // TODO extract into abort_task() function
            in_error = false;
            forth_abort();
            console_out(" ABORTED\n");
        }
    }
}
//
//void start_code(CODE_INDEX at_address)
//{
//    PUSH_RETURN(current_process->ip);
//    // TODO this should be the main current_process only
//    interpreter_process->ip = at_address;
//}

void forth_execute(CODE_INDEX instruction_pointer)
{
    /*
//    if (trace_code && current_process == interpreter_process) {
//        
//        
//        0bf80f230 CONSTANT SYSKEYtask
//        
////        char word_name[64];
////        CODE_INDEX entry = dictionary_find_word_for(instruction_pointer, word_name);
//
//        
////        char buf[100];
//       // dump_parameter_stack(buf, current_process);
////        char *buf = "< ?? >";
//        
//        log_info(LOG, "instruction pointer #%I: %Z:", current_process->id, instruction_pointer);
//
////        
//////        CODE_INDEX location = current_process->ip;
//        dictionary_print_instruction(instruction_pointer); 
//        //2 logconsole_put(NL);
//
//        
//        
//        
//    }
    
//    CODE_INDEX instruction_pointer = dictionary_read_instruction(current_process);
//    if (log_level == INFO && current_process == interpreter_process)
//    { 
//        dictionary_print_instruction(current_process->ip); 
//    }
    
    
//    
//    
//         if (trace_code) {
//    //        char word_name[64];
//    //        dictionary_find_word_for(instruction, word_name);
//    //        log_trace(LOG, "execute #%I %Z: %S", current_process->id, instruction, word_name);
//            
//            char buf[100];
//            dump_parameter_stack(buf, current_process);
//            console_out("%S  %Z ", buf, instruction_pointer);
//
//            CODE_INDEX test = instruction_pointer;
//            dictionary_print_instruction(test);
//            
//            console_put(NL);
//        }
//    
    */
    
    if (dictionary_shortcode(instruction_pointer))
    {
        dictionary_execute_function(instruction_pointer);
    }
    else
    {
        PUSH_RETURN(current_process->ip);
        if (log_level <= TRACE) 
        {
            char word_name[64];
            dictionary_find_word_for(instruction_pointer, word_name);
            log_trace(LOG, "run %S jump to %Z return to %Z", word_name, instruction_pointer, current_process->ip);
        }
        current_process->ip = instruction_pointer;
    }
    if (stack_underflow()) {
        forth_abort();
    }
}

//void execute_next_instruction()
//{
////    in_error = false;
////    
////    // TODO move location into trace block
////    CODE_INDEX location = current_process->ip; // capture first as can't infer how many bytes it was later on
////    
////    if (location == LAST_ENTRY) {
////        return;
////    }
////    
////    CODE_INDEX instruction = dictionary_read_instruction(current_process);
////    if (log_level == INFO && current_process == interpreter_process)
////    { 
////        dictionary_print_instruction(current_process->ip); 
////    }
////    
//////    if (log_level <= TRACE)
//////    {   
////        char word_name[64];
////        dictionary_find_word_for(instruction, word_name);
////        log_trace(LOG, "execute #%I ~%Z: %Z: %S", current_process->id, location, instruction, word_name);
//////    }
////        console_out("execute!\n");
////    forth_execute(instruction);
////
////    // TODO how do we set up the errors? needs to be part of the current_process struct
////    if (in_error)
////    {
////        in_error = false;
////        current_process->sp = -1;
////        current_process->rsp = -1;
////        current_process->ip = LAST_ENTRY;
////        current_process->next_time_to_run = 0;
////        next_task();
////        console_out(" ABORTED\n");
////    }
//}
//   


void forth_interrupt(uint8_t vector)
{
    CODE_INDEX ip = interrupts[vector]; // code to run TODO set up new process
    
    if (ip != NULL) {
        //log_info(LOG, "INT processing %I -> %Z", vector, ip);

        // TODO create 1 process for each interrupt level and use according to priority
        interrupt_process->sp = -1;
        interrupt_process->rsp = -1;
        interrupt_process->ip = ip; 
        interrupt_process->suspended = false;
        interrupt_process->activations++;
        interrupt_process->next_time_to_run = timer; // + 1000;

        return_to_process = current_process;
        current_process = interrupt_process;
        
        
        while (true)
        {
            CODE_INDEX instruction = dictionary_read_instruction(current_process);
            if (instruction == int_return_code && current_process->rsp == -1) {
                current_process->suspended = true;
//                current_process->ip = LAST_ENTRY;
                tasks();
                current_process = return_to_process;
                break;
            }
            forth_execute(instruction);
            
//            // TOD check this is how is still works. Think not!
//            // TODO how do we set up the errors? needs to be part of the current_process struct
//            if (in_error)
//            {
//                // TODO extract into abort_task() function
//                in_error = false;
//                forth_abort();
//                console_out(" ABORTED\n");
//            }
//            
        }
    }
}
//    
//    processes = NULL;

//static void interrupt_return() {
//   log_info(LOG, "INT RET!");
//}

static void write_mask(uint32_t address, CELL mask, bool set)
{
    *((uint32_t *) (address | (set ? 0x08 : 0x04))) = mask;
}

static void interrupt_register()     
//    processes = NULL;

{
    CELL code = POP_DATA;
    CELL vector = POP_DATA;
    interrupts[vector] = (CODE_INDEX)code;
}

static void interrupt_priority() 
{
    CELL vector = POP_DATA;
    CELL priority = (POP_DATA << 2) | POP_DATA;
    CELL offset = vector / 0x04 * 0x10;
    CELL addr = ((CELL) &IPC0) + offset;
    CELL mask = priority << (vector % 4 * 8);
    write_mask(addr, mask, true);
}

static CELL bit_mask(CELL position) {
    return 1 << (position % 0x20);
}

static CELL reg_addr(CELL addr, CELL position) {
    CELL offset = position / 0x20 * 0x10;
    return addr + offset;
}

static void interrupt_disable() 
{
    CELL source = POP_DATA;
    CELL addr = reg_addr(((CELL) &IEC0), source);
    CELL mask = bit_mask(source);
    write_mask(addr, mask, false);
}

static void interrupt_enable() 
{
    CELL source = POP_DATA;
    CELL addr = reg_addr(((CELL) &IEC0), source);
    CELL mask = bit_mask(source);
    write_mask(addr, mask, true);
}

static void interrupt_status() 
{
    CELL source = POP_DATA;
    CELL addr = reg_addr(((CELL) &IFS0), source);
    CELL mask = bit_mask(source);
    log_info(LOG, "int status %Z mask %Y", addr, mask);
    CELL read = *((uint32_t *) addr) & mask;
    PUSH_DATA(read == mask);
}

static void interrupt_clear() 
{
    CELL source = POP_DATA;
    CELL addr = reg_addr(((CELL) &IFS0), source);
    CELL mask = bit_mask(source);
    write_mask(addr, mask, false);
}

static void duplicate() 
{
    CELL read = PEEK_DATA;
    PUSH_DATA(read);
}

static void over()
{
    CELL value = current_process->stack[current_process->sp - 1];
    PUSH_DATA(value);
}

void drop() 
{
//    if (current_process->sp < 0) {
//        console_out("stack underflow; aborting\n");
//        return;
//    }
    current_process->sp--;
}

void nip() 
{
    if (current_process->sp < 0) {
        console_out("stack underflow; aborting\n");
        return;
    }
    CELL tos_value = current_process->stack[current_process->sp--];
    current_process->stack[current_process->sp] = tos_value;
}

void swap()
{
    if (current_process->sp < 1) {
        console_out("stack underflow; aborting\n");
        return;
    }
    CELL tos_value = PEEK_DATA;
    current_process->stack[current_process->sp] = current_process->stack[current_process->sp - 1];
    current_process->stack[current_process->sp - 1] = tos_value;
}

void tuck() 
{
    if (current_process->sp < 1) {
        console_out("stack underflow; aborting\n");
        return;
    }
    CELL tos_value = current_process->stack[current_process->sp];
    current_process->stack[++(current_process->sp)] = tos_value;
    current_process->stack[current_process->sp - 1] = current_process->stack[current_process->sp - 2];
    current_process->stack[current_process->sp - 2] = tos_value;
}

void rot() 
{
    CELL value = current_process->stack[current_process->sp - 2];
    current_process->stack[current_process->sp - 2] = current_process->stack[current_process->sp - 1];
    current_process->stack[current_process->sp - 1] = current_process->stack[current_process->sp];
    current_process->stack[current_process->sp] = value;
}

void lrot() 
{
    CELL tos = current_process->stack[current_process->sp];
    current_process->stack[current_process->sp] = current_process->stack[current_process->sp - 1];
    current_process->stack[current_process->sp - 1] = current_process->stack[current_process->sp - 2];
    current_process->stack[current_process->sp - 2] = tos;
}

/*
 Do nothing.
 */
void nop() {
}

/*
 Read the next cell from the dictionary and push it onto the stack.
 */
void push_literal()
{
    CELL tos_value = dictionary_read(current_process);
    PUSH_DATA(tos_value);
}

void push_double_literal()
{
    uint64_t value = dictionary_read(current_process);
    push_double(value);
}

/*
 * Using the address in the dictionary to work out the corresponding memory address
 */
void memory_address()
{
    PUSH_DATA((CELL) dictionary_aligned(current_process->ip));
    return_to();
}

/*
 * Pushes the address of the data (in the current entry) onto the stack.
*/ 
void data_address()
{
    PUSH_DATA((CELL) dictionary_aligned(current_process->ip));
    return_to();
}

static void add()
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    tos_value = nos_value + tos_value;
    PUSH_DATA(tos_value);
}

static void double_add()
{
    SIGNED_DOUBLE tos_value = POP_2DATA;
    SIGNED_DOUBLE nos_value = POP_2DATA;
    tos_value = nos_value + tos_value;
    push_double(tos_value);
}

static void mixed_add()
{
    SIGNED tos_value = POP_DATA;
    SIGNED_DOUBLE nos_value = POP_2DATA;
    push_double(nos_value + tos_value);
}

static void add_1()
{
    CELL tos_value = POP_DATA;
    tos_value = 1 + tos_value;
    PUSH_DATA(tos_value);
}

static void add_2()
{
    CELL tos_value = POP_DATA;
    tos_value = 2 + tos_value;
    PUSH_DATA(tos_value);
}

static void subtract()
{
    CELL tos_value = POP_DATA;
    CELL nos_value = POP_DATA;
    tos_value = nos_value - tos_value;
    PUSH_DATA(tos_value);
}

static void double_subtract()
{
    SIGNED_DOUBLE tos_value = POP_2DATA;
    SIGNED_DOUBLE nos_value = POP_2DATA;
    tos_value = nos_value - tos_value;
    push_double(tos_value);
}

static void subtract_1()
{
    CELL tos_value = POP_DATA;
    tos_value = tos_value - 1;
    PUSH_DATA(tos_value);
}

static void subtract_2()
{
    CELL tos_value = POP_DATA;
    tos_value = tos_value - 1;
    PUSH_DATA(tos_value);
}

static void negate()
{
    CELL tos_value = POP_DATA;
    tos_value = ~tos_value + 1;
    PUSH_DATA(tos_value);
}

static void double_negate()
{
    SIGNED_DOUBLE tos_value = POP_2DATA;
    tos_value = ~tos_value + 1;
    push_double(tos_value);
}

static void divide()
{
    CELL tos_value = POP_DATA;
    CELL nos_value = POP_DATA;
    tos_value = nos_value / tos_value;
    PUSH_DATA(tos_value);
}

static void mixed_divide()
{
    SIGNED tos_value = POP_DATA;
    SIGNED_DOUBLE nos_value = POP_DATA;
    tos_value = nos_value / tos_value;
    PUSH_DATA(tos_value);
}

static void multiply() 
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    tos_value = nos_value * tos_value;
    PUSH_DATA(tos_value);
}

static void mixed_multiply() 
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    SIGNED_DOUBLE result = nos_value * tos_value;
    push_double(result);
}

static void unsigned_multiply() 
{
    UNSIGNED tos_value = POP_DATA;
    UNSIGNED nos_value = POP_DATA;
    tos_value = nos_value * tos_value;
    PUSH_DATA(tos_value);}

static void mod() 
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    PUSH_DATA(nos_value % tos_value);
}

static void divide_mod() 
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    PUSH_DATA(nos_value % tos_value);
    PUSH_DATA(nos_value / tos_value);
}

static void unsigned_divide_mod() 
{
    UNSIGNED tos_value = POP_DATA;
    UNSIGNED nos_value = POP_DATA;
    PUSH_DATA(nos_value % tos_value);
    PUSH_DATA(nos_value / tos_value);
}

static void multiply_divide()
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    SIGNED los_value = POP_DATA;
    SIGNED_DOUBLE result = los_value * nos_value;
    PUSH_DATA((SIGNED) (result / tos_value));
}

static void mixed_multiply_divide()
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    SIGNED_DOUBLE los_value = POP_2DATA;
    SIGNED_DOUBLE result = los_value * nos_value;
    push_double(result / tos_value);
}

static void absolute() 
{
    SIGNED tos_value = POP_DATA;
    PUSH_DATA(abs(tos_value));
}

static void double_absolute() 
{
    SIGNED_DOUBLE tos_value = POP_2DATA;
    push_double(abs(tos_value));
}

static void single_max()
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    PUSH_DATA(nos_value > tos_value ? nos_value : tos_value);
}

static void double_max()
{
    SIGNED_DOUBLE tos_value = POP_2DATA;
    SIGNED_DOUBLE nos_value = POP_2DATA;
    push_double(nos_value > tos_value ? nos_value : tos_value);
}

static void single_min()
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    PUSH_DATA(nos_value < tos_value ? nos_value : tos_value);
}

static void double_min()
{
    SIGNED_DOUBLE tos_value = POP_2DATA;
    SIGNED_DOUBLE nos_value = POP_2DATA;
    push_double(nos_value < tos_value ? nos_value : tos_value);
}

static void greater_than()
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    tos_value = nos_value > tos_value ? 1 : 0;
    PUSH_DATA(tos_value);
}

static void greater_than_equal()
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    tos_value = nos_value >= tos_value ? 1 : 0;
    PUSH_DATA(tos_value);
}

static void less_than()
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    tos_value = nos_value < tos_value ? 1 : 0;
    PUSH_DATA(tos_value);
}

static void less_than_equal()
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    tos_value = nos_value <= tos_value ? 1 : 0;
    PUSH_DATA(tos_value);
}

static void equal_to()
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    tos_value = nos_value == tos_value ? 1 : 0;
    PUSH_DATA(tos_value);
}

static void not_equal_to()
{
    SIGNED tos_value = POP_DATA;
    SIGNED nos_value = POP_DATA;
    tos_value = nos_value != tos_value ? 1 : 0;
    PUSH_DATA(tos_value);
}

static void equal_to_zero()
{
    SIGNED value = POP_DATA;
    value = value == 0 ? 1 : 0;
    PUSH_DATA(value);
}

static void not_equal_to_zero()
{
    SIGNED value = POP_DATA;
    value = value != 0 ? 1 : 0;
    PUSH_DATA(value);
}

static void greater_than_zero()
{
    SIGNED value = POP_DATA;
    value = value > 0 ? 1 : 0;
    PUSH_DATA(value);
}

static void less_than_zero()
{
    SIGNED value = POP_DATA;
    value = value < 0 ? 1 : 0;
    PUSH_DATA(value);
}

void and() 
{
    CELL tos_value = POP_DATA;
    CELL nos_value = POP_DATA;
    tos_value = nos_value &tos_value;
    PUSH_DATA(tos_value);
}

void or()
{
    if (current_process->sp < 1) {
        console_out("stack underflow; aborting\n");
        return;
    }
    CELL tos_value = POP_DATA;
    CELL nos_value = POP_DATA;
    tos_value = nos_value | tos_value;
    PUSH_DATA(tos_value);
}

void xor()
{
    if (current_process->sp < 1) {
        console_out("stack underflow; aborting\n");
        return;
    }
    CELL tos_value = POP_DATA;
    CELL nos_value = POP_DATA;
    tos_value = nos_value ^ tos_value;
    PUSH_DATA(tos_value);
}

void not()
{
    CELL tos_value = POP_DATA;
    tos_value = tos_value > 0 ? 0 : 1;
    PUSH_DATA(tos_value);
}

static void left_shift()
{
    CELL tos_value = POP_DATA;
    CELL nos_value = POP_DATA;
    tos_value = nos_value << tos_value;
    PUSH_DATA(tos_value);
}

static void right_shift()
{
    CELL tos_value = POP_DATA;
    CELL nos_value = POP_DATA;
    tos_value = nos_value >> tos_value;
    PUSH_DATA(tos_value);
}

static void left_shift_1()
{
    CELL tos_value = POP_DATA;
    tos_value = tos_value << 1;
    PUSH_DATA(tos_value);
}

static void right_shift_1()
{
    CELL tos_value = POP_DATA;
    tos_value = tos_value >> 1;
    PUSH_DATA(tos_value);
}

static void ticks()
{
    PUSH_DATA(timer);
}

static void time()
{
    uint32_t count = _CP0_GET_COUNT();
//    uint32_t compare = _CP0_GET_COMPARE();
//    log_info(LOG, "core timer count %I =? %I", count, compare);
    PUSH_DATA(count / (CORE_TIMER_INTERVAL / 1000));
}

void yield()
{
    wait(0);
}

void wait_for()
{
    uint32_t time = POP_DATA;
    wait(time);
}

void branch()
{
    CODE_INDEX pos = current_process->ip;
    int8_t relative = dictionary_read_next_byte(current_process);
    current_process->ip = pos + relative;
}

void zero_branch()
{
    uint32_t offset = POP_DATA;
    log_trace(LOG, "zbranch for %I -> %S", offset, (offset == 0 ? "zero" : "non-zero"));
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
    log_debug(LOG, "execute from %Z", instruction);
    current_process->return_stack[++(current_process->rsp)] = (CELL) current_process->ip;
    current_process->ip = instruction;
}


static struct Process* get_process()
{
    if (current_process->sp < 0) {
        console_out("stack underflow; aborting\n");
        return NULL;
    }
    CELL id = current_process->stack[current_process->sp--];
    struct Process *process = processes;
    do {
        if (process->id == id) {
            return process;
        }
        process = process->next;
    } while (process != NULL);
    if (process == NULL) {
        log_error(LOG, "no current_process with ID %I", id);                
    }
    return NULL;
}

static void initiate()
{
    struct Process *run_process = get_process();
    if (run_process != NULL) {
        CODE_INDEX instruction = (CODE_INDEX) current_process->stack[current_process->sp--];
        run_process->suspended = false;
        run_process->ip = instruction;
        run_process->next_time_to_run = timer + 1;
        log_info(LOG, "initiate from %Y with %S at %I", run_process->ip, run_process->name, run_process->next_time_to_run);
    }
}

static void terminate()
{
    struct Process *terminate_process = get_process();
    if (terminate_process)
    {
        terminate_process->suspended = true;
        terminate_process->next_time_to_run = 0;
        terminate_process->ip = BASE_ENTRY;
        terminate_process->sp = -1;
        terminate_process->rsp = -1;
    }
}

static void suspend()
{
    struct Process *suspend_process = get_process();
    if (suspend_process)
    {
        suspend_process->suspended = true;
    }
}

static void resume()
{
    struct Process *resume_process = get_process();
    if (resume_process)
    {
        resume_process->suspended = false;    
    }
}

void return_to()
{
    char buf[64];
    if (log_level <= TRACE) 
    {
        if (log_level <= TRACE) 
        {
            dump_return_stack(buf, current_process);
            log_trace(LOG, "return %S", buf);
            dump_parameter_stack(buf, current_process);
            log_trace(LOG, "  %S", buf);
        }
    }
    if (current_process->rsp < 0) {
        // copy of END code - TODO refactor
        if (log_level <= TRACE) 
        {
            dump_parameter_stack(buf, current_process);
            log_trace(LOG, "  %S", buf);
        }
        current_process->ip = BASE_ENTRY;
        current_process->next_time_to_run = 0;
        next_task();
        log_trace(LOG, "end, go to %S", current_process->name);
    } else {
        current_process->ip = (CODE_INDEX) current_process->return_stack[current_process->rsp--];
    }
}

#define NUMBER_FORMAT_LEN 67

static char number_format[NUMBER_FORMAT_LEN];
static uint8_t format_pos;
    
static void start_format_number()
{
    memset(number_format, 0, NUMBER_FORMAT_LEN);
    format_pos = NUMBER_FORMAT_LEN;
}

static bool add_digit()
{
   UNSIGNED_DOUBLE value = POP_2DATA;
   uint8_t digit = value % base_no;
   number_format[--format_pos] = (digit <= 9) ? ('0' + digit) : ('A' + digit - 10);
     
   value /= base_no;
   push_unsigned_double(value);
   return value > 0;
}

static void add_format_digit()
{
    add_digit();
}

static void add_format_digits()
{
    while (add_digit()) {}
}

static void add_format_sign()
{
    if (((SIGNED) POP_DATA) < 0)
    {
        number_format[--format_pos] = '-';
    }
}
    
static void end_format_number()
{
    two_drop();
    uint8_t len = NUMBER_FORMAT_LEN - format_pos;
    PUSH_DATA((UNSIGNED) &number_format[format_pos]);
    PUSH_DATA(len);
}

static void add_format_hold()
{
    UNSIGNED tos_value = POP_DATA;
    number_format[--format_pos] = tos_value;
}

static void format_number()
{
    start_format_number();
    add_format_digits();
    rot();
    add_format_sign();
    end_format_number();            
}

static void print_number()
{
    format_number();
    type();
}

static void print_space()
{
    console_put(SPACE); 
}

static void print_spaces()
{
    CELL len = POP_DATA;
    int i;
    for (i = 0; i < len; i++) {
        console_put(SPACE);
    }
}


static void print_n_top_of_stack(uint32_t radix, bool is_signed)
{
    if (current_process->sp < 0)
    {
        forth_abort();
    } 
    else
    {
        uint32_t base_restore = base_no;
        base_no = radix;
        
        if (is_signed)
        {
            duplicate();
            absolute();
        }
        else 
        {
            PUSH_DATA(1); // not negative
            swap();
        }
        PUSH_DATA(0);
        print_number();
        print_space();
        base_no = base_restore;
    }
}

static void print_top_of_stack()
{
    print_n_top_of_stack(base_no, base_no != BASE_HEX);
}

static void print_hex_top_of_stack()
{
    print_n_top_of_stack(BASE_HEX, false);
}

static void print_decimal_top_of_stack()
{
    print_n_top_of_stack(BASE_DEC, true);
}

static void print_octal_top_of_stack()
{
    print_n_top_of_stack(BASE_OCT, true);
}

static void print_binary_top_of_stack()
{
    print_n_top_of_stack(BASE_BIN, true);
}

static void print_unsigned_top_of_stack()
{
    print_n_top_of_stack(base_no, false);
//   if (current_process->sp < 0)
//    {
//        forth_abort();
//    } 
//    else 
//    {
//        PUSH_DATA(1); // not negative
//        swap();
//        PUSH_DATA(0);
//        print_number();
//        print_space();
//    }
}

static void print_double_top_of_stack() {
    if (current_process->sp < 1)
    {
        forth_abort();
    } 
    else 
    {
        swap();
        over();
        double_absolute();
        print_number();
        print_space();
    }
}

static void print_cell_of_address() {
    read_memory();
    print_unsigned_top_of_stack();
}

static void print_char_of_address() {
    read_char();
    print_unsigned_top_of_stack();
}

inline void print_cr() 
{
    console_put(NL);
}

static void base_hex()
{
    base_no = BASE_HEX;
}

static void base_decimal()
{
    base_no = BASE_DEC;
}

static void base_address()
{
    PUSH_DATA((CELL) &base_no);
}

static void emit()
{
    if (current_process->sp < 0) {
        console_out("stack underflow; aborting\n");
        return;
    }
    uint32_t ch = current_process->stack[current_process->sp--];
    console_put(ch);
}

static void read_memory()
{
    uint32_t address = current_process->stack[current_process->sp--];
    log_trace(LOG, "address %Z", address);
    if (is_accessible_memory(address)) 
    {
        uint32_t *ptr = (uint32_t *) address;
        uint32_t value = (uint32_t) *ptr;
        log_debug(LOG, "value at %Z = %Z", address, value);
        current_process->stack[++(current_process->sp)] = value;
    }
    else 
    {
        in_error = true;
    }
}

static void two_read_memory()
{
    uint32_t address = current_process->stack[current_process->sp--];
    log_trace(LOG, "address %Z", address);
    if (is_accessible_memory(address)) 
    {
        uint32_t *ptr = (uint32_t *) address;
        uint32_t tos = (uint32_t) *ptr;
        uint32_t nos = (uint32_t) *(ptr + 1);
        
        log_debug(LOG, "value at %Z = %Z", address, nos);
        current_process->stack[++(current_process->sp)] = nos;
        log_debug(LOG, "value at %Z = %Z", address, tos);
        current_process->stack[++(current_process->sp)] = tos;
    }
    else 
    {
        in_error = true;
    }
}

static void read_char()
{
    uint32_t address = current_process->stack[current_process->sp--];
    log_trace(LOG, "address %Z", address);
    uint8_t value = dictionary_read_byte((CODE_INDEX) address) & 0xff;
    log_debug(LOG, "char at %Z = %X", address, value);
    current_process->stack[++(current_process->sp)] = value;
    
    
    /*
         uint32_t address = current_process->stack[current_process->sp--];
    uint32_t align = address % 4;
    uint32_t aligned = address - align;
    log_trace(LOG, "address %Z", aligned);
    uint32_t value = dictionary_read_byte((CODE_INDEX) aligned);
    log_trace(LOG, "read %Z", value);
    uint8_t c = word >> ((3 - align) * 8) & 0xff;
    log_debug(LOG, "char at %Z = %X", address, c);
    current_process->stack[++(current_process->sp)] = c;

     */
}

static void write_memory()
{
    uint32_t address = current_process->stack[current_process->sp--]; // address
    uint32_t value = current_process->stack[current_process->sp--]; // write value
    log_debug(LOG, "set address %Z to %Z", address, value);
    if (is_accessible_memory(address)) 
    {
        uint32_t *ptr = (uint32_t *) address;
        *ptr = value;
    }
    else 
    {
        in_error = true;
    }
}

static void write_memory_add_1()
{
    uint32_t address = current_process->stack[current_process->sp--]; // address
    log_debug(LOG, "increment value at address %Z", address);
    if (is_accessible_memory(address)) 
    {
        uint32_t *ptr = (uint32_t *) address;
        *ptr = *ptr + 1;
    }
    else 
    {
        in_error = true;
    }
}

static void two_write_memory()
{
    uint32_t address = current_process->stack[current_process->sp--]; // address
    if (is_accessible_memory(address)) 
    {
        uint32_t value = current_process->stack[current_process->sp--]; // write value
        log_debug(LOG, "set address %Z to %Z", address, value);
        uint32_t *ptr = (uint32_t *) address;
        *ptr = value;
        
        ptr++;
        value = current_process->stack[current_process->sp--]; // write value
        log_debug(LOG, "set address %Z to %Z", address + 4, value);
        *ptr = value;

    }
    else 
    {
        in_error = true;
    }
}

static void write_char()
{
    uint32_t address = POP_DATA; // address
    uint32_t value = POP_DATA % 0xFF; // write value
    log_debug(LOG, "set address %Z to %X", address, value);
    dictionary_write_byte((CODE_INDEX) address, value & 0xff);
//    if (is_accessible_memory(address)) 
//    {
//        uint8_t *ptr = (uint8_t *) address;
//        *ptr = value;
//    }
//    else 
//    {
//        in_error = true;
//    }
}

static void stack()
{
    char buf[64];
    dump_parameter_stack(buf, current_process);
    console_out(buf);
}

static void clear_stack()
{
    current_process->sp = -1;
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
    log_info(LOG, "reseting");
    struct Process* next = processes;
    do {
        next->sp = -1;
        next->rsp = -1;
//        if (next != interpreter_process && next != idle_process)
//        {
            next->ip = BASE_ENTRY;
//        }
        next = next->next;
    } while (next != NULL);
    idle_process->ip = idle_code;
    interpreter_process->ip = interpreter_code;
    
    
    dictionary_master_reset();
//    tasks();
//    dictionary_debug();
//    dump_base();
}

void shorten() {
    struct Dictionary_Entry entry;
    if (dictionary_find_entry_with((CODE_INDEX) POP_DATA, &entry))
    {
        log_info(LOG, "shortening dictionary");
        dictionary_truncate_at(&entry);
    }
}

static void unused()
{
    PUSH_DATA(dictionary_unused());
}

static void allot()
{
    int32_t size = POP_DATA;
    dictionary_allot(size);
    dictionary_end_entry();
}

static void append_char()
{
    uint8_t c = POP_DATA & 0xff;
    dictionary_append_byte(c);
    dictionary_end_entry();
}

static void append_cell()
{
    dictionary_append_cell(POP_DATA);
    dictionary_end_entry();
}

/*
 * determines bytes required for specified number of cells
 */
static void cells()
{
    CELL cells = POP_DATA;
    PUSH_DATA(cells * CELL_SIZE);
}
/*
 * Add four bytes to the address to move to the next cell 
 */
static void add_cell()
{
    CELL address = POP_DATA;
    PUSH_DATA(address + CELL_SIZE);
}

static void fill_with(char c)
{
    CELL size = POP_DATA;
    CELL address = POP_DATA;
    int i;
    for (i = 0; i < size; i++) {
        dictionary_write_byte((CODE_INDEX) address++, c);
    }
}

static void aligned() 
{
    CODE_INDEX address = (CODE_INDEX) POP_DATA;
    address = dictionary_aligned(address);
    PUSH_DATA((CELL) address);
    
}

static void erase()
{
    fill_with(0);
}

static void fill()
{
    char c = (char) POP_DATA;
    fill_with(c);
}

static void here()
{
    PUSH_DATA((CELL) dictionary_here());
}

static void state_address() {
    PUSH_DATA((CELL) &state);
}

static void pad()
{
    PUSH_DATA((CELL) dictionary_pad());
}

static void count()
{
    CELL address = POP_DATA;
    uint8_t len = dictionary_read_byte((CODE_INDEX) address);
    PUSH_DATA(address + 1);
    PUSH_DATA(len);
}

static void type()
{
    uint8_t len = POP_DATA % 0xff;
    UNSIGNED address = POP_DATA;
    int i;
    for (i = 0; i < len; i++) {
        char c = dictionary_read_byte((CODE_INDEX) address++);
        console_put(c);
    }
}

static void clear_registers()
{
    current_process->sp = -1;
    current_process->rsp = -1;
    current_process->ip = BASE_ENTRY;
}

void wait(uint32_t wait_time) {
    current_process->next_time_to_run = timer + wait_time;
    next_task();
}

uint32_t pop_stack()
{
//    if (current_process->sp < 0) {
//        console_out("stack underflow; aborting\n");
//        // TODO need to use exception or some other way of dropping out
//        return;
//    }
    return current_process->stack[current_process->sp--];
}

static void dump_stack(char *buf, CELL * stack, int8_t top, char left, char right)
{
    *buf++ = left;
    if (top >= 0) {
        int i;
		for (i = 0; i <= top; i++) {
            // TODO check length written to avoid overflow
            if ( i > 0)
            {
                *buf++ = SPACE;
            }
            if (i == top)
            {
                *buf++ = '|';
                *buf++ = SPACE;
            }
            
            UNSIGNED number = stack[i];
            
            PUSH_DATA(1);
            PUSH_DATA(number);
            PUSH_DATA(0);

            format_number();
            
            uint8_t len = POP_DATA % 0xff;
            char * address = (char *) POP_DATA;
            strncpy(buf, address, len);
            buf += len;
		}
	} else {
        *buf++ = SPACE;
	}
    *buf++ = right;
    *buf = 0;
}

void dump_return_stack(char *buf, struct Process *p)
{
    uint32_t restore_to = base_no;
    base_no = BASE_HEX;
    dump_stack(buf, p->return_stack, p->rsp, '{' , '}');
    base_no = restore_to; 
}

void dump_parameter_stack(char *buf, struct Process *p)
{
    dump_stack(buf, p->stack, p->sp, '<' , '>');
}

// TODO move to  a util file
void to_upper(char *string)
{
    int len = strlen(string);
    int i;
    for (i = 0; i <= len; i++) {
        if (*string >= 'a' && *string <= 'z') {
            *string = *string - 32;
        }
        string++;
    }
}

static struct Process* new_task(uint8_t priority, char *name)
{
    struct Process* process = processes;
    // check for existing name/find last process in list
    while (process != NULL && process->next != NULL) 
    {
         if (strcicmp(name, process->name) == 0)
         {
             console_out("reusing process %S (%I)", name, process->id);
             return process;
         }
         process = process->next;
    }

    struct Process* new_process;
    new_process = malloc(sizeof(struct Process));
    if (new_process == NULL) {
        log_error(LOG, "failed to create task %S", name);
        return NULL;
    }
    
    new_process->id = next_process_id++;
    new_process->priority = priority;
    new_process->suspended = true;
    new_process->log = true;
    new_process->sp = -1;
    new_process->rsp = -1;
    new_process->next = NULL;
    new_process->next_time_to_run = 0;
    new_process->name = malloc(strlen(name) + 1);
    strcpy(new_process->name, name);
    new_process->ip = BASE_ENTRY;
    
    if (process == NULL) 
    {
        processes = new_process;
    }
    else
    {
        process->next = new_process;
    }
    
    if (current_process)
    {
        log_debug(LOG, "new task %S (P%I)", new_process->name, priority);
    }
    
    return new_process;
}

static void add_task()
{
    char name[32];
    parser_next_text(name); 
    to_upper(name);
    log_info(LOG, "add task %S", name);
    new_task(5, name);
}

static void task_priority()
{
    CELL priority = POP_DATA;
    current_process->priority = priority;
    log_info(LOG, "process priority %I now on %S", priority, current_process->name);
}
/*
 * Find the next task to execute. This is the highest priority task that has a next run
 * time that is less than the current time. If such a current_process exists then the waiting
 * flag is cleared.
 */
void next_task()
{
    uint8_t highest_priority = 0;
    
    struct Process* p = processes;
    struct Process* next = NULL;
    do {
        if (!p->suspended 
                && p->ip != BASE_ENTRY 
                && timer >= p->next_time_to_run 
                && p->priority >= highest_priority) {
            highest_priority = p->priority;
            next = p;
        }
        p = p->next;
    } while (p != NULL);
    if (next != NULL && next != current_process) {
        current_process = next;
        current_process->activations++;
        // log_trace(LOG, "switch to current_process %S", current_process->name);
    }
}
static void print_task(struct Process* p) 
{
    char buf[64];
    console_out("  Task #%I%S %S (P%I) %Z, %I next %I %S ", 
            p->id, p == current_process ? "*" : "", p->name, p->priority,  
            p->ip, p->activations, p->next_time_to_run, 
            p->suspended ? "SUSP" : "");
    dump_return_stack(buf, current_process);
    console_out(buf);
    console_put(SPACE);
    dump_parameter_stack(buf, p);
    console_out(buf);
    console_put(NL);
}

static void tasks()
{
    console_out("\nTasks:\n  Time %I\n", timer);
    struct Process* p = processes;
    do {
        print_task(p);
        p = p->next;
    } while (p != NULL);
}

void dump()
{
    CELL length = POP_DATA;
    CODE_INDEX code_index = (CODE_INDEX) POP_DATA;
    dictionary_memory_dump(code_index, length);
}

static void dump_base()
{
    dictionary_memory_dump(0, 0x100);
}

static void abort_task(struct Process* process)
{
    forth_trace(false);
    char buf[80];
    log_debug(LOG, "abort task %S", process->name);
    dump_parameter_stack(buf, process);
    console_out("\n%S aborted %S\n", process->name, buf);
    process->sp = -1;
    process->rsp = -1;
    process->ip = 0;
    process->ip = (process == interpreter_process) ? interpreter_code : (CODE_INDEX) BASE_ENTRY;
    process->next_time_to_run = 0;
    next_task();
    interpreter_echo();
    uart_dispose();
    INTEnableInterrupts();  // re-enable interrupts

}

void forth_abort() 
{
    abort_task(current_process);
}

static bool get_name_and_find_entry(struct Dictionary_Entry * entry)
{
    char token[32];
    parser_next_text(token);
    to_upper(token);
    
    if (!dictionary_find_entry_for(token, entry))
    {
        console_out("No entry %S!\n", token);
        return false;
    }

    return true;
}

/*
 * Push the address of the word in the dictionary (the execution token) that 
 * matched the next token on the input 
 */
static void tick()
{
    struct Dictionary_Entry entry;
    if (!get_name_and_find_entry(&entry)) 
    {
        forth_abort();
    }
    else
    {
        PUSH_DATA((CELL) entry.instruction);
    }
}

static void debug_word()
{ 
    struct Dictionary_Entry entry;
    if (!get_name_and_find_entry(&entry)) 
    {
        forth_abort();
    }
    else
    {
        console_put(NL);
        dictionary_debug_entry(&entry);
    }
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
    uint8_t level = POP_DATA;
    CELL vos = current_process->stack[current_process->sp - level];
    PUSH_DATA(vos);
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
static void two_over()
{
    PUSH_DATA(current_process->stack[current_process->sp - 3]);  
    PUSH_DATA(current_process->stack[current_process->sp - 3]);  
}

/*
 * In a two cell pair stack copy the bottom pair to the TOS.
 */
static void two_rot()
{
    
    CELL value1 = current_process->stack[current_process->sp - 5];
    CELL value2 = current_process->stack[current_process->sp - 4];
    
    current_process->stack[current_process->sp - 5] = current_process->stack[current_process->sp - 3];
    current_process->stack[current_process->sp - 4] = current_process->stack[current_process->sp - 2];
    current_process->stack[current_process->sp - 3] = current_process->stack[current_process->sp - 1];
    current_process->stack[current_process->sp - 2] = current_process->stack[current_process->sp];
    
    current_process->stack[current_process->sp - 1] = value1;
    current_process->stack[current_process->sp] = value2;
}

/*
 * In a two cell pair stack copy the bottom pair to the TOS.
 */
static void two_swap()
{
    CELL tos = current_process->stack[current_process->sp - 0];
    CELL nos = current_process->stack[current_process->sp - 1];
    
    current_process->stack[current_process->sp - 0] = current_process->stack[current_process->sp - 2];
    current_process->stack[current_process->sp - 1] = current_process->stack[current_process->sp - 3];
    
    current_process->stack[current_process->sp - 2] = tos;
    current_process->stack[current_process->sp - 3] = nos;
}

static void new_s_string()
{
    if (state) 
    {
        compiler_s_string();
    }
    else 
    {
        here();
        compiler_compile_string();
        count();
    }   
}

/*
 Outputs the string to the console.
 */
void print_string()
{
    c_string();
    count();
    type();
    /*
    
    uint8_t len = dictionary_read_next_byte(current_process);
    int i;
    for (i = 0; i < len; i++) {
        char ch = dictionary_read_next_byte(current_process);
        console_out("%c", ch);
    }
     */
}

/*
 * Pushes the count and address of the counted string onto the stack and 
 * moves the IP forward to next instruction.
 */
void s_string()
{
    c_string();
    count();
    
    /*
    uint8_t len = dictionary_read_next_byte(current_process);
    PUSH_DATA((CELL) current_process->ip);
    PUSH_DATA(len);
    current_process->ip += len;
     */
}
/*
 * Pushes the address of the counted string onto the stack and moves the 
 * IP forward to next instruction.
 */
void c_string()
{
    PUSH_DATA((CELL) current_process->ip);
    uint8_t len = dictionary_read_next_byte(current_process);
    current_process->ip += len;
}

static void next_char()
{
    PUSH_DATA(uart_next_char());
}

static void has_next_char()
{
    PUSH_DATA(uart_has_next_char());
}
    
static void load_words()
{
    log_info(LOG, "load words");
    
    dictionary_add_core_word(NULL, nop, false);
    dictionary_add_core_word(NULL, push_literal, false);
    dictionary_add_core_word(NULL, memory_address, false);
    dictionary_add_core_word(NULL, branch, false);
    dictionary_add_core_word(NULL, zero_branch, false);
    // TODO replace with EXIT word
    int_return_code = dictionary_add_core_word(NULL, return_to, false);
    dictionary_add_core_word(NULL, interpreter_run, false);
    dictionary_add_core_word(NULL, print_string, false);
    dictionary_add_core_word(NULL, s_string, false);
    dictionary_add_core_word(NULL, c_string, false);
    dictionary_add_core_word(NULL, data_address, false);
    
    // words with short codes
    dictionary_add_core_word("?DUP", question_dup, false);
    dictionary_add_core_word("DEPTH", depth, false);
    dictionary_add_core_word("DROP", drop, false);
    dictionary_add_core_word("DUP", duplicate, false);
    dictionary_add_core_word("NIP", nip, false);
    dictionary_add_core_word("OVER", over, false);
    dictionary_add_core_word("PICK", pick, false);
    dictionary_add_core_word("ROT", rot, false);
    dictionary_add_core_word("LROT", lrot, false);
    dictionary_add_core_word("SWAP", swap, false);
    dictionary_add_core_word("TUCK", tuck, false);

    dictionary_add_core_word("2DROP", two_drop, false);
    dictionary_add_core_word("2DUP", two_dup, false);
    dictionary_add_core_word("2OVER", two_over, false);
    dictionary_add_core_word("2ROT", two_rot, false);
    dictionary_add_core_word("2SWAP", two_swap, false);

    
    dictionary_add_core_word("DUMP", dump, false);
    dictionary_add_core_word("WORDS", dictionary_words, false);

    dictionary_add_core_word("+", add, false);
    dictionary_add_core_word("D+", double_add, false);
    dictionary_add_core_word("M+", mixed_add, false);
    dictionary_add_core_word("-", subtract, false);
    dictionary_add_core_word("D-", double_subtract, false);
    dictionary_add_core_word("NEGATE", negate, false);
    dictionary_add_core_word("DNEGATE", double_negate, false);
    dictionary_add_core_word("*", multiply, false);
    dictionary_add_core_word("M*", mixed_multiply, false);
    dictionary_add_core_word("MU*", unsigned_multiply, false);
    dictionary_add_core_word("/", divide, false);
    dictionary_add_core_word("M/", mixed_divide, false);
    dictionary_add_core_word("MOD", mod, false);
    dictionary_add_core_word("/MOD", divide_mod, false);
    dictionary_add_core_word("UM/MOD", unsigned_divide_mod, false);
    dictionary_add_core_word("*/", multiply_divide, false);
    dictionary_add_core_word("M*/", mixed_multiply_divide, false);
    dictionary_add_core_word("1+", add_1, false);
    dictionary_add_core_word("2+", add_2, false);
    dictionary_add_core_word("1-", subtract_1, false);
    dictionary_add_core_word("2-", subtract_2, false);
    dictionary_add_core_word("2*", left_shift_1, false);
    dictionary_add_core_word("2/", right_shift_1, false);
    dictionary_add_core_word(">", greater_than, false);
    dictionary_add_core_word(">=", greater_than_equal, false);
    dictionary_add_core_word("<", less_than, false);
    dictionary_add_core_word("<=", less_than_equal, false);
    dictionary_add_core_word("=", equal_to, false);
    dictionary_add_core_word("<>", not_equal_to, false);
    dictionary_add_core_word("0=", equal_to_zero, false);
    dictionary_add_core_word("0>", greater_than_zero, false);
    dictionary_add_core_word("0<", less_than_zero, false);
    dictionary_add_core_word("0<>", not_equal_to_zero, false);
    dictionary_add_core_word("ABS", absolute, false);
    dictionary_add_core_word("DABS", double_absolute, false);
    dictionary_add_core_word("MAX", single_max, false);
    dictionary_add_core_word("DMAX", double_max, false);
    dictionary_add_core_word("MIN", single_min, false);
    dictionary_add_core_word("DMIN", double_min, false);

    dictionary_add_core_word("AND", and, false);
    dictionary_add_core_word("OR", or, false);
    dictionary_add_core_word("XOR", xor, false);
    dictionary_add_core_word("NOT", not, false);
    dictionary_add_core_word("LSHIFT", left_shift, false);
    dictionary_add_core_word("RSHIFT", right_shift, false);
    dictionary_add_core_word(".", print_top_of_stack, false);
    dictionary_add_core_word("HEX.", print_hex_top_of_stack, false);
    dictionary_add_core_word("DEC.", print_decimal_top_of_stack, false);
    dictionary_add_core_word("OCT.", print_octal_top_of_stack, false);
    dictionary_add_core_word("BIN.", print_binary_top_of_stack, false);
    dictionary_add_core_word("U.", print_unsigned_top_of_stack, false);
    dictionary_add_core_word("D.", print_double_top_of_stack, false);
    dictionary_add_core_word("?", print_cell_of_address, false);
    dictionary_add_core_word("C?", print_char_of_address, false);
    dictionary_add_core_word("SPACE", print_space, false);
    dictionary_add_core_word("SPACES", print_spaces, false);
    
    dictionary_add_core_word("<#", start_format_number, false);
    dictionary_add_core_word("#", add_format_digit, false);
    dictionary_add_core_word("#S", add_format_digits, false);
    dictionary_add_core_word("SIGN", add_format_sign, false);
    dictionary_add_core_word("HOLD", add_format_hold, false);
    dictionary_add_core_word("#>", end_format_number, false);
    
    dictionary_add_core_word("BASE", base_address, false);
    dictionary_add_core_word("HEX", base_hex, false);
    dictionary_add_core_word("DECIMAL", base_decimal, false);
    dictionary_add_core_word("CR", print_cr, false);
    dictionary_add_core_word("EMIT", emit, false);
    dictionary_add_core_word("@", read_memory, false);
    dictionary_add_core_word("2@", two_read_memory, false);
    dictionary_add_core_word("C@", read_char, false);
    dictionary_add_core_word("!", write_memory, false);
    dictionary_add_core_word("!+", write_memory_add_1, false);
    dictionary_add_core_word("2!", two_write_memory, false);
    dictionary_add_core_word("C!", write_char, false);
    dictionary_add_core_word("EXECUTE", execute_word, false);
    dictionary_add_core_word(".S", stack, false);
    dictionary_add_core_word("CLEAR", clear_stack, false);
    dictionary_add_core_word("TICKS", ticks, false);
    dictionary_add_core_word("TIME", time, false);
    dictionary_add_core_word("TASK", add_task, false);
    dictionary_add_core_word("PRIORITY", task_priority, false);

    dictionary_add_core_word("'", tick, false);

    dictionary_add_core_word("INITIATE", initiate, false);
    dictionary_add_core_word("TERMINATE", terminate, false);
    dictionary_add_core_word("SUSPEND", suspend, false);
    dictionary_add_core_word("RESUME", resume, false);
    dictionary_add_core_word("PAUSE", yield, false);
    dictionary_add_core_word("MS", wait_for, false);
    
    dictionary_add_core_word("CHAR", push_char, false);
    dictionary_add_core_word("BL", push_blank, false);

    dictionary_add_core_word("VARIABLE", compiler_variable, false);
    dictionary_add_core_word("2VARIABLE", compiler_2variable, false);
    dictionary_add_core_word("CONSTANT", compiler_constant, false);
    dictionary_add_core_word("2CONSTANT", compiler_2constant, false);

    dictionary_add_core_word("SEE", debug_word, false);
    dictionary_add_core_word("TASKS", tasks, false);

    
    // Immediate words
    dictionary_add_core_word("\\", compiler_eol_comment, true);
    dictionary_add_core_word("(", compiler_inline_comment, true);
    dictionary_add_core_word(".(", compiler_print_comment, true);
    dictionary_add_core_word("IF", compiler_if, true);
    dictionary_add_core_word("THEN", compiler_then, true);
    dictionary_add_core_word("ELSE", compiler_else, true);
    dictionary_add_core_word("BEGIN", compiler_begin, true);
    dictionary_add_core_word("AGAIN", compiler_again, true);
    dictionary_add_core_word("UNTIL", compiler_until, true);
    dictionary_add_core_word(":", compiler_compile_definition, false);
    dictionary_add_core_word(";", compiler_end, true);
    dictionary_add_core_word(",\"", compiler_compile_string, true);
    dictionary_add_core_word(".\"", compiler_print_string, true);
    dictionary_add_core_word("S\"", new_s_string, true);
    dictionary_add_core_word("C\"", compiler_c_string, true);
    dictionary_add_core_word("[CHAR]", compiler_char, true);
    dictionary_add_core_word("IMMEDIATE", dictionary_mark_internal, true); 
    
    dictionary_add_core_word("LOG", set_log_level, false);
    dictionary_add_core_word("ALLOT", allot, false);
    dictionary_add_core_word("CREATE", compiler_create_data, false);
    dictionary_add_core_word("ALIGN", dictionary_align, false);
    dictionary_add_core_word("ALIGNED", aligned, false);
    dictionary_add_core_word("UNUSED", unused, false);
    dictionary_add_core_word(",", append_cell, false);
    dictionary_add_core_word("C,", append_char, false);
    dictionary_add_core_word("CELLS", cells, false);
    dictionary_add_core_word("CELL+", add_cell, false);
    dictionary_add_core_word("ERASE", erase, false);
    dictionary_add_core_word("FILL", fill, false);
    dictionary_add_core_word("HERE", here, false);
    dictionary_add_core_word("STATE", state_address, false);
    dictionary_add_core_word("[", compiler_suspend, true);
    dictionary_add_core_word("]", compiler_resume, true);
    dictionary_add_core_word("PAD", pad, false);
    dictionary_add_core_word("COUNT", count, false);
    dictionary_add_core_word("TYPE", type, false);
    
    dictionary_add_core_word("KEY", next_char, false);
    dictionary_add_core_word("KEY?", has_next_char, false);
    dictionary_add_core_word("ABORT", forth_abort, false);

    // interrupts
    dictionary_add_core_word("IREG", interrupt_register, false);
    dictionary_add_core_word("IPRI", interrupt_priority, false);
    dictionary_add_core_word("IEN", interrupt_enable, false);
    dictionary_add_core_word("IDIS", interrupt_disable, false);
    dictionary_add_core_word("ICLR", interrupt_clear, false);
    dictionary_add_core_word("ISTS", interrupt_status, false);
//    dictionary_add_core_word("IRET", interrupt_return, false);

    // other, non-forth standard, words
    dictionary_add_core_word("DICT", dictionary_debug, false);
    dictionary_add_core_word("DRESET", dictionary_master_reset, false);
    dictionary_add_core_word("LOCK", dictionary_lock, false);
    dictionary_add_core_word("UNLOCK", dictionary_unlock, false);
    dictionary_add_core_word("_DUMP", dump_base, false);
    dictionary_add_core_word("_DEBUG", debug_on, false);
    dictionary_add_core_word("_NODEBUG", debug_off, false);
    dictionary_add_core_word("_RESET", reset, false);
    dictionary_add_core_word("_SHORT", shorten, false);
    dictionary_add_core_word("_CLEAR", clear_registers, false);

    // create loop with process instruction
    // =>  : _INTERACTIVE BEGIN {run code} AGAIN ;
    interpreter_code = dictionary_add_entry("_INTERACTIVE");
    compiler_begin();
    dictionary_append_function(interpreter_run);
    compiler_again();
    dictionary_end_entry();
    
    // create loop with pause instruction  
    // =>  : _IDLE BEGIN PAUSE AGAIN ;
    idle_code = dictionary_add_entry("_IDLE");
    compiler_begin();
    struct Dictionary_Entry entry;
    dictionary_append_function(yield);
    compiler_again();
    dictionary_end_entry();

  //  dictionary_find_entry("_INTERACTIVE", &entry);
  //  interpreter_code = entry.instruction;
    interpreter_process->ip = interpreter_code;

 //   log_error(LOG, "%Z == %Z", cdix, interpreter_code);
    
//    dictionary_find_entry("_IDLE", &entry);
//    idle_code = entry.instruction;
    idle_process->ip = idle_code;

//    dictionary_find_entry("IRET", &entry);
//    int_return_code = entry.instruction;
    
//    dictionary_lock();
}