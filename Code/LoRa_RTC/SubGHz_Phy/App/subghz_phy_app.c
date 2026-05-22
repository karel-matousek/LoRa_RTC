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
extern uint32_t time_unformatted_g;
extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart1;
extern uint16_t timer_periods;
extern time_date_t td;
extern int32_t change;
extern uint8_t update_display_flag;
extern uint32_t timer16_periods;
extern TIM_HandleTypeDef htim16;
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

#define NOMINAL_PERIOD 	48000000
#define MAX_INTEGRAL	10000000
#define MAX_CORRECTION	30000

#define DEBUG_PRINT

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Radio events function pointer */
static RadioEvents_t RadioEvents;
static uint16_t ticks_since_last_reset_timer16;
static uint16_t prev_ticks_since_last_reset_timer16;
static uint32_t prev_tim16_periods = 0;
/* USER CODE BEGIN PV */
//static TIM_HandleTypeDef *p_timer;
uint32_t prev_received_time;
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
	ticks_since_last_reset_timer16 = __HAL_TIM_GET_COUNTER(&htim16);
	uint32_t tim16_periods_local = timer16_periods;

	static uint8_t timer_running_flag = 0;

#ifdef DEBUG_PRINT
	static uint16_t beacons_received = 1;
	static uint16_t beacons_passed = 1;
	static uint32_t first_beacon_time = 0;
	static uint32_t current_beacon_time = 0;
