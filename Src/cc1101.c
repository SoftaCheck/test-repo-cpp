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
  * @file           : cc1101.c
  * @brief          : This file contains functions related to the cc101 IC.
  ******************************************************************************
**/

/* [] BEGINNING OF FILE */

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"
#include "cc1101.h"
#include "common.h"
#include "ccpacket.h"
#include "spi.h"
#include "cc1101regs.h"
#include "GW_BRProtocolsImp.h"
#include "modbus_app_layer.h"
#include "modbus_driver.h"
#include "State_machine.h"
#include "encryption.h"

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
  uint16_t  addr;
  uint8_t   data;
}registerSetting_t;

/* Private define ------------------------------------------------------------*/

#define PA_TABLE {0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}

/* Private variables ---------------------------------------------------------*/

	char PA[] =  {0x60};
	const unsigned char PA_LEN = 1;
	/* Sync word qualifier mode = 30/32 sync word bits detected */
	/* CRC autoflush = FALSE */
	/* Channel spacing = 199.951172 */
	/* Data format = Normal mode */
	/* Data rate = 512.573 */
	/* RX filter BW = 58.035714 */
	/* PA ramping = FALSE */
	/* Preamble count = 4 */
	/* Whitening = FALSE */
	/* Address config = No address check */
	/* Carrier frequency = 433.999969 */
	/* Device address = 0 */
	/* TX power = 0 */
	/* Manchester enable = TRUE */
	/* CRC enable = TRUE */
	/* Deviation = 5.157471 */
	/* Packet length mode = Variable packet length mode. Packet length configured by the first byte after sync word */
	/* Packet length = 255 */
	/* Modulation format = GFSK */
	/* Base frequency = 433.999969 */
	/* Modulated = TRUE */
	/* Channel number = 0 */
//static const registerSetting_t preferredSettings[] = 
registerSetting_t preferredSettings[] = 
{ 
	 
  {CC1101_IOCFG0,       0x06},
  {CC1101_FIFOTHR,      0x47},
  {CC1101_PKTCTRL0,     0x05}, // variable packet length
	{CC1101_PKTCTRL1,     0x06}, // Address check and 0 (0x00) broadcast + append as default
  {CC1101_FSCTRL1,      0x06},
  {CC1101_FREQ2,        0x10},
  {CC1101_FREQ1,        0xB1},
	{CC1101_FREQ0,        0x3B},
	{CC1101_ADDR,         GATEWAY_ID},   // this device address 10
  {CC1101_MDMCFG4,      0xF5},
  {CC1101_MDMCFG3,      0x75},
  {CC1101_MDMCFG2,      0x13},
  {CC1101_MDMCFG0,      0xE5},
  {CC1101_DEVIATN,      0x14},
  {CC1101_MCSM0,        0x18},
  {CC1101_FOCCFG,       0x16},
  {CC1101_RESERVED_0X20,0xFB},
  {CC1101_FSCAL3,       0xE9},
  {CC1101_FSCAL2,       0x2A},
  {CC1101_FSCAL1,       0x00},
  {CC1101_FSCAL0,       0x1F},
  {CC1101_TEST2,        0x81},
  {CC1101_TEST1,        0x35},
  {CC1101_TEST0,        0x09},
  {CC11xL_PA_TABLE0,    0xC0},
};

//static bool m_sleepMode = FALSE;

/*
 * RF state
 */
uint8_t rfState;

/**
 * Carrier frequency
 */
uint8_t carrierFreq;

/**
 * Working mode (speed, ...)
 */
uint8_t workMode;

/**
 * Frequency channel
 */
uint8_t channel;

/**
 * Synchronization word
 */
uint8_t syncWord[2];

/**
 * Device address
 */
volatile uint8_t devAddress;


uint8_t RF_CRCsOKFlag = FALSE;
uint8_t RFReceiveFailFlag = FALSE;
uint8_t LocalBurstCounter = 0;
uint8_t RFReceiveDoneFlag = TRUE;
uint8_t Write2UARTFlag = FALSE;
uint8_t StartRXFlag = FALSE;
uint8_t PayloadSize4print = 0;
uint32_t timeSample4RFRecevingTimeout = 0;

/* Private macro -------------------------------------------------------------*/

/**
 * Macros
 */

// Wait until SPI MISO line goes low

