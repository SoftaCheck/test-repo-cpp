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
  * @file           : external_flash.c
  * @brief          : This file contains functions related to the external flash
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/

#include "external_flash.h"
#include "gpio.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define FLASHBUSY_TIMEOUT		100			// [msec] timeout theshold for waiting for flash to finish being busy

/* Private macro -------------------------------------------------------------*/

#define EXT_FLASH_CS_LOW()	GPIO_SetVal(SPI1_NNS,LOW)
#define EXT_FLASH_CS_HIGH()	GPIO_SetVal(SPI1_NNS,HIGH)
#define EXT_FLASH_GET_TICK()	HAL_GetTick()							// for getting the tick timer counter [msec]

/* Private variables ---------------------------------------------------------*/
 
/* Private function prototypes -----------------------------------------------*/

uint32_t EXTERNAL_FLASH_EnableWriting(void);
uint32_t EXTERNAL_FLASH_WaitUntilFlashNotBusy(void);
void CS_ENABLE(void);
void CS_DISABLE(void);

/* Exported functions ---------------------------------------------------------*/

/**
  * @brief  This function initiates the external flash - all that is needed is to drive the CS (chip select) HIGH
  * @param  None
  * @retval None
  */
void EXTERNAL_FLASH_Init(void)
{
    //Set SPI_CS high                  
    GPIO_SetVal(SPI1_NNS,HIGH);                   
}

/**
  * @brief  This function sets the CS (chip select) line to LOW
  * @param  None
  * @retval None
  */
void CS_ENABLE(void)
{
	GPIO_SetVal(SPI1_NNS,LOW); // Set MEM_CS pin to Low
}

/**
  * @brief  This function sets the CS (chip select) line to HIGH
  * @param  None
  * @retval None
  */
void CS_DISABLE(void)
{
	GPIO_SetVal(SPI1_NNS,HIGH); // Set MEM_CS pin to HIGH
}

/**
  * @brief  This function reads the external flash device ID, memory type and memory density (command RDID in datasheet)
  * @param  uint8_t *manufacturerIdPtr	-	pointer to manufacturer ID into which the value is assigned
						uint8_t *memTypePtr	-	pointer to memory type into which the value is assigned
						uint8_t *memDensityPtr	-	pointer to memory density into which the value is assigned
  * @retval uint32_t - error code
  */
uint32_t EXTERNAL_FLASH_ReadDeviceID(uint8_t *manufacturerIdPtr,uint8_t *memTypePtr,uint8_t *memDensityPtr)
{
    uint32_t retErr=PASS;
    uint8_t WriteBuffer[5], buff_len;
		uint8_t rxData[4] = {0};
    

		// Prepare WriteBuffer for transmit. Command with dummy bytes.
		WriteBuffer[0] = RDID_REG;
		WriteBuffer[1] = 0u;
		WriteBuffer[2] = 0u;
		WriteBuffer[3] = 0u;
		buff_len = 4u;
		
		// Set MEM_CS pin to Low
		EXT_FLASH_CS_LOW();
		
		// Send ReadDeviceID command to the external flash and receive the data back fast.
		retErr = HAL_SPI_TransmitReceive(&hspi1, WriteBuffer, rxData, buff_len, SPI_EXT_FLASH_POLLING_TIMEOUT);
		
		// Set MEM_CS pin to High
		EXT_FLASH_CS_HIGH();
		
    if( retErr == PASS )
    {
			// Skip the first byte, Read the ManufacturerID & Memory type and Memory Density from the rx buffer (should be C2 28 14 respectively for MX25R8035F)
			*manufacturerIdPtr = rxData[1];
			*memTypePtr = rxData[2];
			*memDensityPtr = rxData[3];
    }

		// wait until the flash is no longer busy and only then continue (or until a timeout occurs)
    retErr = EXTERNAL_FLASH_WaitUntilFlashNotBusy();
		
		if( retErr != PASS )
    {
        #ifdef FLASH_LOGGER
            printf("\n\n\rEXTERNAL_FLASH_ReadDeviceID: Read device ID procedure failed with return code: %x\n\r",retErr);     
        #endif
        ;
    }

    return retErr;
}

/**
  * @brief  This function performs a blocking check (polling) to wait until the flash is not busy anymore or a timeout occurs.
						This function is used by the various other external flash functions such as write, read, erase, etc. 
  * @param  None
  * @retval uint32_t - error code
  */
uint32_t EXTERNAL_FLASH_WaitUntilFlashNotBusy(void)
{
	Tick_Counter tickstart = 0u;
	uint32_t retErr=PASS;
	bool busyFlg = TRUE;
	
	tickstart = EXT_FLASH_GET_TICK();	// sample the tick timer
	while( (busyFlg == TRUE) && (retErr == PASS) )	// while the flash is still busy and the timeout period hasn't elapsed
	{
			EXTERNAL_FLASH_IsFlashBusy(&busyFlg);    
			if(EXT_FLASH_GET_TICK() - tickstart > FLASHBUSY_TIMEOUT)
				retErr = FLASH_TIMEOUT_ERROR;
	}
	
	return retErr;
}


