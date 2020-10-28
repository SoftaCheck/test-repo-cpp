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
  * @file           : common.c
  * @brief          : This file contains functions related to general elements 
                      and functions that are used by all and could not be 
                      categorized in a dedicated file. 
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "common.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
 
/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/

/**
  * @brief  This function disables interrupts in the system
  * @param  None
  * @retval None
  */
void Disable_Interrupts(void)
{
  __disable_irq();
}

/**
  * @brief  This function enables interrupts in the system
  * @param  None
  * @retval None
  */
void Enable_Interrupts(void)
{
  __enable_irq();
}


/**
  * @brief  This function performs a system reset
  * @param  None
  * @retval None
  */
void SystemReset(void)
{
  HAL_NVIC_SystemReset();
}

/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/
