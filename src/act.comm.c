/* ************************************************************************
*		File: act.comm.c                                    Part of CircleMUD *
*	 Usage: Player-rights communication commands                             *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: act.comm.c,v 1.66 2004/05/11 02:24:00 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "commands.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "dg_scripts.h"
#include "roleplay.h"

/* extern variables */
extern int holler_move_cost;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern const char *languages[];
extern struct skill_info_type skill_info[TOP_SKILL_DEFINE + 1];

/* extern functions */
struct char_data *find_char(long n);

/* local functions */
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
int	is_tell_ok(struct char_data *ch, struct char_data *vict);
void list_languages(struct char_data *ch);
void garble_text(char *string, int percent);
void add_tell_to_buffer(struct char_data *ch, const char *tell);

ACMD(do_say);

ACMD(do_languages);
ACMD(do_sayelven);
ACMD(do_saygnomish);
ACMD(do_saydwarven);
ACMD(do_sayancient);
ACMD(do_saytheif);

ACMD(do_osay);

ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);
ACMD(do_contact);

void list_languages(struct char_data *ch)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	int a = 0, i;
	sprintf(printbuf, "You know the following languages:\r\n");
	
	strcat(printbuf, "&w[&c ");
	
	for (i = MIN_LANGUAGES; i < MAX_LANGUAGES; i++) {
		if (GET_SKILL(ch, i) > 30) {
			a++;
			sprintf(printbuf + strlen(printbuf), "%s%s%s ", ((SPEAKING(ch)==i) ?"&C":""), skill_info[i].name, ((SPEAKING(ch)==i) ?"&c":""));
		}
	}
	
	strcat(printbuf, "&w]&n\r\n");
	
	send_to_char(printbuf, ch);

	send_to_char("Usage: speak <language>\r\n", ch);

	release_buffer(printbuf);

}


ACMD(do_languages)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH), *args = get_buffer(MAX_INPUT_LENGTH);
	int i, found = FALSE;
	one_argument(argument, args);
	
	if (!*args)
		list_languages(ch);
	else {
		for (i = MIN_LANGUAGES; i < MAX_LANGUAGES; i++) {
			if (is_abbrev(args, skill_info[i].name) && GET_SKILL(ch, i) > 60) {
				SPEAKING(ch) = i;
				sprintf(printbuf, "You now speak %s.\r\n", skill_info[i].name);
				send_to_char(printbuf, ch);
				found = TRUE;
				break;
			}
		}
		if (!found)
			list_languages(ch);
	}
	release_buffer(args);
	release_buffer(printbuf);
}


void garble_text(char *string, int percent)
{
	char letters[] = "aeiousqhpwxyz";
	int i;
	
	for (i = 0; i < (int)strlen(string); ++i)
		if (isalpha(string[i]) && number(0, 1) && number(0, 100) > percent)
			string[i] = letters[number(0, 12)];
}