/*
 * Function:       IsFlashBusy.   Check WIP bit in "Read Status Register"
 * Arguments:      None.
 * Description:    Check status register WIP bit.
 *                 If  WIP bit = 1: return TRUE ( Busy )
 *                             = 0: return FALSE ( Ready ).
 * Return Message: TRUE, FALSE
 */
uint32_t EXTERNAL_FLASH_IsFlashBusy(bool *busyPtr)
{
    uint32_t retErr = PASS;
    uint8_t WriteBuffer[5], buff_len, tmpBuffer;
		uint8_t rxData[2] = {0};
    
    // Prepare WriteBuffer for transmit. "Read Status Register" Command with dummy bytes.
    WriteBuffer[0] = RDSR_REG;
    WriteBuffer[1] = 0u;
    buff_len = 2u;
     
    // Set MEM_CS pin to Low
		EXT_FLASH_CS_LOW();		

    // Send ReadDeviceID command to the external flash and receive the data back fast.
		retErr = HAL_SPI_TransmitReceive(&hspi1, WriteBuffer, rxData, buff_len, SPI_EXT_FLASH_POLLING_TIMEOUT);
        
		// Set MEM_CS pin to High
		EXT_FLASH_CS_HIGH();	

    if( retErr == PASS )
    {
        // Skip the first byte
        tmpBuffer = rxData[1];
			
        //iprintf (" WIP bit is: %x \n", tmpBuffer);
    
        if( (tmpBuffer & FLASH_WIP_MASK) == FLASH_WIP_MASK)
          *busyPtr= TRUE;
        else
					*busyPtr=FALSE;
    }
		
    if( retErr != PASS )
		{
				#ifdef FLASH_LOGGER
						printf("\n\n\rEXTERNAL_FLASH_IsFlashBusy: Error during checking if flash is busy, return code is: %d\n\r",retErr);     
				#endif
				;   
		} 
    
    return retErr;
}

/**
  * @brief  This function enables the writing to the external flash - required by some commands
						Set WREN. Sets the (WEL) write enable latch bit
  * @param  None
  * @retval uint32_t - error code
  */
uint32_t EXTERNAL_FLASH_EnableWriting(void)
{
    uint32_t retErr = PASS;
		uint8_t WriteBuffer[1], buff_len;
		uint8_t rxData[2] = {0};  
    
		WriteBuffer[0] = WREN_REG;	// put the Write Enable command into the write buffer
		buff_len = 1;		// the length of the buffer is one byte
		
    // Set MEM_CS pin to Low
		EXT_FLASH_CS_LOW();
    
		// Send WriteEnable command byte to set WEL bit.
		retErr = HAL_SPI_TransmitReceive(&hspi1, WriteBuffer, rxData, buff_len, SPI_EXT_FLASH_POLLING_TIMEOUT);
		
		// Set MEM_CS pin to High
		EXT_FLASH_CS_HIGH(); 
 
		// wait until the flash is no longer busy and only then continue (or until a timeout occurs)
    retErr = EXTERNAL_FLASH_WaitUntilFlashNotBusy();
		
		if( retErr != PASS )
    {
        #ifdef FLASH_LOGGER
            printf("\n\n\rEXTERNAL_FLASH_EnableWriting: Enable writing procedure failed with return code: %x\n\r",retErr);     
        #endif
        ;
    }
    
    return retErr;
}

