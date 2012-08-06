/* ************************************************************************
*		File: race.c                                        Part of CircleMUD *
*	 Usage: Source file for race-specific code                              *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*  Race additions and implementations specific for Arcane Realms are      *
*  Copyright (C) 2001-2003, Arcane Realms MUD.                            *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: race.c,v 1.3 2003/04/07 08:15:20 artovil Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "screen.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "interpreter.h"
#include "constants.h"

extern int top_of_race_list;
extern struct race_list_element *race_list;

extern int top_of_culture_list;
extern struct culture_list_element *culture_list;

extern int magic_enabled;

/* local functions */
void roll_real_abils(struct char_data *ch);
void do_start(struct char_data *ch);
int	invalid_race(struct char_data *ch, struct obj_data *obj);
struct save_modifier_type save_modifier[];
void help_race(struct char_data *ch, int race);
int	parse_race(char arg);
long find_race_bitvector(char arg);
int	calc_saving_throw(struct char_data *ch, struct char_data *victim, int save_type);
void race_menu(struct char_data *ch, bool full, bool error);

/* External Functions */
extern int isname(const char *str, const char *namelist);
ACMD(do_help);

/* External variables */
extern const	char *unused_spellname;

/* Local defines */
#define	SKINFO skill_info[skillnum]
#define	SKTREE skill_tree[skillno]

/*
 * Proxy function to print out help during creation.
 */
void help_race(struct char_data *ch, int race)
{
	char *printbuf = get_buffer(256);
	sprintf(printbuf, "%s", race_list[race].name);
	do_help(ch, printbuf, 0, 0);
	release_buffer(printbuf);
}


void race_menu(struct char_data *ch, bool full, bool error)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	int i = 0;

	sprintf(printbuf, "\r\n"
		"%s.--------------------------[ %sARCANE REALMS RACES%s ]--------------------------.\r\n", QCYN, QHCYN, QCYN);

	for (i = 0; i <= top_of_race_list; i++) {
		if (!race_list[i].selectable && !full)
			continue;
		sprintf(printbuf, "%s| [%s%c%s] &Y%-17.17s%s: %s%-50.50s %s|\r\n", printbuf, QHWHT, menu_letter(i), QCYN, race_list[i].name, QCYN, QYEL, race_list[i].description, QCYN);
	}

	if (!full) {
		sprintf(printbuf, "%s|---------------------------------------------------------------------------|\r\n"
			"| %sYou will receive extended information once you select a race, and you can %s|\r\n"
			"| %salways go back and pick another race when you have read the description.  %s|\r\n"
			"| %sThese races are only starting races.  Once you gain enough RP points      %s|\r\n"
			"| %sand have written a proper background, you can chose a less common race.   %s|\r\n"
			"'---------------------------------------------------------------------------'%s\r\n",
				printbuf,
				QHCYN, QCYN,
				QHCYN, QCYN,
				QHCYN, QCYN,
				QHCYN, QCYN, QNRM);
		if (error)
			sprintf(printbuf, "%s%sThat's not a race %s: ", printbuf, QHWHT, QNRM);
		else
			sprintf(printbuf, "%s%sRace %s: ", printbuf, QHWHT, QNRM);
	} else
		sprintf(printbuf, "%s'---------------------------------------------------------------------------'%s\r\n", printbuf, QNRM);

	send_to_char(printbuf, ch);
	release_buffer(printbuf);
}


struct save_modifier_type save_modifier[] = {
	/* PARA       ROD    PETRI  BREATH    SPELL */
	 {   15,			10,			 20,			15,			 20  }, /* RACE_CYMRI */
	 {   10,			 5,			 15,			10,			 10  }, /* RACE_CELT */
	 {   -5,			-5,			 -5,			-5,			 -5  }, /* RACE_GRAYELF */
	 {   -5,			-5,			 -5,			-5,			-10  }, /* RACE_HIGHELF */
	 {   10,      10,      10,      10,      10  }, /* RACE_SATYR */
	 {    0,      10,       0,       5,      -5  }, /* RACE_PIXIE */
	 {    5,       5,       0,      10,       0  }, /* RACE_SPRITE */
	 {   10,      10,      10,      10,      10  }, /* RACE_NYMPH */
	 {    0,       0,      -5,      15,      10  }, /* RACE_GNOME */
	 {  -10,      -5,       0,      -5,      -5  }, /* RACE_AASIMARI */
	 {   15,			 5,      20,      10,      20  }, /* RACE_GIANT */
	 {    0,       5,      -5,      10,     -10  }, /* RACE_VAMPIRE */
 	 {  -10,      -5,       0,      -5,      -5  },	/* RACE_TIEFLING */
	 {    0,      10,       0,       5,      -5  },	/* RACE_NIXIE */
	 {  -10,      -5,     -15,       0,     -10  }, /* RACE_LEPRECHAUN */
	 {   15,       5,      20,      10,      20  }, /* RACE_TROLL */
	 {    0,       0,      -5,      15,      10  }, /* RACE_GOBLIN */
	 {    5,       0,      -5,      10,       5  }, /* RACE_BROWNIE */ 
	 {  -15,       0,     -15,     -20,     -20  }, /* RACE_DRAGON */
	 {    0,       5,      20,     -20,      20  }, /* RACE_UNDEAD */
	 {  -20,     -15,     -20,     -15,     -20  }, /* RACE_ETHEREAL */ 
	 {   20,      20,      20,      20,      20  }, /* RACE_ANIMAL */
	 {  -15,       0,     -10,     -15,     -15  }, /* RACE_LEGENDARY */ 
	 {  -15,     -15,     -20,     -15,     -15  }, /* RACE_ELEMENTAL */
	 {  -15,     -20,     -15,     -20,     -15  }, /* RACE_MAGICAL */
	 {  -15,     -15,     -15,     -15,     -20  }, /* RACE_SERAPHI */
	 {  -15,     -15,     -15,     -15,     -20  },	/* RACE_ADEPHI */
	 {  -15,     -15,     -15,     -15,     -20  },	/* RACE_DEMON */  
	 {  -20,     -20,     -20,     -20,     -20  }, /* RACE_DEITY */  
	 {  -20,     -20,     -20,     -20,     -20  },	/* RACE_ENDLESS */  
	 {   20,      20,      20,      20,      20  }	/* Unknown */
};


