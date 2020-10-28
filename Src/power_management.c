/* ========================================
 *
 * Copyright Magneto, 2018
 * Written by Novodes for Magneto
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF Magneto.
 *
 * ========================================
*/
/**
  ******************************************************************************
  * @file           : power_management.c
  * @brief          : This file is for managing the various sleep modes
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "power_management.h"
#include "gpio.h"
#include "rtc.h"
#include "watchdog.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define ENABLE_ULTRA_LOW_POWER  //Enable Ultra low power (ULP) mode - setting the ULP bit in the PWR_CR register in order to disable the internal voltage reference when entering the low power mode

#ifdef ENABLE_ULTRA_LOW_POWER   // the fast wakeup is only relevant if the ultra low power is enabled. 
//#define ENABLE_FAST_WAKEUP      // enable this if fast wake up is desired (only relevant in case of STOP mode or Standby Mode - see reference manual). Note that if enabled, then for the first ~3ms after wakeup the internal voltage reference is not reliable and any function that requires it (e.g. ADC) should not work during this startup time.
#endif

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void Enter_STOP_Mode(void);
void SystemPower_Config_Before_LowPower(void);
void SystemClockConfig_After_STOP(void);
void System_Config_After_STOP(void);
void Prepare_MCU_Pins_For_LowPowerMode(void);
void SystemGPIOs_After_STOP(void);

/* Exported functions ---------------------------------------------------------*/

/**
  * @brief  This function sets the device into the desired low power mode for a set period.
  * @param  LowPowerModeTypeDef mode - the desired low power mode
						uint32_t total_sleep_time - how long to remain in sleep [seconds]. If no set wakeup is desired then the value should be zero. It is not possible to set the value to zero if the Watchdog is enabled.
  * @retval None
  */
void Enter_Low_Power_Mode(LowPowerModeTypeDef low_power_mode, uint32_t total_sleep_time)
{
	#ifdef ENABLE_WATCH_DOG
		uint32_t i;												
		uint32_t MiniSleepIterations = 0;	// the number of sleep iterations. Only relevant in case the watchdog is enabled. In which case, the watchdog cannot be disabled during run time and we must wakeup in order to reset the watchdog timer and go back to sleep until the entire desired sleep period is fullfilled.
		uint8_t MiniSleepPeriod = 0;  // The MiniSleepPeriod is the time the MCU goes to sleep and wakes up just to reset the watchdog timer to avoid a watchdog reset.
		
		MiniSleepPeriod = (WATCHDOG_RESET_PERIOD-2);  //It is equal to the WATCHDOG_INTERVAL-2 because we don't want to wakeup EXACTLY when the watchdog timer elapses because that would be too risky.
		if(MiniSleepPeriod == 0)
		{
			#ifdef WATCHDOG_LOGS
				printf("Enter_Low_Power_Mode Function: Not entering sleep mode. Error: MiniSleepIterations = 0. This causes division by zero.\n\r");
			#endif
			return; 
		}
		else
			MiniSleepIterations = ( total_sleep_time / MiniSleepPeriod ) + 1;	
		
		WD_Reload(); 		// reload the watchdog register to prevent watchdog reset event.
		
		for( i=0 ; i<MiniSleepIterations ; i++ )
		{
				#ifdef POWER_MANAGEMENT_LOGS
					printf("MiniSleepIteration Number: %d\n\r",i);
				#endif
				if(i == MiniSleepIterations - 1)	// if this is the last mini sleep iteration
					Set_RTC_Wakeup(total_sleep_time % MiniSleepPeriod);	// if this is the last sleep iteration, then we don't need to necessarily sleep a whole MiniSleepPeriod but only what's left. 
				else
					Set_RTC_Wakeup(MiniSleepPeriod);
	#else
				if(total_sleep_time == 0)	// don't go to sleep (as explained in the function description)
					;//Disable_RTC_Wakeup();	// Eyal Gerber: I disabled this since we don't want to disable the wakeup in this case since we could have woken up previously due to push button interrupt and we don't want to get out of sync of the time interval of awakes
				else
					Set_RTC_Wakeup(total_sleep_time);
	#endif
				RTC_WakeUpInterrupt_Flag = FALSE;	// set the flag to be FALSE
				
				switch(low_power_mode)
				{
					case LOW_POWER_RUN:
					break;
					case SLEEP:
					break;
					case LOW_POWER_SLEEP:
					break;
					case STOP_MODE:
						#ifdef POWER_MANAGEMENT_LOGS
							printf("Entering Stop Mode\n\r");
						#endif
					
						SystemPower_Config_Before_LowPower();
						Enter_STOP_Mode();
						System_Config_After_STOP();
					
						#ifdef POWER_MANAGEMENT_LOGS
							printf("Woke up from Stop Mode\n\r");
						#endif
					break; 
					case STANDBY_MODE:
					break;
					default:
						; //do nothing
				}
				
	#ifdef ENABLE_WATCH_DOG			
				WD_Reload(); 		// reload the watchdog register to prevent watchdog reset event.
	#endif			
				Disable_RTC_Wakeup();
				
				if(RTC_WakeUpInterrupt_Flag == FALSE)	// the reason for waking up was due to a different interrupt source (maybe pushbutton)
				{
					#ifdef POWER_MANAGEMENT_LOGS
						printf("Woke up NOT due to RTC Wakeup Interrupt\n\r");
					#endif
					return; 
				}				
	#ifdef ENABLE_WATCH_DOG
		}
	#endif
				
}

