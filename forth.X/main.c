/******************************************************************************/
/*  Files to Include                                                          */
/******************************************************************************/
#ifdef __XC32
    #include <xc.h>          /* Defines special funciton registers, CP0 regs  */
#endif

// #include <p32xxxx.h>
#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <sys/attribs.h>
#include <stdint.h>         /* For uint32_t definition                        */
#include <stdbool.h>        /* For true/false definition                      */
#include <string.h>
#include "logger.h"
#include "system.h"         /* System funct/params, like osc/periph config    */
#include "user.h"           /* User funct/params, such as InitApp             */
#include "stdio.h"
#include "uart.h"
#include "forth.h"

#ifdef MX270
#include <proc/p32mx270f256d.h>
#define PWR_LED PORTAbits.RA8
#endif

#ifdef MX570
#include <proc/p32mx570f512h.h>
#define PWR_LED PORTEbits.RE1
#endif

bool debug = false;

extern uint32_t timer;

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
    SYS_CFG_PCACHE (configures the pCache if used)
    SYS_CFG_ALL (configures the flash wait states, PB bus, and pCache)*/

    /* TODO Add user clock/system configuration code if appropriate.  */
    SYSTEMConfig(SYS_FREQ, SYS_CFG_ALL); 

    /* Initialize I/O and Peripherals for application */
    InitApp();

    /*Configure Multivector Interrupt Mode.  Using Single Vector Mode
    is expensive from a timing perspective, so most applications
    should probably not use a Single Vector Mode*/
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    INTEnableInterrupts();

    
    // Power LED
#ifdef MX270
    TRISAbits.TRISA8 = 0;
#endif

#ifdef MX570
    TRISEbits.TRISE1 = 0;
#endif

    log_init();
    
    PWR_LED = 1;

    uart_init();
    uart_transmit_buffer("PIC 32MX board\r\n");
    log_info("MAIN", "starting up");
        
    // Timer1 Setup
    T1CONbits.ON = 0;       // disable before set up
    PR1 = 1250;           // divide by to get 1kHz, eg 1ms ticks
    TMR1 = 0X0;
    T1CONbits.TCKPS = 2;    // prescale 1:64    
    T1CONbits.TCS = 0;
    T1CONbits.ON = 1;   // start timer
    
    // Timer1 Interrupt Setup
    IPC1bits.T1IP = 5; // Main Priority
    IPC1bits.T1IS = 0; // Sub-Priority
    IFS0bits.T1IF = 0; // Clear flag
    IEC0bits.T1IE = 1; // Enable Interrupt    
    
    forth_init();
    forth_run();
}

/*
    Timer handle gets called every 1ms and increments the timer variable.
    Getting the timer variable git the time in milliseconds since startup. 
 */
void __ISR(_TIMER_1_VECTOR, IPL5SOFT) Timer1Handler(void)
{
    timer++;
    IFS0bits.T1IF = 0; 
}