ACMD(do_say)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char ibuf[MAX_INPUT_LENGTH];
	char obuf[MAX_INPUT_LENGTH];
	struct char_data *tch;
	int language = 0, roll;
	char color, *nchar = get_buffer(MAX_NAME_LENGTH);
	struct char_data *sayto = NULL;

	if (PLR_FLAGGED(ch, PLR_MUTED)) {
		send_to_char("You try but you can't form the words.\r\n", ch);
		release_buffer(printbuf);
		release_buffer(nchar);
		return;
	}

	/* prevent animals and undefineds from speaking */
	if (GET_RACE(ch) == RACE_ANIMAL || GET_RACE(ch) == RACE_UNDEFINED) {
		send_to_char("Hey! You can't speak... you're not humanoid!", ch);
		return;
	}

	switch (subcmd) {
	case SCMD_OSAY:
		break;
	case SCMD_SAY:
		if (!IS_NPC(ch))
			language = SPEAKING(ch);
		else
			language = LANG_COMMON;
		if (!IS_IC(ch))
			subcmd = SCMD_OSAY;
		break;
	case SCMD_SAYTO:
		skip_spaces(&argument);
		half_chop(argument, nchar, argument);
		if (!*nchar) {
			send_to_char("Whom do you wish to speak to?\r\n", ch);
			release_buffer(nchar);
			release_buffer(printbuf);
			return;
		}
		if (!(sayto = get_char_vis(ch, nchar, NULL, FIND_CHAR_ROOM, 0))) {
			send_to_char(NOPERSON, ch);
			release_buffer(nchar);
			release_buffer(printbuf);
			return;
		}
		break;
	case SCMD_COMMON:		language = LANG_COMMON;		break;
	case SCMD_ELVEN:		language = LANG_ELVEN;		break;
	case SCMD_GNOMISH:	language = LANG_GNOMISH;	break;
	case SCMD_DWARVEN:	language = LANG_DWARVEN;	break;
	case SCMD_ANCIENT:	language = LANG_ANCIENT;	break;
	case SCMD_THIEF:		language = LANG_THIEF;		break;
	case SCMD_SIGNLANG:	language = LANG_SIGN;			break;
	default:
		SPEAKING(ch) = LANG_COMMON;
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "(%s)%s:%d: Invalid SCMD passed (%d)", __FILE__, __FUNCTION__, __LINE__, subcmd);
		release_buffer(printbuf);
		release_buffer(nchar);
		return;
	}

	skip_spaces(&argument);
	
	if(!*argument) {
		send_to_char("Yes, but WHAT do you want to say?\r\n", ch);
		release_buffer(printbuf);
		release_buffer(nchar);
		return;
	}
	
	if (IS_IC(ch) && subcmd != SCMD_OSAY && subcmd != SCMD_SAYTO && subcmd != SCMD_SIGNLANG && !IS_NPC(ch)) {

		strcpy(ibuf, argument); /* preserve original text */

		if (!skill_check(ch, language, 0)) {
			send_to_char("You try, but can't seem to find the right words.\r\n", ch);
			return;
		}

		switch (language) {
		case LANG_ELVEN:
		case LANG_GNOMISH:
		case LANG_DWARVEN:
		case LANG_ANCIENT:
		case LANG_THIEF:
			if (!IS_NPC(ch))
				garble_text(ibuf, GET_SKILL(ch, SPEAKING(ch)));
			break;
		default:
			break;
		}

		color = (GET_COLORPREF_SAY(ch) ? GET_COLORPREF_SAY(ch) : 'C'); 

		for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
			if (tch != ch && AWAKE(tch) && tch->desc && (IS_IC(tch) || IS_IMMORTAL(tch))) {
				
				if (GET_POS(tch) == POS_MEDITATING) {
					if ((roll = number(1, 101)) < (GET_SKILL(tch, SKILL_MEDITATE)/100)) {
						if (roll > 90)
							send_to_char("You hear voices on the edges of your meditation...\r\n", tch);
						else if (roll > 50)
							send_to_char("You hear some murmurs on the edges of your meditation...\r\n", tch);
						else if (roll > 25)
							send_to_char("You hear faint murmurs on the edges of your meditation...\r\n", tch);
						else if (roll > 10)
							send_to_char("You hear something humming softly on the edge of your meditation.\r\n", tch);
						continue;
					}	else {
						send_to_char("The speech around you breaks through your concentration!\r\n", tch);
						GET_POS(ch) = POS_RESTING;
					}
				}

				strcpy(obuf, ibuf); /* preserve the first garble */
				
				if (!IS_NPC(ch) && !IS_NPC(tch))
					garble_text(obuf, GET_SKILL(tch, language));
				
				
				if (!GET_SKILL(tch, language))
					sprintf(printbuf, "$n says in an unfamiliar tongue, '&%c%s&n'", color, obuf);
				else
					sprintf(printbuf, "$n says in the %s tongue,\r\n    '&%c%s&n'", skill_info[language].name, color, obuf);
				
				CAP(printbuf);
				if (!IN_OLC(tch->desc)) {
				  char *trans_buf = get_buffer(MAX_INPUT_LENGTH + MAX_NAME_LENGTH);	
					act(printbuf, TRUE, ch, 0, tch, TO_VICT | DG_NO_TRIG);
					if (!IS_NPC(ch) && !IS_NPC(tch))
						add_to_rplog(tch, translate_code(printbuf, ch, tch, trans_buf));
					release_buffer(trans_buf);
				}
			}
		}

		rpxp_reward(ch, strlen(argument) * RPXP_SAY);
				
		sprintf(printbuf, "You say in the %s tongue,\r\n    '&%c%s&n'", skill_info[language].name, color, argument);
		act(printbuf, TRUE, ch, 0, 0, TO_CHAR);
		if (!IS_NPC(ch)) {
			add_to_rplog(ch, printbuf);
		}

	} else if (IS_IC(ch) && subcmd == SCMD_SIGNLANG && !IS_NPC(ch)) {

		color  = (GET_COLORPREF_SAY(ch) ? GET_COLORPREF_SAY(ch) : 'C');

		for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
			if (tch != ch && AWAKE(tch) && tch->desc && (IS_IC(tch) || IS_IMMORTAL(tch))) {
				
			if (GET_SKILL(tch, language) < 35)
					sprintf(printbuf, "$n makes some strange gestures.");
				else
					sprintf(printbuf, "$n says in sign language,\r\n    '&%c%s&n'", color, argument);
				
				CAP(printbuf);
				if (!IN_OLC(tch->desc))	{
					char *trans_buf = get_buffer(MAX_INPUT_LENGTH + MAX_NAME_LENGTH);
					act(printbuf, TRUE, ch, 0, tch, TO_VICT | DG_NO_TRIG);
					if (!IS_NPC(ch) && !IS_NPC(tch))
						add_to_rplog(tch, translate_code(printbuf, ch, tch, trans_buf));
					release_buffer(trans_buf);
				}
			}
		}

		rpxp_reward(ch, strlen(argument) * RPXP_SAY);
		
		sprintf(printbuf, "&cYou say in sign language,\r\n    '&%c%s&c'&n", color, argument);
		act(printbuf, TRUE, ch, 0, 0, TO_CHAR);
		if (!IS_NPC(ch)) {
			add_to_rplog(ch, printbuf);
		}
	} else {
		/*
		 * We have passed through the other commands, and therefore
		 * it must mean that they speak OOCly.
		 */
		color = (GET_COLORPREF_OSAY(ch) ? GET_COLORPREF_OSAY(ch) : 'c');

		/* Let's send it to the room... */
		if (subcmd == SCMD_SAYTO)
			sprintf(printbuf, "$n says to $N, '&%c%s&n'", color, argument);
		else
			sprintf(printbuf, "$n says%s, '&%c%s&n'", (subcmd == SCMD_OSAY ? " OOCly" : ""), color, argument);
		/*  At this point would it BE anything other than SCMD_OSAY? ^^^^ */

		act(printbuf, FALSE, ch, 0, sayto, TO_NOTVICT | DG_NO_TRIG | TO_OOC);

		/* Let's send it to the victim (sayto) */
		if (subcmd == SCMD_SAYTO)
			sprintf(printbuf, "$n says to you, '&%c%s&n'", color, argument);
		else
			sprintf(printbuf, "$n says%s, '&c%c%s&n'", (subcmd == SCMD_OSAY ? " OOCly" : ""), color, argument);

		act(printbuf, FALSE, ch, 0, sayto, TO_VICT | DG_NO_TRIG | TO_OOC);

		/* Let's send it to the char */
		if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			if (subcmd == SCMD_SAYTO)
				sprintf(printbuf, "You say to $N, '&%c%s&n'", color, argument);
			else
				sprintf(printbuf, "You say%s, '&%c%s&n'", (subcmd == SCMD_OSAY ? " OOCly" : ""), color, argument);
			
			act(printbuf, FALSE, ch, 0, sayto, TO_CHAR | TO_OOC);
		}		

		/* trigger check */
		speech_mtrigger(ch, argument);
		speech_wtrigger(ch, argument);

	}

	release_buffer(printbuf);
	release_buffer(nchar);
	
}


