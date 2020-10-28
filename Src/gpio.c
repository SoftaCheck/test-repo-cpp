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
  * @file           : gpio.c
  * @brief          : Driver for easily controlling the pins that are configured
                      as digital inputs or outputs
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

GPIO_def GPIO_ARRAY[GPIO_PINS_USED_NUM];  //global array that holds the information of all the GPIOs
 
/* Private function prototypes -----------------------------------------------*/

void LedBlink(GPIO_pins gpioIndex, uint32_t delay);

/* Exported functions ---------------------------------------------------------*/
/**
  * @brief  This function sets the output value of a specific GPIO (must be preconfigured as output)
  * @param  index in the GPIO struct array indicating the relevant GPIO pin
  * @retval None
  */
void GPIO_SetVal(GPIO_pins GPIO_Index, GPIO_State State)
{
  static GPIO_PinState PinState;
  if(State)
    PinState = GPIO_PIN_SET;
  else
    PinState = GPIO_PIN_RESET;
  
  HAL_GPIO_WritePin(GPIO_ARRAY[GPIO_Index].GPIO_PORT, GPIO_ARRAY[GPIO_Index].Pin, PinState); 
}

void GPIO_ToggleVal(GPIO_pins GPIO_Index)
{
  static GPIO_PinState NewState;
  
  if(GPIO_ReadVal(GPIO_Index) == GPIO_PIN_SET)
    NewState = GPIO_PIN_RESET;
  else
    NewState = GPIO_PIN_SET;
  
  HAL_GPIO_WritePin(GPIO_ARRAY[GPIO_Index].GPIO_PORT, GPIO_ARRAY[GPIO_Index].Pin, NewState); 
}
    
/**
  * @brief  This function reads the value of a specific GPIO (must be preconfigured as input)
  * @param  index in the GPIO struct array indicating the relevant GPIO pin
  * @retval None
  */
uint8_t GPIO_ReadVal(GPIO_pins GPIO_Index)
{
  return HAL_GPIO_ReadPin(GPIO_ARRAY[GPIO_Index].GPIO_PORT, GPIO_ARRAY[GPIO_Index].Pin);
  
}

/**
  * @brief  This function initializes the GPIO array
  * @param  None
  * @retval None
  */
