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
  * @file           : modbus_app_layer.c
  * @brief          : This file contains functions related to modbus operation from
											the applicatin layer. This means that the functions here are specific
											to this application
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/

#include "modbus_app_layer.h"
#include "modbus_driver.h"
#include "GW_BRProtocolsImp.h"
#include "bootloader_support.h"
#include "cc1101.h"
#include "PC_GW_ProtocolFunctions.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
 
NodeFlag ArrOfFlags4Nodes[TOTAL_NUMBER_OF_NODES] = {0};			//This array is array of flags for all of the nodes that is connected to this GW. These flags will be use to indicate if there's a new data for a specific NodeID

/* Private function prototypes -----------------------------------------------*/

void Write_NodeParameters_CRC(uint8_t *arr2Analyze);
void Firmware_Update_Parsing(uint8_t *arr2Analyze);
void Update_FUTS_Register(Firmware_Upgrade_Transfer_Status upgrade_status);
Firmware_Upgrade_Transfer_Status Read_FUTS_Register(void);
uint16_t Save_DataChunk_ToExternalFlash(uint8_t *arr2Analyze);
uint8_t Check_ReceivedFirmwareImage_CRC(uint8_t *arr2Analyze);
	
/* Exported functions ---------------------------------------------------------*/


/***********************************************************

Function name: IncomingDataAnalysis

Function type: void

Arguments: uint8_t *arr2Analyze

Return: 0

Description: This function is responsible for understanding 
the correct modbus commad out of the incoming data

**********************************************************/
void IncomingDataAnalysis(uint8_t *arr2Analyze, uint8_t arrSize)
{
	byte functionCode = arr2Analyze[1];
	uint8_t retErr = FAIL;	// whether the modbus command execution was successfull
	uint32_t Register2Write = ((uint32_t)((uint16_t)((arr2Analyze[INCOMING_UART_ARR_LOC_2]<<BYTE_SHIFT) & 0xFF00) + (uint32_t)arr2Analyze[INCOMING_UART_ARR_LOC_3])); //CODE_REVIEW: Using the index in the name INCOMING_UART_ARR_LOC_2 (and in several other cases) still does not make the variable understandable. The name should represent the purpose. Is it the second byte in the modbus query?  If so, then is it the function code and THAT should be the name. Please change this and all other similar variable names
	//uint16_t Index_Location_Within_Modbus_Mem_Space = (uint16_t)(Register2Write % REGISTERS_PER_NODE) * REGISTER_BYTE_NUM;
	uint16_t Index_Location_Within_Modbus_Mem_Space = (uint16_t)NewDataFlagsSelector(Register2Write);
	uint8_t NodeIDCalculated = 0;
	
	if(arr2Analyze[0] == GATEWAY_ID)		//If the data is for this GW
	{
		if(BOOTLOADER_PROCESS_START_FLAG == TRUE)
		{
			Update_FUTS_Register(NOT_CLEAR_TO_RECEIVE);	// update the FUTS register with NOT_CLEAR_TO_RECEIVE until after the relevant process is complete.
		}
		
		#ifdef ENABLE_MODBUS_APP_LOGS
				printf("\n\rDATA RECEIVED OVER MODBUS\n\r\n\r");
			for(uint8_t i=0;i<arrSize;i++)
			{
				printf("The value of receivedDataModbus[%d] is %x\n\r",i, arr2Analyze[i]); 
			}
		#endif
		
		if(Register2Write >= NUMBER_OF_REGS_FOR_GW)
			NodeIDCalculated = (Register2Write - NUMBER_OF_REGS_FOR_GW) / REGISTERS_PER_NODE;
		else
			NodeIDCalculated = 0;
		
		#ifdef ENABLE_MODBUS_APP_LOGS
			printf("\n\r\n\r The address within registers map is: %x\n\r", Register2Write);
			//printf("Index_Location_Within_Modbus_Mem_Space = %d\n\r", Index_Location_Within_Modbus_Mem_Space/REGISTER_BYTE_NUM);			//Printed divided by two for the user
			printf("Index_Location_Within_Modbus_Mem_Space = %d\n\r", Index_Location_Within_Modbus_Mem_Space);
			printf("Node ID as Calculated = %d\n\r\n\r", NodeIDCalculated);
		#endif
		
		switch (functionCode)
		{
			 case MB_ReadHoldingRegisters:
					#ifdef ENABLE_MODBUS_APP_LOGS
						printf("function code of MB_ReadHoldingRegisters %d\n\r", functionCode);
					#endif
					retErr = MB_Apply_ReadHoldingRegisters(arr2Analyze);
					break;
			 case MB_ReadInputRegisters:
					#ifdef ENABLE_MODBUS_APP_LOGS
						printf("function code of MB_ReadInputRegisters %d\n\r", functionCode);
					#endif
					retErr = MB_Apply_ReadHoldingRegisters(arr2Analyze); // Eyal Gerber: In this application, reading the input registers or the holding registers is the same thing so even though the command is for input registers we go and read the holding registers.
					break;
			 case MB_WriteSingleRegister:
					#ifdef ENABLE_MODBUS_APP_LOGS
						printf("function code of MB_WriteSingleRegister %d\n\r", functionCode);
					#endif
					retErr = MB_Apply_WriteSingleRegister(arr2Analyze);
					break;
			 case MB_WriteMultipleRegisters:
					#ifdef ENABLE_MODBUS_APP_LOGS
						printf("function code of MB_WriteMultipleRegisters %d\n\r", functionCode);
					#endif
					retErr = MB_Apply_WriteMultipleRegisters(arr2Analyze);
			 break;
			 default:
				 retErr = FAIL;
					#ifdef ENABLE_MODBUS_APP_LOGS
						printf("Unsupported function code %d\n\r", functionCode);
					#endif
		}
	}
	
	state = RF_RECEIVE_STATE;	// set the state again to be RF_RECEIVE_STATE for when we get back to the state machine function
	
	if(retErr == PASS) // only if the modbus execution was successfull then continue to this section
	{
		switch(Index_Location_Within_Modbus_Mem_Space)
		{
			case NODE_QUERY_AREA:
				if(functionCode == 0x10 || functionCode == 0x06)			//If we read from this address also, don't set this flag.
				{
					//CODE_REVIEW: since this entire "switch" has been moved after the modbus functions have been called, we now know if they passed CRC. So if we enter the switch it is because the CRC test has passed and there is no need to check it again.
						SetNewDataFlag4Node(NodeIDCalculated, QUERY);		//Set the flag to indicate that a new data arrived for this node.
						#ifdef ENABLE_MODBUS_APP_LOGS
							printf("Set new data flag of query for node %d\n\r", NodeIDCalculated);
						#endif
				}
				break;
			case NODE_CONFIG_PARAMS_AREA:
				if(functionCode == 0x10 || functionCode == 0x06)			//If we read from this address also, don't set this flag.
				{
					//CODE_REVIEW: since this entire "switch" has been moved after the modbus functions have been called, we now know if they passed CRC. So if we enter the switch it is because the CRC test has passed and there is no need to check it again.
						SetNewDataFlag4Node(NodeIDCalculated, SET_PARAMS);		//Set the flag to indicate that a new data arrived for this node.
						#ifdef ENABLE_MODBUS_APP_LOGS
							printf("Set new data flag of set parameters for node %d\n\r", NodeIDCalculated);
						#endif
						Write_NodeParameters_CRC(arr2Analyze);
				}
				break;
			case FIRMWARE_UPDATE_AREA:
					Firmware_Update_Parsing(arr2Analyze);
				break;
			case GATEWAY_AREA:
				if(Register2Write >= START_OF_CONFIG_REGISTERS_GW_MEM_AREA && Register2Write < FW_UPGRADE_BIT_GW_MEM_AREA) 			//If the write is for GW's configuration registers
					{
						
					}
					else if(Register2Write == FW_UPGRADE_BIT_GW_MEM_AREA)
					{
						
					}
					else if(Register2Write >= START_OF_SLOTS_REGISTERING_REGISTERS_GW_MEM_AREA)		//If the write for the GW area is in the nodes registering area
					{
						if(functionCode == 0x10 || functionCode == 0x06)			//The whole thing is relevant only if something was written to this area
						{
							AddingRemovingNodeAnalyze(arr2Analyze);
						}
					}
				break;
			}
		}
	
}