ACMD(do_gsay)
{
	char *printbuf;
	struct char_data *k;
	struct follow_type *f;

	if (PLR_FLAGGED(ch, PLR_MUTED)) {
		send_to_char("You try but you cannot form the words.\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	if (!AFF_FLAGGED(ch, AFF_GROUP)) {
		send_to_char("But you are not the member of a group!\r\n", ch);
		return;
	}
	printbuf = get_buffer(MAX_STRING_LENGTH);
	if (!*argument)
		send_to_char("Yes, but WHAT do you want to group-say?\r\n", ch);
	else {
		if (ch->master)
			k = ch->master;
		else
			k = ch;

		sprintf(printbuf, "$n tells the group, '%s'", argument);

		if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch))
			act(printbuf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
		for (f = k->followers; f; f = f->next)
			if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch))
				act(printbuf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);

		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			sprintf(printbuf, "You tell the group, '%s'", argument);
			send_to_char(printbuf, ch);
		}
	}
	release_buffer(printbuf);
}


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char *timebuf = get_buffer(32);
	sprintf(printbuf, "&n$n tells you, '&W%s&n'", arg);
	act(printbuf, FALSE, ch, 0, vict, TO_VICT | TO_OOC | TO_SLEEP);

	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
	else {
		sprintf(printbuf, "&nYou tell $N, '&W%s&n'", arg);
		act(printbuf, FALSE, ch, 0, vict, TO_CHAR | TO_OOC | TO_SLEEP);
	}

	if (SESS_FLAGGED(vict, SESS_AFK)) {
		sprintf(printbuf, "%s is AFK and may not see your tell.\r\n", GET_NAME(vict));
		send_to_char(printbuf, ch);
		sprintf(printbuf, "&mMESSAGE: &M%s&n\r\n", GET_AWAY(vict));
		send_to_char(printbuf, ch);
	}

	if (SESS_FLAGGED(vict, SESS_AFW)) {
		sprintf(printbuf, "%s is AFW and may not see your tell.\r\n", GET_NAME(vict));
		send_to_char(printbuf, ch);
		sprintf(printbuf, "&mMESSAGE: &M%s&n\r\n", GET_AWAY(vict));
		send_to_char(printbuf, ch);
	}

	if (!IS_NPC(vict) && !IS_NPC(ch))
		GET_LAST_TELL(vict) = GET_IDNUM(ch);

	if (PRF_FLAGGED(vict, PRF_TIMESTAMPS)) {
		time_t ct = time(0);
		char *time_s = asctime(localtime(&ct));
		time_s[strlen(time_s) - 1] = '\0';
		sprintf(timebuf, "%8.8s: ", time_s + 11);
	} else strcpy(timebuf, "");

	if (!IS_NPC(ch)) {
		sprintf(printbuf, "%s%s: '%s'", timebuf, GET_NAME(ch), arg);
		add_tell_to_buffer(vict, printbuf);
	}

	release_buffer(printbuf);
	release_buffer(timebuf);
}


