#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <GenericTypeDefs.h>

#include "debug.h"
#include "dictionary.h"


#define CODE_SIZE 1024 * 8
#define CORE_WORDS 128

static uint8_t code[CODE_SIZE];
static CORE_FUNC *core_functions;

static CODE_INDEX last_entry_offset; // last non-scratch entry in dictionary
static CODE_INDEX new_entry_offset;  // next free space in dictionary for a new entry
static CODE_INDEX last_core_entry;
static uint8_t last_function_index;

static uint8_t* compilation;
static uint8_t* jumps[6];
static uint8_t jp = 0;


static uint16_t find_word(char *);
static uint16_t find_dictionay_entry(char *);
//static void list_dictionary_entries(CODE_INDEX);
static uint32_t read(CODE_INDEX *);

void dictionary_init()
{
    core_functions = (CORE_FUNC *)malloc(sizeof(CORE_FUNC) * CORE_WORDS);
    dictionary_reset();
}
    
void dictionary_reset()
{
    last_entry_offset = 0xffff;
    new_entry_offset = 0;
    compilation = code;
    
    memset(code, 0, CODE_SIZE);   
}

CORE_FUNC dictionary_core_word_function(CODE_INDEX instruction) 
{
    return core_functions[instruction];
}

static uint32_t read(CODE_INDEX * offset) 
{
    uint8_t segment;
    uint32_t value = 0;
    uint8_t i = 0;
    
    while (true)
    {
        segment = code[(*offset)++];
        // printf("#    read @%04x => %02x\n", *offset - 1, segment);
        
        value =  (segment & 0x7F) << (i * 7)  | value ; // (value * 0x80) | (segment & 0x7F);
        if (segment < 0x80) {
            // printf("#        => %i/0x%04x\n", value, value);
            return value;
        }
        i++;
    }
}

uint32_t dictionary_read(struct Process *process) 
{
    return read(&(process->ip));
}

uint32_t dictionary_read_at(CODE_INDEX offset)
{
    return read(&offset);
}

/*
    Returns the address of the code for for the specified word.
 */
 // TODO RENAME to compiler_find_word())
CODE_INDEX dictionary_code_for(char * instruction)
{
    CODE_INDEX entry_index;
    CODE_INDEX current_index;
    char entry_name[32];    
    
    entry_index = last_entry_offset;
    while (entry_index != CODE_END) {
        current_index = entry_index;
//        if (trace) {
//            printf("# looking for '%s' at @%04x\n", instruction, current_index);
//        }
        entry_index = read(&current_index);
        uint8_t len = code[current_index++] & 0x1f;
        strncpy(entry_name, &(code[current_index]), len);
        entry_name[len] = 0;

//        if (trace) {
//            printf("#   checking '%s' at @%04x/@%04x\n", entry_name, current_index, entry_index);
//        }
//        
        if (strcmp(entry_name, instruction) == 0) {
            current_index += len;
            if (trace) {
                printf("# code found '%s' at ~%04x\n", entry_name, current_index);
            }
            // the next byte is the start of the code
            if (current_index < last_core_entry)
            {
                // use short code
                return code[current_index];
            }
            else 
            {
                return current_index;
            }
        }
    }
    return CODE_END;
}

bool dictionary_is_core_word(CODE_INDEX entry)
{
    return entry <= last_function_index;
}

void dictionary_append(uint32_t value)
{
    uint8_t segment;
    while (true) {
        segment = value % 0x80;
        if (value > segment) {
            segment = segment | 0x80;
            *compilation++ = segment;
            value = value / 0x80;
        } else {
            *compilation++ = segment;
            break;
        }
    }
}

uint16_t dictionary_index() 
{
    return compilation - code;
}

void dictionary_add_entry(char *name)
{
    int len = strlen(name);
    compilation = code + new_entry_offset;              
    if (trace) {
        printf("# adding entry for '%s' at @%04X\n", name, dictionary_index());
    }
    // previous entry
    uint16_t previous_entry;
    if (new_entry_offset == 0) {
        previous_entry = 0xffff;  // first entry; code to indicate no more previous
    } else {
        previous_entry = last_entry_offset;
    }
    if (trace) printf("#   previous_entry @%04x, %i\n", previous_entry, len & 0x1f);
    dictionary_append(previous_entry);
    dictionary_append(len & 0x1f);

    strcpy(compilation, name);
    compilation += len;
    if (trace) {
        printf("#   code at ~%04X (at %08X)\n", dictionary_index(), compilation);
    }
}