/***********************************************************

Function name: NewDataFlagsSelector

Function type: Modbus_Mem_Space

Arguments: uint32_t received_register

Return: //uint8_t val2Ret	//  CODE_REVIEW: You cannot just write the name of the variable in the description of what a function returns unless it is 100% obvious. This is not the case. What is Val2Ret? What are the various values that could be returned? I masked this and changed the line below
Return: Modbus_Mem_Space Mem_Space_Code	- the code specifying where the received_register is positioned within the modbus area space

Description: This function checks the received register
and returns in which section of the modbus memory space the register is located

**********************************************************/
Modbus_Mem_Space NewDataFlagsSelector(uint32_t received_register)
{
	Modbus_Mem_Space val2Ret = UNDEFINED_AREA;
	//uint16_t temp1 = 0, temp2 =0;	// CODE_REVIEW: NEVER USER THESE NAMES AGAIN. Always use a name that has a meaning. I masked these lines and gave the variables new names.
	uint16_t Byte_Num_Within_Node_Memory_Space;
	
	//temp1 = (uint16_t)((received_register % REGISTERS_PER_NODE) * REGISTER_BYTE_NUM)-NUMBER_OF_REGS_FOR_GW; //For node ID		//CODE_REVIEW: The calculation here is WRONG. I masked this line and re-wrote it below correctly.
	//uint16_t Node_ID = (uint16_t)((received_register - NUMBER_OF_REGS_FOR_GW) / REGISTERS_PER_NODE);	// the node ID which the received register refers to.
	
	Byte_Num_Within_Node_Memory_Space = ((received_register - NUMBER_OF_REGS_FOR_GW) % REGISTERS_PER_NODE) * REGISTER_BYTE_NUM;	// the calculated byte number within the node memory space 
	
	if(received_register < NUMBER_OF_REGS_FOR_GW)			//If the address is for the GW registers
		val2Ret = GATEWAY_AREA;
	else if((received_register >= NUMBER_OF_REGS_FOR_GW) && (received_register <= (NUMBER_OF_REGS_FOR_GW+REGISTERS_PER_NODE*TOTAL_NUMBER_OF_NODES))) //If in the range of the nodes
	{
		//temp2 = temp1-NUMBER_OF_REGS_FOR_GW;				//Removing the 200 registers of the GW	// CODE_REVIEW: I masked this because it was wrong.
		#ifdef ENABLE_MODBUS_APP_LOGS
			//printf("The value of temp2/2 is: %d and the value of received_register is: %x\n\r", temp2/REGISTER_BYTE_NUM, received_register);	//CODE_REVIEW: I masked this and rewrote it below. 
			printf("The value of received_register is: %x\n\r",received_register);
		#endif
		
		//if(temp2 == QUERY_ADDRESS_WITHIN_NODE_MEM_SPACE)				//If query	// CODE_REVIEW: I masked this because it was wrong. 
		if( Byte_Num_Within_Node_Memory_Space == QUERY_ADDRESS_WITHIN_NODE_MEM_SPACE)
			val2Ret = NODE_QUERY_AREA;
		else if( (Byte_Num_Within_Node_Memory_Space >= BR_COM_INTERVAL_LOCATION_WITHIN_NODE_MEM_SPACE) &&  (Byte_Num_Within_Node_Memory_Space <= ALARM_WAKEUP_INTERVAL_LOCATION_WITHIN_NODE_MEM_SPACE))			//If in range of configurable parameters
			val2Ret = NODE_CONFIG_PARAMS_AREA;
	}	
	else if( received_register > (NUMBER_OF_REGS_FOR_GW + (REGISTERS_PER_NODE * TOTAL_NUMBER_OF_NODES)) )			//If after the nodes area
		val2Ret = FIRMWARE_UPDATE_AREA;
	
	#ifdef ENABLE_MODBUS_APP_LOGS
		printf("\n\r\n\r The value returned from NewDataFlagsSelector function: %d\n\r\n\r", val2Ret);
	#endif
	
	return val2Ret;
}

