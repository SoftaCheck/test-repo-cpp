/* ========================================
 *
 * Copyright AIO SYSTEMS, 2018
 * Written by Novodes for AIO SYSTEMS
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * ========================================
*/
/**
  ******************************************************************************
  * @file           : receive_interrupt.c
  * @brief          : This file contains functions related to the reception of 
                      data via the uart and handling them. 
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "logger.h"
#include "receive_interrupt.h"
#include "State_machine.h"
#include "modbus_app_layer.h"

/* Private typedef -----------------------------------------------------------*/


/* Private define ------------------------------------------------------------*/

//################################################
//definitions for the logger channel
#define LOGGER_MESSAGE_END_BASED_BYTE          // enable this to identify the end of a received message based on a single byte (e.g. 0x0D or 13 decimal for "enter" carriage return in ASCII)
//#define LOGGER_MESSAGE_END_BASED_TIME       // enable this to identify the end of a received message based on a defined time period after the last received byte

#ifdef LOGGER_MESSAGE_END_BASED_BYTE
  #define LOGGER_MESSAGE_END_BYTE      13      // 13 = carriage return (enter) in ascii // the byte received that represents the end of the message/command
#endif
#ifdef  LOGGER_MESSAGE_END_BASED_TIME
  #define LOGGER_MESSAGE_END_TIME      10      // [msec] time period after the last received byte which defines the end of the reception of the current message/command
#endif

//################################################
//definitions for the main controller (MC) channel
//#define MC_MESSAGE_END_BASED_BYTE          // enable this to identify the end of a received message based on a single byte (e.g. 0x0D or 13 decimal for "enter" carriage return in ASCII)
#define MC_MESSAGE_END_BASED_TIME       // enable this to identify the end of a received message based on a defined time period after the last received byte

#ifdef MC_MESSAGE_END_BASED_BYTE
  #define MC_MESSAGE_END_BYTE      13      // 13 = carriage return (enter) in ascii // the byte received that represents the end of the message/command
#endif
#ifdef  MC_MESSAGE_END_BASED_TIME
  #define MC_MESSAGE_END_TIME      10      // [msec] time period after the last received byte which defines the end of the reception of the current message/command
#endif

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

volatile uint8_t logger_command_buffer[LOGGER_COMMAND_BUFFER_SIZE];       // the buffer that accumulates the bytes received
volatile uint8_t mc_command_buffer[MC_COMMAND_BUFFER_SIZE];       // the buffer that accumulates the bytes received
uint8_t logger_command_data[LOGGER_COMMAND_DATA_SIZE];  // the temporary array that holds only one byte each time it is received and later transfered to the logger command buffer
uint8_t mc_command_data[MC_COMMAND_DATA_SIZE];	//temporary array that holds only one byte each time it is received and later transfered to the MC command buffer
volatile uint16_t mc_Rx_index = 0; // the current index in the command buffer to which to add the next received byte
volatile uint16_t logger_Rx_index = 0; // the current index in the command buffer to which to add the next received byte
volatile uint8_t UARTStartReceiveFlag = FALSE;
#ifdef MC_MESSAGE_END_BASED_TIME
	volatile uint8_t mc_UART_Timeout_Flag = FALSE;
	volatile uint8_t mc_dataReceivedFlag = FALSE;
#endif
#ifdef LOGGER_MESSAGE_END_BASED_TIME
	volatile uint8_t logger_UART_Timeout_Flag = FALSE;
	volatile uint8_t logger_dataReceivedFlag = FALSE;
#endif
#ifdef LOGGER_MESSAGE_END_BASED_BYTE
  volatile uint8_t logger_end_message_flag = FALSE;	// indicates that it's the end of the message
