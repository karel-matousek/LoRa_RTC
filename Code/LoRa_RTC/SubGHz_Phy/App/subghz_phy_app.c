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
#include <string.h>
#include <stddef.h>
#include <main.h>
#include <inttypes.h>

#include "radio_board_if.h"
#include "stm32wlxx_hal_uart.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "utils.h"
#include "stm32wlxx_hal_tim.h"
#include "time_format.h"
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */
extern uint32_t time_unform;
extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart1;
extern uint16_t tmr_pers;
extern time_date_t td;
extern int32_t change;
extern uint8_t update_display_flag;
extern uint32_t timer16_periods;
extern TIM_HandleTypeDef htim16;
extern uint32_t sec_start;
extern uint32_t tim_per;
extern uint8_t pulse_state;
extern uint32_t nom_per;
extern uint16_t tim_overflow;
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
#define PAYLOAD_LENGTH		17			// Payload length 17 symbols

#define MAX_INTEGRAL	10000

#define DEBUG_PRINT

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

uint16_t time_crc_check(uint8_t *data, size_t length);
void register_timer(TIM_HandleTypeDef *htim);

/* USER CODE END PFP */

/* Exported functions ---------------------------------------------------------*/
void SubghzApp_Init(void)
{
  /* USER CODE BEGIN SubghzApp_Init_1 */
#ifdef DEBUG_PRINT
	printf("\r\n=== STM32WL LoRa Receiver ===\r\n");
	printf("SubghzApp_Init: initializing radio for LoRaWAN beacon listening\r\n");
#endif

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
	Radio.SetRxConfig(
			MODEM_LORA, 			// Modem: 								LoRa
			BEACON_BW, 				// Bandwidth: 							125 kHz
			BEACON_SF, 				// Spreading factor: 					9
			BEACON_CR, 				// Coderate: 							4/5
			0, 						// BandwidthAfc (FSK only): 			0 (N/A)
			BEACON_PREAMBLE, 		// Preamble length: 					10
			0, 						// Timeout symbols: 					0
			true, 					// Packet length: 						fixed
			PAYLOAD_LENGTH,			// Payload length: 						17
			false, 					// CRC: 								off
			false, 					// Frequency hopping: 					off
			0, 						// Number of symbols between each hop: 	0
			false,		 			// IQ signal: 							non-inverted
			true);					// Reception mode: 						continuous

	// Set public network syncword (0x34)
	Radio.Write(REG_LR_SYNCWORD, 0x34);

	// Start continuous receive
	Radio.RxBoosted(0);
#ifdef DEBUG_PRINT
	printf("RX activated\r\n");
#endif

  /* USER CODE END SubghzApp_Init_2 */
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private functions ---------------------------------------------------------*/
static void OnTxDone(void)
{
  /* USER CODE BEGIN OnTxDone */
  /* USER CODE END OnTxDone */
}

static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo)
{
  /* USER CODE BEGIN OnRxDone */

	uint32_t timestamp = __HAL_TIM_GET_COUNTER(&htim2);
	uint32_t ticks_slr = timestamp - sec_start;	// Ticks since last timer reset
	uint32_t time_unform_local = time_unform;

	static uint8_t phase_flag = 0;

	/*======================================================== DATA EXTRACTION AND CRC CALCULATION ========================================================*/

	uint8_t time_data[4];
	uint8_t time_crc[2];

	// Time data extraction
	for (uint8_t i = 0; i < sizeof(time_data); i++) {
			time_data[i] = payload[i + 2];
	}

	// Time CRC data extraction
	for (uint8_t i = 0; i < sizeof(time_crc); i++) {
			time_crc[i] = payload[i + 6];
	}

	// CRC control calculation
	static uint16_t last_crc_result = 0;
	static uint8_t bcn_rx_enabled_flag = 0;
	uint16_t crc_result = time_crc_check(time_data, sizeof(time_data));
	uint8_t *crc_bytes = (uint8_t*) &crc_result;

	if(crc_result == last_crc_result) bcn_rx_enabled_flag = 0;
	else bcn_rx_enabled_flag = 1;
	last_crc_result = crc_result;

/*
#ifdef DEBUG_PRINT
	// Packet info
	printf("\r=== PACKET RECEIVED ===\r\n\n");
	printf("Size: %d bytes\r\nRSSI: %d dBm, SNR: %d dB\r\n", size, rssi, LoraSnr_FskCfo);
	printf("Data: ");
	for (uint8_t i = 0; i < size; i++) {
		printf("%02X ", payload[i]);
	}

	// Time data
	printf("\r\nTime data: ");
	for (uint8_t i = 0; i < 4; i++) {
		printf("%02X ", time_data[3 - i]);
	}

	// Time CRC data
	printf("\r\nTime CRC data: ");
	for (uint8_t i = 0; i < 2; i++) {
		printf("%02X ", time_crc[i]);
	}

	// Calculation of CRC control
	printf("\r\nTime CRC calculated: ");
	for (uint8_t i = 0; i < sizeof(crc_result); i++) {
		printf("%02X ", crc_bytes[i]);
	}
#endif
*/

	/*======================================================== TIME DECODING AND REGULATING ========================================================*/

	if (crc_bytes[0] == time_crc[0] && crc_bytes[1] == time_crc[1] && bcn_rx_enabled_flag == 1) {
		bcn_rx_enabled_flag = 0;

		static uint32_t prev_timestamp = 0;

		uint32_t bcn_time_unform = 0;
		bcn_time_unform |= time_data[3] << 24;
		bcn_time_unform |= time_data[2] << 16;
		bcn_time_unform |= time_data[1] << 8;
		bcn_time_unform |= time_data[0];

		static uint16_t secs_btw_bcns = 0;
		static uint32_t prev_bcn_time = 0;

		static int32_t offset = 0;
//		static int32_t prev_offset = 0;
//		static int64_t integral_err = 0;
//		static int32_t p_term = 0;
//		static int32_t i_term = 0;

//		static int64_t offset_sum = 0;
//		static int32_t offset_avg = 0;

		static uint64_t per_sum = 0;
		static uint32_t per_avg = 0;
		static uint16_t bcn_cnt = 0;

//		static uint16_t secs_cnt = 0;

		// Timer period configuration
//		if (phase_flag > 2) {
//
//			// Counter of seconds between beacons
//			secs_btw_bcns = (uint16_t)(bcn_time_unform - prev_bcn_time);
//			if (secs_btw_bcns == 0) secs_btw_bcns = 128;
//
//			// Offset calculation
//			offset = (int32_t)(timestamp - sec_start) + (int32_t)tim_per * (int32_t)(time_unform - bcn_time_unform);
//			int32_t offset_correction = offset / 128;
//
//			// PI values calculation
//			integral_err += offset_correction;
//			p_term = offset_correction / 64;
//			i_term = integral_err / 512;
//
//			// PPS correction
//			tim_per = (uint32_t)((int32_t)nom_per + p_term + i_term);
//			if (pulse_state == 0) __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, sec_start + tim_per);
//
//			// Average period calculation
//			secs_btw_bcns = (uint16_t)(bcn_time_unform - prev_bcn_time);
//			uint64_t ticks_btw_bcns = (uint64_t)(timestamp - prev_timestamp) + 0xFFFFFFFF;
//			tim_overflow = 0;
//
//			per_sum += (uint64_t)(ticks_btw_bcns / secs_btw_bcns);
//			per_avg = (uint64_t)(per_sum / (bcn_cnt - 1));
//
//			// Beacons counting
//			beacons_received ++;
//			current_beacon_time = time_unformatted;
//			beacons_passed = (uint16_t)(1 + (current_beacon_time - first_bcn_time) / 128);
//		}

		// Phase 3
		if (phase_flag > 1) {
			secs_btw_bcns = (uint16_t)(bcn_time_unform - prev_bcn_time);
			int64_t diff_timestamp = (int64_t)((int32_t)timestamp - (int32_t)prev_timestamp);
			uint64_t expected_ticks = (uint64_t)secs_btw_bcns * (uint64_t)tim_per;
			int32_t correction = (int32_t)(diff_timestamp - (int64_t)expected_ticks);
			uint64_t ticks_btw_bcns = expected_ticks + correction;

			// Period calculation
			nom_per = (uint32_t)((ticks_btw_bcns + (uint64_t)(secs_btw_bcns / 2)) / (uint64_t)secs_btw_bcns);
			if (nom_per > 48400000) nom_per -= per_avg / 128;
			if (nom_per < 47850000) nom_per += per_avg / 128;

			// Offset calculation
			offset = (int32_t)((int64_t)ticks_slr + (int64_t)tim_per * (int64_t)(time_unform_local - bcn_time_unform));
//			if (phase_flag > 2 && ((offset - prev_offset) > 20000000 || (offset - prev_offset) < 20000000)) {
//				if (offset > prev_offset) offset -= per_avg;
//				if (offset < prev_offset) offset += per_avg;
//			}
//			prev_offset = offset;

			// Average period calculation
			per_sum += (uint64_t)nom_per;
			per_avg = (uint32_t)(per_sum / (uint64_t)bcn_cnt);

//			// Average offset calculation
//			offset_sum += (int64_t)offset;
//			offset_avg = (int32_t)(offset_sum / ((int64_t)bcn_cnt - 1));

			// Offset correction
			if (phase_flag > 15) {
				int32_t offset_corr = offset / 1024;
				per_avg += offset_corr;
			}

			// Timer period configuration
			tim_per = per_avg;
			update_display_flag = 1;

			phase_flag ++;
		}

		// Phase 2
		else if (phase_flag == 1) {
			secs_btw_bcns = (uint16_t)(bcn_time_unform - prev_bcn_time);
			int64_t diff_timestamp = (int64_t)((int32_t)timestamp - (int32_t)prev_timestamp);
			uint64_t expected_ticks = (uint64_t)secs_btw_bcns * (uint64_t)tim_per;
			int32_t correction = (int32_t)(diff_timestamp - (int64_t)expected_ticks);
			uint64_t ticks_btw_bcns = expected_ticks + correction;

			nom_per = (uint32_t)((ticks_btw_bcns + (secs_btw_bcns / 2)) / secs_btw_bcns);
			if (nom_per > 48400000 || nom_per < 47850000) nom_per = 48154224;

			// Average period calculation
			per_sum += (uint64_t)nom_per;
			per_avg = (uint32_t)(per_sum / (uint64_t)bcn_cnt);

			// Timer reset
			tim_per = per_avg;
			__HAL_TIM_SET_COUNTER(&htim2, 0);
			sec_start = 0;
			timestamp = 0;
			pulse_state = 0;
			__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, sec_start + tim_per);

			time_unform = bcn_time_unform;
			update_display_flag = 1;

			phase_flag ++;
		}

		// Phase 1
		else {
			HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);

			time_unform = bcn_time_unform;
			update_display_flag = 1;

			phase_flag ++;
		}

		prev_timestamp = timestamp;
		prev_bcn_time = bcn_time_unform;
		bcn_cnt ++;


	/*======================================================== SERIAL PRINTING ========================================================*/

