#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <GenericTypeDefs.h>
#include <proc/p32mx570f512h.h>
#include <pic32m_builtins.h>
#include "sys/kmem.h"

#include "uart.h"
#include "logger.h"
#include "dictionary.h"
#include "forth.h"
#include "interpreter.h"
//#include "flash.h"


#define LOG "Dictionary"

#define PAGE_SIZE 4096

#ifdef MX130
    #define CODE_RAM_SIZE (PAGE_SIZE * 2)
    #define CODE_FLASH_SIZE (PAGE_SIZE * 4)
#else
    #define CODE_RAM_SIZE (PAGE_SIZE * 4)
    #define CODE_FLASH_SIZE (PAGE_SIZE * 8)
    #define CODE_FLASH_PROXY_SIZE (PAGE_SIZE * 4)
#endif
#define CORE_WORDS 220


#define PAD 24

static uint8_t code_ram[CODE_RAM_SIZE];
static uint8_t code_flash_proxy[CODE_FLASH_PROXY_SIZE] = {0};
static const uint8_t __attribute__ ((aligned (PAGE_SIZE))) code_flash[CODE_FLASH_SIZE] = {[0 ... CODE_FLASH_SIZE - 1] = 0xff};

static uint8_t flash_buffer[160];

struct Memory
{
    bool flash_write;
    CODE_INDEX first_entry;
    CODE_INDEX insertion_point;
    CODE_INDEX next_entry;
    CODE_INDEX last_entry;
    CODE_INDEX limit;
};

static struct Memory ram_memory;
static struct Memory flash_proxy_memory;
static struct Memory flash_memory;
//static struct Memory flash_buffer_memory;
static struct Memory* memory;


static void clear_memory(CODE_INDEX);
static bool is_valid_address(uint32_t);
static uint64_t read(CODE_INDEX *);
static CODE_INDEX peek_address(CODE_INDEX);
static CODE_INDEX read_address(CODE_INDEX *);
static CODE_INDEX find_entry(CODE_INDEX, char *);
//static void read_top_entry(struct Dictionary_Entry *);
static bool read_next_entry(struct Dictionary_Entry *);
static void debug(bool);
static void out_mem_map();
static void append_cell(CODE_INDEX *, CELL);
void flash_write_word_to(uint32_t, uint32_t);
static void flash_write_word(CELL);
static void flash_buffer_to_flash();



static void reset(struct Memory *m, CODE_INDEX code, uint32_t size)
{
   m->first_entry = m->next_entry = m->last_entry = m->insertion_point = code;
   m->limit = code + size;
}

void dictionary_init()
{
    log_info(LOG, "%I bytes allocated at %Z", CODE_RAM_SIZE, code_ram);
    log_info(LOG, "code (RAM) at %Z", code_ram);
    log_info(LOG, "code (FLASH) at %Z", code_flash);
    log_info(LOG, "code (FLASH proxy) at %Z", code_flash_proxy);
    log_info(LOG, "functions at %Z", core_funcs);
        
    reset(&flash_memory, (CODE_INDEX) code_flash, CODE_FLASH_SIZE);
    reset(&flash_proxy_memory, code_flash_proxy, CODE_FLASH_PROXY_SIZE);
    reset(&ram_memory, code_ram, CODE_RAM_SIZE);

//    struct Memory *m = &flash_buffer_memory;
//    m->first_entry = m->next_entry = m->last_entry = (CODE_INDEX) code_flash;
//    m->insertion_point = flash_buffer;
//    m->limit = flash_buffer + 160;

    
    clear_memory(ram_memory.first_entry);
    
    memory = &flash_proxy_memory;
    
    dictionary_append_cell(BASE_ENTRY);
}

void dictionary_init_done()
{
    memory = &ram_memory;
    dictionary_append_cell((CELL) flash_proxy_memory.last_entry);
    ram_memory.last_entry = flash_proxy_memory.last_entry;
}

void dictionary_master_reset()
{   
    uint8_t i = 0;
    while (peek_address((CODE_INDEX) (flash_proxy_memory.limit - i * 8)))
    {
        i++;
    }
    
    if (i > 0)
    {
        CODE_INDEX addr = peek_address((CODE_INDEX) (flash_proxy_memory.limit - i * 4));
        ram_memory.limit -= i * 8;
        ram_memory.first_entry = addr;
        ram_memory.next_entry = addr;
        
        addr = peek_address((CODE_INDEX) (flash_proxy_memory.limit - i * 8));
        flash_proxy_memory.limit -= i * 8;
        flash_proxy_memory.next_entry = addr;
    }
    
    
    clear_memory(ram_memory.first_entry);

    out_mem_map();
}

static void truncate_after(struct Dictionary_Entry *entry)
{        
    // TODO reimplement
    
//    insertion_point = next_ram_entry = entry->ends + 1;
//    last_ram_entry = entry->starts;
//    log_debug(LOG, "top entry reset to %Z, next entry reset to %Z", last_ram_entry, next_ram_entry);
//    clear_memory(next_ram_entry);
}

void dictionary_truncate_at(struct Dictionary_Entry *entry)
{        
        read_next_entry(entry);
        truncate_after(entry);
}

