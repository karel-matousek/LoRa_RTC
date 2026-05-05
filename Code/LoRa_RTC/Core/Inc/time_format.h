/*
 * time_format.h
 *
 *  Created on: Apr 17, 2026
 *      Author: Karel
 */

#ifndef TIME_FORMAT_H_
#define TIME_FORMAT_H_

#include <stdio.h>
#include <stdint.h>

// Structures
typedef struct time_date_s{
	uint16_t year;
	char month[4];
	uint8_t day;

	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
} time_date_t;

// Functions
void format_time(uint32_t time_u, time_date_t *td);
void format_date(uint32_t time_u, time_date_t *td);

#endif /* INC_TIME_FORMAT_H_ */
