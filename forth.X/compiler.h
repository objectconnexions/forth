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

void compiler_compile_definition(void);

void compiler_end(void);

void compiler_if(void);

void compiler_constant(void);

void compiler_2constant(void);

void compiler_variable(void);

void compiler_2variable(void);

void compiler_begin(void);

void compiler_then(void);

void compiler_else(void);

void compiler_again(void);

void compiler_until(void);

void compiler_eol_comment(void);

void compiler_inline_comment(void);

void compiler_print_comment(void);

void compiler_char(void);

void compiler_compile_string(void);

void compiler_print_string(void);

void compiler_s_string(void);

void compiler_c_string(void);

void compiler_create_data(void);

#ifdef __cplusplus
}
#endif

#endif
