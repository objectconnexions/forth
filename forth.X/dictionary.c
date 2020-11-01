#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <GenericTypeDefs.h>
#include <machine/types.h>

#include "debug.h"
#include "logger.h"
#include "dictionary.h"
#include "forth.h"
#include "interpreter.h"


#define LOG "Dictionary"

#ifdef MX130
    #define CODE_SIZE (1024 * 8)
#else
    #define CODE_SIZE (1024 * 40)
#endif
#define CORE_WORDS 180

//static CODE_INDEX memory;
static uint8_t memory[CODE_SIZE];
static CORE_FUNC core_functions[CORE_WORDS];

static CODE_INDEX search_from; // last non-scratch entry in dictionary
static CODE_INDEX new_entry_offset;  // next free space in dictionary for a new entry
static CODE_INDEX last_core_entry;
static CODE_INDEX locked_before;
static uint8_t last_function_index;
static CODE_INDEX insertion_point;

static uint16_t find_word(char *);
static uint16_t find_dictionay_entry(char *);
static uint64_t read(CODE_INDEX *);
static CODE_INDEX read_address(CODE_INDEX *);
static CODE_INDEX find_entry(CODE_INDEX, char *);
static CODE_INDEX align(CODE_INDEX);


void dictionary_init()
{
 //   memory = malloc(CODE_SIZE);
//    log_info(LOG, "%I bytes allocated at %Z", CODE_SIZE, memory);
//    uint16_t size = sizeof(CORE_FUNC) * CORE_WORDS;
//    log_info(LOG, "allocating %I bytes for function lookup", size);
//    core_functions = (CORE_FUNC *)malloc(size);
    log_info(LOG, "memory at %Z", memory);
    log_info(LOG, "functions at %Z", core_functions);
    log_info(LOG, "success");
    search_from = LAST_ENTRY;
    locked_before = memory;
    last_core_entry = memory;
    last_function_index = 0;
    dictionary_reset();
}
    
void dictionary_reset()
{
    CODE_INDEX entry;

    entry = search_from;
    while (entry != LAST_ENTRY) {
        entry = read_address(&entry);
        
        if (entry < locked_before) {
            search_from = entry;
            break;
        }
    }

    insertion_point = locked_before;
    new_entry_offset = locked_before;
    log_debug(LOG, "last entry reset to %Z, next entry reset to %Z", search_from, new_entry_offset);
    
    uint32_t offset = ((uint32_t) locked_before) - ((uint32_t) memory);
    memset(locked_before, 0, CODE_SIZE - offset);   
}

uint32_t dictionary_unused() 
{
    uint32_t used = ((uint32_t) insertion_point) - ((uint32_t) memory);
    return CODE_SIZE - used;
}

CODE_INDEX dictionary_here()
{
    return insertion_point;
}

CODE_INDEX dictionary_pad()
{
    return new_entry_offset;
}

static void add_entry(char *name, uint8_t flags)
{
    //log_trace(LOG, "   previous word ends at %04X", dictionary_offset() - 1);

    int len = strlen(name);
    insertion_point = new_entry_offset;              
    log_debug(LOG, "   new entry for '%S' (%I chars) at %Z", name, len & 0x1f, insertion_point);

    // previous entry
    CODE_INDEX previous_entry;
    if (new_entry_offset == 0) {
        previous_entry = LAST_ENTRY;  // first entry; memory to indicate no more previous
    } else {
        previous_entry = search_from;
    }

    search_from = new_entry_offset;
    
    log_debug(LOG, "   link to previous_entry at %Z", previous_entry);
    dictionary_append_instruction(previous_entry);
    dictionary_append_value((len & 0x1f) | (flags << 5));
    
    strcpy(insertion_point, name);
    insertion_point += len;
    log_debug(LOG, "   code start %Z", insertion_point);
}

void dictionary_lock()
{
    locked_before = new_entry_offset;
}

void dictionary_restart(CODE_INDEX entry)
{
    locked_before = entry;
}

void dictionary_add_entry(char *name)
{
    add_entry(name, 0);
}

static CODE_INDEX align(CODE_INDEX address)
{
    uint8_t offset = ((uint32_t) address) % 4;
    log_debug(LOG, "alignment offset %I", offset);
    address = offset == 0 ? address : address + 4 - offset;
    log_debug(LOG, "aligned to %Z", address);
    return address;
}

/*
 * Align data in memory so it is on a four-byte boundary as accessing memory at other 
 * positions fails with an exception.
 */
void dictionary_align(void)
{
    log_debug(LOG, "alignment offset %I", ((uint32_t) insertion_point) % 4);
    while (((uint32_t) insertion_point) % 4 != 0) {
        dictionary_append_byte(0);
    }
    log_debug(LOG, "aligned to %Z", insertion_point);
}

