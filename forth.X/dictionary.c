#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <GenericTypeDefs.h>
// #include <proc/p32mx570f512h.h>
#include <proc/p32mx270f256d.h>
#include <pic32m_builtins.h>
#include "sys/kmem.h"

#include "uart.h"
#include "logger.h"
#include "dictionary.h"
#include "forth.h"
#include "interpreter.h"
#include "flash.h"
#include "util.h"


#define LOG "Dictionary"

// #define PAGE_SIZE 4096  -- for MX795

#ifdef MX130
    #define PAGE_SIZE 1024
    #define RAM_PAGES 20
    #define FLASH_PAGES 64
#else
    #define PAGE_SIZE 1024
    #define FLASH_PAGES 128
    #define RAM_PAGES 32
    #define CODE_RAM_SIZE (PAGE_SIZE * RAM_PAGES)
    #define CODE_FLASH_SIZE (PAGE_SIZE * FLASH_PAGES)
//    #define CODE_FLASH_PROXY_SIZE (PAGE_SIZE * 4)
#endif
#define CORE_WORDS 220

#define CODE_RAM_SIZE (PAGE_SIZE * RAM_PAGES)
#define CODE_FLASH_SIZE (PAGE_SIZE * FLASH_PAGES)


#define PAD 24
#define EMBED_CODE_SIZE 16

static uint8_t code_ram[CODE_RAM_SIZE];static const uint8_t __attribute__ ((aligned (PAGE_SIZE))) code_flash[CODE_FLASH_SIZE] = {[0 ... CODE_FLASH_SIZE - 1] = 0xff};

CODE_INDEX first_entry;
CODE_INDEX next_entry;
CODE_INDEX insertion_point;

CODE_INDEX next_flash_entry;
// TODO remove - this is based on next entry above
CODE_INDEX flash_insertion_point;


static void clear_memory(CODE_INDEX);
static bool is_valid_address(uint32_t);
static uint64_t read(CODE_INDEX *);
static CODE_INDEX peek_address(CODE_INDEX);
static CODE_INDEX read_address(CODE_INDEX *);
static CODE_INDEX find_entry(CODE_INDEX, char *);
static bool read_next_entry(struct Dictionary_Entry *);
static void debug(bool);
static void append_cell(CODE_INDEX *, CELL);
static uint32_t encode_function(CORE_FUNC);
static void write_memory_setup(void); 





void dictionary_init(CODE_INDEX *interpreter_code, CODE_INDEX *idle_code)
{
    log_info(LOG, "%I bytes allocated at %Z", CODE_RAM_SIZE, code_ram);
    log_info(LOG, "code (RAM) at %Z", code_ram);
    log_info(LOG, "code (FLASH) at %Z", code_flash);
    log_info(LOG, "functions at %Z", core_funcs);

    dictionary_master_reset();
    
    *interpreter_code = (CODE_INDEX) code_flash;
    *idle_code = (CODE_INDEX) code_flash + 8;
}

static CODE_INDEX ram_limit() 
{
    return code_ram + CODE_RAM_SIZE;
}

static CODE_INDEX flash_limit() 
{
    CODE_INDEX end_of_flash = (CODE_INDEX) (code_flash + CODE_FLASH_SIZE);

    while (peek_address((CODE_INDEX) end_of_flash - 8) != (CODE_INDEX) 0xffffffff)
    {
        log_debug(LOG, "  ends at %Z", end_of_flash);
        // addresses in flash after main code
        end_of_flash -= 8;
    }
    return end_of_flash;

}

/*
 * Master reset ensures the flash, ram and its pointers are all set up before they
 * are used. If there is no code in flash, then it is initialised with two routines
 * that allow for the interpreter to run and an idle loop pick up any inactivity.
 * 
 * If there are not offsets  specified in the end of the flash area, then there is 
 * no code in flash and none in ram. Hence all pointers are set to the default positions.
 * 
 * Otherwise, the offsets are read in and the pointers are set up allocate any variable
 * space and to point to the next entry in flash. Also, the link from the ram entries
 * to flash entries is established.
 */
