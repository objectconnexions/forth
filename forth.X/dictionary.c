#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <GenericTypeDefs.h>
//#include <machine/types.h>

#include "uart.h"
#include "logger.h"
#include "dictionary.h"
#include "forth.h"
#include "interpreter.h"


#define LOG "Dictionary"

#ifdef MX130
    #define CODE_SIZE (1024 * 8)
#else
    #define CODE_SIZE (1024 * 4)
#endif
#define CORE_WORDS 220

#define PAD 24

static uint8_t code_ram[CODE_SIZE];
//static const uint8_t __attribute__ ((aligned (4096))) code_flash[1024 * 16] = {0};
static uint8_t code_flash[1024 * 16] = {0};

static CORE_FUNC core_functions[CORE_WORDS];
static uint8_t top_function_index;

static CODE_INDEX last_ram_entry; // last non-scratch entry in dictionary
static CODE_INDEX next_ram_entry;  // next free space in dictionary for a new entry
static CODE_INDEX last_core_entry;
static CODE_INDEX last_sys_entry;
static CODE_INDEX ram_insertion_point;

static CODE_INDEX last_flash_entry; // last non-scratch entry in dictionary
static CODE_INDEX next_flash_entry;  // next free space in dictionary for a new entry

static void clear_memory(CODE_INDEX);
static uint64_t read(CODE_INDEX *);
static CODE_INDEX read_address(CODE_INDEX *);
static CODE_INDEX find_entry(CODE_INDEX, char *);
static void read_top_entry(struct Dictionary_Entry *);
static bool read_next_entry(struct Dictionary_Entry *);


void dictionary_init()
{
//    memory = malloc(CODE_SIZE);
//    log_info(LOG, "%I bytes allocated at %Z", CODE_SIZE, memory);
//    uint16_t size = sizeof(CORE_FUNC) * CORE_WORDS;
//    log_info(LOG, "allocating %I bytes for function lookup", size);
//    core_functions = (CORE_FUNC *)malloc(size);
    log_info(LOG, "code (RAM) at %Z", code_ram);
    log_info(LOG, "code (FLASH) at %Z", code_flash);
    log_info(LOG, "functions at %Z", core_functions);
    top_function_index = 0;
    
    last_core_entry = code_ram;
    last_sys_entry = code_ram;
    last_ram_entry = BASE_ENTRY;
    next_ram_entry = code_ram;
    ram_insertion_point = next_ram_entry;
    
    last_flash_entry = BASE_ENTRY;
    next_ram_entry = code_flash;

    clear_memory(code_ram);
}

void dictionary_init_done()
{
    last_sys_entry = next_ram_entry;
}

void dictionary_master_reset()
{
    struct Dictionary_Entry entry;
    if (next_ram_entry > last_sys_entry)
    {
        entry.starts = last_sys_entry;
        dictionary_truncate_at(&entry);
    }

    //    dictionary_truncate_after(last_sys_entry);
}

static void truncate_after(struct Dictionary_Entry *entry)
{        
    ram_insertion_point = next_ram_entry = entry->ends + 1;
    last_ram_entry = entry->starts;
    log_debug(LOG, "top entry reset to %Z, next entry reset to %Z", last_ram_entry, next_ram_entry);
    clear_memory(next_ram_entry);
}

void dictionary_truncate_at(struct Dictionary_Entry *entry)
{        
        read_next_entry(entry);
        truncate_after(entry);
}

/*
void dictionary_truncate_after(CODE_INDEX entry_with)
{        
    struct Dictionary_Entry entry;
    if (dictionary_find_entry_with(entry_with, &entry))
    {
        insertion_point = next_entry = entry.ends + 1;
        last_entry = entry.starts;
        log_debug(LOG, "top entry reset to %Z, next entry reset to %Z", last_entry, next_entry);
        clear_memory(next_entry);
    }
}
  */  
