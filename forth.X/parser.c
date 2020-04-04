#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "code.h"
#include "forth.h"
/*
#include "compiler.h"
*/

static bool compile = false;

static uint8_t code_for(char * instruction)
{
    int i;
    for (i = 0; i < WORD_COUNT; i++) {
        if (core_words[i].word != NULL && strcmp(instruction, core_words[i].word) == 0) {
            return i;
        }
    }
    return 0xff;
}

void parse(char* source)
{
    char * start;
    
    if (trace) {
        printf("# input: %s\n", source);
    }

	while (true) {
        start = source;
		while (*source == ' ' || *source == '\t') {
			source++;
			start = source;
		}
		if (*source == 0 /* || *source == '\n' */) {
			return;
		}
		while (*source != 0 && *source != ' ' && *source != '\t') {
			source++;
		}

		char instruction[64];
		int len = source - start;
		strncpy(instruction, start, len);
		instruction[len] = 0;
        to_upper(instruction, len);

		if (trace) {
			printf("# instruction [%s] %i\n", instruction, len);
		}

        uint8_t code = code_for(instruction);  // RENAME to find_word())
        if (code != 0xff) {
            if (compile) compile_code(code);
            else interpret_code(code);
            
        } else {
            if (trace) printf("# not word 0x%0X\n", code);
            
            bool is_number = true;
            int radix = 10;
            int i = 0;
            // TODO determine based number by looking for 0, 0B or 0X first, then confirm every other char is a digit
            if (instruction[i] == '-')
            {
                i++;
            }
            if (instruction[i] == '0')
            {
                i++;
                if (instruction[i] == 'X')
                {
                    radix = 16;
                    i++;
                }   
                else if (instruction[i] == 'B')
                {
                    radix = 2;
                    i++;
                }   
                else if (isdigit(instruction[i]))
                {
                    radix = 8;
                    i++;
                }   
                else if (strlen(instruction) > i)
                {
                    is_number = false;
                }
            }
            for (; i < strlen(instruction); ++i) {
                if (!isdigit(instruction[i]) && !(radix > 10 && instruction[i] < ('a' + radix - 10))) {
                    is_number = false;
                    break;
                }
            }

            if (is_number) {
                int64_t value = strtoll(instruction, NULL, 0);
                if (trace) {
                    printf("# literal = 0x%09llX\n", value);
                }
                if (compile) compile_number(value);
                else interpret_number(value);
            } else {
                printf("%s!\n", instruction);
                return;
            }
        }
    	source++;
    }
}    