void GPIO_Init()
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	//Red_LED
	GPIO_ARRAY[Red_LED].Pin = Red_LED_Pin;
  GPIO_ARRAY[Red_LED].GPIO_PORT = Red_LED_GPIO_Port;
	GPIO_InitStruct.Pin = Red_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Red_LED_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(Red_LED,LOW);
	
	//Yellow_LED
	GPIO_ARRAY[Yellow_LED].Pin = Yellow_LED_Pin;
  GPIO_ARRAY[Yellow_LED].GPIO_PORT = Yellow_LED_GPIO_Port;
	GPIO_InitStruct.Pin = Yellow_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Yellow_LED_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(Yellow_LED,LOW);
	
	//Green_LED
	GPIO_ARRAY[Green_LED].Pin = Green_LED_Pin;
  GPIO_ARRAY[Green_LED].GPIO_PORT = Green_LED_GPIO_Port;
	GPIO_InitStruct.Pin = Green_LED_Pin;		
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Green_LED_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(Green_LED,LOW);
	
	//Blue_LED
	GPIO_ARRAY[Blue_LED].Pin = Blue_LED_Pin;
  GPIO_ARRAY[Blue_LED].GPIO_PORT = Blue_LED_GPIO_Port;
	GPIO_InitStruct.Pin = Blue_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Blue_LED_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(Blue_LED,LOW);
	
	//Jumper1
	GPIO_ARRAY[Jumper1].Pin = Jumper1_Pin;
  GPIO_ARRAY[Jumper1].GPIO_PORT = Jumper1_GPIO_Port;
  GPIO_InitStruct.Pin = Jumper1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Jumper1_GPIO_Port, &GPIO_InitStruct);
	
	//Jumper2
	GPIO_ARRAY[Jumper2].Pin = Jumper2_Pin;
  GPIO_ARRAY[Jumper2].GPIO_PORT = Jumper2_GPIO_Port;
  GPIO_InitStruct.Pin = Jumper2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Jumper2_GPIO_Port, &GPIO_InitStruct);
	
	//Jumper3
	GPIO_ARRAY[Jumper3].Pin = Jumper3_Pin;
  GPIO_ARRAY[Jumper3].GPIO_PORT = Jumper3_GPIO_Port;
  GPIO_InitStruct.Pin = Jumper3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Jumper3_GPIO_Port, &GPIO_InitStruct);
	
	//Jumper4
	GPIO_ARRAY[Jumper4].Pin = Jumper4_Pin;
  GPIO_ARRAY[Jumper4].GPIO_PORT = Jumper4_GPIO_Port;
  GPIO_InitStruct.Pin = Jumper4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Jumper4_GPIO_Port, &GPIO_InitStruct);
	
	//Jumper5
	GPIO_ARRAY[Jumper5].Pin = Jumper5_Pin;
  GPIO_ARRAY[Jumper5].GPIO_PORT = Jumper5_GPIO_Port;
  GPIO_InitStruct.Pin = Jumper5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Jumper5_GPIO_Port, &GPIO_InitStruct);

	//IO1
	GPIO_ARRAY[IO1].Pin = IO1_Pin;
  GPIO_ARRAY[IO1].GPIO_PORT = IO1_GPIO_Port;
  GPIO_InitStruct.Pin = IO1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IO1_GPIO_Port, &GPIO_InitStruct);
	
	//IO2
	GPIO_ARRAY[IO2].Pin = IO2_Pin;
  GPIO_ARRAY[IO2].GPIO_PORT = IO2_GPIO_Port;
  GPIO_InitStruct.Pin = IO2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IO2_GPIO_Port, &GPIO_InitStruct);
	
	//IO3
	GPIO_ARRAY[IO3].Pin = IO3_Pin;
  GPIO_ARRAY[IO3].GPIO_PORT = IO3_GPIO_Port;
  GPIO_InitStruct.Pin = IO3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IO3_GPIO_Port, &GPIO_InitStruct);
	
	//IO4
	GPIO_ARRAY[IO4].Pin = IO1_Pin;
  GPIO_ARRAY[IO4].GPIO_PORT = IO4_GPIO_Port;
  GPIO_InitStruct.Pin = IO4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IO4_GPIO_Port, &GPIO_InitStruct);

	//SPI1_NNS
	GPIO_ARRAY[SPI1_NNS].Pin = SPI1_NNS_Pin;
  GPIO_ARRAY[SPI1_NNS].GPIO_PORT = SPI1_NNS_GPIO_Port;
  GPIO_InitStruct.Pin = SPI1_NNS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_NNS_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(SPI1_NNS,LOW);
	
	//SPI2_CS
	GPIO_ARRAY[SPI2_CS].Pin = SPI2_CS_Pin;
  GPIO_ARRAY[SPI2_CS].GPIO_PORT = SPI2_CS_GPIO_Port;
  GPIO_InitStruct.Pin = SPI2_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI2_CS_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(SPI2_CS,HIGH);
	
	//U_BTN
	GPIO_ARRAY[U_BTN].Pin = U_BTN_Pin;
  GPIO_ARRAY[U_BTN].GPIO_PORT = U_BTN_GPIO_Port;
  GPIO_InitStruct.Pin = U_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(U_BTN_GPIO_Port, &GPIO_InitStruct);
	
	//VDD_PER_EN
	GPIO_ARRAY[VDD_PER_EN].Pin = VDD_PER_EN_Pin;
  GPIO_ARRAY[VDD_PER_EN].GPIO_PORT = VDD_PER_EN_GPIO_Port;
	GPIO_InitStruct.Pin = VDD_PER_EN_Pin;		
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(VDD_PER_EN_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(VDD_PER_EN,LOW);
	
	//EN_Vsys
	GPIO_ARRAY[EN_Vsys].Pin = EN_Vsys_Pin;
  GPIO_ARRAY[EN_Vsys].GPIO_PORT = EN_Vsys_GPIO_Port;
  GPIO_InitStruct.Pin = EN_Vsys_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(EN_Vsys_GPIO_Port, &GPIO_InitStruct);
	
	//EN_12V
	GPIO_ARRAY[EN_12V].Pin = EN_12V_Pin;
  GPIO_ARRAY[EN_12V].GPIO_PORT = EN_12V_GPIO_Port;
	GPIO_InitStruct.Pin = EN_12V_Pin;		
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EN_12V_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(EN_12V,LOW);
	
	//RS485_DE1
	GPIO_ARRAY[RS485_DE1].Pin = RS485_DE1_Pin;
  GPIO_ARRAY[RS485_DE1].GPIO_PORT = RS485_DE1_GPIO_Port;
  GPIO_InitStruct.Pin = RS485_DE1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RS485_DE1_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(RS485_DE1,LOW);
	
	//RS485_RE1
	GPIO_ARRAY[RS485_RE1].Pin = RS485_RE1_Pin;
  GPIO_ARRAY[RS485_RE1].GPIO_PORT = RS485_RE1_GPIO_Port;
  GPIO_InitStruct.Pin = RS485_RE1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RS485_RE1_GPIO_Port, &GPIO_InitStruct);
	
	//RS485_RE2
	GPIO_ARRAY[RS485_RE2].Pin = RS485_RE2_Pin;
  GPIO_ARRAY[RS485_RE2].GPIO_PORT = RS485_RE2_GPIO_Port;
  GPIO_InitStruct.Pin = RS485_RE2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RS485_RE2_GPIO_Port, &GPIO_InitStruct);
	
	//DB_3V_MCU_EN
	GPIO_ARRAY[DB_3V_MCU_EN].Pin = DB_3V_MCU_EN_Pin;
  GPIO_ARRAY[DB_3V_MCU_EN].GPIO_PORT = DB_3V_MCU_EN_GPIO_Port;
	GPIO_InitStruct.Pin = DB_3V_MCU_EN_Pin;		
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DB_3V_MCU_EN_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(DB_3V_MCU_EN,LOW);
	
	//EN_MCPU
	GPIO_ARRAY[EN_MCPU].Pin = EN_MCPU_Pin;
  GPIO_ARRAY[EN_MCPU].GPIO_PORT = EN_MCPU_GPIO_Port;
	GPIO_InitStruct.Pin = EN_MCPU_Pin;		
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EN_MCPU_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(EN_MCPU,HIGH);
	
	//PG_PS
	GPIO_ARRAY[PG_PS].Pin = PG_PS_Pin;
  GPIO_ARRAY[PG_PS].GPIO_PORT = PG_PS_GPIO_Port;
  GPIO_InitStruct.Pin = PG_PS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PG_PS_GPIO_Port, &GPIO_InitStruct);
	
	//GDO0
	GPIO_ARRAY[GDO0].Pin = GDO0_Pin;
  GPIO_ARRAY[GDO0].GPIO_PORT = GDO0_GPIO_Port;
	GPIO_InitStruct.Pin = GDO0_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GDO0_GPIO_Port, &GPIO_InitStruct);

	//MCU_DB_3V
	GPIO_ARRAY[MCU_DB_3V].Pin = MCU_DB_3V_Pin;
  GPIO_ARRAY[MCU_DB_3V].GPIO_PORT = MCU_DB_3V_GPIO_Port;
  GPIO_InitStruct.Pin = MCU_DB_3V_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MCU_DB_3V_GPIO_Port, &GPIO_InitStruct);
	
	//AN_IN
	GPIO_ARRAY[AN_IN].Pin = AN_IN_Pin;
  GPIO_ARRAY[AN_IN].GPIO_PORT = AN_IN_GPIO_Port;
  GPIO_InitStruct.Pin = AN_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(AN_IN_GPIO_Port, &GPIO_InitStruct);

	//RS485_DE2
	GPIO_ARRAY[RS485_DE2].Pin = RS485_DE2_Pin;
  GPIO_ARRAY[RS485_DE2].GPIO_PORT = RS485_DE2_GPIO_Port;
  GPIO_InitStruct.Pin = RS485_DE2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RS485_DE2_GPIO_Port, &GPIO_InitStruct);
	GPIO_SetVal(RS485_DE2,LOW);

	HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);	
}