static void clear_memory(CODE_INDEX from)
{
    uint32_t offset = ((uint32_t) from) - ((uint32_t) code_ram);
    memset(from, 0, CODE_SIZE - offset);   
}

uint32_t dictionary_unused() 
{
    uint32_t used = ((uint32_t) ram_insertion_point) - ((uint32_t) code_ram);
    return CODE_SIZE - used;
}

CODE_INDEX dictionary_here()
{
    return ram_insertion_point;
}

CODE_INDEX dictionary_pad()
{
    return next_ram_entry;
}

static CODE_INDEX add_entry(char *name, uint8_t flags)
{
    int len = strlen(name);
    ram_insertion_point = next_ram_entry;              
    log_debug(LOG, "   new entry for '%S' (%I chars) at %Z", name, len & 0x1f, ram_insertion_point);

    // previous entry
    CODE_INDEX previous_entry;
    if (next_ram_entry == 0) {
        previous_entry = BASE_ENTRY;  // first entry; memory to indicate no more previous
    } else {
        previous_entry = last_ram_entry;
    }

    last_ram_entry = next_ram_entry;
    
    log_debug(LOG, "   link to previous_entry at %Z", previous_entry);
    dictionary_append_instruction(previous_entry);
    dictionary_append_literal((len & 0x1f) | (flags << 5));
    
    strcpy(ram_insertion_point, name);
    ram_insertion_point += len;
    log_debug(LOG, "   code start %Z", ram_insertion_point);
    return ram_insertion_point;
}

// TODO these two methods will become unneeded when code becomes permanent in FLASH
void dictionary_lock()
{
    CODE_INDEX lock_at = next_ram_entry;
    log_info(LOG, "locked at %Z", lock_at);
    dictionary_add_entry("-LOCK-");    
    dictionary_append_function(return_to);
    dictionary_end_entry();
}

void dictionary_unlock()
{
    struct Dictionary_Entry entry;
    if (!dictionary_find_entry_for("-LOCK-", &entry)) 
    {
        console_out("NO LOCK!");
    } 
    else 
    {
        dictionary_truncate_at(&entry);
//        dictionary_truncate_after(entry.starts);
    }
}

CODE_INDEX dictionary_add_entry(char *name)
{
    return add_entry(name, 0);
}

CODE_INDEX dictionary_aligned(CODE_INDEX address)
{
    uint8_t offset = ((uint32_t) address) % 4;
    log_debug(LOG, "alignment offset %I", offset);
    address = offset == 0 ? address : address + 4 - offset;
    log_debug(LOG, "aligned to %Z", address);
    return address;
}

/*
 * Align data in code_ram so it is on a four-byte boundary as accessing memory at other 
 * positions fails with an exception.
 */
void dictionary_align()
{
    log_debug(LOG, "alignment offset %I", ((uint32_t) ram_insertion_point) % 4);
    while (((uint32_t) ram_insertion_point) % 4 != 0) {
        dictionary_append_byte(0);
    }
    log_debug(LOG, "aligned to %Z", ram_insertion_point);
}

void dictionary_allot(int32_t size)
{
    size = size < 0 ? 0 : size;
    log_debug(LOG, "allot %I bytes", size);
    ram_insertion_point += size;
}

void dictionary_append_byte(uint8_t value)
{
    *ram_insertion_point++ = value;
}

/*
 * Append the specified value to the next four bytes. This sets up the memory in a 
 * way the CPU can access it via it hardware. 
 */
void dictionary_append_cell(CELL value)
{
    dictionary_append_byte(value >> 24 & 0xff);
    dictionary_append_byte(value >> 16 & 0xff);
    dictionary_append_byte(value >> 8 & 0xff);
    dictionary_append_byte(value & 0xff);
}

/*
 Encode the specified value as a series (variable number of bytes)
 */
void dictionary_append_literal(uint64_t value)
{
    uint8_t segment;
    while (true) {
        segment = value % 0x80;
        if (value > segment) {
            segment = segment | 0x80;
            *ram_insertion_point++ = segment;
            value = value / 0x80;
        } else {
            *ram_insertion_point++ = segment;
            break;
        }
    }
}

