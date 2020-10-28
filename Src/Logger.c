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
  * @file           : logger.c
  * @brief          : This file contains functions related to the logging of data.
                      It can contain either functions related to printing data
                      outwards (e.g. with UART) or saving the data internally
                      for later extraction by the user (internal or external flash)
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "logger.h"
#include "external_flash.h"
#include "gpio.h"
#include "encryption.h"

/* Private typedef -----------------------------------------------------------*/
#ifdef ENABLE_DEBUG_MODE
	enum commands
	{
		NOTHING,										//0
		EXT_FLASH_FILL_SECTOR,			//1
		EXT_FLASH_DEVICE_ID,				//2
		EXT_FLASH_AUTO_WRITE,				//3	
		PRINT_FROM_EXTERNAL_FLASH		//4
	};
#endif
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
 
/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/

//#############################################################################/
/**
  * @brief  This section is for supporting the printf function
  * @param  None
  * @retval None
  */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)&ch, 1, LOGGER_UART_TRANSMIT_POLLING_TIMEOUT); //##FOR NEW BOARD BRINGUP: CHANGE THE UART HANDLE HERE BASED ON THE UART THAT IS USED FOR PRINTING LOGS##
  return ch;
}
/* end of the section of the printf support */
//#############################################################################/

#ifdef ENABLE_DEBUG_MODE	// if debug mode is disabled, then all this code is not compiled, saving precious flash space

/**
  * @brief  This function analyzes and executes a command receieved via the debug port
	* @param  uint8_t *dataPtr - pointer to the string of data received as a command
  * @retval None
  */
void Command_Analyzer(uint8_t *dataPtr)
{
	const char param[2] = " ";
	static uint8_t command_index;
	
	// Command code
	command_index = (uint8_t)atoi(strtok((char *)dataPtr, param));
	
	#ifdef COMMAND_HANDLER_LOGS
		printf("\n\rCommand Analyzer: received command index: %d\n\r",command_index);
	#endif
	
	switch(command_index)
	{
		case NOTHING:
			#ifdef COMMAND_HANDLER_LOGS
				printf("\n\rCommand Analyzer: Error, command index must be a number equal to 1 or higher.\n\r");
			#endif
		case EXT_FLASH_FILL_SECTOR:
			;
			/*uint16_t sector_num = (uint16_t)atoi(strtok(NULL, param));
			uint8_t data = (uint8_t)atoi(strtok(NULL, param));
			uint32_t write_byte_num =  (uint32_t)atoi(strtok(NULL, param));
			uint16_t delay =  (uint16_t)atoi(strtok(NULL, param));
			EXTERNAL_FLASH_FILL_SECTOR(sector_num,data,write_byte_num,delay);*/
			break;
		case EXT_FLASH_DEVICE_ID:
			EXTERNAL_FLASH_GetDeviceID();
			break;
		case EXT_FLASH_AUTO_WRITE:
			;
			uint16_t sector_num = (uint16_t)atoi(strtok(NULL, param));
			uint8_t data = (uint8_t)atoi(strtok(NULL, param));
			uint32_t write_byte_num =  (uint32_t)atoi(strtok(NULL, param));
			uint16_t delay =  (uint16_t)atoi(strtok(NULL, param));
			EXTERNAL_FLASH_AUTO_FILL_SECTORS(sector_num,data,write_byte_num,delay);
			break; 
		case PRINT_FROM_EXTERNAL_FLASH:
			printf("Print from flash logger command\n\r");
			PrintFromFlashHandler(dataPtr);
			break;
		default:
			#ifdef COMMAND_HANDLER_LOGS
				printf("\n\rCommand Analyzer: Error, command index does not exist\n\r");
			#endif
			break;
	}
}


/**
  * @brief  This function reads a section of memory from the external flash.
	* @param  uint8_t *dataPtr - pointer to the string of data received as a command
  * @retval None
  */
