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
  * @file           : GW_BRProtocolsImp.c
  * @brief          : This file contains functions related to the implemenation of the
											communication protocol between the gateway and the bridge. 
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "GW_BRProtocolsImp.h"
#include "gpio.h"
#include "modbus_app_layer.h"
#include "main.h"
#include "modbus_driver.h"
#include "cc1101.h"
#include "app_main.h"
#include "encryption.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define QUERY_GW2BR_MESSAGE_CODE 		0x31u
#define QUERY_BR2GW1_MESSAGE_CODE 	0x32u
#define QUERY_BR2GW2_MESSAGE_CODE 	0x33u
#define KEEP_ALIVE_MESSAGE_CODE 		0x11u
#define COMMUNICATION_OK 						0x01u
#define COMMUNICATION_NOT_OK 				0x00u
#define ACK_CODE_MESSAGE						0x21u
#define ACK													0xAAu

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
 
/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/


uint8_t CurrentNodeID = GW_AVALIABLE_4_COMM;			//This variable is the NodeID of the board that we currently speak to. When the communication is done, its value will set to 0xFF

/***********************************************************

Function name: GetDataAndSizeOfQuery

Function type: uint8_t

Arguments: uint8_t *QueryData

Return: QuerySize

Description: This function is responsible for receiving the 
data of the Query and return its size.

**********************************************************/
uint8_t GetDataAndSizeOfQuery(uint8_t *QueryData)
{
//	#ifdef GENERAL_INDICATIONS
//	printf("\n\r\n\rThe address to read from is: %x\n\r\n\r", add2ReadFromFlash(CurrentNodeID, QUERY_ADDRESS_WITHIN_NODE_MEM_SPACE));
//	#endif
	uint8_t QueryLengthArr[2] = {0};
	EXTERNAL_FLASH_ReadData(add2ReadFromFlash(CurrentNodeID, QUERY_SIZE_LOCATION_WITHIN_NODE_MEM_SPACE), QueryLengthArr, 2);	// 2 bytes are allocated to the register QUERY_SIZE_LOCATION_WITHIN_NODE_MEM_SPACE
	printf("\n\r\n\r The size as read from flash at address %x is: %x %x \n\r\n\r",add2ReadFromFlash(CurrentNodeID, QUERY_SIZE_LOCATION_WITHIN_NODE_MEM_SPACE) , QueryLengthArr[0], QueryLengthArr[1]);
	EXTERNAL_FLASH_ReadData(add2ReadFromFlash(CurrentNodeID, QUERY_ADDRESS_WITHIN_NODE_MEM_SPACE), QueryData, QueryLengthArr[1]);	//the query length is actually just 8 bits so that's why we only refer to QueryLengthArr[0]
	for(uint8_t i=0;i<QueryLengthArr[1];i++)
		printf("And the data at %d is: %x\n\r", i, QueryData[i]);
	return QueryLengthArr[1];
}




/***********************************************************

Function name: QueryResponseAnalys

Function type: uint8_t

Arguments: uint8_t *IncomingDataArr

Return: uint8_t. Return OK or not, depend on the type
of the response received.

Description: This function is responsible for sending the 
Query from the GW to the BR, for AIO Sensor. The query is
sent over the air and via SPI and RF.

**********************************************************/

uint8_t QueryResponseAnaylas(uint8_t *IncomingDataArr)
{
	if(IncomingDataArr[0] == QUERY_BR2GW1_MESSAGE_CODE)
	{
		return COMMUNICATION_OK;
	}
	else if(IncomingDataArr[0] == QUERY_BR2GW2_MESSAGE_CODE)
	{
		return COMMUNICATION_NOT_OK;
	}
	else
		return COMMUNICATION_NOT_OK;
}




/***********************************************************

Function name: ACK_SendRF

Function type: void

Arguments: none

Return: none

Description: This function is responsible for sending ACK
over the RF

**********************************************************/

void ACK_SendRF(void)
{
	#ifdef RF_INDICATIONS
	printf("\n\rACK message - to BR\n\r\n\r");
	#endif
	uint8_t ACKArr2Send[ACK_MESSAGE_SIZE] = {0};		//Init the
	ACKArr2Send[ACK_ARR_LOC_0] = ACK_CODE_MESSAGE;
	ACKArr2Send[ACK_ARR_LOC_1] = GATEWAY_ID;		//The node ID of the GW.
	ACKArr2Send[ACK_ARR_LOC_2] = ACK;
	
	uint16_t OutgoingRFCRC = CalcCRC(ACKArr2Send, ACK_MESSAGE_SIZE-2);
	
	ACKArr2Send[ACK_ARR_LOC_3] = (OutgoingRFCRC>>8)&0x00FF;
	ACKArr2Send[ACK_ARR_LOC_4] = (OutgoingRFCRC)&0x00FF;
	
	sendDataOverRF(ACKArr2Send, ACK_MESSAGE_SIZE);
}



/***********************************************************

Function name: NACK_SendRF

Function type: void

Arguments: none

Return: none

Description: This function is responsible for sending NACK
over the RF

**********************************************************/

