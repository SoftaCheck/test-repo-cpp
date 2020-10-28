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
  * @file           : bootloader_support.c
  * @brief          : This is a user application driver for supporting the bootloader
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "bootloader_support.h"
#include "external_flash.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
 
uint8_t BOOTLOADER_PROCESS_START_FLAG = FALSE; 

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

//uint16_t CalcCRC(uint8_t * buf, uint16_t size);

/* Exported functions ---------------------------------------------------------*/


/**
  * @brief  This function prepares the MCU for bootloading. It erases the relevant memory space in the external flash for writing the image there.
						This function doesn't receive the image and save it in the external flash.
  * @param  None
  * @retval None
  */
void Prepare_For_Bootloading(void)
{
	uint8_t sector_index;
	
	//Erase the relevant sectors destined to hold the application image for bootloading
	for(sector_index = APP_IMAGE_SECTOR_START; sector_index < APP_IMAGE_SECTOR_ACCESSORIES ; sector_index++)
		EXTERNAL_FLASH_EraseSector(sector_index*BASE_SECTOR_SIZE);
		
	#ifdef ENABLE_BOOTLOADING_LOGS
		printf("Prepare_For_Bootloading: firmware upgrade process started.\n\r");
	#endif
}

/**
  * @brief  This function launches the bootloader (by performing a SW reset) and sets the ENABLE_FIRMWARE_UPGRADE as TRUE
						so that the bootloader can detect it and perform the bootloading process instead of jumping back to the application
  * @param  None
  * @retval None
  */
void Begin_Bootloading(void)
{
	#ifdef ENABLE_BOOTLOADING_LOGS
		printf("Beginning bootloading. Performing system reset in order to launch the bootloader\n\r");
	#endif
	Param_SetVal(ENABLE_FIRMWARE_UPGRADE,TRUE);	// set the parameter as TRUE so that the bootloader can detect it and perform the bootloading process instead of jumping back to the application
	
	SystemReset();	// perform a software reset in order to go to the bootloader and load the application from the external flash
}

/**
  * @brief  This function saved the binary image size to the external flash in the destined location
  * @param  uint16_t binary_image_bytes_received
  * @retval None
  */
void Save_Image_Size_To_ExternalFlash(uint16_t binary_image_bytes_received)
{
	uint8_t data[APP_IMAGE_BYTE_NUM_SIZE];
	uint32_t add2Write = (APP_IMAGE_SECTOR_ACCESSORIES * BASE_SECTOR_SIZE) + APP_IMAGE_BYTE_NUM_POSITION;
	
	data[0] = (uint8_t)((binary_image_bytes_received >> 8) & 0x00FF);	// MSB
	data[1] = (uint8_t)(binary_image_bytes_received & 0x00FF);	// LSB
	EXTERNAL_FLASH_WriteData_Automatic(add2Write, APP_IMAGE_BYTE_NUM_SIZE, data);
	
	#ifdef ENABLE_BOOTLOADING_LOGS
		printf("Saved binary image size to the external flash\n\r");
	#endif
}

/**
  * @brief  This function saved the binary image size to the external flash in the destined location
  * @param  uint16_t binary_image_bytes_received
  * @retval None
  */
uint16_t Read_Image_Size_From_ExternalFlash(void)
{
	uint8_t data[APP_IMAGE_BYTE_NUM_SIZE];
	uint32_t add2Read = (APP_IMAGE_SECTOR_ACCESSORIES * BASE_SECTOR_SIZE) + APP_IMAGE_BYTE_NUM_POSITION;
	uint16_t binary_image_bytes_received;
	
	EXTERNAL_FLASH_ReadData(add2Read, data, APP_IMAGE_BYTE_NUM_SIZE);
	binary_image_bytes_received = ((uint16_t)(data[0] << 8) & 0xFF00) + (uint16_t)data[1];	// MSB + LSB
	
	return binary_image_bytes_received;
}

/**
  * @brief  This function calculates the CRC of the binary image stored in the external flash 
						and then stores the CRC in the designated location in the external flash so that when the bootloader loads it can do a validation of the CRC
  * @param  None
  * @retval uint16_t crc - the calculated CRC value
  */
uint16_t Calc_And_Save_Binary_Image_CRC(void)
{
	#define DATA_PACKET_READ_SIZE 	256			// choose a number that doesn't seem too big for a buffer, yet not too small.
	uint32_t add2Read = APP_IMAGE_SECTOR_START * BASE_SECTOR_SIZE;	// the beginning address in the external flash where the binary image is saved
	uint8_t read_num; 
	uint16_t binary_image_bytes_received = Read_Image_Size_From_ExternalFlash();
	uint16_t total_number_of_reads = (binary_image_bytes_received/DATA_PACKET_READ_SIZE) + 1;	// number of times we shall read data from the flash until all the data is read and the CRC is calculated
	uint8_t data[DATA_PACKET_READ_SIZE];
	uint16_t Bytes2Read;
	uint16_t crc = 65535;	// init value of the crc
	uint16_t i;	// loop index for crc calculation
	uint8_t tableIndex;	// index for the crc calculation
	
	for( read_num = 0 ; read_num < total_number_of_reads ; read_num++ )
	{
		if(read_num == (total_number_of_reads - 1))
				Bytes2Read = binary_image_bytes_received % DATA_PACKET_READ_SIZE; // the last read won't necessarily divide by DATA_PACKET_READ_SIZE exactly so we need to calculate exactly how many bytes to read
		else
			Bytes2Read = DATA_PACKET_READ_SIZE;
		
		EXTERNAL_FLASH_ReadData(add2Read, data, Bytes2Read);
		add2Read += Bytes2Read;
		
		//calculate CRC here:
		for( i = 0 ; i < Bytes2Read ; i++ )
		{
			tableIndex = (uint8_t)(crc ^ data[i]);
			crc >>= 8;
			crc ^= crcTable[tableIndex];
		}
		
	}
	
	//save crc in the external flash:
	uint32_t add2Write = (APP_IMAGE_SECTOR_ACCESSORIES * BASE_SECTOR_SIZE) + APP_IMAGE_CRC_POSITION;
	data[0] = (uint8_t)((crc >> 8) & 0x00FF);	// MSB
	data[1] = (uint8_t)(crc & 0x00FF);	// LSB
	EXTERNAL_FLASH_WriteData_Automatic(add2Write, APP_IMAGE_CRC_SIZE, data);
	
	#ifdef ENABLE_BOOTLOADING_LOGS
		printf("Calculated binary image CRC and saved it to the external flash\n\r");
	#endif
	
	return crc;
}

/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/