#ifdef DEBUG_PRINT
		format_time(bcn_time_unform, &td);

//		printf("\r\n");
//		printf("Received time: %02u:%02u:%02u: ", td.hours, td.minutes, td.seconds);
//		printf("Received time unformatted: %" PRIu32 ": ", bcn_time_unform);
//		printf("Assumed time unformatted: %" PRIu32 ": ", time_unform);
//		printf("Seconds between beacons: %" PRIu16 ": ", secs_btw_bcns);
		printf("Nominal period calculated: %" PRIu32 ": ", nom_per);
		printf("Average period: %" PRIu32 ": ", per_avg);
//		printf("Number of beacons received: %" PRIu16 ": ", bcn_cnt);
//		printf("Ticks since start of the last second: %" PRIu32 ": ", ticks_slr);
		printf("Offset: %" PRId32 ": ", offset);
//		printf("Average offset: %" PRId32 "\r\n", offset_avg);
//		printf("Timer overflow: %" PRIu16 "\r\n", tim_overflow);
//		printf("New timer period: %" PRIu32 ": ", tim_per);
//		printf("P term: %" PRId32 ": ", p_term);
//		printf("I term: %" PRId32 "\r\n", i_term);


//		printf("\r\n--- TIME VALID ---\r\n");
//		printf("Previous timer period: %" PRIu32 "\r\n", ticks_in_period);
//		printf("Timer periods since last beacon counted: %" PRIu16 "\r\n", periods_since_last_beacon);
//		printf("Total periods counted: %" PRIu16 "\r\n", secs_cnt);
//		printf("Timestamp: %" PRIu32 "\r\n", timestamp);
//		printf("Previous timestamp: %" PRIu32 "\r\n", prev_timestamp);
//		printf("Number of received beacons: %u\r\n", beacons_received);
//		printf("Number of passed beacons: %u\r\n", beacons_passed);
#endif

		tim_overflow = 0;
	} else {
#ifdef DEBUG_PRINT
//		printf("-- TIME INVALID --\r\n");
#endif
	}

	// End of message
