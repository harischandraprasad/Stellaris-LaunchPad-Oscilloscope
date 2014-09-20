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
 *			- Dual trace mode with both burst and continuous modes is implemented
 ******************************************************************************/


/*
 * main.c
 */


#include "stdint.h"
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
#define BURST_DATA_SIZE 16100 		// (~15.7 * KBytes)	// Memory Depth
uint16_t burstData[BURST_DATA_SIZE];
uint16_t burstDataCounter;

//These are OPTIONAL. These are just for debuging purpose that I can see that some thing is going on board.
#define LED_RED		2
#define LED_BLUE	4
#define LED_GREEN	8

#define LEDOn_Red() 	{ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);}
#define LEDOff_Red()	{ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, ~GPIO_PIN_1);}
#define LEDOn_Blue()	{ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);}
#define LEDOff_Blue()	{ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, ~GPIO_PIN_2);}


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

#define Transmit() {ROM_UARTCharPut(UART0_BASE, TXByte);}
volatile unsigned char TXByte;		// Value sent over UART when Transmit() is called
volatile unsigned char RXByte;		// Value recieved once hasRecieved is set

unsigned long int ulADC_0_Value;
unsigned long int ulADC_1_Value;
#define ADC_0_SEQUENCER	3
#define ADC_1_SEQUENCER 3

bool enableDualChannel = true;


void InitADC(){
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    //GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3); // Not sure why this throws an exception with my Stellaris Launchpad.
	ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_ADC0);
	ROM_SysCtlADCSpeedSet(SYSCTL_ADCSPEED_1MSPS);
	ROM_ADCHardwareOversampleConfigure(ADC0_BASE, 8);
	ROM_ADCReferenceSet(ADC0_BASE, ADC_REF_INT);

	ROM_ADCSequenceDisable(ADC0_BASE, ADC_0_SEQUENCER);
	ROM_ADCSequenceConfigure(ADC0_BASE, ADC_0_SEQUENCER, ADC_TRIGGER_PROCESSOR, 0);
	ROM_ADCSequenceStepConfigure(ADC0_BASE, ADC_0_SEQUENCER, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
	ROM_ADCSequenceEnable(ADC0_BASE, ADC_0_SEQUENCER);


	if(enableDualChannel){
		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
		//GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3); // Not sure why this throws an exception with my Stellaris Launchpad.
		ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_ADC1);
		ROM_SysCtlADCSpeedSet(SYSCTL_ADCSPEED_1MSPS);
		ROM_ADCHardwareOversampleConfigure(ADC1_BASE, 8);
		ROM_ADCReferenceSet(ADC1_BASE, ADC_REF_INT);

		ROM_ADCSequenceDisable(ADC1_BASE, ADC_1_SEQUENCER);
		ROM_ADCSequenceConfigure(ADC1_BASE, ADC_1_SEQUENCER, ADC_TRIGGER_PROCESSOR, 0);
		ROM_ADCSequenceStepConfigure(ADC1_BASE, ADC_1_SEQUENCER, 0, ADC_CTL_CH1 | ADC_CTL_IE | ADC_CTL_END);
		ROM_ADCSequenceEnable(ADC1_BASE, ADC_1_SEQUENCER);
	}


	ROM_ADCIntClear(ADC0_BASE, ADC_0_SEQUENCER);
	ROM_ADCIntEnable(ADC0_BASE, ADC_0_SEQUENCER);

	if(enableDualChannel){
		ROM_ADCIntClear(ADC1_BASE, ADC_1_SEQUENCER);
		ROM_ADCIntEnable(ADC1_BASE, ADC_1_SEQUENCER);
	}
}

void Send_BurstData(){
	uint16_t i;
	unsigned long int adcVal;

	LEDOn_Red();
	for (i = 0; i < BURST_DATA_SIZE; i++)
	{
		adcVal = burstData[i];
		TXByte = adcVal & 0x00FF;			// Set TXByte to the lower 8 bits
		Transmit();							// Send
		TXByte = (adcVal >> 8) & 0x00FF;	// Set TXByte to the upper 8 bits
		Transmit();							// Send

		// Speed is not an important but memeory, so use burstData[i] multiple times instead of storing into an extra variable
		/*
		TXByte = burstData[i] & 0x00FF;			// Set TXByte to the lower 8 bits
		Transmit();								// Send
		TXByte = (burstData[i] >> 8) & 0x00FF;	// Set TXByte to the upper 8 bits
		Transmit();								// Send
		*/
	}
	LEDOff_Red();
}

void Start_Stream(){
	sendStream = true;
	ROM_ADCSequenceEnable(ADC0_BASE, ADC_0_SEQUENCER);
	if(enableDualChannel)
		ROM_ADCSequenceEnable(ADC1_BASE, ADC_1_SEQUENCER);

	ROM_ADCIntClear(ADC0_BASE, ADC_0_SEQUENCER);
	if(enableDualChannel)
		ROM_ADCIntClear(ADC1_BASE, ADC_1_SEQUENCER);

	ROM_ADCIntEnable(ADC0_BASE, ADC_0_SEQUENCER);
	if(enableDualChannel)
		ROM_ADCIntEnable(ADC1_BASE, ADC_1_SEQUENCER);

	ROM_IntEnable(INT_ADC0SS3);	 // Turn on Sequence Interrupts for Sequence 3 for ADC0 module
	if(enableDualChannel)
		ROM_IntEnable(INT_ADC1SS3);	 // Turn on Sequence Interrupts for Sequence 3 for ADC1 module

	ROM_ADCProcessorTrigger(ADC0_BASE, ADC_0_SEQUENCER);
	if(enableDualChannel)
		ROM_ADCProcessorTrigger(ADC1_BASE, ADC_1_SEQUENCER);
	LEDOn_Red();
}