void dictionary_append_function(CORE_FUNC function)
{
    int i;
    for (i = 0; i < top_function_index; i++) {
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
    dictionary_append_literal((uint32_t) word);
}

uint8_t dictionary_read_byte(CODE_INDEX index) 
{
    return *index;
}

void dictionary_write_byte(CODE_INDEX index, uint8_t value)
{
    *index = value;
}
/*
 * Abort the current entry (or last) entry by reseting the dictionary start location to
 * the previous entry. Needed because the entry is create before it contents are 
 * added, and adding those content could fail (for example if the compile is invalid).
 */
void dictionary_abort_entry() {
//    if (new_entry_offset > 0) {
//        new_entry_offset = ???
//        insertion_point = read_address(&search_from);
//    }
    if (last_ram_entry > 0) {
        last_ram_entry = read_address(&last_ram_entry);
    }
        
    log_info(LOG, "abort, stick at %Z, reset top to %Z", next_ram_entry, last_ram_entry);
}

void dictionary_end_entry() 
{
    log_trace(LOG, "    entry ends at %Z", ram_insertion_point - 1);
//        search_from = new_entry_offset;
    next_ram_entry = ram_insertion_point;
}

void dictionary_insert_internal_instruction(uint8_t entry, CORE_FUNC function)
{
    core_functions[entry] = function;
    top_function_index = entry >= top_function_index ? entry + 1: top_function_index;
}

CODE_INDEX dictionary_add_core_word(char * name, CORE_FUNC function, bool immediate)
{
    if (top_function_index + 1 >= CORE_WORDS)
    {
        log_error(LOG, "too many core words");
    }
    else
    {
        last_core_entry = next_ram_entry;
        
        log_debug(LOG, "core word %S (func %X)", name, top_function_index);
        core_functions[top_function_index] = function;

        if (name)
        {
            add_entry(name, 0);
            dictionary_append_literal(top_function_index);
            dictionary_end_entry();
        }
        if (immediate)
        {
            dictionary_mark_internal();
        }

        top_function_index++;
//        last_core_entry = next_user_entry;
        
        return (CODE_INDEX) (top_function_index - 1);
    }
}

/*
 Mark the most recent entry as IMMEDIATE
 */
void dictionary_mark_internal() {
   CODE_INDEX current_index = last_ram_entry;
   read_address(&current_index);    // move the pointer to right address
   *current_index = *current_index | IMMEDIATE << 5;
}

// TODO move to utils.c
int strcicmp(char const *a, char const *b)
{
    for (;; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a)
            return d;
    }
}

/*
 * Read the dictionary and set up the entry parameter with the details from the 
 * dictionary.
 */

static void entry_details(CODE_INDEX current_index, CODE_INDEX end, struct Dictionary_Entry *entry)
{
    entry->starts = current_index;
    entry->ends = end;

//    next_index = 
    read_address(&current_index);

    entry->flags = *current_index >> 5;
    uint8_t len = *current_index++ & 0x1f;
    strncpy( entry->name, current_index, len);
    entry->name[len] = 0;

    current_index += len;
    log_trace(LOG, " code for '%S' at %Z~%Z", entry->name, entry->starts, entry->ends);
    
    // the next byte is the start of the code_ram
    if (current_index <= last_core_entry)
    {
        // use short memory
        entry->instruction = (CODE_INDEX) (uint32_t) *current_index;
    }
    else 
    {
        entry->instruction = current_index;
    }
}

/*
 * Set up the entry parameter with the top entry of the dictionary.
 */ *ram_insertion_point++ = segment;
static void read_top_entry(struct Dictionary_Entry *entry)
{
    entry_details(last_ram_entry, next_ram_entry - 1, entry);
}

/*
 * Update the entry parameter with the next entry in the dictionary.
 */
