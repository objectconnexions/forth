#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <GenericTypeDefs.h>

#include "debug.h"
#include "logger.h"
#include "dictionary.h"


#define LOG "Dictionary"

#define CODE_SIZE 1024 * 8
#define CORE_WORDS 128

static uint8_t code[CODE_SIZE];
static CORE_FUNC *core_functions;

static CODE_INDEX last_entry_offset; // last non-scratch entry in dictionary
static CODE_INDEX new_entry_offset;  // next free space in dictionary for a new entry
static CODE_INDEX last_core_entry;
static uint8_t last_function_index;
static uint8_t* insertion_point;

static uint16_t find_word(char *);
static uint16_t find_dictionay_entry(char *);
static uint32_t read(CODE_INDEX *);

void dictionary_init()
{
    core_functions = (CORE_FUNC *)malloc(sizeof(CORE_FUNC) * CORE_WORDS);
    dictionary_reset();
}
    
void dictionary_reset()
{
    last_entry_offset = LAST_ENTRY;
    new_entry_offset = 0;
    insertion_point = code;
    
    memset(code, 0, CODE_SIZE);   
}

static void add_entry(char *name, uint8_t flags)
{
    log_trace(LOG, "   previous word ends at %04X", dictionary_offset() - 1);

    int len = strlen(name);
    insertion_point = code + new_entry_offset;              
    log_debug(LOG, "   adding entry for '%s' at @%04X", name, dictionary_offset());

    // previous entry
    uint16_t previous_entry;
    if (new_entry_offset == 0) {
        previous_entry = LAST_ENTRY;  // first entry; code to indicate no more previous
    } else {
        previous_entry = last_entry_offset;
    }
    log_debug(LOG, "   previous_entry @%04x, %i", previous_entry, len & 0x1f);
    dictionary_append_value(previous_entry);
    dictionary_append_value((len & 0x1f) | (flags << 5));

    strcpy(insertion_point, name);
    insertion_point += len;
    log_debug(LOG, "   code at ~%04X (at %08X)", dictionary_offset(), insertion_point);
}

void dictionary_add_entry(char *name)
{
    add_entry(name, 0);
}

void dictionary_append_byte(uint8_t value)
{
    *insertion_point++ = value;
}

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

void dictionary_append_instruction(FORTH_WORD word)
{
    dictionary_append_value(word);
}

void dictionary_write_byte(CODE_INDEX index, uint8_t value)
{
    code[index] = value;
}

void dictionary_end_entry() 
{
    last_entry_offset = new_entry_offset;
    new_entry_offset = insertion_point - code;
}

void dictionary_insert_internal_instruction(uint8_t entry, CORE_FUNC function)
{
    core_functions[entry] = function;
    last_function_index = entry >= last_function_index ? entry + 1: last_function_index;
}

void dictionary_add_core_word(char * name, CORE_FUNC function, bool immediate)
{
    uint8_t flags = 0;
    
    log_debug(LOG, "core word %s %02X @%04x", name, last_function_index, dictionary_offset());
    if (immediate)
    {
        flags = IMMEDIATE;
        add_entry(name, flags);
    }
  
    add_entry(name, flags);
    core_functions[last_function_index] = function;
    dictionary_append_value(last_function_index);
    last_function_index++;
    dictionary_end_entry();
    last_core_entry = new_entry_offset;
}




/*
    Returns the address of the code for for the specified word.
 */
 // TODO RENAME to compiler_find_word())