void PrintFromFlashHandler(uint8_t *dataPtr)
{
	const char param[2] = " ";
	//static uint8_t command_index;
	  
	uint32_t address = (uint32_t)atoi(strtok(NULL, param));
	uint16_t length = (uint16_t)atoi(strtok(NULL, param));
	uint8_t tempArr[255] = {0};
	
	EXTERNAL_FLASH_ReadData(address, tempArr, length);
	printf("External flash read via logger. Address = 0x%x, length = %d\n\r", address, length);
	for(uint8_t i=0;i<length;i++)
		printf("%x ", tempArr[i]);
	
}

/**
  * @brief  This function fills a sector in the external flash with a value. The procedure
						is first, erase the sector, then write to the sector and then read from the sector and print what's read.
						The erase is done using an erase sector command.
						The write to the flash is done in chunk sizes set by the user. So it is possible to write
						to the flash sector one byte at a time or in chunks of 300 bytes (max is currently defined as MAX_BYTE_NUM_BUFFER per chunk).
						The read is performed in similar chunk sizes. 
						It is possible to define a delay between each process (erase, write, read) in units of milliseconds. This is usefull
						in case the user wants to analyze with an oscilloscope the SPI lines for each procedure separately. Also, if the delay is 
						not zero then when printing, each chunk will be printed seperately for easier debugging. 
	* @param  uint16_t sector_num
						uint8_t data
						uint32_t write_byte_num
						uint16_t delay 
						See the brief for the explanations on the input parameters
  * @retval None
  */
void EXTERNAL_FLASH_FILL_SECTOR(uint16_t sector_num, uint8_t data, uint32_t write_byte_num, uint16_t delay)
{ 
		#define     MAX_BYTE_NUM_BUFFER     512
		#define			MAX_SECTOR_NUM					255						// the number of sectors is actually this value plus 1 since the count includes zero. 
    uint32_t    address;
    uint32_t    end_sector_address;
    uint32_t     write_fail_num = 0, read_fail_num = 0;
    static uint8_t     data_array[MAX_BYTE_NUM_BUFFER];
    uint32_t     i;
    uint32_t     temp_byte_num = 0;      // used for in case that the write_byte_num doesn't divide equally with the number of bytes in the sector.
    uint32_t     ret_err; 
    uint32_t     mismatch_err = PASS;
    
    if(write_byte_num > MAX_BYTE_NUM_BUFFER)
    {
      printf("Command Failure: write/read packet size is too long. Must not exceed %d. Current value is: %d\n\r",MAX_BYTE_NUM_BUFFER,write_byte_num);
      return; 
    }
		
    if(sector_num > MAX_SECTOR_NUM)
    {
      printf("Command Failure: Sector address should be between 0 and %d. Current value is: %d\n\r\n\r",MAX_SECTOR_NUM,sector_num);
      return; 
    }
    
		EXTERNAL_FLASH_Init();	// initialize the external flash
		
    end_sector_address = (((uint32_t)sector_num+1) * (uint32_t)SECTOR_SIZE) - 1;    // Eyal Gerber:  (0x00001000u - 1) = 0x00000FFFu = the size of one sector in the flash
    address = end_sector_address - (uint32_t)(SECTOR_SIZE-1); // Eyal Gerber: the beginning address of the sector
		
    #ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
    printf("sector_num = %d, data = %d, write_byte_num = %d, end_sector_address = %x, address = %x\n\r",sector_num,data, write_byte_num,end_sector_address,address);
    #endif
    
    for(i=0;i<MAX_BYTE_NUM_BUFFER;i++)
      data_array[i] = data;
		
    #ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
			printf("Start erase of sector %d in the external flash\r\n",sector_num);
    #endif
    
    EXTERNAL_FLASH_EraseSector(address);
		
    #ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
			printf("Finished erase of sector %d in the external flash\r\n",sector_num);
    #endif
		
    HAL_Delay(delay);
		
    #ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
			printf("Start writing data = %d to all bytes in sector %d in external flash in chuncks of %d bytes\r\n",data,sector_num,write_byte_num);
    #endif
    
    for( ; address < end_sector_address ; address+=write_byte_num)
    { 
      if(address + write_byte_num > end_sector_address)        //if there is a potential overflow beyond the address of the sector
          temp_byte_num = end_sector_address - address;
      else
        temp_byte_num = write_byte_num;
      
      if( (ret_err = EXTERNAL_FLASH_WriteData(address, temp_byte_num, data_array)) != PASS )
      {
        write_fail_num++;
				printf("Write Fail at Address: %x, Fail Reason: %x\n\r",address,ret_err);
      }

    }
		
    #ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
			printf("Finished write operation to external flash\r\n");
    #endif

    HAL_Delay(delay);
    
    for(i=0;i<MAX_BYTE_NUM_BUFFER;i++)
      data_array[i] = 0x10;        // erase the array before the read (set it with a specific value and not 0 or FF to know whether any value was successfully written/read from the flash)
		
    #ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
			printf("Begin read and print operation of sector %d in external flash in chuncks of %d bytes\r\n",sector_num,write_byte_num);
			printf("\r\n");
    #endif

    address = end_sector_address - 0x00000FFFu; // the beginning address of the sector
    
    for( ; address < end_sector_address ; address+=write_byte_num)
    { 
			if(delay!=0)      //Eyal Gerber: (16.08.2018) only if the user asked for a delay then we also print the data in chunks for better readability but much slower
      {
        #ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
					printf("\n\r");
        #endif
			}
			
      if(address + write_byte_num > end_sector_address)        //if there is a potential overflow beyond the address of the sector
          temp_byte_num = end_sector_address - address;
      else
        temp_byte_num = write_byte_num;
      
      if( (ret_err = EXTERNAL_FLASH_ReadData(address, data_array, temp_byte_num))!=PASS )
			{
        read_fail_num++;
				printf("Read Fail at address: 0x%x, Fail Reason: 0x%x\n\r",address,ret_err);
			}
			
			if(delay!=0)      //Eyal Gerber: (16.08.2018) only if the user asked for a delay then we also print the data in chunks for better readability but much slower
      {
        #ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
					printf("ADDRESS: 0x%x   -   ",address);
        #endif
        HAL_Delay(1);
      }

      for(i=0;i<temp_byte_num;i++)
      {
          if(data_array[i] != data) 
						mismatch_err = FAIL;
          #ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
						printf("%d ",data_array[i]);
          #endif
             if(delay!=0) // Eyal Gerber: (16.08.2018) only if the user asked for a delay then we also add this delay here to print the data in chunks (at the top of the function) - this delay is to help the UART not get overflowed. If the chunk is really small (1 byte) then the uart still gets overflowed and not everything is always printed. In such case it may be wise to increase the delay beyond 1 ms.
                 HAL_Delay(1);
      }
      
    }
    
    printf("\n\rwrite_fail_num = %d, ||| read_fail_num = %d\n\r",write_fail_num,read_fail_num);
    
    if(mismatch_err == PASS)
        printf("Console: FILL_SECTOR: Success\r\n");
    else 
        printf("Console: FILL_SECTOR: Fail\r\n");
}