void NACK_SendRF(void)
{
	#ifdef RF_INDICATIONS
	printf("\n\rNACK message - to BR\n\r\n\r");
	#endif
	uint8_t NACKArr2Send[NACK_MESSAGE_SIZE] = {0};		//Init the arr
	NACKArr2Send[NACK_ARR_LOC_0] = NACK_CODE_MESSAGE;
	NACKArr2Send[NACK_ARR_LOC_1] = GATEWAY_ID;
	NACKArr2Send[NACK_ARR_LOC_2] = NACK;
	
	uint16_t OutgoingRFCRC = CalcCRC(NACKArr2Send, NACK_MESSAGE_SIZE-2);
	
	NACKArr2Send[ACK_ARR_LOC_3] = (OutgoingRFCRC>>8)&0x00FF;
	NACKArr2Send[ACK_ARR_LOC_4] = (OutgoingRFCRC)&0x00FF;
	
	sendDataOverRF(NACKArr2Send, NACK_MESSAGE_SIZE);
}




/***********************************************************

Function name: GW2BR_QuerySendRF

Function type: void

Arguments: none

Return: none

Description: This function is responsible for sendingQuert
from GW to bridge over the RF

**********************************************************/
void GW2BR_QuerySendRF(void)
{
	#ifdef RF_INDICATIONS
	printf("\n\rQuery message - to BR\n\r");
	#endif
	uint8_t QueryArr2Send[QUERY_GW2BR_MAX_SIZE] = {0};		//Init the array
	uint8_t tempArr[QUERY_GW2BR_MAX_SIZE] = {0};					//Init a temp array for receiving the query data
	QueryArr2Send[QUERY_GW2BR_ARR_LOC_0] = QUERY_GW2BR_MESSAGE_CODE;
	QueryArr2Send[QUERY_GW2BR_ARR_LOC_1] = GATEWAY_ID;
	QueryArr2Send[QUERY_GW2BR_ARR_LOC_2] = GetDataAndSizeOfQuery(tempArr);			//Get the data and its size;
	
	for(uint8_t i=0;i< QueryArr2Send[QUERY_GW2BR_ARR_LOC_2];i++)			//Assign the data of the query to the array to send
		QueryArr2Send[QUERY_GW2BR_ARR_LOC_3+i] = tempArr[i];
	
	uint16_t OutgoingRFCRC = CalcCRC(QueryArr2Send, (QUERY_GW2BR_ARR_LOC_3+QueryArr2Send[QUERY_GW2BR_ARR_LOC_2]));
	
	QueryArr2Send[QUERY_GW2BR_ARR_LOC_3+QueryArr2Send[QUERY_GW2BR_ARR_LOC_2]+1] = (OutgoingRFCRC>>8)&0x00FF;
	QueryArr2Send[QUERY_GW2BR_ARR_LOC_3+QueryArr2Send[QUERY_GW2BR_ARR_LOC_2]+2] = (OutgoingRFCRC)&0x00FF;
	
	UnsetNewDataFlag4Node(CurrentNodeID, QUERY);
	sendDataOverRF(QueryArr2Send, QueryArr2Send[QUERY_GW2BR_ARR_LOC_2]+QUERY_GW2BR_EXTRA_DATA);		//send the data and its size
	
}


/***********************************************************

Function name: RebootCommand_SendRF

Function type: void

Arguments: none

Return: none

Description: This function is responsible for sending 
 the reboot command to Bridge. 

**********************************************************/
void RebootCommand_SendRF(void)
{
	#ifdef RF_INDICATIONS
	printf("\n\rReboot message - to BR\n\r");
	#endif
	uint8_t RebootCommandArr2Send[REBOOT_COMMAND_MESSAGE_SIZE] = {0};		//Init the array to  be sent
	RebootCommandArr2Send[REBOOT_COMMAND_ARR_LOC_0] = REBOOT_COMMAND_MESSAGE_CODE;
	RebootCommandArr2Send[REBOOT_COMMAND_ARR_LOC_1] = CurrentNodeID;
	RebootCommandArr2Send[REBOOT_COMMAND_ARR_LOC_2] = 0;

	uint16_t OutgoingRFCRC = CalcCRC(RebootCommandArr2Send, REBOOT_COMMAND_MESSAGE_SIZE-2);
	
	RebootCommandArr2Send[REBOOT_COMMAND_ARR_LOC_3] = (OutgoingRFCRC>>8)&0x00FF;
	RebootCommandArr2Send[REBOOT_COMMAND_ARR_LOC_4] = (OutgoingRFCRC)&0x00FF;
	
	sendDataOverRF(RebootCommandArr2Send, REBOOT_COMMAND_MESSAGE_SIZE);
}



