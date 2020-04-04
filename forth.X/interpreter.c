#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "forth.h"

void interpret_number(uint32_t value) 
{
    if (trace) printf("@ push 0x%04X\n", value);
    push(value);
}

void interpret_code(uint32_t code)
{
    if (trace) printf("@ execute 0x%04X\n", code);
    execute(code);
}