/***********************************************************

Function name: CheckIfNewData4Node

Function type: uint8_t

Arguments: uint8_t NodeID, node_data_type FlagType

Return: uint8_t val2Ret - DATA_CHANGED or DATA_NOT_CHANGED

Description: This function is responsible for checking the
ArrOfFlags4Nodes array to see if there's a new data for 
NodeID. If so, the val2Ret will be set to DATA_CHANGED, else 
to DATA_NOT_CHANGED.
**********************************************************/
uint8_t CheckIfNewData4Node(uint8_t NodeID, node_data_type FlagType)
{
	uint8_t val2Ret = RESET;
	switch(FlagType)
	{
		case QUERY:
			if(ArrOfFlags4Nodes[NodeID].QueryFlag == DATA_CHANGED)
			val2Ret = DATA_CHANGED;
			else if(ArrOfFlags4Nodes[NodeID].QueryFlag == DATA_NOT_CHANGED)
				val2Ret = DATA_NOT_CHANGED;
			break;
		case SET_PARAMS:
			if(ArrOfFlags4Nodes[NodeID].SetParamsFlag == DATA_CHANGED)
			val2Ret = DATA_CHANGED;
			else if(ArrOfFlags4Nodes[NodeID].SetParamsFlag == DATA_NOT_CHANGED)
				val2Ret = DATA_NOT_CHANGED;
			break;
	}
	return val2Ret;
}


