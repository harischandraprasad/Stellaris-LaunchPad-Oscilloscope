/*
 * adc.c
 *
 *  Created on: 20-Sep-2014
 *      Author: Harish
 */

#include "stdint.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/adc.h"
#include "driverlib/fpu.h"

#include "stdbool.h"

#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"


#include "status_led.h"

#include "comm/comm_core.h"
#include "comm/comm_usb.h"
#include "adc.h"

#define HW_OVERSAMPLE_FACTOR 8

void InitADC(void){
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    //GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3); // Not sure why this throws an exception with my Stellaris Launchpad.
	ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_ADC0);
	ROM_SysCtlADCSpeedSet(SYSCTL_ADCSPEED_1MSPS);
	ROM_ADCHardwareOversampleConfigure(ADC0_BASE, HW_OVERSAMPLE_FACTOR);
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
		ROM_ADCHardwareOversampleConfigure(ADC1_BASE, HW_OVERSAMPLE_FACTOR);
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
		TransmitData(0b10000000 | ((ulADC_0_Value >> 8) & 0x00FF), ulADC_0_Value & 0x00FF);
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
		TransmitData(0b01000000 | ((ulADC_0_Value >> 8) & 0x00FF), ulADC_1_Value & 0x00FF);
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