// Get GDO0 pin state
#define getGDO0state()  CC1101_digitalRead(CC1101_GDO0)
// Wait until GDO0 line goes high
#define wait_GDO0_high()  while(!getGDO0state())
// Wait until GDO0 line goes low
#define wait_GDO0_low()  while(getGDO0state())
	
/**
 * Macros
 */
// Read CC1101 Config register
#define CC1101_readConfigReg(regAddr)    CC1101_readReg(regAddr, 0)
// Read CC1101 Status register
#define CC1101_readStatusReg(regAddr)    CC1101_readReg(regAddr , CC1101_STATUS_REGISTER)
// Enter Rx state
//#define setRxState()              CC1101_cmdStrobe(CC1101_SRX)
// Enter Tx state
//#define setTxState()              CC1101_cmdStrobe(CC1101_STX)

// Flush Rx FIFO
#define flushRxFifo()             CC1101_cmdStrobe(CC1101_SFRX)
// Flush Tx FIFO
#define flushTxFifo()             CC1101_cmdStrobe(CC1101_SFTX)
// Disable address check
#define disableAddressCheck()     CC1101_writeReg(CC1101_PKTCTRL1, 0x04)
// Enable address check
#define enableAddressCheck()      CC1101_writeReg(CC1101_PKTCTRL1, 0x06)
// Disable CCA
#define disableCCA()              CC1101_writeReg(CC1101_MCSM1, 0)
// Enable CCA
#define enableCCA()               CC1101_writeReg(CC1101_MCSM1, CC1101_DEFVAL_MCSM1)

/* Private function prototypes -----------------------------------------------*/

/* Exported functions ---------------------------------------------------------*/

void delayUs(int time)
{
	for(int i = 0 ; i < time * 100; i++)
	{
		  __asm("NOP");
	}
}


uint8_t CC1101_digitalRead(uint8_t signal)
{
	
	switch (signal)
	{
		case MISO:
		{
			
		}
		break;
		case CC1101_GDO0:
		{
			
		}
	  break;		
	}	
	return 1;	
}

void CC1101(int deviceAddress)
{
  carrierFreq = CFREQ_433;
  channel = CC1101_DEFVAL_CHANNR;
  syncWord[0] = CC1101_DEFVAL_SYNC1;
  syncWord[1] = CC1101_DEFVAL_SYNC0;
	if (deviceAddress == -1)
		 devAddress = CC1101_DEFVAL_BROADCAST_ADDR; 
	else {
		 devAddress = deviceAddress & 0xFF;
	}
}

static void wait_Miso()  
{ 
   delayUs(20);
}

static void delayMicroseconds(int delay)
{
	for (int i = 0 ; i < delay * ONE_MICROSEC_CYCLE; i++)
	{
		  __asm("NOP");
	}
  
}


/**
	 * CC1101_setTxPowerAmp
	 * 
	 * Set PATABLE value
	 * 
	 * @param paLevel amplification value
	 */
void CC1101_setTxPowerAmp(uint8_t paLevel)
{
	CC1101_writeReg(CC1101_PATABLE, paLevel);
}
 


/**
 * wakeUp
 * 
 * Wake up CC1101 from Power Down state
 */
void CC1101_wakeUp(void)
{
  CC1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  CC1101_Deselect();                    // Deselect CC1101
}

/**
 * CC1101_writeReg
 * 
 * Write single register into the CC1101 IC via SPI
 * 
 * 'regAddr'	Register address
 * 'value'	Value to be writen
 */

void Halt(char *str)
{
	printf("%s\n" , str);
	while (1){}
}

uint8_t ReadStatus()
{
	  return CC1101_cmdStrobe(CC1101_SNOP);	
}

uint8_t CC1100_setIdleState(void)
{
    uint8_t marcState;

	  CC1101_cmdStrobe(CC1101_SIDLE);
   
    marcState = 0xFF;                     //set unknown/dummy state value

	  int timeOut = 100;
    while(marcState != 0x01)              //0x01 = sidle
    {
			  marcState = CC1101_readStatusReg(CC1101_MARCSTATE) & 0x1F; //read out state of cc1100 to be sure in RX
			  if (timeOut == 0)
				{
					 printf("SetIdle marcState : %d\n", marcState);
					 return 0;
				}
				timeOut--;
    }
    //Serial.println();
    delayMicroseconds(100);
    return 1;
}
	
