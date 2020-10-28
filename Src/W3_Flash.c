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
  * @file           : W3_Flash.c
  * @brief          : This file contains functions related to the communication with
											the internal flash of the MCU.
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"
#include "stm32l0xx_hal_uart.h"

#include <stdio.h>
#include <string.h>
#include "usart.h"
#include <stdint.h>
#include "W3_Flash.h"
#include "modbus_driver.h"
#include "modbus_impl.h"
#include "eeprom.h"
#include "GW_BRProtocolsImp.h"
#include "State_machine.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
 
uint32_t FirstPage = 0, NbOfPages = 0, Address = 0;
uint32_t PageError = 0;
__IO uint32_t data32 = 0, MemoryProgramStatus = 0;

static FLASH_EraseInitTypeDef EraseInitStruct;

uint32_t pageBuffer[128], * pPageBuffer;


 
/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/

/**
  * @brief  
  * @param  None
  * @retval None
  */
//int flashEraseTable(void)
//{

//	/* Erase the user Flash area
//	  (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

//	/* Get the number of sector to erase from 1st sector*/
//	NbOfPages = (FLASH_USER_END_ADDR - FLASH_USER_START_ADDR + 1) >> 7;

//	/* Fill EraseInit structure*/
//	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
//	EraseInitStruct.PageAddress = FLASH_USER_START_ADDR;
//	EraseInitStruct.NbPages = NbOfPages;

//	if ( HAL_FLASHEx_Erase( &EraseInitStruct, &PageError ) != HAL_OK )
//	{
//		/* 
//		  Error occurred while page erase. 
//		  User can add here some code to deal with this error. 
//		  PageError will contain the faulty page.
//		*/
//		printf( "PageError %x\r\n", PageError );
//		Error_Handler( );
//	}
//	return ( 0 );
//}




/***********************************************************

Function name: FlashPageEraseAndWrite

Function type: void

Arguments: uint32_t Address, uint32_t *Data, uint32_t size

Return: none

Description: This function is responsible for writing to 
flash the new data from the user.
This function deal with the technical side of the flash
page erase and write.
This function is called by function code 06 of the 
Modbus and by function code 16

**********************************************************/

//void FlashPageEraseAndWrite(uint32_t Address, uint32_t *Data, uint32_t size)
//{
//	/*Size is the number of modbus commands to write. Since in this function we write only 16bit of data in each
//	word of 32bit, each page can contain up to 32 modbus commands*/
//	uint32_t NbOfPages2Write;
//	uint32_t WordsInPage = 0x20;
//	if(size > WordsInPage )
//	{
//		if(size%WordsInPage == 0)
//		{
//			NbOfPages2Write = size/WordsInPage;
//		}
//		else
//		{
//			NbOfPages2Write = (size/WordsInPage)+1;
//		}
//	}
//	else
//	{
//		NbOfPages2Write = 1;
//	}
//	
//	uint32_t  *pageAddr;
//	uint32_t offset_w;
//	
//	/* Get the number of sector to erase from 1st sector*/
//	NbOfPages = (Address - FLASH_BASE) / FLASH_PAGE_SIZE;
//	//printf( "The number of page is: 0x%x\n\r", NbOfPages);

//	/* Fill EraseInit structure*/
//	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
//	EraseInitStruct.PageAddress = NbOfPages * FLASH_PAGE_SIZE + FLASH_BASE;
//	pageAddr = (uint32_t *)EraseInitStruct.PageAddress;
//	EraseInitStruct.NbPages = NbOfPages2Write;		//The number of pages to erase.
//	
//	/*Copy the content of the desire pages before erasing*/
//	printf( "The address of the first page to write is is: 0x%x\n\r", EraseInitStruct.PageAddress);
//	offset_w = (Address - (EraseInitStruct.PageAddress))/4;	//Offset is the address of the firts word of the new data to write. We divide by 4 due to the fact that each word made out of 4bytes
//	//printf("The value of offset is: %x\n\r", offset_w);
//	printf("The size of message to write to flash is: %d\n\r", size);
//	printf("The address of flash to write is: %x\n\r", Address);
//			
//	
//	HAL_FLASH_Unlock();
//	
//	/*Erase the desire pages*/
//	if ( HAL_FLASHEx_Erase( &EraseInitStruct, &PageError ) != HAL_OK )
//	{
//		Error_Handler( );
//	}
//	printf("After Erase\n\r");
//	
//	uint8_t DataIndex = 0;	
//	
//			
//	uint32_t NewData2Write[WordsInPage*NbOfPages2Write];		//The array for the mixed old and new data to be write to flash