bool dictionary_find_entry(char * name, struct Dictionary_Entry *entry)
{
    CODE_INDEX entry_index;
    CODE_INDEX current_index;
    char entry_name[32];    
    
    entry_index = last_entry_offset;
    log_debug(LOG, " looking for '%s' at @%04x", name, entry_index);
    while (entry_index != LAST_ENTRY) {
        current_index = entry_index;
        entry_index = read(&current_index);
        entry->flags = code[current_index] >> 5;
        uint8_t len = code[current_index++] & 0x1f;
        strncpy(entry_name, &(code[current_index]), len);
        entry_name[len] = 0;

        log_trace(LOG, "   checking '%s' at @%04x/@%04x", entry_name, current_index, entry_index);
        if (strcmp(entry_name, name) == 0) {
            current_index += len;
            log_debug(LOG, " code found '%s' at ~%04x", entry_name, current_index);
            // the next byte is the start of the code
            if (current_index < last_core_entry)
            {
                // use short code
                entry->instruction = code[current_index];
            }
            else 
            {
                entry->instruction = current_index;
            }
            return true;
        }
    }
    log_debug(LOG, "   token not found");
    return false;
}


uint8_t dictionary_read_byte(struct Process *process) 
{
    return code[process->ip++];
}

static uint32_t read(CODE_INDEX * offset) 
{
    uint8_t segment;
    uint32_t value = 0;
    uint8_t i = 0;
    
    while (true)
    {
        segment = code[(*offset)++];
        value =  (segment & 0x7F) << (i * 7)  | value ; // (value * 0x80) | (segment & 0x7F);
        if (segment < 0x80) {
            return value;
        }
        i++;
    }
}

uint32_t dictionary_read(struct Process *process) 
{
    return read(&(process->ip));
}

uint32_t dictionary_read_instruction(struct Process *process) 
{
    return read(&(process->ip));
}

bool dictionary_shortcode(CODE_INDEX instruction)
{
    return instruction < CORE_WORDS;
}

void dictionary_execute_function(CODE_INDEX instruction)
{
    core_functions[instruction]();
}

CODE_INDEX dictionary_offset() 
{
    return insertion_point - code;
}

uint32_t dictionary_data_address(CODE_INDEX offset)
{
    return (uint32_t) insertion_point + offset;
}

/*
 Return the address of the dictionary entry for the code at the specified address
 */
void dictionary_find_word_for(CODE_INDEX instruction, char *name) {
    CODE_INDEX entry;
    CODE_INDEX next_entry;

    log_debug(LOG, "seeking name for instruction %04x | %04x", instruction, last_core_entry);

    bool short_code = instruction <= last_core_entry;
    entry = last_entry_offset;
    strcpy(name, "Unknown!");        
    while (entry != LAST_ENTRY) {
        next_entry = read(&entry);
        if (entry >= last_core_entry)
        {
            entry = next_entry;
            continue;
        }
        
        uint8_t len = (code[entry++] & 0x1f);
        log_trace(LOG, " checking entry ~%04x %02x", entry + len, code[entry+ len]);
        if (short_code ? instruction == code[entry + len] : instruction == entry + len) {
            strncpy(name, &(code[entry]), len);
            name[len] = 0;
            break;
        }

        entry = next_entry;
    }
}

bool dictionary_is_core_word(CODE_INDEX entry)
{
    return entry <= last_function_index;
}

/*
 * List the entries in the dictionary
 */
void dictionary_words() {
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    char name[32];
    uint8_t width = 0;
    
	if (last_entry_offset == LAST_ENTRY)
    {
		printf("No entries\n");
	}
    else
    {
        entry = last_entry_offset;
        while (entry != LAST_ENTRY)
        {
            next_entry = (CODE_INDEX) read(&entry);
            uint8_t len = code[entry++] & 0x1f;
            strncpy(name, &(code[entry]), len);
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
    }
}

void dictionary_memory_dump(uint16_t start, uint16_t size) {
    int i, j;
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
		printf("%02X%s", code[i], (!new_line && i == start - 1) ? "[" : (i == end - 1 ? "]" : " "));
        
        if (i % 16 == 15) {
            printf("  ");
            for (j = 0; j < 16; j++)
            {
                if (j == 8) printf(" ");
                char c = code[i - 15 + j];
                if (c < 32) c = '.';
                printf("%c", c);
            }
        }
	}
	printf("\n\n");
}