int	is_tell_ok(struct char_data *ch, struct char_data *vict)
{
	if (ch == vict)
		send_to_char("You try to tell yourself something.\r\n", ch);
	else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
		send_to_char("You can't tell other people while you have notell on.\r\n", ch);
	else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF))
		send_to_char("The walls seem to absorb your words...\r\n", ch);
	else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
		act("$E's linkless at the moment...", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (!IS_NPC(vict) && EDITING(vict->desc))
		act("$E's writing a message right now...", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (PLR_FLAGGED(vict, PLR_OLC))
		act("$E's using OLC right now...", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF))
		act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else
		return (TRUE);

	return (FALSE);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
	char *printbuf = get_buffer(MAX_INPUT_LENGTH);
	char *printbuf2 = get_buffer(MAX_STRING_LENGTH);
	char *timebuf = get_buffer(32);
	struct char_data *vict = NULL;

	half_chop(argument, printbuf, printbuf2);

	if (!*printbuf || !*printbuf2)
		send_to_char("Who do you wish to tell what?\r\n", ch);
	else if (!(vict = get_char_vis(ch, printbuf, NULL, FIND_CHAR_WORLD, 1)))
		send_to_char(NOPERSON, ch);
	else if (is_tell_ok(ch, vict))
		perform_tell(ch, vict, printbuf2);
	else {
		if (ch != vict && !IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOTELL)) {
			if (PRF_FLAGGED(vict, PRF_TIMESTAMPS)) {
				time_t ct = time(0);
				char *time_s = asctime(localtime(&ct));
				time_s[strlen(time_s) - 1] = '\0';
				sprintf(timebuf, "%8.8s: ", time_s + 11);
			} else strcpy(timebuf, "");
			sprintf(printbuf, "%s%s: '%s'", timebuf, GET_NAME(ch), printbuf2);
			act("&RThe tell has been added to $S tell buffer.&n", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
			add_tell_to_buffer(vict, printbuf);
		}
	}
	release_buffer(printbuf);
	release_buffer(printbuf2);
	release_buffer(timebuf);
}


ACMD(do_reply)
{
	struct char_data *tch = character_list;

	if (IS_NPC(ch))
		return;

	skip_spaces(&argument);

	if (SESS_FLAGGED(ch, SESS_REPLYLOCK)) {
		if (!(tch = get_player_vis(ch, GET_REPLYTO(ch), NULL, FIND_CHAR_WORLD))) {
			send_to_char("They're not here!\r\n", ch);
			(void)SESS_TOG_CHK(ch, SESS_REPLYLOCK);
			return;
		} else {
			GET_LAST_TELL(ch) = GET_IDNUM(tch);
		}
	}

	if (GET_LAST_TELL(ch) == NOBODY)
		send_to_char("You have no-one to reply to!\r\n", ch);
	else if (!*argument)
		send_to_char("What is your reply?\r\n", ch);
	else {
		/*
		 * Make sure the person you're replying to is still playing by searching
		 * for them.  Note, now last tell is stored as player IDnum instead of
		 * a pointer, which is much better because it's safer, plus will still
		 * work if someone logs out and back in again.
		 */
																		 
		/*
		 * XXX: A descriptor list based search would be faster although
		 *      we could not find link dead people.  Not that they can
		 *      hear tells anyway. :) -gg 2/24/98
		 */
		while (tch != NULL && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
			tch = tch->next;

		if (tch == NULL)
			send_to_char("They are no longer playing.\r\n", ch);
		else if (IS_MORTAL(ch) && !(tch = get_player_vis(ch, GET_NAME(tch), NULL, FIND_CHAR_WORLD)))
			send_to_char(NOPERSON, ch);
		else if (IS_IMMORTAL(ch) && !(tch = get_char_vis(ch, GET_NAME(tch), NULL, FIND_CHAR_WORLD, 1)))
			send_to_char(NOPERSON, ch);
		else if (is_tell_ok(ch, tch))
			perform_tell(ch, tch, argument);
	}
}


ACMD(do_spec_comm)
{
	char *printbuf, *printbuf2;
	struct char_data *vict;
	const char *action_sing, *action_plur, *action_others;

	if (PLR_FLAGGED(ch, PLR_MUTED)) {
		send_to_char("You try but you cannot seem to form the words...\r\n", ch);
		return;
	}

	switch (subcmd) {
	case SCMD_COMM_WHISPER:
		action_sing = "whisper to";
		action_plur = "whispers to";
		action_others = "$n whispers something to $N.";
		break;

	case SCMD_COMM_ASK:
		action_sing = "ask";
		action_plur = "asks";
		action_others = "$n asks $N a question.";
		break;
		
	default:
		action_sing = "oops";
		action_plur = "oopses";
		action_others = "$n is tongue-tied trying to speak with $N.";
		break;
	}

	printbuf = get_buffer(MAX_STRING_LENGTH);
	printbuf2 = get_buffer(MAX_STRING_LENGTH);

	half_chop(argument, printbuf, printbuf2);

	if (!*printbuf || !*printbuf2) {
		sprintf(printbuf, "Whom do you want to %s.. and what??\r\n", action_sing);
		send_to_char(printbuf, ch);
	} else if (!(vict = get_char_vis(ch, printbuf, NULL, FIND_CHAR_ROOM, 0)))
		send_to_char(NOPERSON, ch);
	else if (vict == ch)
		send_to_char("You can't get your mouth close enough to your ear...\r\n", ch);
	else {
		sprintf(printbuf, "$n %s you, '%s'", action_plur, printbuf2);
		act(printbuf, FALSE, ch, 0, vict, TO_VICT);
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			sprintf(printbuf, "You %s %s, '%s'\r\n", action_sing, GET_NAME(vict), printbuf2);
			send_to_char(printbuf, ch);
		}
		act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
		/* trigger check */
		speech_mtrigger(ch, printbuf2);
	}
	release_buffer(printbuf);
	release_buffer(printbuf2);
}


