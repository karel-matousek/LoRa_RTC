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
#include <string.h>
#include "stm32wlxx_hal_uart.h"
#include <stddef.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
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
#define BEACON_PREAMBLE		10			// Preamble 10 symbols
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

uint16_t timeCrcCheck(uint8_t *data, size_t length);

/* USER CODE END PFP */

/* Exported functions ---------------------------------------------------------*/
void SubghzApp_Init(void)
{
  /* USER CODE BEGIN SubghzApp_Init_1 */
	printf("\r\n=== STM32WL LoRa Receiver ===\r\n");
	printf("SubghzApp_Init: initializing radio for LoRaWAN beacon listening\r\n");

	//RBI_Init();
	//RBI_ConfigRFSwitch(RBI_SWITCH_RX);

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

  Radio.SetModem(MODEM_LORA);

  // Set channel
  Radio.SetChannel(BEACON_FREQUENCY);

  // Receive parameters configuration
  Radio.SetRxConfig(MODEM_LORA, 		// Modem: 								LoRa
		  	  	  	BEACON_BW, 			// Bandwidth: 							125 kHz
					BEACON_SF, 			// Spreading factor: 					9
					BEACON_CR, 			// Coderate: 							4/5
					0, 					// BandwidthAfc (FSK only): 			0 (N/A)
					BEACON_PREAMBLE, 	// Preamble length: 					10
					0, 					// Timeout symbols: 					0
					true, 				// Packet length: 						fixed
					17, 				// Payload length: 						17
					false, 				// CRC: 								off
					false, 				// Frequency hopping: 					off
					0, 					// Number of symbols between each hop: 	0
					false,		 		// IQ signal: 							non-inverted
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

	static uint8_t timeData[] = {0x00, 0x00, 0x00, 0x00};
	static char text_buffer[20];
	uint8_t timeCrc[] = {0x00, 0x00};

	// Same packet control
	uint8_t controlTimeData[] = {0x00, 0x00, 0x00, 0x00};

	for (uint8_t i = 0; i < 4; i++)
		{
			controlTimeData[i] = payload[i+2];
		}

	if(controlTimeData[0] == timeData[0] && controlTimeData[1] == timeData[1] && controlTimeData[2] == timeData[2] && controlTimeData[3] == timeData[3])
	{
		printf("\r\n=== SAME PACKET RECEIVED ===\r\n");
	}
	else
	{
		// Packet info
		printf("\r\n=== PACKET RECEIVED ===\r\n");
		printf("Size: %d bytes\r\nRSSI: %d dBm, SNR: %d dB\r\n", size, rssi, LoraSnr_FskCfo);
		printf("Data: ");
		for (uint16_t i = 0; i < size; i++)
		{
			printf("%02X ", payload[i]);
		}

		// Time data
		printf("\r\nTime data: ");
		for (uint8_t i = 0; i < 4; i++)
		{
			timeData[i] = payload[i+2];
			printf("%02X ", timeData[3-i]);
		}

		// Time CRC data
		printf("\r\nTime CRC data:       ");
		for (uint8_t i = 0; i < 2; i++)
		{
			timeCrc[i] = payload[i+6];
			printf("%02X ", timeCrc[i]);
		}

		// Calculation of CRC control
		uint16_t crcResult = timeCrcCheck(timeData, sizeof(timeData));
		uint8_t *crcBytes = (uint8_t *)&crcResult;
		printf("\r\nTime CRC calculated: ");
		for (uint8_t i = 0; i < sizeof(crcResult); i++)
		{
			printf("%02X ", crcBytes[i]);
		}

		if (crcBytes[0] == timeCrc[0] && crcBytes[1] == timeCrc[1])
		{
			printf("\r\n--- TIME VALID ---\r\n");

			// Time decode
			uint32_t timeUnformatted = 0;

			timeUnformatted |= timeData[3] << 24;
			timeUnformatted |= timeData[2] << 16;
			timeUnformatted |= timeData[1] << 8;
			timeUnformatted |= timeData[0];

			timeUnformatted += 3600; // + 1 hour (Czech Republic time)
			timeUnformatted -= 16;	 // - 16 seconds

			uint32_t secondsInDay = 86400;

			uint32_t secondsToday = timeUnformatted % secondsInDay;
			uint8_t hours = (secondsToday / 3600);
			uint8_t minutes = (secondsToday % 3600) / 60;
			uint8_t seconds = (secondsToday % 3600) % 60;

			printf("\r\nTime: %02u:%02u:%02u\r\n", hours, minutes, seconds);

			// OLED
			sprintf(text_buffer, "%02u:%02u:%02u\r\n", hours, minutes, seconds);
			ssd1306_SetCursor(0, 0);
			ssd1306_WriteString(text_buffer, Font_11x18, White);

			sprintf(text_buffer, "Time valid");
			ssd1306_SetCursor(2, 30);
			ssd1306_WriteString(text_buffer, Font_6x8, White);

			ssd1306_UpdateScreen();
		}
		else
		{
			printf("\r\n-- TIME INVALID --\r\n");
		}

		// End of message
		printf("\r\n=======================\r\n");
	}

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
uint16_t timeCrcCheck(uint8_t *data, size_t length)
{
	const uint16_t crcPolynom = 0x1021;	//P(x) = x^16 + x^12 + x^5 + x^0
	uint16_t crcReg = 0x0000;

	for(size_t i = 0; i < length; i++)
	{
		crcReg ^= ((uint16_t)data[i] << 8);	// XOR current byte with higher byte of register

		for(uint8_t j = 0; j < 8; j++)
		{
			if(crcReg & 0x8000)	// MSB check
			{
				crcReg = (crcReg << 1) ^ crcPolynom;
			}
			else
			{
				crcReg = (crcReg << 1);
			}
		}
	}

	return crcReg;
}
/* USER CODE END PrFD */
