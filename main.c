/******************************************************************************
 *                  Hari's Stellaris LaunchPad Oscilloscope
 *
 * Description:	This code turns the Stellaris Launchpad into a simple
 *				oscilloscope using a bi-directional software UART.
 * 				The MCU will accept ASCII commands from the computer for to
				measure external channels using internal ADC in two modes
					1. Continuous
					2. Burst

  				This was originally written by NJC for "NJC's MSP430 LaunchPad Blog" for MSP430 LaunchPad.
  				PS:
  					http://www.msp430launchpad.com/2010/12/njcs-launchscope-launchpad-oscilloscope.html
  					http://www.oscilloscope-lib.com/

  				I have written this as per my requirement with Stellaris LaunchPad.

 * Author:	Harischandra Prasad Tirumani
 * Email:
 * Date:	04-09-2014 23:51 - version - 1.0
 * 			- Initial implementation. Basic UART stuff is done.
 * Date:	10-09-2014 23:51 - version - 1.1
 *			- Basic stream start/stop implemented with sample values sent back to PC.
 * Date:	12-09-2014 00:06 - version - 1.2
 *			- Stream start/stop with ADC values implemented.
 * Date:	13-09-2014 00:15 - version - 1.3
 *			- Burst mode implemented.
 * Date:	18-09-2014 22:04
 *			- Dual trace mode started implementing
 * Date:	20-09-2014 01:11 - version - 1.4
 *			- Dual trace mode with both burst and continuous modes is implemented.
 * Date:	21-09-2014 01:03 - version - 1.5
 *			- Code refactoring, code splitted into different module files.
 *			- USB mode started implementing.
 ******************************************************************************/


/*
 * main.c
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

#include "comm/comm_core.h"
#include "status_led.h"
#include "adc.h"

int main(void){
	//
	// Enable lazy stacking for interrupt handlers. This allows floating-point
	// instructions to be used within interrupt handlers, but at the expense of
	// extra stack usage.
	//
	ROM_FPUEnable();
	ROM_FPULazyStackingEnable();

	// 50MHz
	//ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	// 80 MHz
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	// Initialize LED port as out
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

	InitCommunication();

	InitADC();

	while (1){
		;
	}
}
