/* ************************************************************************
*		File: weather.c                                     Part of CircleMUD *
*	 Usage: functions handling time and the weather                         *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: weather.c,v 1.2 2001/11/09 00:12:11 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"

extern struct time_info_data time_info;

void weather_and_time(int mode);
void weather_change(void);
void another_hour(int mode);


void weather_and_time(int mode)
{
	another_hour(mode);
	if (mode)
		weather_change();
}


void weather_change(void)
{
	int diff, change;
	if ((time_info.month >= 9) && (time_info.month <= 16))
		diff = (weather_info.pressure > 985 ? -2 : 2);
	else
		diff = (weather_info.pressure > 1015 ? -2 : 2);

	weather_info.change += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));

	weather_info.change = MIN(weather_info.change, 12);
	weather_info.change = MAX(weather_info.change, -12);

	weather_info.pressure += weather_info.change;

	weather_info.pressure = MIN(weather_info.pressure, 1040);
	weather_info.pressure = MAX(weather_info.pressure, 960);

	change = 0;

	switch (weather_info.sky) {
	case SKY_CLOUDLESS:
		if (weather_info.pressure < 990)
			change = 1;
		else if (weather_info.pressure < 1010)
			if (dice(1, 4) == 1)
				change = 1;
			break;
	case SKY_CLOUDY:
		if (weather_info.pressure < 970)
			change = 2;
		else if (weather_info.pressure < 990) {
			if (dice(1, 4) == 1)
				change = 2;
			else
				change = 0;
		} else if (weather_info.pressure > 1030)
			if (dice(1, 4) == 1)
				change = 3;

		break;
	case SKY_RAINING:
		if (weather_info.pressure < 970) {
			if (dice(1, 4) == 1)
				change = 4;
			else
				change = 0;
		} else if (weather_info.pressure > 1030)
			change = 5;
		else if (weather_info.pressure > 1010)
			if (dice(1, 4) == 1)
				change = 5;

		break;
	case SKY_LIGHTNING:
		if (weather_info.pressure > 1010)
			change = 6;
		else if (weather_info.pressure > 990)
			if (dice(1, 4) == 1)
				change = 6;

		break;
	default:
		change = 0;
		weather_info.sky = SKY_CLOUDLESS;
		break;
	}

	switch (change) {
	case 0:
		break;
	case 1:
		send_to_outdoor("The sky starts to get cloudy.\r\n");
		weather_info.sky = SKY_CLOUDY;
		break;
	case 2:
		send_to_outdoor("It starts to rain.\r\n");
		weather_info.sky = SKY_RAINING;
		break;
	case 3:
		send_to_outdoor("The clouds disappear.\r\n");
		weather_info.sky = SKY_CLOUDLESS;
		break;
	case 4:
		send_to_outdoor("Lightning starts to show in the sky.\r\n");
		weather_info.sky = SKY_LIGHTNING;
		break;
	case 5:
		send_to_outdoor("The rain stops.\r\n");
		weather_info.sky = SKY_CLOUDY;
		break;
	case 6:
		send_to_outdoor("The lightning stops.\r\n");
		weather_info.sky = SKY_RAINING;
		break;
	default:
		break;
	}
}
