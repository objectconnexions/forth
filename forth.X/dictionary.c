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


#define LOG "Dictionary"

#ifdef MX130
    #define CODE_SIZE (1024 * 8)
#else
    #define CODE_SIZE (1024 * 40)
#endif
#define CORE_WORDS 128

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
static CELL read(CODE_INDEX *);
static CODE_INDEX read_address(CODE_INDEX *);
static CODE_INDEX find_entry(CODE_INDEX, char *);
static CODE_INDEX align(CODE_INDEX);


void dictionary_init()
{
 //   memory = malloc(CODE_SIZE);
//    log_info(LOG, "%i bytes allocated at %08x", CODE_SIZE, memory);
//    uint16_t size = sizeof(CORE_FUNC) * CORE_WORDS;
//    log_info(LOG, "allocating %i bytes for function lookup", size);
//    core_functions = (CORE_FUNC *)malloc(size);
    log_info(LOG, "memory at %08x", memory);
    log_info(LOG, "functions at %08x", core_functions);
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
    log_debug(LOG, "last entry reset to %08x, next entry reset to %08x", search_from, new_entry_offset);
    
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
    log_debug(LOG, "   new entry for '%s' (%i chars) at %08x", name, len & 0x1f, insertion_point);

    // previous entry
    CODE_INDEX previous_entry;
    if (new_entry_offset == 0) {
        previous_entry = LAST_ENTRY;  // first entry; memory to indicate no more previous
    } else {
        previous_entry = search_from;
    }

    search_from = new_entry_offset;
    
    log_debug(LOG, "   link to previous_entry at %08x", previous_entry);
    dictionary_append_instruction(previous_entry);
    dictionary_append_value((len & 0x1f) | (flags << 5));
    
    strcpy(insertion_point, name);
    insertion_point += len;
    log_debug(LOG, "   code start %08x", insertion_point);
}

void dictionary_lock()
{
    locked_before = new_entry_offset;
}

void dictionary_add_entry(char *name)
{
    add_entry(name, 0);
}

static CODE_INDEX align(CODE_INDEX address)
{
    uint8_t offset = ((uint32_t) address) % 4;
    log_debug(LOG, "alignment offset %i", offset);
    address = offset == 0 ? address : address + 4 - offset;
    log_debug(LOG, "aligned to %08x", address);
    return address;
}

/*
 * Align data in memory so it is on a four-byte boundary as accessing memory at other 
 * positions fails with an exception.
 */
void dictionary_align(void)
{
    log_debug(LOG, "alignment offset %i", ((uint32_t) insertion_point) % 4);
    while (((uint32_t) insertion_point) % 4 != 0) {
        dictionary_append_byte(0);
    }
    log_debug(LOG, "aligned to %08x", insertion_point);
}

void dictionary_allot(int32_t size)
{
    size = max(0, size);
    log_debug(LOG, "allot %i bytes", size);
    insertion_point += size;
}

void dictionary_append_byte(uint8_t value)
{
    *insertion_point++ = value;
}


// TODO rename to append_literal
void dictionary_append_value(uint32_t value)
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
    log_trace(LOG, "    entry ends at %08x", insertion_point - 1);
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
    uint8_t flags = 0;
    
    log_debug(LOG, "core word %s (func %02X)", name, last_function_index);
    if (immediate)
    {
        flags = IMMEDIATE;
//        add_entry(name, flags);
//        dictionary_end_entry();
    }
  
    core_functions[last_function_index] = function;

    if (name)
    {
        add_entry(name, flags);
        dictionary_append_value(last_function_index);
        dictionary_end_entry();
    }
    
    last_function_index++;
    last_core_entry = new_entry_offset;
}

/*
    Returns the address of the memory for for the specified word.
 */
 // TODO RENAME to compiler_find_word())
bool dictionary_find_entry(char * name, struct Dictionary_Entry *entry)
{
    CODE_INDEX entry_index;
    CODE_INDEX current_index;
    CODE_INDEX next_entry;
    char entry_name[32];    
    
    entry_index = search_from;
    log_debug(LOG, " looking for '%s' starting at @%08x", name, entry_index);
    while (entry_index != LAST_ENTRY) {
        current_index = entry_index;
        next_entry = read_address(&current_index);
        entry->flags = *current_index >> 5;
        uint8_t len = *current_index++ & 0x1f;
        strncpy(entry_name, current_index, len);
        entry_name[len] = 0;

        // log_trace(LOG, "   checking '%s' at %08x (%08x)", entry_name, entry_index, current_index + len);
        if (strcmp(entry_name, name) == 0) {
            current_index += len;
            log_debug(LOG, " code found for '%s' at %08x", entry_name, current_index);
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
            return true;
        }
        entry_index = next_entry;
    }
    log_debug(LOG, "   token not found");
    return false;
}

// TODO rename to refer to character?
uint8_t dictionary_read_next_byte(struct Process *process) 
{
    return *(process->ip)++;
}

static CELL read(CODE_INDEX *offset) 
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

uint32_t dictionary_read(struct Process *process) 
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
void dictionary_find_word_for(CODE_INDEX instruction, char *name) {
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    bool short_memory;

    log_debug(LOG, "seeking name for instruction %08x (%02x)", instruction, last_function_index);

    entry = search_from;
    strcpy(name, "Unknown!");        
    while (entry != LAST_ENTRY) {
        next_entry = read_address(&entry);
//        if (entry >= last_core_entry)
//        {
//            entry = next_entry;
//            continue;
//        }
        
        uint8_t len = *entry++ & 0x1f;
        CODE_INDEX memory_at = entry + len;
        short_memory = entry <= last_core_entry;
//        log_trace(LOG, " checking %sentry %08x (%02x)", short_memory ? "short " : "", memory_at, *memory_at);
        if ((short_memory && ((uint32_t) instruction) == *memory_at) || (!short_memory && instruction == memory_at)) {
            strncpy(name, entry,len);
            name[len] = 0;
            log_debug(LOG, " found %s %08x", name, memory_at);
            break;
        }

        entry = next_entry;
    }
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
		printf("No entries\n");
	}
    else
    {
        printf("\n");
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
                printf("\n");
                width = len;
            }
            printf("%s ", name);
            entry = next_entry;
        }
        printf("\n\n");
    }
}

