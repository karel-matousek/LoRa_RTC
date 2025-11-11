/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    subghz_phy_app.c
  * @author  MCD Application Team
  * @brief   Application of the SubGHz_Phy Middleware
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "sys_app.h"
#include "subghz_phy_app.h"
#include "radio.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "radio_board_if.h"
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BEACON_FREQUENCY 	869525000	// Frequency 869525 kHz
#define BEACON_BW			0			// Bandwidth 125 kHz
#define BEACON_SF			9			// Spreading factor 9
#define BEACON_CR			1			// Code rate 4/5
#define BEACON_PREAMBLE		8			// Preamble 8 symbols
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Radio events function pointer */
static RadioEvents_t RadioEvents;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/*!
 * @brief Function to be executed on Radio Tx Done event
 */
static void OnTxDone(void);

/**
  * @brief Function to be executed on Radio Rx Done event
  * @param  payload ptr of buffer received
  * @param  size buffer size
  * @param  rssi
  * @param  LoraSnr_FskCfo
  */
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo);

/**
  * @brief Function executed on Radio Tx Timeout event
  */
static void OnTxTimeout(void);

/**
  * @brief Function executed on Radio Rx Timeout event
  */
static void OnRxTimeout(void);

/**
  * @brief Function executed on Radio Rx Error event
  */
static void OnRxError(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Exported functions ---------------------------------------------------------*/
void SubghzApp_Init(void)
{
  /* USER CODE BEGIN SubghzApp_Init_1 */
	printf("SubghzApp_Init: initializing radio for LoRaWAN beacon listening\r\n");

	RBI_Init();
	RBI_ConfigRFSwitch(RBI_SWITCH_RX);

  //memset(&RadioEvents, 0, sizeof(RadioEvents));	// Clear radio memory

  /* USER CODE END SubghzApp_Init_1 */

  /* Radio initialization */
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;

  Radio.Init(&RadioEvents);

  /* USER CODE BEGIN SubghzApp_Init_2 */

  // Set channel
  Radio.SetChannel(BEACON_FREQUENCY);

  // Receive parameters configuration
  Radio.SetRxConfig(MODEM_LORA, 		// Modem: 								LoRa
		  	  	  	BEACON_BW, 			// Bandwidth: 							125 kHz
					BEACON_SF, 			// Spreading factor: 					9
					BEACON_CR, 			// Coderate: 							4/5
					0, 					// BandwidthAfc (FSK only): 			0 (N/A)
					BEACON_PREAMBLE, 	// Preamble length: 					8
					0, 					// Timeout symbols: 					0
					false, 				// Packet length: 						variable
					0, 					// Payload length: 						0
					true, 				// CRC: 								on
					false, 				// Frequency hopping: 					off
					0, 					// Number of symbols between each hop: 	0
					true,		 		// IQ signal: 							inverted
					true);				// Reception mode: 						continuous

  // Set public network syncword (0x34)
  Radio.Write(REG_LR_SYNCWORD, 0x34);

  // Start continuous receive
  Radio.RxBoosted(0);
  printf("RX activated\r\n");

  /* USER CODE END SubghzApp_Init_2 */
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private functions ---------------------------------------------------------*/
static void OnTxDone(void)
{
  /* USER CODE BEGIN OnTxDone */
	printf("Radio: TX done\r\n");
	Radio.RxBoosted(0);
  /* USER CODE END OnTxDone */
}

static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo)
{
  /* USER CODE BEGIN OnRxDone */
	printf("\r\n=== PACKET RECEIVED ===\r\n");
	printf("Size: %d bytes\r\nRSSI: %d dBm, SNR: %d dB\r\n", size, rssi, LoraSnr_FskCfo);
	printf("Data: ");
	for (uint16_t i = 0; i < size; i++)
	{
		printf("%02X ", payload[i]);
	}
	printf("\r\n========================\r\n");

	Radio.RxBoosted(0);
  /* USER CODE END OnRxDone */
}

static void OnTxTimeout(void)
{
  /* USER CODE BEGIN OnTxTimeout */
	printf("TX timeout\r\n");
	Radio.RxBoosted(0);
  /* USER CODE END OnTxTimeout */
}

static void OnRxTimeout(void)
{
  /* USER CODE BEGIN OnRxTimeout */
	printf("RX timeout\n\r");
	Radio.RxBoosted(0);
  /* USER CODE END OnRxTimeout */
}

static void OnRxError(void)
{
  /* USER CODE BEGIN OnRxError */
	printf("RX error\n\r");
	Radio.RxBoosted(0);
  /* USER CODE END OnRxError */
}

/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */
