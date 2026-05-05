/*
 * time_format.c
 *
 *  Created on: Apr 17, 2026
 *      Author: Karel
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "time_format.h"

// Function for time formatting
void format_time(uint32_t time_u, time_date_t *td) {
	uint32_t seconds_today = time_u % 86400;

	td->hours = (uint8_t)(seconds_today / 3600);
	td->minutes = (uint8_t)((seconds_today % 3600) / 60);
	td->seconds = (uint8_t)((seconds_today % 3600) % 60);
}

// Function for date formatting
void format_date(uint32_t time_u, time_date_t *td) {
	uint32_t days_passed = (time_u / 86400) + 6;	// Days passed since Jan 1 1980

	// =============== Year format ===============
	uint8_t quadrennium;
	uint8_t year_in_quad;
	uint8_t leap_year;
	uint16_t days_in_quad;
	uint16_t days_in_curr_year;

	quadrennium = (uint16_t)(days_passed / 1461);	// Split of the passed time to 4-years periods (because of leap years)
	days_in_quad = (uint16_t)(days_passed % 1461);
	leap_year = 0;

	if (days_in_quad <= 366) {
		year_in_quad = 0;
		days_in_curr_year = days_in_quad;
		leap_year = 1;
	}
	else if (days_in_quad > 366 && days_in_quad <= 731) {
		year_in_quad = 1;
		days_in_curr_year = days_in_quad - 366;
	}
	else if (days_in_quad > 731 && days_in_quad <= 1096) {
		year_in_quad = 2;
		days_in_curr_year = days_in_quad - 731;
	}
	else if (days_in_quad > 1096 && days_in_quad <= 1461) {
		year_in_quad = 3;
		days_in_curr_year = days_in_quad - 1096;
	}

	td->year = 1980 + 4*(uint16_t)(quadrennium) + (uint16_t)(year_in_quad);

	// =============== Month and day format ===============
	if (days_in_curr_year <= 31) {
		strcpy(td->month, "Jan");
		td->day = (uint8_t)(days_in_curr_year);
	}
	else if (days_in_curr_year > 31 && days_in_curr_year <= (59 + leap_year)) {
		strcpy(td->month, "Feb");
		td->day = (uint8_t)(days_in_curr_year - 31);
	}
	else if (days_in_curr_year > (59 + leap_year) && days_in_curr_year <= (90 + leap_year)) {
		strcpy(td->month, "Mar");
		td->day = (uint8_t)(days_in_curr_year - (59 + leap_year));
	}
	else if (days_in_curr_year > (90 + leap_year) && days_in_curr_year <= (120 + leap_year)) {
		strcpy(td->month, "Apr");
		td->day = (uint8_t)(days_in_curr_year - (90 + leap_year));
	}
	else if (days_in_curr_year > (120 + leap_year) && days_in_curr_year <= (151 + leap_year)) {
		strcpy(td->month, "May");
		td->day = (uint8_t)(days_in_curr_year - (120 + leap_year));
	}
	else if (days_in_curr_year > (151 + leap_year) && days_in_curr_year <= (181 + leap_year)) {
		strcpy(td->month, "Jun");
		td->day = (uint8_t)(days_in_curr_year - (151 + leap_year));
	}
	else if (days_in_curr_year > (181 + leap_year) && days_in_curr_year <= (212 + leap_year)) {
		strcpy(td->month, "Jul");
		td->day = (uint8_t)(days_in_curr_year - (181 + leap_year));
	}
	else if (days_in_curr_year > (212 + leap_year) && days_in_curr_year <= (243 + leap_year)) {
		strcpy(td->month, "Aug");
		td->day = (uint8_t)(days_in_curr_year - (212 + leap_year));
	}
	else if (days_in_curr_year > (243 + leap_year) && days_in_curr_year <= (273 + leap_year)) {
		strcpy(td->month, "Sep");
		td->day = (uint8_t)(days_in_curr_year - (243 + leap_year));
	}
	else if (days_in_curr_year > (273 + leap_year) && days_in_curr_year <= (304 + leap_year)) {
		strcpy(td->month, "Oct");
		td->day = (uint8_t)(days_in_curr_year - (273 + leap_year));
	}
	else if (days_in_curr_year > (304 + leap_year) && days_in_curr_year <= (334 + leap_year)) {
		strcpy(td->month, "Nov");
		td->day = (uint8_t)(days_in_curr_year - (304 + leap_year));
	}
	else if (days_in_curr_year > (334 + leap_year) && days_in_curr_year <= (365 + leap_year)) {
		strcpy(td->month, "Dec");
		td->day = (uint8_t)(days_in_curr_year - (334 + leap_year));
	}
}

