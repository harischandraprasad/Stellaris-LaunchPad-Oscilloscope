/*
 * comm_core.h
 *
 *  Created on: 20-Sep-2014
 *      Author: Harish
 */

#ifndef COMM_CORE_H_
#define COMM_CORE_H_


//#define USB_MODE true


#ifdef USB_MODE
#include "comm_usb.h"
#else
#include "comm_uart.h"
#endif

// ASCII values for the commands
#define		TEST_SPEED		0x31
#define		M_A3			0x32
#define		STREAM			0x33
#define		STOP			0x34
#define		M_TEMP			0x35
#define		M_VCC			0x36
#define		M_BURST			0x37



extern unsigned char g_ucTXByte;		// Value sent over UART when Transmit() is called
extern unsigned char g_ucRXByte;		// Value recieved once hasRecieved is set
extern bool sendStream;
extern bool burstMode;

#define KBytes 1024
#define BURST_DATA_SIZE 16100 		// (~15.7 * KBytes)	// Memory Depth
extern uint16_t burstData[BURST_DATA_SIZE];
extern uint16_t burstDataCounter;

#define ADC_0_SEQUENCER	3
#define ADC_1_SEQUENCER 3
extern unsigned long int ulADC_0_Value;
extern unsigned long int ulADC_1_Value;

extern bool enableDualChannel;


extern void Receive(void);
extern void Transmit(void);
extern void InitCommunication(void);


extern void Start_Stream();
extern void Stop_Stream();
extern void Send_BurstData();


// Need to send 16-Bit value as host expetcs 16-bit value. So, send the counter as if it were a 16 bit value
#define TransmitData(msb, lsb) { g_ucTXByte = lsb; Transmit(); g_ucTXByte = msb; Transmit(); }



#endif /* COMM_CORE_H_ */
