/*
 * adc.h
 *
 *  Created on: 20-Sep-2014
 *      Author: Harish
 */

#ifndef ADC_H_
#define ADC_H_

extern void InitADC(void);


extern void ADC_0_S3_IntHandler(void);
extern void ADC_1_S3_IntHandler(void);
extern void ADC_0_S2_IntHandler(void);
extern void ADC_0_S1_IntHandler(void);
extern void ADC_0_S0_IntHandler(void);
extern void ADC_1_S2_IntHandler(void);
extern void ADC_1_S1_IntHandler(void);
extern void ADC_1_S0_IntHandler(void);

#endif /* ADC_H_ */
