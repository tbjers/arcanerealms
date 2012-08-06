/* *************************************************************************
*  File: act.roleplay.c                           Extension to CircleMUD   *
*  Usage: Source file for roleplay commands code                           *
*                                                                          *
*  This file made exclusively for Arcane Realms MUD by Catherine Gore.     *
*  Copyright (C) 1999-2002, Torgny Bjers, and Catherine Gore.              *
*                                                                          *
*	 All rights reserved.  See license.doc for complete information.         *
*                                                                          *
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University  *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
************************************************************************** */

#define	__ACT_ROLEPLAY_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "interpreter.h" 
#include "constants.h"
#include "screen.h"
#include "comm.h"
#include "handler.h"
#include "buffer.h"
#include "roleplay.h"


/* external variables */
extern struct room_data *world;

/* external functions */
void extract_char_final(struct char_data *ch);

/* local functions */
ACMD(do_rplog);
ACMD(do_echo);
ACMD(do_pemote);
ACMD(do_doing);


ACMD(do_rplog)
{

	int i, j = 0;
	char *printbuf;
	struct char_data *victim = NULL;
	unsigned char snoop = 0, chk_file = 0; 
	
	if (IS_ADMIN(ch)) {
		char *printbuf2 = get_buffer(MAX_INPUT_LENGTH);
		char *printbuf3 = get_buffer(MAX_INPUT_LENGTH);
		two_arguments(argument, printbuf2, printbuf3);

		if (printbuf2 && *printbuf2 && is_abbrev(printbuf2, "player")) { /* if the admin provided some text, let's check for name*/		

			if (!printbuf3 || !(*printbuf3)) {
				send_to_char("What player do you want to inspect?\r\n", ch);
				release_buffer(printbuf2);			
				release_buffer(printbuf3);
				return;
			}
			
			victim = get_player_vis(ch,printbuf3,NULL,FIND_CHAR_WORLD);
			if (victim == NULL) {
				send_to_char("No such player around.\r\n", ch);
				release_buffer(printbuf2);			
				release_buffer(printbuf3);
				return;
			}
			if (compare_rights(ch, victim) == -1) {
				send_to_char("Your rights are not sufficient to view that person's rplog.\r\n", ch);
				release_buffer(printbuf2);			
				release_buffer(printbuf3);
				return;
			}	
			snoop = 1;		
		} /* printbuf2 ... is_abbrev */

		else if (printbuf2 && *printbuf2 && is_abbrev(printbuf2, "file")) {

			CREATE(victim, struct char_data, 1);
			clear_char(victim);
			CREATE(victim->player_specials, struct player_special_data, 1);
			
			if (!(printbuf3 && *printbuf3)) {
				send_to_char("Who?\r\n", ch);
				release_buffer(printbuf2);			
				release_buffer(printbuf3);
				return;
			}

			if (load_char(printbuf3, victim) > -1) {
				if (compare_rights(ch, victim) == -1) { 
					send_to_char("Your rights are not sufficient to view that person's rplog.\r\n", ch);
					free_char(victim);
					release_buffer(printbuf2);			
					release_buffer(printbuf3);
					return;
				}
				else {
					chk_file = 1;
					snoop = 1;
				}		
			}
			else { /* the load char returned -1 */
				send_to_char("Player doesn't exist.\r\n", ch);
				free_char(victim);
				victim = NULL;
				release_buffer(printbuf2);			
				release_buffer(printbuf3);
				return;
			} /* end the else */

		} else if (printbuf2 && *printbuf2 && is_abbrev(printbuf2, "clear")) {
			for (i = 0; i < RPLOG_SIZE; i++) 
				GET_RPLOG(ch, i) = NULL;
			send_to_char("Cleared your rplog.\r\n", ch);
			release_buffer(printbuf2);			
			release_buffer(printbuf3);
			return;
		}

		release_buffer(printbuf2);			
		release_buffer(printbuf3);

	} /* IS_ADMIN */

  if (snoop) /* invasion of privacy in some instances, worth logging */
		extended_mudlog(BRF, SYSL_SNOOPS, TRUE, "%s spied %s's rplog.", GET_NAME(ch), GET_NAME(victim));

	printbuf = get_buffer(MAX_STRING_LENGTH);
	
	for (i = 0; i < RPLOG_SIZE; i++) {
		if (GET_RPLOG((snoop ? victim : ch), i)) {
			sprintf(printbuf, "(%2d): %s\r\n", i+1, GET_RPLOG((snoop ? victim : ch), i));	
			send_to_char(printbuf, ch);
			j++;
		}
	}

	if (j == 0) {
		sprintf(printbuf, "%s rplog appears to be empty.\r\n", snoop ? HSHR(victim) : "Your");
		send_to_char(CAP(printbuf), ch);
	}

	if (chk_file)
		free_char(victim);	
						    				
	release_buffer(printbuf);
}