void dictionary_memory_dump(CODE_INDEX start, uint16_t size) {
    uint32_t i, j;
    uint32_t offset = (uint32_t) (start == 0 ? memory : start);
    uint32_t from = offset;
    from = from - (from % 16);
    uint32_t end = offset + size;
    uint32_t to = end / 16 * 16 + (end % 16 == 0 ? 0 : 16);
    
	printf ("\n%08x   ", offset);
	for (i = 0; i < 16; i++) {
		printf("%X   ", i);
	}
	printf ("\n");
    
	for (i = from; i < to; i++) {
        bool new_line = i % 16 == 0;
		if (new_line) {
			printf("\n%08X %s", i, i == offset ? "[" : " ");
		}
		printf("%02X%s", *((CODE_INDEX) i), (!new_line && i == offset - 1) ? "[" : (i == end - 1 ? "]" : " "));
        
        if (i % 16 == 15) {
            printf("  ");
            for (j = 0; j < 16; j++)
            {
                if (j == 8) printf(" ");
                //uint8_t cc = *((CODE_INDEX) (i - 15 + j));
                char c = *(((CODE_INDEX) i) - 15 + j);
                if (c < 32) c = '.';
                printf("%c", c);
            }
        }
	}
	printf("\n\n");
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
         printf("func<%08X>", instruction);
     }
     else
     {
        char name[32];
        CODE_INDEX location = find_entry(instruction, name);
        printf("%s @%04X", name, location);
     }
}
      
static void debug_print(CODE_INDEX addr, uint8_t length)
{
    printf("  %04X ", addr);
    int i;
    for (i = 0; i < length; i++) {
        printf("%02X", *addr++);
    }
    printf("  ");
}

void dictionary_debug_entry(CODE_INDEX instruction)
{
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    CODE_INDEX start_at, end_at;
    CODE_INDEX addr;
    char name[32];
    CODE_INDEX value;
    int8_t relative;
    
    log_debug(LOG, " debug entry for @%08x", instruction);
    
    entry = search_from;
    end_at = new_entry_offset;
    while (entry != LAST_ENTRY)
    {
        start_at = entry;
        next_entry = read_address(&entry);
        if (start_at <= instruction) {
            uint8_t len = *entry++ & 0x1f;
            strncpy(name, entry, len);
            name[len] = 0;
            break;
        }
        
        end_at = start_at;
        entry = next_entry;
    }

    printf("Word '%s' at %04X~%04X", name, start_at, end_at);
    entry = start_at;
    value = read_address(&entry);
    printf(", next %04X\n", value);
    entry += strlen(name) + 1;

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
                printf("LIT %i", value2);
            }
            else if (function == memory_address)
            {
                value = align(entry);
                debug_print(addr, entry - addr);
                printf("ADDR %08x", value);
            }
            else if (function == return_to)
            {
                debug_print(addr, entry - addr);
                printf("RETURN");
            }
            else if (function == branch)
            {
                relative = *entry++;
                debug_print(addr, entry - addr);
                printf("BRANCH %i", relative);
            }
            else if (function == zero_branch)
            {
                relative = *entry++;
                debug_print(addr, entry - addr);
                printf("ZBRANCH %i", relative);
            }
            else if (function == nop)
            {
                debug_print(addr, entry - addr);
                printf("NOP");
            }
            else
            {
                dictionary_find_word_for(value, name);
                CORE_FUNC function = core_functions[(uint32_t) value];
                debug_print(addr, entry - addr);
                printf("%s  func(%x)<%08x>", name, value, function);
            }
        }
        else 
        {
            dictionary_find_word_for(value, name);
            debug_print(addr, entry - addr);
            printf("%s (%08x)", name, value);
        }
        printf("\n");
    }
}

void dictionary_debug()
{
//    int i;
//    for (i = 0; i < CORE_WORDS; i++) {
//        printf("%0X -> %08X", i, core_functions[i]);
//        
//        printf("\n");
//    }
//   
    printf("last: %08x\n", search_from);
    printf("new: %08x\n", new_entry_offset);
    printf("locked at: %08x\n", locked_before);
    printf("memory at: %08x\n\n", memory);
    
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    char name[32];

	if (search_from == LAST_ENTRY) {
		printf("No entries\n");
	}
    
    entry = search_from;
    while (entry != LAST_ENTRY)
    {
        printf("%08x ", entry);
        next_entry = read_address(&entry);
        uint8_t byte = *entry++;
        uint8_t len = byte & 0x1f;
        strncpy(name, entry, len);
        name[len] = 0;
        printf("%s [%i] ", name, byte >> 5);
        if (log_level <= DEBUG) printf("[%08x + %i]  ", entry, len);
        if (entry <= last_core_entry)
        {
            // C function address
            uint8_t short_memory = *(entry + len);
            printf(" func(%x)<%08x>\n", short_memory, core_functions[short_memory]);            
        }
        else
        {
            // position of the memory
            printf(" -> %08x\n", entry + len);
        }
        entry = next_entry;
    }
}
