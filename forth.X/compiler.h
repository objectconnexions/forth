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


void compiler_init(void);

void compiler_reset(void);

bool compiler_compile(char*);

void compiler_dump(void);

uint16_t compiler_scratch(void);



#ifdef __cplusplus
}
#endif

#endif
