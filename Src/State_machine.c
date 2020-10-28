#include "State_machine.h"



/*Global variables and flag used by this state machine.
All of them are defined and declared here only and in a dedicated section in the header file.*/
uint8_t RFData2AnalyzeFlag = FALSE;
uint8_t Wait4QueryResponse1 = FALSE;
uint8_t Wait4QueryResponse2 = FALSE;

uint8_t ACK_RF_Flag = FALSE;
uint8_t NACK_RF_Flag = FALSE;
uint8_t QueryRF_Flag = FALSE;
uint8_t SetParamsRF_Flag = FALSE;
uint8_t RESET_RF_Flag = FALSE;
uint8_t RESET_AndDef_RF_Flag = FALSE;


uint8_t state = RF_RECEIVE_STATE;					//This is a global variable. This variable indicate what is the current state of the state machine.

/*End of global variables and flags area*/


uint8_t MainStateMachineLoop(uint8_t CurrentState)
{
	switch(CurrentState)
	{
		case UART_RECEIVE_STATE:
			UART_receiveState();
			break;
		case UART_TRANSMIT_STATE:
			break;
		case UART_INCOMMING_DATA_ANALYZE_STATE:
			break;
		case RF_INCOMING_DATA_ANALYZE_STATE:
			return RF_IncomingDataAnalayzeState();
		
		case RF_RECEIVE_STATE:
			return RF_receiveState();
		
		case RF_TRANSMIT_STATE:
			RF_transmitState();
			break;
		case NODES_MEMORY_MANAGE_WRITE_STATE:
			break;
		case NODES_MEMORY_MANAGE_READ_STATE:
			break;
		case GW_MEMORY_MANAGE_READ_STATE:
			break;
		case GW_MEMORY_MANAGE_WRITE_STATE:
			break;
		default:
			return 0;
	}
}



uint8_t RF_receiveState(void)
{
	uint32_t TimeSample4QAndGetParamsResponses = 0;
	CC1101_GPIOInterruptEnable();				//Enable RF interrupt
	if(Wait4QueryResponse1 || Wait4QueryResponse2)			//if we know that a specific response should come
	{
		TimeSample4QAndGetParamsResponses = HAL_GetTick();			//Sample the time for timeout calcuation
		if(!RFData2AnalyzeFlag)
		{
			while(HAL_GetTick() - TimeSample4QAndGetParamsResponses < TIMEOUT4Q_AND_GET_PARAMS_RESPONSES)
			{
				if(RFData2AnalyzeFlag)
				{
					RFData2AnalyzeFlag = FALSE;
					CC1101_GPIOInterruptDisable();
					state = RF_INCOMING_DATA_ANALYZE_STATE;
					return BACK_FROM_RF_RECEIVE_STATE;
				}
				else
					state = RF_RECEIVE_STATE;

			}
		}
		if(!RFData2AnalyzeFlag)				//If the while loop ended due to timeout
		{
			NACK_RF_Flag = TRUE;				//Set the NACK flag, to indicate that no response has come
			state = RF_TRANSMIT_STATE;
			return BACK_FROM_RF_RECEIVE_STATE;
		}
			
	}
	
	if(RFData2AnalyzeFlag)
	{
		RFData2AnalyzeFlag = FALSE;
		CC1101_GPIOInterruptDisable();
		state = RF_INCOMING_DATA_ANALYZE_STATE;
	}
	else
		state = RF_RECEIVE_STATE;
	return BACK_FROM_RF_RECEIVE_STATE;
}




uint8_t RF_transmitState(void)
{
	if(ACK_RF_Flag)
	{
		ACK_RF_Flag = FALSE;
		HAL_Delay(10);
		RF_SendCommand(ACK_COMMAND);
		CurrentNodeID = GW_AVALIABLE_4_COMM;			//Delete the node ID from the RAM
	}
	else if(QueryRF_Flag)
	{
		QueryRF_Flag = FALSE;
		HAL_Delay(10);
		RF_SendCommand(QUERY_GW2BR_SEND_COMMAND);
	}
	else if(SetParamsRF_Flag)
	{
		SetParamsRF_Flag = FALSE;
		HAL_Delay(10);
		RF_SendCommand(SET_PARAMETERS);
	}
	else if(RESET_RF_Flag)
	{
		RESET_RF_Flag = FALSE;
		HAL_Delay(10);
		RF_SendCommand(REBOOT_COMMAND);
		UnsetResetBit();
	}
	else if(RESET_AndDef_RF_Flag)
	{
		RESET_AndDef_RF_Flag = FALSE;
		HAL_Delay(10);
		RF_SendCommand(REBOOT_AND_DEF_COMMAND);
		UnsetResetAndDefBit();
	}
	state = RF_RECEIVE_STATE;									// CODE_REVIEW: I see in the code that there are about 10 places in the code where you put this line for RF_RECEIVE_STATE. Please reduce it or at least make it more readable. For example, this line can be put in the function MainStateMachineLoop() right after RF_transmitState(). Please reduce the calls to this state from ALL OVER the code and try and concentrate it in MainStateMachineLoop()
	return BACK_FROM_RF_TRANSMIT_STATE;
}


uint8_t UART_receiveState(void)
{
	EndOfReceiveUART();
	return BACK_FROM_UART_RECEIVE_STATE;
}



uint8_t UART_transmitState(void)
{
	
	return BACK_FROM_UART_TRANSMIT_STATE;
}


uint8_t UART_InsomingDataAnalyzeState(void)
{
	
	return BACK_FROM_UART_INCOMMING_DATA_ANALZE_STATE;
}


uint8_t RF_IncomingDataAnalayzeState(void)
{
	RFIncomingData4BRAnalays(RF_RXBuffer);
	return BACK_FROM_RF_INCOMMING_DATA_ANALZE_STATE;
}


uint8_t NodesMemoryManageWriteState(void)
{
	
	return BACK_FROM_NODES_MEMORY_MANAGE_WRITE_STATE;
}


uint8_t NodesMemoryManageReadState(void)
{
	
	return BACK_FROM_NODES_MEMORY_MANAGE_READ_STATE;
}


uint8_t GWMemoryManageWriteState(void)
{
	
	return BACK_FROM_GW_MEMORY_MANAGE_WRITE_STATE;
}



uint8_t GWMemoryManageReadState(void)
{
	
	return BACK_FROM_GW_MEMORY_MANAGE_READ_STATE;
}



