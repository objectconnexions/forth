/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef _COMPILER_H    /* Guard against multiple inclusion */
#define _COMPILER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h> 
#include "dictionary.h"

//#define FUNCTION 1 

void compiler_compile_new();

bool compiler_compile(char*);

//void compiler_compile_core_word(char *, void (*)(void));

// void compiler_dump(void);

//uint16_t compiler_scratch(void);

//void compiler_words(void);

#ifdef __cplusplus
}
#endif

#endif
