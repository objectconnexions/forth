/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.c

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#include <p32xxxx.h>
#include <proc/p32mx270f256d.h>
#include "stdio.h"

#define _SUPPRESS_PLIB_WARNING 1
  
uint8_t usb_data[1024];
uint32_t buffer[8];
uint8_t *bdt;

static void debug() 
{
     uint32_t address = (uint32_t) &usb_data;
    int i;
    for (i = 0; i < 64; i++) {
        printf("%08x ", address + i * 16);
        int k;
        for (k = 0; k < 16; k++) {
            printf("%02x ", usb_data[i * 16 + k]);
        }
        printf("\n");
    }

}
    
void usb_init()   
{
        
    printf("USB\nU1PWR = %0x\n", U1PWRC);
    printf("U1CON = %0x\n", U1CON);
    
       
    // TODO replace with memset call
    int j;
    for (j = 0; j < 1024; j++) {
        usb_data[j] = 0;
    }

    uint32_t memory = (uint32_t) &usb_data;
    printf("Memory = %0x\n", memory);
    uint32_t boundary = (memory + 512) & 0xfffffe00;
    printf("Boundary = %0x\n", boundary);
    
    bdt = (uint8_t *) boundary;
    printf("BDT = %0x\n", bdt);
    
    U1PWRCSET = 1;  // turn on USB
    uint32_t address = (uint32_t)&bdt;
    U1BDTP1 = (address >> 8) & 0xff;
    U1BDTP2 = (address >> 16) & 0xff;
    U1BDTP3 = address >> 24;
    
    
    address = (uint32_t)&usb_data;

    uint32_t table_end = boundary + 512;
    
    uint16_t offset = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        if (address < table_end && address + 64 > boundary) 
        {
            address = table_end;
        }
        
        uint16_t row = i * 8;
        bdt[row + 4] = (address >> 24);
        bdt[row + 5] = (address >> 16) & 0xff;
        bdt[row + 6] = (address >> 8) & 0xff;
        bdt[row + 7] = address & 0xff;
        buffer[i] = address;
        printf("Buffer %i @ %0x\n", i, buffer[i]);

        address += 64;
    }

    for (i = 0; i < 8; i++)
    {
        uint16_t row = i * 8;
        bdt[row + 2] = 0x1;
    }
    debug();
    


    U1PWRCbits.USBPWR = 1;
    U1OTGCONbits.OTGEN = 0;
    U1CONbits.HOSTEN = 0;
    U1CONbits.USBEN = 1;    
 
    printf("init: %i, %x\n", U1IRbits.TRNIF, U1STAT);
}

void usb_debug_status() {
    printf("usb: %i, %x\n", U1IRbits.TRNIF, U1STAT);
}
    
    

/* *****************************************************************************
 End of File
 */
