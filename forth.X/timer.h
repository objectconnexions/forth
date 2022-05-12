/* 
 * File:   timer.h
 * Author: rcm
 *
 */

#ifndef TIMER_H
#define	TIMER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SYSCLK 48000000L
#define CORE_TIMER_INTERVAL (SYSCLK / 2 / 1000)


// #include "dictionary.h"

extern volatile uint32_t timer;

#ifdef	__cplusplus
}
#endif

#endif	/* TIMER_H */

