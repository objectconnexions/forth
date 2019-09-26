/* 
 * File:   forth.h
 * Author: rcm
 *
 * Created on 13 September 2019, 08:54
 */

#ifndef FORTH_H
#define	FORTH_H

#ifdef	__cplusplus
extern "C" {
#endif


int forth_init();

void forth_execute(uint8_t*);

void forth_tasks(uint32_t ticks);

#ifdef	__cplusplus
}
#endif

#endif	/* FORTH_H */