/***********************************************************

Function name: RebootAndDefCommand_SendRF

Function type: void

Arguments: none

Return: none

Description: This function is responsible for sending 
 the reboot and set params to default command to Bridge. 

**********************************************************/
void RebootAndDefCommand_SendRF(void)
{
	#ifdef RF_INDICATIONS
	printf("\n\rReboot and default message - to BR\n\r");
	#endif
	uint8_t RebootCommandArr2Send[REBOOT_COMMAND_MESSAGE_SIZE] = {0};		//Init the array to  be sent
	RebootCommandArr2Send[REBOOT_COMMAND_ARR_LOC_0] = REBOOT_COMMAND_MESSAGE_CODE;
	RebootCommandArr2Send[REBOOT_COMMAND_ARR_LOC_1] = CurrentNodeID;
	RebootCommandArr2Send[REBOOT_COMMAND_ARR_LOC_2] = 1;

	uint16_t OutgoingRFCRC = CalcCRC(RebootCommandArr2Send, REBOOT_COMMAND_MESSAGE_SIZE-2);
	
	RebootCommandArr2Send[REBOOT_COMMAND_ARR_LOC_3] = (OutgoingRFCRC>>8)&0x00FF;
	RebootCommandArr2Send[REBOOT_COMMAND_ARR_LOC_4] = (OutgoingRFCRC)&0x00FF;
	
	sendDataOverRF(RebootCommandArr2Send, REBOOT_COMMAND_MESSAGE_SIZE);
}
	


/***********************************************************

Function name: SetParams_GW2BRSendRF

Function type: void

Arguments: none

Return: none

Description: This function is responsible for sending 
 the Set parameters command to the Bridge. 

**********************************************************/
void SetParams_GW2BRSendRF(void)
{
	uint8_t SetParamsArr2Send[SET_PARAMS_MESSAGE_SIZE] = {0}; 	//Init the array
	SetParamsArr2Send[SET_PARAMS_ARR_LOC_0] = SET_PARAMS_MESSAGE_CODE;
	SetParamsArr2Send[SET_PARAMS_ARR_LOC_1] = CurrentNodeID;
	GetDataOfParamsFromFlash(SetParamsArr2Send+2);
	uint16_t OutgoingRFCRC = CalcCRC(SetParamsArr2Send, SET_PARAMS_MESSAGE_SIZE-2);
	
	SetParamsArr2Send[SET_PARAMS_ARR_LOC_18] = (OutgoingRFCRC>>8)&0x00FF;
	SetParamsArr2Send[SET_PARAMS_ARR_LOC_19] = (OutgoingRFCRC)&0x00FF;
	printf("\n\r\n\rThe value of set parameters are:\n\r\n\r");
	
	for(uint8_t i=0;i<SET_PARAMS_MESSAGE_SIZE-2;i++)
		printf("SetParamsArr2Send[%d] = %x\n\r", i, SetParamsArr2Send[i]); 
	
	UnsetNewDataFlag4Node(CurrentNodeID, SET_PARAMS);
	sendDataOverRF(SetParamsArr2Send, SET_PARAMS_MESSAGE_SIZE);
}


/***********************************************************

Function name: GetDataOfParamsFromFlash

Function type: void

Arguments: none

Return: none

Description: This function is responsible readignt the 
values of parameters of set params from flash.
**********************************************************/