void dictionary_purge(struct Dictionary_Entry *entry)
{
    if (entry->starts > ram_memory.first_entry)
    {
        /*
         * adjust pointers within the RAM to overwrite the remaining section
         */
        ram_memory.next_entry = entry->starts;
        ram_memory.insertion_point = entry->starts;
        ram_memory.last_entry = read_address(&ram_memory.insertion_point);
        
    } else {
        /*
         * adjust pointers within the RAM so all old code will be overwritten
         */
        ram_memory.insertion_point = ram_memory.first_entry;
        ram_memory.last_entry = ram_memory.first_entry;
        ram_memory.next_entry = ram_memory.first_entry;
        
        /*
         * adjust back pointer from RAM to the remaining flash area
         */        
        append_cell(&ram_memory.insertion_point, (CELL) flash_proxy_memory.insertion_point);
        
        /*
         * adjust back pointers within the flash to go back to remaining code (_IDLE word)
         */        
        dictionary_find_entry_for("_IDLE", entry);
        flash_write_word((CELL) entry->starts);
        flash_buffer_to_flash();
    }
}

static void clear_memory(CODE_INDEX from)
{
    uint32_t offset = ((uint32_t) from) - ((uint32_t) ram_memory.first_entry);
    memset(from, 0, CODE_RAM_SIZE - offset);   
}

uint32_t dictionary_unused() 
{
    uint32_t used = ((uint32_t) memory->insertion_point) - ((uint32_t) memory->last_entry);
    return CODE_RAM_SIZE - used;
}

CODE_INDEX dictionary_here()
{
    return memory->insertion_point;
}

CODE_INDEX dictionary_pad()
{
    // TODO what should this be. It's not the same as dictionary_here
    return memory->insertion_point;
}

static void append_flag(uint8_t len, uint8_t flags)
{
    dictionary_append_byte((len & 0x1f) | (flags << 5));
}

// TODO move this code into next function
static CODE_INDEX add_entry(char *name, uint8_t flags)
{
    // note, the pointer to the previous entry alreay exists.
    int len = strlen(name);
    log_debug(LOG, "   new entry for '%S' (%I chars) (%Z; next previous %Z)", name, len & 0x1f, memory->insertion_point, memory->next_entry);
    append_flag(len, flags);
    dictionary_append_string(name);
    log_debug(LOG, "   code start %Z", memory->insertion_point);
    return memory->insertion_point;
}

CODE_INDEX dictionary_add_entry(char *name)
{
    return add_entry(name, 0);
}

void dictionary_end_entry() 
{
    log_trace(LOG, "    entry ends at %Z", memory->insertion_point - 1);
    memory->last_entry = memory->next_entry;
    memory->next_entry = memory->insertion_point;
    log_debug(LOG, "   link for previous_entry at %Z", memory->last_entry);
    dictionary_append_cell((CELL) memory->last_entry);
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
    // TODO check this logic is still valid now the adding of back pointers has changed
//    if (last_ram_entry > 0) {
//        last_ram_entry = read_address(&last_ram_entry);
//    }
//    memory->insertion_point = memory->link_to_previous;
    memory->insertion_point = memory->next_entry;
    
    log_info(LOG, "abort, stick at %Z", memory->insertion_point);
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
    log_debug(LOG, "alignment offset %I", ((uint32_t) memory->insertion_point) % 4);
    while (((uint32_t) memory->insertion_point) % 4 != 0) {
        dictionary_append_byte(0);
    }
    log_debug(LOG, "aligned to %Z", memory->insertion_point);
}

void dictionary_allot(int32_t size)
{
    size = size < 0 ? 0 : size;
    log_debug(LOG, "allot %I bytes", size);
    memory->insertion_point += size;
}

static void write_byte(CODE_INDEX *code, uint8_t value)
{
    log_trace(LOG, "write %X to %Z", value, *code);
    *(*code) = value;
    (*code)++;
}

void dictionary_append_byte(uint8_t value)
{
    // TODO refactor into function with multiple callers
    if (memory->flash_write && memory->insertion_point >= memory->limit)
    {
        log_warn(LOG, "out of memory %Z", memory->insertion_point);
    } 
    else
    {
        write_byte(&(memory->insertion_point), value);
    }
}

/*
 * Append the specified value to the next four bytes. This sets up the memory in a 
 * way the CPU can access it via it hardware. 
 */
static void append_cell(CODE_INDEX *code, CELL value)
{
    if (is_valid_address((uint32_t) *code)) 
    {
        write_byte(code, value & 0xff);
        write_byte(code, value >> 8 & 0xff);
        write_byte(code, value >> 16 & 0xff);
        write_byte(code, value >> 24 & 0xff);
    }
}

/*
 * Append the specified value to the next four bytes. This sets up the memory in a 
 * way the CPU can access it via it hardware. 
 */
void dictionary_append_cell(CELL value)
{
    // TODO check memory
    append_cell(&(memory->insertion_point), value);
}

static void write_literal(CODE_INDEX *code, uint32_t value)
{
    append_cell(code, value);
}

/*
 Encode the specified value as a series (variable number of bytes)
 */