//	for(uint32_t i=0x0;i<WordsInPage*NbOfPages2Write;i++)		//Assign the data to NewData2Write array
//	{
//		if((i >= offset_w) && (i <  (offset_w+size)))
//		{
//			if(size == 1)		//If funcrion code 06 of Modbus
//				NewData2Write[i] = (Data[DataIndex]<<16) & 0xFFFF0000;
//			else		//If function code 16 of Modbus
//				NewData2Write[i] = (Data[DataIndex]<<24 & 0xFF000000) | (Data[DataIndex+1]<<16 & 0x00FF0000);
//			DataIndex = DataIndex +2;
//		}
//		else
//			NewData2Write[i] = pageBuffer[i];
//	}
//	
//	
//	/*Program the flash with the NewData2Write array*/
//	for(uint32_t i=0x0;i<WordsInPage*NbOfPages2Write;i++)
//	{
//		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, EraseInitStruct.PageAddress, NewData2Write[i]) == HAL_OK)
//		{
//			//HAL_Delay(5);
//			EraseInitStruct.PageAddress = EraseInitStruct.PageAddress + 4;
//		}
//		else
//		{ 
//			Error_Handler();
//		}
//	}
//	
//	HAL_FLASH_Lock(); 
//	printf("Finish\n\r");
//	state = RF_RECEIVE_STATE;
//}

//void writeFlashRegTable(uint32_t Address, uint32_t data, uint16_t size)
//{
//	for( int i = 0; i < size; i++ )
//	{
//		if ( HAL_FLASH_Program( FLASH_TYPEPROGRAM_WORD, Address, *(uint32_t *)data ) == HAL_OK )
//		{

//			Address += 4;
//			data += 4;
//			HAL_Delay( 5 );
//		}
//		else
//		{
//			/* Error occurred while writing data in Flash memory. 
//			   User can add here some code to deal with this error */
//			/*
//			  FLASH_ErrorTypeDef errorcode = HAL_FLASH_GetError();
//			*/
//			Error_Handler( );
//		}
//	}
//	/* Lock the Flash to disable the flash control register access (recommended
//	   to protect the FLASH memory against possible unwanted operation) *********/

//	return;

//}


//#if 0
//void flashTest(void)
//{

//  /* Unlock the Flash to enable the flash control register access *************/ 
//  HAL_FLASH_Unlock();

//  /* Erase the user Flash area
//    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

//   /* Get the number of sector to erase from 1st sector*/
//  NbOfPages = (FLASH_USER_END_ADDR - FLASH_USER_START_ADDR + 1) >> 7;

//  /* Fill EraseInit structure*/
//  EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
//  EraseInitStruct.PageAddress = FLASH_USER_START_ADDR;
//  EraseInitStruct.NbPages = NbOfPages;
//  
//  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
//  { 
//    /* 
//      Error occurred while page erase. 
//      User can add here some code to deal with this error. 
//      PageError will contain the faulty page.
//    */
//    Error_Handler();
//  }

//  /* Program the user Flash area word by word
//    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

//	//Address = REMOTE_UNIT_REG_ADRRESS;
//	Address  = 0x08006200;
//	printf("The address to write to is: %x", REMOTE_UNIT_REG_ADRRESS);
//	printf("\n\r");
//	uint32_t chnagingNumber1 = 0;
//	uint32_t daa[1] = {0};
//  
//	uint16_t i;
//	for(i=0;i<2000;i++)
//	{
//		daa[0] = chnagingNumber1++;
//    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address,daa[0]) == HAL_OK)
//    {
//      HAL_Delay(5);
//      Address = Address + 4;
//    }
//    else
//    { 
//      /* Error occurred while writing data in Flash memory. 
//         User can add here some code to deal with this error */
//      /*
//        FLASH_ErrorTypeDef errorcode = HAL_FLASH_GetError();
//      */
//      Error_Handler();
//    }
//	}
//		//printf("The value from flash is: %x\n\r", readFromFlashTable(FLASH_USER_START_ADDR, 1));

