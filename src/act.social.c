/* ************************************************************************
*		File: act.social.c                                  Part of CircleMUD *
*	 Usage: Functions to handle socials                                     *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001, Torgny Bjers.                                       *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
************************************************************************ */
/* $Id: act.social.c,v 1.32 2003/01/12 23:18:08 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "dg_scripts.h"
#include "roleplay.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;

/* local globals */
int	top_of_socialt = -1;

/* local functions */
int	find_action(int cmd);
void boot_social_messages(void);
void create_command_list(void);
void free_action(struct social_messg *mess);
void free_social_messages(void);
ACMD(do_action);
ACMD(do_insult);

struct social_messg *soc_mess_list = NULL;

void free_action(struct social_messg *mess)
{
	if (mess->command) free(mess->command);
	if (mess->sort_as) free(mess->sort_as);
	if (mess->char_no_arg) free(mess->char_no_arg);
	if (mess->others_no_arg) free(mess->others_no_arg);
	if (mess->char_found) free(mess->char_found);
	if (mess->others_found) free(mess->others_found);
	if (mess->vict_found) free(mess->vict_found);
	if (mess->char_body_found) free(mess->char_body_found);
	if (mess->others_body_found) free(mess->others_body_found);
	if (mess->vict_body_found) free(mess->vict_body_found);
	if (mess->not_found) free(mess->not_found);
	if (mess->char_auto) free(mess->char_auto);
	if (mess->others_auto) free(mess->others_auto);
	if (mess->char_obj_found) free(mess->char_obj_found);
	if (mess->others_obj_found) free(mess->others_obj_found);
	memset(mess, 0, sizeof(struct social_messg));
}


void free_social_messages(void)
{
	int ac;
	struct social_messg *mess;

	for (ac = 0; ac <= top_of_socialt; ac++) {
		mess = &soc_mess_list[ac];

		if (mess->command) free(mess->command);
		if (mess->sort_as) free(mess->sort_as);
		if (mess->char_no_arg) free(mess->char_no_arg);
		if (mess->others_no_arg) free(mess->others_no_arg);
		if (mess->char_found) free(mess->char_found);
		if (mess->others_found) free(mess->others_found);
		if (mess->vict_found) free(mess->vict_found);
		if (mess->char_body_found) free(mess->char_body_found);
		if (mess->others_body_found) free(mess->others_body_found);
		if (mess->vict_body_found) free(mess->vict_body_found);
		if (mess->not_found) free(mess->not_found);
		if (mess->char_auto) free(mess->char_auto);
		if (mess->others_auto) free(mess->others_auto);
		if (mess->char_obj_found) free(mess->char_obj_found);
		if (mess->others_obj_found) free(mess->others_obj_found);
	}
	free(soc_mess_list);
}


int	find_action(int cmd)
{
	int bot, top, mid;

	bot = 0;
	top = top_of_socialt;

	if (top < 0)
		return (-1);

	for (;;) {
		mid = (bot + top) / 2;

		if (soc_mess_list[mid].act_nr == cmd)
			return (mid);
		if (bot >= top)
			return (-1);

		if (soc_mess_list[mid].act_nr > cmd)
			top = --mid;
		else
			bot = ++mid;
	}
}



ACMD(do_action)
{
	int act_nr;
	struct social_messg *action;
	struct char_data *vict;
	struct obj_data *targ;

 /* For future possible outvoting from the polls
	 if (IS_IC(ch)) {
		send_to_char("Socials may not be used while IC, try oemote instead.\r\n", ch);
		return;
	}
*/

	if ((act_nr = find_action(cmd)) < 0) {
		send_to_char("That action is not supported.\r\n", ch);
		return;
	}

	action = &soc_mess_list[act_nr];

	two_arguments(argument, buf, buf2);

	if ((!action->char_body_found) && (*buf2)) {
		send_to_char("Sorry, this social does not support body parts.\r\n", ch);
		return;
	}

	if (!action->char_found) *buf = '\0';

	if (action->char_found && argument)
		one_argument(argument, buf);
	else
		*buf = '\0';

	if (!*buf) {
		send_to_char(action->char_no_arg, ch);
		send_to_char("\r\n", ch);
		act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM | TO_OOC);
		return;
	}
	if (!(vict = get_char_room_vis(ch, buf, NULL, 0))) {
		if ((action->char_obj_found) &&
				((targ = get_obj_in_list_vis(ch, buf, NULL, ch->carrying)) ||
				(targ = get_obj_in_list_vis(ch, buf, NULL, world[IN_ROOM(ch)].contents)))) {
			act(action->char_obj_found, action->hide, ch, targ, 0, TO_CHAR | TO_OOC);
			act(action->others_obj_found, action->hide, ch, targ, 0, TO_ROOM | TO_OOC);
			return;
		}
		if (action->not_found)
			send_to_char(action->not_found, ch);
		else
			send_to_char("I don't see anything by that name here.", ch);
		send_to_char("\r\n", ch);
		return;
	} else if (vict == ch) {
		if (action->char_auto)
			send_to_char(action->char_auto, ch);
		else
			send_to_char("Erm, no.", ch);
		send_to_char("\r\n", ch);
		act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM | TO_OOC);
	} else {
		if (GET_POS(vict) < action->min_victim_position)
			act("$N is not in a proper position for that.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP | TO_OOC);
		else {
			if (*buf2) {
				act(action->char_body_found, 0, ch, (struct obj_data *)buf2, vict, TO_CHAR | TO_SLEEP | TO_OOC);
				act(action->others_body_found, action->hide, ch, (struct obj_data *)buf2, vict, TO_NOTVICT);
				act(action->vict_body_found, action->hide, ch, (struct obj_data *)buf2, vict, TO_VICT);
			} else {
				act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP | TO_OOC);
				act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT | TO_OOC);
				act(action->vict_found, action->hide, ch, 0, vict, TO_VICT | TO_OOC);
			}
		}
	}
}