void dictionary_allot(int32_t size)
{
    size = max(0, size);
    log_debug(LOG, "allot %I bytes", size);
    insertion_point += size;
}

void dictionary_append_byte(uint8_t value)
{
    *insertion_point++ = value;
}


// TODO rename to append_literal
void dictionary_append_value(uint64_t value)
{
    uint8_t segment;
    while (true) {
        segment = value % 0x80;
        if (value > segment) {
            segment = segment | 0x80;
            *insertion_point++ = segment;
            value = value / 0x80;
        } else {
            *insertion_point++ = segment;
            break;
        }
    }
}

void dictionary_append_function(CORE_FUNC function)
{
    int i;
    for (i = 0; i < last_function_index; i++) {
        if (core_functions[i] == function)
        {
            dictionary_append_instruction((CODE_INDEX) i);
            return;
        }
    }
    log_error(LOG, "failed to find function %Y", function);
    dictionary_append_instruction(0);
}

void dictionary_append_instruction(CODE_INDEX word)
{
    dictionary_append_value((uint32_t) word);
}

uint8_t dictionary_read_byte(CODE_INDEX index) 
{
    return *index;
}

void dictionary_write_byte(CODE_INDEX index, uint8_t value)
{
    *index = value;
}

void dictionary_end_entry() 
{
    log_trace(LOG, "    entry ends at %Z", insertion_point - 1);
//        search_from = new_entry_offset;
    new_entry_offset = insertion_point;
}

void dictionary_insert_internal_instruction(uint8_t entry, CORE_FUNC function)
{
    core_functions[entry] = function;
    last_function_index = entry >= last_function_index ? entry + 1: last_function_index;
}

void dictionary_add_core_word(char * name, CORE_FUNC function, bool immediate)
{
    if (last_function_index + 1 >= CORE_WORDS)
    {
        log_error(LOG, "too many core words");
    }
    else
    {
        log_debug(LOG, "core word %S (func %X)", name, last_function_index);
        core_functions[last_function_index] = function;

        if (name)
        {
            add_entry(name, 0);
            dictionary_append_value(last_function_index);
            dictionary_end_entry();
        }
        if (immediate)
        {
            dictionary_mark_internal();
        }

        last_function_index++;
        last_core_entry = new_entry_offset;
    }
}

/*
 Mark the most recent entry as IMMEDIATE
 */
void dictionary_mark_internal() {
   CODE_INDEX current_index = search_from;
   read_address(&current_index);    // move the pointer to right address
   *current_index = *current_index | IMMEDIATE << 5;
}

/*
    Returns the address of the memory for for the specified word.
 */
 // TODO RENAME to compiler_find_word())
bool dictionary_find_entry(char * name, struct Dictionary_Entry *entry)
{
    struct Dictionary_Entry e;
    CODE_INDEX current_index;
    CODE_INDEX next_entry;

    e.starts = search_from;
    e.ends = new_entry_offset;
    log_debug(LOG, " looking for '%S' starting at %Z", name, e.starts);
    while (e.starts != LAST_ENTRY) {
        current_index = e.starts;
        next_entry = read_address(&current_index);
        e.flags = *current_index >> 5;
        uint8_t len = *current_index++ & 0x1f;
        strncpy(e.name, current_index, len);
        e.name[len] = 0;

        // log_trace(LOG, "   checking '%S' at %08x (%08x)", entry_name, entry_index, current_index + len);
        if (strcmp(e.name, name) == 0) {
            current_index += len;
            log_debug(LOG, " code found for '%S' at %Z (%Z~%Z)", e.name, current_index, e.starts, e.ends);
            // the next byte is the start of the memory
            if (current_index < last_core_entry)
            {
                // use short memory
                entry->instruction = (CODE_INDEX) (uint32_t) *current_index;
            }
            else 
            {
                entry->instruction = current_index;
            }
            entry->starts = e.starts; 
            entry->ends = e.ends; 
            entry->flags = e.flags; 
            strcpy(entry->name, e.name);
            return true;
        }
        e.ends = e.starts;
        e.starts = next_entry;
    }
    entry->starts = new_entry_offset;
    entry->ends = LAST_ENTRY;
    strcpy(entry->name, name);
    log_debug(LOG, "   token not found (%S)", name);
    return false;
}

// TODO rename to refer to character?
uint8_t dictionary_read_next_byte(struct Process *process) 
{
    return *(process->ip)++;
}

static uint64_t read(CODE_INDEX *offset) 
{
    uint8_t segment;
    uint32_t value = 0;
    uint8_t i = 0;
    
    while (true)
    {
        // log_trace(LOG, "read %08x", *offset);
        
        segment = *(*offset);
        (*offset)++;
        value =  (segment & 0x7F) << (i * 7)  | value ; // (value * 0x80) | (segment & 0x7F);
        if (segment < 0x80) {
            return value;
        }
        i++;
    }
}

