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
  * @file           : BIT.c
  * @brief          : This file contains functions related to the BIT of data.
                      
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "BIT.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
 
/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/



/***********************************************************

Function name: BIT_Tests

Function type: uint8_t

Arguments: None

Return: uint8_t val2Ret

Description: This function is responsible for running
all of the functions of the BIT and give a final outcome

**********************************************************/
uint8_t BIT_Tests(void)
{
	uint8_t val2Ret = 0;
	
	val2Ret = BIT_SPI_FlashTest();
	if(val2Ret != BIT_FAIL)
	{
		val2Ret = BIT_ChecksumOfNodesData();
	}
	
	if(val2Ret == BIT_PASS)
		printf("\n\r\n\rBIT PASS\n\r\n\r");
	else if(val2Ret == BIT_FAIL)
		printf("\n\r\n\rBIT FAIL\n\r\n\r");
	return val2Ret;
}


/***********************************************************

Function name: BIT_SPI_FlashTest

Function type: uint8_t

Arguments: None

Return: uint8_t val2Ret

Description: This function is responsible for checking the SPI
channel of the external flash.

**********************************************************/
uint8_t BIT_SPI_FlashTest(void)
{
	uint8_t val2Ret = 0;
	uint8_t manufacturerIdPtr;
	uint8_t memTypePtr;
	uint8_t memDensityPtr;
	
	EXTERNAL_FLASH_Init();

	if(EXTERNAL_FLASH_ReadDeviceID(&manufacturerIdPtr,&memTypePtr,&memDensityPtr) == PASS)
	{
		val2Ret = BIT_PASS;
		#ifdef GENERAL_INDICATIONS
		printf("BIT: SPI test - PASS\n\r");
		#endif
	}
	else
	{
		val2Ret = BIT_FAIL;
		#ifdef GENERAL_INDICATIONS
		printf("BIT: SPI test - FAIL\n\r");
		#endif
	}
		
	return val2Ret;
}


/***********************************************************

Function name: BIT_VoltageTest

Function type: uint8_t

Arguments: None

Return: uint8_t val2Ret

Description: This function is responsible for checking the SPI
channel of the external flash.

**********************************************************/
uint8_t BIT_VoltageTest(void)
{
	uint8_t val2Ret = 0;
	
	return val2Ret;
}


/***********************************************************

Function name: BIT_ChecksumOfGW_Data

Function type: uint8_t

Arguments: None

Return: uint8_t val2Ret

Description: This function is responsible for checking the SPI
channel of the external flash.

**********************************************************/
uint8_t BIT_ChecksumOfGW_Data(void)
{
	uint8_t val2Ret = 0;
	
	
	return val2Ret;
}



/***********************************************************

Function name: BIT_ChecksumOfNodesData

Function type: uint8_t

Arguments: None

Return: uint8_t val2Ret

Description: This function is responsible for checking the SPI
channel of the external flash.

**********************************************************/
uint8_t BIT_ChecksumOfNodesData(void)
{
	uint8_t val2Ret = 0;
	
	for(uint8_t i=0;i<1/*50*/;i++)
	{
		uint8_t NodeParmasChecksumArr[18] = {0};
		uint8_t ChecksumStored[2] = {0};
		
		uint32_t adrs2ReadData = GW_MODBUS_REGISTERS_START_ADRS + 8/*i*/ * REGISTERS_PER_NODE*REGISTER_BYTE_NUM + NUMBER_OF_REGS_FOR_GW*REGISTER_BYTE_NUM;
		EXTERNAL_FLASH_ReadData(adrs2ReadData, NodeParmasChecksumArr, 18);
		uint32_t adrs2ReadChecksum = adrs2ReadData +CHECKSUM_OF_NODE_PARAM_LOCATION_WITHIN_NODE_MEM_SPACE*REGISTER_BYTE_NUM;
		
		EXTERNAL_FLASH_ReadData(adrs2ReadChecksum, ChecksumStored, 2);
		
		
		uint16_t ChecksunCalc = 0;
		ChecksunCalc = CalcCRC(NodeParmasChecksumArr, 18);
		
		uint16_t readChecksum = 0;
		readChecksum = (uint16_t)(((ChecksumStored[1]  & 0x00FF) << 8) | ChecksumStored[0]);
		
		if(readChecksum != ChecksunCalc)
		{
				val2Ret = BIT_FAIL;
			printf("BIT: Checksum test - FAIL\n\r");
		}
		else
		{
			printf("BIT: Checksum test - PASS\n\r");
			val2Ret = BIT_PASS;
		}
	}
	
	return val2Ret;
}





/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/
