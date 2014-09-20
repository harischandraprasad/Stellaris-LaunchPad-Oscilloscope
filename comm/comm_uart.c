/*
 * uart_comm.c
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
#include "comm_uart.h"


void InitCommunication_UART(void){
	/* Initialize the UART */
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	//Set the Rx/Tx pins as UART pins
	ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
	ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
	ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	//Configure the UART baud rate, data configuration
	const unsigned long baud = 128000;
	ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), baud, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);	// receiver interrupts (RX) and receiver timeout interrupts (RT)

	ROM_IntMasterEnable();
	ROM_IntEnable(INT_UART0);
}

void Transmit_UART(void) {
	ROM_UARTCharPut(UART0_BASE, g_ucTXByte);
}


void UARTIntHandler(void)
{
	unsigned long ulStatus;

	ulStatus = ROM_UARTIntStatus(UART0_BASE, true);	//get interrupt status
	ROM_UARTIntClear(UART0_BASE, ulStatus); 		//clear the asserted interrupts
	while(ROM_UARTCharsAvail(UART0_BASE)) 			//loop while there are chars
	{
		g_ucRXByte = ROM_UARTCharGet(UART0_BASE);
		Receive();
	}
}

