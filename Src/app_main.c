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
  * @file           : app_main.c
  * @brief          : This is where the application begins and where the application
                      main loop occurs 
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "app_main.h"

#include "gpio.h"
#include "spi.h"
#include "modbus_app_layer.h"
#include "modbus_driver.h"
#include "radio.h"
#include "GW_BRProtocolsImp.h"
#include "cc1101.h"
#include "watchdog.h"
#include "power_management.h"
#include "eeprom.h"
#include "external_flash.h"
#include "bootloader_support.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

volatile uint8_t RFReceiveTransmitFlag = FALSE;		//This flag is used to indicate if the current interrupt has occur due to receive or transmit
uint8_t BR_Mode_GW_Or_BR;	// global variable to identify the type of board (bridge or gateway) - relevant only for bridge mode
uint8_t FLAG_PushButton = FALSE;        // flag to determine whether the pushbutton has been pressed. 

/* Private function prototypes -----------------------------------------------*/

void App_Init(void);

/* Exported functions ---------------------------------------------------------*/
																		 
/**
  * @brief  This function is where the main app initializations occur
  * @param  None
  * @retval None
  */
void App_Init(void)
{
	RADIO_ERR recErr;
	
	/* Initialize all configured peripherals */
  GPIO_Init();
	
	#ifdef ENABLE_WATCH_DOG
		Check_WD_Reset();     // check if we woke up due to a watchdog reset
		WD_Period_Set(WATCHDOG_RESET_PERIOD); // define the watchdog period
	#endif
  
  #ifdef GENERAL_INDICATIONS
    printf("\r\n\r\n ##### Application Initialization ##### \r\n\r\n");
		printf("\n\r ##### Under Low Power mode, this board is a GATEWAY ##### \n\r\n\r");
		printf("\r\n\r\n ##### FW version 1.1.11 ##### \r\n\r\n");
  #endif
	
	
	recErr = Radio_Init();
	#ifdef RF_INDICATIONS
	if(recErr != RADIO_OK)
		 printf("Radio initialization/identification FAILED");
	else
		printf("Radio initialized and identified successfully\n\r");
	#endif
  
  Init_Receive_Interrupt();		//Initialized the arrays and the interrupt of UART1
	
	EXTERNAL_FLASH_Init();	// Initialize the external flash driver
}

/**
  * @brief  This function is where the main loop occurs and the application is initialized
  * @param  None
  * @retval None
  */
void App_Main()
{
  App_Init();
	
	CC1101_GPIOInterruptEnable();
		
	BIT_Tests();
	
	for(;;)
	{	
		MainStateMachineLoop(state);
		WD_Reload();								// reload the watchdog register to prevent watchdog reset event.
		
		#ifdef ENABLE_DEBUG_MODE	
			CommandHandler();	// scan for commands coming from the user (for debugging)
		#endif
		
		#ifdef ENABLE_DEBUG_MODE
			if(FLAG_PushButton == TRUE)
			{
				FLAG_PushButton = FALSE;
				system_tests();
				//HAL_Delay(4000);
			}
		#endif
	}
}




/***********************************************************

Function name: HAL_GPIO_EXTI_Callback

Function type: void

Arguments: uint16_t GPIO_Pin

Return: None

Description: This function is where the SW jumps to upon 
interrupt from an external GPIO, in our case, the GDO0 
of CC1101 and the push button

**********************************************************/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if( HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_6) == RESET ) // if the push button has been pressed. 
	{
		FLAG_PushButton = TRUE;
	}
	else if( HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13) == RESET )	// if the GDO0 pin from the CC1101 is the one that triggerred the interrupt
	{
		CC1101_GPIOInterruptDisable();
		RFReceiveTransmitFlag = RF_Routine();
		CC1101_GPIOInterruptEnable();
	}
}


/***********************************************************

Function name: CC1101_GPIOInterruptEnable

Function type: void

Arguments: None

Return: None

Description: This function is responsible to enable the
external GPIO interrupt from the CC1101

**********************************************************/
void CC1101_GPIOInterruptEnable(void)
{
	if(BOOTLOADER_PROCESS_START_FLAG == FALSE)
		HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}


/***********************************************************

Function name: CC1101_GPIOInterruptDisable

Function type: void

Arguments: None

Return: None

Description: This function is responsible to disable the
external GPIO interrupt

**********************************************************/
void CC1101_GPIOInterruptDisable(void)
{
	HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);
}

/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/
