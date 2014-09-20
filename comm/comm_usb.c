/*
 * usb_comm.c
 *
 *  Created on: 20-Sep-2014
 *      Author: Harish
 */

#include "stdbool.h"
#include "stdint.h"
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/adc.h"
#include "driverlib/fpu.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"


#include "comm_core.h"
#include "comm_usb.h"


void InitCommunication_USB(void){
	/* Initialize the USB */
}

void Transmit_USB(void) {
	//ROM_UARTCharPut(UART0_BASE, TXByte);
}
