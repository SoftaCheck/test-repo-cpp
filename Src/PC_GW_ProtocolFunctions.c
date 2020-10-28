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
  * @file           : PC_GW_ProtocolFunctions.c
  * @brief          : This is a user application layer file for the relevant 
											functions of the protocol between the PC or the main controller
											and the GW
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "PC_GW_ProtocolFunctions.h"
#include "main.h"
#include "modbus_app_layer.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

//CRYP_HandleTypeDef hcryp;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/



/***********************************************************

Function name: SignNewNode

Function type: void

Arguments: uint32_t UniqueNodeID

Return: none

Description: This function is responsible for assigning a 
place and node ID for a new node. The node ID is a number
between 0 and 49 and it will be coresponding to the unique
ID of the node, until this node delete, by the user.
This function will store the unique node ID in the flash,
based on a free space and with priority to the front places
(for example, if node 7 was deleted and and node 9 is the last
occupied place, the new node will assign to place 7 and not 
place 10)

**********************************************************/
void SignNewNode(uint32_t UniqueNodeID)
{
	
}



/***********************************************************

Function name: RemoveOldNode

Function type: void

Arguments: uint8_t NodeID

Return: none

Description: This function is responsible for removing an
old node from the nodes registering area and also delete 
all of its data from the memory map.

**********************************************************/
void RemoveOldNode(uint8_t NodeID)
{
	
}


/***********************************************************

Function name: AddingRemovingNodeAnalyze

Function type: uint8_t

Arguments: uint8_t *IncomingArr

Return: uint8_t val2Ret

Description: This function is responsible for receiving 
the data came over MD if it was for the nodes registering
area in the GW memory area and check if a new nodes was 
checked in and if so, if the command is legal or if an 
old nodes removed and now all of its data in flash needs to 
be erased.

**********************************************************/
uint8_t AddingRemovingNodeAnalyze(uint8_t *IncomingArr)
{
	uint32_t Register2Write = ((uint32_t)((uint16_t)((IncomingArr[INCOMING_UART_ARR_LOC_2]<<BYTE_SHIFT) & 0xFF00) + (uint32_t)IncomingArr[INCOMING_UART_ARR_LOC_3]));		//The number of register, to calculate the node number of which this action will apply
	uint8_t SlotID = ((Register2Write - NB_OF_NODES_REGISTERING_REGISTERS)/2) TOTAL_NUMBER_OF_NODES
	if(IncomingArr[LOCATION_OF_NB_OF_BYTES] == SIZE_OF_UNIQUE_ID_OF_SLOT)
	{
		if(IncomingArr[PAYLOAD_BYTE_NB_0] == 0 && IncomingArr[PAYLOAD_BYTE_NB_1] == 0 && IncomingArr[PAYLOAD_BYTE_NB_2] == 0 && IncomingArr[PAYLOAD_BYTE_NB_3] == 0)		//If the command was to remove an existing node
		{
			
		}
	}
}


/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/