ACMD(do_write)
{
	struct obj_data *paper, *pen = NULL;
	char *papername, *penname;

	if (!ch->desc)
		return;

	papername = get_buffer(MAX_INPUT_LENGTH);
	penname = get_buffer(MAX_INPUT_LENGTH);

	two_arguments(argument, papername, penname);

	if (!*papername) {                /* nothing was delivered */
		send_to_char("Write?  With what?  ON what?  What are you trying to do?!\r\n", ch);
		release_buffer(papername);
		release_buffer(penname);
		return;
	}
	if (*penname) {                /* there were two arguments */
		if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
			char *printbuf = get_buffer(1024);
			sprintf(printbuf, "You have no %s.\r\n", papername);
			send_to_char(printbuf, ch);
			release_buffer(printbuf);
			release_buffer(papername);
			release_buffer(penname);
			return;
		}
		if (!(pen = get_obj_in_list_vis(ch, penname, NULL, ch->carrying))) {
			char *printbuf = get_buffer(1024);
			sprintf(printbuf, "You have no %s.\r\n", penname);
			send_to_char(printbuf, ch);
			release_buffer(printbuf);
			release_buffer(papername);
			release_buffer(penname);
			return;
		}
	} else {                /* there was one arg.. let's see what we can find */
		if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
			char *printbuf = get_buffer(1024);
			sprintf(printbuf, "There is no %s in your inventory.\r\n", papername);
			send_to_char(printbuf, ch);
			release_buffer(printbuf);
			release_buffer(papername);
			release_buffer(penname);
			return;
		}
		if (GET_OBJ_TYPE(paper) == ITEM_PEN) {        /* oops, a pen.. */
			pen = paper;
			paper = NULL;
		} else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
			send_to_char("That thing has nothing to do with writing.\r\n", ch);
			release_buffer(papername);
			release_buffer(penname);
			return;
		}
		/* One object was found.. now for the other one. */
		if (!GET_EQ(ch, WEAR_HOLD)) {
			char *printbuf = get_buffer(1024);
			sprintf(printbuf, "You can't write with %s %s alone.\r\n", AN(papername),
							papername);
			send_to_char(printbuf, ch);
			release_buffer(printbuf);
			release_buffer(papername);
			release_buffer(penname);
			return;
		}
		if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
			send_to_char("The stuff in your hand is invisible!  Yeech!!\r\n", ch);
			release_buffer(papername);
			release_buffer(penname);
			return;
		}
		if (pen)
			paper = GET_EQ(ch, WEAR_HOLD);
		else
			pen = GET_EQ(ch, WEAR_HOLD);
	}


	/* ok.. now let's see what kind of stuff we've found */
	if (GET_OBJ_TYPE(pen) != ITEM_PEN)
		act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
	else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
		act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
	else {
		char *backstr = NULL;

		/* Something on it, display it as that's in input buffer. */
		if (paper->action_description) {
			backstr = str_dup(paper->action_description);
			send_to_char("There's something written on it already:\r\n", ch);
			send_to_char(paper->action_description, ch);
		}

		/* we can write - hooray! */
		act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
		string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, EDIT_NOTE);
	}

	release_buffer(papername);
	release_buffer(penname);
}



