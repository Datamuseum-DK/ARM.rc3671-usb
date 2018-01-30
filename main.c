/**************************************************************************/
/*! 
    @file     main.c
    @author   K. Townsend (microBuilder.eu)

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2011, microBuilder SARL
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "projectconfig.h"
#include "sysinit.h"

#include "core/uart/uart.h"
#include "core/gpio/gpio.h"
#include "core/systick/systick.h"

#ifdef CFG_INTERFACE
  #include "core/cmd/cmd.h"
#endif

#include "core/usbcdc/cdcuser.h"

/*
 * Pin  Signal  Direc   GPIO    LED     Color       Comment  
    A   -D12    in      1.11            hvid/blå*
    B   -D11    in      1.10            blå/brun*
    C   -D0     in      1.0             grå/pink*
    D   -D1     in      1.1             grå/gul*
    K   -D2     in      1.2             hvid/gul*
    L   -D3     in      1.3             gul/rød*
    M   -D4     in      1.4             gul/pink*
    N   -D5     in      1.5             pink/rød*
    U   -D6     in      1.6             brun/grå*
    V   -D7     in      1.7             hvid/grå*
    Y   -D8     in      1.8             brun/pink*
    Z   -D9     in      1.9             hvid/grå*
    AA  -IM     in      3.3     LED     rød/rød*    (int- char rdy)
    BB  -RDY    in      3.0     LED     blå/blå*
    CC  -GND    in      GND             sort*
    HH  -ERR    in      2.6     LED     grøn/grøn
    JJ  -HCK    in      2.5     LED     gul/gul     (int- EOF)
    KK  -MCK    in      2.4     LED     grå/grå
    LL  -PC     out     3.1     LED     pink/pink*
    MM  -BSY    in      3.2     LED     brun/brun*  (int+ card read)
        ACT     out     2.7     LED 
 
 * Commands
    R   Reset
    o   pick one
    P   continous pick on (until error)
    p   continous pick off
    s   print status + buffer

 * Output
    0   (default = RAW)
    1   RAW HHH HHH HHH HHH HHH HHH ... HHH CR LF
    2   BIN BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
    3   RCB [NNNN][HEADSTMT]CCCCCCCC[TAILSTMT] CR LF 
    4   EBCDIC  CCCCCCCCCCCCCCCCCCCCCCCC CR LF
    5   ASCII   CCCCCCCCCCCCCCCCCCCCCCCC CR LF
 
*/

#define ROWS 4
#define MAXPOS 82
int data[ROWS][MAXPOS];
int curpos[ROWS];
volatile int currrowrd = 0;
volatile int currrowwr = 0;
volatile int currpos = 0;
volatile int overrun = 0;
int outfmt = 0;
volatile int cardsread = 0;
volatile int multipick = 0;

void Reset(void) 
{
    currrowrd=0;
    currrowwr=0;
    currpos=0;
    overrun=0;
    cardsread=0;
    multipick=0;
    gpioSetValue(3,1,1);
}

void PIOINT2_IRQHandler(void)
{
    if (gpioIntStatus(2, 5)) //HOCK
    {
        gpioSetValue(3,1,1);
        multipick = 0;
        gpioIntClear(2, 5);
    }
}


void
PIOINT3_IRQHandler(void)
{
    if (gpioIntStatus(3, 3)) // IM-
    {
      int * row = data[currrowwr];
      if (curpos[currrowwr] < MAXPOS)
        row[curpos[currrowwr]++] = ~GPIO_GPIO1DATA;
      gpioIntClear(3, 3);
    }

    if (gpioIntStatus(3, 2)) // BSY+
    {
         if (!multipick)
             gpioSetValue(3,1,1);

         if (currrowwr == (currrowrd - 1) % ROWS)
         {
             overrun = 1;
         }
         else
         {
             currrowwr = (currrowwr+1) % ROWS;
             curpos[currrowwr] = 0;
         }
         gpioIntClear(3, 2);
         cardsread++;
    }
 }

int GetData(int * card)
{
    int rc = 0;
    gpioSetValue(2,7,1);    
    if (overrun)
        rc = -1;
    else
        if (currrowwr != currrowrd)
        {
            int len = curpos[currrowrd];
            int i ;
            for (i=0; i<len; i++)
              *card++ = data[currrowrd][i];
            currrowrd = (currrowrd+1) % ROWS;
            rc = len;
        }
    gpioSetValue(2,7,0);    
    return rc;
}

void puthexdigit(unsigned int val)
{
    val &= 0xF;
    if (val > 9) 
        val+= 'A'-'9'-1;
    CDC_putchar('0'+val);
}

void put2hexspace(unsigned int val)
{
    puthexdigit(val/16);
    puthexdigit(val);
    CDC_putchar(' ');
}

void put3hexspace(unsigned int val)
{
    puthexdigit(val/256);
    put2hexspace(val % 256);
}


void put4hexspace(unsigned int val)
{
    puthexdigit(val/(256*16));
    put3hexspace(val%(256*16));
}

void putCRNL()
{
    CDC_putchar(13);
    CDC_putchar(10);
}

void putstring(char * s)
{
     while (*s)
         CDC_putchar(*s++);
}

void putstringint(char * str, int val)
{
    char tmp[10];
    char *pos = tmp;
    putstring(str);
    CDC_putchar('=');

    if (val)
    {
        while (val)
        {
            *pos++ = (val % 10)+'0';
            val = val / 10; 
        }
        do
        {
            CDC_putchar(*--pos);
        } while (tmp != pos);
    }
    else
        CDC_putchar('0');
    CDC_putchar(',');
    CDC_putchar(' ');
}