void dictionary_master_reset()
{      
    bool blank_memory = peek_address((CODE_INDEX) code_flash) == (CODE_INDEX) 0xFFFFFFFF;   
    if (blank_memory) {
        // new device, no flash code
        uint32_t addr = (uint32_t) code_flash;
        
        // code for INTERP process, running interpreter_run()
        flash_write_word_to(addr, encode_function(interpreter_run));
        addr += 4;
        flash_write_word_to(addr, 0xC000FFF8); // branch back (8 bytes) to re-run interpreter
                                        // F8 FF 00   TERACTIV E.y.....
                                        // A00040A0  C0
        addr += 4;
                
        // code for IDLE process, running yield()
        flash_write_word_to(addr, encode_function(yield));
        addr += 4;

        flash_write_word_to(addr, 0xC000FFF8); // branch back to be re run idle loop
        addr += 4;

        // set up first entries previous link-back, which indicates it is the  first entry
        flash_write_word_to(addr, (uint32_t) BASE_ENTRY);      
        
        log_info(LOG, "loaded initial flash data, next at %Z", addr + 4);
    }

    CODE_INDEX dict_offsets = flash_limit();
    // TODO name is wrong - to invert
    bool code_in_flash = dict_offsets != ((CODE_INDEX) code_flash + CODE_FLASH_SIZE);
    if (!code_in_flash)
    {
        next_flash_entry = (CODE_INDEX) code_flash + EMBED_CODE_SIZE;
        // TODO move to point before copying to flash
        flash_insertion_point = next_flash_entry + 4;

        first_entry = next_entry = insertion_point = code_ram;
        
        dictionary_append_cell((CELL) BASE_ENTRY);
        log_info(LOG, "  initialise empty flash, ram code starts at %Z, insert at %Z", next_entry, insertion_point);

    }
    else
    {
        next_flash_entry = peek_address(dict_offsets + 4);
        flash_insertion_point = next_flash_entry + 4;
        log_debug(LOG, "  flash code continues at %Z, insert at %Z", next_flash_entry, flash_insertion_point);

        
        first_entry = peek_address(dict_offsets);
        next_entry = first_entry;
        insertion_point = next_entry;
        // set up back pointer for first entry in ram so it points to last entry in flash
        CODE_INDEX previous_entry = peek_address(next_flash_entry);
        dictionary_append_cell((CELL) previous_entry);
        log_debug(LOG, "  ram code starts at %Z, insert at %Z", first_entry, insertion_point);
    }
 
//    dictionary_display_memory();
    
    log_error(LOG, "completed master reset");
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

// TODO check over this logic
void dictionary_purge(struct Dictionary_Entry *entry)
{
    if (entry->start > first_entry)
    {
        /*
         * adjust pointers within the RAM to overwrite the remaining section
         */
        next_entry = entry->start;
        insertion_point = entry->start + 4;
        
        clear_memory(insertion_point);
        
    } else {
        /*
         * adjust pointers within the RAM so all old code will be overwritten
         */
        next_entry = first_entry;
        insertion_point = first_entry;
        clear_memory(insertion_point);
        
        /*
         * adjust back pointer from RAM to the remaining flash area
         */        
        CODE_INDEX next = peek_address(entry->start);
        append_cell(&insertion_point, (CELL) next);
        
        next_flash_entry = flash_insertion_point;
        
        /*
         * adjust back pointers within the flash to go back to what will be
         * the last entry in the remaining code.
         */        
        flash_prepare_buffer((uint32_t) flash_insertion_point);
        flash_buffer_add_cell((CELL) next);
        flash_write_buffer();
        flash_flush_buffer();
        
        write_memory_setup();
    }
}

// TODO rename to clear ram
static void clear_memory(CODE_INDEX from)
{
    uint32_t offset = ((uint32_t) from) - ((uint32_t) first_entry);
    memset(from, 0, CODE_RAM_SIZE - offset);   
}

uint32_t dictionary_unused() 
{
    uint32_t used = ((uint32_t) insertion_point) - ((uint32_t) first_entry);
    return CODE_RAM_SIZE - used;
}

CODE_INDEX dictionary_here()
{
    return insertion_point;
}

CODE_INDEX dictionary_pad()
{
    // TODO what should this be. It's not the same as dictionary_here
    return insertion_point;
}

static void append_flag(uint8_t len, uint8_t flags)
{
    dictionary_append_byte((len & 0x1f) | (flags << 5));
}

// TODO move this code into next function
static CODE_INDEX add_entry(char *name, uint8_t flags)
{
    to_upper(name);
    // note, the pointer to the previous entry already exists.
    int len = strlen(name);
    log_debug(LOG, "   new entry for '%S' (%I chars) (%Z; next previous %Z)", name, len & 0x1f, 
            insertion_point, next_entry);
    append_flag(len, flags);
    dictionary_append_string(name);
    log_debug(LOG, "   code start %Z", insertion_point);
    return insertion_point;
}

CODE_INDEX dictionary_add_entry(char *name)
{
    return add_entry(name, 0);
}

void dictionary_end_entry() 
{
    log_debug(LOG, "    entry ends at %Z", insertion_point - 1);
    CODE_INDEX previous_entry = next_entry;
    next_entry = insertion_point;
    log_debug(LOG, "   link for previous_entry at %Z", previous_entry);
    dictionary_append_cell((CELL) previous_entry);
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
    insertion_point = next_entry + 4;
    
    log_info(LOG, "abort, stick at %Z", insertion_point);
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
    log_debug(LOG, "alignment offset %I", ((uint32_t) insertion_point) % 4);
    while (((uint32_t) insertion_point) % 4 != 0) {
        dictionary_append_byte(0);
    }
    log_debug(LOG, "aligned to %Z", insertion_point);
}

void dictionary_allot(int32_t size)
{
    size = size < 0 ? 0 : size;
    log_debug(LOG, "allot %I bytes", size);
    insertion_point += size;
}

static void append_byte(CODE_INDEX *code, uint8_t value)
{
    log_trace(LOG, "write %X to %Z", value, *code);
    *(*code) = value;
    (*code)++;
}

void dictionary_append_byte(uint8_t value)
{
    // TODO refactor into function with multiple callers
    if (insertion_point >= ram_limit())
    {
        log_warn(LOG, "out of memory %Z", insertion_point);
    } 
    else
    {
        append_byte(&(insertion_point), value);
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
        append_byte(code, value & 0xff);
        append_byte(code, value >> 8 & 0xff);
        append_byte(code, value >> 16 & 0xff);
        append_byte(code, value >> 24 & 0xff);
    }
}

/*
 * Append the specified value to the next four bytes. This sets up the memory in a 
 * way the CPU can access it via it hardware. 
 */
void dictionary_append_cell(CELL value)
{
    // TODO check memory
    append_cell(&(insertion_point), value);
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
    write_literal(&(insertion_point), value);
}

void dictionary_append_string(char const * text)
{
    uint8_t len = strlen(text);
    // TODO check memory
    strcpy(insertion_point, text);
    insertion_point += len;
}

static uint32_t encode_function(CORE_FUNC function)
{
    uint32_t instruction =  (uint32_t) function;
    instruction &= 0x00FFFFFF;
    instruction |= FUNCTION;
    return instruction;
}

void dictionary_append_function(CORE_FUNC function)
{
    // TODO does this actually do anything?
    dictionary_append_cell(encode_function(function));
}

void dictionary_append_instruction(struct Dictionary_Entry entry)
{
    if (entry.is_core)
    {
        dictionary_append_function((CORE_FUNC) entry.instruction);
    }
    else
    {
        // TODO does this actually change any values?
        uint32_t instruction =  (uint32_t) entry.instruction;
        if (entry.instruction >= first_entry && entry.instruction <= ram_limit())
        {
//            instruction &= 0x0FFFFFFF;
//            instruction |= 0xB0000000;
        } 
        else
        {
            // TODO this can be detected by looking for 0x9D....
//            instruction &= 0x0FFFFFFF;
//            instruction |= 0x80000000;
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
   CODE_INDEX current_index = next_entry;
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
static void entry_details(CODE_INDEX code, CODE_INDEX end, struct Dictionary_Entry *entry)
{
    entry->start = code;
    // swap to flash memory after all entries in RAM
    if (end == first_entry - 1) 
    {
//        entry->ends = flash_proxy_memory.next_entry;
        entry->end = next_flash_entry;
    }
    else
    {
        entry->end = end;
    }
    read_address(&code);

    entry->flags = *code >> 5;
    uint8_t len = *code++ & 0x1f;
    strncpy( entry->name, code, len);
    entry->name[len] = 0;

    code += len;
    log_trace(LOG, " code for '%S' at %Z~%Z", entry->name, entry->start, entry->end);
    entry->is_core = false;
    // TODO refactor - same calculation in move to flash code
    entry->instruction = code;
}

/*
 * Update the entry parameter with the next entry in the dictionary.
 */
static bool read_next_entry(struct Dictionary_Entry *entry)
{
    CODE_INDEX end_of_previous = entry->start - 1;
    CODE_INDEX start_of_previous = read_address(&(entry->start));
    if (start_of_previous == BASE_ENTRY)
    {
        return false;
    } else
    {
        entry_details(start_of_previous, end_of_previous, entry);
        return true;
    }
}

/*
    Returns the address of the memory for for the specified word.
 */
 // TODO RENAME to compiler_find_word())
bool dictionary_find_entry_with(CODE_INDEX search_index, struct Dictionary_Entry *entry)
{
    entry_details(next_entry, next_entry - 1, entry);
    do
    {
        if (search_index >= entry->start && search_index <= entry->end) 
        {
            log_debug(LOG, " code found for '%S' at %Z (%Z~%Z)", entry->name, search_index, entry->start, entry->end);
            return true;
        }
    } while(read_next_entry(entry));
    
    log_debug(LOG, "   entry not found (%Z)", search_index);
    return false;
}

static CODE_INDEX last_entry()
{
    return peek_address(next_entry);    
}

/*
    Returns the address of the memory for for the specified word.
 */
 // TODO RENAME to compiler_find_word())
bool dictionary_find_entry_for(char * name, struct Dictionary_Entry *entry)
{
    CODE_INDEX start = last_entry();
    log_debug(LOG, " start scan for '%S' at entry at %Z", name, start);
    if (start != BASE_ENTRY)
    {
        entry_details(start, next_entry - 1, entry);
        do
        {
            if (strcicmp(entry->name, name) == 0)
            {
                log_debug(LOG, " code found for '%S' at %Z (~%Z)", entry->name, entry->start, entry->end);
                return true;
            }
        } while (read_next_entry(entry));
    }
    
    int i;
    // TODO store size in constant
    for (i = 0; i < 200; i++) {
        struct CORE_ENTRY elem = core_funcs[i];
        if (elem.name != NULL &&  strcicmp(elem.name, name) == 0)
        {
            log_debug(LOG, " core code found for '%S' at %Z", elem.name, elem.function);            
            strcpy(entry->name, elem.name);
            entry->instruction = (INSTRUCTION) ((uint32_t) elem.function & 0x80FFFFFF);
            entry->is_core = true;
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

// TODO a similar function appears in forth.c
// TODO is valid RAM address (upper limit will depend on chip!)
static bool is_valid_address(uint32_t address)
{
    // TODO these are specific to the device (its memory size))
    if ((address >= 0xBF800000 && address <= 0xBF8FFFFF) ||
            (address >= 0x9D000000 && address <= 0x9D07FFFF) ||
            (address >= 0xA0000000 && address <= 0xA000FFFF) ||
            (address >= 0x80000000 && address <= 0x8000FFFF)) 
    {
        return true;
    }
    else
    {
        console_out("MEMORY ACCESS %Z!\n", address); 
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
    return insertion_point;
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

    entry = last_entry();
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
    CODE_INDEX next;
    char name[32];
    uint8_t width = 0;
    
    console_put(NL);
    entry = next_entry;
    entry = read_address(&entry);

    while (entry != BASE_ENTRY)
    {
        next = read_address(&entry);
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
        entry = next;
    }

    int i;
    for (i = 220 - 1; i >= 0; i--)
    {
        struct CORE_ENTRY elem = core_funcs[i];
        if (elem.function != NULL && elem.name != NULL)
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
    uint32_t offset = (uint32_t) (start == 0 ? first_entry : start);
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
    CODE_INDEX next;
    CODE_INDEX start_at;
       
    entry = last_entry();
    entry = read_address(&entry);

    while (entry != BASE_ENTRY)
    {
        start_at = entry;
        next = read_address(&entry);
        if (start_at <= offset) {
            uint8_t len = *entry++ & 0x1f;
            strncpy(name, entry, len);
            name[len] = 0;
            return start_at;
        }
        entry = next;
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
        if (i > 0 && i % 8 == 0) {
            console_out("  ");
        }
        console_out("%X ", *addr++);
    }

    uint8_t pad = PAD - length * 2 - ((length -1) / 2);
    console_pad(pad);
}

void dictionary_debug_entry(struct Dictionary_Entry * entry)
{
    CODE_INDEX entry_index;
    CODE_INDEX start_at, end_at;
    CODE_INDEX value;
   
    start_at = entry->start;
    log_debug(LOG, " debug entry for %Z", start_at);
    end_at = entry->end;
    // TODO would this ever occur?
    if (end_at == BASE_ENTRY)
    {
        end_at = insertion_point - 1;
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
            
        case WORD_IN_FLASH:
            // flash word
            // TODO change to 0x90... for real flash
            dictionary_find_word_for((CODE_INDEX) (instruction | 0x9D000000), name);
            debug_print(*addr, ptr);
            console_out("%S", name);
            console_pad(PAD - strlen(name));
            console_out("word(%Z)", instruction | 0x9D000000);
            level = 1;
            
            break;

        case FUNCTION:
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
            else if (function == data_address)
            {
               value = dictionary_aligned(ptr);
               debug_print(*addr, ptr);
               console_out("ADDR");
               console_pad(PAD - 4);
               console_out("%Z", value);
               
               ptr = value;
               console_put(NL);
               console_out("  - aligned to %Z", value);
               
            }
            else if (function == do_loop_begin)
            {
               debug_print(*addr, ptr);
               console_out("DO");
               console_pad(PAD - 2);
              
            }
            else if (function == do_loop_add_step_and_check)
            {
               debug_print(*addr, ptr);
               console_out("LOOP+");
               console_pad(PAD - 4);
               
            }
            else if (function == do_loop_increment_and_check)
            {
               debug_print(*addr, ptr);
               console_out("LOOP");
               console_pad(PAD - 5);
               
            }
            else if (function == process_address)
            {
               debug_print(*addr, ptr);
               console_out("PROC ADD");
               console_pad(PAD - 8);
               CELL var_addr = read(&ptr);
               console_out("%Z", var_addr);
                
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
            
        case WORD_IN_RAM:
            dictionary_find_word_for((CODE_INDEX) (instruction | WORD_IN_RAM), name);
            debug_print(*addr, ptr);
            console_out("%S", name);
            console_pad(PAD - strlen(name));
            console_out("(%Z)", instruction | WORD_IN_RAM);
            level = 1;
            
            break;
            
        case BRANCH:
            relative = instruction & 0x0000FFFF;
            debug_print(*addr, ptr);
            console_out("BRANCH");
            console_pad(PAD - 6);
            console_out("%Z (%I)", *addr + 4 + relative, relative);            
            break;

        case ZERO_BRANCH:
            relative = instruction & 0x0000FFFF;
            debug_print(*addr, ptr);
            console_out("ZBRANCH");
            console_pad(PAD - 6);
            console_out("%Z (%I)", *addr + 4 + relative, relative);            
            break;
 
        case LITERAL:
            debug_print(*addr, ptr);
            console_out("LIT");
            console_pad(PAD - 3);
            console_out("%Z", value);
            break;

        case 0x40000000:
            debug_print(*addr, ptr);
            console_out("DBL ???");
            
            // for double literals
            break;
            
        default:
            debug_print(*addr, ptr);
            console_out("???");
     
            break;

     }

    *addr = ptr;

    return level;
}

void dictionary_display_memory() 
{
    console_out("RAM\n");
    console_out("  allocated: %Z~%Z\n", code_ram, ram_limit());    
    console_out("  used: %I/%I\n", next_entry - code_ram, CODE_RAM_SIZE);
    console_out("  first entry: %Z\n", first_entry);
    console_out("  next entry: %Z\n", next_entry);
    console_out("  insert at: %Z\n\n", insertion_point);

    console_out("Flash\n");
    console_out("  allocated: %Z~%Z\n", code_flash, code_flash + CODE_FLASH_SIZE);    
    console_out("  used: %I/%I\n", next_flash_entry - code_flash, CODE_FLASH_SIZE);
    console_out("  next entry: %Z\n", next_flash_entry);
    console_out("  insert at: %Z\n\n", flash_insertion_point);
}

// TODO rename to a more appropriate name
static void debug(bool include_functions)
{
    dictionary_display_memory();
    
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
    
    CODE_INDEX prev_code = last_entry();
    CODE_INDEX last_code = next_entry;
    CODE_INDEX code = prev_code;
    char name[32];

    log_debug(LOG, "first entry at %Z, last at %Z", code, last_code);
        
    bool in_ram = true;
    
    while (code != BASE_ENTRY)
    {     
        CODE_INDEX start = code;
        if (last_code == first_entry)
        {
            last_code = next_flash_entry;
        }
        
        uint16_t size = last_code - start;
        prev_code = read_address(&code);
        uint8_t byte = *code++;
        uint8_t name_len = byte & 0x1f;
        strncpy(name, code, name_len);
        name[name_len] = 0;
        code += name_len;
        console_out("%Z  %Z  (%Z)  %S", start, code, prev_code, name);
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


static void write_memory_setup() 
{
    if (next_entry == 0 || first_entry == 0)
    {
        
        log_debug(LOG, "ram memory failure %Z / %Z", first_entry, next_entry);
        dictionary_display_memory();
        return;
    }

    //    IS this writing to unaligned memory?
        
    // allocate the previous 8 bytes to store the two offsets
    CODE_INDEX addr = flash_limit() - 8;
    log_debug(LOG, "write startup addresses %Z & %Z to %Z", next_flash_entry, first_entry, addr);
    // point in  RAM where the variable storage finishes and volatile dictionary entries start
    flash_write_word_to((uint32_t) addr, (uint32_t) first_entry);
    // point in FLASH where the next entry will be placed
    flash_write_word_to((uint32_t) addr + 4, (uint32_t) next_flash_entry);
}

void dictionary_move_to_flash()
{
    struct Dictionary_Entry entry;
    CODE_INDEX source_code;
    CODE_INDEX source_instruction;
    uint32_t destination_instruction;
    CODE_INDEX variables_boundary;
    CODE_INDEX flash_destination;
    CODE_INDEX previous_flash_entry;

    
    flash_insertion_point = next_flash_entry + 4;
    flash_destination = flash_insertion_point;
    flash_prepare_buffer((uint32_t) flash_destination);

    if (next_entry == 0 || first_entry == 0)
    {
            log_debug(LOG, "ram memory failure %Z / %Z", first_entry, next_entry);
            dictionary_display_memory();
            return;
    }
    
    uint16_t size = next_entry - first_entry;
    if (size == 0)
    {
        console_out("No code to copy to FLASH\n");
        return;
    }
    console_out("Copying %I bytes to FLASH\n", size);
    log_debug(LOG, "copy code from %Z to %Z", first_entry, flash_destination);
     
    variables_boundary = first_entry;
    log_debug(LOG, "variables go to %Z", variables_boundary);
 
    source_code = first_entry;
    while (source_code < next_entry)
    {
        log_debug(LOG, "- looking for entry at %Z", source_code);
        if (dictionary_find_entry_with(source_code, &entry))
        {
            source_code = entry.start;
            uint16_t entry_len = entry.end - source_code + 1;
            uint8_t name_len = strlen(entry.name);
            console_out("%S : %Z~%Z -> %Z/%Z (%I bytes)\n", entry.name, entry.start, entry.end, next_flash_entry, flash_destination, entry_len);
            log_debug(LOG, "   copy entry for '%S' (%I chars) at %Z -> %Z", entry.name, name_len, source_code, flash_destination);

            // consume the back-pointer
            read_address(&source_code); 
            
            // copy length/flag and name
            flash_buffer_add_byte(*source_code++);
            int i;
            for (i = 0; i < name_len; i++) {
                flash_buffer_add_byte(*source_code++);
            }
            flash_destination += 1 + name_len;

            // record the start for the code in the source and destination, so we
            // can map one to the other for later entries that refer back to this
            // entry.
            source_instruction = source_code;
            destination_instruction = (uint32_t) flash_destination;
            
            previous_flash_entry = next_flash_entry;

            // copy code - exactly
            while (source_code <= entry.end)
            {
                CODE_INDEX instruction = read_address(&source_code);
                uint32_t type = ((uint32_t) instruction) & 0xFF000000;
                if (type == FUNCTION)
                {
                    // if instruction is function then translate address: 0x80... -> 0x9D...
                    CORE_FUNC function = (CORE_FUNC) (((uint32_t) instruction) | 0x9D000000);
                    log_debug(LOG, "   copying function %Z (%Z)", function, instruction);
                    if (function == push_literal)
                    {
                        CELL value = (CELL) read_address(&source_code);
                        flash_buffer_add_cell(encode_function(push_literal));
                        flash_buffer_add_cell(value);
                        log_debug(LOG, "     append literal value %Z", value);
                        flash_destination += 8;
                        continue;

                    }
                    else if (function == process_address)
                    {
                        // variables are stored aligned in RAM
                        variables_boundary = dictionary_aligned(variables_boundary);
                        flash_buffer_add_cell(encode_function(process_address));
                        flash_buffer_add_cell((CELL) variables_boundary);
                        flash_destination += 8;
                        source_code = entry.end;
                        variables_boundary += 4;

                    }
                    else if (function == data_address)
                    {
                        // variables are stored aligned in RAM
                        variables_boundary = dictionary_aligned(variables_boundary);
                        source_code = dictionary_aligned(source_code);  
                        uint8_t data_len = entry.end - source_code + 1;
                        log_debug(LOG, "     for variable (%I bytes) from %Z~%Z: %Z", data_len, source_code, entry.end, variables_boundary);

                        flash_buffer_add_cell(encode_function(push_literal));
                        flash_buffer_add_cell((CELL) variables_boundary);
                        flash_buffer_add_cell(encode_function(return_to));
                        flash_destination += 12;
                        source_code += data_len;
                        variables_boundary += data_len;

                    }
                    else if (function == print_string || function == c_string || function == s_string)
                    {
                        flash_buffer_add_cell(encode_function((CORE_FUNC) instruction));
                        log_debug(LOG, "     append string");
                        uint8_t len = read_byte(&source_code);
                        flash_buffer_add_byte(len);
                        int i;
                        for (i = 0; i < len; i++) {
                            flash_buffer_add_byte(read_byte(&source_code));
                        }
                        flash_destination += 4 + len + 1;

                    } 
                    else
                    {
                        flash_buffer_add_cell((CELL) instruction);
                        flash_destination += 4;
                    }
                }
                else 
                {
                    if (type == WORD_IN_RAM)
                    {
                        // map old ram instruction to flash instruction (saved earlier)
                        log_debug(LOG, "       translate %Z to %Z", instruction, peek_address(instruction));
                        instruction = peek_address(instruction);
                    }
                    
                    log_debug(LOG, "     append instruction %Z", instruction);
                    flash_buffer_add_cell((CELL) instruction);
                    flash_destination += 4;
                }
            }

            log_debug(LOG, "   link for previous_entry at %Z", previous_flash_entry);
            flash_buffer_add_cell((CELL) previous_flash_entry);
            next_flash_entry = flash_destination;
            flash_destination += 4;
            
            flash_write_buffer();
 
            // writes the new address into the RAM entry for lookup during rest of transfer
            write_literal(&source_instruction, destination_instruction);
        }
        else
        {
            break;
        }
    }
    flash_flush_buffer();
    flash_write_buffer();
    
    flash_insertion_point = flash_destination;

    first_entry = variables_boundary;
    next_entry = variables_boundary;
//    ram_memory.last_entry = flash_memory.last_entry;
    insertion_point = variables_boundary;

    log_debug(LOG, "write last entry %Z to %Z", previous_flash_entry, insertion_point);
    dictionary_append_cell((CELL) previous_flash_entry); // sets the previous link in ram to the last entry in flash
    
    write_memory_setup();
    
    dictionary_display_memory();
}

void dictionary_debug2()
{
    dictionary_display_memory();
    dictionary_memory_dump(first_entry, 250);
//    dictionary_memory_dump(flash_proxy_memory.first_entry, 200);
    dictionary_memory_dump((CODE_INDEX) code_flash, 500);
    dictionary_memory_dump((CODE_INDEX) code_flash + CODE_FLASH_SIZE - 128, 128);
//    dictionary_memory_dump(flash_buffer, 130);
}






// FLASH Code

#define OP_ERASE_PAGE 4
#define OP_WRITE_WORD 1



static uint8_t flash_buffer[160];
static uint8_t flash_buffer_index;
static uint32_t next_flash_write;

static void flash_write_next_word(uint32_t);




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
//    NVMCONbits.WREN = 1;

    if (disabled_interrupts & 0x1)
    {
        __builtin_enable_interrupts();
    }
    
}

void flash_buffer_add_byte(uint8_t byte)
{
    flash_buffer[flash_buffer_index++] = byte;
}

void flash_buffer_add_cell(CELL data)
{
    flash_buffer_add_byte(data & 0xff);
    flash_buffer_add_byte(data >> 8 & 0xff);
    flash_buffer_add_byte(data >> 16 & 0xff);
    flash_buffer_add_byte(data >> 24 & 0xff);
}

/*
 * Set up buffer with 0xff for each byte already written previously. This will ensure
 * the code is not changed when a word is written to flash.
 */
void flash_prepare_buffer(uint32_t address)
{
    flash_buffer_index = 0;

    uint8_t offset = address % 4;
    if (offset > 0) {
        log_debug(LOG, "set up flash buffer with %I blank bytes", offset);
        // fill buffer with non-changing bytes (blank flash is 0xff) where code already exists
        uint8_t i;
        for (i = 0; i < offset; i++)
        {
            flash_buffer_add_byte(0xff);
        }
        // move back so insert into aligned memory
    }
    next_flash_write = address - offset;
    log_debug(LOG, "will write to %Z", next_flash_write);
}

void flash_flush_buffer()
{
          
    //    TODO stuff the last bytes,  but keep the insertion point; set up next entry to new  position
    
    uint8_t len = 4 - flash_buffer_index;
    // fill in remaining bytes to the cell boundary
    int i;
    for (i = 0; i < len; i++)
    {
        flash_buffer[flash_buffer_index++] = 0xff;
        log_debug(LOG, "   - stuff %I with 0xff", i);

    }
    flash_write_buffer();
    flash_insertion_point -= len + 1;

}

void flash_write_buffer()
{
    uint8_t over = flash_buffer_index % 4;
    uint8_t end = flash_buffer_index / 4 * 4;
    log_debug(LOG, "  flash %Z: @%I, end %I, over %I", next_flash_write, flash_buffer_index, end, over);     
    uint8_t i;
    for (i = 0; i < end; i += 4)
    {
        uint32_t cell = flash_buffer[i + 3] << 24 |
                flash_buffer[i + 2] << 16 |
                flash_buffer[i + 1] << 8 |
                flash_buffer[i + 0];
        flash_write_next_word(cell);
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

void flash_erase()
{
    int i;
    for (i = 0; i < FLASH_PAGES; i++) {
        CODE_INDEX page = (CODE_INDEX) code_flash + i * PAGE_SIZE;
        log_info(LOG, "erase page at %Z (to %Z)", page, page + PAGE_SIZE - 1);
        NVMADDR = KVA_TO_PA(page);
        flash_op(OP_ERASE_PAGE);
        log_debug(LOG, "erased %Z, %I - %Z", NVMCON, NVMCONbits.WRERR, NVMADDR);
    }
    log_debug(LOG, "erased %I pages", FLASH_PAGES);

}
    
void flash_write_word_to(uint32_t index, uint32_t data)
{
    NVMADDR = KVA_TO_PA(index);
    NVMDATA = data;
    flash_op(OP_WRITE_WORD);
}
    
static void flash_write_next_word(uint32_t data)
{
    flash_write_word_to((uint32_t) next_flash_write, (uint32_t) data);
    next_flash_write += 4;
}
   