/* *************************************************************************
*  File: roleplay.c                               Extension to CircleMUD   *
*  Usage: Source file for roleplay-specific code                          *
*                                                                          *
*  This file made exclusively for Arcane Realms MUD by Torgny Bjers.       *
*  Copyright (C) 1999-2002, Torgny Bjers, and Catherine Gore.              *
*                                                                          *
*	 All rights reserved.  See license.doc for complete information.         *
*                                                                          *
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University  *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
************************************************************************** */
/* $Id: roleplay.c,v 1.22 2004/03/16 14:33:57 cheron Exp $ */

#define	__ROLEPLAY_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "events.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "skills.h"
#include "interpreter.h" 
#include "constants.h"
#include "screen.h"
#include "comm.h"
#include "handler.h"
#include "dg_scripts.h"
#include "roleplay.h"
#include "loadrooms.h"

/* external variables */
extern struct room_data *world;
extern sh_int mortal_start_room[NUM_STARTROOMS +1];

int	find_house_owner(struct char_data *ch);

bool rpxp_reward(struct char_data *ch, unsigned int multiplier)
{
	struct char_data *wch;
	int ic = 0;

	if (IS_NPC(ch))
		return (FALSE);

	if (!IS_IC(ch))
		return (FALSE);

	for (wch = world[IN_ROOM(ch)].people; wch; wch = wch->next_in_room) 
		if ((wch != ch) && IS_IC(wch) && !IS_NPC(wch)) ic++;
	
	if (!ic)
		return (FALSE);

	if (GET_RPXP(ch) + multiplier + ic >= RPXP_CAP) {
		GET_RPXP(ch) = RPXP_CAP;
		extended_mudlog(NRM, SYSL_RPEXP, TRUE, "%s gained %d RPxp.", GET_NAME(ch), multiplier);
		return (TRUE);
	}

	if (multiplier > 0) {
		GET_RPXP(ch) += multiplier;
		GET_RPXP(ch) += ic;
	}	else
		return (FALSE);

	extended_mudlog(NRM, SYSL_RPEXP, TRUE, "%s gained %d RPxp.", GET_NAME(ch), multiplier);
	return (TRUE);
}


void add_to_rplog(struct char_data *ch, char *emote_text)
{
	char **ptr = &(GET_RPLOG(ch, 0));
	int i = 0;

	while (*ptr && (i < RPLOG_SIZE)) {
		ptr++;
		i++;
	}
 
	if (i == RPLOG_SIZE) { // rplog is full, cycle it.
		for (i = 0; i < RPLOG_SIZE - 1; i++)
			strcpy(GET_RPLOG(ch, i), GET_RPLOG(ch, i + 1));
		ptr = &(GET_RPLOG(ch, RPLOG_SIZE - 1));
		release_memory(*ptr);
	}

	if (!(*ptr)) {
		*ptr = get_memory(MAX_INPUT_LENGTH);
		strcpy(*ptr, CAP(emote_text));  
	}
}


/* The Emote speech parser was written by Zach Ibach (Zaquenin)
 * Many thanks, Zach!
 */
void parse_emote(const char *input, char *output, const char color, int gecho) 
{ 
	int flag = 0, i, j;

  for (i = 0, j = 0; i < strlen(input); i++) { 
		if (input[i] == '\"') { 
			if(flag) {
				output[j] = '&'; 
				output[++j] = 'n'; 
				output[++j] = input[i];    
			}	else { 
				output[j] = input[i]; 
				output[++j] = '&'; 
				output[++j] = color; 
			} 
			flag = !flag;
		} else if (gecho && (input[i] == '\\')) {
			if (input[++i] == 'n') {
				output[j] = '\n';
			} else {
				output[j] = input[i];
			} 
		} else { 
			output[j] = input[i];
		}
		j++; 
	} 
	
	output[j] = '&'; /* stop color bleed before it starts, kids */ 
	output[++j] = 'n';
}


int parse_pemote(char *argument, char *vict_arg, char *room_arg)
{
	int i, found = 0;
  char *tester = argument;

	if (!*argument)	return 0;

  for ( ; *tester; tester++) {
    if (*tester == '|')
      found = 1;
  }

  if (!found) return 0;

	/* get the to_vict argument */
	skip_spaces(&argument);
  for (i = 0; *argument && *argument != '|'; i++) { 
		vict_arg[i] = *argument;
		argument++;
	} 
	/* Close off color and close up the string */
	vict_arg[i] = '&';
	vict_arg[++i] = 'n';
	vict_arg[++i] = '\0';

  if (*argument == '|') argument++;  //which it *should* at this point

  /* get the to_room argument */
	skip_spaces(&argument);
	if (!*argument) return 0;

  for (i = 0; *argument; i++) { 
		room_arg[i] = *argument;
		argument++;
	} 
	/* Close off color and close up the string */
	room_arg[i] = '&';
	room_arg[++i] = 'n';
	room_arg[++i] = '\0';

  return 1;
}