/***********************************************************

Function name: SetNewDataFlag4Node

Function type: void

Arguments: uint8_t NodeID, uint8_t FlagType

Return: None

Description: This function is responsible for setting the 
coresponding flag if a new data arrived from the GW to Node
**********************************************************/
void SetNewDataFlag4Node(uint8_t NodeID, uint8_t FlagType)	//CODE_REVIEW: We need to change so that this array will be in the EEPROM so that if there is a reset event or if there is a bootloading event, this information doesn't go to waste. 
{
	switch(FlagType)
	{
		case QUERY:
			ArrOfFlags4Nodes[NodeID].QueryFlag = SET;
			break;
		case SET_PARAMS:
			ArrOfFlags4Nodes[NodeID].SetParamsFlag = SET;
			break;
	}
}

/***********************************************************

Function name: UnsetNewDataFlag4Node

Function type: void

Arguments: uint8_t NodeID, uint8_t FlagType

Return: None

Description: This function is responsible for resetting the 
coresponding flag if the node already updated about the new
data
**********************************************************/
void UnsetNewDataFlag4Node(uint8_t NodeID, uint8_t FlagType)
{
	switch(FlagType)
	{
		case QUERY:
			ArrOfFlags4Nodes[NodeID].QueryFlag = RESET;
			break;
		case SET_PARAMS:
			ArrOfFlags4Nodes[NodeID].SetParamsFlag = RESET;
			break;
	}
}

/**
  * @brief  This function calculates and writes the CRC of the parameters of the node addressed in the modbus command (that is received as an argument). 
						This is used for the BIT functionality upon startup to verify that the node parameters have not accidently changed. 
  * @param  uint8_t *arr2Analyze - the modbus command to analyze
  * @retval None
  */