/*
 * Return the offset of the dictionary that contains the specified offset (typically called using the offset
 * of the code.
 */
static CODE_INDEX find_entry(CODE_INDEX offset, char * name)
{
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    CODE_INDEX start_at;
       
    entry = last_entry_offset;
    while (entry != LAST_ENTRY)
    {
        start_at = entry;
        next_entry = (CODE_INDEX) read(&entry);
        if (start_at <= offset) {
            uint8_t len = code[entry++] & 0x1f;
            strncpy(name, &(code[entry]), len);
            name[len] = 0;
            return start_at;
        }
        entry = next_entry;
    }
    
    return LAST_ENTRY;
}
       
// TODO removed as not in use
void dictionary_debug_summary(uint32_t instruction)
{
     if (instruction > 0x90000000) 
     {
         // TODO lookup word that matches function address
         printf("func()");
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
        printf("%02X", code[addr++]);
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
    uint32_t value;
    uint8_t offset;
    
    log_debug(LOG, " debug entry for @%04x", instruction);
    
    entry = last_entry_offset;
    end_at = new_entry_offset;
    while (entry != LAST_ENTRY)
    {
        start_at = entry;
        next_entry = (CODE_INDEX) read(&entry);
        if (start_at <= instruction) {
            uint8_t len = code[entry++] & 0x1f;
            strncpy(name, &(code[entry]), len);
            name[len] = 0;
            break;
        }
        
        end_at = start_at;
        entry = next_entry;
    }

    printf("Word '%s' at %04X~%04X", name, start_at, end_at);
    entry = start_at;
    value = read(&entry);
    printf(", next %04X\n", value);
    entry += strlen(name) + 1;

    while (entry < end_at) 
    {
        addr = entry;
        value = read(&entry);
        if (value <= last_function_index) {
            if (value == LIT)
            {
                value = read(&entry);
                debug_print(addr, entry - addr);
                printf("LIT %i", value);
            }
            else if (value == RETURN)
            {
                debug_print(addr, entry - addr);
                printf("RETURN");
            }
            else if (value == BRANCH)
            {
                offset = code[entry++];
                debug_print(addr, entry - addr);
                printf("BRANCH %i", offset - 0x80);
            }
            else if (value == ZBRANCH)
            {
                offset = code[entry++];
                debug_print(addr, entry - addr);
                printf("ZBRANCH %i", offset - 0x80);
            }
            else if (value == NOP)
            {
                debug_print(addr, entry - addr);
                printf("NOP");
            }
            else
            {
                dictionary_find_word_for(value, name);
                value = (uint32_t) core_functions[value];
                debug_print(addr, entry - addr);
                printf("%s  func()<%08x>", name, value);
            }
        }
        else 
        {
            dictionary_find_word_for(value, name);
            debug_print(addr, entry - addr);
            printf("%s", name);
        }
        printf("\n");
    }
}

void dictionary_debug()
{
    int i;
    for (i = 0; i < CORE_WORDS; i++) {
        printf("%0X -> %08X", i, core_functions[i]);
        
        printf("\n");
    }
   
    
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    char name[32];

	if (last_entry_offset == LAST_ENTRY) {
		printf("No entries\n");
	}
    
    entry = last_entry_offset;
    while (entry != LAST_ENTRY)
    {
        printf("%04x ", entry);
        next_entry = (CODE_INDEX) read(&entry);
        uint8_t byte = code[entry++];
        uint8_t len = byte & 0x1f;
        strncpy(name, &(code[entry]), len);
        name[len] = 0;
        printf("%s [%i] ", name, byte >> 5);
        if (log_level <= DEBUG) printf("[%04x + %i]  ", entry, len);
        if (entry <= last_core_entry)
        {
            // C function address
            printf(" func()<%08x>\n", core_functions[code[entry + len]]);            
        }
        else
        {
            // position of the code
            printf(" -> %04x\n", entry + len);
        }
        entry = next_entry;
    }
}