#define TOG_OFF 0
#define TOG_ON  1

void tog_rp(struct char_data *ch)
{
	bitvector_t result;
	room_rnum to_room = NOWHERE;

	const char *tog_messages[2][2] = {
		{"You go &WOut Of Character&n.\r\n",
		"$n goes &WOut Of Character&n."},
		{"You enter &WIn Character&n mode.\r\n",
		"$n goes &WIn Character&n."}
	};

	if (IS_NPC(ch))
		return;

	if (!IS_ACTIVE(ch)) {
		send_to_char("&RYou cannot participate in IC activities since your name has to be approved.&n\r\n", ch);
		if (IS_IC(ch))
			REMOVE_BIT(SESSION_FLAGS(ch), SESS_IC);
		return;
	}

	// Check to make sure they can toggle.
	if (IS_IC(ch) && (FIGHTING(ch) || GET_POS(ch) < POS_RESTING) && IS_MORTAL(ch)) {
		send_to_charf(ch, "You are not allowed to toggle in OOC mode now.\r\n");
		return;
	}

	result = SESS_TOG_CHK(ch, SESS_IC);

	if (result) {

		/*
		 * Toggling into IC, take them to the last room they were in.
		 * Torgny Bjers (artovil@arcanerealms.org), 2002-12-30.
		 * Or, if there's no last room, if they own a house, take them there.
		 * Catherine Gore (cheron@arcanerealms.org, 2003-01-17.
		 * Or, if there's a home town set and it's not one of the OOC areas, take
		 * them there.
		 * Catherine Gore (cheron@arcanerealms.org, 2004-03-15.
		 */

		REMOVE_BIT(SESSION_FLAGS(ch), SESS_IC);

		send_to_char(tog_messages[TOG_ON][0], ch);
		act(tog_messages[TOG_ON][1], FALSE, ch, 0, 0, TO_ROOM);

		SET_BIT(SESSION_FLAGS(ch), SESS_IC);

		to_room = real_room(GET_LOADROOM(ch));

		if (to_room == NOWHERE || to_room == IN_ROOM(ch) || to_room < 10) {
			if ((to_room = real_room(find_house_owner(ch))) == NOWHERE) {
				if (GET_HOME(ch) != ooc_lounge_index && GET_HOME(ch) != start_zone_index) {
					if ((to_room = real_room(mortal_start_room[GET_HOME(ch)])) == NOWHERE)
						to_room = real_room(mortal_start_room[default_ic_index]);
				} else {
					to_room = real_room(mortal_start_room[default_ic_index]);
				}
			}
		}

	} else {

		/*
		 * Toggling into OOC, take them to the OOC lounge.
		 * Torgny Bjers (artovil@arcanerealms.org), 2002-12-30.
		 */

		if (IS_MORTAL(ch))
			GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

		free_doing(ch);
		SET_BIT(SESSION_FLAGS(ch), SESS_IC);
	
		act(tog_messages[TOG_OFF][1], FALSE, ch, 0, 0, TO_ROOM);
		send_to_char(tog_messages[TOG_OFF][0], ch);

		REMOVE_BIT(SESSION_FLAGS(ch), SESS_IC);
	
		to_room = real_room(mortal_start_room[ooc_lounge_index]);
	}

	if (IS_MORTAL(ch)) {
		char_from_room(ch);
		char_to_room(ch, to_room);
		act("&M$n appears in the middle of the room.&n", TRUE, ch, 0, 0, TO_ROOM);
		look_at_room(ch, 0);
		entry_memory_mtrigger(ch);
		greet_mtrigger(ch, -1);
		greet_memory_mtrigger(ch);
	}
	
}


void free_doing(struct char_data *ch) 
{
	if (GET_DOING(ch))
		free(GET_DOING(ch));
	GET_DOING(ch) = NULL;
	send_to_char("Standard doing message will now be shown.\r\n", ch);
}

/* 
 * Ch: You
 * Victim: The person whose name you want to see.
 * Returns true if you are an immortal, you are the victim, or if your name
 * appears on their list of recognized people.
 */
bool find_recognized(struct char_data *ch, struct char_data *vict)
{
	int i;

	if (ch == NULL)
		return (FALSE);

	if (vict == NULL)
		return (FALSE);

	if (!ch->desc || !vict->desc)
		return (FALSE);

	if (IS_IMMORTAL(ch))
		return (TRUE);

	if (ch == vict)
		return (TRUE);

	for (i = 0; i < MAX_RECOGNIZED; i++)
		if (GET_RECOGNIZED(vict, i) == GET_IDNUM(ch))
			return (TRUE);

	return (FALSE);
}