#endif

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
	static uint8_t beacon_rx_allowed_flag = 0;
	uint16_t crc_result = time_crc_check(time_data, sizeof(time_data));
	uint8_t *crc_bytes = (uint8_t*) &crc_result;

	if(crc_result == last_crc_result) beacon_rx_allowed_flag = 0;
	else beacon_rx_allowed_flag = 1;
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
	/*======================================================== TIME DECODING AND FORMATTING ========================================================*/

	if (crc_bytes[0] == time_crc[0] && crc_bytes[1] == time_crc[1] && beacon_rx_allowed_flag == 1) {

		uint64_t ticks_between_beacons = (uint64_t)ticks_since_last_reset_timer16 + 65536 * ((uint64_t)tim16_periods_local - (uint64_t)prev_tim16_periods) - (uint64_t)prev_ticks_since_last_reset_timer16;
		prev_ticks_since_last_reset_timer16 = ticks_since_last_reset_timer16;
		prev_tim16_periods = tim16_periods_local;

		uint32_t time_unformatted = 0;
		time_unformatted |= time_data[3] << 24;
		time_unformatted |= time_data[2] << 16;
		time_unformatted |= time_data[1] << 8;
		time_unformatted |= time_data[0];

/*
#ifdef DEBUG_PRINT
		beacons_received ++;

		if (timer_running_flag == 0) {
			first_beacon_time = time_unformatted;
		}

		current_beacon_time = time_unformatted;

		beacons_passed = (uint16_t)(1 + (current_beacon_time - first_beacon_time) / 128);
#endif
*/

//		time_unformatted += 3600 * TIME_ZONE + 3600 * SUMMER_TIME + LEAP_SECS;
		__disable_irq();
		uint32_t ticks_since_last_rst = __HAL_TIM_GET_COUNTER(&htim2);
		uint16_t periods_measured = timer_periods;
//		uint16_t timer_periods_local = timer_periods;
//		static uint16_t last_timer_periods = 0;
//		uint16_t periods_since_last_beacon = timer_periods_local - last_timer_periods;
//		last_timer_periods = timer_periods_local;
//
//		if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE)) {
//
//			if (ticks_since_last_rst < (htim2.Init.Period / 2)) {
//				timer_periods_local++;
//
//				__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
//				NVIC_ClearPendingIRQ(TIM2_IRQn);
//			}
//		}

		timer_periods = 0;
		__enable_irq();

		static uint16_t secs_between_beacons = 0;
		static int64_t total_ticks;
		static int64_t correct_ticks;
		static int64_t diff;
		static uint32_t ticks_in_period;
		static int32_t error_per_sec;
		static int64_t integral_error = 0;
		static int32_t p_term = 0;
		static int32_t i_term = 0;
		int32_t error_ms = 0;

		secs_between_beacons = (uint16_t)(time_unformatted - prev_received_time);
		if (secs_between_beacons == 0) secs_between_beacons = 128;

		// Timer period configuration
		static uint32_t prev_ticks_since_last_rst = 0;

		if (timer_running_flag > 0) {
//			ticks_in_period = htim2.Init.Period;
			ticks_in_period = __HAL_TIM_GET_AUTORELOAD(&htim2);

			int32_t phase_error_ticks = ((int32_t)periods_measured - (int32_t)secs_between_beacons) * (int32_t)ticks_in_period + (int32_t)ticks_since_last_rst;
			error_ms = phase_error_ticks / 48000;
			diff = phase_error_ticks;

			error_per_sec = (int32_t)(diff / secs_between_beacons);
			integral_error += error_per_sec; // + offset_correction;

			if (integral_error > MAX_INTEGRAL) integral_error = MAX_INTEGRAL;
			if (integral_error < -MAX_INTEGRAL) integral_error = -MAX_INTEGRAL;

			p_term = error_per_sec / 4;
			i_term = integral_error / 128;
//
//			int64_t raw_hw_ticks = (int64_t)ticks_in_period * (int64_t)periods_since_last_beacon + (int64_t)ticks_since_last_rst;
//			total_ticks = raw_hw_ticks - (int64_t)prev_ticks_since_last_rst;
//			prev_ticks_since_last_rst = ticks_since_last_rst;
//
//			correct_ticks = (int64_t)NOMINAL_PERIOD * (int64_t)secs_between_beacons;
//			int32_t offset_correction = (int32_t)(diff / 4096);
//
//			error_ms = (int32_t)(diff / 48000); // error in ms

			int32_t total_correction = p_term + i_term;
			if (total_correction > MAX_CORRECTION) total_correction = MAX_CORRECTION;
			if (total_correction < -MAX_CORRECTION) total_correction = -MAX_CORRECTION;

			uint32_t new_period = (uint32_t)((int32_t)NOMINAL_PERIOD + total_correction);
			htim2.Init.Period = new_period;
			__HAL_TIM_SET_AUTORELOAD(&htim2, new_period);

			// Beacons counting
			beacons_received ++;
			current_beacon_time = time_unformatted;
			beacons_passed = (uint16_t)(1 + (current_beacon_time - first_beacon_time) / 128);
		}
		else {
			timer_running_flag ++;
			first_beacon_time = time_unformatted;
			timer_periods = 0; // 0b1111111111111111;
		}

			// Reseting timer
//			__HAL_TIM_SET_COUNTER(&htim2, 0);
//
//			__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
//			__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
//
//			__disable_irq();
//			uint32_t ticks_since_last_rst = __HAL_TIM_GET_COUNTER(&htim2);
//			uint16_t timer_periods_local = timer_periods;
//
//			if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE)) {
//
//				if (ticks_since_last_rst < (htim2.Init.Period / 2)) {
//					timer_periods_local++;
//
//					__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
//					NVIC_ClearPendingIRQ(TIM2_IRQn);
//				}
//			}

//		timer_periods = 0;
//			__enable_irq();

		HAL_TIM_PWM_Start_IT(&htim2, TIM_CHANNEL_1);
//		}

		prev_received_time = time_unformatted;

		// Time formatting
//		format_date(time_unformatted, &td);
//
		time_unformatted_g = time_unformatted;
//
//		// OLED
//		char text_buffer[30];
//
//		sprintf(text_buffer, "%02u:%02u:%02u\r\n", td.hours, td.minutes, td.seconds);
//		ssd1306_SetCursor(2, 0);
//		ssd1306_WriteString(text_buffer, Font_11x18, White);
//
//		sprintf(text_buffer, "%s %u %u\r\n", td.month, td.day, td.year);
//		ssd1306_SetCursor(2, 20);
//		ssd1306_WriteString(text_buffer, Font_6x8, White);
//
//		ssd1306_UpdateScreen();

		/*======================================================== SERIAL PRINTING ========================================================*/

#ifdef DEBUG_PRINT
//		int32_t total_ticks1 = (int32_t)(total_ticks >> 32);
//		int32_t total_ticks2 = (int32_t)(total_ticks);
//		int32_t correct_ticks1 = (int32_t)(correct_ticks >> 32);
//		int32_t correct_ticks2 = (int32_t)(correct_ticks);
		int32_t diff1 = (int32_t)(diff >> 32);
		int32_t diff2 = (int32_t)(diff);

		format_time(time_unformatted, &td);

		printf("\r\n--- TIME VALID ---\r\n");
		printf("\r\nTime: %02u:%02u:%02u\r\n", td.hours, td.minutes, td.seconds);
		printf("Previous timer period: %" PRIu32 "\r\n", ticks_in_period);
//		printf("Timer periods since last beacon counted: %" PRIu16 "\r\n", periods_since_last_beacon);
		printf("Seconds between beacons: %" PRIu16 "\r\n", secs_between_beacons);
		printf("Total periods counted: %" PRIu16 "\r\n", periods_measured);
		printf("Ticks since last reset: %" PRIu32 "\r\n", ticks_since_last_rst);

		// Total ticks
//		printf("Ticks total:   %ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld",
//					(total_ticks1 >> 31) & 1, (total_ticks1 >> 30) & 1, (total_ticks1 >> 29) & 1, (total_ticks1 >> 28) & 1,
//					(total_ticks1 >> 27) & 1, (total_ticks1 >> 26) & 1, (total_ticks1 >> 25) & 1, (total_ticks1 >> 24) & 1,
//					(total_ticks1 >> 23) & 1, (total_ticks1 >> 22) & 1, (total_ticks1 >> 21) & 1, (total_ticks1 >> 20) & 1,
//					(total_ticks1 >> 19) & 1, (total_ticks1 >> 18) & 1, (total_ticks1 >> 17) & 1, (total_ticks1 >> 16) & 1,
//					(total_ticks1 >> 15) & 1, (total_ticks1 >> 14) & 1, (total_ticks1 >> 13) & 1, (total_ticks1 >> 12) & 1,
//					(total_ticks1 >> 11) & 1, (total_ticks1 >> 10) & 1, (total_ticks1 >> 9) & 1, (total_ticks1 >> 8) & 1,
//					(total_ticks1 >> 7) & 1, (total_ticks1 >> 6) & 1, (total_ticks1 >> 5) & 1, (total_ticks1 >> 4) & 1,
//					(total_ticks1 >> 3) & 1, (total_ticks1 >> 2) & 1, (total_ticks1 >> 1) & 1, (total_ticks1 >> 0) & 1);
//		printf(" %ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld\r\n",
//					(total_ticks2 >> 31) & 1, (total_ticks2 >> 30) & 1, (total_ticks2 >> 29) & 1, (total_ticks2 >> 28) & 1,
//					(total_ticks2 >> 27) & 1, (total_ticks2 >> 26) & 1, (total_ticks2 >> 25) & 1, (total_ticks2 >> 24) & 1,
//					(total_ticks2 >> 23) & 1, (total_ticks2 >> 22) & 1, (total_ticks2 >> 21) & 1, (total_ticks2 >> 20) & 1,
//					(total_ticks2 >> 19) & 1, (total_ticks2 >> 18) & 1, (total_ticks2 >> 17) & 1, (total_ticks2 >> 16) & 1,
//					(total_ticks2 >> 15) & 1, (total_ticks2 >> 14) & 1, (total_ticks2 >> 13) & 1, (total_ticks2 >> 12) & 1,
//					(total_ticks2 >> 11) & 1, (total_ticks2 >> 10) & 1, (total_ticks2 >> 9) & 1, (total_ticks2 >> 8) & 1,
//					(total_ticks2 >> 7) & 1, (total_ticks2 >> 6) & 1, (total_ticks2 >> 5) & 1, (total_ticks2 >> 4) & 1,
//					(total_ticks2 >> 3) & 1, (total_ticks2 >> 2) & 1, (total_ticks2 >> 1) & 1, (total_ticks2 >> 0) & 1);
//
//		// Correct ticks
//		printf("Correct ticks: %ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld",
//					(correct_ticks1 >> 31) & 1, (correct_ticks1 >> 30) & 1, (correct_ticks1 >> 29) & 1, (correct_ticks1 >> 28) & 1,
//					(correct_ticks1 >> 27) & 1, (correct_ticks1 >> 26) & 1, (correct_ticks1 >> 25) & 1, (correct_ticks1 >> 24) & 1,
//					(correct_ticks1 >> 23) & 1, (correct_ticks1 >> 22) & 1, (correct_ticks1 >> 21) & 1, (correct_ticks1 >> 20) & 1,
//					(correct_ticks1 >> 19) & 1, (correct_ticks1 >> 18) & 1, (correct_ticks1 >> 17) & 1, (correct_ticks1 >> 16) & 1,
//					(correct_ticks1 >> 15) & 1, (correct_ticks1 >> 14) & 1, (correct_ticks1 >> 13) & 1, (correct_ticks1 >> 12) & 1,
//					(correct_ticks1 >> 11) & 1, (correct_ticks1 >> 10) & 1, (correct_ticks1 >> 9) & 1, (correct_ticks1 >> 8) & 1,
//					(correct_ticks1 >> 7) & 1, (correct_ticks1 >> 6) & 1, (correct_ticks1 >> 5) & 1, (correct_ticks1 >> 4) & 1,
//					(correct_ticks1 >> 3) & 1, (correct_ticks1 >> 2) & 1, (correct_ticks1 >> 1) & 1, (correct_ticks1 >> 0) & 1);
//		printf(" %ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld\r\n",
//					(correct_ticks2 >> 31) & 1, (correct_ticks2 >> 30) & 1, (correct_ticks2 >> 29) & 1, (correct_ticks2 >> 28) & 1,
//					(correct_ticks2 >> 27) & 1, (correct_ticks2 >> 26) & 1, (correct_ticks2 >> 25) & 1, (correct_ticks2 >> 24) & 1,
//					(correct_ticks2 >> 23) & 1, (correct_ticks2 >> 22) & 1, (correct_ticks2 >> 21) & 1, (correct_ticks2 >> 20) & 1,
//					(correct_ticks2 >> 19) & 1, (correct_ticks2 >> 18) & 1, (correct_ticks2 >> 17) & 1, (correct_ticks2 >> 16) & 1,
//					(correct_ticks2 >> 15) & 1, (correct_ticks2 >> 14) & 1, (correct_ticks2 >> 13) & 1, (correct_ticks2 >> 12) & 1,
//					(correct_ticks2 >> 11) & 1, (correct_ticks2 >> 10) & 1, (correct_ticks2 >> 9) & 1, (correct_ticks2 >> 8) & 1,
//					(correct_ticks2 >> 7) & 1, (correct_ticks2 >> 6) & 1, (correct_ticks2 >> 5) & 1, (correct_ticks2 >> 4) & 1,
//					(correct_ticks2 >> 3) & 1, (correct_ticks2 >> 2) & 1, (correct_ticks2 >> 1) & 1, (correct_ticks2 >> 0) & 1);

		// Difference
		printf("Difference:    %ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld",
					(diff1 >> 31) & 1, (diff1 >> 30) & 1, (diff1 >> 29) & 1, (diff1 >> 28) & 1,
					(diff1 >> 27) & 1, (diff1 >> 26) & 1, (diff1 >> 25) & 1, (diff1 >> 24) & 1,
					(diff1 >> 23) & 1, (diff1 >> 22) & 1, (diff1 >> 21) & 1, (diff1 >> 20) & 1,
					(diff1 >> 19) & 1, (diff1 >> 18) & 1, (diff1 >> 17) & 1, (diff1 >> 16) & 1,
					(diff1 >> 15) & 1, (diff1 >> 14) & 1, (diff1 >> 13) & 1, (diff1 >> 12) & 1,
					(diff1 >> 11) & 1, (diff1 >> 10) & 1, (diff1 >> 9) & 1, (diff1 >> 8) & 1,
					(diff1 >> 7) & 1, (diff1 >> 6) & 1, (diff1 >> 5) & 1, (diff1 >> 4) & 1,
					(diff1 >> 3) & 1, (diff1 >> 2) & 1, (diff1 >> 1) & 1, (diff1 >> 0) & 1);
		printf(" %ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld\r\n",
					(diff2 >> 31) & 1, (diff2 >> 30) & 1, (diff2 >> 29) & 1, (diff2 >> 28) & 1,
					(diff2 >> 27) & 1, (diff2 >> 26) & 1, (diff2 >> 25) & 1, (diff2 >> 24) & 1,
					(diff2 >> 23) & 1, (diff2 >> 22) & 1, (diff2 >> 21) & 1, (diff2 >> 20) & 1,
					(diff2 >> 19) & 1, (diff2 >> 18) & 1, (diff2 >> 17) & 1, (diff2 >> 16) & 1,
					(diff2 >> 15) & 1, (diff2 >> 14) & 1, (diff2 >> 13) & 1, (diff2 >> 12) & 1,
					(diff2 >> 11) & 1, (diff2 >> 10) & 1, (diff2 >> 9) & 1, (diff2 >> 8) & 1,
					(diff2 >> 7) & 1, (diff2 >> 6) & 1, (diff2 >> 5) & 1, (diff2 >> 4) & 1,
					(diff2 >> 3) & 1, (diff2 >> 2) & 1, (diff2 >> 1) & 1, (diff2 >> 0) & 1);
		printf("Error: %" PRId32 "ms\r\n", error_ms);
//		printf("Error bin: %ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld\r\n",
//			(error_ms >> 31) & 1, (error_ms >> 30) & 1, (error_ms >> 29) & 1, (error_ms >> 28) & 1,
//			(error_ms >> 27) & 1, (error_ms >> 26) & 1, (error_ms >> 25) & 1, (error_ms >> 24) & 1,
//			(error_ms >> 23) & 1, (error_ms >> 22) & 1, (error_ms >> 21) & 1, (error_ms >> 20) & 1,
//			(error_ms >> 19) & 1, (error_ms >> 18) & 1, (error_ms >> 17) & 1, (error_ms >> 16) & 1,
//			(error_ms >> 15) & 1, (error_ms >> 14) & 1, (error_ms >> 13) & 1, (error_ms >> 12) & 1,
//			(error_ms >> 11) & 1, (error_ms >> 10) & 1, (error_ms >> 9) & 1, (error_ms >> 8) & 1,
//			(error_ms >> 7) & 1, (error_ms >> 6) & 1, (error_ms >> 5) & 1, (error_ms >> 4) & 1,
//			(error_ms >> 3) & 1, (error_ms >> 2) & 1, (error_ms >> 1) & 1, (error_ms >> 0) & 1);

		printf("New timer period: %" PRIu32 "\r\n", htim2.Init.Period);
		printf("P term: %" PRId32 "\r\n", p_term);
		printf("I term: %" PRId32 "\r\n", i_term);
		printf("Number of received beacons: %u\r\n", beacons_received);
		printf("Number of passed beacons: %u\r\n", beacons_passed);
#endif

	} else {
#ifdef DEBUG_PRINT
		printf("-- TIME INVALID --\r\n");
#endif

	}

	// End of message
#ifdef DEBUG_PRINT
	printf("\r\n=======================\r\n\n");
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
			if (crc_reg & 0x8000)	// MSB check
					{
				crc_reg = (crc_reg << 1) ^ crc_polynom;
			} else {
				crc_reg = (crc_reg << 1);
			}
		}
	}

	return crc_reg;
}


// Function for timer start
//void register_timer(TIM_HandleTypeDef *htim) {
//    p_timer = htim;
//}
/* USER CODE END PrFD */