#endif
uint8_t AfterInitFlag = FALSE;
uint8_t dataReceivedFlag1 = FALSE, dataReceivedFlag2 = FALSE;
volatile uint8_t mc_missed_byte_flag = FALSE;
volatile uint8_t logger_missed_byte_flag = FALSE;
volatile uint32_t Time4UARTReceiving = 0;

	
/***********************************************************

Function name: Init_Receive_Interrupt

Function type: void

Arguments: None

Return: None

Description: This function is for initiating the reception 
of commands over the UART so that the commands from the 
user could be received. This function should be called 
upon at the intialization stage of the program so that 
reception is ready for commands. 

**********************************************************/
void Init_Receive_Interrupt()
{
    uint8_t index;
        
    for (index=0;index<MC_COMMAND_BUFFER_SIZE;index++)
		{
      mc_command_buffer[index]=0;	// clear mc_command_buffer before receiving new data
			logger_command_buffer[index]=0;	// clear mc_command_buffer before receiving new data
		}

    for (index=0;index<MC_COMMAND_DATA_SIZE;index++)
		{
      mc_command_data[index]=0;	// clear command_data before receiving new data
			logger_command_data[index]=0;	// clear command_data before receiving new data
		}
	
		HAL_UART_Receive_IT(&huart1, mc_command_data, 1);   // activate receive  ##FOR NEW BOARD BRINGUP: CHANGE THE UART HANDLE HERE BASED ON THE UART THAT IS USED FOR RECEIVING COMMANDS##
		HAL_UART_Receive_IT(&hlpuart1, logger_command_data, 1);   // activate receive  ##FOR NEW BOARD BRINGUP: CHANGE THE UART HANDLE HERE BASED ON THE UART THAT IS USED FOR RECEIVING COMMANDS##
}

/**
  * @brief  This function is the callback function for the interrupt of the UART
            in case of reception of data
  * @param  UART_HandleTypeDef *huart
  * @retval None
  */
/***********************************************************

Function name: HAL_UART_RxCpltCallback

Function type: void

Arguments: UART_HandleTypeDef *huart

Return: None

Description: This function is the callback function for 
the interrupt of the UART in case of reception of data

**********************************************************/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

		state = UART_RECEIVE_STATE;
 
    if(huart->Instance == MAIN_CONTROLLER_CHANNEL)   //All the various UART receive interrupts get sent to this callback function so we need to verify that the interrupt source is from the relevant UART.
    {
			#ifdef MC_MESSAGE_END_BASED_TIME
				if (mc_UART_Timeout_Flag == FALSE)    //if there is still no timeout. In case there is a timeout, this variable is unset outside of this callback function but from the moment the flag is set until it is unset, we must not allow to receive anymore data in order not to overide the data in the array before it has been saved.
				{
					mc_dataReceivedFlag = TRUE;
					
					mc_command_buffer[mc_Rx_index]= mc_command_data[0];  // store data in buffer
					mc_Rx_index++;
				}
				else
					mc_missed_byte_flag = TRUE;	// indication that we received a byte that wasn't saved in the buffer
			#endif
			HAL_UART_Receive_IT(&huart1, mc_command_data, 1);   // activate receive  ##FOR NEW BOARD BRINGUP: CHANGE THE UART HANDLE HERE BASED ON THE UART THAT IS USED FOR RECEIVING COMMANDS##
    }
		else 
		{
			if(huart->Instance == LOGGER_COMMAND_HANDLER_UART) //All the various UART receive interrupts get sent to this callback function so we need to verify that the interrupt source is from the relevant UART.
			{
				#ifdef LOGGER_MESSAGE_END_BASED_BYTE
					if (logger_command_data[0]!=LOGGER_MESSAGE_END_BYTE)    //if received data other than the message end byte
					{
						if(logger_end_message_flag == FALSE)	// if the flag is unset. If it is set it meanst that we received an END_BYTE and we haven't yet processed the buffer and, therefore, we shouldn't save more data on the buffer
						{
							logger_command_buffer[logger_Rx_index] = logger_command_data[0];  // store data in buffer
							logger_Rx_index++;
						}
					}
					else    // it's the end of the message
						logger_end_message_flag = TRUE;
				#endif
				#ifdef LOGGER_MESSAGE_END_BASED_TIME
					logger_dataReceivedFlag = TRUE;
					
					if (logger_UART_Timeout_Flag == FALSE)    //if there is still no timeout. In case there is a timeout, this variable is unset outside of this callback function but from the moment the flag is set until it is unset, we must not allow to receive anymore data in order not to overide the data in the array before it has been saved.
					{
							logger_command_buffer[logger_Rx_index] = logger_command_data[0];  // store data in buffer
							logger_Rx_index++;
					}
					else
						logger_missed_byte_flag = TRUE;	// indication that we received a byte that wasn't saved in the buffer
				#endif
				HAL_UART_Receive_IT(&hlpuart1, logger_command_data, 1);   // activate receive  ##FOR NEW BOARD BRINGUP: CHANGE THE UART HANDLE HERE BASED ON THE UART THAT IS USED FOR RECEIVING COMMANDS##
			}
		}
}