void GetDataOfParamsFromFlash(uint8_t *Arr2write)
{
	uint32_t add2read;
	uint8_t brComIntervalTemp[4] = {0};
	uint8_t responseTimeoutTemp[2] = {0};
	uint8_t repeatMessageTemp[2] = {0};
	uint8_t minRandTemp[2] = {0};
	uint8_t maxRandTemp[2] = {0};
	uint8_t sensorTimeoutTemp[2] = {0};
	uint8_t alarmWakeupIntervalTemp[4] = {0};
	
	uint8_t i=0, j=0;
	
	add2read = (MEM_SPACE_BYTE_NUM_FOR_EACH_NODE * CurrentNodeID) + GW_MODBUS_REGISTERS_START_ADRS+(NUMBER_OF_REGS_FOR_GW*REGISTER_BYTE_NUM) + BR_COM_INTERVAL_LOCATION_WITHIN_NODE_MEM_SPACE;
	printf("\n\r\n\rThe address to read the first params is: %x\n\r\n\r", add2read);
	EXTERNAL_FLASH_ReadData(add2read, brComIntervalTemp, 4);
	
	for(i=0; j< 4;i++, j++)
	{
		Arr2write[i] = brComIntervalTemp[j];
	}
	
	add2read = (MEM_SPACE_BYTE_NUM_FOR_EACH_NODE * CurrentNodeID) + GW_MODBUS_REGISTERS_START_ADRS+(NUMBER_OF_REGS_FOR_GW*REGISTER_BYTE_NUM) + RESPONSE_TIMEOUT_LOCATION_WITHIN_NODE_MEM_SPACE; 
	EXTERNAL_FLASH_ReadData(add2read, responseTimeoutTemp, 2);
	
	j=0;
	for(; j< 2;i++, j++)
	{
		Arr2write[i] = responseTimeoutTemp[j];
	}
	
	add2read = (MEM_SPACE_BYTE_NUM_FOR_EACH_NODE * CurrentNodeID) + GW_MODBUS_REGISTERS_START_ADRS+(NUMBER_OF_REGS_FOR_GW*REGISTER_BYTE_NUM) + REPEAT_MESSAGES_LOCATION_WITHIN_NODE_MEM_SPACE; 
	EXTERNAL_FLASH_ReadData(add2read, repeatMessageTemp, 2);
	
	j=0;
	for(; j< 2;i++, j++)
	{
		Arr2write[i] = repeatMessageTemp[j];
	}
	
	add2read = (MEM_SPACE_BYTE_NUM_FOR_EACH_NODE * CurrentNodeID) + GW_MODBUS_REGISTERS_START_ADRS+(NUMBER_OF_REGS_FOR_GW*REGISTER_BYTE_NUM) + MIN_RAND_LOCATION_WITHIN_NODE_MEM_SPACE; 	
	EXTERNAL_FLASH_ReadData(add2read, minRandTemp, 2);
	
	j=0;
	for(; i< 2;i++, j++)
		Arr2write[i] = sensorTimeoutTemp[j];
		
		add2read = (MEM_SPACE_BYTE_NUM_FOR_EACH_NODE * CurrentNodeID) + GW_MODBUS_REGISTERS_START_ADRS+(NUMBER_OF_REGS_FOR_GW*REGISTER_BYTE_NUM) + MAX_RAND_LOCATION_WITHIN_NODE_MEM_SPACE; 	
	EXTERNAL_FLASH_ReadData(add2read, maxRandTemp, 2);
	
	for(j=0; i< 2;i++, j++)
		Arr2write[i] = sensorTimeoutTemp[j];
	
	add2read = (MEM_SPACE_BYTE_NUM_FOR_EACH_NODE * CurrentNodeID) + GW_MODBUS_REGISTERS_START_ADRS+(NUMBER_OF_REGS_FOR_GW*REGISTER_BYTE_NUM) + SENSOR_TIMEOUT_LOCATION_WITHIN_NODE_MEM_SPACE; 	
	EXTERNAL_FLASH_ReadData(add2read, sensorTimeoutTemp, 2);
	
	for(j=0; i< 2;i++, j++)
		Arr2write[i] = sensorTimeoutTemp[j];
	
	add2read = (MEM_SPACE_BYTE_NUM_FOR_EACH_NODE * CurrentNodeID) + GW_MODBUS_REGISTERS_START_ADRS+(NUMBER_OF_REGS_FOR_GW*REGISTER_BYTE_NUM) + ALARM_WAKEUP_INTERVAL_LOCATION_WITHIN_NODE_MEM_SPACE; 	
	EXTERNAL_FLASH_ReadData(add2read, alarmWakeupIntervalTemp, 4);
	
	for(j=0; i< 4;i++, j++)
		Arr2write[i] = alarmWakeupIntervalTemp[j];
	
	
}


/***********************************************************

Function name: add2ReadFromFlash

Function type: uint32_t

Arguments: uint8_t NodeID, uint8_t parameter

Return: add2ret

Description: This function is responsible for calculating 
the address to read from flash, for the current node

**********************************************************/
uint32_t add2ReadFromFlash(uint8_t NodeID, uint16_t parameter)
{
	uint32_t add2ret;
	add2ret = MEM_SPACE_BYTE_NUM_FOR_EACH_NODE * NodeID + (parameter) + GW_MODBUS_REGISTERS_START_ADRS+400; 		
	//printf("\n\r\n\rThe address to read from4 is : %x\n\r\n\r", add2ret);
	return add2ret;
}


/***********************************************************

Function name: RF_SendCommand

Function type: void

Arguments: uint8_t command

Return: none

Description: This function is responsible for sending all of
the commands from the bridge to the GW

**********************************************************/
void RF_SendCommand(uint8_t command)
{
	switch(command)
	{
		case ACK_COMMAND:
			ACK_SendRF();
			break;
		case NACK_COMMAND:
			NACK_SendRF();
			break;
		case RES1_2_MB_QUE:
			break;
		case RES2_2_MB_QUE:
			break;
		case RES_2_GET_PARAM:
			break;
		case QUERY_GW2BR_SEND_COMMAND:
			GW2BR_QuerySendRF();
			break;
		case SET_PARAMETERS:
			SetParams_GW2BRSendRF();
			break;
		case REBOOT_COMMAND:
			RebootCommand_SendRF();
			break;
		case REBOOT_AND_DEF_COMMAND:
			RebootAndDefCommand_SendRF();
			break;
	}
}



/***********************************************************

Function name: RFIncomingData4BRAnalays

Function type: void

Arguments: uint8_t *arr2analyze

Return: none

Description: This function is responsible for analays the 
incoming data over RF and figure out, if the data is for 
BR and not for the sensor, which command is that and what is 
the correct response.

**********************************************************/
uint8_t ACK2SendFlag = FALSE;						//This flag is set to TRUE if the current response to the node should be ACK.
uint8_t GetParams2SendFlag = FALSE;			//This flag is set to TRUE if the current response to the node should be get parameters.
uint8_t Query2SendFlag = FALSE;

