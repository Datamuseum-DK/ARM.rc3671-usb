/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 *      Name:    cdcuser.c
 *      Purpose: USB Communication Device Class User module 
 *      Version: V1.10
 *----------------------------------------------------------------------------
*      This software is supplied "AS IS" without any warranties, express,
 *      implied or statutory, including but not limited to the implied
 *      warranties of fitness for purpose, satisfactory quality and
 *      noninfringement. Keil extends you a royalty-free right to reproduce
 *      and distribute executable files created using this software for use
 *      on NXP Semiconductors LPC microcontroller devices only. Nothing else 
 *      gives you the right to use this software.
 *
 * Copyright (c) 2009 Keil - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include "projectconfig.h"

#include "usb.h"
#include "usbhw.h"
#include "usbcfg.h"
#include "usbcore.h"
#include "cdc.h"
#include "cdcuser.h"

#include "phkfifo.h"
FIFO_TYPE(fifo, int8_t, int, -1, int8_t)

FIFO(rx_fifo, fifo, 64);
FIFO(tx_fifo, fifo, 64);

static int8_t tx_idle = 1;
static int8_t rx_idle = 1;

unsigned char BulkBufOut [64];            // Buffer to store USB OUT packet
unsigned char NotificationBuf [10];

CDC_LINE_CODING CDC_LineCoding  = {CFG_USBCDC_BAUDRATE, 0, 0, 8};
unsigned short  CDC_SerialState = 0x0000;
unsigned short  CDC_DepInEmpty  = 1;                   // Data IN EP is empty

/*----------------------------------------------------------------------------
  CDC Initialisation
  Initializes the data structures and serial port
  Parameters:   None 
  Return Value: None
 *---------------------------------------------------------------------------*/
void CDC_Init (void) {

//  ser_OpenPort ();
//  ser_InitPort (CDC_LineCoding.dwDTERate,
//                CDC_LineCoding.bDataBits, 
//                CDC_LineCoding.bParityType,
//                CDC_LineCoding.bCharFormat);

  CDC_DepInEmpty  = 1;
  CDC_SerialState = CDC_GetSerialState();

  tx_idle = 1;
}

/*----------------------------------------------------------------------------
  CDC SendEncapsulatedCommand Request Callback
  Called automatically on CDC SEND_ENCAPSULATED_COMMAND Request
  Parameters:   None                          (global SetupPacket and EP0Buf)
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SendEncapsulatedCommand (void) {

  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC GetEncapsulatedResponse Request Callback
  Called automatically on CDC Get_ENCAPSULATED_RESPONSE Request
  Parameters:   None                          (global SetupPacket and EP0Buf)
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetEncapsulatedResponse (void) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC SetCommFeature Request Callback
  Called automatically on CDC Set_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetCommFeature (unsigned short wFeatureSelector) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC GetCommFeature Request Callback
  Called automatically on CDC Get_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetCommFeature (unsigned short wFeatureSelector) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC ClearCommFeature Request Callback
  Called automatically on CDC CLEAR_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_ClearCommFeature (unsigned short wFeatureSelector) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC SetLineCoding Request Callback
  Called automatically on CDC SET_LINE_CODING Request
  Parameters:   none                    (global SetupPacket and EP0Buf)
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetLineCoding (void) {

  CDC_LineCoding.dwDTERate   =   (EP0Buf[0] <<  0)
                               | (EP0Buf[1] <<  8)
                               | (EP0Buf[2] << 16)
                               | (EP0Buf[3] << 24); 
  CDC_LineCoding.bCharFormat =  EP0Buf[4];
  CDC_LineCoding.bParityType =  EP0Buf[5];
  CDC_LineCoding.bDataBits   =  EP0Buf[6];

//  ser_ClosePort();
//  ser_OpenPort ();
//  ser_InitPort (CDC_LineCoding.dwDTERate,
//                CDC_LineCoding.bDataBits, 
//                CDC_LineCoding.bParityType,
//                CDC_LineCoding.bCharFormat);    
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC GetLineCoding Request Callback
  Called automatically on CDC GET_LINE_CODING Request
  Parameters:   None                         (global SetupPacket and EP0Buf)
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetLineCoding (void) {

  EP0Buf[0] = (CDC_LineCoding.dwDTERate >>  0) & 0xFF;
  EP0Buf[1] = (CDC_LineCoding.dwDTERate >>  8) & 0xFF;
  EP0Buf[2] = (CDC_LineCoding.dwDTERate >> 16) & 0xFF;
  EP0Buf[3] = (CDC_LineCoding.dwDTERate >> 24) & 0xFF;
  EP0Buf[4] =  CDC_LineCoding.bCharFormat;
  EP0Buf[5] =  CDC_LineCoding.bParityType;
  EP0Buf[6] =  CDC_LineCoding.bDataBits;

  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC SetControlLineState Request Callback
  Called automatically on CDC SET_CONTROL_LINE_STATE Request
  Parameters:   ControlSignalBitmap 
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetControlLineState (unsigned short wControlSignalBitmap) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC SendBreak Request Callback
  Called automatically on CDC Set_COMM_FATURE Request
  Parameters:   0xFFFF  start of Break 
                0x0000  stop  of Break
                0x####  Duration of Break
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SendBreak (unsigned short wDurationOfBreak) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC_BulkIn call on DataIn Request
  Parameters:   none
  Return Value: none
 *---------------------------------------------------------------------------*/