static bool read_next_entry(struct Dictionary_Entry *entry)
{
    CODE_INDEX ends = entry->starts - 1;
    CODE_INDEX next = read_address(&(entry->starts));
    if (next == BASE_ENTRY)
    {
        return false;
    } else
    {
        entry_details(next, ends, entry);
        return true;
    }
}




/*
    Returns the address of the memory for for the specified word.
 */
 // TODO RENAME to compiler_find_word())
bool dictionary_find_entry_with(CODE_INDEX search_index, struct Dictionary_Entry *entry)
{
    
    
    read_top_entry(entry);
    do
    {
        if (search_index >= entry->starts && search_index <= entry->ends) 
        {
            log_debug(LOG, " code found for '%S' at %Z (%Z~%Z)", entry->name, search_index, entry->starts, entry->ends);
            return true;
        }
    } while(read_next_entry(entry));
    
    log_debug(LOG, "   entry not found (%Z)", search_index);
    return false;
    
    
    
    /*
    struct Dictionary_Entry e;
    CODE_INDEX current_index;
    CODE_INDEX next_index;

    e.starts = last_entry;
    e.ends = next_entry - 1;
    log_debug(LOG, " looking for entry containing %Z", search_index);
    while (e.starts != BASE_ENTRY) {
        current_index = e.starts;
        next_index = read_address(&current_index);
        e.flags = *current_index >> 5;
        uint8_t len = *current_index++ & 0x1f;
        strncpy(e.name, current_index, len);
        e.name[len] = 0;

        log_trace(LOG, "   checking '%S' at %Z<b<%z", e.name, e.starts, e.ends);
        if (search_index >= e.starts && search_index <= e.ends) {
//        if (strcicmp(e.name, name) == 0) {
            current_index += len;
            log_debug(LOG, " code found for '%S' at %Z (%Z~%Z)", e.name, current_index, e.starts, e.ends);
            // the next byte is the start of the memory
            if (current_index <= last_core_entry)
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
        e.ends = e.starts - 1;
        e.starts = next_index;
    }
    entry->starts = next_index;
    entry->ends = BASE_ENTRY;
//    strcpy(entry->name, name);
    log_debug(LOG, "   entry not found (%Z)", search_index);
    return false;
     * *
     */
}

/*
    Returns the address of the memory for for the specified word.
 */
 // TODO RENAME to compiler_find_word())
bool dictionary_find_entry_for(char * name, struct Dictionary_Entry *entry)
{
    
    
      
    
    read_top_entry(entry);
    do
    {
        if (strcicmp(entry->name, name) == 0)
        {
            log_debug(LOG, " code found for '%S' at %Z (~%Z)", entry->name, entry->starts, entry->ends);
            return true;
        }
    } while( read_next_entry(entry));
    
//    log_debug(LOG, "   entry not found (%Z)", search_index);
    log_debug(LOG, "   token not found (%S)", name);
    return false;
    

    
    
    /*
    
    struct Dictionary_Entry e;
    
    CODE_INDEX current_index;
    CODE_INDEX next_index;

    e.starts = last_entry;
    e.ends = next_entry - 1;
    log_debug(LOG, " looking for '%S' starting at %Z", name, e.starts);
    while (e.starts != BASE_ENTRY) {
        current_index = e.starts;
        next_index = read_address(&current_index);
        e.flags = *current_index >> 5;
        uint8_t len = *current_index++ & 0x1f;
        strncpy(e.name, current_index, len);
        e.name[len] = 0;

        // log_trace(LOG, "   checking '%S' at %08x (%08x)", entry_name, entry_index, current_index + len);
        if (strcicmp(e.name, name) == 0) {
            current_index += len;
            log_debug(LOG, " code found for '%S' at %Z (%Z~%Z)", e.name, current_index, e.starts, e.ends);
            // the next byte is the start of the memory
            if (current_index <= last_core_entry)
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
        e.ends = e.starts - 1;
        e.starts = next_index;
    }
    entry->starts = next_index;
    entry->ends = BASE_ENTRY;
    strcpy(entry->name, name);
    log_debug(LOG, "   token not found (%S)", name);
    return false;
     * 
     * 
     */
}