void dictionary_append_literal(uint32_t value)
{
    // TODO check memory
    write_literal(&(memory->insertion_point), value);
}

void dictionary_append_string(char const * text)
{
    uint8_t len = strlen(text);
    // TODO check memory
    strcpy(memory->insertion_point, text);
    memory->insertion_point += len;
}

void dictionary_append_function(CORE_FUNC function)
{
    uint32_t instruction =  (uint32_t) function;
    instruction &= 0x0FFFFFFF;
    instruction |= 0x90000000;
    dictionary_append_cell(instruction);
}

void dictionary_append_instruction(struct Dictionary_Entry entry)
{
    if (entry.core)
    {
        dictionary_append_function((CORE_FUNC) entry.instruction);
    }
    else
    {
        uint32_t instruction =  (uint32_t) entry.instruction;
        if (entry.instruction >= ram_memory.first_entry && entry.instruction <= ram_memory.limit)
        {
            instruction &= 0x0FFFFFFF;
            instruction |= 0xB0000000;
        } 
        else
        {
            instruction &= 0x0FFFFFFF;
            instruction |= 0xA0000000;
        }
        dictionary_append_cell(instruction);
    }
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
 Mark the most recent entry as IMMEDIATE
 */
void dictionary_mark_internal() {
   CODE_INDEX current_index = memory->next_entry;
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
    if (end < ram_memory.first_entry) 
    {
        entry->ends = flash_proxy_memory.next_entry;
    }
    else
    {
        entry->ends = end;
    }
    read_address(&current_index);

    entry->flags = *current_index >> 5;
    uint8_t len = *current_index++ & 0x1f;
    strncpy( entry->name, current_index, len);
    entry->name[len] = 0;

    current_index += len;
    log_trace(LOG, " code for '%S' at %Z~%Z", entry->name, entry->starts, entry->ends);
    entry->core = false;
    entry->instruction = current_index;
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
    entry_details(ram_memory.next_entry, ram_memory.next_entry - 1, entry);
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
}

/*
    Returns the address of the memory for for the specified word.
 */
 // TODO RENAME to compiler_find_word())
bool dictionary_find_entry_for(char * name, struct Dictionary_Entry *entry)
{
    CODE_INDEX start = memory->last_entry;
    log_debug(LOG, " start scan for '%S' at entry at %Z", name, start);
    entry_details(start, ram_memory.next_entry - 1, entry);
    do
    {
        if (strcicmp(entry->name, name) == 0)
        {
            log_debug(LOG, " code found for '%S' at %Z (~%Z)", entry->name, entry->starts, entry->ends);
            return true;
        }
    } while (read_next_entry(entry));
    
    int i;
    // TODO store size in constant
    for (i = 0; i < 200; i++) {
        struct CORE_ENTRY elem = core_funcs[i];
        if (elem.name != NULL &&  strcicmp(elem.name, name) == 0)
        {
            log_debug(LOG, " core code found for '%S' at %Z", elem.name, elem.function);            
            strcpy(entry->name, elem.name);
            entry->instruction = (INSTRUCTION) elem.function;
            entry->core = true;
            entry->flags = elem.immediate ? IMMEDIATE : 0;      
            return true;
        }
    }

    log_debug(LOG, "   token not found (%S)", name);
    return false;
}

static uint8_t read_byte(CODE_INDEX *code) 
{
    CODE_INDEX addr = *code;
//    log_trace(LOG, "read %Z (from %Z)", addr, code);
    
    uint8_t read = *(*code);
//    log_trace(LOG, "     %Z -> %X", addr, read);
    (*code)++;
    return read;
}

// TODO rename to refer to character?
uint8_t dictionary_read_next_byte(struct Process *process) 
{
    return read_byte(&process->ip);
}

// TODO is valid RAM address (upper limit will depend on chip!)
static bool is_valid_address(uint32_t address)
{
    if ((address >= 0xBF800000 && address <= 0xBF8FFFFF) ||
            (address >= 0x9D000000 && address <= 0x9D07FFFF) ||
            (address >= 0xA0000000 && address <= 0xA000FFFF) ||
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
    if (is_valid_address((uint32_t) *offset))
    {
    //    CODE_INDEX addr = *offset;
        uint32_t read =
                read_byte(offset) |
                read_byte(offset) << 8 |
                read_byte(offset) << 16 |
                read_byte(offset) << 24;
    //    log_trace(LOG, "read %Z -> %Z", addr, read);   
        return read;
    }
}

static CODE_INDEX peek_address(CODE_INDEX code)
{
    CODE_INDEX at = code;
    return read_address(&at);
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

CODE_INDEX dictionary_offset() 
{
    return memory->insertion_point;
}

/*
 Return the address of the dictionary entry for the memory at the specified address
 */
bool dictionary_find_word_for(CODE_INDEX code_pointer, char *name) {
    CODE_INDEX entry;
    CODE_INDEX next;
    
    log_debug(LOG, "seeking name for instruction %Z", code_pointer);
        
    strcpy(name, "Unknown!");        
    if (code_pointer == BASE_ENTRY) 
    {
        return false;
    }

    entry = ram_memory.last_entry;
    while (entry != BASE_ENTRY)
    {
        next = read_address(&entry);
        
        uint8_t name_len = *entry++ & 0x1f;
        CODE_INDEX memory_at = entry + name_len;
        if (code_pointer == memory_at)
        {
            strncpy(name, entry, name_len);
            name[name_len] = 0;
            log_debug(LOG, " found %S %Z", name, memory_at);
            return true;
        }

        entry = next;
    }
    
   int i;
    // TODO store size in constant
    for (i = 0; i < 200; i++) 
    {
        struct CORE_ENTRY elem = core_funcs[i];
        if (((CODE_INDEX) elem.function) == code_pointer)
        {
            log_debug(LOG, " core code found for '%S' at %Z", elem.name, elem.function);          
            if (elem.name != NULL) {
                strcpy(name, elem.name);
            }
            return true;
        }
    }
   
   return false;
}

/*
 * List the entries in the dictionary
 */
void dictionary_words() {
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    char name[32];
    uint8_t width = 0;
    
    console_put(NL);
    entry = ram_memory.next_entry;
    entry = read_address(&entry);

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

    int i;
    for (i = 0; i < 200; i++)
    {
        struct CORE_ENTRY elem = core_funcs[i];
        if (elem.function == NULL)
        {
            break;
        }
        if (elem.name != NULL)
        {
            uint8_t len = strlen(elem.name);
            width += len + 1;
            if (width > 80) 
            {
                console_put(NL);
                width = len;
            }
            console_out(elem.name);
            console_put(SPACE);
        }
    }

    console_put(NL);
    console_put(NL);
}

void dictionary_memory_dump(CODE_INDEX start, uint16_t size) {
    uint32_t addr, col;
    uint32_t offset = (uint32_t) (start == 0 ? ram_memory.first_entry : start);
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
       
    entry = ram_memory.last_entry;
    entry = read_address(&entry);

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
    CODE_INDEX value;
   
    start_at = entry->starts;
    log_debug(LOG, " debug entry for %Z", start_at);
    end_at = entry->ends;
    // TODO would this ever occur?
    if (end_at == BASE_ENTRY)
    {
        end_at = memory->insertion_point - 1;
    }
    
    
    console_out("Word '%S'\n  %Z - %Z\n  %I bytes", entry->name, start_at, end_at, end_at - start_at + 1);
    
    entry_index = start_at;
    value = read_address(&entry_index);
    console_out("\n", value);
    debug_print(start_at, entry_index);
    console_out("^%Z\n", value);
    
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
       console_put(NL);
    }
}

// TODO this code look similar to that in the loop above
int8_t dictionary_print_instruction(CODE_INDEX *addr)
{
    char name[32];               // entry's name
    int8_t level = 0;
    int16_t relative;
    CODE_INDEX ptr = *addr;
    CODE_INDEX value = read_address(&ptr);
    
    uint32_t instruction = ((uint32_t) value);
    uint32_t type = instruction & 0xF0000000;
    switch(type)
    {
            
        case 0x80000000:
            // flash word
            // TODO change to 0x90... for real flash
            dictionary_find_word_for((CODE_INDEX) (instruction | 0xA0000000), name);
            debug_print(*addr, ptr);
            console_out("%S", name);
            console_pad(PAD - strlen(name));
            console_out("(%Z)", instruction | 0xA0000000);
            level = 1;
            
            break;

        case 0x90000000:
            ;
            CORE_FUNC function = (CORE_FUNC) (instruction | 0x9D000000);
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
               
               ptr = value;
               console_put(NL);
               console_out("  - aligned to %Z", value);
               
               
               // value is the address where the data starts; don't know where it ends!
               // if we pass it in then we can loop through here and print data
               // same for next
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
               dictionary_find_word_for((CODE_INDEX) function, name);
               debug_print(*addr, ptr);
               console_out("%S", name);
               console_pad(PAD - strlen(name));
               console_out("func <%Z>", (CODE_INDEX)function);
            }
            break;
            
        case 0xA0000000:
            // flash word
            dictionary_find_word_for((CODE_INDEX) (instruction | 0xA0000000), name);
            debug_print(*addr, ptr);
            console_out("%S", name);
            console_pad(PAD - strlen(name));
            console_out("(%Z)", instruction | 0xA0000000);
            level = 1;
            
            break;
            
        case 0xB0000000:
            // ram word
            dictionary_find_word_for((CODE_INDEX) (instruction & 0xEFFFFFFF), name);
            debug_print(*addr, ptr);
            console_out("%S", name);
            console_pad(PAD - strlen(name));
            console_out("(%Z)", instruction & 0xEFFFFFFF);
            level = 1;
            
            break;
            
        case 0xC0000000:
            // branch
            relative = instruction & 0x0000FFFF;
            debug_print(*addr, ptr);
            console_out("BRANCH");
            console_pad(PAD - 6);
            console_out("%Z (%I)", *addr + 4 + relative, relative);            
            break;

        case 0xD0000000:
            // zero branch
            relative = instruction & 0x0000FFFF;
            debug_print(*addr, ptr);
            console_out("ZBRANCH");
            console_pad(PAD - 6);
            console_out("%Z (%I)", *addr + 4 + relative, relative);            
            break;
 
        case 0x00000000:
            debug_print(*addr, ptr);
            console_out("LIT");
            console_pad(PAD - 3);
            console_out("%Z", value);
            break;

        case 0x40000000:
            // for double literals
            break;
            
        default:
     
            break;

     }

    *addr = ptr;

    return level;
}

static void out_mem_map() 
{
    console_out("Flash\n");
    console_out("  first entry: %Z\n", flash_memory.first_entry);
    console_out("  last entry: %Z\n", flash_memory.last_entry);
    console_out("  next entry: %Z\n", flash_memory.next_entry);
    console_out("  limit: %Z\n", flash_memory.limit);
    console_out("  used: %I/%I\n", flash_memory.next_entry - flash_memory.first_entry, flash_memory.limit - flash_memory.first_entry);
    console_out("  insert at: %Z\n\n", flash_memory.insertion_point);

    console_out("flash buffer: %Z\n\n", flash_buffer);
     
    console_out("Proxy\n");
    console_out("  first entry: %Z\n", flash_proxy_memory.first_entry);
    console_out("  last entry: %Z\n", flash_proxy_memory.last_entry);
    console_out("  next entry: %Z\n", flash_proxy_memory.next_entry);
    console_out("  limit: %Z\n", flash_proxy_memory.limit);
    console_out("  used: %I/%I\n", flash_proxy_memory.next_entry - flash_proxy_memory.first_entry, flash_proxy_memory.limit - flash_proxy_memory.first_entry);
    console_out("  insert at: %Z\n\n", flash_proxy_memory.insertion_point);

    console_out("RAM\n");
    console_out("  first entry: %Z\n", ram_memory.first_entry);
    console_out("  last entry: %Z\n", ram_memory.last_entry);
    console_out("  next entry: %Z\n", ram_memory.next_entry);
    console_out("  limit: %Z\n", ram_memory.limit);
    console_out("  used: %I/%I\n", ram_memory.next_entry - ram_memory.first_entry, ram_memory.limit - ram_memory.first_entry);
    console_out("  insert at: %Z\n\n", ram_memory.insertion_point);
}

// TODO rename to a more appropriate name
static void debug(bool include_functions)
{
    out_mem_map();
    
    if (include_functions)
    {
        int i;

        CORE_FUNC last = core_funcs[0].function;
        CORE_FUNC first = last;
        for (i = 0; i < 200; i++) {
            struct CORE_ENTRY func = core_funcs[i];
            if (func.function != NULL)
            {
                if (func.function < first) first = func.function;
                if (func.function > last) last = func.function;
            }
        }
        console_out("functions\n");
        console_out("  first entry: %Z\n", first);
        console_out("  last entry: %Z\n\n", last);
           
        for (i = 0; i < 200; i++) {
            struct CORE_ENTRY func = core_funcs[i];
            if (func.function != NULL) 
            {
                console_out("  %I. %S func<%Z> %S\n", i, func.name, func.function, func.immediate ? " immediate" : "");            
            }

        }
        console_out(" --- \n");
    }
    
    CODE_INDEX prev_code = ram_memory.last_entry;
    CODE_INDEX last_code = ram_memory.next_entry;
    CODE_INDEX code = prev_code;
    char name[32];

    log_debug(LOG, "first entry at %Z, last at %Z", code, last_code);
        
    bool in_ram = true;
    
    while (code != BASE_ENTRY)
    {     
        CODE_INDEX start = code;
//        console_out("%Z  ", start);
        
        if (last_code == ram_memory.first_entry)
        {
            last_code = flash_proxy_memory.next_entry;
        }
        
        uint16_t size = last_code - start;
        prev_code = read_address(&code);
        uint8_t byte = *code++;
        uint8_t name_len = byte & 0x1f;
        strncpy(name, code, name_len);
        name[name_len] = 0;
        code += name_len;
        console_out("%Z  %Z  (%Z)  %S", start, code, prev_code, name);
//        console_out(name);
        console_pad(30 - name_len);
        console_out("[%X]  ", byte >> 5);

        // position of the code_ram
//        console_out("  %I/%I word<%Z>  (%Z)\n", len, size, code + len, prev_code);
        uint16_t code_len = last_code - code;
        console_out("  %I (%I/%I)\n", size, name_len, code_len);

        last_code = start;
        code = prev_code;
    }
}

void dictionary_debug()
{
    debug(false);
}

void dictionary_debug_all()
{
    debug(true);
}

void dictionary_move_to_proxy()
{
    console_out("Copy %I bytes to FLASH\n", ram_memory.next_entry - ram_memory.first_entry);
    
    memory = &flash_proxy_memory;
    log_debug(LOG, "copy code from %Z to %Z", ram_memory.first_entry, memory->insertion_point);
     
    struct Dictionary_Entry entry;
    CODE_INDEX ram_src;
    CODE_INDEX old_instruction, new_instruction;
    CODE_INDEX start;
    
    CODE_INDEX variables = ram_memory.first_entry;
    log_debug(LOG, "variables go to %Z", variables);
 
    ram_src = ram_memory.first_entry;
    
    while (ram_src < ram_memory.next_entry)
    {
        log_debug(LOG, "- looking for entry at %Z", ram_src);
        if (dictionary_find_entry_with(ram_src, &entry))
        {
            start = memory->next_entry;
            int len = entry.ends - entry.starts + 1;
            console_out("%S : %Z~%Z -> %Z/%Z (%I bytes)\n", entry.name, entry.starts, entry.ends, memory->next_entry, memory->insertion_point, len);

            uint8_t name_len = strlen(entry.name);
            ram_src = entry.starts;
            read_address(&ram_src);
            log_debug(LOG, "   copy entry for '%S' (%I chars) at %Z -> %Z", entry.name, name_len, ram_src, memory->insertion_point);
            append_flag(name_len, entry.flags);
            dictionary_append_string(entry.name);

            ram_src += 1 + name_len;
            old_instruction = ram_src;
            new_instruction = dictionary_here();  // where the new code now starts
            
            // copy code - exactly
            while (ram_src <= entry.ends)
            {
//                log_trace(LOG, "   copy code %X from %Z -> %Z", *ram_src, ram_src, memory->insertion_point);
//                dictionary_append_byte(*ram_src++);
                CODE_INDEX instruction = read_address(&ram_src);
                
                log_debug(LOG, "   copy instruction %Z", instruction);
                
                
                if (instruction == (CODE_INDEX) push_literal)
                {
                    CELL value2 = (CELL) read_address(&ram_src);
                    dictionary_append_cell((CELL) instruction);
                    dictionary_append_cell(value2);
                    log_debug(LOG, "     append literal %Z", value2);
                    continue;
                    
                }
                else if (instruction == (CODE_INDEX) memory_address)
                {
//                    CELL value2 = read_address(&ram_src);
//                    log_debug(LOG, "     append address %Z", value2);

//                   value = dictionary_aligned(ptr);
//                   debug_print(*addr, ptr);
//                   console_out("ADDR");
//                   console_pad(PAD - 4);
//                   console_out("%Z", value);
                    
                    
                    len = entry.ends - ram_src;
                    variables = dictionary_aligned(variables);
                    log_debug(LOG, "     variable (%I bytes) at %Z", len, variables);

                    dictionary_append_function(push_literal);
                    dictionary_append_literal((uint32_t) variables);
                    dictionary_append_function(return_to);
                    dictionary_end_entry();
                    
                    ram_src += len;
                    variables += len;
                    
                }
                else if (instruction == (CODE_INDEX) data_address)
                {
                    CELL value2 = (CELL) read_address(&ram_src);
                    log_debug(LOG, "     append data %Z", value2);
//
//                   value = dictionary_aligned(ptr);
//                   debug_print(*addr, ptr);
//                   console_out("DATA");
//                   console_pad(PAD - 4);
//                   console_out("%Z", value);
                }
                else if (instruction == (CODE_INDEX) print_string || instruction == (CODE_INDEX) c_string || instruction == (CODE_INDEX) s_string)
                {
                    log_debug(LOG, "     append string");
                    continue;
                }
                
                uint32_t type = ((uint32_t) instruction) & 0xF0000000;
                switch(type)
                {
                    case 0xB0000000:
                        instruction = (CODE_INDEX) (((uint32_t) instruction) & 0xEFFFFFFF);
                        log_debug(LOG, "       translate %Z to %Z", instruction, peek_address(instruction));
                        instruction = peek_address(instruction);
                        break;
                }
                log_debug(LOG, "     append instruction %Z", instruction);
                dictionary_append_cell((uint32_t) instruction);
            }

            
                
            memory->last_entry = memory->next_entry;
            memory->next_entry = memory->insertion_point;
            log_debug(LOG, "   link for previous_entry at %Z", new_instruction);
            dictionary_append_cell((CELL) start);
            
            // writes the new address into the RAM entry for lookup in rest of transfer
            write_literal(&old_instruction, (uint32_t) new_instruction);
        }
        else
        {
            break;
        }
    }
    reset(&ram_memory, ram_memory.first_entry, CODE_RAM_SIZE);
    ram_memory.next_entry = variables;
    ram_memory.insertion_point = variables;
    memory = &ram_memory;
    log_debug(LOG, "write last entry %Z to %Z", start, memory->insertion_point);
    dictionary_append_cell((CELL) start); // sets the previous link in ram to the last entry in flash
    
    ram_memory.last_entry = flash_proxy_memory.last_entry;
    
//    IS this writing to unaligned memory?
        
        
    CODE_INDEX data_map = flash_proxy_memory.limit - 8;
    log_debug(LOG, "write addresses %Z & %Z to %Z", flash_proxy_memory.next_entry, variables, data_map);
    write_literal(&data_map, (uint32_t) variables);
    write_literal(&data_map, (uint32_t) flash_proxy_memory.next_entry);
    
    out_mem_map();

}

//static uint8_t flash_buffer[100];

static uint8_t flash_buffer_index;

static void add_flash_buffer_byte(uint8_t byte)
{
    flash_buffer[flash_buffer_index++] = byte;
}

static void add_flash_bufffer_cell(CELL data)
{
    add_flash_buffer_byte(data & 0xff);
    add_flash_buffer_byte(data >> 8 & 0xff);
    add_flash_buffer_byte(data >> 16 & 0xff);
    add_flash_buffer_byte(data >> 24 & 0xff);
}

static void flash_buffer_to_flash()
{
    uint8_t over = flash_buffer_index % 4;
    uint8_t end = flash_buffer_index / 4 * 4;
    log_debug(LOG, "  flash @%I, end %I, over %I", flash_buffer_index, end, over);
    int i;
//    int last = flash_buffer_index - 4;
    for (i = 0; i < end; i += 4)
    {
        CELL cell = flash_buffer[i + 3] << 24 |
                flash_buffer[i + 2] << 16 |
                flash_buffer[i + 1] << 8 |
                flash_buffer[i + 0];
        flash_write_word(cell);
        log_debug(LOG, "   - write @%Z", cell);

    }
//    log_debug(LOG, "  written @%I end %I", flash_buffer_index, end);
    for (i = 0; i < over; i++)
    {
        log_debug(LOG, "   - copy %I to %I", end+i, i);
        flash_buffer[i] = flash_buffer[end + i];
    }
    flash_buffer_index = over;
}


void dictionary_move_to_flash()
{
    console_out("Copy %I bytes to FLASH\n", ram_memory.next_entry - ram_memory.first_entry);
    
    memory = &flash_proxy_memory;
    CODE_INDEX insert_at = memory->insertion_point;
    log_debug(LOG, "copy code from %Z to %Z", ram_memory.first_entry, insert_at);
     
    struct Dictionary_Entry entry;
    CODE_INDEX ram_src;
    CODE_INDEX old_instruction, new_instruction;
    CODE_INDEX new_entry;

    CODE_INDEX variables = ram_memory.first_entry;
    log_debug(LOG, "variables go to %Z", variables);
 
    ram_src = ram_memory.first_entry;
    
    flash_buffer_index = 0;
    
    while (ram_src < ram_memory.next_entry)
    {
        log_debug(LOG, "- looking for entry at %Z", ram_src);
        if (dictionary_find_entry_with(ram_src, &entry))
        {
            new_entry = memory->next_entry;
            
            ram_src = entry.starts;
            int entry_len = entry.ends - ram_src + 1;
            console_out("%S : %Z~%Z -> %Z/%Z (%I bytes)\n", entry.name, entry.starts, entry.ends, memory->next_entry, insert_at, entry_len);

            uint8_t name_len = strlen(entry.name);
            
            read_address(&ram_src); // consume backpointer
            log_debug(LOG, "   copy entry for '%S' (%I chars) at %Z -> %Z", entry.name, name_len, ram_src, insert_at);
            
            // copy length/flag and name
            add_flash_buffer_byte(*ram_src++);
            int i;
            for (i = 0; i < name_len; i++) {
                add_flash_buffer_byte(*ram_src++);
            }
            insert_at += 1 + name_len;


            old_instruction = ram_src;
            new_instruction = insert_at;
            
            // copy code - exactly
            while (ram_src <= entry.ends)
            {
                CODE_INDEX instruction = read_address(&ram_src);
                log_debug(LOG, "   copy instruction %Z", instruction);
                if (instruction == (CODE_INDEX) push_literal)
                {
                    CELL value2 = (CELL) read_address(&ram_src);
                    add_flash_bufffer_cell((CELL) instruction);
                    add_flash_bufffer_cell(value2);
                    log_debug(LOG, "     append literal %Z", value2);
                    insert_at += 8;
                    continue;
                    
                }
                else if (instruction == (CODE_INDEX) memory_address)
                {
                    // variables are store aligned in RAM
                    ram_src = dictionary_aligned(ram_src);  
                    uint8_t data_len = entry.ends - ram_src + 1;
                    variables = dictionary_aligned(variables);
                    log_debug(LOG, "     for variable (%I bytes) from %Z~%Z: %Z", data_len, ram_src, entry.ends, variables);

                    add_flash_bufffer_cell((CELL) push_literal);
                    add_flash_bufffer_cell((CELL) variables);
                    add_flash_bufffer_cell((CELL) return_to);
                    insert_at += 12;

                    memory->last_entry = memory->next_entry;
                    memory->next_entry = insert_at;
                    
                    ram_src += data_len;
                    variables += data_len;
                    
                }
                else if (instruction == (CODE_INDEX) data_address)
                {
                    // TODO not always aligned!
//                    
//                   // variables are store aligned in RAM
//                    ram_src = dictionary_aligned(ram_src);  
//                    variables = dictionary_aligned(variables);
//                    len = entry.ends - ram_src + 1;
//                    log_debug(LOG, "     variable (%I bytes) at %Z", len, variables);
//
//                    add_flash_bufffer_cell((CELL) push_literal);
//                    add_flash_bufffer_cell((CELL) variables);
//                    add_flash_bufffer_cell((CELL) return_to);
//                    
//                    memory->last_entry = memory->next_entry;
//                    memory->next_entry = new_instruction + 12;
////                    log_debug(LOG, "     link for previous_entry at %Z", memory->last_entry);
////                    add_flash_bufffer_cell((CELL) memory->last_entry);
//                    
//                    ram_src += len;
//                    variables += len;
                    
                    
                    
                    
                    CELL value2 = (CELL) read_address(&ram_src);
                    log_debug(LOG, "     append data %Z", value2);
//
//                   value = dictionary_aligned(ptr);
//                   debug_print(*addr, ptr);
//                   console_out("DATA");
//                   console_pad(PAD - 4);
//                   console_out("%Z", value);
                    
                }
                else if (instruction == (CODE_INDEX) print_string || instruction == (CODE_INDEX) c_string || instruction == (CODE_INDEX) s_string)
                {
                    log_debug(LOG, "     append string");
                    continue;
                    
                } 
                else 
                {
                    uint32_t type = ((uint32_t) instruction) & 0xF0000000;
                    switch(type)
                    {
                        case 0xB0000000:
                            instruction = (CODE_INDEX) (((uint32_t) instruction) & 0xEFFFFFFF);
                            log_debug(LOG, "       translate %Z to %Z", instruction, peek_address(instruction));
                            instruction = peek_address(instruction);
                            break;
                    }
                    log_debug(LOG, "     append instruction %Z", instruction);
                    add_flash_bufffer_cell((CELL) instruction);
                    insert_at += 4;
    //                dictionary_append_cell((uint32_t) instruction);
                }
            }


                
            memory->last_entry = memory->next_entry;
            memory->next_entry = insert_at;
            log_debug(LOG, "   link for previous_entry at %Z", new_entry);
            add_flash_bufffer_cell((CELL) new_entry);
            insert_at += 4;
            
            flash_buffer_to_flash();
 


            
            // writes the new address into the RAM entry for lookup in rest of transfer
            write_literal(&old_instruction, (((uint32_t) new_instruction) & 0x0FFFFFFF) | 0x80000000);
        }
        else
        {
            break;
        }
    }

    // fill in remaining bytes to cell boundary
    int i;
    for (i = flash_buffer_index; i < 4; i++)
    {
        flash_buffer[flash_buffer_index++] = 0xff;
        log_debug(LOG, "   - stuff %I with 0xff", i);

    }
    flash_buffer_to_flash();

    memory->insertion_point = insert_at;
    
    reset(&ram_memory, ram_memory.first_entry, CODE_RAM_SIZE);
    ram_memory.next_entry = variables;
    ram_memory.insertion_point = variables;
    memory = &ram_memory;
    log_debug(LOG, "write last entry %Z to %Z", new_entry, memory->insertion_point);
    dictionary_append_cell((CELL) new_entry); // sets the previous link in ram to the last entry in flash
    
    ram_memory.last_entry = flash_proxy_memory.last_entry;
    
//    IS this writing to unaligned memory?
        
        
    CODE_INDEX data_map = flash_proxy_memory.limit - 8;
    log_debug(LOG, "write addresses %Z & %Z to %Z", flash_proxy_memory.next_entry, variables, data_map);
    flash_write_word_to((uint32_t) data_map, (uint32_t) variables);
    flash_write_word_to((uint32_t) data_map + 4, (uint32_t) flash_proxy_memory.next_entry);
//    write_literal(&data_map, (uint32_t) variables);
//    write_literal(&data_map, (uint32_t) flash_proxy_memory.next_entry);
    
    out_mem_map();

}

void dictionary_debug2()
{
    out_mem_map();
    dictionary_memory_dump(ram_memory.first_entry, 200);
    dictionary_memory_dump(flash_proxy_memory.first_entry, 200);
    dictionary_memory_dump((CODE_INDEX) code_flash, 200);
    dictionary_memory_dump(flash_buffer, 200);
}






// FLASH Code

#define OP_ERASE_PAGE 4
#define OP_WRITE_WORD 1


static void flash_op(uint8_t op)
{
    int disabled_interrupts = __builtin_disable_interrupts();
    
    /*
     * Set operation, enable write and WR bit.
     * 
     * Unlock the mechanism with the key.
     * 
     * Wait for unlocking and disable
     */
    NVMCONbits.NVMOP = op;
    NVMCONbits.WREN = 1;
    NVMKEY = 0xAA996655;
    NVMKEY = 0x556699AA;
    NVMCONSET = 0x8000;
    while (NVMCONbits.WR)
    {
        ;   // wait
    }
    NVMCONbits.WREN = 0;
    NVMCONbits.WREN = 1;

    if (disabled_interrupts & 0x1)
    {
        __builtin_enable_interrupts();
    }
    
}

void flash_erase()
{
    NVMADDR = KVA_TO_PA(code_flash);
    flash_op(OP_ERASE_PAGE);
}
    
void flash_write_word_to(uint32_t index, uint32_t data)
{
    uint8_t *addr = (uint8_t *)index;
    *addr++ = data & 0xff;
    *addr++ = data >> 8 & 0xff;
    *addr++ = data >> 16 & 0xff;
    *addr++ = data >> 24 & 0xff;
    
//    NVMADDR = KVA_TO_PA(code_flash + index);
//    NVMDATA = data;
//    flash_op(OP_WRITE_WORD);
}
    
static void flash_write_word(CELL data)
{
    flash_write_word_to((uint32_t) flash_proxy_memory.insertion_point, (uint32_t) data);
    flash_proxy_memory.insertion_point += 4;
}
   