void Write_NodeParameters_CRC(uint8_t *arr2Analyze)
{
	uint32_t register_num = ((uint32_t)(arr2Analyze[REGISTER_ADDRESS_MSB]<<8) & 0x0000FF00) + (uint32_t)arr2Analyze[REGISTER_ADDRESS_LSB];	// extract the register number
	uint32_t add2Write = GW_MODBUS_REGISTERS_START_ADRS + register_num*REGISTER_BYTE_NUM;	// calculate the physical address in the memory to write to
	
	#ifdef ENABLE_CHECKSUM_FOR_NODE_PARAM
		uint8_t NodeIDCalc=0;
		
		if(add2Write >= NUMBER_OF_REGS_FOR_GW)
			 NodeIDCalc = (register_num-NUMBER_OF_REGS_FOR_GW) / REGISTERS_PER_NODE;
		
		if(CheckIfNewData4Node(NodeIDCalc, SET_PARAMS))			//If the new data written are within the parameters section
		{
			
			uint8_t ConfigurableParams[18] = {0};
			uint32_t address2Read = GW_MODBUS_REGISTERS_START_ADRS + NodeIDCalc * REGISTERS_PER_NODE*REGISTER_BYTE_NUM + NUMBER_OF_REGS_FOR_GW*REGISTER_BYTE_NUM;
			
			#ifdef ENABLE_MODBUS_APP_LOGS
				printf("The register_num = %d and the calculated nodeID is: %d\n\r", register_num, NodeIDCalc);
				printf("\n\r\n\rThe address to Read from the conf params %x\n\r\n\r", address2Read);
			#endif
			
			EXTERNAL_FLASH_ReadData(address2Read, ConfigurableParams, 18);
			
			#ifdef ENABLE_MODBUS_APP_LOGS
				printf("The parameters are:\n\r");
				for(uint8_t i=0;i<18;i++)
					printf("ConfigurableParams[%d] = %x\n\r",i, ConfigurableParams[i]); 
			#endif
			
			uint16_t NodeParamsChecksum = 0;
			NodeParamsChecksum = CalcCRC(ConfigurableParams, 18);
			#ifdef ENABLE_MODBUS_APP_LOGS
				printf("The CRC is %x\n\r", NodeParamsChecksum);
			#endif
			
			uint8_t NodeParamsChecksum2Write[2] = {0};
			
			NodeParamsChecksum2Write[0] = NodeParamsChecksum & 0xFF;
			NodeParamsChecksum2Write[1] = (NodeParamsChecksum >> 8) & 0xFF;
			
			#ifdef ENABLE_MODBUS_APP_LOGS
				printf("NodeParamsChecksum2Write[0] = %x\n\r", NodeParamsChecksum2Write[0]);
				printf("NodeParamsChecksum2Write[1] = %x\n\r", NodeParamsChecksum2Write[1]);
			#endif
			
			uint32_t address4ChecksumOfNodeParam = address2Read + CHECKSUM_OF_NODE_PARAM_LOCATION_WITHIN_NODE_MEM_SPACE*REGISTER_BYTE_NUM;
			
			#ifdef ENABLE_MODBUS_APP_LOGS
				printf("The address to write the CRC is: %x\n\r", address4ChecksumOfNodeParam);
			#endif
			
			EXTERNAL_FLASH_WriteData_Automatic(address4ChecksumOfNodeParam, REGISTER_BYTE_NUM, NodeParamsChecksum2Write);
			
		}
	#endif
	;
}

/**
  * @brief  This function parses the modbus command (that is received as an argument) and based on what was received does the required action
						relevant to the firmware update process. 
  * @param  uint8_t *arr2Analyze - the modbus command to analyze
  * @retval None
  */