ACMD(do_insult)
{
	struct char_data *victim;

	one_argument(argument, arg);

	if (*arg) {
		if (!(victim = get_char_room_vis(ch, arg, NULL, 0)))
			send_to_char("Can't hear you!\r\n", ch);
		else {
			if (victim != ch) {

				sprintf(buf, "You insult %s.\r\n", GET_NAME(victim));
				send_to_char(buf, ch);

				switch (number(0, 2)) {
				case 0:
					if (GET_SEX(ch) == SEX_MALE) {
						if (GET_SEX(victim) == SEX_MALE)
							act("$n accuses you of fighting like a woman!", FALSE, ch, 0, victim, TO_VICT | TO_OOC);
						else
							act("$n says that women can't fight.", FALSE, ch, 0, victim, TO_VICT | TO_OOC);
					} else {		/* Ch == Woman */
						if (GET_SEX(victim) == SEX_MALE)
							act("$n accuses you of having the smallest... (brain?)",
						FALSE, ch, 0, victim, TO_VICT | TO_OOC);
						else
							act("$n tells you that you'd lose a beauty contest against a troll.",
						FALSE, ch, 0, victim, TO_VICT | TO_OOC);
					}
					break;
				case 1:
					act("$n calls your mother a bitch!", FALSE, ch, 0, victim, TO_VICT | TO_OOC);
					break;
				default:
					act("$n tells you to get lost!", FALSE, ch, 0, victim, TO_VICT | TO_OOC);
					break;
				}			/* end switch */

				act("$n insults $N.", TRUE, ch, 0, victim, TO_NOTVICT | TO_OOC);
			} else {			/* ch == victim */
				send_to_char("You feel insulted.\r\n", ch);
			}
		}
	} else
		send_to_char("I'm sure you don't want to insult *everybody*...\r\n", ch);
}


void boot_social_messages(void)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int curr_soc = -1;
	unsigned long *fieldlength;

	if (!(result = mysqlGetResource(TABLE_SOCIALS, "SELECT * FROM %s ORDER BY sort_as ASC;", TABLE_SOCIALS))) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Could not load social table.");
		extended_mudlog(NRM, SYSL_SQL, TRUE, "Query error (%s)%s:%d: %s", __FILE__, __FUNCTION__, __LINE__, mysql_error(&mysql));
		kill_mysql();
		exit(1);
	}

	/* count socials & allocate space */
	top_of_socialt = mysql_num_rows(result);
	mlog("   %d socials loaded.", top_of_socialt);

	CREATE(soc_mess_list, struct social_messg, top_of_socialt);

	for(curr_soc = 0; curr_soc < top_of_socialt; curr_soc++)
	{
		row = mysql_fetch_row(result);
		fieldlength = mysql_fetch_lengths(result);
		soc_mess_list[curr_soc].id = atoi(row[0]);
		soc_mess_list[curr_soc].command = str_dup(row[1]);
		soc_mess_list[curr_soc].sort_as = str_dup(row[2]);
		/* numbers */
		soc_mess_list[curr_soc].hide = atoi(row[3]);
		soc_mess_list[curr_soc].min_char_position = atoi(row[4]);
		soc_mess_list[curr_soc].min_victim_position = atoi(row[5]);
		/* strings */
		soc_mess_list[curr_soc].char_no_arg = NULL_STR(fieldlength[6], row[6]);
		soc_mess_list[curr_soc].others_no_arg = NULL_STR(fieldlength[7], row[7]);
		soc_mess_list[curr_soc].char_found = NULL_STR(fieldlength[8], row[8]);
		soc_mess_list[curr_soc].others_found = NULL_STR(fieldlength[9], row[9]);
		soc_mess_list[curr_soc].vict_found = NULL_STR(fieldlength[10], row[10]);
		soc_mess_list[curr_soc].not_found = NULL_STR(fieldlength[11], row[11]);
		soc_mess_list[curr_soc].char_auto = NULL_STR(fieldlength[12], row[12]);
		soc_mess_list[curr_soc].others_auto = NULL_STR(fieldlength[13], row[13]);
		soc_mess_list[curr_soc].char_body_found = NULL_STR(fieldlength[14], row[14]);
		soc_mess_list[curr_soc].others_body_found = NULL_STR(fieldlength[15], row[15]);
		soc_mess_list[curr_soc].vict_body_found = NULL_STR(fieldlength[16], row[16]);
		soc_mess_list[curr_soc].char_obj_found = NULL_STR(fieldlength[17], row[17]);
		soc_mess_list[curr_soc].others_obj_found = NULL_STR(fieldlength[18], row[18]);
	}

	top_of_socialt = curr_soc - 1;

	mysql_free_result(result);

}