void Stop_Stream(){
	sendStream = false;
	ROM_ADCSequenceDisable(ADC0_BASE, ADC_0_SEQUENCER);
	if(enableDualChannel)
		ROM_ADCSequenceDisable(ADC1_BASE, ADC_1_SEQUENCER);

	ROM_IntDisable(INT_ADC0SS3);
	if(enableDualChannel)
		ROM_IntDisable(INT_ADC1SS3);
	LEDOff_Red();
}

/*
 * ADC_0 Sequence 3 interrupt handler.
 *
 * Check running mode
 * 	1. Continuous mode
 * 		Send ADC value immediately to requester(host application).
 * 	2. Burst mode
 * 		Store ADC value in memory. Once samples count reaches BURST_DATA_SIZE then send entire data to requester.
 */
void ADC_0_S3_IntHandler(void){
	ROM_ADCSequenceDisable(ADC0_BASE, ADC_0_SEQUENCER);
	ROM_ADCIntClear(ADC0_BASE, ADC_0_SEQUENCER);
	ROM_ADCSequenceDataGet(ADC0_BASE, ADC_0_SEQUENCER, &ulADC_0_Value);

	if(burstMode){
		burstData[burstDataCounter++] = 0x8000 | ulADC_0_Value;
		if(burstDataCounter >= BURST_DATA_SIZE){
			LEDOff_Blue();
			burstMode = false;
			Stop_Stream();
			Send_BurstData();
		}
	}else{
		TXByte = ulADC_0_Value & 0x00FF;						// Set TXByte
		Transmit();												// Send
		TXByte = 0b10000000 | ((ulADC_0_Value >> 8) & 0x00FF);	// Set TXByte to the upper 8 bits
		Transmit();
	}

	if(sendStream){
		ROM_ADCSequenceEnable(ADC0_BASE, ADC_0_SEQUENCER);
		ROM_ADCProcessorTrigger(ADC0_BASE, ADC_0_SEQUENCER);
	}
}

/*
 * ADC_1 Sequence 3 interrupt handler.
 *
 * Check running mode
 * 	1. Continuous mode
 * 		Send ADC value immediately to requester(host application).
 * 	2. Burst mode
 * 		Store ADC value in memory. Once samples count reaches BURST_DATA_SIZE then send entire data to requester.
 */
void ADC_1_S3_IntHandler(void){
	ROM_ADCSequenceDisable(ADC1_BASE, ADC_1_SEQUENCER);
	ROM_ADCIntClear(ADC1_BASE, ADC_1_SEQUENCER);
	ROM_ADCSequenceDataGet(ADC1_BASE, ADC_1_SEQUENCER, &ulADC_1_Value);

	if(burstMode){
		burstData[burstDataCounter++] = 0x4000 | ulADC_1_Value;
		if(burstDataCounter >= BURST_DATA_SIZE){
			LEDOff_Blue();
			burstMode = false;
			Stop_Stream();
			Send_BurstData();
		}
	}else{
		TXByte = ulADC_1_Value & 0x00FF;							// Set TXByte
		Transmit();													// Send
		TXByte = 0b01000000 | ((ulADC_0_Value >> 8) & 0x00FF);		// Set TXByte to the upper 8 bits
		Transmit();
	}

	if(sendStream){
		ROM_ADCSequenceEnable(ADC1_BASE, ADC_1_SEQUENCER);
		ROM_ADCProcessorTrigger(ADC1_BASE, ADC_1_SEQUENCER);
	}
}

void ADC_0_S2_IntHandler(void){return;}
void ADC_0_S1_IntHandler(void){return;}
void ADC_0_S0_IntHandler(void){return;}
void ADC_1_S2_IntHandler(void){return;}
void ADC_1_S1_IntHandler(void){return;}
void ADC_1_S0_IntHandler(void){return;}

/**
 * Handles the received byte and calls the needed functions.
 **/
void Receive()
{
	uint16_t i;

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

	ulStatus = ROM_UARTIntStatus(UART0_BASE, true);	//get interrupt status
	ROM_UARTIntClear(UART0_BASE, ulStatus); 		//clear the asserted interrupts
	while(ROM_UARTCharsAvail(UART0_BASE)) 			//loop while there are chars
	{
		RXByte = ROM_UARTCharGet(UART0_BASE);
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
	ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), baud, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	ROM_IntMasterEnable();
	ROM_IntEnable(INT_UART0);
	ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);	// receiver interrupts (RX) and receiver timeout interrupts (RT)

	InitADC();

	while (1){
		;
	}
}
