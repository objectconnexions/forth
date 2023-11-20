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

#ifndef _FLASH_H    /* Guard against multiple inclusion */
#define _FLASH_H

#ifdef __cplusplus
extern "C" {
#endif



void flash_erase(void);

void flash_write_word(uint32_t, uint32_t);
    
#ifdef __cplusplus
}
#endif

#endif