// TODO rename to refer to character?
uint8_t dictionary_read_next_byte(struct Process *process) 
{
    return *(process->ip)++;
}

// TODO is valid RAM address (upper limit will depend on chip!)
static bool is_valid_address(uint32_t address)
{
    if ((address >= 0xA0000000 && address <= 0xA000FFFF) ||
            (address >= 0x80000000 && address <= 0x8000FFFF)) 
    {
        return true;
    }
    else
    {
        console_out("RAM LIMIT %Z!", address); 
        forth_abort();
        return false;
    }
}

// TODO rename - about reading an encoded literal
static uint64_t read(CODE_INDEX *offset) 
{
    uint8_t segment;
    uint32_t value = 0;
    uint8_t i = 0;
    
    uint32_t address = (uint32_t) *offset;
    if (is_valid_address(address)) 
    {
        while (true)
        {
            segment = *(*offset);
            (*offset)++;
            value =  (segment & 0x7F) << (i * 7)  | value ;
            if (segment < 0x80) {
                return value;
            }
            i++;
        }   
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

// TODO rename is_
bool dictionary_shortcode(CODE_INDEX code_pointer)
{
    return ((uint32_t) code_pointer) < CORE_WORDS;
}

void dictionary_execute_function(CODE_INDEX instruction_pointer)
{
    core_functions[(uint32_t) instruction_pointer]();
}

CODE_INDEX dictionary_offset() 
{
    return ram_insertion_point;
}

/*
 Return the address of the dictionary entry for the memory at the specified address
 */
void dictionary_find_word_for(CODE_INDEX code_pointer, char *name) {
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    bool short_memory;
    
    log_debug(LOG, "seeking name for instruction %Z (%X)", code_pointer, top_function_index);
        
    entry = last_ram_entry;
    strcpy(name, "Unknown!");        
    while (entry != BASE_ENTRY)
    {
        next_entry = read_address(&entry);
        uint8_t len = *entry++ & 0x1f;
        CODE_INDEX memory_at = entry + len;
        short_memory = entry <= last_core_entry;
//        log_trace(LOG, " checking %Sentry %08x (%02x)", short_memory ? "short " : "", memory_at, *memory_at);
        if ((short_memory && ((uint32_t) code_pointer) == *memory_at) || 
                (!short_memory && code_pointer == memory_at)) 
        {
            strncpy(name, entry,len);
            name[len] = 0;
            log_debug(LOG, " found %S %Z", name, memory_at);
            return;
        }

        entry = next_entry;
    }
    
    // TODO return a flag showing success
}

// TODO is this 
bool dictionary_is_core_word(CODE_INDEX entry)
{
    return entry <= (CODE_INDEX) ((uint32_t) top_function_index);
}

/*
 * List the entries in the dictionary
 */
void dictionary_words() {
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    char name[32];
    uint8_t width = 0;
    
	if (last_ram_entry == BASE_ENTRY)
    {
		console_out("No entries\n");
	}
    else
    {
        console_put(NL);
        entry = last_ram_entry;
        while (entry != BASE_ENTRY)
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
    uint32_t offset = (uint32_t) (start == 0 ? code_ram : start);
    uint32_t from = offset;
    from = from - (from % 16);
    uint32_t end = offset + size;
    uint32_t to = end / 16 * 16 + (end % 16 == 0 ? 0 : 16);
    
    if (is_valid_address(from) && is_valid_address(to))
    {
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
}

/*
 * Return the offset of the dictionary that contains the specified offset (typically called using the offset
 * of the code_ram.
 */
// TODO add check for setting of SCRUB flag to prevent recursive calls
static CODE_INDEX find_entry(CODE_INDEX offset, char * name)
{
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    CODE_INDEX start_at;
       
    entry = last_ram_entry;
    while (entry != BASE_ENTRY)
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
    
    return (CODE_INDEX) BASE_ENTRY;
}
       
// TODO removed as not in use
void dictionary_debug_summary(CODE_INDEX code_pointer)
{
     if (code_pointer > (CODE_INDEX) 0x90000000) 
     {
         // TODO lookup word that matches function address
         console_out("func<%Z>", code_pointer);
     }
     else
     {
        char name[32];
        CODE_INDEX location = find_entry(code_pointer, name);
        console_out("%S @%Y", name, location);
     }
}

static void debug_print(CODE_INDEX addr, CODE_INDEX addr2)
{
    uint8_t length = addr2 - addr;
    console_out("%Z  ", addr);
    int i;
    for (i = 0; i < length; i++) {
        if (i > 0 && i % 2 == 0) {
            console_out(" ");
        }
        console_out("%X", *addr++);
    }

    uint8_t pad = PAD - length * 2 - ((length -1) / 2);
    console_pad(pad);
}

void dictionary_debug_entry(struct Dictionary_Entry * entry)
{
    CODE_INDEX entry_index;
    CODE_INDEX start_at, end_at;
//    CODE_INDEX addr;
    CODE_INDEX value;
   

    start_at = entry->starts;
    log_debug(LOG, " debug entry for %Z", start_at);
    end_at = entry->ends;
    if (end_at == BASE_ENTRY)
    {
        end_at = next_ram_entry - 1;
    }
    
    
    console_out("Word '%S' at %Z~%Z", entry->name, start_at, end_at);
    
    entry_index = start_at;
    value = read_address(&entry_index);
    console_out(" (next %Z)\n", value);

    debug_print(start_at, entry_index);
    console_out("%Z\n", value);
    
    uint8_t val = *(entry_index);
    uint8_t flags = val >> 5;
    uint8_t len = val & 0x1F;
    debug_print(entry_index, entry_index + 1);
    console_out("%X | %X\n", flags, len);
    
    debug_print(entry_index + 1, entry_index + 1 + len);
    console_out("%S\n", entry->name);
    
    entry_index += len + 1;

    console_put(NL);
    while (entry_index <= end_at) 
    {        
       dictionary_print_instruction(&entry_index);
//        console_out("         # ");
//        dictionary_print_instruction(addr);
       console_put(NL);

    }
}

// TODO this code look similar to that in the loop above
int8_t dictionary_print_instruction(CODE_INDEX *addr)
{
    char name[32];               // entry's name
    int8_t level = 0;
    int8_t relative;
    CODE_INDEX ptr = *addr;
    CODE_INDEX value = read_address(&ptr);
    if (value <= (CODE_INDEX) ((uint32_t) top_function_index)) {
         CORE_FUNC function = core_functions[(uint32_t) value];
         if (function == push_literal)
         {
            CELL value2 = read(&ptr);
            debug_print(*addr, ptr);
            console_out("LIT");
            console_pad(PAD - 3);
            console_out("%Z", value2);
         }
         else if (function == memory_address)
         {
            value = dictionary_aligned(ptr);
            debug_print(*addr, ptr);
            console_out("ADDR");
            console_pad(PAD - 4);
            console_out("%Z", value);
         }
         else if (function == data_address)
         {
            value = dictionary_aligned(ptr);
            debug_print(*addr, ptr);
            console_out("DATA");
            console_pad(PAD - 4);
            console_out("%Z", value);
         }
         else if (function == return_to)
         {
            debug_print(*addr, ptr);
            console_out("EXIT");
            level = -1;
         }
         else if (function == interpreter_run)
         {
             debug_print(*addr, ptr);
             console_out("INTERPRET");
         }
         else if (function == branch)
         {
            relative = *ptr++ + 1;
            debug_print(*addr, ptr);
            console_out("BRANCH");
            console_pad(PAD - 6);
            console_out("(%I) %Z", relative, *addr + relative);
         }
         else if (function == zero_branch)
         {
            relative = *ptr++ + 1;
            debug_print(*addr, ptr);
            console_out("ZBRANCH");
            console_pad(PAD - 7);
            console_out("(%I) %Z", relative, *addr + relative);
         }
         else if (function == print_string || function == c_string || function == s_string)
         {
            relative = *ptr++;
            debug_print(*addr, ptr);
            // TODO distinguish between print, s and c strings
            console_out("STRING");
            console_pad(PAD - 6);
            console_out("(%I) '", relative);
            int i;
            for (i = 0; i < relative; i++) {
                char c = *ptr++;
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
            debug_print(*addr, ptr);
            console_out("NOP");
         }
         else
         {
            dictionary_find_word_for(value, name);
            CORE_FUNC function = core_functions[(uint32_t) value];
            debug_print(*addr, ptr);
            console_out("%S", name);
            console_pad(PAD - strlen(name));
            console_out("func(%Y)<%Z>", value, function);
         }
     }
     else 
     {
        dictionary_find_word_for(value, name);
        debug_print(*addr, ptr);
        console_out("%S", name);
        console_pad(PAD - strlen(name));
        console_out("(%Z)", value);
        level = 1;
     }        

    *addr = ptr;

    return level;



    /*
    CODE_INDEX addr = code_pointer;
    CODE_INDEX value = read_address(&code_pointer);
    int8_t relative;
    char name[32]; 

    log_trace(LOG, "instruction @ %Z : %Z", code_pointer, value);
        
    if (value <= (CODE_INDEX) ((uint32_t) top_function_index)) {
        CORE_FUNC function = core_functions[(uint32_t) value];
        if (function == push_literal)
        {
            CELL value2 = read(&code_pointer);
            console_out("LIT %Z", value2);
        }
        else if (function == memory_address)
        {
            value = dictionary_aligned(code_pointer);
            console_out("ADDR %Z", value);
        }
        else if (function == data_address)
        {
            value = dictionary_aligned(code_pointer);
           console_out("DATA %Z", value);
        }
        else if (function == return_to)
        {
            console_out("EXIT");
        }
//            else if (function == interpreter_run)
//            {
//                console_out("INTERPRET");
//            }
        else if (function == branch)
        {
            relative = *code_pointer++;
            console_out("BRANCH (%I) %Z", relative, addr + relative);
        }
        else if (function == zero_branch)
        {
            relative = *code_pointer++;
            console_out("ZBRANCH (%I) %Z", relative, addr + relative);
        }
        else if (function == print_string || function == c_string || function == s_string)
        {
            relative = *code_pointer++;
            console_out("STRING (%I) '", relative);
            int i;
            for (i = 0; i < relative; i++) {
                char c = *code_pointer++;
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
            console_out("NOP");
        }
        else
        {
            dictionary_find_word_for(value, name);
            console_out("%S", name);
//            CORE_FUNC function = core_functions[(uint32_t) value];
//            console_out("%S  func(%Y)<%Z>", name, value, function);
        }
    }
    else 
    {
        dictionary_find_word_for(value, name);
        console_out("%S (%Z)", name, value);
    }
     * */

}

static void out_mem_map() 
{
    console_out("base ram: %Z\n", code_ram);
    console_out("base flash: %Z\n", code_flash);
    console_out("next: %Z\n", next_ram_entry);
    console_out("last user:  %Z\n", last_ram_entry);
    console_out("last sys:  %Z\n", last_sys_entry);
    console_out("last core: %Z\n\n", last_core_entry);
}

void dictionary_debug()
{
//    console_out("base: %Z\n", code_ram);
//    console_out("last core: %Z\n", last_core_entry);
//    console_out("last sys:  %Z\n", last_sys_entry);
//    console_out("last user:  %Z\n", last_entry);
//    console_out("next: %Z\n\n", next_entry);
    out_mem_map();
    CODE_INDEX code_pointer;
    CODE_INDEX next_code_pointer;
    CODE_INDEX last_code_pointer;
    char name[32];

	if (last_ram_entry == BASE_ENTRY) {
		console_out("No entries\n");
	}
    
    code_pointer = last_ram_entry;
    last_code_pointer = next_ram_entry;
    while (code_pointer != BASE_ENTRY)
    {
        console_out("%Z  ", code_pointer);
        uint16_t size = last_code_pointer - code_pointer;
        last_code_pointer = code_pointer;
        next_code_pointer = read_address(&code_pointer);
        uint8_t byte = *code_pointer++;
        uint8_t len = byte & 0x1f;
        strncpy(name, code_pointer, len);
        name[len] = 0;
        console_out(name);
        console_pad(30 - len);
        console_out("[%X]  ", byte >> 5);
                
        if (code_pointer <= last_core_entry)
        {
            // C function address
            uint8_t short_memory = *(code_pointer + len);
            console_out("  %I/%I func(%I)<%Z>\n", len, size, short_memory, core_functions[short_memory]);            
        }
        else
        {
            // position of the code_ram
            console_out("  %I/%I -> %Z\n", len, size, code_pointer + len);
        }
        code_pointer = next_code_pointer;
    }
}

void dictionary_move_to_flash()
{
    int offset = last_sys_entry - code_ram;
    int size = last_ram_entry - last_sys_entry;
    
    console_out("Copy %I bytes to FLASH, offset by %I\n", size, offset);
    
    CODE_INDEX start = last_sys_entry;
    struct Dictionary_Entry entry;

//    CODE_INDEX dst = code_flash;
    CODE_INDEX insertion_point = next_flash_entry;

    
    do {
        dictionary_find_entry_with(start, &entry);
        int len = entry.ends - entry.starts + 1;
        console_out("%S : %Z -> %Z (%I bytes)\n", entry.name, entry.starts, entry.ends, len);

//         CODE_INDEX next = read_address(&(entry->starts));
//        
//        
        CODE_INDEX src = entry.starts;
        
             // dispose of next address and get flag
        while (*src++ & 0x80 == 0x80) {
            log_debug(LOG, "%I read", *src); // next byte
        }
        uint8_t flags = *src;
        
        
        int slen = strlen(entry.name);
//        insertion_point = next_entry;              
        log_debug(LOG, "   copy entry for '%S' (%I chars) at %Z", entry.name, slen & 0x1f, insertion_point);

        
        // previous entry
        CODE_INDEX previous_entry;
        if (next_flash_entry == 0) {
            TODO this shoudl link to core code?
            previous_entry = BASE_ENTRY;  // first entry; memory to indicate no more previous
        } else {
            previous_entry = last_flash_entry;
        }

        last_flash_entry = next_flash_entry;

        // TODO all the memory writes need to all be write to FLASH versions
        log_debug(LOG, "   link to previous_entry at %Z", previous_entry);
        
        // build up number for address  - - replaces  dictionary_append_instruction(previous_entry);
        // TODO could refactor dictionary_append_instruction to pass in the destination
        uint64_t value = (uint32_t) previous_entry;
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
        // copy flag/length -- replaces dictionary_append_literal(flags);
        *insertion_point++ = flags;
        
        strcpy(insertion_point, entry.name);
        insertion_point += slen;
        
        log_debug(LOG, "   code start %Z", insertion_point);
        
//        return insertion_point;

        
 
        
//        CODE_INDEX src = entry.starts;
        
        len = entry.ends - src;
 
        int i;
        for(i = 0; i < len; i++)
        {
            *insertion_point++ = *src++;
        }
        
                
        start = entry.ends + 1;
    } while (start < last_ram_entry);
                
}

void dictionary_debug2()
{
    out_mem_map();
    dictionary_memory_dump(last_sys_entry, 200);
    dictionary_memory_dump(code_flash, 200);
}