void Firmware_Update_Parsing(uint8_t *arr2Analyze)
{
	uint32_t register_num = ((uint32_t)(arr2Analyze[REGISTER_ADDRESS_MSB]<<8) & 0x0000FF00) + (uint32_t)arr2Analyze[REGISTER_ADDRESS_LSB];	// extract the register number
	uint16_t TheData2Write = ((uint16_t)(arr2Analyze[DATA_ADDRESS_MSB]<<8) & 0x0000FF00) + (uint16_t)arr2Analyze[DATA_ADDRESS_LSB];	// extract the data to write
	static uint8_t Chunk_Index_Counter = 0;	// variable that counts the chunk indexes and makes sure that they are received in consecutive order.
	uint8_t Received_Chunk_Index;	// the received chunk index in the current modbus packet
	uint8_t functionCode = arr2Analyze[1];	// the function code of the modbus command received
	static firmware_update_state Firmware_Update_Status = FIRMWARE_UPDATE_STOPPED;
	static uint16_t binary_image_bytes_received = 0;	// the total number of bytes received for the binary image. 
	
	//Parsing of data for detecting bootloading start
	switch(register_num)	//CODE_REVIEW_done: Eyal Gerber added this part. It should be moved to somewhere in the application level and not within the modbus driver. I want to put in the state machine. To consult with Yehuda.
	{
		case GATEWAY_FIRMWARE_UPGRADE_START_REG:
			if( TheData2Write ==  GATEWAY_FIRMWARE_UPGRADE_ENABLE )	// if the modbus command was to set the bootloading bit in order to begin bootloading
			{
				Prepare_For_Bootloading();
				BOOTLOADER_PROCESS_START_FLAG = TRUE;
				CC1101_GPIOInterruptDisable();	// disable RF reception when bootloading process is in progress
				
				#ifdef ENABLE_MODBUS_APP_LOGS
					printf("GFUS bit ENABLED. Firmware upgrade process has started.\n\r");
				#endif
				
				Chunk_Index_Counter = 0;	// init the chunk index
				binary_image_bytes_received = 0;	// init the value
				Firmware_Update_Status = RECEIVING_DATA;
				Update_FUTS_Register(CLEAR_TO_RECEIVE);	// update the FUTS register with the relevant firmware update status
				
			}
			else	//the value in register GATEWAY_FIRMWARE_UPGRADE_START_REG != GATEWAY_FIRMWARE_UPGRADE_ENABLE
			{			
				BOOTLOADER_PROCESS_START_FLAG = FALSE;
				Firmware_Update_Status = FIRMWARE_UPDATE_STOPPED;
				
				#ifdef ENABLE_MODBUS_APP_LOGS
					printf("GFUS bit DISABLED. Firmware upgrade process has been disabled.\n\r");
				#endif
				
				Update_FUTS_Register(NOT_CLEAR_TO_RECEIVE);	// update the FUTS register with the relevant firmware update status
			}
			break;
			
		case FIRMWARE_DATA_PACKET_CHUNK_REG_BEGIN:
				if( (functionCode == MB_WriteMultipleRegisters) && (Firmware_Update_Status == RECEIVING_DATA) )
				{
					Received_Chunk_Index = arr2Analyze[CHUNK_INDEX_POSITION];
					#ifdef ENABLE_MODBUS_APP_LOGS
						printf("Received data packet chunk. Chunk Index = %d\n\r",Received_Chunk_Index);
					#endif
					
					if(Read_FUTS_Register() == NOT_CLEAR_TO_RECEIVE)
						Update_FUTS_Register(NOT_CLEAR_TO_RECEIVE_ERROR);
					else
					{
						if(Received_Chunk_Index != Chunk_Index_Counter)
							Update_FUTS_Register(WRONG_DATA_CHUNK_INDEX);
						else
						{
							Chunk_Index_Counter++;	// increment the chunk index counter
							Firmware_Update_Status = RECEIVING_DATA;
							binary_image_bytes_received += Save_DataChunk_ToExternalFlash(arr2Analyze);
							Update_FUTS_Register(CLEAR_TO_RECEIVE);	// update the FUTS register with the relevant firmware update status
						}
					}
				}
				else
					Update_FUTS_Register(GENERAL_ERROR);	// General Error because this register must be written to in a multiple register command, otherwise the update sequence could get ruined. 
		
				break;
				
		case IMAGE_CRC_REG:
				if(functionCode == MB_WriteSingleRegister && (Firmware_Update_Status == RECEIVING_DATA) )
				{
					#ifdef ENABLE_MODBUS_APP_LOGS
						printf("Binary image CRC %d written to IMAGE_CRC_REG\n\r",TheData2Write);
					#endif
					
					if(Read_FUTS_Register() == NOT_CLEAR_TO_RECEIVE)
						Update_FUTS_Register(NOT_CLEAR_TO_RECEIVE_ERROR);
					else
					{	
						Update_FUTS_Register(IMAGE_CRC_ANALYSIS_IN_PROGRESS);
						Firmware_Update_Status = RECEIVED_IMAGE_CRC;
						Save_Image_Size_To_ExternalFlash(binary_image_bytes_received);
						if(Check_ReceivedFirmwareImage_CRC(arr2Analyze) == PASS)		// calculates the CRC based on the image saved in the external flash and compares it with the one recieved over modbus
						{
							Update_FUTS_Register(IMAGE_CRC_VALIDATED);	// update the FUTS register with the relevant firmware update status
						}
						else
							Update_FUTS_Register(IMAGE_CRC_ERROR);	// update the FUTS register with the relevant firmware update status
					}
				}
				else
					Update_FUTS_Register(GENERAL_ERROR);	// General Error because this register must be written to in a single register command, otherwise the update sequence could get ruined. 
				
				break;
				
		case FIRMWARE_UPGRADE_START_REG:
				if( (functionCode == MB_WriteSingleRegister) && (Firmware_Update_Status == RECEIVED_IMAGE_CRC) )
				{
					if( TheData2Write == FIRMWARE_UPGRADE_START_ENABLE )
					{
						#ifdef ENABLE_MODBUS_APP_LOGS
							printf("Received value %d for FIRMWARE_UPGRADE_START_REG\n\r",TheData2Write);
						#endif
						
						if(Read_FUTS_Register() == IMAGE_CRC_VALIDATED)	// verify that the image is validated before starting the bootloading process
							Begin_Bootloading();
						else
						{
							; // do nothing since the FUTS register is already set with the relevant status from previous stages
				
						}
					}
				}
				else
					Update_FUTS_Register(GENERAL_ERROR);	// General Error because this register must be written to in a single register command, otherwise the update sequence could get ruined. 
				
			break;
		
		default:
			#ifdef ENABLE_MODBUS_APP_LOGS
				printf("ERROR! Exepcted a register from the firmware update area in the modbus memory area\n\r");
			#endif
			break;
	}
}