/**
  * @brief  System Power Configuration
  *         The system Power is configured as follow :
  *            + Regulator in LP mode
  *            + Fast wakeup enabled
  *            + HSI as SysClk after Wake Up
  * @param  None
  * @retval None
  */
void SystemPower_Config_Before_LowPower(void)
{
  #ifdef ENABLE_ULTRA_LOW_POWER
    HAL_PWREx_EnableUltraLowPower(); //Enable Ultra low power (ULP) mode (the reference voltage is also turned off)
  #endif
  
  #ifdef ENABLE_FAST_WAKEUP
    HAL_PWREx_EnableFastWakeUp();       //Enable the fast wake up from Ultra low power mode (there is ia short period of about 3ms - see reference manual to confirm) during which the reference voltage is still not accurate because it taskes time to stabelize.
  #endif
    
  //Prepare_MCU_Pins_For_LowPowerMode();  // Set the pins for low power mode
}

/**
  * @brief  This function puts the device into the low power mode called STOP Mode
  * @param  None
  * @retval None
  */
void Enter_STOP_Mode(void)
{
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI); // Enter Stop Mode, set the regulator in low power mode and the exit of stop mode by interrupt
}

/**
  * @brief  Configures system elements after wake-up from STOP
  * @param  None
  * @retval None
  */
void System_Config_After_STOP(void)
{
  SystemClockConfig_After_STOP();
  SystemGPIOs_After_STOP();       // reconfigure the pins after STOP mode
}

/**
  * @brief  Configures system clock after wake-up from STOP
  * @param  None
  * @retval None
  */
void SystemClockConfig_After_STOP(void)
{
	/*#############################################
	
	THIS FUNCTION WORKS BUT IT IS VERY SPECIFIC FOR THIS CODE.
	IT IS POSSIBLE TO MAKE IT MORE MODULAR BY SENDING OUT THE CONFIGURATIONS OF
	RCC_ClkInitTypeDef, RCC_OscInitTypeDef AND RCC_PeriphCLKInitTypeDef THAT ARE
	SET IN THE FUNCTION SystemClock_Config() IN main.c. 
	AS AN EXAMPLE, SEE THE FUNCTION Set_RTCAlarmTypeDef() THAT DOES JUST THIS
	BUT FOR THE RTCAlarmTypeDef.
	
	#############################################*/
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit;
  
  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Poll VOSF bit of in PWR_CSR. Wait until it is reset to 0 */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_VOS) != RESET) {};

  /* Get the Oscillators configuration according to the internal RCC registers */
  HAL_RCC_GetOscConfig(&RCC_OscInitStruct);

  /* After wake-up from STOP reconfigure the system clock: Enable HSI and PLL */
  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState 						= RCC_LSE_ON;
	RCC_OscInitStruct.LSIState 						= RCC_LSI_ON;
  RCC_OscInitStruct.HSEState            = RCC_HSE_OFF;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL4;
  RCC_OscInitStruct.PLL.PLLDIV          = RCC_PLL_DIV2;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
	
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART2
                              |RCC_PERIPHCLK_LPUART1|RCC_PERIPHCLK_RTC
                              |RCC_PERIPHCLK_USB;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
	
	/**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

}

/**
  * @brief  Configures system GPIO pins after wake-up from STOP
            Basically, this is just copy-->paste from MX_GPIO_Init(). 
            It is not possible to call MX_GPIO_Init() because it is set as
            static by the Cube and it is preferable not to meddle with the generated
            code.
  * @param  None
  * @retval None
  */
void SystemGPIOs_After_STOP(void)
{
  GPIO_Init();
}

/**
  * @brief  Configures MCU pins for low power mode to optimize power consumption
  * @param  None
  * @retval None
  */
void Prepare_MCU_Pins_For_LowPowerMode(void)
{
  HAL_GPIO_WritePin(EN_MCPU_GPIO_Port, EN_MCPU_Pin, GPIO_PIN_RESET);		//disabe U21 - 3.3V BB
	HAL_GPIO_WritePin(EN_12V_GPIO_Port, EN_12V_Pin, GPIO_PIN_SET);		//Disable U15 - 12V power line
	HAL_GPIO_WritePin(VDD_PER_EN_GPIO_Port, VDD_PER_EN_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);		//CS of CC1101
	HAL_GPIO_WritePin(U_BTN_GPIO_Port, U_BTN_Pin, GPIO_PIN_RESET);		//User PB
}


/************************ (C) COPYRIGHT Magneto *****END OF FILE****/
