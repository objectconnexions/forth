    /******************************************************************************/
/*  Files to Include                                                          */
/******************************************************************************/
//#ifdef __XC32
//#include <xc.h>          /* Defines special funciton registers, CP0 regs  */
//#endif

#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <sys/attribs.h>
#include <stdint.h>         /* For uint32_t definition                        */
#include <stdbool.h>        /* For true/false definition                      */
#include <string.h>
#include <stdint.h>
#include "logger.h"
#include "system.h"         /* System funct/params, like osc/periph config    */
#include "user.h"           /* User funct/params, such as InitApp             */
#include "stdio.h"
#include "uart.h"
#include "timer.h"
#include "forth.h"

#ifdef MX130
//    #include <proc/p32mx130f064d.h>
#define PWR_LED PORTBbits.RB14
#elif MX270
#include <proc/p32mx270f256d.h>
#define PWR_LED PORTAbits.RA8
#elif MX570
#include <proc/p32mx570f512h.h>
#define PWR_LED PORTEbits.RE1
#endif

/******************************************************************************/
/* Global Variable Declaration                                                */
/******************************************************************************/

bool run_task = false;

/******************************************************************************/
/* Main Program                                                               */

/******************************************************************************/

int32_t main(void) 
{

#ifndef PIC32_STARTER_KIT
    /*The JTAG is on by default on POR.  A PIC32 Starter Kit uses the JTAG, but
    for other debug tool use, like ICD 3 and Real ICE, the JTAG should be off
    to free up the JTAG I/O */
    DDPCONbits.JTAGEN = 0;
#endif

    /*Refer to the C32 peripheral library documentation for more
    information on the SYTEMConfig function.
    
    This function sets the PB divider, the Flash Wait States, and the DRM
    /wait states to the optimum value.  It also enables the cacheability for
    the K0 segment.  It could has side effects of possibly alter the pre-fetch
    buffer and cache.  It sets the RAM wait states to 0.  Other than
    the SYS_FREQ, this takes these parameters.  The top 3 may be '|'ed
    together:
    
    SYS_CFG_WAIT_STATES (configures flash wait states from system clock)
    SYS_CFG_PB_BUS (configures the PB bus from the system clock)
    SYS_CFG_PCACHE (configif (ip != NULL) {
        log_info(LOG, "INT processing %I -> %Z", vector, ip);

        interrupt_process->ip = ip; 
        interrupt_process->next = current_process; 
        current_process = interrupt_process;
        current_process->activations++;

        log_info(LOG, " - swap to task %S", current_process->name);
        log_info(LOG, " - execute at %Z", current_process->ip);
        log_info(LOG, " - return to %Z", current_process->next);
    }ures the pCache if used)
    SYS_CFG_ALL (configures the flash wait states, PB bus, and pCache)*/

    /* TODO Add user clock/system configuration code if appropriate.  */
    SYSTEMConfig(SYS_FREQ, SYS_CFG_ALL);

    /* Initialize I/O and Peripherals for application */
    InitApp();

    /*Configure Multivector Interrupt Mode.  Using Single Vector Mode
    is expensive from a timing perspective, so most applications
    should probably not use a Single Vector Mode*/
    //INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    INTConfigureSystem(INT_SYSTEM_CONFIG_SINGLE_VECTOR);
    INTEnableInterrupts();


    // Power LED
#ifdef MX130
    ANSELBbits.ANSB14 = 0;
    TRISBbits.TRISB14 = 0;
//    ANSELBbits.ANSB15 = 0;
//    TRISBbits.TRISB15 = 0;
#elif MX270
    TRISAbits.TRISA8 = 0;
#elif MX570
    TRISEbits.TRISE1 = 0;
#endif

    PWR_LED = 1;

    uart_init();
    log_init();

    uart_transmit_buffer("PIC 32MX board\r\n");
//
//    // Timer2 Setup
//    T2CONbits.ON = 0; // disable before set up
//    PR2 = 48000; // divide by to get 1kHz, eg 1ms ticks
//    TMR2 = 0X0;
//    T2CONbits.TCKPS = 0; // prescale 1:1    
//    T2CONbits.TCS = 0;
//    T2CONbits.ON = 1; // start timer
//
//    // Timer1 Interrupt Setup
//    IPC2bits.T2IP = 5; // Main Priority
//    IPC2bits.T2IS = 0; // Sub-Priority
//    IFS0bits.T2IF = 0; // Clear flag
//    IEC0bits.T2IE = 1; // Enable Interrupt    

    // Core Timer Interrupt Setup
    _CP0_SET_COMPARE(CORE_TIMER_INTERVAL);
    _CP0_SET_COUNT(0);
    IPC0bits.CTIP = 7; // Main Priority
    IPC0bits.CTIS = 0; // Sub-Priority
   // _CP0_SET_COMPARE(_CP0_GET_COMPARE() + 40000000);
    IFS0bits.CTIF = 1; // Clear flag  ---  Set flag so timer starts and compare register is set up
    IEC0bits.CTIE = 1; // Enable Interrupt    

    
//    _CP0_SET_COMPARE(0);
    console_out("core timer interval %I - %I\n", CORE_TIMER_INTERVAL, _CP0_GET_COUNT());
    
    forth_init();
    forth_run();
}

void __ISR_SINGLE() interrupts(void)
{
    uint8_t vector = INTSTAT & 0x3F;    
    if (vector == _CORE_TIMER_VECTOR) 
    {
      // console_out("core timer %I\n", timer);
      timer++;
//       _CP0_GET_COUNT
       // TODO if the compare  register ever lags the counter then this interrupt will stop!
       _CP0_SET_COMPARE(_CP0_GET_COMPARE() + CORE_TIMER_INTERVAL);
       IFS0bits.CTIF = 0;
    }
    else if (vector == _UART_2_VECTOR)
    {
          Uart2Handler();
    } 
//    else if (vector == _TIMER_2_VECTOR) 
//    {
//        /*
//            Timer handler gets called every 1ms and increments the timer variable.
//            Getting the timer variable git the time in milliseconds since startup. 
//         */
//        timer++;
//        IFS0bits.T2IF = 0;
//    } 
    else
    {
//        uint8_t priority = (INTSTAT >> 8) & 0x7;
//        log_error("MAIN", "%X @ %I", vector, priority)
        forth_interrupt(vector);
    }
}