/*******************************************************************
*   @Function: EXTERNAL_FLASH_WriteData
*   @Description: Write data to the flash - write any amount of data as long as does not exceed the size of the flash.
                      The data is written to the flash using the page program comamnd. 
                      In case the addresses written are on different pages then it will perform several page program commands
                      so that the data will be written correctly. 
                      It is assumed that the data is already erased (0xFF) at the relevant addresses. 
                      It is assumed that the area to write to is within the limits of the flash and that there will be no
                      overflow beyond the last address of the flash. 
*   @Input argumments: uint32_t address - start address of memory
                      uint16_t data_length - amount of bytes to write
                      uint8_t *data - pointer to array of data
*   @Output argumments: uint32_t retErr -  Error code
*******************************************************************/
uint32_t EXTERNAL_FLASH_WriteData(uint32_t address, uint16_t data_length, uint8_t *data)
{
  uint32_t retErr=PASS;
  uint16_t buff_len_for_calc;
  uint8_t *buffer_pointer;
  uint16_t bytes_until_end_of_page = 0;
  
  buffer_pointer = data; 
  buff_len_for_calc = data_length;
  
  bytes_until_end_of_page = (uint16_t)(EXTERNAL_FLASH_PAGE_SIZE - (address % EXTERNAL_FLASH_PAGE_SIZE));  // number of bytes until the end of the current page where "address" is located
  if( buff_len_for_calc <= bytes_until_end_of_page )   // if the number of bytes to write fits within the current page
  {
    retErr = EXTERNAL_FLASH_PageProgram(address, buff_len_for_calc, buffer_pointer);        // write to the flash the number of bytes required
    //buffer_pointer += buff_len_for_calc;  // increment the pointer by the number of bytes written to the flash
    //address += buff_len_for_calc; // adjust the address to be equal to the next place we need to write data to (the beginning of the next page)
    buff_len_for_calc = 0;       // decrease the size of the data to be written by the amount just written
  }
  else  // if there is an overflow beyond the first page.
  {
    retErr = EXTERNAL_FLASH_PageProgram(address, bytes_until_end_of_page, buffer_pointer);        // write to the flash until the end of the page
    buffer_pointer += bytes_until_end_of_page;  // increment the pointer by the number of bytes written to the flash
    address += bytes_until_end_of_page; // adjust the address to be equal to the next place we need to write data to (the beginning of the next page)
    buff_len_for_calc -= bytes_until_end_of_page;       // decrease the size of the data to be written by the amount just written
  }

  for( ; buff_len_for_calc!=0 ;  )
  {
      if(buff_len_for_calc <= EXTERNAL_FLASH_PAGE_SIZE)   // if the remaining bytes to write are less than a full EXTERNAL_FLASH_PAGE_SIZE
      {
        retErr = EXTERNAL_FLASH_PageProgram(address, buff_len_for_calc, buffer_pointer); 
        break;  // end of writing to flash
      }
      else        // send maximum possible with page program command (256 bytes)
      {
        retErr = EXTERNAL_FLASH_PageProgram(address, EXTERNAL_FLASH_PAGE_SIZE, buffer_pointer);
        if(retErr!=PASS)
          break;
        buffer_pointer += EXTERNAL_FLASH_PAGE_SIZE;    // increment the pointer by the amount we just wrote to the flash
        address += EXTERNAL_FLASH_PAGE_SIZE; // adjust the address to be equal to the next place we need to write data to (the beginning of the next page)
        buff_len_for_calc -= EXTERNAL_FLASH_PAGE_SIZE;       // decrease the size of the data to be written by the amount just written
      }
  }
  
  return retErr;
}