/**
  * @brief  This function is responsible for comparing the received image CRC with the calculation of the CRC based on the received image that was saved in the external flash. 
  * @param  None
  * @retval uint8_t PASS or FAIL
  */
uint8_t Check_ReceivedFirmwareImage_CRC(uint8_t *arr2Analyze)
{
	uint16_t ReceivedCRC_Val;
	uint8_t calculated_CRC;
	
	ReceivedCRC_Val = ((uint16_t)(arr2Analyze[DATA_ADDRESS_MSB]<<8) & 0xFF00) + (uint16_t)arr2Analyze[DATA_ADDRESS_LSB];	// the received CRC over modbus
	
	#ifdef ENABLE_MODBUS_APP_LOGS
		printf("ReceivedCRC_Val = %d\n\r",ReceivedCRC_Val);
	#endif
	
	calculated_CRC = Calc_And_Save_Binary_Image_CRC();	// the calculated CRC based on the data saved in the external flash
	
	#ifdef ENABLE_MODBUS_APP_LOGS
		printf("calculated_CRC = %d\n\r",calculated_CRC);
	#endif
	
	if(ReceivedCRC_Val != calculated_CRC)
		return FAIL;
	
	return PASS;
}

/**
  * @brief  This function is responsible for saving the data packet chunks received over modbus to the external flash
						This function takes the data chunk data from the RAM and not from the modbus memory space. This is so that there would be an option to 
						possibly make an exception in the modbus driver to not write to the modbus memory space the data chunks in order to reduce the number of erases to the external flash
						in order to prolong the life of the external flash. 
  * @param  Firmware_Upgrade_Transfer_Status upgrade_status - the status to write to the register
  * @retval uint16_t - the number of data bytes in the chunk (not counting the chunk index)
  */