static void CC1101_writeReg(byte regAddr, byte value) 
{

	uint8_t status[2];
	
  CC1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low  
	SPI1_Transmit(regAddr, value, status);
	CC1101_Deselect();                    // Deselect CC1101

}

/**
 * writeBurstReg
 * 
 * Write multiple registers into the CC1101 IC via SPI
 * 
 * 'regAddr'	Register address
 * 'buffer'	Data to be writen
 * 'len'	Data length
 */
void CC1101_writeBurstReg(byte regAddr, byte* buffer, byte len)
{
  
  uint8_t status[len];
	
  CC1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
 
	SPI1_TransmitBurst(regAddr | WRITE_BURST, buffer, len, status);


  CC1101_Deselect();                    // Deselect CC1101  
}

/**
 * CC1101_cmdStrobe
 * 
 * Send command strobe to the CC1101 IC via SPI
 * 
 * 'cmd'	Command strobe
 */     
uint8_t CC1101_cmdStrobe(byte cmd) 
{
	byte status;
  CC1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  uint8_t res = SPI1_TransmitStrobe(cmd, &status);
	//printf("cmd probe status %x\n" , status);
  CC1101_Deselect();                    // Deselect CC1101
	
	return res;
}

/**
 * readReg
 * 
 * Read CC1101 register via SPI
 * 
 * 'regAddr'	Register address
 * 'regType'	Type of register: CC1101_CONFIG_REGISTER or CC1101_STATUS_REGISTER
 * 
 * Return:
 * 	Data byte returned by the CC1101 IC
 */
byte CC1101_readReg(byte regAddr, byte regType)
{
  byte val;
	byte status;	
  CC1101_Select();                      // Select CC1101		
	wait_Miso();                          // Wait until MISO goes low	
	SPI1_Receive(regAddr | regType, &val, &status);
	
  //printf("status %x  val = %d\n" , status, val);
  CC1101_Deselect();                    // Deselect CC1101

  return val;
}


 

/**
 * readBurstReg
 * 
 * Read burst data from CC1101 via SPI
 * 
 * 'buffer'	Buffer where to copy the result to
 * 'regAddr'	Register address
 * 'len'	Data length
 */
void CC1101_readBurstReg(byte *buffer, byte regAddr, byte len) 
{
  byte addr;
  byte status[len];
  addr = regAddr | READ_BURST;
  CC1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
	SPI1_ReceiveBurst(addr, buffer , len, status);
	
  CC1101_Deselect();                    // Deselect CC1101
}

/**
 * 
 * Reset CC1101
 */
void CC1101_reset(void) 
{

	
	/*

	from the datasheet: 	
	
	Set SCLK = 1 and SI = 0, to avoid
  potential problems with pin control mode
 (see Section 11.3).
 ? Strobe CSn low / high.
 ? Hold CSn low and then high for at least 40
 µs relative to pulling CSn low
 ? Pull CSn low and wait for SO to go low
 (CHIP_RDYn).
 ? Issue the SRES strobe on the SI line.
 ? When SO goes low again, reset is
 complete and the chip is in the IDLE state.
	
	*/
	 
	 
	CC1101_Deselect();
  delayUs(2000);
	CC1101_Select();
	delayUs(2000);
	CC1101_Deselect();
		
  
  wait_Miso();                          // Wait until MISO goes low
	
	 
	CC1101_Select();		
	uint8_t status;  
	SPI1_TransmitStrobe(CC1101_SRES , &status);
  wait_Miso();             
	// When SO goes low again, reset is
  // complete and the chip is in the IDLE state.

  CC1101_Deselect();                    // Deselect CC1101

 
}

/**
 * setCCregs
 * 
 * Configure CC1101 registers
 */