/***********************************************************

Function name: EndOfReceiveUART

Function type: void

Arguments: None

Return: None

Description: This function is for dealing with the data that
came over UART. This function decides whether the incoming 
string is over or not and transfer the data to a dedicated array

**********************************************************/
void EndOfReceiveUART(void)
{
	
	static uint8_t receivedDataModbus[MAX_MODBUS_SLAVE_COMMAND_SIZE] = {0};	// define as static in order to prevent the MCU from re-allocating memory to this huge array ALL THE TIME
	
	static uint16_t i; 
	
	if(mc_dataReceivedFlag == TRUE)	// if some data has been received (not necessarily an entire command yet)
	{
		Time4UARTReceiving = HAL_GetTick();		//Sample the time
		mc_dataReceivedFlag = FALSE;
		UARTStartReceiveFlag = TRUE;
	}
	else
	{
		if( ((HAL_GetTick() - Time4UARTReceiving) > MC_MESSAGE_END_TIME) && (UARTStartReceiveFlag == TRUE) )	// if an entire command has been received (presumed to be an entire command due to the timeout that passed)
		{
			UARTStartReceiveFlag = FALSE;
			
			mc_UART_Timeout_Flag = TRUE;	// set flag to indicate that we are now saving the data to our local buffer (future improvement: do this on the fly and not only when the timeout expires or even better to do this with DMA)
			
			for(i=0;i<mc_Rx_index;i++)
				receivedDataModbus[i] = mc_command_buffer[i];
			
			mc_UART_Timeout_Flag = FALSE;	// unset the flag to indicate that the saving of the data to our local buffer has completed. 
			
			for( /*continue from last point*/ ; i<MAX_MODBUS_SLAVE_COMMAND_SIZE ; i++)	
				receivedDataModbus[i] = 0; // add zero padding to the buffer
			
			if(mc_missed_byte_flag == TRUE)	// have we missed any received bytes and not stored in them in the buffer
			{
				#ifdef USART1_INDICATIONS
					printf("\r\nUSART1 Error: Missed receiving at least one byte!r\n");
				#endif
				mc_missed_byte_flag = FALSE;	// reset the flag
			}
			
			IncomingDataAnalysis(receivedDataModbus, mc_Rx_index);
	
			mc_Rx_index = 0;       // reset the index to point to the beginning of the mc_command_buffer array
		}
	}
	
}

#ifdef ENABLE_DEBUG_MODE	// if debug mode is disabled, then all this code is not compiled, saving precious flash space

/**
  * @brief  This function handles the reception of commands via the logger channel and analyzes and executes those commands
  * @param  None
  * @retval None
  */
void CommandHandler(void)
{
	uint8_t index;
	static uint8_t received_command[LOGGER_COMMAND_BUFFER_SIZE] = {0};
	
	if(logger_end_message_flag == TRUE)
	{	
		for( index = 0 ; index < logger_Rx_index ; index++ )
				received_command[index] = logger_command_buffer[index];
		
		logger_Rx_index = 0;
		
		logger_end_message_flag = FALSE; // unset the flag to indicate that the saving of the data to our local buffer has completed. 
		
		Command_Analyzer(received_command);
		state = RF_RECEIVE_STATE;			// CODE_REVIEW: I believe this is wrong to put this here. Why should be transition states due to command handler. The command handler is not part of the state machine and does not affect the transition of states... Consider masking this line.
	}
}

#endif //ENABLE_DEBUG_MODE





/************************ (C) COPYRIGHT AIO SYSTEMS *****END OF FILE****/