void dictionary_insert_internal_instruction(uint8_t entry, CORE_FUNC function)
{
    core_functions[entry] = function;
    last_function_index = entry > last_function_index ? entry : last_function_index;
}

void dictionary_add_core_word(char * name, CORE_FUNC function)
{
    printf("# core word %s %02X @%04x\n", name, last_function_index, dictionary_index());
    core_functions[last_function_index] = function;
    dictionary_add_entry(name);
    dictionary_append(last_function_index);
    last_function_index++;
 
    dictionary_end_entry();
    last_core_entry = new_entry_offset;
}


uint32_t dictionary_memory_address(uint16_t offset)
{
    return (uint32_t) code + offset;
}

/*
 Return the address of the dictionary entry for the code at the specified address
 */
void dictionary_find_word_for(CODE_INDEX instruction, char *name) {
    CODE_INDEX entry;
    CODE_INDEX next_entry;

    if (trace) {
        printf("# seeking name for instruction %04x | %04x\n", instruction, last_core_entry);
    }
   
    bool short_code = instruction <= last_core_entry;
    entry = last_entry_offset;
    strcpy(name, "Unknown!");        
    while (entry != CODE_END) {
        next_entry = read(&entry);
        if (short_code && entry >= last_core_entry)
        {
            entry = next_entry;
            continue;
        }
        
        uint8_t len = (code[entry++] & 0x1f);
//        if (trace) {
//            printf("# checking entry ~%04x %02x\n", entry + len, code[entry+ len]);
//        }
        if (short_code ? instruction == code[entry + len] : instruction == entry + len) {
//        if (instruction == entry + len) {
            strncpy(name, &(code[entry]), len);
            name[len] = 0;
            break;
        }

        entry = next_entry;
    }
}

void dictionary_end_entry() 
{
    last_entry_offset = new_entry_offset;
    new_entry_offset = compilation - code;
}

/*
 * List the entries in the dictionary
 */
void dictionary_words() {
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    char name[32];

	if (last_entry_offset == CODE_END) {
		printf("No entries\n");
	}
    
    entry = last_entry_offset;
    while (entry != CODE_END)
    {
        printf("@%04x ", entry);
        next_entry = (CODE_INDEX) read(&entry);
        uint8_t len = code[entry++] & 0x1f;
        if (trace) printf("[%04x + %i]  ", entry, len);

        strncpy(name, &(code[entry]), len);
        name[len] = 0;
        printf("%s [~%04x]\n", name, entry + len);
        entry = next_entry;
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

static void debug_print(CODE_INDEX addr, uint8_t length)
{
    printf("  %04X ", addr);
    int i;
    for (i = 0; i < length; i++) {
        printf("%02X", code[addr++]);
    }
    printf("  ");
}

void dictionary_debug_entry(CODE_INDEX offset)
{
    CODE_INDEX entry;
    CODE_INDEX next_entry;
    CODE_INDEX start_at, end_at;
    CODE_INDEX addr;
    char name[32];
    uint32_t value;
    
    if (trace) printf("# debug entry for @%04x\n", offset);
    
    entry = last_entry_offset;
    end_at = new_entry_offset;
    while (entry != CODE_END)
    {
        start_at = entry;
        next_entry = (CODE_INDEX) read(&entry);
        if (start_at <= offset) {
            uint8_t len = code[entry++] & 0x1f;
            strncpy(name, &(code[entry]), len);
            name[len] = 0;
            break;
        }
        
        end_at = start_at;
        entry = next_entry;
    }

    if (trace) printf("# Entry from %04x to %04x\n", start_at, end_at);
    printf("Word '%s' at %04X\n", name, start_at);
    entry = start_at;
    value = read(&entry);
    printf("  Next word at %04X\n", value);
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
            } else {
                dictionary_find_word_for(value, name);
                debug_print(addr, entry - addr);
                printf("func() (%08x) %s", value, name);
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