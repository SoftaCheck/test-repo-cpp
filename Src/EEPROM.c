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
  * @file           : eeprom.c
  * @brief          : This is a user application driver for the EEPROM 
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "eeprom.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
 
/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/


/**
  * @brief  This function performs a Write sequence to the eeprom of one word - 32 bits
  * @param  uint32_t relative_address - Relative addres of eeprom  within the eeprom memory bank. (e.g. the value 0 indicates it's the beginning of the bank)
            uint32_t data - Data to be written to eeprom
  * @retval None
  */
void EEPROM_WRITE_WORD(uint32_t relative_address, uint32_t data)
{
  uint8_t SUCCESS_FLAG = 0, attempts_num = 0;
  uint32_t address_location = FLASH_EEPROM_START_ADDR + (uint32_t)relative_address;
   
  // only write to the EEPROM address area of the memory
  if( address_location >= FLASH_EEPROM_START_ADDR && address_location <= FLASH_EEPROM_END_ADDR)
  {
    HAL_FLASHEx_DATAEEPROM_Unlock();// unlock the EEPROM
    do
    {
      #ifdef STM32L152xE
        HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEPROGRAM_WORD,FLASH_EEPROM_START_ADDR + (uint32_t)relative_address);
      #else
        #ifdef STM32L063xx
          HAL_FLASHEx_DATAEEPROM_Erase(FLASH_EEPROM_START_ADDR + (uint32_t)relative_address);
        #endif
      #endif
      if (HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, FLASH_EEPROM_START_ADDR + (uint32_t)relative_address, data) != HAL_OK)
        attempts_num++;
      else
        SUCCESS_FLAG = 1;
    }
    while(!SUCCESS_FLAG && attempts_num<2);
       HAL_FLASH_Lock();     // lock the flash  
       
    if(!SUCCESS_FLAG)
      Error_Handler();
  }
}

/**
  * @brief  This function performs a Write sequence to the eeprom of one byte - 8 bits
  * @param  uint32_t relative_address - Relative addres of eeprom  within the eeprom memory bank. (e.g. the value 0 indicates it's the beginning of the bank)
            uint32_t data - Data to be written to eeprom
  * @retval None
  */
void EEPROM_WRITE_BYTE(uint32_t relative_address, uint32_t data)
{
	uint8_t SUCCESS_FLAG = 0, attempts_num = 0;
  uint32_t address_location = FLASH_EEPROM_START_ADDR + (uint32_t)relative_address;
	
	// only write to the EEPROM address area of the memory
  if( address_location >= FLASH_EEPROM_START_ADDR && address_location <= FLASH_EEPROM_END_ADDR)
  {
    HAL_FLASHEx_DATAEEPROM_Unlock();// unlock the EEPROM
    do
    {
      #ifdef STM32L152xE
        HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEPROGRAM_WORD,FLASH_EEPROM_START_ADDR + (uint32_t)relative_address);
      #else
        #ifdef STM32L063xx
          HAL_FLASHEx_DATAEEPROM_Erase(FLASH_EEPROM_START_ADDR + (uint32_t)relative_address);
        #endif
      #endif
      if (HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, FLASH_EEPROM_START_ADDR + (uint32_t)relative_address, data) != HAL_OK)
        attempts_num++;
      else
        SUCCESS_FLAG = 1;
    }
    while(!SUCCESS_FLAG && attempts_num<2);
       HAL_FLASH_Lock();     // lock the flash  
       
    if(!SUCCESS_FLAG)
      Error_Handler();
  }
}

/**
  * @brief  This function performs a Read sequence from the eeprom of one word - 32 bits
  * @param  uint32_t relative_address - Relative addres of eeprom within the eeprom memory bank. (e.g. the value 0 indicates it's the beginning of the bank)
  * @retval uint32_t - the value read from the relative address given
  */
uint32_t EEPROM_READ_WORD(uint32_t relative_address)
{
  return (*(__IO uint32_t*)(FLASH_EEPROM_START_ADDR + (uint32_t)relative_address));
} 

/**
  * @brief  This function performs a Read sequence from the eeprom of one byte - 8 bits
  * @param  uint32_t relative_address - Relative addres of eeprom  within the eeprom memory bank. (e.g. the value 0 indicates it's the beginning of the bank)
  * @retval uint8_t - the value read from the relative address given
  */
uint8_t  EEPROM_READ_BYTE(uint32_t relative_address)
{
  return (*(__IO uint8_t*)(FLASH_EEPROM_START_ADDR + (uint32_t)relative_address));
}

/**
  * @brief  This function performs a reading of a stream of data from the eeprom
  * @param  uint32_t relative_address - Relative address of eeprom  within the eeprom memory bank. (e.g. the value 0 indicates it's the beginning of the bank)
            uint16_t initialAddress - stream size to be read
  * @retval None
  */ 
void EEPROM_READ_BYTE_SECTOR(uint8_t *arrayPtr, uint16_t arraySize,uint16_t initialAddress)
{
  uint16_t i=0;
  for(;i<arraySize;i++)
  {
    arrayPtr[i]=EEPROM_READ_BYTE((uint32_t)(initialAddress+i));
  }
}


/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/