ACMD(do_page)
{
	char *args = get_buffer(MAX_INPUT_LENGTH), *printbuf2 = get_buffer(MAX_INPUT_LENGTH);
	struct descriptor_data *d;
	struct char_data *vict;

	half_chop(argument, args, printbuf2);

	if (IS_NPC(ch))
		send_to_char("Monsters can't page.. go away.\r\n", ch);
	else if (!*args)
		send_to_char("Whom do you wish to page?\r\n", ch);
	else {
		char *printbuf = get_buffer(MAX_STRING_LENGTH);
		sprintf(printbuf, "\007\007*$n* %s", printbuf2);
		if (!str_cmp(args, "all")) {
			if (IS_GRGOD(ch)) {
				for (d = descriptor_list; d; d = d->next)
					if ((STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) && d->character)
						act(printbuf, FALSE, ch, 0, d->character, TO_VICT);
			} else
				send_to_char("You will never be godly enough to do that!\r\n", ch);
			release_buffer(printbuf);
			release_buffer(args);
			release_buffer(printbuf2);
			return;
		}
		if ((vict = get_char_vis(ch, args, NULL, FIND_CHAR_WORLD, 1)) != NULL) {
			act(printbuf, FALSE, ch, 0, vict, TO_VICT);
			if (PRF_FLAGGED(ch, PRF_NOREPEAT))
				send_to_char(OK, ch);
			else
				act(printbuf, FALSE, ch, 0, vict, TO_CHAR);
		} else
			send_to_char("There is no such person in the game!\r\n", ch);
		release_buffer(printbuf);
		release_buffer(args);
		release_buffer(printbuf2);
	}
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
	*********************************************************************/

/*
* com_msgs: [0] Message if you can't perform the action because of noshout
*           [1] name of the action
*           [2] message if you're not on the channel
*           [3] a color string.
*/
const char *com_msgs[][5] = {
	{"You cannot holler!\r\n",
		"holler",
		"",
		"&y",
		"&Y"},

	{"You cannot shout!\r\n",
		"shout",
		"Turn off your noshout flag first!\r\n",
		"&y",
		"&Y"},

	{"You cannot socialize!\r\n",
		"social",
		"You aren't even on the channel!\r\n",
		"&m",
		"&M"},

	{"You cannot use the newbie channel!\r\n",
		"newbie",
		"You aren't even on the channel!\r\n",
		"&G",
		"&Y"},

	{"You cannot sing!\r\n",
		"sing",
		"You aren't even on the sing channel!\r\n",
		"&b",
		"&B"},

	{"You cannot obscene!\r\n",
		"obscene",
		"You aren't even on the obscene channel!\r\n",
		"&R",
		"&r"}
};


void add_to_comm_buff(int subcmd, char *message)
{
	char **ptr;
	int i = 0;

	switch (subcmd)	{
	case SCMD_COMM_SOC:
		ptr = &(gossip[0]);
		while (*ptr && (i < COMM_SIZE)) {
			ptr++;
			i++;
		}
	 
		if (i == COMM_SIZE) { // comm buffer is full, cycle it.
			for (i = 0; i < COMM_SIZE - 1; i++)
				strcpy(gossip[i], gossip[i+1]);
			ptr = &(gossip[COMM_SIZE - 1]);
			release_memory(*ptr);
		}
		break;
	case SCMD_COMM_NEWBIE:
		ptr = &(newbie[0]);
		while (*ptr && (i < COMM_SIZE)) {
			ptr++;
			i++;
		}
	 
		if (i == COMM_SIZE) { // comm buffer is full, cycle it.
			for (i = 0; i < COMM_SIZE - 1; i++)
				strcpy(newbie[i], newbie[i+1]);
			ptr = &(newbie[COMM_SIZE - 1]);
			release_memory(*ptr);
		}
		break;
	case SCMD_COMM_SING:
		ptr = &(sing[0]);
		while (*ptr && (i < COMM_SIZE)) {
			ptr++;
			i++;
		}
	 
		if (i == COMM_SIZE) { // comm buffer is full, cycle it.
			for (i = 0; i < COMM_SIZE - 1; i++)
				strcpy(sing[i], sing[i+1]);
			ptr = &(sing[COMM_SIZE - 1]);
			release_memory(*ptr);
		}
		break;
	case SCMD_COMM_OBSCENE:
		ptr = &(obscene[0]);
		while (*ptr && (i < COMM_SIZE)) {
			ptr++;
			i++;
		}
	 
		if (i == COMM_SIZE) { // comm buffer is full, cycle it.
			for (i = 0; i < COMM_SIZE - 1; i++)
				strcpy(obscene[i], obscene[i+1]);
			ptr = &(obscene[COMM_SIZE - 1]);
			release_memory(*ptr);
		}
		break;
	default:
		return;
	}

	if (!(*ptr)) {
		*ptr = get_memory(MAX_INPUT_LENGTH);
		strcpy(*ptr, message);  
	}
}


void show_comm_buff(struct char_data *ch, int subcmd)
{
	int i=0, j = 0;

	switch (subcmd)	{
		case SCMD_COMM_SOC:
			for (i = 0; i < COMM_SIZE; i++) {
				if (gossip[i]) {
					send_to_charf(ch, "%s\r\n", gossip[i]);
					j++;
				}
			}
			break;
		case SCMD_COMM_NEWBIE:
			for (i = 0; i < COMM_SIZE; i++) {
				if (newbie[i]) {
					send_to_charf(ch, "%s\r\n", newbie[i]);
					j++;
				}
			}
			break;
		case SCMD_COMM_SING:
			for (i = 0; i < COMM_SIZE; i++) {
				if (sing[i]) {
					send_to_charf(ch, "%s\r\n", sing[i]);
					j++;
				}
			}
			break;
		case SCMD_COMM_OBSCENE:
			for (i = 0; i < COMM_SIZE; i++) {
				if (obscene[i]) {
					send_to_charf(ch, "%s\r\n", obscene[i]);
					j++;
				}
			}
			break;
		default:
			return;
	}

	if (j == 0) {
		send_to_charf(ch, "The %s log appears to be empty.\r\n", com_msgs[subcmd][1]);
	}
}


ACMD(do_gen_comm)
{
	char *printbuf;
	struct descriptor_data *i;
	char color_braces[4];
	char color_text[4];
	char channel_name[24];

	/* Array of flags which must _not_ be set in order for comm to be heard */
	bitvector_t channels[] = {
		0,
		PRF_DEAF,
		PRF_NOSOC,
		PRF_NONEWBIE,
		PRF_NOSING,
		PRF_NOOBSCENE,
		0
	};


	/* to keep pets, etc from being ordered to shout */
	if (!ch->desc)
		return;
	if (PLR_FLAGGED(ch, PLR_MUTED)) {
		send_to_char("You cannot speak while muted.\r\n", ch);
		return;
	}

	if (PLR_FLAGGED(ch, PLR_NOSHOUT) && subcmd != SCMD_COMM_NEWBIE) {
		send_to_char(com_msgs[subcmd][0], ch);
		return;
	}
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && subcmd != SCMD_COMM_NEWBIE) {
		send_to_char("The walls seem to absorb your words.\r\n", ch);
		return;
	}
	/* make sure the char is on the channel */
	if (PRF_FLAGGED(ch, channels[subcmd])) {
		send_to_char(com_msgs[subcmd][2], ch);
		return;
	}
	/* skip leading spaces */
	skip_spaces(&argument);

	/* make sure that there is something there to say! */
	if (!*argument) {
		printbuf = get_buffer(1024);
		sprintf(printbuf, "Yes, %s, fine, %s we must, but WHAT???\r\n",
						com_msgs[subcmd][1], com_msgs[subcmd][1]);
		send_to_char(printbuf, ch);
		release_buffer(printbuf);
		return;
	}
	if (subcmd == SCMD_COMM_HOLLER) {
		if (GET_MOVE(ch) < holler_move_cost) {
			send_to_char("You're too exhausted to holler.\r\n", ch);
			return;
		} else
			GET_MOVE(ch) -= holler_move_cost;
	}
	/* make the colors strings */
	strcpy (color_braces, com_msgs[subcmd][3]);
	strcpy (color_text, com_msgs[subcmd][4]);

	/* make the name uppercase for strings */
	strcpy (channel_name, com_msgs[subcmd][1]);
	ALLCAP(channel_name);

	printbuf = get_buffer(MAX_STRING_LENGTH);

	if (*argument == '@' && (subcmd != SCMD_COMM_HOLLER && subcmd != SCMD_COMM_SHOUT)) {
		argument = argument+1;
		skip_spaces(&argument);
		if (!*argument) {
			sprintf(printbuf, "Yes, %s, fine, %s we must, but WHAT???\r\n",
							com_msgs[subcmd][1], com_msgs[subcmd][1]);
			send_to_char(printbuf, ch);
			release_buffer(printbuf);
			return;
		}
		sprintf(printbuf, "%s[%s%s%s] &W%s %s&n", color_braces, color_text, channel_name, color_braces, GET_NAME(ch), argument);
	} else if (*argument == '#' && (subcmd != SCMD_COMM_HOLLER && subcmd != SCMD_COMM_SHOUT))	{
		show_comm_buff(ch, subcmd);
		release_buffer(printbuf);
		return;
	} else {
		if (subcmd != SCMD_COMM_HOLLER && subcmd != SCMD_COMM_SHOUT)
			sprintf(printbuf, "%s[%s%s%s] &W%s&n: %s", color_braces, color_text, channel_name, color_braces, GET_NAME(ch), argument);
		else
			sprintf(printbuf, "%s$n %ss, '%s%s%s'&n", color_braces, com_msgs[subcmd][1], color_text, argument, color_braces);
	}
	/* first, set up strings to be given to the communicator */
	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
	else {
		if (subcmd != SCMD_COMM_HOLLER && subcmd != SCMD_COMM_SHOUT) {
			act(printbuf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
		} else {
			char *printbuf1 = get_buffer(MAX_STRING_LENGTH);
			sprintf(printbuf1, "%sYou %s, '%s%s%s'&n", color_braces, com_msgs[subcmd][1], color_text, argument, color_braces);
			act(printbuf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
			release_buffer(printbuf1);
		}
	}

	/* now send all the strings out */
	for (i = descriptor_list; i; i = i->next) {
		if ((STATE(i) == CON_PLAYING || STATE(i) == CON_COPYOVER) && i != ch->desc && i->character &&
				!PRF_FLAGGED(i->character, channels[subcmd]) &&
				!EDITING(i) &&
				!PLR_FLAGGED(i->character, PLR_OLC) &&
				!ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF)) {

			if (subcmd == SCMD_COMM_SHOUT &&
					((world[IN_ROOM(ch)].zone != world[IN_ROOM(i->character)].zone) ||
					 !AWAKE(i->character)))
				continue;

			act(printbuf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
		}
	}

	/* and add a copy to the global buffer */
	add_to_comm_buff(subcmd, printbuf);

	release_buffer(printbuf);
}


ACMD(do_qcomm)
{
	char *printbuf;
	struct descriptor_data *i;

	if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_QUEST)) {
		send_to_char("You aren't even part of the quest!\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	printbuf = get_buffer(1024);

	if (!*argument) {
		sprintf(printbuf, "%s?  Yes, fine, %s we must, but WHAT??\r\n", CMD_NAME,
						CMD_NAME);
		CAP(printbuf);
		send_to_char(printbuf, ch);
	} else {
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			if (subcmd == SCMD_QSAY)
				sprintf(printbuf, "You quest-say, '%s'", argument);
			else
				strcpy(printbuf, argument);
			act(printbuf, FALSE, ch, 0, argument, TO_CHAR);
		}

		if (subcmd == SCMD_QSAY)
			sprintf(printbuf, "$n quest-says, '%s'", argument);
		else
			strcpy(printbuf, argument);

		for (i = descriptor_list; i; i = i->next)
			if ((STATE(i) == CON_PLAYING || STATE(i) == CON_COPYOVER) && i != ch->desc &&
					PRF_FLAGGED(i->character, PRF_QUEST))
				act(printbuf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
	}
	release_buffer(printbuf);
}


ACMD(do_contact)
{
  struct char_data *contact;
	int bonus = 0, roll;
  
  if (GET_CONTACT(ch)) {
		if (GET_POS(ch) == POS_MEDITATING)
			bonus += 2;
		if ((roll = number(1, 101)) < GET_SKILL(ch, SKILL_MEDITATE)) {
			if (roll >= 90) bonus += 5;
			else if (roll >= 75) bonus += 4;
			else if (roll >= 50) bonus += 3;
			else if (roll >= 25) bonus += 2;
			else bonus += 1;
		}
		if ((number(0, 25) - bonus) <= 18) {
			if ((contact = find_char(get_id_by_name(GET_CONTACT(ch)))) != NULL) {
				if (STATE(contact->desc) == CON_PLAYING || STATE(contact->desc) == CON_COPYOVER) {
					char *printbuf = get_buffer(1024);
					sprintf(printbuf, "%s is trying to contact you.\r\n", GET_NAME(ch));
					send_to_char(printbuf, contact);
					sprintf(printbuf, "You attempt to contact %s.\r\n", GET_NAME(contact));
					send_to_char(printbuf, ch);
					release_buffer(printbuf);
					return;
				}
			}
    }
  } 
  
  send_to_char("Your calls seem to go unanswered.\r\n", ch);
}


ACMD(do_replylock)
{
	struct char_data *tch = character_list;
 
	skip_spaces(&argument);
 
	if (!*argument) {
		if (GET_LAST_TELL(ch) == NOBODY) {
			send_to_char("Who do you want to lock your replies to?\r\n", ch);
			return;
		} else {
 
			while (tch != NULL && GET_IDNUM(tch) != GET_LAST_TELL(ch))
				tch = tch->next;
 
			if (tch == NULL) {
				send_to_char("They are no longer playing.\r\n", ch);
			} else {
				char *printbuf = get_buffer(256);
				GET_REPLYTO(ch) = GET_NAME(tch);
				sprintf(printbuf, "Replies are now locked to %s.\r\n", GET_REPLYTO(ch));
				send_to_char(printbuf, ch);
				if (!SESS_FLAGGED(ch, SESS_REPLYLOCK)) (void)SESS_TOG_CHK(ch, SESS_REPLYLOCK);
				release_buffer(printbuf);
			}
			return;
		}
	} else {
		if (!strcmp(argument, "off")) {
			if (SESS_FLAGGED(ch, SESS_REPLYLOCK)) {
				(void)SESS_TOG_CHK(ch, SESS_REPLYLOCK);
				send_to_char("Replies are no longer locked.\r\n", ch);
			} else {
				send_to_char("You don't have replies locked!\r\n", ch);
			}
		} else {
			if (!(tch = get_player_vis(ch, argument, NULL, FIND_CHAR_WORLD)))
				send_to_char(NOPERSON, ch);
			else {
				if (ch == tch) {
					send_to_char("Wouldn't that be a rather one-sided conversation?\r\n", ch);
				} else {
					char *printbuf = get_buffer(256);
					GET_REPLYTO(ch) = GET_NAME(tch);
					if (!SESS_FLAGGED(ch, SESS_REPLYLOCK)) (void)SESS_TOG_CHK(ch, SESS_REPLYLOCK);        
						sprintf(printbuf, "Replies are now locked to %s.\r\n", GET_REPLYTO(ch));
					send_to_char(printbuf, ch);
					release_buffer(printbuf);
				}
			}
		}
	}
}


void add_tell_to_buffer(struct char_data *ch, const char *tell)
{
	char **ptr = &(GET_TELLS(ch, 0));
	int i = 0;

	while (*ptr && (i < TELLS_SIZE)) {
		ptr++;
		i++;
	}
 
	if (i == TELLS_SIZE) { // tell buffer is full, cycle it.
		for (i = 0; i < TELLS_SIZE - 1; i++)
			strcpy(GET_TELLS(ch, i), GET_TELLS(ch, i + 1));
		ptr = &(GET_TELLS(ch, TELLS_SIZE - 1));
		release_memory(*ptr);
	}

	if (!(*ptr)) {
		*ptr = get_memory(MAX_INPUT_LENGTH);
		strcpy(*ptr, tell);  
	}
	
	if (!SESS_FLAGGED(ch, SESS_HAVETELLS))
		SET_BIT(SESSION_FLAGS(ch), SESS_HAVETELLS);
}


ACMD(do_tells)
{
	int i, j = 0;
	char *input = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, input);
	
	if (SESS_FLAGGED(ch, SESS_HAVETELLS))
		REMOVE_BIT(SESSION_FLAGS(ch), SESS_HAVETELLS);

	if (!*argument) {
		// List all the tells.
		for (i = 0; i < TELLS_SIZE; i++) {
			if (GET_TELLS(ch, i)) {
				send_to_charf(ch, "(%2d): %s\r\n", i+1, GET_TELLS(ch, i));	
				j++;
			}
		}
		if (j == 0)
			send_to_char("You have no buffered tells.\r\n", ch);
		send_to_charf(ch, "Usage: %s {clear}\r\n", CMD_NAME);
	} else if (!strcmp(input, "clear")) {
		for (i = 0; i < TELLS_SIZE; i++)
			if (GET_TELLS(ch, i))
				release_memory(GET_TELLS(ch, i));
		send_to_char("Tell buffer has been cleared.\r\n", ch);
	}

	release_buffer(input);

}