/*******************************************************************
*   @Function: EXTERNAL_FLASH_WriteData_Automatic
*   @Description: Write data to the flash - write any amount of data as long as does not exceed the size of the flash.
                      The data is written to the flash using the page program comamnd. 
                      In case the addresses written are on different pages then it will perform several page program commands
                      so that the data will be written correctly. 
                      When using this function, it isn't required to have the designated area already erased since the function takes care of that
											by:
											Case 1: The relevant pages to write to are all in the same sector - saving temperarly the data of the sector in which the relevant pages are located
															to a designated sector in the flash. Then erase the sector and rewrite the temperarly stored data to the sector together with the new data. 
											Case 2: The relevant pages to write to are in two sectors - Do the same thing as in Case 1 just twice - once for each sector. 
											Case 3 (NOT SUPPORTED): The relevant pages to write to are in three or more sectors - not supported since it would require a lot of RAM (each sector is 4K) so it would require
															at least 8K RAM which the current MCU does not have. 
											Assumptions:
                      1. It is assumed that the area to write to is within the limits of the flash and that there will be no
                         overflow beyond the last address of the flash. 
											2. It is assumbed that the received address does not fall within the holding sectors
*   @Input argumments: uint32_t received_address - address of where to begin to write in the memory
                      uint16_t data_length - amount of bytes to write
                      uint8_t *data - pointer to array of data
*   @Output argumments: uint32_t retErr -  Error code
*******************************************************************/
uint32_t EXTERNAL_FLASH_WriteData_Automatic(uint32_t received_address, uint16_t data_length, uint8_t *data)
{
  uint32_t retErr=PASS;
  uint16_t buff_len_for_calc;
  uint8_t *buffer_pointer;
  uint16_t bytes_until_end_of_page = 0;				// the number of bytes from the end of the page (from the received address) counting from the received address
	uint16_t bytes_until_received_address = 0;	// the number of bytes until the recieved address from the beginning of the page it is located in
	static uint16_t sector_index = FIRST_SECTOR_INDEX_FOR_MEM_HOLD;	// this must be static so that we remember what was the previous sector that we erased/programmed
	uint32_t beginning_of_holding_sector, beginning_of_second_holding_sector;
	uint32_t counter_address; 									// variable used for counting and progressing the memory in "for" loops
	uint32_t end_address_of_data;								// variable that holds the memory address of the last byte that needs to be written
	uint32_t relative_address; 
	uint32_t current_sector;	// the current sector in which the received address is located
	uint32_t last_sector; 		// the last sector in which the last byte to be written to is located
	uint32_t beginning_of_current_sector, beginning_of_second_sector;
	static uint8_t COPY_ARRAY[EXTERNAL_FLASH_PAGE_SIZE] = {0};	// array used for copying data between sectors. Initialize it to zeros for easier debugging. 
	uint8_t write_two_sectors_flag = FALSE;	// flag that indicates whether the data that needs to be written is over two sectors and not just one. 
	uint32_t write_sector_address, holding_sector_address;
	
	//################CODE SECTION SEPARATOR#####################################
	// perform memory boundry check to see if the received memory address falls within the limits of the external flash at all. 
	end_address_of_data = received_address + data_length; // variable that holds the memory address of the last byte that needs to be written
	if( (received_address > EXTERNAL_FLASH_SIZE) || (end_address_of_data > EXTERNAL_FLASH_SIZE) )	// both received_address and end_address_of_data are checked because end_address_of_data could potentially be overflowed and be within limits
	{
		retErr = FLASH_MEM_BOUNDRY_ERROR;
		#ifdef FLASH_LOGGER
				printf("\n\nEXTERNAL_FLASH_WriteData_Automatic: Address exceeded flash limits, return code is: 0x%x\n\r",retErr);     
		#endif
		return retErr; 
	}
	
	//make relevant calculations regarding the current sector
	current_sector = received_address / BASE_SECTOR_SIZE;	// calculate the current sector in which the received address is located
	//end_of_current_sector = (current_sector * (BASE_SECTOR_SIZE + 1)) - 1 ;
	beginning_of_current_sector = current_sector * BASE_SECTOR_SIZE;
	
	//check to see if the data to be written overflows into the following sector
	last_sector = end_address_of_data / BASE_SECTOR_SIZE; // calculate the sector in which the last address to be written is located
	if( last_sector != current_sector)	// if the last address to write to is not within the current sector
	{
		write_two_sectors_flag = TRUE;	// set the flag indicating that two sectors need to be written to	
		beginning_of_second_sector = last_sector * BASE_SECTOR_SIZE;
		
		if( last_sector - current_sector > 1)	// if the last sector to write to is not the sector right after the current sector (i.e. more than two sectors need to be written too)
		{
			retErr = FLASH_MEM_MAX_SECTOR_NUM_ERROR;
			#ifdef FLASH_LOGGER
					printf("\n\nEXTERNAL_FLASH_WriteData_Automatic: data is to be written over three or more sectors (max allowed is two), return code is: 0x%x\n\r",retErr);     
			#endif
			return retErr; 
		}	
	}
	
	// perform another memory boundry check to see if the received memory falls within the limits of the holding sectors
	if( (current_sector >= FIRST_SECTOR_INDEX_FOR_MEM_HOLD) && (current_sector <= LAST_SECTOR_INDEX_FOR_MEM_HOLD))
	{
		retErr = FLASH_MEM_HOLD_SECTOR_BOUNDRY_ERROR;
		#ifdef FLASH_LOGGER
				printf("\n\nEXTERNAL_FLASH_WriteData_Automatic: Current Sector is %d, Address is within holding sector limits, return code is: 0x%x\n\r",current_sector,retErr);     
		#endif
		return retErr; 
	}
	
	//################CODE SECTION SEPARATOR#####################################
	//Erase the designated memory holding sectors for copying the memory to it temporarily. 
	//The memory holding sectors are a group of sectors that each time, a different sector is erased in a loop
	//as a wear leveling technique to prevent the erosion of the flash which is limited in the number of its
	//erase cycles (100,000) per sector. 
	beginning_of_holding_sector = sector_index * BASE_SECTOR_SIZE;
	EXTERNAL_FLASH_EraseSector(beginning_of_holding_sector);	// erase the relevant holding sector
	sector_index++;
	if(sector_index > LAST_SECTOR_INDEX_FOR_MEM_HOLD)
		sector_index = FIRST_SECTOR_INDEX_FOR_MEM_HOLD;
	
	if(write_two_sectors_flag == TRUE)	// if there is another sector that needs to be written to
	{
		beginning_of_second_holding_sector = sector_index * BASE_SECTOR_SIZE;
		EXTERNAL_FLASH_EraseSector(beginning_of_second_holding_sector);	// erase the relevant holding sector
		sector_index++;
		if(sector_index > LAST_SECTOR_INDEX_FOR_MEM_HOLD)
			sector_index = FIRST_SECTOR_INDEX_FOR_MEM_HOLD;
	}
	
	//################CODE SECTION SEPARATOR#####################################
	//copy the entire current sector to the holding sector
	for( counter_address = 0 ; counter_address < BASE_SECTOR_SIZE ; counter_address += EXTERNAL_FLASH_PAGE_SIZE )
	{
		retErr = EXTERNAL_FLASH_ReadData(counter_address + beginning_of_current_sector, COPY_ARRAY, EXTERNAL_FLASH_PAGE_SIZE);	// read data from current sector
		if(retErr!=PASS)
			return retErr;
		
		retErr = EXTERNAL_FLASH_PageProgram(counter_address + beginning_of_holding_sector, EXTERNAL_FLASH_PAGE_SIZE, COPY_ARRAY);        // write to the holding sector the copied data
		if(retErr!=PASS)
			return retErr;
		
		if(write_two_sectors_flag == TRUE)	// if there is another sector that needs to be written to
		{
			retErr = EXTERNAL_FLASH_ReadData(counter_address + beginning_of_second_sector, COPY_ARRAY, EXTERNAL_FLASH_PAGE_SIZE);	// read data from current sector
			if(retErr!=PASS)
				return retErr;
			
			retErr = EXTERNAL_FLASH_PageProgram(counter_address + beginning_of_second_holding_sector, EXTERNAL_FLASH_PAGE_SIZE, COPY_ARRAY);        // write to the holding sector the copied data
			if(retErr!=PASS)
				return retErr;
		}
	}
	
	//################CODE SECTION SEPARATOR#####################################
	// erase the sectors to which we need to write
	EXTERNAL_FLASH_EraseSector(beginning_of_current_sector);	// erase the current sector
	if(write_two_sectors_flag == TRUE)	// if there is another sector that needs to be written to
		EXTERNAL_FLASH_EraseSector(beginning_of_second_sector);	// erase the second sector
	
	//################CODE SECTION SEPARATOR#####################################
	//Calculate relative addresses
	relative_address = received_address % BASE_SECTOR_SIZE;	// calculate the relative address within the sector of the received address. For example, since the sector size is 0x1000 then whether the received address is 0x0300 (sector 0) or 0x1300 (sector 1), the relative address is 0x0300
	//relative_page_index = relative_address / EXTERNAL_FLASH_PAGE_SIZE;	// calculate the relative page index within the sector of the received address. For example, since the sector size is 0x1000 and the page size is 256 then whether the received address is 0x0300 (sector 0) or 0x1300 (sector 1), the relative page index is 1 (index count starts from 0)
	//relative_address_within_page = relative_address % EXTERNAL_FLASH_PAGE_SIZE; // calculate the relative address within a page of the received address. For example, since the page size is 256 then whether the received address is 0x0300 or 0x1300, the relative address within the page is 44 (300-256 = 44)
	
	//Copy back from the holding sector the data up until the received address and write it back to the current sector
	for( counter_address = 0 ; counter_address < relative_address ; )
	{
		if( (relative_address - counter_address) >= EXTERNAL_FLASH_PAGE_SIZE)	// if more than EXTERNAL_FLASH_PAGE_SIZE bytes are left until the received address
		{
			retErr = EXTERNAL_FLASH_ReadData(counter_address + beginning_of_holding_sector, COPY_ARRAY, EXTERNAL_FLASH_PAGE_SIZE);	// read data from holding sector
			if(retErr!=PASS)
				return retErr;
			
			retErr = EXTERNAL_FLASH_PageProgram(counter_address + beginning_of_current_sector, EXTERNAL_FLASH_PAGE_SIZE, COPY_ARRAY);        // write to the current sector the copied data
			if(retErr!=PASS)
				return retErr;
			
			counter_address += EXTERNAL_FLASH_PAGE_SIZE; // increment the counter
		}
		else 	// if less than EXTERNAL_FLASH_PAGE_SIZE bytes are left until the received address
		{
			bytes_until_received_address = (uint16_t)(relative_address - counter_address);	// how many bytes are left to write until reaching the relative address within the sector from the current counter_address.
			retErr = EXTERNAL_FLASH_ReadData(counter_address + beginning_of_holding_sector, COPY_ARRAY, bytes_until_received_address);	// read data from holding sector
			if(retErr!=PASS)
				return retErr;
			
			retErr = EXTERNAL_FLASH_PageProgram(counter_address + beginning_of_current_sector, bytes_until_received_address, COPY_ARRAY);        // write to the current sector the copied data
			if(retErr!=PASS)
				return retErr;
			
			counter_address += bytes_until_received_address; // increment the counter
		}
	}
	
	//################CODE SECTION SEPARATOR#####################################
	// Write the new data to the memory starting from the received address: 
  buffer_pointer = data; 
  buff_len_for_calc = data_length;
  
  bytes_until_end_of_page = (uint16_t)(EXTERNAL_FLASH_PAGE_SIZE - (received_address % EXTERNAL_FLASH_PAGE_SIZE));  // number of bytes until the end of the current page where "received_address" is located
  if( buff_len_for_calc <= bytes_until_end_of_page )   // if the number of bytes to write fits within the current page
  {
    retErr = EXTERNAL_FLASH_PageProgram(received_address, buff_len_for_calc, buffer_pointer);        // write to the flash the number of bytes required
    //buffer_pointer += buff_len_for_calc;  // increment the pointer by the number of bytes written to the flash
    //received_address += buff_len_for_calc; // adjust the received_address to be equal to the next place we need to write data to (the beginning of the next page)
    buff_len_for_calc = 0;       // decrease the size of the data to be written by the amount just written
  }
  else  // if there is an overflow beyond the first page.
  {
    retErr = EXTERNAL_FLASH_PageProgram(received_address, bytes_until_end_of_page, buffer_pointer);        // write to the flash until the end of the page
    buffer_pointer += bytes_until_end_of_page;  // increment the pointer by the number of bytes written to the flash
    received_address += bytes_until_end_of_page; // adjust the received_address to be equal to the next place we need to write data to (the beginning of the next page)
    buff_len_for_calc -= bytes_until_end_of_page;       // decrease the size of the data to be written by the amount just written
  }

  for( ; buff_len_for_calc!=0 ;  )
  {
      if(buff_len_for_calc <= EXTERNAL_FLASH_PAGE_SIZE)   // if the remaining bytes to write are less than a full EXTERNAL_FLASH_PAGE_SIZE
      {
        retErr = EXTERNAL_FLASH_PageProgram(received_address, buff_len_for_calc, buffer_pointer); 
        break;  // end of writing to flash
      }
      else        // send maximum possible with page program command (256 bytes)
      {
        retErr = EXTERNAL_FLASH_PageProgram(received_address, EXTERNAL_FLASH_PAGE_SIZE, buffer_pointer);
        if(retErr!=PASS)
          return retErr;
        buffer_pointer += EXTERNAL_FLASH_PAGE_SIZE;    // increment the pointer by the amount we just wrote to the flash
        received_address += EXTERNAL_FLASH_PAGE_SIZE; // adjust the received_address to be equal to the next place we need to write data to (the beginning of the next page)
        buff_len_for_calc -= EXTERNAL_FLASH_PAGE_SIZE;       // decrease the size of the data to be written by the amount just written
      }
  }
	
	//################CODE SECTION SEPARATOR#####################################
	//Calculate relative addresses
	relative_address = end_address_of_data % BASE_SECTOR_SIZE;	// calculate the relative address within the sector of the end_address_of_data. For example, since the sector size is 0x1000 then whether the end_address_of_data is 0x0300 (sector 0) or 0x1300 (sector 1), the relative address is 0x0300
	//relative_page_index = relative_address / EXTERNAL_FLASH_PAGE_SIZE;	// calculate the relative page index within the sector of the end_address_of_data. For example, since the sector size is 0x1000 and the page size is 256 then whether the end_address_of_data is 0x0300 (sector 0) or 0x1300 (sector 1), the relative page index is 1 (index count starts from 0)
	//relative_address_within_page = relative_address % EXTERNAL_FLASH_PAGE_SIZE; // calculate the relative address within a page of the end_address_of_data. For example, since the page size is 256 then whether the end_address_of_data is 0x0300 or 0x1300, the relative address within the page is 44 (300-256 = 44)
	
	//Copy back from the holding sector the data starting from the end of where the data was written until the end of the sector and write it back to the current sector
	if(write_two_sectors_flag == TRUE)	// if there is another sector that needs to be written to
	{
		holding_sector_address = beginning_of_second_holding_sector;
		write_sector_address = beginning_of_second_sector;
	}
	else // There is not another sector to be written to
	{
		holding_sector_address = beginning_of_holding_sector;
		write_sector_address = beginning_of_current_sector;
	}
	
	for( counter_address = relative_address ; counter_address < BASE_SECTOR_SIZE ; /*counter_address += EXTERNAL_FLASH_PAGE_SIZE*/ )
	{
		if( counter_address == relative_address)	// if it's the first iteration of this "for" loop
		{
			bytes_until_end_of_page = (uint16_t)(EXTERNAL_FLASH_PAGE_SIZE - (end_address_of_data % EXTERNAL_FLASH_PAGE_SIZE));  // number of bytes until the end of the current page where "end_address_of_data" is located
	
			retErr = EXTERNAL_FLASH_ReadData(counter_address + holding_sector_address, COPY_ARRAY, bytes_until_end_of_page);	// read data from holding sector
			if(retErr!=PASS)
				return retErr;
			
			retErr = EXTERNAL_FLASH_PageProgram(counter_address + write_sector_address, bytes_until_end_of_page, COPY_ARRAY);        // write to the current sector the copied data
			if(retErr!=PASS)
				return retErr;
			
			counter_address += bytes_until_end_of_page;	// increment the counter
		}
		else 	// if this is not the first iteration of this "for" loop
		{
			retErr = EXTERNAL_FLASH_ReadData(counter_address + holding_sector_address, COPY_ARRAY, EXTERNAL_FLASH_PAGE_SIZE);	// read data from holding sector
			if(retErr!=PASS)
				return retErr;
			
			retErr = EXTERNAL_FLASH_PageProgram(counter_address + write_sector_address, EXTERNAL_FLASH_PAGE_SIZE, COPY_ARRAY);        // write to the current sector the copied data
			if(retErr!=PASS)
				return retErr;
			
			counter_address += EXTERNAL_FLASH_PAGE_SIZE;	// increment the counter
		}
	}
  
  return retErr;
}

