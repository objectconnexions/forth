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

void flash_write_word_to(uint32_t, uint32_t);
    

void flash_prepare_buffer(uint32_t);
void flash_buffer_add_byte(uint8_t);
void flash_buffer_add_cell(uint32_t);
void flash_write_buffer();
void flash_flush_buffer();



#ifdef __cplusplus
}
#endif

#endif