int	calc_saving_throw(struct char_data *ch, struct char_data *victim, int save_type)
{
	float base_save = 0;
	float saving_throw = 0;
	float mod = 0;
	if (IS_IMMORTAL(ch)) {
		/* Gods have all the fun! :) */
		return(1);
	}
	switch (save_type) {
		case SAVING_PARA:
			mod = save_modifier[(int)GET_RACE(victim)].paralyze;
		break;
		case SAVING_ROD:
			mod = save_modifier[(int)GET_RACE(victim)].rod;
		break;
		case SAVING_PETRI:
			mod = save_modifier[(int)GET_RACE(victim)].petrify;
		break;
		case SAVING_BREATH:
			mod = save_modifier[(int)GET_RACE(victim)].breath;
		break;
		case SAVING_SPELL:
			mod = save_modifier[(int)GET_RACE(victim)].spell;
		break;
		default:
			mlog("SYSERR: Unknown saving throw type (%d) in saving throw chart.", save_type);
		return (99);
	}
	
	base_save = (50 - (GET_INTELLIGENCE(victim) - GET_INTELLIGENCE(ch))) / 2;
	saving_throw = base_save + ((mod / 100) * base_save);
	
	return MIN(100,(int)saving_throw);
}


/*
 * invalid_race is used by handler.c to determine if a piece of equipment is
 * usable by a particular race, based on the ITEM_ANTI_{race} bitvectors.
 */

int	invalid_race(struct char_data *ch, struct obj_data *obj) {
	if ((OBJ_FLAGGED(obj, ITEM_ANTI_FAE)				&& IS_FAE(ch))      	||
			(OBJ_FLAGGED(obj, ITEM_ANTI_DEMONIC)		&& IS_DEMONIC(ch))	  ||
			(OBJ_FLAGGED(obj, ITEM_ANTI_DIVINE)			&& IS_DIVINE(ch)) 	  ||
			(OBJ_FLAGGED(obj, ITEM_ANTI_VAMPIRE)		&& IS_VAMPIRE(ch))    ||
			(OBJ_FLAGGED(obj, ITEM_ANTI_LITTLE_FAE)	&& IS_LITTLE_FAE(ch)) ||
			(OBJ_FLAGGED(obj, ITEM_ANTI_UNDEAD)			&& IS_UNDEAD(ch))   	||
			(!OBJ_FLAGGED(obj, ITEM_OK_ANIMAL)			&& IS_ANIMAL(ch))   	||
			(OBJ_FLAGGED(obj, ITEM_ANTI_HUMAN)			&& IS_HUMAN(ch))    	||
			(OBJ_FLAGGED(obj, ITEM_ANTI_DEITY)			&& IS_DEITY(ch)))
		return 1;
	else
		return 0;
}


/*
 * Proxy function to print out help during creation.
 */
void help_culture(struct char_data *ch, int culture)
{
	char *printbuf = get_buffer(256);
	sprintf(printbuf, "%s", culture_list[culture].name);
	do_help(ch, printbuf, 0, 0);
	release_buffer(printbuf);
}


void culture_menu(struct char_data *ch, bool full, bool error)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	int i = 0;

	sprintf(printbuf, "\r\n"
		"%s.------------------------[ %sARCANE REALMS CULTURES%s ]-------------------------.\r\n", QCYN, QHCYN, QCYN);

	for (i = 0; i <= top_of_culture_list; i++) {
		if (!culture_list[i].selectable && !full)
			continue;
		sprintf(printbuf, "%s| [%s%c%s] &Y%-17.17s%s: %s%-50.50s %s|\r\n", printbuf, QHWHT, menu_letter(i), QCYN, culture_list[i].name, QCYN, QYEL, culture_list[i].description, QCYN);
	}

	sprintf(printbuf, "%s'---------------------------------------------------------------------------'%s\r\n", printbuf, QNRM);

	if (!full) {
		if (error)
			sprintf(printbuf, "%s%sThat's not a culture %s: ", printbuf, QHWHT, QNRM);
		else
			sprintf(printbuf, "%s%sCulture %s: ", printbuf, QHWHT, QNRM);
	}

	send_to_char(printbuf, ch);
	release_buffer(printbuf);
}