/*******************************************************************
*   @Function: EXTERNAL_FLASH_PageProgram
*   @Description: Perform Page Program command - write between 1-255 bytes to the flash within a specific page. 
                 The program operation must be within the page limits. The memory is divided into equal chuncks of 256 bytes called pages.
                 If the operation is not within the page limits (begin at one page and finish program at the other page) then what will happen
                 is that the program operation will program until the end of the page and then loop back to the beginning of the page and continue
                 programming from there (see datasheet for page program). 
                 **NOTE: The maximum number of bytes is 255 and not 256 because the cypress 
*   @Input argumments: uint32_t address - start address of memory
                      uint16_t data_length - amount of bytes to write (1 to 255 bytes). 
                      uint8_t *data - pointer to array of data
*   @Output argumments: uint32_t retErr -  Error code
*******************************************************************/
uint32_t EXTERNAL_FLASH_PageProgram(uint32_t address, uint16_t data_length, uint8_t *data)
{
    uint32_t retErr=PASS;
    uint16_t index, buff_len;
    static uint8_t WriteBuffer[MAX_SPI_BUFFER_LENGTH]; // has to be set as static in order for the function to work.

        
    // Set Command byte.
    WriteBuffer[0] = PP_REG;
    buff_len = 1u;
        
    /* Address bytes */
    WriteBuffer[1] = (uint8_t) (address >> 16u);
    WriteBuffer[2] = (uint8_t) (address >> 8u);
    WriteBuffer[3] = (uint8_t) (address);
    buff_len += 3u;
        
		// Prepare ReadBuffer for transmition with dummy bytes.
		for(index = 0; index < data_length; index++)
		{
				WriteBuffer[buff_len + index] = data[index];
		}
		buff_len += data_length;
		
		// Set WEL (Write Enable Latch) bit
		retErr=EXTERNAL_FLASH_EnableWriting();
    
		if(retErr==PASS)
		{
			// Set MEM_CS pin to Low
			EXT_FLASH_CS_LOW(); 
		}			
    
    if(retErr==PASS)
    {
        // Send data to slave. Write dummy content so MOSI is busy while MISO takes data.
       retErr = HAL_SPI_Transmit(&hspi1, WriteBuffer, buff_len, SPI_EXT_FLASH_POLLING_TIMEOUT);
    }
		
		// Set MEM_CS pin to High
		EXT_FLASH_CS_HIGH();
		
		// wait until the flash is no longer busy and only then continue (or until a timeout occurs)
    retErr = EXTERNAL_FLASH_WaitUntilFlashNotBusy();
		
		if(retErr!=PASS)
    {
        #ifdef FLASH_LOGGER
            printf("\n\n\rEXTERNAL_FLASH_WriteData: Error during writing data to flash procedure, return code is: %x\n\r",retErr);     
        #endif
        ;
    }
    
    return retErr;
}