void RFIncomingData4BRAnalays(uint8_t *arr2analyze)
{
	uint8_t MessageCode = arr2analyze[MESSAGE_CODE_LOC_IN_RF_MES];			//Assign the code of the incoming data
	uint8_t NodeId = arr2analyze[NODE_ID_LOC_IN_RF_MES];			//Assign the Node ID of the bridge that sent this message
	switch(MessageCode)
	{
		case KEEP_ALIVE_MESSAGE_CODE:
			CurrentNodeID = NodeId;				//Lock the GW for this Node only, until ACK from the GW to this node.
			#ifdef GENERAL_INDICATIONS
			printf("\n\rNew keep alive message arrived\n\r");
			#endif
			KeepAliveResponseHandler(arr2analyze, NodeId);
			break;
		case NACK_CODE_MESSAGE:
			break;
		case ACK_CODE_MESSAGE:
			ACKResponseHandler();
			break;
		case QUERY_BR2GW1_MESSAGE_CODE:
			QueryResp1_ResponseHnadler(arr2analyze, NodeId);
			break;
		case QUERY_BR2GW2_MESSAGE_CODE:
			#ifdef GENERAL_INDICATIONS
			printf("\n\rQuery response 2 came in\n\r");
			#endif
			QueryResp2_ResponseHnadler(arr2analyze, NodeId);
			break;
			
		default:
		   printf("Unsupported command  %d\n\r", MessageCode);
	}
}


/***********************************************************

Function name: ACKResponseHandler

Function type: void

Arguments: None

Return: none

Description: This function is responsible for analays the 
incoming ACK message and send back the correct response

**********************************************************/

void ACKResponseHandler(void)
{
	#ifdef RF_INDICATIONS
	printf("\n\rACK message, from BR\n\r");
	#endif
	if(CheckIfNewData4Node(CurrentNodeID, SET_PARAMS))		//If there's some new data fot this node
	{
		SetParamsRF_Flag = TRUE;
		state = RF_TRANSMIT_STATE;
	}
	else if(CheckIfReset() == 1)
	{
		RESET_RF_Flag = TRUE;
		state = RF_TRANSMIT_STATE;
	}
	else if(CheckIfReset() == 2)
	{
		RESET_AndDef_RF_Flag = TRUE;
		state = RF_TRANSMIT_STATE;
	}
	else
	{
		ACK_RF_Flag = TRUE;		
		state = RF_TRANSMIT_STATE;
	}
}


/***********************************************************

Function name: CheckIfReset

Function type: uint8_t

Arguments: None

Return: none

Description: This function is responsible for checking in
flash if a reset for this node is required, and if so, is 
it a regular reset or also with setting the parmas values 
to default. 

**********************************************************/
uint8_t CheckIfReset(void)
{
	uint8_t val2ret = 0;
	uint8_t tempArr[4];
	uint32_t RESET_Address = GW_MODBUS_REGISTERS_START_ADRS + 400 + CurrentNodeID*MEM_SPACE_BYTE_NUM_FOR_EACH_NODE + RESET_REMOTE_UNIT_LOCATION_WITHIN_NODE_MEM_SPACE;
	EXTERNAL_FLASH_ReadData(RESET_Address, tempArr, 4);
	printf("\n\r\n\rThe value of reset\n\r");
	for(uint8_t i=0;i<4;i++)
		printf("tempArr[%d] = %x\n\r",i, tempArr[i]);
	
	if(tempArr[1] == 1)
		val2ret = 1;
	else
		val2ret = 0;
	if(tempArr[3] == 1)
			val2ret = 2;
	else if(tempArr[1] != 1)
		val2ret = 0;
	
	return val2ret;
}


/***********************************************************

Function name: UnsetResetBit

Function type: void

Arguments: None

Return: none

Description: This function is responsible for erasing the 
bit of reset of the current node

**********************************************************/

void UnsetResetBit(void)
{
	uint32_t RESET_Address = GW_MODBUS_REGISTERS_START_ADRS + 400 + CurrentNodeID*MEM_SPACE_BYTE_NUM_FOR_EACH_NODE + RESET_REMOTE_UNIT_LOCATION_WITHIN_NODE_MEM_SPACE;
	uint8_t tempArr[2] = {0};
	EXTERNAL_FLASH_WriteData_Automatic(RESET_Address, 2, tempArr);
}




/***********************************************************

Function name: UnsetResetAndDefBit

Function type: void

Arguments: None

Return: none

Description: This function is responsible for erasing the 
bit of reset and set params to default of the current node

**********************************************************/

void UnsetResetAndDefBit(void)
{
	uint32_t RESET_Address = GW_MODBUS_REGISTERS_START_ADRS + 400 + CurrentNodeID*MEM_SPACE_BYTE_NUM_FOR_EACH_NODE + RST_2_DEFAULT_LOCATION_WITHIN_NODE_MEM_SPACE;
	uint8_t tempArr[2] = {0};
	EXTERNAL_FLASH_WriteData_Automatic(RESET_Address, 2, tempArr);
}


