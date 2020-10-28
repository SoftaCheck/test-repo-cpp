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
  * @file           : watchdog.c
  * @brief          : This is a user application driver for the watchdog
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "watchdog.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

//IWDG_HandleTypeDef hiwdg;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/

/**
  * @brief  This function reconfigures the period of the watchdog timer
  * @param  uint8_t seconds - the period in seconds (maximum value is 28)
  * @retval None
  */
void WD_Period_Set(uint8_t seconds)   // max value of seconds is around ~28 seconds.
{
  uint32_t uwLsiFreq = LSI_FREQUENCY;   // it is equal ~39Khz according to the datasheet (but there can be a serious drift ob +4% and -10% and the nominal value can range between 25Khz and 50Khz.
                                // it would be wise to verify the value of the LSI for each MCU using the GetLSIFrequency() function and call it every now and then (for example every 5 minutes).
                                // Calling GetLSIFrequency is a good thing also for the RTC since it also sources its clock from the LSI and in order for the RTC to be accurate such a periodic calibration would be wise.
  
  /*### Configure the IWDG peripheral ######################################*/
  /* Example for calculation of setting counter reload value to obtain 20s IWDG TimeOut.
     IWDG counter clock Frequency = LsiFreq / 32
     Counter Reload Value = 20s / IWDG counter clock period
                          = 20s / (256/LsiFreq)
                          = (LsiFreq * 20) / (256) */
  
  hiwdg.Instance = IWDG;

  hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
  hiwdg.Init.Reload    = (uwLsiFreq * (uint32_t)seconds) / 256;
	hiwdg.Init.Window = 4095;	// set it to 4095 to disable window mode.

  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    /* Initialization Error */
    ;//Status_Handler(WATCHDOG_INIT_ERROR);
  }
 
}

/**
  * @brief  This function reloads the timer of the iWatchdog
  * @param  None
  * @retval None
  */
void WD_Reload()
{
    HAL_IWDG_Refresh(&hiwdg);
}

/**
  * @brief  This function checks if the program woke up due to a watchdog reset event
  * @param  None
  * @retval None
  */
void Check_WD_Reset()
{
  // Check if the system has resumed from IWDG reset
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)  
  {
    //Clear reset flags
    __HAL_RCC_CLEAR_RESET_FLAGS();
		
    #ifdef WATCHDOG_LOGS
      printf("Watchdog event occured!\r\n");
    #endif
    //Error_Handler();	// Eyal Gerber (17.08.2018): Don't go to error handler in case of WD event. This application does not have a human safety consideration. If it can continue to run after reset, let it. 
  }
}

/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/
