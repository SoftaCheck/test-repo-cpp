/* ========================================
 *
 * Copyright AIO Systems, 2018
 * Written by Novodes for AIO Systems
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF AIO Systems.
 *
 * ========================================
*/
/**
  ******************************************************************************
  * @file           : modbus_funcs.c
  * @brief          : This file contains functions related to the modbus implementation
										  with regards to the memory space of the modbus registers. Here there
										  are functions that implement the high level management commands and 
											communicating with the modbus memory space. 
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/

#include "modbus_driver.h"
#include "gpio.h"
#include "modbus_app_layer.h"
#include "GW_BRProtocolsImp.h"
#include "State_machine.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static uint16_t crcTable[] = {
	0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
	0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
	0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
	0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
	0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
	0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
	0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
	0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
	0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
	0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
	0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
	0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
	0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
	0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
	0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
	0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
	0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
	0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
	0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
	0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
	0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
	0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
	0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
	0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
	0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
	0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
	0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
	0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
	0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
	0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
	0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
	0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};
 
/* Private function prototypes -----------------------------------------------*/


/* Exported functions ---------------------------------------------------------*/


/***********************************************************

Function name: MB_Apply_ReadHoldingRegisters

Function type: void

Arguments: uint8_t * IncomingArr

Return: uint8_t - PASS or FAIL

Description: This function is responsible for dealing with
the whole prosess of FC = 3. This function is reading from
the flash is the area after 0x08006000 + 200regs and based 
on the address from the user. This function also send the 
respone over modbus to the master. The data from the flash 
is written to a dedicated array which can be use somewhere
else in this code.

(0x03) Read Holding Registers

**********************************************************/

uint8_t MB_Apply_ReadHoldingRegisters(uint8_t * IncomingArr)
{
	//uint32_t Adrs2Read = START_OF_NODES_MEM_AREA_ADRS + ((uint32_t)((uint16_t)((IncomingArr[ARR_LOC_2]<<BYTE_SHIFT) & 0xFF00) + (uint32_t)IncomingArr[ARR_LOC_3])); // CODE_REVIEW: This is WRONG! You are using the register number as the offset for the address location. Instead you should multiply this offset by the number of bytes allocated to each register which is 2.
	uint32_t register_num = ((uint32_t)(IncomingArr[REGISTER_ADDRESS_MSB]<<8) & 0x0000FF00) + (uint32_t)IncomingArr[REGISTER_ADDRESS_LSB];	// extract the register number
	
	uint32_t Adrs2Read = MODBUS_MEM_SPACE_START_ADDRESS + register_num*REGISTER_BYTE_NUM;
	
	//uint32_t Adrs2Read = START_OF_NODES_MEM_AREA_ADRS + register_num*REGISTER_BYTE_NUM; // Eyal Gerber: I fixed this because this was a critical bug
	uint16_t AmountOfBytes =  ((uint16_t)((IncomingArr[ARR_LOC_4]<<BYTE_SHIFT) & 0xFF00) + (uint32_t)IncomingArr[ARR_LOC_5])*2;		//Mult by two because the size is in MB regs, which are 16bit.
	uint16_t IncomingCRC = (IncomingArr[ARR_LOC_7]<< BYTE_SHIFT) + IncomingArr[ARR_LOC_6];
	uint16_t IncomingCRCCalc = CalcCRC(IncomingArr, MB_FC_3_SIZE-2);						//The -2 is to eliminate the CRC two bytes
	
	// CODE_REVIEW_done: There is no check of CRC. If CRC check fails then memory should NOT be read and the relevant response should be sent back in such case. 
	
	#ifdef ENABLE_MODBUS_LOGS
		printf("IncomingCRC = %x    IncomingCRCCalc = %x\n\r",IncomingCRC, IncomingCRCCalc);
	#endif
	if (IncomingCRCCalc != IncomingCRC)
	{
		#ifdef ENABLE_MODBUS_LOGS
			printf( "incorrect incoming CRC! the incoming is: %x and the calculated is: %x\n\r", IncomingCRC, IncomingCRCCalc );
		#endif
		return FAIL;
	}
	
	#ifdef ENABLE_MODBUS_LOGS	//CODE REVIEW: You are using ENABLE_MODBUS_LOGS in many cases that are not general. In these cases you should use a proper definition the represents the case such as MODBUS_LOGS for example. 
		printf("Reading %d holding register from physical address 0x%x , or register number %d\n\r", AmountOfBytes/2, Adrs2Read, register_num);
	#endif
	
	uint8_t MBResponse[MAX_MODBUS_SLAVE_COMMAND_SIZE] = {0};
	MBResponse[ARR_LOC_0] = IncomingArr[ARR_LOC_0];  // slave address
	MBResponse[ARR_LOC_1] = IncomingArr[ARR_LOC_1];  // function code
	MBResponse[ARR_LOC_2] = AmountOfBytes;  // byte count

	uint8_t DataFromFlash[MAX_MODBUS_SLAVE_COMMAND_SIZE] = {0};			//Init an array to store the data read from flash.
	
	EXTERNAL_FLASH_ReadData(Adrs2Read, DataFromFlash, AmountOfBytes);
	
	for(uint8_t i=0;i<20;i++)
	{
		#ifdef ENABLE_MODBUS_LOGS
			printf("The data from flash is: %x\n\r", DataFromFlash[i]);
		#endif
		MBResponse[ARR_LOC_3+i] = DataFromFlash[i];
	}

	 uint16_t ResponseCRC = CalcCRC(MBResponse, (MB_FC_3_RESPONSE_EXTRA_DATA-2) + AmountOfBytes);			//The -2 is to eliminate the CRC two bytes
	 #ifdef ENABLE_MODBUS_LOGS
		printf("Outgoing crc = 0x%x\n\r", ResponseCRC);
	 #endif

	MBResponse[(AmountOfBytes)+(MB_FC_3_RESPONSE_EXTRA_DATA-2)] = ResponseCRC & 0xFF;
	MBResponse[(AmountOfBytes)+(MB_FC_3_RESPONSE_EXTRA_DATA-1)] = (ResponseCRC >> BYTE_SHIFT) & 0xFF;
	UART1_Transmit(MBResponse, AmountOfBytes+MB_FC_3_RESPONSE_EXTRA_DATA, 200);
	
	return PASS;
}




