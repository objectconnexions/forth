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
    

void compile_number(uint32_t);

void compile_code(uint32_t);


    
void compiler_init(void);

void compiler_reset(void);

bool compiler_compile(char*);

void compiler_dump(void);

uint16_t compiler_scratch(void);

void find_word_for(uint16_t, char*);

void compiler_words(void);

#ifdef __cplusplus
}
#endif

#endif