ACMD(do_echo)
{
	char color; 
	char *buf, *buf2;
	struct char_data *room;
	skip_spaces(&argument);

	if (PLR_FLAGGED(ch, PLR_MUTED)) {
		send_to_char("You cannot do this while muted.\r\n", ch);
		return;
	}

	buf = get_buffer(MAX_STRING_LENGTH);
	buf2 = get_buffer(MAX_STRING_LENGTH);

	if (!*argument)
		send_to_char("Yes... but what?\r\n", ch);
	else {
		switch (subcmd) {
			case SCMD_ECHO:
				color = (GET_COLORPREF_ECHO(ch) ? GET_COLORPREF_ECHO(ch) : 'c');
				strcpy(buf2, argument);
				break;
			case SCMD_EMOTE:
				color = (GET_COLORPREF_EMOTE(ch) ? GET_COLORPREF_EMOTE(ch) : 'c');
				if (IS_OOC(ch)) 
					sprintf(buf2, "(ooc) %s %s", GET_NAME(ch), argument);
				else {
					sprintf(buf2, "$n %s", argument);
					rpxp_reward(ch, (int)strlen(argument) * RPXP_EMOTE);
				}
				break;
			case SCMD_POSE:
				color = (GET_COLORPREF_POSE(ch) ? GET_COLORPREF_POSE(ch) : 'c');
				if (IS_OOC(ch))
					sprintf(buf2, "(ooc) %s", argument);
				else {
					sprintf(buf2, "%s", argument);
					rpxp_reward(ch, (int)strlen(argument) * RPXP_POSE);
				}
				break;
			case SCMD_OEMOTE:
				color = (GET_COLORPREF_EMOTE(ch) ? GET_COLORPREF_EMOTE(ch) : 'c');
				sprintf(buf2, "(ooc) %s %s", GET_NAME(ch), argument);
				break;
			case SCMD_OPOSE:
				color = (GET_COLORPREF_POSE(ch) ? GET_COLORPREF_POSE(ch) : 'c');
				sprintf(buf2, "(ooc) %s", argument);
				break;
			default:
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Default case reached in do_echo.");
				release_buffer(buf);
				release_buffer(buf2);
				return;
		}

		parse_emote(buf2, buf, color, 0);
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			if ((subcmd == SCMD_POSE || subcmd == SCMD_OPOSE) && PRF_FLAGGED(ch, PRF_POSEID))
				send_to_charf(ch, "%s (%s)", buf, GET_NAME(ch));
			else
				act(buf, FALSE, ch, 0, 0, TO_CHAR);
		}
		
		if (!(subcmd == SCMD_POSE || subcmd == SCMD_OPOSE)) {
			act(buf, FALSE, ch, 0, 0, TO_ROOM);
		}

		/* leave buf alone, use another variable for the pose ids */
		for (room = world[IN_ROOM(ch)].people; room; room = room->next_in_room) {
			release_buffer(buf2);
			buf2 = get_buffer(MAX_STRING_LENGTH);
			sprintf(buf2, buf);
			if (subcmd == SCMD_POSE || subcmd == SCMD_OPOSE) {
				if (PRF_FLAGGED(room, PRF_POSEID))
					sprintf(buf2 + strlen(buf2), " ($n)");
				if (room != ch)
					act(buf2, FALSE, ch, 0, room, TO_VICT);
			}
			if (!(subcmd == SCMD_OEMOTE || subcmd == SCMD_OPOSE) && IS_IC(room) && IS_IC(ch) && !IS_NPC(ch) && !IS_NPC(room)) {
				char *tbuf = get_buffer(MAX_INPUT_LENGTH + MAX_NAME_LENGTH);
				add_to_rplog(room, translate_code(buf2, ch, room, tbuf));
				release_buffer(tbuf);
			}
		}		

		release_buffer(buf);
		release_buffer(buf2);
	}
}


