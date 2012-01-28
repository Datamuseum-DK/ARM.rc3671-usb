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

#define NPIN 17

/*
 * Pin connection:
 *
 *	DB25	GPIO	Dir	Name
 *	1	1.1	in	-Strobe
 *	2	2.0	in	Data0
 *	3	2.1	in	Data1
 *	4	2.2	in	Data2
 *	5	2.3	in	Data3
 *	6	2.4	in	Data4
 *	7	2.5	in	Data5
 *	8	2.6	in	Data6
 *	9	2.7	in	Data7
 *	10	0.11	out	-Ack
 *	11	0.10	out	Busy
 *	12	0.9	out	PaperEnd
 *	13	0.8	out	SelectStatus
 *	14	3.0	in	AutoFeed
 *	15	3.1	out	-Error
 *	16	3.2	in	-Initialize
 *	17	3.3	in	-Select
 *	18-25	-	-	GND
 */

static int8_t got_prn;

void
PIOINT1_IRQHandler(void)
{
	int i;

	i = gpioIntStatus(1, 1);
	if (i) {
		gpioSetValue(0,10,1); 
		got_prn = GPIO_GPIO2DATA;
		gpioIntClear(1, 1);
	}
}


static void
prt(void)
{
	int i, j;
	int t;
	char s[NPIN+10], ls[NPIN+10];

	printf("\r\nHello World\r\n");

	gpioSetDir(1, 1, gpioDirection_Input);
	for (i = 0; i < 8; i++)
		gpioSetDir(2, i, gpioDirection_Input);
	gpioSetDir(0, 8, gpioDirection_Output);
	gpioSetDir(0, 9, gpioDirection_Output);
	gpioSetDir(0, 10, gpioDirection_Output);

	IOCON_JTAG_TCK_PIO0_10 = 0
	    | IOCON_JTAG_TDI_PIO0_11_FUNC_GPIO
	    | IOCON_JTAG_TDI_PIO0_11_ADMODE_DIGITAL;

	IOCON_JTAG_TDI_PIO0_11 = 0
	    | IOCON_JTAG_TDI_PIO0_11_FUNC_GPIO
	    | IOCON_JTAG_TDI_PIO0_11_ADMODE_DIGITAL;

	gpioSetDir(0, 11, gpioDirection_Output);
	gpioSetDir(3, 0, gpioDirection_Input);
	gpioSetDir(3, 1, gpioDirection_Output);
	gpioSetDir(3, 2, gpioDirection_Input);
	gpioSetDir(3, 3, gpioDirection_Input);

	gpioSetInterrupt(1,                  // Port
	    1,                               // Pin
	    gpioInterruptSense_Edge,         // Edge/Level Sensitive
	    gpioInterruptEdge_Single,        // Single/Double Edge
	    gpioInterruptEvent_ActiveLow);   // Rising/Falling
	gpioIntEnable(1, 1);

	gpioSetValue(0,11,1);
	gpioSetValue(0,10,0);
	gpioSetValue(0,9,0);

	t = 0;
	*ls = '\0';
	while (1) {
		if (systickGetSecondsActive() != t) {
			t = systickGetSecondsActive();
			*ls = '\0';
		}
		j = CDC_getchar();
		switch (j) {
		case 'a':	gpioSetValue(0,8,0); break;
		case 'A':	gpioSetValue(0,8,1); break;
		case 'b':	gpioSetValue(0,9,0); break;
		case 'B':	gpioSetValue(0,9,1); break;
		case 'c':	gpioSetValue(0,10,0); break;
		case 'C':	gpioSetValue(0,10,1); break;
		case 'd':	gpioSetValue(0,11,0); break;
		case 'D':	gpioSetValue(0,11,1); break;
		case 'e':	gpioSetValue(3,1,0); break;
		case 'E':	gpioSetValue(3,1,1); break;
		case '?':
			j = 0;
			s[j++] = '0' + gpioGetValue(1, 1);
			s[j++] = ' ';
			for (i = 0; i < 8; i++)
				s[j++] = '0' + gpioGetValue(2, i);
			s[j++] = ' ';
			for (i = 0; i < 4; i++)
				s[j++] = '0' + gpioGetValue(0, 8 + i);
			s[j++] = ' ';
			for (i = 0; i < 4; i++)
				s[j++] = '0' + gpioGetValue(3, i);
			
			s[j++] = '\0';
			printf("%s %x", s, got_prn);
			puts("\r\n");
			break;
		}

		if (gpioGetValue(0, 10)) {
			CDC_putchar(got_prn);
			for (j = 0; j < 10; j++)
				gpioSetValue(0,11,0);
			gpioSetValue(0,11,1);
			gpioSetValue(0,10,0);
			continue;
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

  uint32_t currentSecond, lastSecond;
  currentSecond = lastSecond = 0;
  
  // uartInit(115200);
  prt();

  while (1)
  {
    // Toggle LED once per second
    currentSecond = systickGetSecondsActive();
    if (currentSecond != lastSecond)
    {
      lastSecond = currentSecond;
      gpioSetValue(CFG_LED_PORT, CFG_LED_PIN, !(gpioGetValue(CFG_LED_PORT, CFG_LED_PIN)));
      uartSendByte('*');
    }

    // Poll for CLI input if CFG_INTERFACE is enabled in projectconfig.h
    #ifdef CFG_INTERFACE 
      cmdPoll(); 
    #endif
  }

  return 0;
}
