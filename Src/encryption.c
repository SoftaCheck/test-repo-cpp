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
  * @file           : encryption.c
  * @brief          : This is a user application driver for encryption
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "encryption.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

//CRYP_HandleTypeDef hcryp;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/

/**
  * @brief  This function encryptes an array of bytes. The function uses the polling
						method of the encryption module. Therefore, this function is blocking.
  * @param  uint8_t *InputArray	-	pointer to the array of bytes to encrypt
						uint16_t Byte_Num	- the number of bytes to encrypt
						uint8_t *EncryptedArray	- pointer to the encrypted array
  * @retval None
  */
void Encrypt_Data(uint8_t *InputArray, uint16_t Byte_Num, uint8_t *EncryptedArray)
{
	if (HAL_CRYP_DeInit(&hcryp) != HAL_OK)
  {
    #ifdef ENCRYPTION_LOGS
      printf("AES DeInit Failed\r\n");
    #endif
  }    
	
	MX_AES_Init();
	
	if (HAL_CRYP_AESECB_Encrypt(&hcryp, InputArray, Byte_Num, EncryptedArray, ENCRYPTION_TIMEOUT_VALUE) != HAL_OK)
	{
		#ifdef ENCRYPTION_LOGS
      printf("AES Encryption Failed\r\n");
    #endif
	}  
}

/**
  * @brief  This function decryptes an array of bytes. The function uses the polling
						method of the encryption module. Therefore, this function is blocking.
  * @param  uint8_t *EncryptedArray	-	pointer to the array of encrypted bytes
						uint16_t Byte_Num	- the number of bytes to decrypt
						uint8_t *DecryptedArray	- pointer to the decrypted array
  * @retval None
  */
void Decrypt_Data(uint8_t *EncryptedArray, uint16_t Byte_Num, uint8_t *DecryptedArray)
{
	if (HAL_CRYP_DeInit(&hcryp) != HAL_OK)
  {
    #ifdef ENCRYPTION_LOGS
      printf("AES DeInit Failed\r\n");
    #endif
  }    
	
	MX_AES_Init();
	
	if (HAL_CRYP_AESECB_Decrypt(&hcryp, EncryptedArray, Byte_Num, DecryptedArray, ENCRYPTION_TIMEOUT_VALUE) != HAL_OK)
	{
		#ifdef ENCRYPTION_LOGS
      printf("AES Decryption Failed\r\n");
    #endif
	}  
}
  
  
/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/