void CC1101_SettingsFromMSP430(void)
{

	 uint8_t writeByte;
//   uint8_t readByte;
	#if 0 // check burst read write and single read write  - pass all
	uint8_t temp[5] = {11,12,13,14,15};
	uint8_t temp1[5] = {0};
	CC1101_writeBurstReg(CC1101_IOCFG2,temp, sizeof(temp));
	CC1101_readBurstReg(temp1, CC1101_IOCFG2, sizeof(temp1));
	
	while (1){}
	#endif 
	
	
	 int i;
#ifdef PA_TABLE
   uint8_t paTable[] = PA_TABLE;
#endif
	
	for(i = 0; i < (sizeof  preferredSettings/sizeof(registerSetting_t)); i++) 
	{
			writeByte =  preferredSettings[i].data;
			CC1101_writeReg( preferredSettings[i].addr, writeByte);
  }
	
	
  for(i = 0; i < (sizeof  preferredSettings/sizeof(registerSetting_t)); i++) 
	{
     //readByte = CC1101_readConfigReg( preferredSettings[i].addr);
		 //printf("Address %d = 0x%x\n",preferredSettings[i].addr, readByte); 
  }
	
	
	#ifdef PA_TABLE
		 // write PA_TABLE
		 CC1101_writeBurstReg(CC11xL_PA_TABLE0,paTable, sizeof(paTable));
	#endif
	
}
 


/**
 * init
 * 
 * Initialize CC1101 radio
 *
 * @param freq Carrier frequency
 * @param mode Working mode (speed, ...)
 */
void CC1101_init(uint8_t freq, uint8_t mode)
{
  carrierFreq = freq;
  workMode = mode;
  
  //pinMode(CC1101_GDO0, INPUT);          // Config GDO0 as input

  CC1101_reset();                              // Reset CC1101

  
}

/**
 * CC1101_setSyncWord
 * 
 * Set synchronization word
 * 
 * 'syncH'	Synchronization word - High byte
 * 'syncL'	Synchronization word - Low byte
 */
void CC1101_setSyncWord1(uint8_t syncH, uint8_t syncL) 
{
  CC1101_writeReg(CC1101_SYNC1, syncH);
  CC1101_writeReg(CC1101_SYNC0, syncL);
  syncWord[0] = syncH;
  syncWord[1] = syncL;
}

/**
 * CC1101_setSyncWord2 (overriding method)
 * 
 * Set synchronization word
 * 
 * 'syncH'	Synchronization word - pointer to 2-byte array
 */
void CC1101_setSyncWord2(byte *sync) 
{
  CC1101_setSyncWord1(sync[0], sync[1]);
}

/**
 * setDevAddress
 * 
 * Set device address
 * 
 * @param addr	Device address
 */
void CC1101_setDevAddress(byte addr) 
{
  CC1101_writeReg(CC1101_ADDR, addr);
  devAddress = addr;
}

/**
 * setChannel
 * 
 * Set frequency channel
 * 
 * 'chnl'	Frequency channel
 */
void CC1101_setChannel(byte chnl) 
{
  CC1101_writeReg(CC1101_CHANNR,  chnl);
  channel = chnl;
}

bool CC1101_IsExist(void)
{
	
	byte version = CC1101_readReg(CC1101_VERSION, CC1101_STATUS_REGISTER);
	printf("version = %d\n\r", version);
	//HAL_Delay(1000);	//Eyal Gerber: This delay is irrelevant.
	if (version == 0x14 /*0x14*/) // the version is subject to change , in the datasheet its 20
		return 1;
	
	return 0;
}

/**
 * setCarrierFreq
 * 
 * Set carrier frequency
 * 
 * 'freq'	New carrier frequency
 */
void CC1101_setCarrierFreq(byte freq)
{
  switch(freq)
  {
    case CFREQ_915:
      CC1101_writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_915);
      CC1101_writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_915);
      CC1101_writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_915);
      break;
    case CFREQ_433:
      CC1101_writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_433);
      CC1101_writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_433);
      CC1101_writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_433);
      break;
    case CFREQ_918:
      CC1101_writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_918);
      CC1101_writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_918);
      CC1101_writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_918);
      break;
    default:
      CC1101_writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_868);
      CC1101_writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_868);
      CC1101_writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_868);
      break;
  }
   
  carrierFreq = freq;  
}

/**
 * setPowerDownState
 * 
 * Put CC1101 into power-down state
 */
void CC1101_setPowerDownState(void) 
{
  // Comming from RX state, we need to enter the IDLE state first
  CC1101_cmdStrobe(CC1101_SIDLE);
  // Enter Power-down state
  CC1101_cmdStrobe(CC1101_SPWD);
	
}




/**
 * receiveData
 * 
 * Read data packet from RX FIFO
 *
 * 'packet'	Container for the packet received
 * 
 * Return:
 * 	Amount of bytes received
 */
