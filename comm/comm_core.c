/*
 * core_comm.c
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


#include "../status_led.h"
#include "comm_core.h"


unsigned char g_ucTXByte;		// Value sent over UART when Transmit() is called
unsigned char g_ucRXByte;		// Value recieved once hasRecieved is set
bool sendStream = false;
bool burstMode = false;
uint16_t burstData[BURST_DATA_SIZE];
uint16_t burstDataCounter;
unsigned long int ulADC_0_Value;
unsigned long int ulADC_1_Value;

bool enableDualChannel = true;



#ifdef USB_MODE
#include "comm_usb.h"
#else
#include "comm_uart.h"
#endif

void Transmit(void){
#ifdef USB_MODE
	Transmit_USB();
#else
	Transmit_UART();
#endif
}

void InitCommunication(void){
#ifdef USB_MODE
	InitCommunication_USB();
#else
	InitCommunication_UART();
#endif
}


/**
 * Handles the received byte and calls the needed functions.
 **/
void Receive()
{
	uint16_t i;

	switch(g_ucRXByte)			// Switch depending on command value received
	{
		case TEST_SPEED:
			LEDOn_Red();
			for (i = 0; i != 0x100; i++)	// Loop 256 times
			{
				TransmitData(0, i);
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
			TransmitData(0x0A, 0x8F);
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
	g_ucRXByte = 0;
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



void Send_BurstData(){
	uint16_t i;
	unsigned long int adcVal;

	LEDOn_Red();
	for (i = 0; i < BURST_DATA_SIZE; i++)
	{
		adcVal = burstData[i];
		TransmitData((adcVal >> 8) & 0x00FF, adcVal & 0x00FF);

		// Speed is not an important but memeory, so use burstData[i] multiple times instead of storing into an extra variable
		//TransmitData((burstData[i] >> 8) & 0x00FF, burstData[i] & 0x00FF);
	}
	LEDOff_Red();
}