static CODE_INDEX read_address(CODE_INDEX *offset)
{
    uint32_t value = read(offset);
    return (CODE_INDEX) value;
}

uint64_t dictionary_read(struct Process *process) 
{
    return read(&(process->ip));
}

CODE_INDEX dictionary_read_instruction(struct Process *process) 
{
    return read_address(&(process->ip));
}

bool dictionary_shortcode(CODE_INDEX instruction)
{
    return ((uint32_t) instruction) < CORE_WORDS;
}

void dictionary_execute_function(CODE_INDEX instruction)
{
    core_functions[(uint32_t) instruction]();
}

CODE_INDEX dictionary_offset() 
{
    return insertion_point;
}

// TODO rename
CODE_INDEX dictionary_data_address(CODE_INDEX offset)
{
    return align(offset);
}

/*
 Return the address of the dictionary entry for the memory at the specified address
 */
CODE_INDEX dictionary_find_word_for(CODE_INDEX instruction, char *name) {
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    bool short_memory;

    log_debug(LOG, "seeking name for instruction %Z (%X)", instruction, last_function_index);

    entry = search_from;
    strcpy(name, "Unknown!");        
    while (entry != LAST_ENTRY) {
        next_entry = read_address(&entry);
        uint8_t len = *entry++ & 0x1f;
        CODE_INDEX memory_at = entry + len;
        short_memory = entry <= last_core_entry;
//        log_trace(LOG, " checking %Sentry %08x (%02x)", short_memory ? "short " : "", memory_at, *memory_at);
        if ((short_memory && ((uint32_t) instruction) == *memory_at) || (!short_memory && instruction == memory_at)) {
            strncpy(name, entry,len);
            name[len] = 0;
            log_debug(LOG, " found %S %Z", name, memory_at);
            return entry;
        }

        entry = next_entry;
    }
    return LAST_ENTRY;
}

// TODO is this 
bool dictionary_is_core_word(CODE_INDEX entry)
{
    return entry <= (CODE_INDEX) ((uint32_t) last_function_index);
}

/*
 * List the entries in the dictionary
 */
void dictionary_words() {
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    char name[32];
    uint8_t width = 0;
    
	if (search_from == LAST_ENTRY)
    {
		console_out("No entries\n");
	}
    else
    {
        console_put(NL);
        entry = search_from;
        while (entry != LAST_ENTRY)
        {
            next_entry = read_address(&entry);
            uint8_t len = *entry++ & 0x1f;
            strncpy(name, entry, len);
            name[len] = 0;
            width += len + 1;
            if (width > 80) 
            {
                console_put(NL);
                width = len;
            }
            console_out(name);
            console_put(SPACE);
            entry = next_entry;
        }
        console_put(NL);
        console_put(NL);
    }
}

void dictionary_memory_dump(CODE_INDEX start, uint16_t size) {
    uint32_t addr, col;
    uint32_t offset = (uint32_t) (start == 0 ? memory : start);
    uint32_t from = offset;
    from = from - (from % 16);
    uint32_t end = offset + size;
    uint32_t to = end / 16 * 16 + (end % 16 == 0 ? 0 : 16);
    
	console_out("\n%Z  ", offset);
	for (col = 0; col < 16; col++) {
		console_out("%X ", col);
	}
	console_put(NL);
    
	for (addr = from; addr < to; addr++) {
        bool new_line = addr % 16 == 0;
		if (new_line) {
			console_out("\n%Z %S", addr, addr == offset ? "[" : " ");
		}
		console_out("%X%S", *((CODE_INDEX) addr), (!new_line && addr == offset - 1) ? "[" : (addr == end - 1 ? "]" : " "));
        
        if (addr % 16 == 15) {
            console_put(SPACE);
            console_put(SPACE);
            for (col = 0; col < 16; col++)
            {
                if (col == 8) console_put(SPACE);
                //uint8_t cc = *((CODE_INDEX) (addr - 15 + col));
                char c = *(((CODE_INDEX) addr) - 15 + col);
                if (c < 32 || c > 127) c = '.';
                console_put(c);
            }
        }
	}
    console_put(NL);
    console_put(NL);
}

/*
 * Return the offset of the dictionary that contains the specified offset (typically called using the offset
 * of the memory.
 */
// TODO add check for setting of SCRUB flag to prevent recursive calls
static CODE_INDEX find_entry(CODE_INDEX offset, char * name)
{
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    CODE_INDEX start_at;
       
    entry = search_from;
    while (entry != LAST_ENTRY)
    {
        start_at = entry;
        next_entry = read_address(&entry);
        if (start_at <= offset) {
            uint8_t len = *entry++ & 0x1f;
            strncpy(name, entry, len);
            name[len] = 0;
            return start_at;
        }
        entry = next_entry;
    }
    
    return (CODE_INDEX) LAST_ENTRY;
}
       