/**
  * @brief  This function blinks a led several times based on the parameter sent
            The led will blink in a dutycycle of 50% (led on period = led off period)
  * @param  GPIO_pins gpioIndex - the index of the GPIO pin
            uint32_t delay - the on/off period of the led in milliseconds
            uint8_t times - number of times to blink
  * @retval None
  */
void LedBlinkNTimes(GPIO_pins gpioIndex, uint32_t delay, uint8_t times)
{
    for(;times>0;times--)
    {
        LedBlink(gpioIndex,delay);
        HAL_Delay(delay);
    }
}

/**
  * @brief  This function turns on a led for a certain period of time based on
            the variable "delay" sent.
  * @param  GPIO_pins gpioIndex - the index of the GPIO pin
            uint32_t delay - the on period of the led in milliseconds
  * @retval None
  */
void LedBlink(GPIO_pins gpioIndex, uint32_t delay)
{
  GPIO_SetVal(gpioIndex,HIGH);
  HAL_Delay(delay);
  GPIO_SetVal(gpioIndex,LOW);
}

/**
  * @brief  This function blinks all three leds several times based on the parameter sent
            The led will blink in a dutycycle of 50% (led on period = led off period)
  * @param  GPIO_pins gpioIndex - the index of the GPIO pin
            uint32_t delay - the on period of the led in milliseconds
  * @retval None
  */
void AllLedsBlink(uint32_t delay, uint8_t times)
{
    for(;times>0;times--)
    {
        GPIO_SetVal(Red_LED,HIGH);
        GPIO_SetVal(Green_LED,HIGH);
        GPIO_SetVal(Blue_LED,HIGH);
				GPIO_SetVal(Yellow_LED,HIGH);
        HAL_Delay(delay);
        
        GPIO_SetVal(Red_LED,LOW);
        GPIO_SetVal(Green_LED,LOW);
        GPIO_SetVal(Blue_LED,LOW);
			GPIO_SetVal(Yellow_LED,LOW);
        HAL_Delay(delay);
    }
}
/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/