byte CC1101_receiveData1(CCPACKET * packet)
{
  byte val;
  byte rxBytes = CC1101_readStatusReg(CC1101_RXBYTES);
	printf("rxBytes: %d\n" , rxBytes);

  // Any byte waiting to be read and no overflow?
  if (rxBytes & 0x7F && !(rxBytes & 0x80))
  {
    // Read data length
    packet->length = CC1101_readConfigReg(CC1101_RXFIFO);
    // If packet is too long
    if (packet->length > CCPACKET_DATA_LEN)
      packet->length = 0;   // Discard packet
    else
    {
      // Read data packet
      CC1101_readBurstReg(packet->data, CC1101_RXFIFO, packet->length);
      // Read RSSI
      packet->rssi = CC1101_readConfigReg(CC1101_RXFIFO);
      // Read LQI and CRC_OK
      val = CC1101_readConfigReg(CC1101_RXFIFO);
      packet->lqi = val & 0x7F;
      packet->crc_ok = bitRead(val, 7);
    }
  }
  else
    packet->length = 0;

  CC1100_setIdleState();       // Enter IDLE state
  flushRxFifo();        // Flush Rx FIFO
  //cmdStrobe(CC1101_SCAL);

  // Back to RX state
  CC1101_setRxState();

  return packet->length;
}

/**
 * setRxState
 * 
 * Enter Rx state
 */
void CC1101_setRxState(void)
{
  CC1101_cmdStrobe(CC1101_SRX);
  rfState = RFSTATE_RX;
}
/**
 * setTxState
 * 
 * Enter Tx state
 */
void CC1101_setTxState(void)
{
  CC1101_cmdStrobe(CC1101_STX);
  rfState = RFSTATE_TX;
}