/**
  * @brief  This function performs various system tests
  * @param  None
  * @retval None
  */
void system_tests(void)
{
  static uint8_t press_counter = 0;
	
  //press_counter++;
  
  switch(press_counter)
  {
    case 1:   
			
      //printf("\n\r\n\r## External Flash Test ##\n\r");
      //EXTERNAL_FLASH_FILL_SECTOR(0,5,100,0);
      break;
    default:
			printf("\n\r\n\r## Blink LEDs ##\n\r");
			AllLedsBlink(100,20);
			Encryption_Test();
			press_counter = 0;	// elapse the counter	
      break;
  }
}

#define AES_TEXT_SIZE 64
#define TIMEOUT_VALUE 0xFF
uint8_t aPlaintext[AES_TEXT_SIZE] = {0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
                                     0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a,
                                     0xae,0x2d,0x8a,0x57,0x1e,0x03,0xac,0x9c,
                                     0x9e,0xb7,0x6f,0xac,0x45,0xaf,0x8e,0x51,
                                     0x30,0xc8,0x1c,0x46,0xa3,0x5c,0xe4,0x11,
                                     0xe5,0xfb,0xc1,0x19,0x1a,0x0a,0x52,0xef,
                                     0xf6,0x9f,0x24,0x45,0xdf,0x4f,0x9b,0x17,
                                     0xad,0x2b,0x41,0x7b,0xe6,0x6c,0x37,0x10};
