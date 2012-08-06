/* ************************************************************************
*   File: color.c                                       Part of CircleMUD *
*  Usage: interprets inline color codes                                   *
*   Name: Easy Color v2.2                                                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*  Modifications Copyright Trevor Man 1997                                *
*  Based on the Easy Color patch by mud@proimages.proimages.com           *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "color.h"
#include "constants.h"

#define COLOR_NAME			40
#define COLOR_JOKER			41
#define COLOR_ANA				42
#define COLOR_SANA			43
#define COLOR_SPACE			44

#define RESOURCE_NAME		50
#define RESOURCE_SPACE	51

#define COLOR_OFF				98
#define COLOR_ON				99

#define CNRM  "\x1B[0;0m"
#define CBLK  "\x1B[0;30m"
#define CRED  "\x1B[0;31m"
#define CGRN  "\x1B[0;32m"
#define CYEL  "\x1B[0;33m"
#define CBLU  "\x1B[0;34m"
#define CMAG  "\x1B[0;35m"
#define CCYN  "\x1B[0;36m"
#define CWHT  "\x1B[0;37m"
#define CNUL  ""

#define BBLK  "\x1B[1;30m"
#define BRED  "\x1B[1;31m"
#define BGRN  "\x1B[1;32m"
#define BYEL  "\x1B[1;33m"
#define BBLU  "\x1B[1;34m"
#define BMAG  "\x1B[1;35m"
#define BCYN  "\x1B[1;36m"
#define BWHT  "\x1B[1;37m"

#define BKBLK  "\x1B[40m"
#define BKRED  "\x1B[41m"
#define BKGRN  "\x1B[42m"
#define BKYEL  "\x1B[43m"
#define BKBLU  "\x1B[44m"
#define BKMAG  "\x1B[45m"
#define BKCYN  "\x1B[46m"
#define BKWHT  "\x1B[47m"

#define CAMP  "&"
#define CSLH  "\\"

#define CUDL  "\x1B[4m"	/* Underline ANSI code */
#define CFSH  "\x1B[5m"	/* Flashing ANSI code.  Change to #define CFSH "" if you want to disable flashing color codes */
#define CRVS  "\x1B[7m" /* Reverse video ANSI code */

const char *COLORLIST[] = {CNRM, CRED, CGRN, CYEL, CBLU, CMAG, CCYN, CWHT,
			    BRED, BGRN, BYEL, BBLU, BMAG, BCYN, BWHT,
			    BKRED, BKGRN, BKYEL, BKBLU, BKMAG, BKCYN, BKWHT,
			    CAMP, CSLH, BKBLK, CBLK, CFSH, CRVS, CUDL, BBLK };

const char *ANSINAMES[] = {"", "red", "green", "yellow", "blue", "magenta", "cyan", "gray",
			    "bright red", "bright green", "bright yellow", "bright blue", "bright magenta", "bright cyan", "white",
			    "inv red", "inv green", "inv yellow", "inv blue", "inv magenta", "inv cyan", "inv white",
			    "", "", "", "", "flashing", "reverse", "underline", "black" };

struct color_data EXTENDED_COLORS[] = {
	{	"!UNUSED!",	0	},
	// reds
	{	"rose",							8		},	// 1
	{	"burgundy",					1		},
	{	"cherry red",				8		},
	{	"crimson",					1		},
	{	"radiant scarlet",	8		},	// 5
	// greens
	{	"jade",							9		},
	{	"hunter green",			9		},
	{	"forest green",			2		},
	{	"moss green",				2		},
	{	"olive",						2		},	// 10
	// yellows
	{	"amber",						3		},
	{	"cream",						10	},
	{	"golden",						3		},
	{	"ochre",						3		},
	{	"tawny yellow",			10	},	// 15
	// oranges
	{	"apricot",					3		},
	{	"peach",						3		},	// 17
	// blues
	{	"azure",						11	},
	{	"cobalt blue",			4		},
	{	"navy",							4		},	// 20
	{	"teal",							6		},
	{	"midnight blue",		4		},
	{	"turquoise",				11	},	// 23
	// purples
	{	"indigo",						5		},
	{	"lavender",					12	},	// 25
	{	"lilac",						12	},
	{	"maroon",						5		},
	{	"plum",							12	},
	{	"violet",						12	},
	{	"wine",							5		},	// 30
	// browns
	{	"beige",						3		},
	{	"chestnut",					1		},
	{	"chocolate",				29	},
	{	"fawn",							1		},
	{	"russet",						1		},	// 35
	// blacks
	{	"charcoal",					29	},
	{	"ebony",						29	},
	{	"ink black",				29	},
	{	"raven",						29	},
	{	"slate gray",				7		},	// 40
	// whites
	{	"alabaster white",	14	},
	{	"ash grey",					7		},
	{	"ivory",						14	},
	{	"pearl",						14	},	// 44
	// end
	{	"\n",								0		}		// 45
};

const char *COLORANA[] = {"An", "an"};

int isnum(char s)
{
  return( (s >= '0') && (s <= '9') );
}