/***********************************************************

Function name: CC1101_receiveData

Function type: byte

Arguments: uint8_t *rxData, uint8_t *overallSize

Return: byte, depende on the amount of data received.

Description: This function is responsible of receiving
the data that come over RF.

**********************************************************/
byte CC1101_receiveData(uint8_t *rxData, uint8_t *overallSize)
{
	 #ifdef GENERAL_INDICATIONS
		printf("within CC1101 receive function\n\r");
	 #endif
	 uint8_t RXTempArr[64] = {0};
   uint8_t rxBytes;
   uint8_t rxBytesVerify;	 	 
	 // set radio back in RX
   CC1101_setRxState();
	 StartRXFlag = FALSE;
	 rxBytesVerify = CC1101_readStatusReg(CC1101_RXBYTES);
	 uint32_t loopTimeout = HAL_GetTick();
	 do
	 {
		 rxBytes = rxBytesVerify;
		 rxBytesVerify = CC1101_readStatusReg(CC1101_RXBYTES);
	 }
	 while((rxBytes != rxBytesVerify) && (HAL_GetTick() - loopTimeout < CC1101_RECEIVE_LOOP_TIMEOUT));
	 if (rxBytes == 0)
	 {
		  printf("no data\n\r");
		  return  0x03;
	 }
	 
	 if (rxBytes > 64)
	 {
		 printf("error in rx size is > 64\n");
		 //flushRxFifo();
		 return 0;
	 }
	
	 
	 uint8_t TempArr4Encryption1[64] = {0};
	 uint8_t TempArr4Encryption2[RF_ONE_PAYLOAD_MAX_SIZE] = {0};
	 uint8_t TempArr4Encryption3[RF_ONE_PAYLOAD_MAX_SIZE] = {0};		 
	 
	 //CC1101_readBurstReg(RXTempArr , CC1101_RXFIFO, rxBytes); 
	 
	 CC1101_readBurstReg(TempArr4Encryption1 , CC1101_RXFIFO, rxBytes); 
	 
//	 for(uint8_t k=0;k<64;k++)
//	 {
//		 printf("TempArr4Encryption1[%d] = %x\n\r", k, TempArr4Encryption1[k]);
//	 }
	 
	 for(uint8_t i=5;i<PKTLEN-8;i++)
	 {
		 TempArr4Encryption2[i-5] = TempArr4Encryption1[i];		//Take out the encrypted part only
	 }
	 MX_AES_Init();		//The call for this function here is needed. We did that after revealing that writing to flash and AES nort working together without this function call. The error handler inside this function was removed.
	
	 Decrypt_Data(TempArr4Encryption2, RF_ONE_PAYLOAD_MAX_SIZE, TempArr4Encryption3);	 											//Dencrypte the data and move it to TempArr4Encryption3
	 
	 
	 uint8_t j=0;
	 for(j=0;j<5;j++)
	 {
		 RXTempArr[j] = TempArr4Encryption1[j];			//Copy the first 5 bytes that are NOT encrypted.
	 }
	 for(;j<53;j++)
	 {
		 RXTempArr[j] = TempArr4Encryption3[j-5];
	 }
	 for(;j<PKTLEN+1;j++)
	 {
		 RXTempArr[j] = TempArr4Encryption1[j];
	 }
	 #ifdef GENERAL_INDICATIONS
		printf("\n\r\n\r DATA CAME OVER RF - DECRYPTED\n\r\n\r");
		
	 #endif
	 
	 if(StartRXFlag == FALSE)		//Indicate receiving start
	 {
			StartRXFlag = TRUE;	
			RFReceiveFailFlag = FALSE;
		  RFReceiveDoneFlag = FALSE;
		  
	 }
	 if((RXTempArr[4] != (LocalBurstCounter) && (LocalBurstCounter <= (RXTempArr[2]/RF_ONE_PAYLOAD_MAX_SIZE)+1)))
	 {
		 RFReceiveFailFlag = TRUE;
		 #ifdef GENERAL_INDICATIONS
		 printf("LocalBurstCounter = %d\n\r", LocalBurstCounter);
		 printf("The number of burst is not OK\n\r");
		 #endif
	 }
		
	 if(CalcCRC(TempArr4Encryption1, PKTLEN-2) != (((RXTempArr[PKTLEN-1] <<8) & 0xFF00) | ((RXTempArr[PKTLEN-2] & 0x00FF))))//If the CRC of each burst are not the same
	 {
		 RFReceiveFailFlag = TRUE;
		 #ifdef GENERAL_INDICATIONS
		 printf("The CRC is not OK\n\r");
		 #endif
	 }
	 
	 
	 if(!RFReceiveFailFlag)
	 {
		 timeSample4RFRecevingTimeout = HAL_GetTick();		//Sample the time of the start of the current RF receiving.
		 LocalBurstCounter++;
		 
		 for(uint8_t i=0;i<PKTLEN-13;i++)
		 {
			 rxData[i+ (LocalBurstCounter-1)*(PKTLEN - 13)] = RXTempArr[i+5];		//Copy the new burst to its right location in the array to send over UART.
			 #ifdef GENERAL_INDICATIONS
			 printf("rxData[%d] = %x\n\r", (i+ (LocalBurstCounter-1)*(PKTLEN - 13)), rxData[i+ (LocalBurstCounter-1)*(PKTLEN - 13)]);
			 #endif
		 }
		 
		 if(LocalBurstCounter == ((RXTempArr[2]/RF_ONE_PAYLOAD_MAX_SIZE)+1))	//If all of the burst arrived, as calculated from the size of the incoming data (based on dedicated byte).
		 {
			 RFReceiveDoneFlag = TRUE;
			 LocalBurstCounter = 0;
		 }
		 
		 
		 if(CurrentNodeID == 0xFF || CurrentNodeID == RXTempArr[NODE_ID_LOC_IN_RF_MES+5])			//If the GW is avaliable to communicate or if the incoming data is from the Node that we currently speak to
		 {
			overallSize = &RXTempArr[3];				//The size of the current burst
			if(RFReceiveDoneFlag)
				RFData2AnalyzeFlag = TRUE;				//The data arrived is legal and shall be analyzed			
		 }
		 else
		 {
			 state = RF_RECEIVE_STATE;
		 }

	 }

  return 1;
}


uint8_t Data4Sensor(uint8_t *arr2analyze)
{
	uint8_t val2ret = 0;
 
		val2ret = 0;
	return val2ret;
	
}

/***********************************************************

Function name: CC1101_ReceiveRoutineFlagsInit

Function type: void

Arguments: None

Return: None

Description: This function is called if the timeout of RF
receiving has passed without being complete.

**********************************************************/
void CC1101_ReceiveRoutineFlagsInit(void)
{
	LocalBurstCounter = 0;		//Set the counter of the burst to be 0
	RFReceiveFailFlag = FALSE;
	RFReceiveDoneFlag = FALSE;
	
}

/************************ (C) COPYRIGHT AIO Systems *****END OF FILE****/

