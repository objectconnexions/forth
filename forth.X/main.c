/******************************************************************************/
/*  Files to Include                                                          */
/******************************************************************************/
#ifdef __XC32
    #include <xc.h>          /* Defines special funciton registers, CP0 regs  */
#endif

#define _SUPPRESS_PLIB_WARNING 1 

#include <p32xxxx.h>
#include <sys/attribs.h>
#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <stdint.h>         /* For uint32_t definition                        */
#include <stdbool.h>        /* For true/false definition                      */
#include <string.h>
#include "system.h"         /* System funct/params, like osc/periph config    */
#include "user.h"           /* User funct/params, such as InitApp             */
#include "stdio.h"
#include "uart.h"
#include "forth.h"

/******************************************************************************/
/* Global Variable Declaration                                                */
/******************************************************************************/

/* i.e. uint32_t <variable_name>; */

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

    /* TODO <INSERT USER APPLICATION CODE HERE> */

    // Power LED
    // ANSELAbits.ANSA0 = 0;
    TRISAbits.TRISA8 = 0;
    PORTAbits.RA8 = 1;

    // ERROR LED
    
    ANSELBbits.ANSB1 = 0;
    TRISBbits.TRISB1 = 0;
    PORTBbits.RB1 = 0;

    // COMMS LED
    TRISCbits.TRISC4 = 0;
    PORTCbits.RC4 = 0;
    
    // IO#1
    ANSELCbits.ANSC0 = 0;
    TRISCbits.TRISC0 = 1;
    CNPUCbits.CNPUC0 = 1;
    

    char buf[1024];       // declare receive buffer with max size 1024    
 
    uart_init();
    uart_transmit("PIC 32MX board\r\n");
    
    U1PWRCbits.USBPWR = 0; // reset USB
    U1OTGCONbits.OTGEN = 0; // not OTG
    U1CONbits.HOSTEN = 0; // device
    U1CONbits.USBEN = 1; // active
    
    U1PWRCbits.USBPWR = 1; 
    
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
    
    while(1)
    {
        
        PORTBbits.RB1 = U1OTGSTATbits.SESVD;

        uart_receive();

        int length = uart_input_length();
        if (length > 0) {
            uart_read_input(buf);
            
            if (strcmp(buf, "usb") == 0) {
                sprintf(buf, "OTG = %08x CON = %08x PWRC = %08x\n", U1OTGCON, U1CON, U1PWRC);   
                uart_transmit(buf);    
            } else if (strncmp(buf, "led ", 4) == 0) {
                if (strcmp(buf + 4, "on") == 0) {
                    PORTCbits.RC4 = 1;
                    uart_transmit("led on\n");
                } else if (strcmp(buf + 4, "off") == 0) {
                    PORTCbits.RC4 = 0;
                    uart_transmit("led off\n");
                }
                
            } else if (strcmp(buf, "timer") == 0) {
                sprintf(buf, "timer = %04x\n", TMR1);   
                uart_transmit(buf);    
                
                
                /*
            } else if (strncmp(buf, "&", 1) == 0) {
                int address = 0;
                sscanf(buf + 1, "%x", &address);
                int *ptr = (int *)address;
                sprintf(buf, "0x%08x = 0x%08x\n", address, *ptr);
                uart_transmit(buf);
                uart_transmit("\n");
                /*
            } else if (strncmp(buf, "@", 1) == 0) {
                int address = 0;
                sscanf(buf + 1, "%x", &address);
                int *ptr = (int *)address;
                sprintf(buf, "0x%08x = 0x%08x\n", address, *ptr);
                uart_transmit(buf);
                uart_transmit("\n");
                 * 
                 * 
                 */
            } else {                
                forth_execute(buf);
    /*            uart_transmit("unknown command ");
                uart_transmit(buf);
                uart_transmit("\n");

      */
            }
         //   uart_transmit("OK> ");

             
        }
       /*
        uart_serial_receive(buf, 1024);     // wait here until data is received
        
        uart_serial_transmit("recv'd ");
        uart_serial_transmit(buf);  
        uart_serial_transmit("\r\n");
         */
        
        
        /*
        if(IFS0bits.T1IF)
        {
            IFS0bits.T1IF = 0;            // Put breakpoint here and single step past it.  Is the T1IF bit still set?
            PORTCbits.RC4 = ! PORTCbits.RC4;
        }
           */

	// TODO change to display the bits of int
        if (run_task) { 
            run_task = false;
      //      forth_tasks(timer);
          /*
            if (timer % 5000 == 0) {            
                char buf[40];
                sprintf(buf, "timer tick %i\n", timer);
                uart_transmit(buf);
            }
           * */
            if (timer % 100 == 0) {            
                PORTAbits.RA8 = ! PORTAbits.RA8;
            }
        }

        
        forth_run();

    }
}

/*
Timer handle gets called every 1ms and increments the timer variable.
Getting the timer variable git the time in milliseconds since startup. 
 */
void __ISR(_TIMER_1_VECTOR, IPL5SOFT) Timer1Handler(void)
{
    timer++;
    run_task = true;
    IFS0bits.T1IF = 0; 
}