/*******************************************************************
*   @Function: FLASH_ReadData (updated 16.08.2018 by Eyal Gerber)
*   @Description: Perform Fast Read Command - It is possible to read the ENTIRE flash with just this command. However, currently, it is possible with 
                 this function only to read 256 bytes maximum. This is because the function was originally written in a certain way to support even less
                 bytes but I (Eyal) improved it a bit to support 256 bytes without making drastic changes due to time limitations and due to actual necessity.
*   @Input argumments: uint32_t address - start address of memory
                      uint8_t *data - pointer to array of data
                      uint16_t data_length - amount of bytes to write (1 to 255 bytes). 
*   @Output argumments: uint32_t retErr -  Error code
*******************************************************************/
uint32_t EXTERNAL_FLASH_ReadData(uint32_t address, uint8_t *data, uint16_t data_length)
{
    uint32_t retErr=PASS;
    uint16_t index = 0;
    static uint8_t WriteBuffer[MAX_SPI_BUFFER_LENGTH];	// has to be set as static in order for the function to work.
		static uint8_t rxData[MAX_SPI_BUFFER_LENGTH];				// has to be set as static in order for the function to work.
    uint16_t buff_len; //it has to be uint16_t and not uint8_t because it can contain a value of 256.
		uint8_t command_bytes_num = 5;   // number of bytes used for the fast read command (including dummy bytes)
    
    // Set Command byte.
    WriteBuffer[0] = FAST_READ_REG;
    buff_len = 1u;
        
    /* Address bytes */
    WriteBuffer[1] = (uint8_t) (address >> 16u);
    WriteBuffer[2] = (uint8_t) (address >> 8u);
    WriteBuffer[3] = (uint8_t) (address);
    buff_len += 3u;
    
    // Add another dummy byte for "FAST READ" command
    WriteBuffer[4] = 0u;
    buff_len += 1u;
        
    // Prepare ReadBuffer for transmition with dummy bytes.
    for(index = 0; index < data_length; index++)
        WriteBuffer[buff_len + index] = 0u;
        
    buff_len += data_length;
    
    // Set MEM_CS pin to Low
    EXT_FLASH_CS_LOW(); 
    
		// Send data to slave. Write dummy content, so MOSI is busy while MISO takes data.
		retErr = HAL_SPI_TransmitReceive(&hspi1, WriteBuffer, rxData, buff_len, SPI_EXT_FLASH_POLLING_TIMEOUT);
     
		// Set MEM_CS pin to High
    EXT_FLASH_CS_HIGH();                
    
		for(index = 0 ; index < data_length ; index++ )
			data[index] = rxData[index + command_bytes_num];	// take the received data and put it in the pointer to the receive array. Take the data received only after the command_bytes were sent.
			 
		// wait until the flash is no longer busy and only then continue (or until a timeout occurs)
    retErr = EXTERNAL_FLASH_WaitUntilFlashNotBusy();
		
		if(retErr!=PASS)
    {
        #ifdef FLASH_LOGGER
            printf("\n\n\rEXTERNAL_FLASH_ReadData: Error during reading data to flash procedure, return code is: %x\n\r",retErr);     
        #endif
        ; 
    }

    return retErr;
}

