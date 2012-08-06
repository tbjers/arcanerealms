/* ************************************************************************
*   File: color.h                                       Part of CircleMUD *
*  Usage: header for the easy-color package.                              *
*   Name: Easy Color v2.2                                                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*  Modifications Copyright Trevor Man 1997                                *
*  Based on the Easy Color patch by mud@proimages.proimages.com           *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define MAX_COLORS 30

#define NUM_EXT_COLORS			44

struct color_data {
	const char *name;
	int color;
};

extern const char *COLORLIST[];
extern struct color_data EXTENDED_COLORS[];

int is_color(char code);
char *number_to_color(int color);
size_t proc_color(char *inbuf, int color, int use_color, int colorize_num, int resource_num);
bool has_ana(char *inbuf);