/* Couldn't make it part of do_echo above because of the many places it needs to be sent */
ACMD(do_pemote)
{
  struct char_data *vict;
 	char color = (GET_COLORPREF_EMOTE(ch) ? GET_COLORPREF_EMOTE(ch) : 'c'); 
	char *mvict, *mroom, *mchar, *buf, *buf1, *buf2;

	if (PLR_FLAGGED(ch, PLR_MUTED)) {
		send_to_char("You cannot do this while muted.\r\n", ch);
		return;
	}

	if (IS_OOC(ch) && !IS_GOD(ch)) {
    send_to_char("This can only be done In-Character.\r\n", ch);
    return;
  }

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Usage: pemote <victim> <to-vict message> | <to-room message>\r\n", ch);
    return;
  }
  
	buf = get_buffer(MAX_STRING_LENGTH);
  half_chop(argument, buf, argument);

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM, 0))) {
    send_to_char(NOPERSON, ch);
		release_buffer(buf);
    return;
  }

  if (!IS_GOD(ch)) {
    if (IS_OOC(vict) && !IS_GOD(vict)) {
      send_to_char("That person is out of character.\r\n", ch);
      sprintf(buf, "%s tried to include you in the roleplay, but you are OOC.\r\n", GET_NAME(ch));
      send_to_char(buf, vict);
			release_buffer(buf);
      return;
    }
  }

	buf1 = get_buffer(MAX_STRING_LENGTH);
	buf2 = get_buffer(MAX_STRING_LENGTH);

  if (!(parse_pemote(argument, buf1, buf2))) {
    send_to_char("Usage: pemote <victim> <to-vict message> | <to-room message>\r\n", ch);
		release_buffer(buf);
		release_buffer(buf1);
		release_buffer(buf2);
    return;
  } else {
		/*buffer for translating $n to an actual name in translate_code function */
		char *trans_buf = get_buffer(MAX_NAME_LENGTH + MAX_INPUT_LENGTH);
		struct char_data *room; /* points to current person in room */
		mvict = get_buffer(MAX_STRING_LENGTH);
		mroom = get_buffer(MAX_STRING_LENGTH);
		mchar = get_buffer(MAX_STRING_LENGTH);

  	rpxp_reward(ch, (int)(strlen(buf1) + strlen(buf2)) * RPXP_EMOTE);
		parse_emote(buf1, buf, color, 0);
		sprintf(mvict, "$n %s", buf);
		act(mvict, FALSE, ch, 0, vict, TO_VICT); // Send out the to_vict message
		if (!IS_NPC(vict) && !IS_NPC(ch)) {
			add_to_rplog(vict, translate_code(mvict, ch, vict, trans_buf));
		}

		sprintf(mchar, "$n %s (to: %s)", buf, (find_recognized(ch, vict) ? GET_NAME(vict) : GET_SDESC(vict)));
		act(mchar, FALSE, ch, 0, vict, TO_CHAR); // Send out the to_char message
		release_buffer(trans_buf);
		trans_buf = get_buffer(MAX_NAME_LENGTH + MAX_INPUT_LENGTH);
		if (!IS_NPC(vict) && !IS_NPC(ch)) {
			add_to_rplog(ch, translate_code(mchar, ch, ch, trans_buf));
		}

		release_buffer(buf);
		buf = get_buffer(MAX_STRING_LENGTH);
		parse_emote(buf2, buf, color, 0);
    sprintf(mroom, "$n %s", buf);
    act(mroom, FALSE, ch, 0, vict, TO_NOTVICT); // Send out the to_room message
		for (room = world[IN_ROOM(ch)].people; room; room=room->next_in_room) {
			if (room && room != ch && room != vict && IS_IC(room)) {
				release_buffer(trans_buf);
				trans_buf = get_buffer(MAX_NAME_LENGTH + MAX_INPUT_LENGTH);
				if (!IS_NPC(room) && !IS_NPC(ch)) {
					add_to_rplog(room, translate_code(mroom, ch, room, trans_buf));
				}
			}
		}
		release_buffer(trans_buf);
  }
	release_buffer(buf);
	release_buffer(buf1);
	release_buffer(buf2);
	release_buffer(mvict);
	release_buffer(mroom);
	release_buffer(mchar);
}