/**
  * @brief  This function erases a sector (4096 bytes) based on the address it receives. The sector is determined by the address.
						So if the address is within a specific sector, then that sector will be erased (all values will become 0xFF)
  * @param  None
  * @retval uint32_t - error code
  */
uint32_t EXTERNAL_FLASH_EraseSector(uint32_t address)
{
    uint32_t retErr=PASS;   
    uint32_t buff_len;
    uint8_t WriteBuffer[4];
 
    // Set Sector Erase Command byte.
    WriteBuffer[0] = SE_REG;
    buff_len = 1u;
	
    /* Address bytes */
    WriteBuffer[1] = (uint8_t) (address >> 16u);
    WriteBuffer[2] = (uint8_t) (address >> 8u);
    WriteBuffer[3] = (uint8_t) (address);
    buff_len += 3u;
	
    // Set WEL (Write Enable Latch) bit
    retErr=EXTERNAL_FLASH_EnableWriting();
 
    if(retErr==PASS)
    {
        // Set MEM_CS pin to Low
        EXT_FLASH_CS_LOW();                  
    }
    
    if(retErr==PASS)
    {
        // Send data to slave. 
        retErr = HAL_SPI_Transmit(&hspi1, WriteBuffer, buff_len, SPI_EXT_FLASH_POLLING_TIMEOUT);  
    }
  
    // Set MEM_CS pin to High
    EXT_FLASH_CS_HIGH();                    
    
		// wait until the flash is no longer busy and only then continue (or until a timeout occurs)
    retErr = EXTERNAL_FLASH_WaitUntilFlashNotBusy();
		
		if(retErr!=PASS)
    {
        #ifdef FLASH_LOGGER
            printf("\n\n\rEXTERNAL_FLASH_EraseBlock: Error during erasing data to flash procedure, return code is: %x\n\r",retErr);     
        #endif
        ;  
    }
    
    return retErr;
}

