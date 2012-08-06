/* $Id: time.h,v 1.4 2003/04/06 11:38:08 artovil Exp $ */

extern const char *weekdays[];
extern const char *month_name[];
extern const char *basictimeofday[];
extern const char *holidays[];

extern int in_season(int season);

#define SEASON_WINTER			1
#define SEASON_SPRING			2
#define SEASON_SUMMER			3
#define SEASON_AUTUMN			4