/***********************************************************

Function name: QueryResp1_ResponseHnadler

Function type: void

Arguments: uint8_t *arr2Analyze, uint8_t NodeID

Return: none

Description: This function is responsible for analays the 
incoming response 1 of MB query message and return the correct
answer.

**********************************************************/

void QueryResp1_ResponseHnadler(uint8_t *arr2Analyze, uint8_t NodeID)
{
	#ifdef RF_INDICATIONS
		printf("\n\rQuery response 1, from BR came in\n\r");
	#endif
	/*Store the data in the dedicated place in the flash*/
	uint32_t Address = (uint32_t)(NodeID*MEM_SPACE_BYTE_NUM_FOR_EACH_NODE+QUERY_RESPONSE_SIZE_LOCATION_WITHIN_NODE_MEM_SPACE+GW_MODBUS_REGISTERS_START_ADRS + 400);
	printf("The address is: %x\n\r", Address);
//	printf("The base address is: %x\n\r", START_OF_NODES_MEM_AREA_ADRS);
//	printf("The address of query response is: %x\n\r", Address);
//	printf("The size of data to write is: %d\n\r", arr2Analyze[5]*2);
//	printf("The node ID is: %d\n\r", NodeID);
	
	uint8_t data2Write2Flash[QUERY_BR2GW1_MAX_SIZE+1] = {0};			//The +1 is for the size of this data that should also be saved in the flash.
	data2Write2Flash[0] = 0;
	data2Write2Flash[1] = (arr2Analyze[5]*2)+2;				//Add the size of this response of the data to write to flash.
	for(uint8_t i=0;i<(arr2Analyze[5]*2)+2;i++)			//The +2 is for the two bytes of the CRC
		data2Write2Flash[i+2] = arr2Analyze[i+3];
	
	EXTERNAL_FLASH_WriteData_Automatic(Address, (arr2Analyze[5]*2)+4, data2Write2Flash);		//+4 is for the size and two bytes of the CRC
	
	Wait4QueryResponse1 = FALSE;
	
	if(CheckIfNewData4Node(NodeID, QUERY))		//If there's some new data fot this node
	{
		QueryRF_Flag = TRUE;
		state = RF_TRANSMIT_STATE;
	}
	else if(CheckIfNewData4Node(NodeID, SET_PARAMS))
	{
		SetParamsRF_Flag = TRUE;
		state = RF_TRANSMIT_STATE;
	}
	else if(CheckIfReset() == 1)
	{
		RESET_RF_Flag = TRUE;
		state = RF_TRANSMIT_STATE;
	}
	else if(CheckIfReset() == 2)
	{
		RESET_AndDef_RF_Flag = TRUE;
		state = RF_TRANSMIT_STATE;
	}
	else
	{
		ACK_RF_Flag = TRUE;		
		state = RF_TRANSMIT_STATE;
	}
}




/***********************************************************

Function name: QueryResp2_ResponseHnadler

Function type: void

Arguments: uint8_t *arr2Analyze, uint8_t NodeID

Return: none

Description: This function is responsible for analays the 
incoming response 2 of MB query message and return the correct
answer.

**********************************************************/
void QueryResp2_ResponseHnadler(uint8_t *arr2Analyze, uint8_t NodeID)
{
	#ifdef RF_INDICATIONS
	printf("\n\rQuery response 2, from BR\n\r");
	#endif
	Wait4QueryResponse2 = FALSE;
	if(CheckIfNewData4Node(NodeID, QUERY))		//If there's some new data fot this node
	{
		QueryRF_Flag = TRUE;
		state = RF_TRANSMIT_STATE;		// CODE_REVIEW: Way too many uses of this statement throughout the code. Please consider reducing it substantially and using it as much as possible within MainStateMachineLoop() if possible
	}
	else if(CheckIfNewData4Node(CurrentNodeID, SET_PARAMS))
	{
		SetParamsRF_Flag = TRUE;
		state = RF_TRANSMIT_STATE;
	}
	else if(CheckIfReset() == 1)
	{
		RESET_RF_Flag = TRUE;
		state = RF_TRANSMIT_STATE;
	}
	else if(CheckIfReset() == 2)
	{
		RESET_AndDef_RF_Flag = TRUE;
		state = RF_TRANSMIT_STATE;
	}
	else
	{
		ACK_RF_Flag = TRUE;		//Just send ACK
		state = RF_TRANSMIT_STATE;
	}
}



