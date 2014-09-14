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
 ******************************************************************************/


/*
 * main.c
 */


#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/adc.h"
#include "driverlib/fpu.h"

#include "stdbool.h"

#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"

#define KBytes 1024
#define BURST_DATA_SIZE 6 * KBytes			// Memory Depth
unsigned int burstData[BURST_DATA_SIZE];
unsigned int burstDataCounter;

//These are OPTIONAL. These are just for debuging purpose that I can see that some thing is going on board.
#define LED_RED		2
#define LED_BLUE	4
#define LED_GREEN	8

#define LEDOn_Red() 	{GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);}
#define LEDOff_Red()	{GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, ~GPIO_PIN_1);}
#define LEDOn_Blue()	{GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);}
#define LEDOff_Blue()	{GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, ~GPIO_PIN_2);}


// ASCII values for the commands
#define		TEST_SPEED		0x31
#define		M_A3			0x32
#define		STREAM			0x33
#define		STOP			0x34
#define		M_TEMP			0x35
#define		M_VCC			0x36
#define		M_BURST			0x37

bool sendStream = false;
bool burstMode = false;

#define Transmit() {UARTCharPut(UART0_BASE, TXByte);}
unsigned char TXByte;		// Value sent over UART when Transmit() is called
unsigned char RXByte;		// Value recieved once hasRecieved is set

unsigned long ulADCValue;
#define ADC_SEQUENCER	3


void InitADC(){
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    //GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3); // Not sure why this throws an exception with my Stellaris Launchpad.
	SysCtlPeripheralReset(SYSCTL_PERIPH_ADC0);
	SysCtlADCSpeedSet(SYSCTL_ADCSPEED_1MSPS);
	ADCHardwareOversampleConfigure(ADC0_BASE, 8);
	ADCReferenceSet(ADC0_BASE, ADC_REF_INT);

	ADCSequenceDisable(ADC0_BASE, ADC_SEQUENCER);
	ADCSequenceConfigure(ADC0_BASE, ADC_SEQUENCER, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQUENCER, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
	ADCSequenceEnable(ADC0_BASE, ADC_SEQUENCER);

	ADCIntClear(ADC0_BASE, ADC_SEQUENCER);
	ADCIntEnable(ADC0_BASE, ADC_SEQUENCER);
}

void Send_BurstData(){
	unsigned int i;

	unsigned long adcVal;
	LEDOn_Red();
	for (i = 0; i < BURST_DATA_SIZE; i++)
	{
		adcVal = burstData[i];
		TXByte = adcVal & 0x00FF;			// Set TXByte to the lower 8 bits
		Transmit();							// Send
		TXByte = (adcVal >> 8) && 0x00FF;	// Set TXByte to the upper 8 bits
		Transmit();							// Send
	}
	LEDOff_Red();
}

void Start_Stream(){
	sendStream = true;
	ADCSequenceEnable(ADC0_BASE, ADC_SEQUENCER);

	ADCIntClear(ADC0_BASE, ADC_SEQUENCER);
	ADCIntEnable(ADC0_BASE, ADC_SEQUENCER);

	IntEnable(INT_ADC0SS3);	 // Turn on Sequence Interrupts for Sequence 3

	ADCProcessorTrigger(ADC0_BASE, ADC_SEQUENCER);
	LEDOn_Red();
}

void Stop_Stream(){
	sendStream = false;
	ADCSequenceDisable(ADC0_BASE, ADC_SEQUENCER);

	IntDisable(INT_ADC0SS3);
	LEDOff_Red();
}

/*
 * ADC Sequence 0 interrupt handler.
 *
 * Check running mode
 * 	1. Continuous mode
 * 		Send ADC value immediately to requester(host application).
 * 	2. Burst mode
 * 		Store ADC value in memory. Once samples count reaches BURST_DATA_SIZE then send entire data to requester.
 */
void ADC_S0_IntHandler(void){
	ADCSequenceDisable(ADC0_BASE, ADC_SEQUENCER);
	ADCIntClear(ADC0_BASE, ADC_SEQUENCER);
	ADCSequenceDataGet(ADC0_BASE, ADC_SEQUENCER, &ulADCValue);

	if(burstMode){
		burstData[burstDataCounter++] = ulADCValue;
		if(burstDataCounter >= BURST_DATA_SIZE){
			LEDOff_Blue();
			burstMode = false;
			Stop_Stream();
			Send_BurstData();
		}
	}else{
		TXByte = ulADCValue & 0x00FF;			// Set TXByte
		Transmit();								// Send
		TXByte = (ulADCValue >> 8) && 0x00FF;	// Set TXByte to the upper 8 bits
		Transmit();
	}

	if(sendStream){
		ADCSequenceEnable(ADC0_BASE, ADC_SEQUENCER);
		ADCProcessorTrigger(ADC0_BASE, ADC_SEQUENCER);
	}
}


/**
 * Handles the received byte and calls the needed functions.
 **/
void Receive()
{
	unsigned int i;

	switch(RXByte)			// Switch depending on command value received
	{
		case TEST_SPEED:
			LEDOn_Red();
			for (i = 0; i != 0x100; i++)	// Loop 256 times
			{
				TXByte = i;
				Transmit();
				TXByte = 0;					// Need to send 16-Bit value as host expetcs 16-bit value. So, send the counter as if it were a 16 bit value
				Transmit();
			}
			LEDOff_Red();
			break;

		case STREAM:
			Start_Stream();					// Send ADC value continuously to host
			break;

		case STOP:
			Stop_Stream();					// Stops the ADC
			break;

		case M_A3:
			//NOT-IMPLEMENTED				// Reads ADC only once
			break;

		case M_TEMP:
			//NOT-IMPLEMENTED;				// Reads the temperature sensor once
			break;

		case M_VCC:
			// Reads VCC once. Until somebody finds a way to read accurate VCC(instead of just using another ADC input channel), pass hardcoded value
			// Make sure that this should end up as 3.3v on Oscilloscope application
			TXByte = 0x8F;
			Transmit();
			TXByte = 0x0A;
			Transmit();
			break;

		case M_BURST:
			burstDataCounter = 0;
			burstMode = true;
			LEDOn_Blue();
			Start_Stream();
			break;

		default:
			break;
    }
	RXByte = 0;
}


void UARTIntHandler(void)
{
	unsigned long ulStatus;

	ulStatus = UARTIntStatus(UART0_BASE, true);	//get interrupt status
	UARTIntClear(UART0_BASE, ulStatus); 		//clear the asserted interrupts
	while(UARTCharsAvail(UART0_BASE)) 			//loop while there are chars
	{
		RXByte = UARTCharGet(UART0_BASE);
		Receive();
	}
}


int main(void){
	//
	// Enable lazy stacking for interrupt handlers. This allows floating-point
	// instructions to be used within interrupt handlers, but at the expense of
	// extra stack usage.
	//
	ROM_FPUEnable();
	ROM_FPULazyStackingEnable();

	// 50MHz
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	// 80 MHz
	//ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	// Initialize LED port as out
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

	/* Initialize the UART */
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	//Set the Rx/Tx pins as UART pins
	ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
	ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
	ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	//Configure the UART baud rate, data configuration
	const unsigned long baud = 128000;
	ROM_UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), baud, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	ROM_IntMasterEnable();
	ROM_IntEnable(INT_UART0);
	ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);	// receiver interrupts (RX) and receiver timeout interrupts (RT)

	InitADC();

	while (1){
		;
	}
}