#ifdef DEBUG_PRINT
//	printf("\r\n=======================\r\n\n");
#endif

	Radio.RxBoosted(0);
  /* USER CODE END OnRxDone */
}

static void OnTxTimeout(void)
{
  /* USER CODE BEGIN OnTxTimeout */
  /* USER CODE END OnTxTimeout */
}

static void OnRxTimeout(void)
{
  /* USER CODE BEGIN OnRxTimeout */
#ifdef DEBUG_PRINT
	printf("RX timeout\n\r");
#endif
	Radio.RxBoosted(0);
  /* USER CODE END OnRxTimeout */
}

static void OnRxError(void)
{
  /* USER CODE BEGIN OnRxError */
#ifdef DEBUG_PRINT
	printf("RX error\n\r");
#endif
	Radio.RxBoosted(0);
  /* USER CODE END OnRxError */
}

/* USER CODE BEGIN PrFD */
// Function for CRC calculation
uint16_t time_crc_check(uint8_t *data, size_t length) {
	const uint16_t crc_polynom = 0x1021;	//P(x) = x^16 + x^12 + x^5 + x^0
	uint16_t crc_reg = 0x0000;

	for (size_t i = 0; i < length; i++) {
		crc_reg ^= ((uint16_t) data[i] << 8);	// XOR current byte with higher byte of register

		for (uint8_t j = 0; j < 8; j++) {
			if (crc_reg & 0x8000) crc_reg = (crc_reg << 1) ^ crc_polynom;	// MSB check
			else crc_reg = (crc_reg << 1);
		}
	}

	return crc_reg;
}
/* USER CODE END PrFD */
