/* ************************************************************************
*		File: time.c                                        Part of CircleMUD *
*	 Usage: Loading/saving mud time                                         *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*	Copyright 2000 by Torgny Bjers (bjers@unmade.com)                       *
************************************************************************ */
/* $Id: time.c,v 1.14 2004/04/16 19:13:14 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "time.h"

#define	READ_SIZE 256

/**************************************************************************
*	 declarations of most of the 'global' variables                         *
**************************************************************************/

extern struct time_info_data time_info;
extern struct time_info_data *mud_time_passed(time_t t2, time_t t1);
extern void get_one_line(FILE *fl, char *buf);

/* local functions */
void read_mud_date_from_file(void);
void write_mud_date_to_file(void);
void another_hour(int mode);
ACMD(do_time);


/*************************************************************************
*	 routines for booting the time                                         *
*************************************************************************/

void another_hour(int mode)
{
	int type;

	time_info.hours++;

	if (mode) {
		switch (time_info.hours) {
		case 3:
			weather_info.sunlight = SUN_RISE;
			send_to_outdoor("The sun rises in the east.\r\n");
			break;
		case 4:
			weather_info.sunlight = SUN_LIGHT;
			send_to_outdoor("The day has begun.\r\n");
			break;
		case 16:
			weather_info.sunlight = SUN_SET;
			send_to_outdoor("The sun slowly disappears in the west.\r\n");
			break;
		case 17:
			weather_info.sunlight = SUN_DARK;
			send_to_outdoor("The night has begun.\r\n");
			break;
		default:
			break;
		}
	}

	if (time_info.hours > 17) {	/* Changed by HHS due to bug ??? */
		time_info.hours = 0;
		time_info.day++;

		if (time_info.month == 1 && time_info.day == 12)
			type = 1;
		else if (time_info.month == 3 && time_info.day == 2)
			type = 2;
		else if (time_info.month == 4 && time_info.day == 15)
			type = 3;
		else if (time_info.month == 6 && time_info.day == 11)
			type = 4;
		else if (time_info.month == 7 && time_info.day == 25)
			type = 5;
		else if (time_info.month == 9 && time_info.day == 20)
			type = 6;
		else if (time_info.month == 11 && (time_info.day > 2 && time_info.day < 6))
			type = 7;
		else if (time_info.month == 12 && time_info.day == 26)
			type = 8;
		else if (time_info.month == 12 && time_info.day == 27)
			type = 9;
		else
			type = 0;
		
		if (type > 0) {
			sprintf(buf, "Today we celebrate %s.\r\n", holidays[type]);
			send_to_all(buf);
		}
		
		if (time_info.day > 27) {
			time_info.day = 0;
			time_info.month++;
			
			if (time_info.month > 13) {
				time_info.month = 0;
				time_info.year++;
			}
		}
	}
}


void read_mud_date_from_file(void)
{
	FILE *fl;
	char line[READ_SIZE+1];
	/* struct time_write read_date; */
	
	if (!(fl = fopen("etc/date_record", "r"))) {
		mudlog("SYSERR: Cannot open etc/date_record file for read!", BRF, RIGHTS_DEVELOPER, TRUE);
		return;
	}
	
	/* get the first keyword line */
	get_one_line(fl, line);
	
	if (sscanf(line, "%d %d %d %d", &time_info.year, &time_info.month, &time_info.day, &time_info.hours) != 4) {
		mlog("SYSERR: Format error in 4-constant line of etc/date_record\r\n");
		exit(1);
	}
	
	fclose(fl);
}


void write_mud_date_to_file(void)
{
	FILE *fp;
	
	if (!(fp = fopen("etc/date_record", "w"))) {
		mudlog("SYSERR: Cannot open etc/date_record file for write!", BRF, RIGHTS_DEVELOPER, TRUE);
		return;
	}
	
	sprintf(buf, "%d %d %d %d\n",
		time_info.year,
		time_info.month,
		time_info.day,
		time_info.hours);
	fputs(buf, fp);
	
	/*
	* Write final line and close.
	*/
	fprintf(fp, "$\n");
	fclose(fp);

}


const	char *basictimeofday[] = {
	"midnight",                /* 0 */
	"late night",
	"approaching dawn",
	"dawn",
	"early morning",
	"morning",								/* 5 */
	"midmorning",
	"late morning",
	"noon",
	"early afternoon",
	"afternoon",							/* 10 */
	"afternoon",
	"late afternoon",
	"early evening",
	"evening",
	"late evening",						/* 15 */
	"twilight",
	"night"
};