ACMD(do_doing)
{
	if (ch->desc == NULL)
		return;

	if (!*argument) {
		free_doing(ch);
		return;
	}

	skip_spaces(&argument);
	
	if (GET_DOING(ch))
		GET_DOING(ch) = NULL;
	
	GET_DOING(ch) = str_dup(argument);
	sprintf(buf, "Okay, you are now %s\r\n", GET_DOING(ch));
	send_to_char(buf, ch);
}


ACMD(do_recognize)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	struct char_data *victim;
	int vict_id = 0;
	bool found = FALSE;
	int i;

	if (ch->desc == NULL)
		return;

	if (!*argument) {
		for (i = 0; i < MAX_RECOGNIZED; i++) {
			if (GET_RECOGNIZED(ch, i) > 0 && get_name_by_id(GET_RECOGNIZED(ch, i)) != NULL) {
				send_to_charf(ch, "%s\r\n", CAP(strdup(get_name_by_id(GET_RECOGNIZED(ch, i)))));
				found = TRUE;
			}
		}
		send_to_charf(ch, "%sUsage: %s { <name> | remove <name> }\r\n", found ? "\r\n" : "", CMD_NAME);
		return;
	}

	skip_spaces(&argument);

	two_arguments(argument, arg1, arg2);

	if (!(strn_cmp(arg1, "remove", 6))) {
		if (!*arg2)
			send_to_charf(ch, "Usage: %s { <name> | remove <name> }\r\n", CMD_NAME);
		else if ((vict_id = get_id_by_name(arg2)) == NOBODY)
			send_to_char(NOPERSON, ch);
		else {
			for (i = 0; i < MAX_RECOGNIZED; i++) {
				if (GET_RECOGNIZED(ch, i) == vict_id) {
					GET_RECOGNIZED(ch, i) = 0;
					if (!found)
						send_to_charf(ch, "Removed %s from your recognized list.\r\n", CAP(strdup(get_name_by_id(vict_id))));
					found = TRUE;
				}
			}
			if (!found)
				send_to_charf(ch, "No such person recognized.\r\n");
		}
	} else if (!(victim = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM, 1)))
		send_to_char(NOPERSON, ch);
	else {
		if (ch == victim) {
			send_to_charf(ch, "You should be able to recognize yourself, don't you think?\r\n");
			return;
		}
		if (IS_OOC(victim) || IS_NPC(victim))
			send_to_charf(ch, "You may not add NPC's or OOC characters.\r\n");
		else {
			for (i = 0; i < MAX_RECOGNIZED; i++) {
				if (GET_RECOGNIZED(ch, i) == GET_IDNUM(victim)) {
					found = TRUE;
					send_to_charf(ch, "You have already allowed %s to recognize you.\r\n", GET_NAME(victim));
				}
			}
			if (!found) {
				for (i = 0; i < MAX_RECOGNIZED; i++) {
					if (GET_RECOGNIZED(ch, i) == 0) {
						GET_RECOGNIZED(ch, i) = GET_IDNUM(victim);
						send_to_charf(ch, "Now allowing %s to recognize you.\r\n", GET_NAME(victim));
						found = TRUE;
						break;
					}
				}
				if (!found) {
					for (i = 0; i < MAX_RECOGNIZED; i++)
						if (GET_RECOGNIZED(ch, i - 1))
							GET_RECOGNIZED(ch, i - 1) = GET_RECOGNIZED(ch, i);
					GET_RECOGNIZED(ch, MAX_RECOGNIZED - 1) = GET_IDNUM(victim);
					send_to_charf(ch, "Now allowing %s to recognize you.\r\n", GET_NAME(victim));
				}
			}
		}
	}
}