uint16_t Save_DataChunk_ToExternalFlash(uint8_t *arr2Analyze)
{
	uint8_t data[MAX_NUMBER_OF_BYTES_FOR_DATA_CHUNK] = {0};
	uint16_t byteNum2send;
	uint32_t add2Write;
	uint8_t Received_Chunk_Index = arr2Analyze[CHUNK_INDEX_POSITION];
	uint8_t i;
	
	add2Write = (APP_IMAGE_SECTOR_START * BASE_SECTOR_SIZE) + (Received_Chunk_Index * MAX_NUMBER_OF_BYTES_FOR_DATA_CHUNK);
	byteNum2send = arr2Analyze[CHUNK_BYTE_NUM_POSITION]-1;	// minus one because the chunk index is not part of the image data packet chunk
	for( i = 0 ; i < byteNum2send ; i++ )
		data[i] = arr2Analyze[CHUNK_DATA_POSITION_BEGIN+i];	// populating array to save to external flash
	
	EXTERNAL_FLASH_WriteData(add2Write, byteNum2send, data);
	
	return byteNum2send;
}

/**
  * @brief  This function is responsible for updating the FUTS (FIRMWARE UPGRADE TRANSFER STATUS) register
  * @param  Firmware_Upgrade_Transfer_Status upgrade_status - the status to write to the register
  * @retval None
  */
void Update_FUTS_Register(Firmware_Upgrade_Transfer_Status upgrade_status)
{
	uint8_t data[2] = {0};
	uint16_t byteNum2send;
	uint32_t add2Write;
	
	data[0] = (uint8_t)upgrade_status;
	byteNum2send = 1;	// because we're sending one byte
	add2Write = MODBUS_MEM_SPACE_START_ADDRESS + (FIRMWARE_UPGRADE_TRANSFER_STATUS_REG * REGISTER_BYTE_NUM);
	EXTERNAL_FLASH_WriteData_Automatic(add2Write, byteNum2send, data);
	
	#ifdef ENABLE_MODBUS_APP_LOGS
		printf("Firmware_Upgrade_Transfer_Status_Register = ");
		switch(upgrade_status)
		{
			case CLEAR_TO_RECEIVE: printf("CLEAR_TO_RECEIVE");
		  case WRONG_DATA_CHUNK_INDEX: printf("WRONG_DATA_CHUNK_INDEX");
			case NOT_CLEAR_TO_RECEIVE_ERROR: printf("NOT_CLEAR_TO_RECEIVE_ERROR");
			case IMAGE_CRC_ERROR: printf("IMAGE_CRC_ERROR");
			case IMAGE_CRC_VALIDATED: printf("IMAGE_CRC_VALIDATED");
			case IMAGE_CRC_ANALYSIS_IN_PROGRESS: printf("IMAGE_CRC_ANALYSIS_IN_PROGRESS");
			case GENERAL_ERROR: printf("GENERAL_ERROR");
			default: printf("UNKOWN STATUS");
				break;
		}				
		printf("\n\r");
	#endif
}

/**
  * @brief  This function is responsible for reading the FUTS (FIRMWARE UPGRADE TRANSFER STATUS) register and returing its value
  * @param  None
  * @retval Firmware_Upgrade_Transfer_Status upgrade_status - the status to read from the register
  */
Firmware_Upgrade_Transfer_Status Read_FUTS_Register(void)
{
	uint16_t byteNum2read = 1;	// because we're reading just 1 byte
	uint8_t data[2] = {0};
	uint32_t add2Read;
	Firmware_Upgrade_Transfer_Status upgrade_status;
	
	add2Read = MODBUS_MEM_SPACE_START_ADDRESS + (FIRMWARE_UPGRADE_TRANSFER_STATUS_REG * REGISTER_BYTE_NUM);
	EXTERNAL_FLASH_ReadData(add2Read, data, byteNum2read);
	
	upgrade_status = (Firmware_Upgrade_Transfer_Status)data[0];
	
	return upgrade_status;
}

/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/