uint8_t aEncryptedtext[AES_TEXT_SIZE];
uint8_t aDecryptedtext[AES_TEXT_SIZE];
void Encryption_Test(void)
{
	uint8_t i;
	printf("\n\rBefore Encryption:\n\r");
	for(i=0;i<AES_TEXT_SIZE;i++)
		printf("%d",aPlaintext[i]);
	
	Encrypt_Data(aPlaintext,AES_TEXT_SIZE,aEncryptedtext);
	
	printf("\n\rAfter Encryption:\n\r");
	for(i=0;i<AES_TEXT_SIZE;i++)
		printf("%d",aEncryptedtext[i]);
	
	Decrypt_Data(aEncryptedtext,AES_TEXT_SIZE,aDecryptedtext);
	
	printf("\n\rAfter Decryption:\n\r");
	for(i=0;i<AES_TEXT_SIZE;i++)
		printf("%d",aDecryptedtext[i]);
}


/**
  * @brief  This function gets the external flash device ID using the RDID command and prints the data that is read from the external flash
  * @param  None
  * @retval None
  */
void EXTERNAL_FLASH_GetDeviceID(void)
{
	uint8_t manufacturerIdPtr;
	uint8_t memTypePtr;
	uint8_t memDensityPtr;
	
	EXTERNAL_FLASH_Init();

	EXTERNAL_FLASH_ReadDeviceID(&manufacturerIdPtr,&memTypePtr,&memDensityPtr);
	printf("\n\rmanufacturerIdPtr = %d, memTypePtr = %d, memDensityPtr = %d\n\r",manufacturerIdPtr,memTypePtr,memDensityPtr);	
}

/**
  * @brief  This function fills two sectors in the external flash with a value. The procedure
						is first, write to the sectors and then read from the sectors and print what's read.
						The first sector is received is written to first and the following sector is written to afterwards. 
						The write to the flash is done in chunk sizes set by the user. So it is possible to write
						to the flash sector one byte at a time or in chunks of 300 bytes (max is currently defined as MAX_BYTE_NUM_BUFFER per chunk).
						The read is performed in similar chunk sizes. 
						It is possible to define a delay between each process (write, read) in units of milliseconds. This is usefull
						in case the user wants to analyze with an oscilloscope the SPI lines for each procedure separately. Also, if the delay is 
						not zero then when printing, each chunk will be printed seperately for easier debugging. 
	* @param  uint16_t sector_num
						uint8_t data
						uint32_t write_byte_num
						uint16_t delay 
						See the brief for the explanations on the input parameters
  * @retval None
  */
