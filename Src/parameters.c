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
  * @file           : parameters.c
  * @brief          : This is a user application driver for managing the parameters of the program
  * @Instructions   : How to add new parameters to the code:
                      1. Add the new parameter to the parameters enum in the header file
                      2. Increment the PARAM_NUM definition in the header file
                      3. Add a new definition for the default value of the new parameter in the header file
                      4. Add a new assignment of the value to the default values array in the UploadDefaultSystemParameters() function
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "parameters.h"
#include "eeprom.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
 
static Parameter PARAMS[PARAM_NUM] = {0};             // A local array that holds all the parameters from the EEPROM for easy access to their values. 
static uint32_t DEFAULT_PARAMS[PARAM_NUM] = {0};      // A local array that holds all the default values for the parameters

/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/

/**
  * @brief  This function initializes the system parameters array by populating it with the parameters from the EEPROM
  * @param  None
  * @retval None
  */
void Init_SystemParameters(void)
{
  uint32_t index = 0;
  
  #ifdef PARAMETERS_INDICATIONS
    printf("Init_SystemParameters() function\n\r");
  #endif
  
  //assigning the address for each parameter (all parameters have a fixed length of 32 bits - one word)  
  for(index = 0 ; index < PARAM_NUM ; index++ )
  {
    PARAMS[index].ee_adrs = PARAMETERS_RELATIVE_ADDRESS_START + index*PARAM_SIZE;
  }
  
  if( EEPROM_READ_WORD( PARAMS[FIRST_TIME_RUN_FLAG_DEFAULT].ee_adrs ) == (uint32_t)ERASED_CELL )       //if this is the first time the system is running
  {
    UploadDefaultSystemParameters();      // upload the default system parameters to the eeprom
    Param_SetVal((parameters)FIRST_TIME_RUN_FLAG_DEFAULT,(uint32_t)FALSE);  // unset the flag
  }
  else
    Download_SystemParameters();  // populate the parameters array with values from the eeprom
}

/**
  * @brief  This function retrieves the value of a parameter from the parameters array. 
            CRITICAL: It is assumed that the value has already been downloaded to the array from the eeprom
                      during initialization so that it won't be necessary to read from the eeprom all the time during run time.
  * @param  parameters param_index - the index of the parameter
  * @retval uint32_t - the value of the parameter
  */
uint32_t Param_GetVal(parameters param_index)
{
  return PARAMS[param_index].val;
}

/**
  * @brief  This function sets the value of the parameter
  * @param  parameters param_index - the index of the parameter
            uint32_t value - the value of the parameter to set
  * @retval None
  */
void Param_SetVal(parameters param_index, uint32_t value)
{
  EEPROM_WRITE_WORD( PARAMS[param_index].ee_adrs, value );      // write the value to the eeprom
  PARAMS[param_index].val = EEPROM_READ_WORD( PARAMS[param_index].ee_adrs );    // get the written value from the eeprom and assign it to the parameters array
}

/**
  * @brief  This function assigns the values to the default parameters array and 
            uploads the default parameters to the EEPROM
  * @param  None
  * @retval None
  */
void UploadDefaultSystemParameters(void)
{
  uint32_t index = 0;
  
  #ifdef PARAMETERS_INDICATIONS
    printf("UploadDefaultSystemParameters() function\n\r");
  #endif
  
  //Assignment of defualt values to the array
  DEFAULT_PARAMS[ENABLE_FIRMWARE_UPGRADE] = ENABLE_FIRMWARE_UPGRADE_DEFAULT;
	DEFAULT_PARAMS[BR_COM_INTERVAL] = BR_COM_INTERVAL_DEF;
	DEFAULT_PARAMS[REPEAT_MESSAGES] = REPEAT_MESSAGES_DEF;
	DEFAULT_PARAMS[RESPONSE_TIMEOUT] = RESPONSE_TIMEOUT_DEF;
	DEFAULT_PARAMS[MIN_RAND] = MIN_RAND_DEF;
	DEFAULT_PARAMS[MAX_RAND] = MAX_RAND_DEF;
	DEFAULT_PARAMS[ALARM_WAKEUP_INTERVAL] = ALARM_WAKEUP_INTERVAL_DEF;
	
	
  for(index = 0 ; index < PARAM_NUM ; index++ )
  {
    Param_SetVal( (parameters)index , DEFAULT_PARAMS[index] );  // when setting the values in the eeprom they are also assigned to the parameters array
    
    #ifdef PARAMETERS_INDICATIONS
      printf("DEFAULT_PARAMS[%d] = %d ,PARAMS[%d].val = %d , PARAMS[%d].ee_adrs = %d \n\r",index,DEFAULT_PARAMS[index],index,PARAMS[index].val,index,PARAMS[index].ee_adrs);
    #endif
  }
}

/**
  * @brief  This function downloads the parameters that are written in the EEPROM and saves them in the parameters array
  * @param  None
  * @retval None
  */
void Download_SystemParameters(void)
{
  uint32_t index = 0; 
  
  #ifdef PARAMETERS_INDICATIONS
    printf("Download_SystemParameters() function\n\r");
  #endif
  
  // retrieving the values from the memory
  for(index = 0 ; index < PARAM_NUM ; index++ )
  {
    PARAMS[index].val = EEPROM_READ_WORD( PARAMS[index].ee_adrs );
    
    #ifdef PARAMETERS_INDICATIONS
      printf("PARAMS[%d].val = %d , PARAMS[%d].ee_adrs = %d \n\r",index,PARAMS[index].val,index,PARAMS[index].ee_adrs);
    #endif
  }
}



void SetParams2Default(uint8_t NodeID)
{
	uint8_t ParamsTempArr[20] = {0};
	ParamsTempArr[0] =(uint8_t)((BR_COM_INTERVAL_DEF & 0xFF000000)>>24);
	ParamsTempArr[1] =(uint8_t)((BR_COM_INTERVAL_DEF & 0x00FF0000) >> 16);
	ParamsTempArr[2] =(uint8_t)((BR_COM_INTERVAL_DEF & 0x0000FF00) >> 8);
	ParamsTempArr[3] =(uint8_t)((BR_COM_INTERVAL_DEF) & 0x000000FF);
	
	
	for(uint8_t i=0;i<20;i++)
	printf("ParamsTempArr[%d] = %x\n\r", i, ParamsTempArr[i]);
	
}

/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/