/***********************************************************

Function name: KeepAliveResponseHandler

Function type: void

Arguments: uint8_t *arr2Analyze, uint8_t NodeID

Return: none

Description: This function is responsible for analays the 
incoming keep alive message and return the correct
answer.

**********************************************************/
void KeepAliveResponseHandler(uint8_t *arr2Analyze, uint8_t NodeID)
{
	#ifdef RF_INDICATIONS
		printf("\n\rKA message, from BR\n\r");
	#endif
	
	if(arr2Analyze[QUERY_RESPONSE_FLAF_IN_KA_MES] == 1)
	{
		Wait4QueryResponse1 = TRUE;
		Wait4QueryResponse2 = FALSE;
	}
	else
	{
		Wait4QueryResponse2 = TRUE;		
		Wait4QueryResponse1 = FALSE;
	}
	
	uint32_t Address = (uint32_t)(NodeID*MEM_SPACE_BYTE_NUM_FOR_EACH_NODE + ADDRESS_OF_PARMAS_OF_KA_WITHIN_NODE_MEM_SPACE+START_OF_NODES_MEM_AREA_ADRS);
	#ifdef GENERAL_INDICATIONS
	printf("The address of params of KA in flash is: %x\n\r", Address);
	#endif
	
	uint8_t data2Write2Flash[SIZE_OF_PARMAS_OF_KA] = {0};			//Creating a temp arr for assigning the relevant data came of KA message. The data will be set into the arr in the correct order and as in the ICD doc
	
	data2Write2Flash[RSSI_FIRST_BYTE_LOC] = 0;
	data2Write2Flash[RSSI_SECOND_BYTE_LOC] = arr2Analyze[10];
	data2Write2Flash[V_BAT_FIRST_BYTE_LOC] = arr2Analyze[6];
	data2Write2Flash[V_BAT_SECOND_BYTE_LOC] = arr2Analyze[7];
	data2Write2Flash[BR_FW_VERSION_FIRST_BYTE_LOC] = arr2Analyze[8];
	
	data2Write2Flash[BR_FW_VERSION_SECOND_BYTE_LOC] = arr2Analyze[9];
	uint32_t tempaAdress = (uint32_t)(NodeID*MEM_SPACE_BYTE_NUM_FOR_EACH_NODE+TIME_SINCE_LAST_COMM_LOCATION_WITHIN_NODE_MEM_SPACE+START_OF_NODES_MEM_AREA_ADRS);		//This is for reading the value of this parameter from flash for avoiding two seperated flash writing.
	uint8_t tempArr[NUMBER_OF_BYTES_TIME_LAST_COM]={0};
	EXTERNAL_FLASH_ReadData(tempaAdress, tempArr, NUMBER_OF_BYTES_TIME_LAST_COM);
	
	printf("The temp data read\n\r");
	for(uint8_t i=0;i<NUMBER_OF_BYTES_TIME_LAST_COM;i++)
	{
		data2Write2Flash[TIME_SINCE_LAST_COM_FIRST_BYTE_LOC+i] = tempArr[i];
		printf("Data[%d] = %x\n\r",i, tempArr[i]);
	}
	
	data2Write2Flash[ALARM_CODE_FIRST_BYTE_LOC] = 0;
	data2Write2Flash[ALARM_CODE_SECOND_BYTE_LOC] = arr2Analyze[11];
	
	tempaAdress = (uint32_t)(NodeID*MEM_SPACE_BYTE_NUM_FOR_EACH_NODE+NO_CONNECTION_TIME_LOCATION_WITHIN_NODE_MEM_SPACE+START_OF_NODES_MEM_AREA_ADRS);		//This is for reading the value of this parameter from flash for avoiding two seperated flash writing.
	
	uint8_t tempArr1[NUMBER_OF_BYTES_BETWEEN_NO_COMM_AND_TIME_DATA]={0};
	
	
	printf("The temp data 1 read\n\r");
	for(uint8_t i=0;i<NUMBER_OF_BYTES_BETWEEN_NO_COMM_AND_TIME_DATA;i++)
	{
		data2Write2Flash[NO_CONNECTION_TIME_FIRST_BYTE_LOC+i] = tempArr1[i];
		printf("Data[%d] = %x\n\r",i, tempArr1[i]);
	}
	
	for(uint8_t i=0;i<4;i++)
	{
		data2Write2Flash[TIME_DATA_FIRST_BYTE_LOC+i] = arr2Analyze[2+i];
	}
	
	
	EXTERNAL_FLASH_WriteData_Automatic(Address, SIZE_OF_PARMAS_OF_KA, data2Write2Flash);
	
	
	
	Wait4QueryResponse1 = FALSE;

	state = RF_RECEIVE_STATE;
}

/***********************************************************

Function name: GW2BRResponseHandler

Function type: void

Arguments: uint8_t response

Return: none

Description: This function is responsible for sending back over RF
the required response while communication session is ON

**********************************************************/
void GW2BRResponseHandler(void)
{
	if(ACK2SendFlag)
	{
		RF_SendCommand(ACK_COMMAND);
		CurrentNodeID = GW_AVALIABLE_4_COMM;			//Delete the node ID from the RAM
		ACK2SendFlag = FALSE;
	}
	else if(GetParams2SendFlag)
	{
		GetParams2SendFlag = FALSE;
		RF_SendCommand(SET_PARAMETERS);
	}
	else if(Query2SendFlag)
	{
		Query2SendFlag = FALSE;
		RF_SendCommand(QUERY_GW2BR_SEND_COMMAND);
	}
}