int is_color(char code)
{
	switch (code) {
	/* Normal colors */
	case	'k':	return 25;	break;	/* Black															*/
	case	'r':	return 1;		break;	/* Red																*/
	case	'g':	return 2;		break;	/* Green															*/
	case	'y':	return 3;		break;	/* Yellow															*/
	case	'b':	return 4;		break;	/* Blue																*/
	case	'm':	return 5;		break;	/* Magenta														*/
	case	'c':	return 6;		break;	/* Cyan																*/
	case	'w':	return 7;		break;	/* White															*/

	/* Bold colors */
	case	'K':	return 29;	break;	/* Bold black (Just for completeness)	*/
	case	'R':	return 8;		break;	/* Bold red														*/
	case	'G':	return 9;		break;	/* Bold green													*/
	case	'Y':	return 10;	break;	/* Bold yellow												*/
	case	'B':	return 11;	break;	/* Bold blue													*/
	case	'M':	return 12;	break;	/* Bold magenta												*/
	case	'C':	return 13;	break;	/* Bold cyan													*/
	case	'W':	return 14;	break;	/* Bold white													*/
	
	/* Background colors */
	case	'0':	return 24;	break; 	/* Black background										*/
	case	'1':	return 15;	break;	/* Red background											*/
	case	'2':	return 16;	break;	/* Green background										*/
	case	'3':	return 17;	break;	/* Yellow background									*/
	case	'4':	return 18;	break;	/* Blue background										*/
	case	'5':	return 19;	break;	/* Magenta background									*/
	case	'6':	return 20;	break;	/* Cyan background										*/
	case	'7':	return 21;	break;	/* White background										*/

	/* Misc characters */
	case	'&':	return 22;	break;	/* The & character										*/
	case '\\':	return 23;	break;	/* The \ character										*/
	
	/* Special codes */
	case	'n':	return 0;		break;	/* Normal															*/
	case	'f':	return 26;	break;	/* Flash															*/
	case	'v':	return 27;	break;	/* Reverse video											*/
	case	'u':	return 28;	break;	/* Underline (Only for mono screens)	*/

	/* Object color codes */
	case	'N':	return COLOR_NAME;			break;	/* Color name string				*/
	case	'X':	return COLOR_JOKER;			break;	/* Color joker code					*/
	case	'a':	return COLOR_SANA;			break;	/* color name an/a string		*/
	case	'A':	return COLOR_ANA;				break;	/* color name An/A string		*/
	case	'S':	return COLOR_SPACE;			break;	/* color name space string	*/
	case	'Z':	return RESOURCE_NAME;		break;	/* Resource name string			*/
	case	'V':	return RESOURCE_SPACE;	break;	/* Resource name space			*/

	default:		return -1;	break;
	}
	return -1;
}


char *number_to_color(int color)
{
  switch (color) {
  /* Normal colors */
	case	25:	return "&k";	break;	/* Black															*/
	case	1:	return "&r";	break;	/* Red																*/
	case	2:	return "&g";	break;	/* Green															*/
	case	3:	return "&y";	break;	/* Yellow															*/
	case	4:	return "&b";	break;	/* Blue																*/
	case	5:	return "&m";	break;	/* Magenta														*/
	case	6:	return "&c";	break;	/* Cyan																*/
	case	7:	return "&w";	break;	/* White															*/

	/* Bold colors */
	case	29:	return "&K";	break;	/* Bold black (Just for completeness)	*/
	case	8:	return "&R";	break;	/* Bold red														*/
	case	9:	return "&G";	break;	/* Bold green													*/
	case	10:	return "&Y";	break;	/* Bold yellow												*/
	case	11:	return "&B";	break;	/* Bold blue													*/
	case	12:	return "&M";	break;	/* Bold magenta												*/
	case	13:	return "&C";	break;	/* Bold cyan													*/
	case	14:	return "&W";	break;	/* Bold white													*/
	
	/* Background colors */
	case	24:	return "&0";	break; 	/* Black background										*/
	case	15:	return "&1";	break;	/* Red background											*/
	case	16:	return "&2";	break;	/* Green background										*/
	case	17:	return "&3";	break;	/* Yellow background									*/
	case	18:	return "&4";	break;	/* Blue background										*/
	case	19:	return "&5";	break;	/* Magenta background									*/
	case	20:	return "&6";	break;	/* Cyan background										*/
	case	21:	return "&7";	break;	/* White background										*/

	/* Special codes */
	case	0:	return "&n";	break;	/* Normal															*/
	case	26:	return "&f";	break;	/* Flash															*/
	case	27:	return "&v";	break;	/* Reverse video											*/
	case	28:	return "&u";	break;	/* Underline (Only for mono screens)	*/

	default:	return "";		break;
	}
	return "";
}


/*
 * Added new functionality to proc_color:
 *   - Can now turn parsing on and off inline with \\c98 and \\c99
 *   - Color names and color wildcards (jokers) can be used
 *     for objects, providing that colorize_num is larger than 0.
 *   - Added a switch to catch the special codes, and also made
 *     sure that &x color codes are never parsed when use_color is
 *     off, although \\c works so color parsing can be turned on
 *     again.
 * Torgny Bjers (artovil@arcanerealms.org), 2002-11-11
 */