static int cardswritten=0;
void OutFmtData(int *data, int len)
{
    int i;
    switch (outfmt)
    {
        case 0:
        case 1: // RAW
                putstring("DATA: ");
                put4hexspace(cardswritten++);
                for (i=0; i<len; i++)
                    put3hexspace(*data++);
                putCRNL();
                break;
        case 2: // BIN 8-12 bit binary
                break;
        case 3: // RCB / RC BATCH cards
                break;
        case 4: // EBCDIC original 
                break;
        case 5: // ASCII 
                break;
    }
}


static int lcnt=0;
void OutStatus()
{
        putstring("CTRL: ");
    put4hexspace(lcnt++);
        putstringint("multipick",multipick);
        putstringint("cardread",cardsread);
        putstringint("outfmt",outfmt);
        putstringint("overrun",overrun);
        putstringint("HoCk",gpioGetValue(2,5));
        putstringint("MoCk",gpioGetValue(2,4));
        putstringint("Error",gpioGetValue(2,6));
        putstringint("Ready",gpioGetValue(3,0));
        putstringint("Pick",gpioGetValue(3,1));
        putstring("Data "); put3hexspace( ~GPIO_GPIO1DATA);
    putCRNL();
}

static void prt(void)
{
    int i, j;
    int  actmode=0;
    long cnt=0;

    printf("\r\nUSB-RC3671 v20180117cb\r\n");

    // All GPIO's input per default
    
    //data
    for (i = 0; i < 12; i++)
        gpioSetDir(1, i, gpioDirection_Input);

    //control
    gpioSetDir(3, 3, gpioDirection_Input);
    gpioSetDir(3, 2, gpioDirection_Input);
    gpioSetDir(3, 1, gpioDirection_Output);
    gpioSetValue(3, 1, 1);
    gpioSetDir(3, 0, gpioDirection_Input);
    gpioSetDir(2, 7, gpioDirection_Output);
    gpioSetDir(2, 6, gpioDirection_Input);
    gpioSetDir(2, 5, gpioDirection_Input);
    gpioSetDir(2, 4, gpioDirection_Input);
    
    IOCON_JTAG_TMS_PIO1_0 = 0
        | IOCON_JTAG_TMS_PIO1_0_FUNC_GPIO
        | IOCON_JTAG_TMS_PIO1_0_ADMODE_DIGITAL
        | IOCON_JTAG_TMS_PIO1_0_MODE_PULLUP;

    IOCON_JTAG_TDO_PIO1_1 = 0
        | IOCON_JTAG_TDO_PIO1_1_FUNC_GPIO
        | IOCON_JTAG_TDO_PIO1_1_ADMODE_DIGITAL
        | IOCON_JTAG_TDO_PIO1_1_MODE_PULLUP;

    IOCON_JTAG_nTRST_PIO1_2 = 0
        | IOCON_JTAG_nTRST_PIO1_2_FUNC_GPIO
        | IOCON_JTAG_nTRST_PIO1_2_ADMODE_DIGITAL
        | IOCON_JTAG_nTRST_PIO1_2_MODE_PULLUP;

    IOCON_SWDIO_PIO1_3 = 0
        | IOCON_SWDIO_PIO1_3_FUNC_GPIO
        | IOCON_SWDIO_PIO1_3_ADMODE_DIGITAL
        | IOCON_SWDIO_PIO1_3_MODE_PULLUP;

    //interrupt
    gpioSetInterrupt(3, 3, gpioInterruptSense_Edge, gpioInterruptEdge_Single, gpioInterruptEvent_ActiveLow);
    gpioSetInterrupt(3, 2, gpioInterruptSense_Edge, gpioInterruptEdge_Single, gpioInterruptEvent_ActiveHigh);
    gpioSetInterrupt(2, 5, gpioInterruptSense_Edge, gpioInterruptEdge_Single, gpioInterruptEvent_ActiveLow);
    gpioIntEnable(3, 3);
    gpioIntEnable(3, 2);
    gpioIntEnable(2, 5);
    NVIC_EnableIRQ(EINT2_IRQn);
    NVIC_EnableIRQ(EINT3_IRQn);

    //code
    multipick = 0;
    while (1) 
    {
        j = CDC_getchar();
            if (j)
            {
                    switch (j)
                    {   
                        case 'R':   Reset();cardswritten=0;break;
                        case 'o':   gpioSetValue(3,1,0); cardsread = 0; multipick = 0; break;
                        case 'P':   gpioSetValue(3,1,0); cardsread = 0; multipick = 1; break;
                        case 'p':   multipick = 0;  break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':   outfmt = j-'0';  break;
                            case 's':   
                        case 'a':   actmode = (actmode + 1) % 3;break;
                        case '?':   OutStatus(); break;
                    }
            }   

            int len;
            int card[MAXPOS];

            while (0 < (len = GetData(card)))
            {
                    OutFmtData(card, len);
            }   
        if (actmode < 2)
            gpioSetValue(2,7,actmode);
        else

        {
            if (cnt % 100000 == 0)
                OutStatus();
            gpioSetValue(2,7,cnt++ / 100000 % 2);
        }

    }
}

/**************************************************************************/
/*! 
    Main program entry point.  After reset, normal code execution will
    begin here.
*/
/**************************************************************************/
int
main(void)
{
    // Configure cpu and mandatory peripherals
    systemInit();

    prt();

    return 0;
}
