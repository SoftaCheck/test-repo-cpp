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
  * @file           : modbus_impl.c
  * @brief          : This file contains functions related to the implemenation of
											modbus and the interaction with the memory space of the modbus. 
										  Generally speaking, the functions here are called from modbus_funcs.c
											and the functions here use functions from W3_Flash.c
	* @DateUpdated		: 18.08.2018
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/

/* USER CODE BEGIN Includes */
#include "stdio.h"
#include <string.h>
#include "usart.h"
#include <stdint.h>
#include "gpio.h"
#include "modbus_app_layer.h"
#include "modbus_impl.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
 
/* Private function prototypes -----------------------------------------------*/

//extern int flashErasePageWriteData(uint32_t Address, uint32_t *Data, uint32_t size);
//extern void writeFlashRegTable(uint32_t Address, uint32_t data, uint16_t size);
//extern void FlashPageEraseAndWrite(uint32_t Address, uint32_t *Data, uint32_t size);
//extern uint8_t response[MAX_MODBUS_SLAVE_COMMAND_SIZE];

/* Exported functions ---------------------------------------------------------*/




//int writeTOFlashTable(uint32_t Address, uint16_t *Value, uint16_t size)
//{
//	printf("The data to write also before changing to 32 second is%x\n\r", Value[0]);

//		//flashErasePageWriteData( Address, (uint32_t)Value, (uint32_t)size);
//		HAL_FLASH_Lock();

//	return ( 8 );
//}



//int writeTOFlashTableMultiData(uint32_t Address, uint32_t *Value, uint32_t size)
//{

//	HAL_FLASH_Unlock( );
//	FlashPageEraseAndWrite( Address, Value, size);
//	HAL_FLASH_Lock( );

//	return ( 1 );
//}


//void Modbus_WriteSingleRegister(uint32_t startAddress, uint16_t *value, uint16_t size)
//{
//	printf("The data to write also before changing to 32 is%x\n\r", value[0]);
//	writeTOFlashTable(startAddress, value, size);
//}

//void Modbus_WriteSingleCoil(uint32_t startAddress, uint16_t value)
//{

//	if ( value == 0 )
//	{
//		printf( "writing to single coil 0 address %x : %x\n\r", startAddress, value );
//	}
//	else if ( value == 0xFF00 )
//	{
//		printf( "writing to single coil 1 address %x : %x\n\r", startAddress, value );
//	}
//	else if ( value == 1 )
//	{
//		printf( "writing to single coil 1 address %x : %x\n\r", startAddress, value );
//	}
//}

//int_WriteMultipleRegister2FlashTable( uint32_t Address, uint16_t Value )


/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/

