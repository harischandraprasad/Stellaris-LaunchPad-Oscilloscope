/*
 * status_led.h
 *
 *  Created on: 20-Sep-2014
 *      Author: Harish
 */

#ifndef STATUS_LED_H_
#define STATUS_LED_H_

//These are OPTIONAL. These are just for debuging purpose that I can see that some thing is going on board.
#define LED_RED		2
#define LED_BLUE	4
#define LED_GREEN	8

#define LEDOn_Red() 	{ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);}
#define LEDOff_Red()	{ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, ~GPIO_PIN_1);}
#define LEDOn_Blue()	{ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);}
#define LEDOff_Blue()	{ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, ~GPIO_PIN_2);}


#endif /* STATUS_LED_H_ */