// TODO removed as not in use
void dictionary_debug_summary(CODE_INDEX instruction)
{
     if (instruction > (CODE_INDEX) 0x90000000) 
     {
         // TODO lookup word that matches function address
         console_out("func<%Z>", instruction);
     }
     else
     {
        char name[32];
        CODE_INDEX location = find_entry(instruction, name);
        console_out("%S @%Y", name, location);
     }
}
      
static void debug_print(CODE_INDEX addr, uint8_t length)
{
    console_out("    %Y  ", addr);
    int i;
    for (i = 0; i < length; i++) {
        console_out("%X", *addr++);
    }
    console_out("    ");
}

void dictionary_debug_entry(struct Dictionary_Entry * entry_data)
{
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    CODE_INDEX start_at, end_at;
    CODE_INDEX addr;
    CODE_INDEX value;
    int8_t relative;
    
    start_at = entry_data->starts;
    log_debug(LOG, " debug entry for %Z", start_at);
    end_at = entry_data->ends;
    
    console_out("Word '%S' at %Z~%Z", entry_data->name, start_at, end_at);
    entry = start_at;
    value = read_address(&entry);
    console_out(" (next %Z)\n", value);
    entry += strlen(entry_data->name) + 1;

    while (entry < end_at) 
    {
        addr = entry;
        value = read_address(&entry);
        if (value <= (CODE_INDEX) ((uint32_t) last_function_index)) {
            CORE_FUNC function = core_functions[(uint32_t) value];
            if (function == push_literal)
            {
                CELL value2 = read(&entry);
                debug_print(addr, entry - addr);
                console_out("LIT %Z", value2);
            }
            else if (function == memory_address)
            {
                value = align(entry);
                debug_print(addr, entry - addr);
                console_out("ADDR %Z", value);
            }
            else if (function == data_address)
            {
                value = align(entry);
                debug_print(addr, entry - addr);
                console_out("DATA %Z", value);
            }
            else if (function == return_to)
            {
                debug_print(addr, entry - addr);
                console_out("EXIT");
            }
            else if (function == interpreter_run)
            {
                debug_print(addr, entry - addr);
                console_out("INTERPRET");
            }
            else if (function == branch)
            {
                relative = *entry++;
                debug_print(addr, entry - addr);
                console_out("BRANCH (%I) %Z", relative, addr + relative);
            }
            else if (function == zero_branch)
            {
                relative = *entry++;
                debug_print(addr, entry - addr);
                console_out("ZBRANCH (%I) %Z", relative, addr + relative);
            }
            else if (function == print_string || function == c_string || function == s_string)
            {
                relative = *entry++;
                debug_print(addr, entry - addr);
                console_out("STRING (%I) '", relative);
                int i;
                for (i = 0; i < relative; i++) {
                    char c = *entry++;
                    if (c < 32)
                    {
                        console_out("{%I}", c);
                    }
                    else 
                    {
                        console_put(c);
                    }
                }
                console_put('\'');

            }
            else if (function == nop)
            {
                debug_print(addr, entry - addr);
                console_out("NOP");
            }
            else
            {
                dictionary_find_word_for(value, entry_data->name);
                CORE_FUNC function = core_functions[(uint32_t) value];
                debug_print(addr, entry - addr);
                console_out("%S  func(%Y)<%Z>", entry_data->name, value, function);
            }
        }
        else 
        {
            dictionary_find_word_for(value, entry_data->name);
            debug_print(addr, entry - addr);
            console_out("%S (%Z)", entry_data->name, value);
        }
        console_put(NL);
    }
}

void dictionary_debug()
{
    console_out("last: %Z\n", search_from);
    console_out("new: %Z\n", new_entry_offset);
    console_out("locked at: %Z\n", locked_before);
    console_out("memory at: %Z\n\n", memory);
    
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    char name[32];

	if (search_from == LAST_ENTRY) {
		console_out("No entries\n");
	}
    
    entry = search_from;
    while (entry != LAST_ENTRY)
    {
        console_out("%Z ", entry);
        next_entry = read_address(&entry);
        uint8_t byte = *entry++;
        uint8_t len = byte & 0x1f;
        strncpy(name, entry, len);
        name[len] = 0;
        console_out("%S  [%X] ", name, byte >> 5);
        console_out("%S  [%I] ", name, byte >> 5);
        if (log_level <= DEBUG) console_out("[%Z + %I]  ", entry, len);
        if (entry <= last_core_entry)
        {
            // C function address
            uint8_t short_memory = *(entry + len);
            console_out(" func(%I)<%Z>\n", short_memory, core_functions[short_memory]);            
        }
        else
        {
            // position of the memory
            console_out(" -> %Z\n", entry + len);
        }
        entry = next_entry;
    }
}