void EXTERNAL_FLASH_AUTO_FILL_SECTORS(uint16_t sector_num, uint8_t data, uint32_t write_byte_num, uint16_t delay)
{ 
		#define     MAX_BYTE_NUM_BUFFER     512
		#define			MAX_SECTOR_NUM					255						// the number of sectors is actually this value plus 1 since the count includes zero. 
    uint32_t    address;
    uint32_t    end_sector_address;
    uint32_t     write_fail_num = 0, read_fail_num = 0;
    static uint8_t     data_array[MAX_BYTE_NUM_BUFFER];
    uint32_t     i;
    uint32_t     temp_byte_num = 0;      // used for in case that the write_byte_num doesn't divide equally with the number of bytes in the sector.
    uint32_t     ret_err; 
    uint32_t     mismatch_err = PASS;
    
    if(write_byte_num > MAX_BYTE_NUM_BUFFER)
    {
      printf("Command Failure: write/read packet size is too long. Must not exceed %d. Current value is: %d\n\r",MAX_BYTE_NUM_BUFFER,write_byte_num);
      return; 
    }
		
    if(sector_num > MAX_SECTOR_NUM-1)	// must be minus one because we write to two sectors. 
    {
      printf("Command Failure: Sector address should be between 0 and %d. Current value is: %d\n\r\n\r",MAX_SECTOR_NUM-1,sector_num);
      return; 
    }
    
		EXTERNAL_FLASH_Init();	// initialize the external flash
		
		end_sector_address = (((uint32_t)sector_num+2) * (uint32_t)SECTOR_SIZE) - 1;    // Eyal Gerber:  (0x00001000u - 1) = 0x00000FFFu = the size of one sector in the flash
		address = ((uint32_t)sector_num)*((uint32_t)SECTOR_SIZE); // Eyal Gerber: the beginning address of the sector
		
		#ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
			printf("sector_num = %d, data = %d, write_byte_num = %d, end_sector_address = %x, address = %x\n\r",sector_num,data, write_byte_num,end_sector_address,address);
		#endif
		
		for(i=0;i<MAX_BYTE_NUM_BUFFER;i++)
			data_array[i] = data;
		
		#ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
			printf("Start writing data = %d to all bytes in sector %d and in sector %d in external flash in chuncks of %d bytes\r\n",data,sector_num,sector_num+1,write_byte_num);
		#endif
		
		for( ; address < end_sector_address ; address+=write_byte_num)
		{ 
			if(address + write_byte_num > end_sector_address)        //if there is a potential overflow beyond the address of the sector
					temp_byte_num = end_sector_address - address;
			else
				temp_byte_num = write_byte_num;
			
			if( (ret_err = EXTERNAL_FLASH_WriteData_Automatic(address, temp_byte_num, data_array)) != PASS )
			{
				write_fail_num++;
				printf("Write Fail at Address: %x, Fail Reason: %x\n\r",address,ret_err);
			}

		}
		
		#ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
			printf("Finished write operation to external flash\r\n");
		#endif

		HAL_Delay(delay);
		
		for(i=0;i<MAX_BYTE_NUM_BUFFER;i++)
			data_array[i] = 0x10;        // erase the array before the read (set it with a specific value and not 0 or FF to know whether any value was successfully written/read from the flash)
		
		#ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
			printf("Begin read and print operation of sector %d in external flash in chuncks of %d bytes\r\n",sector_num,write_byte_num);
			printf("\r\n");
		#endif

		address = end_sector_address - 0x00000FFFu; // the beginning address of the sector
		
		for( ; address < end_sector_address ; address+=write_byte_num)
		{ 
			if(delay!=0)      //Eyal Gerber: (16.08.2018) only if the user asked for a delay then we also print the data in chunks for better readability but much slower
			{
				#ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
					printf("\n\r");
				#endif
			}
			
			if(address + write_byte_num > end_sector_address)        //if there is a potential overflow beyond the address of the sector
					temp_byte_num = end_sector_address - address;
			else
				temp_byte_num = write_byte_num;
			
			if( (ret_err = EXTERNAL_FLASH_ReadData(address, data_array, temp_byte_num))!=PASS )
			{
				read_fail_num++;
				printf("Read Fail at address: 0x%x, Fail Reason: 0x%x\n\r",address,ret_err);
			}
			
			if(delay!=0)      //Eyal Gerber: (16.08.2018) only if the user asked for a delay then we also print the data in chunks for better readability but much slower
			{
				#ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
					printf("ADDRESS: 0x%x   -   ",address);
				#endif
				HAL_Delay(1);
			}

			for(i=0;i<temp_byte_num;i++)
			{
					if(data_array[i] != data) 
						mismatch_err = FAIL;
					#ifdef ENABLE_FILL_SECTOR_ADVANCED_LOGS
						printf("%d ",data_array[i]);
					#endif
						 if(delay!=0) // Eyal Gerber: (16.08.2018) only if the user asked for a delay then we also add this delay here to print the data in chunks (at the top of the function) - this delay is to help the UART not get overflowed. If the chunk is really small (1 byte) then the uart still gets overflowed and not everything is always printed. In such case it may be wise to increase the delay beyond 1 ms.
								 HAL_Delay(1);
			}
			
		}
		
		printf("\n\rwrite_fail_num = %d, ||| read_fail_num = %d\n\r",write_fail_num,read_fail_num);
		
		if(mismatch_err == PASS)
				printf("Console: FILL_SECTOR: Success\r\n");
		else 
				printf("Console: FILL_SECTOR: Fail\r\n");
}

#endif //ENABLE_DEBUG_MODE
/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/