/***********************************************************

Function name: CalcCRC

Function type: uint16_t

Arguments: uint8_t * buf, uint16_t size

Return: CRC

Description: This function is responsible for calculating
the CRC of the data in the buf array.

**********************************************************/
uint16_t CalcCRC(uint8_t * buf, uint16_t size)
{
	uint16_t crc = 65535;

	for( uint16_t i = 0; i < size; i++ )
	{
		byte tableIndex = (byte)(crc ^ buf[i]);
		crc >>= 8;
		crc ^= crcTable[tableIndex];
	}

	return ( crc );
	
}



/***********************************************************

Function name: MB_Apply_WriteSingleRegister

Function type: void

Arguments: uint8_t * IncomingArr

Return: uint8_t - PASS or FAIL

Description: This function is responsible for writing single 
MB register to the memory map of the GW. 

**********************************************************/

uint8_t MB_Apply_WriteSingleRegister(uint8_t * IncomingArr)
{
	uint32_t register_num = ((uint32_t)(IncomingArr[REGISTER_ADDRESS_MSB]<<8) & 0x0000FF00) + (uint32_t)IncomingArr[REGISTER_ADDRESS_LSB];	// extract the register number
	uint32_t add2Write = MODBUS_MEM_SPACE_START_ADDRESS + (register_num * REGISTER_BYTE_NUM); // Eyal Gerber: I fixed this because this was a critical bug
	
	#ifdef ENABLE_MODBUS_LOGS
		printf("The address to write is 0x%x and the register number is %d\n\r", add2Write, register_num);	//CODE REVIEW: Please add for these logs #ifdef for log control. Please make sure not to use general indications but rather something that is relevant to this case like MODBUS_LOGS for example
		printf("the base address is: 0x%x\n\r", START_OF_NODES_MEM_AREA_ADRS);
	#endif
	
	uint16_t IncomingCRC = (IncomingArr[ARR_LOC_7]<< BYTE_SHIFT) + IncomingArr[ARR_LOC_6];
	//uint16_t IncomingCRC =  ((IncomingArr[IncomingArr[6]+7] <<8) & 0xFF00) | (IncomingArr[IncomingArr[6]+8] & 0x00FF);
	
	uint16_t IncomingCRCCalc = CalcCRC(IncomingArr, 6);
	
	#ifdef ENABLE_MODBUS_LOGS
		printf("Incoming crc = 0x%x   Calculated CRC = 0x%x\n\r",IncomingCRC, IncomingCRCCalc);
	#endif
	
	
	if(IncomingCRCCalc != IncomingCRC)
	{
		#ifdef ENABLE_MODBUS_LOGS
			printf( "incorrect incoming CRC! the incoming is: %x and the calculated is: %x\n\r", IncomingCRC, IncomingCRCCalc );
		#endif
		return FAIL;
	}

	uint8_t data2Write[2] = {0};
	data2Write[0] = IncomingArr[DATA_ADDRESS_MSB];
	data2Write[1] = IncomingArr[DATA_ADDRESS_LSB];
	printf("data2Write[0] = %x\n\r", data2Write[0]);
	printf("data2Write[1] = %x\n\r", data2Write[1]);
	EXTERNAL_FLASH_WriteData_Automatic(add2Write, REGISTER_BYTE_NUM, data2Write);	// write the data to the memory space
	
	// send Modbus response
	UART1_Transmit(IncomingArr, RESPOSNE_LENGTH_SINGLE_REGISTER_WRITE, 100);		// the response to the command of single register write is exactly the same command received
	
	return PASS;
}
	