//  /* Lock the Flash to disable the flash control register access (recommended
//     to protect the FLASH memory against possible unwanted operation) *********/
//  HAL_FLASH_Lock(); 
//	return;
//}


/***********************************************************

Function name: ReadFromFlash

Function type: void

Arguments: uint32_t Address, uint8_t RegCount ,uint8_t * data

Return: none

Description: This function is responsible for reading data from
flash based on the address argument and the regcount argument
from the user. The data read is stored in the poiter to an array
of uint8_t. This function is used by the modbus commands of 
reading registers.

**********************************************************/
//void ReadFromFlash(uint32_t Address, uint8_t RegCount ,uint8_t * data)
//{
//	printf("\n\r\n\rReading from Flash at address %x\n\r", Address);
//	if(RegCount<2)
//	{
//		int i;
//		for(i=0;i<RegCount;i++)
//		{		
//			data[i] = *(uint32_t *)Address;
//			Address += 1;
//		}
//	}
//	else
//	{
//		for(uint8_t i=0;i<(RegCount);i+=2)
//		{
//					data[i+1] = *(uint32_t *)Address;
//					data[i] = *(uint32_t *)(Address+1);
//			Address += 4;
//		}
//	}
//	
//}



/***********************************************************

Function name: ManageArrOfFlags

Function type: void

Arguments: uint8_t NodeID

Return: none

Description: This function is responsible for set ir unset
the correct flag for each bridge connected to the GW.
This function should store this arr in the flash and read 
it after power up or reset.

This function is NOT ready yet!!!!!!

**********************************************************/
/*uint8_t ManageArrOfFlags(uint8_t NodeID, uint8_t action)
{
	uint8_t val2ret = 0;
	uint32_t tempArr[2] = {0};
	uint32_t arrOfFlags[2] = {0};
	for(uint8_t i=0;i<4;i++)
	{
		arrOfFlags[0] = (arrOfFlags[0]<<i*8) | EEPROM_READ_BYTE(BRIDGES_FLAG_START+i);
	}
	
	for(uint8_t i=0;i<4;i++)
	{
		arrOfFlags[1] = (arrOfFlags[1]<<i*8) | EEPROM_READ_BYTE(BRIDGES_FLAG_START+i+4);
	}
		
	if(NodeID < 0 || NodeID > 0x40)
	{
		val2ret = 0;
		printf("Returning 0\n\r");
		return val2ret;
	}
	else
	{
		if(NodeID >= 0x20)
		{
			if(action == SET)
			{
				tempArr[1] = 1;
				tempArr[1] = (tempArr[1] << (NodeID - 0x20));
				arrOfFlags[1] |= tempArr[1];
			}
			else if(action == RESET)
			{
				tempArr[1] = 1;
				tempArr[1] = (tempArr[1] << (NodeID - 0x20));
				arrOfFlags[1] &= ~tempArr[1];
			}
			printf("The value to write1 is: %x\n\r", arrOfFlags[1]);
			EEPROM_WRITE_WORD(BRIDGES_FLAG_START, tempArr[1]);
		}
		else
		{
			if(action == SET)
			{
				tempArr[0] = 1;
				tempArr[0] = (tempArr[0] << NodeID);
				arrOfFlags[0] |= tempArr[0];
			}
			else if(action == RESET)
			{
				tempArr[0] = 1;
				tempArr[0] = (tempArr[0] << NodeID);
				arrOfFlags[0] &= ~tempArr[0];
			}
			printf("The value to write0 is: %x\n\r", arrOfFlags[0]);
			EEPROM_WRITE_WORD(BRIDGES_FLAG_START+4, tempArr[1]);
			
		}
		
		val2ret = 1;
		return val2ret;
	}
		
}*/




/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/



