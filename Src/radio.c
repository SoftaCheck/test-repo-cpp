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
  * @file           : radio.c
  * @brief          : This file contains functions related to dealing with the RF
											peripheral.
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"
#include "cc1101.h"
#include "common.h"
#include "spi.h"
#include "radio.h"
#include "ccpacket.h"
#include "GW_BRProtocolsImp.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
 
 uint8_t RF_RXBuffer[RF_ALL_PAYLOADS_MAX_SIZE] = {0};
 uint8_t RF_RXBufferSize = 0;
 
/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/

RADIO_ERR Radio_Init(void)
{	
	uint8_t freq = CFREQ_868;
	CC1101(-1); 
	uint8_t mode = 0;
	CC1101_init(freq, mode);	
	HAL_Delay(1);
		
	if (CC1101_IsExist() == 0)
		 return RADIO_NOT_CONNECTED;
	
	CC1101_SettingsFromMSP430();
	printf("Config ok\n\r");
	return RADIO_OK;
}

/***********************************************************

Function name: RF_Routine

Function type: uint8_t

Arguments: None

Return: ret_err

Description: This function is responsible for checking if
any data came over RF and receiving it.

**********************************************************/
uint8_t RF_Routine(void)
{
  uint8_t ret_err = 0;
	ret_err = CC1101_receiveData(RF_RXBuffer, &RF_RXBufferSize);
	//ExternalGPIOInterruptEnable();	//Eyal Gerber (16.08.2018): line disabled. The interrupt should be enabled at the exit of the callback function for better readability
  return ret_err;
}



/***********************************************************

Function name: EndOfReceiveRF

Function type: void

Arguments: None

Return: None

Description: This function is responsible for managing the 
timeout of the RF receive routine.

**********************************************************/
void EndOfReceiveRF(void)
{
	if(StartRXFlag)		//If we are in the middle of receive routine.
	{
		if(HAL_GetTick() - timeSample4RFRecevingTimeout > RF_TIMEOUT)		//If the time has passed
		{
			CC1101_ReceiveRoutineFlagsInit();
		}
	}
}

/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/