size_t proc_color(char *inbuf, int color, int use_color, int colorize_num, int resource_num)
{
  register int j = 0, p = 0;
  int k, max, c = 0;
  char out_buf[32768];

  if (inbuf[0] == '\0')
    return (0);

	while (inbuf[j] != '\0') {
		if ((inbuf[j]=='\\') && (inbuf[j+1]=='c')
				&& isnum(inbuf[j + 2]) && isnum(inbuf[j + 3])) {
			c = (inbuf[j + 2] - '0')*10 + inbuf[j + 3]-'0';
			j += 4;
		} else if ((inbuf[j] == '&') && !(is_color(inbuf[j + 1]) == -1) && use_color) {
			c = is_color(inbuf[j + 1]);
			j += 2;
		} else {
			out_buf[p] = inbuf[j];
			j++;
			p++;
			continue;
		}
		switch (c) {
		case COLOR_OFF:
			/* Raw color codes, do not replace. */
			use_color = FALSE;
			break;
		case COLOR_ON:
			/* Turn color replacement on again. */
			use_color = TRUE;
			break;
		case COLOR_NAME:
			/* Got a name code, use the color in colorize_num for color name. */
			if (colorize_num > 0 && colorize_num <= NUM_EXT_COLORS) {
				c = colorize_num;
				max = strlen(EXTENDED_COLORS[c].name);
				for (k = 0; k < max; k++) {
					out_buf[p] = EXTENDED_COLORS[c].name[k];
					p++;
				}
			}
			break;
		case COLOR_JOKER:
			/* Got a joker code, use the color in colorize_num. */
			if (colorize_num > 0 && colorize_num <= NUM_EXT_COLORS)
				c = colorize_num;
			else
				c = 0;
			max = strlen(COLORLIST[EXTENDED_COLORS[c].color]);
			if (use_color && (color || max == 1)) {
				for (k = 0; k < max; k++) {
					out_buf[p] = COLORLIST[EXTENDED_COLORS[c].color][k];
					p++;
				}
			}
			break;
		case COLOR_ANA:
			/* Got AN/A ucword code. */
			if (colorize_num > 0 && colorize_num <= NUM_EXT_COLORS) {
				c = colorize_num;
				max = strlen(STRANA(EXTENDED_COLORS[c].name));
				for (k = 0; k < max; k++) {
					out_buf[p] = COLORANA[0][k];
					p++;
				}
			}
			break;
		case COLOR_SANA:
			/* Got AN/A lowercase code. */
			if (colorize_num > 0 && colorize_num <= NUM_EXT_COLORS) {
				c = colorize_num;
				max = strlen(STRSANA(EXTENDED_COLORS[c].name));
				for (k = 0; k < max; k++) {
					out_buf[p] = COLORANA[1][k];
					p++;
				}
			}
			break;
		case COLOR_SPACE:
			/* Got space code. */
			if (colorize_num > 0 && colorize_num <= NUM_EXT_COLORS) {
				out_buf[p] = ' ';
				p++;
			}
			break;
		case RESOURCE_NAME:
			/* Got a name code, use the color in colorize_num for color name. */
			if (resource_num > 0 && resource_num <= NUM_RESOURCES) {
				c = resource_num;
				max = strlen(resource_names[c]);
				for (k = 0; k < max; k++) {
					out_buf[p] = resource_names[c][k];
					p++;
				}
			}
			break;
		case RESOURCE_SPACE:
			/* Got space code. */
			if (resource_num > 0 && resource_num <= NUM_RESOURCES) {
				out_buf[p] = ' ';
				p++;
			}
			break;
		default:
			/* Normal color replacement. */
			if (c > MAX_COLORS)
				c = 0;
			max = strlen(COLORLIST[c]);
			if (use_color && (color || max == 1)) {
				for (k = 0; k < max; k++) {
					out_buf[p] = COLORLIST[c][k];
					p++;
				}
			}
			break;
		}
	}

  out_buf[p] = '\0';

  strcpy(inbuf, out_buf);
  
  return (strlen(inbuf));
}


/*
 * Utility function to check if an object name uses
 * AN/A strings.  This function destroys the string, so
 * it should not be used directly on the prototype name.
 * Torgny Bjers (artovil@arcanerealms.org) 2002-11-16.
 */
bool has_ana(char *inbuf) {
  int j = 0;
  int c = 0;

  if (inbuf[0] == '\0')
    return (0);

	while (inbuf[j] != '\0') {
		if ((inbuf[j]=='\\') && (inbuf[j+1]=='c')
				&& isnum(inbuf[j + 2]) && isnum(inbuf[j + 3])) {
			c = (inbuf[j + 2] - '0')*10 + inbuf[j + 3]-'0';
			j += 4;
		} else if ((inbuf[j] == '&') && !(is_color(inbuf[j + 1]) == -1)) {
			c = is_color(inbuf[j + 1]);
			j += 2;
		} else {
			j++;
			continue;
		}
		switch (c) {
		case COLOR_ANA:
		case COLOR_SANA:
			return (TRUE);
		}
	}
	return (FALSE);
}