/***********************************************************

Function name: MB_Apply_WriteMultipleRegisters

Function type: void

Arguments: uint8_t * IncomingArr

Return: uint8_t - PASS or FAIL

Description: This function is responsible for writing meny 
MB registers to the memory map of the GW. In case of query
from the main PC, there's a bit different due to the size of
the query, that also beig written to the flash and hence 
the extra argument.

**********************************************************/
uint8_t MB_Apply_WriteMultipleRegisters(uint8_t * IncomingArr)
{
	uint8_t  i;
	//uint32_t size = (uint32_t)(((IncomingArr[4]<<8)&0xFF00) + (uint16_t)IncomingArr[5]);	//This size is in modbus registers - 16bit
	byte NumBytes2Write = IncomingArr[6];			//This is in bytes - size*2
//	SizeOfLastQuery = NumBytes2Write;
	
	uint32_t register_num = ((uint32_t)(IncomingArr[REGISTER_ADDRESS_MSB]<<8) & 0x0000FF00) + (uint32_t)IncomingArr[REGISTER_ADDRESS_LSB];	// extract the register number
	//uint32_t add2Write = START_OF_NODES_MEM_AREA_ADRS + register_num*REGISTER_BYTE_NUM; // Eyal Gerber: I fixed this because this was a critical bug
	
	uint32_t add2Write = MODBUS_MEM_SPACE_START_ADDRESS + register_num*REGISTER_BYTE_NUM;
		
	#ifdef ENABLE_MODBUS_LOGS
		printf("The base address is: %x\n\r", MODBUS_MEM_SPACE_START_ADDRESS);
		printf("The physical address which is written to is: 0x%x, which is register number %d\n\r", add2Write, register_num);
	#endif
	
	uint8_t data2write[MAX_BYTE_NUMBER_TO_WRITE] = {0};
	for( i = 0 ; i < NumBytes2Write ; i++ )
		data2write[i] = IncomingArr[i+7];		//Ignoring the other bytes before the actual data
	
	#ifdef ENABLE_MODBUS_LOGS
		for(uint8_t i=0;i<NumBytes2Write;i++)
			printf("The content of write to flash is: 0x%x\n\r", data2write[i]);
	#endif
	
	uint16_t IncomingCRC = (IncomingArr[NumBytes2Write+(MB_FC_10_EXTRA_DATA-1)]<< BYTE_SHIFT) + IncomingArr[NumBytes2Write+(MB_FC_10_EXTRA_DATA-2)];
	uint16_t IncomingCRCCalc = CalcCRC(IncomingArr, NumBytes2Write+(MB_FC_10_EXTRA_DATA-2));
	
	#ifdef ENABLE_MODBUS_LOGS
		printf("IncomingCRC = %x    IncomingCRCCalc = %x\n\r",IncomingCRC, IncomingCRCCalc);
	#endif
	if (IncomingCRCCalc != IncomingCRC)
	{
		#ifdef ENABLE_MODBUS_LOGS
			printf( "incorrect incoming CRC! the incoming is: %x and the calculated is: %x\n\r", IncomingCRC, IncomingCRCCalc );
		#endif
		return FAIL;
	}

	EXTERNAL_FLASH_WriteData_Automatic(add2Write, NumBytes2Write+2, data2write);
	
	// send Modbus response section:
	uint8_t WriteMultRegsResponse[RESPOSNE_LENGTH_MULTIPLE_REGISTER_WRITE] = {0};
	
	memcpy(WriteMultRegsResponse, IncomingArr, 6);	// first six bytes of the command received are also the first six bytes in the response
	uint16_t OutgoingCRCCalc = CalcCRC(WriteMultRegsResponse, 6);
	
	#ifdef ENABLE_MODBUS_LOGS
		printf( "Outgoing crc = 0x%x\n\r", OutgoingCRCCalc );
	#endif

	WriteMultRegsResponse[6] = OutgoingCRCCalc & 0xFF;
	WriteMultRegsResponse[7] = (OutgoingCRCCalc >> 8) & 0xFF;
	UART1_Transmit(WriteMultRegsResponse, RESPOSNE_LENGTH_MULTIPLE_REGISTER_WRITE, 100);
	
	return PASS;
}


/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/