/***********************************************************

Function name: sendDataOverRF

Function type: int

Arguments: byte *inComingData, uint8_t size

Return: o or 1, depending if ACK has sent back

Description: This function is responsible for sending the 
package to a bridge over RF and receiving ACK in return.
If ACK has sent from the destenation BR, the function will
return 1, else, return 0

**********************************************************/

void sendDataOverRF(byte *inComingData, uint8_t size)
{
	CC1101_setTxState();
	uint8_t NbOfBursts;
	uint8_t PayloadInsertCounter = 0;
	if(size <= RF_ONE_PAYLOAD_MAX_SIZE)
		NbOfBursts = ONE_BURST;
	else
		NbOfBursts = (size/RF_ONE_PAYLOAD_MAX_SIZE)+1;	
	
	//printf("NbOfBursts = %d\n\r", NbOfBursts);
	
	uint8_t txBuffer[PKTLEN + 1] = {0};
	txBuffer[0] = PKTLEN;			//This is the size of one burst. The first byte in every burst

	txBuffer[1] = CurrentNodeID;
	
	txBuffer[2] = size;				//The size of the total message. The third byte in every burst.
	
	CC1101_GPIOInterruptDisable();		//Disable interrupts while CC1101 is transmitting
	HAL_NVIC_DisableIRQ(USART1_IRQn);
	for(uint8_t BurstIndex=0;BurstIndex<NbOfBursts;BurstIndex++)
	{
		
		uint8_t TempArr4Encryption1[RF_ONE_PAYLOAD_MAX_SIZE] = {0};
		uint8_t TempArr4Encryption2[RF_ONE_PAYLOAD_MAX_SIZE] = {0};		
		
		
		
		for(uint8_t i=0;i<RF_ONE_PAYLOAD_MAX_SIZE;i++)
		{
			if(PayloadInsertCounter !=  size)
			{
				TempArr4Encryption1[i] = inComingData[i+(RF_ONE_PAYLOAD_MAX_SIZE*BurstIndex)];		//Insert the 48 bytes of this round. 
				PayloadInsertCounter++;
			}
			else
				TempArr4Encryption1[i] = 0;			//Zero padding
		}
		
		MX_AES_Init();			//The call for this function here is needed. We did that after revealing that writing to flash and AES nort working together without this function call. The error handler inside this function was removed.
		Encrypt_Data(TempArr4Encryption1, RF_ONE_PAYLOAD_MAX_SIZE, TempArr4Encryption2);		//Encrypte the 48 bytes and transfer them to TempArr4Encryption2
		
		

		txBuffer[3] = PayloadInsertCounter - (BurstIndex*RF_ONE_PAYLOAD_MAX_SIZE);				//The size of the current burst. The fourth byte in every burst.
		txBuffer[4] = BurstIndex;				//The burst's index. The fifth byte in every burst.
		
		
		
		for(uint8_t j=5;j<(PKTLEN-8);j++)
		{
			txBuffer[j] = TempArr4Encryption2[j-5];
		}
		
		uint16_t OutgoingRFCRC = CalcCRC(txBuffer, PKTLEN-2);
		txBuffer[PKTLEN-1] = (OutgoingRFCRC>>8)&0x00FF;		//puting the CRC of the data that came on the UART into the last two bytes of the string for the RF to send
		txBuffer[PKTLEN-2] = (OutgoingRFCRC)&0x00FF;
		
		#ifdef GENERAL_INDICATIONS
		printf("\n\r\n\rDATA SENT OVER RF - ENCRYPTED\n\r\n\r");
		for(uint8_t k=0;k<PKTLEN + 1;k++)
			printf("Sent data[%d] = %x\n\r", k, txBuffer[k]);
		#endif
		
		CC1101_writeBurstReg(CC1101_TXFIFO, txBuffer, sizeof(txBuffer));
		
		
		CC1101_cmdStrobe(CC1101_STX);
		uint32_t Tick4LoopBreak = HAL_GetTick();
		while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) != GPIO_PIN_SET)
		{	
				//HAL_Delay(1);	
				if(HAL_GetTick() - Tick4LoopBreak > CC1101_TRANSMISSION_TIMEOUT)
				{
					printf("\n\rRF Transmission 1st Timeout Failure\n\r\n\r");
					break;
				}
		} 
		while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) != GPIO_PIN_RESET)
		{	
				//HAL_Delay(1);	
				if(HAL_GetTick() - Tick4LoopBreak > CC1101_TRANSMISSION_TIMEOUT)
				{
					printf("\n\rRF Transmission 2nd Timeout Failure\n\r\n\r");
					break;
				}
		} 
	}
	CC1101_cmdStrobe(CC1101_SFTX);
	CC1101_setRxState();
	HAL_NVIC_EnableIRQ(USART1_IRQn);
	CC1101_GPIOInterruptEnable();	// enable interrupts after the transmission has ended
	
}

/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/