void
CDC_BulkIn(void)
{
	uint8_t buf[64];
	uint8_t i;

	for (i = 0; i < sizeof buf && !fifo_empty(&tx_fifo); i++)
		buf[i] = fifo_get(&tx_fifo);
	if (i > 0) {
		USB_WriteEP (CDC_DEP_IN, buf, i);
		tx_idle = 0;
	} else {
		tx_idle = 1;
	}
} 


/*----------------------------------------------------------------------------
  CDC_BulkOut call on DataOut Request
  Parameters:   none
  Return Value: none
 *---------------------------------------------------------------------------*/

void
CDC_BulkOut(void)
{
	int numBytesRead;
	uint8_t buf[64];
	uint8_t i;

	rx_idle = 0;
	numBytesRead = USB_ReadEP(CDC_DEP_OUT, buf);
	// XXX: We don't handle flow/overrun yet
	for (i = 0; i < numBytesRead; i++)
		fifo_put(&rx_fifo, buf[i]);
}


/*----------------------------------------------------------------------------
  Get the SERIAL_STATE as defined in usbcdc11.pdf, 6.3.5, Table 69.
  Parameters:   none
  Return Value: SerialState as defined in usbcdc11.pdf
 *---------------------------------------------------------------------------*/
unsigned short CDC_GetSerialState (void) {
//  unsigned short temp;

  CDC_SerialState = 0;
//  ser_LineState (&temp);
//
//  if (temp & 0x8000)  CDC_SerialState |= CDC_SERIAL_STATE_RX_CARRIER;
//  if (temp & 0x2000)  CDC_SerialState |= CDC_SERIAL_STATE_TX_CARRIER;
//  if (temp & 0x0010)  CDC_SerialState |= CDC_SERIAL_STATE_BREAK;
//  if (temp & 0x4000)  CDC_SerialState |= CDC_SERIAL_STATE_RING;
//  if (temp & 0x0008)  CDC_SerialState |= CDC_SERIAL_STATE_FRAMING;
//  if (temp & 0x0004)  CDC_SerialState |= CDC_SERIAL_STATE_PARITY;
//  if (temp & 0x0002)  CDC_SerialState |= CDC_SERIAL_STATE_OVERRUN;

  return (CDC_SerialState);
}


/*----------------------------------------------------------------------------
  Send the SERIAL_STATE notification as defined in usbcdc11.pdf, 6.3.5.
 *---------------------------------------------------------------------------*/
void CDC_NotificationIn (void) {

  NotificationBuf[0] = 0xA1;                           // bmRequestType
  NotificationBuf[1] = CDC_NOTIFICATION_SERIAL_STATE;  // bNotification (SERIAL_STATE)
  NotificationBuf[2] = 0x00;                           // wValue
  NotificationBuf[3] = 0x00;
  NotificationBuf[4] = 0x00;                           // wIndex (Interface #, LSB first)
  NotificationBuf[5] = 0x00;
  NotificationBuf[6] = 0x02;                           // wLength (Data length = 2 bytes, LSB first)
  NotificationBuf[7] = 0x00; 
  NotificationBuf[8] = (CDC_SerialState >>  0) & 0xFF; // UART State Bitmap (16bits, LSB first)
  NotificationBuf[9] = (CDC_SerialState >>  8) & 0xFF;

  USB_WriteEP (CDC_CEP_IN, &NotificationBuf[0], 10);   // send notification
}

/*----------------------------------------------------------------------------
  PHK's sane API for the CDC port...
 *---------------------------------------------------------------------------*/

int
CDC_getchar(void)
{

	if (fifo_empty(&rx_fifo)) 
		return (-1);
	return (fifo_get(&rx_fifo));
}

int
CDC_putchar(int8_t c)
{
	int retval = 0;

	while (fifo_full(&tx_fifo)) {
		// XXX: This is a hack
		if (++retval == 100000) {
			CDC_BulkIn();
			break;
		}
	}
	if (fifo_full(&tx_fifo))
		return (-1);
	retval = 0;
	__disable_irq();
	if (fifo_put(&tx_fifo, c) < 0)
		retval = -1;
	else if (tx_idle)
		CDC_BulkIn();
	__enable_irq();
	return (retval);
}