/**
  * @brief  This function erases a block (32K bytes) based on the address it receives. The block is determined by the address.
						So if the address is within a specific block, then that block will be erased (all values will become 0xFF)
  * @param  None
  * @retval uint32_t - error code
  */
uint32_t FLASH_EraseBlock(uint32_t address)
{    
    uint32_t buff_len;
    uint8_t WriteBuffer[4];
    uint32_t retErr=PASS;

    // Set BlockErase Command byte.
    WriteBuffer[0] = BE32K;
    buff_len = 1u;
	
    /* Address bytes */
    WriteBuffer[1] = (uint8_t) (address >> 16u);
    WriteBuffer[2] = (uint8_t) (address >> 8u);
    WriteBuffer[3] = (uint8_t) (address);
    buff_len += 3u;
	
    // Set WEL (Write Enable Latch) bit
    retErr=EXTERNAL_FLASH_EnableWriting();
    
    
    if(retErr==PASS)
    {
        // Set MEM_CS pin to Low
        EXT_FLASH_CS_LOW();                
    }
    
    if(retErr==PASS)
    {
        // Send data to slave. 
        retErr = HAL_SPI_Transmit(&hspi1, WriteBuffer, buff_len, SPI_EXT_FLASH_POLLING_TIMEOUT);  
    }
    
		// Set MEM_CS pin to High
    EXT_FLASH_CS_HIGH();                    
    
    // wait until the flash is no longer busy and only then continue (or until a timeout occurs)
    retErr = EXTERNAL_FLASH_WaitUntilFlashNotBusy();
		
		if(retErr!=PASS)
    {
        #ifdef FLASH_LOGGER
            printf("EXTERNAL_FLASH_EraseBlock: Error during erasing data to flash procedure, return code is: 0x%x\n\r",retErr);     
        #endif
        ;
    }

    return retErr;
}

/**
  * @brief  This function erases the entire flash memory (all values will become 0xFF). This operation takes several seconds to complete. 
  * @param  None
  * @retval uint32_t - error code
  */
uint32_t FLASH_EraseAll(void)
{ 
    uint32_t retErr=PASS; 
		uint32_t buff_len;
    uint8_t WriteBuffer[1];
    
		// Set BlockErase Command byte.
    WriteBuffer[0] = CE_REG_SPI;
    buff_len = 1u;
	
    // Set WEL bit to high
    retErr=EXTERNAL_FLASH_EnableWriting();
    
    if(retErr==PASS)
    {       
        // Set MEM_CS pin to Low
        EXT_FLASH_CS_LOW(); 
    }
    
    if(retErr==PASS)
    {
			// Send Chip Erase command 
      retErr = HAL_SPI_Transmit(&hspi1, WriteBuffer, buff_len, SPI_EXT_FLASH_POLLING_TIMEOUT);  
    }
		
		// Set MEM_CS pin to High
    EXT_FLASH_CS_HIGH(); 
    
    // wait until the flash is no longer busy and only then continue (or until a timeout occurs)
    retErr = EXTERNAL_FLASH_WaitUntilFlashNotBusy();
		
		if(retErr!=PASS)
    {
        #ifdef FLASH_LOGGER
            printf("EXTERNAL_FLASH_EraseAll: Error during erase all flash , return code is: 0x%x\n\r",retErr);     
        #endif
				;
    }  
    
    return SUCCESS;
}



/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/