/* Not used in sprinttype(). */
const	char *weekdays[] = {
	"the Moon",     /* 0(1) - Monday    */
	"Tiw",          /* 1(2) - Tuesday   */
	"Woden",        /* 2(3) - Wednesday */
	"Thor",         /* 3(4) - Thursday  */
	"Frigga",       /* 4(5) - Friday    */
	"Seterne",      /* 5(6) - Saturday  */
	"the Sun",      /* 6(7) - Sunday    */
	"the Secret of the Unhewn Stone" /* Intercalary Day, New Years, Dec 23 */
};


const	char *month_name[] = {
	"Birch",    /*  0(1)  - Dec 24, Jan 20                    */
	"Rowan",    /*  1(2)  - Jan 21, Feb 17 - Imbolc           */
	"Ash",      /*  2(3)  - Feb 18, Mar 17                    */
	"Alder",    /*  3(4)  - Mar 18, Apr 14 - Spring Equinox   */
	"Willow",   /*  4(5)  - Apr 15, May 12 - Beltaine         */
	"Hawthorn", /*  5(6)  - May 13, Jun 9                     */
	"Oak",      /*  6(7)  - Jun 10, Jul 7  - Summer Solstice  */
	"Holly",    /*  7(8)  - Jul 8, Aug 4   - Lughnasadh       */
	"Hazel",    /*  8(9)  - Aug 5, Sep 1                      */
	"Vine",     /*  9(10) - Sep 2, Sep 29  - Autumnal Equinox */
	"Ivy",      /* 10(11) - Sep 30, Oct 27                    */
	"Reed",     /* 11(12) - Oct 28, Nov 24 - Samhuinn         */
	"Elder"     /* 12(13) - Nov 25, Dec 22 - Winter Solstice  */
};


const	char *holidays[] = {
	"!RESERVED!",
	"Imbolc",
	"Vernal Equinox",
	"Beltaine",
	"Summer Solstice",
	"Lughnasadh",
	"Autumnal Equinox",
	"Samhuinn",
	"Winter Solstice",
	"New Years Eve"
};


ACMD(do_time)
{
	int weekday, day=0, type;
	const char *suf[] = {
		"!UNUSED!",
		"st",
		"nd",
		"rd"
	};
	
	sprintf(buf, "It is %s, on the day of ", basictimeofday[time_info.hours]);
	
	day = time_info.day + 1;        /* day in [1..28] */
	
	/* 28 days in a month */
	if (!(time_info.month == 12 && time_info.day + 1 == 28)) {
		weekday = ((28 * time_info.month) + day) % 7;
	} else {
		weekday = 7;
	}
	strcat(buf, weekdays[weekday]);
	
	strcat(buf, ",\r\n");
	send_to_char(buf, ch);
	
	if (time_info.month == 1 && day == 13)
		type = 1;
	else if (time_info.month == 3 && day == 3)
		type = 2;
	else if (time_info.month == 4 && day == 16)
		type = 3;
	else if (time_info.month == 6 && day == 12)
		type = 4;
	else if (time_info.month == 7 && day == 26)
		type = 5;
	else if (time_info.month == 9 && day == 21)
		type = 6;
	else if (time_info.month == 11 && (day > 3 && day < 7))
		type = 7;
	else if (time_info.month == 12 && day == 27)
		type = 8;
	else if (time_info.month == 12 && day == 28)
		type = 9;
	else
		type = 0;
	
	if (type == 0 && !(time_info.month == 12 && time_info.day + 1 == 28))
		sprintf(buf, "the %d%s Day of the month %s,\r\nin the year %d after the First Cataclysm.\r\n",
		day, (((day % 10) < 4 && (day % 10) > 0 && (day < 10 || day > 20))?suf[(day % 10)]:"th"), month_name[(int) time_info.month], time_info.year);
	else if (type > 0)
		sprintf(buf, "Today we celebrate %s, Year %d.\r\n", holidays[type], time_info.year);
	send_to_char(buf, ch);
}


int in_season(int season)
{
	int current = 0;
	if (time_info.month <= 2 || time_info.month > 10)
		current = SEASON_WINTER;
	else if (time_info.month > 2 && time_info.month < 5)
		current = SEASON_SPRING;
	else if (time_info.month >= 5 && time_info.month < 8)
		current = SEASON_SUMMER;
	else if (time_info.month >= 8 && time_info.month <= 10)
		current = SEASON_AUTUMN;
	return (season == current);
}
