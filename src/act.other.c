/* 
************************************************************************
*		File: act.other.c                                   Part of CircleMUD *
*	 Usage: Miscellaneous player-level commands                             *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: act.other.c,v 1.81 2004/04/22 16:42:10 cheron Exp $ */

#define	__ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "handler.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "loadrooms.h"
#include "roleplay.h"
#include "tedit.h"

/* extern variables */
extern sh_int mortal_start_room[NUM_STARTROOMS +1];
extern int free_rent;
extern int pt_allowed;
extern int max_filesize;
extern int nameserver_is_slow;
extern int auto_save;
extern int track_through_doors;
extern char *ideas;
extern char *typos;
extern char *bugs;

/* extern procedures */
void list_skills(struct char_data *ch);
void appear(struct char_data *ch);
void write_aliases(struct char_data *ch);
void perform_immort_vis(struct char_data *ch);
SPECIAL(shop_keeper);
ACMD(do_gen_comm);
ACMD(do_reload);
void die(struct char_data *ch, struct char_data *killer);
void send_tip_message(void);
void extract_char_final(struct char_data *ch);
EVENTFUNC(sysintensive_delay);
void add_tell_to_buffer(struct char_data *ch, const char *tell);

/* local functions */
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
int	perform_group(struct char_data *ch, struct char_data *vict);
void print_group(struct char_data *ch);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_wimpy);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_rolldie);
ACMD(do_recall);
ACMD(do_cycletip);
void gen_tog(struct char_data *ch, int subcmd);
ACMD(do_qptransfer);
void bti_add(struct char_data*, char *, const char*);

ACMD(do_quit)
{
	struct descriptor_data *k;
	char *notify;

	if (IS_NPC(ch) || !ch->desc)
		return;

	if (subcmd != SCMD_QUIT && IS_MORTAL(ch))
		send_to_char("You have to type quit--no less, to quit!\r\n", ch);
	else if (GET_POS(ch) == POS_FIGHTING)
		send_to_char("No way!  You're fighting for your life!\r\n", ch);
	else if (GET_POS(ch) < POS_STUNNED) {
		send_to_char("You die before your time...\r\n", ch);
		die(ch, NULL);
	} else {
		int loadroom = GET_ROOM_VNUM(IN_ROOM(ch));

		if (GET_TRAVELS(ch) && GET_TRAVELS(ch)->gout)
			strcpy(buf, GET_TRAVELS(ch)->gout);
		else
			strcpy(buf, travel_defaults[TRAV_GOUT]);
		act(buf, TRUE, ch, 0, 0, TO_ROOM);
		/* tell the people who should be notified that this person has logged out */
		notify = get_buffer(1024);
		for (k = descriptor_list; k; k = k->next) {
			if ((STATE(k) == CON_PLAYING || STATE(k) == CON_COPYOVER) && !EDITING(k) && k->character && 
				k->character != ch && find_notify(k->character, ch)) {
				sprintf(notify, "&C%s has logged off.&n", GET_NAME(ch));
				send_to_charf(k->character, "%s\r\n", notify);
				add_tell_to_buffer(k->character, notify);
			}
		}
		release_buffer(notify);

		extended_mudlog(BRF, SYSL_LOGINS, TRUE, "%s@%s has quit the game.", GET_NAME(ch), GET_HOST(ch));
		send_to_char("Fare thee well, traveller!\r\n", ch);

		/*  We used to check here for duping attempts, but we may as well
		 *  do it right in extract_char(), since there is no check if a
		 *  player rents out and it can leave them in an equally screwy
		 *  situation.
		 */

		if (free_rent)
			crash_datasave(ch, 0, RENT_RENTED);

		extract_char(ch);	/* Char is saved in extract char */

		save_char(ch, loadroom, FALSE);
		write_aliases(ch);
		house_crashsave_objects(find_house_owner(ch));
	}
}


ACMD(do_save)
{
	int loadroom = GET_ROOM_VNUM(IN_ROOM(ch));

	if (IS_NPC(ch) || !ch->desc)
		return;

	/* Only tell the char we're saving if they actually typed "save" */
	if (cmd) {
		if (GET_PLAYER_EVENT(ch, EVENT_SYSTEM) && !IS_IMMORTAL(ch)) {
			send_to_char("&RYou recently performed a system intensive task. Try again later...&n\r\n", ch);
			return;
		} else {
			if (!IS_IMMORTAL(ch)) {
				struct delay_event_obj *deo;
				CREATE(deo, struct delay_event_obj, 1);
				deo->vict = ch;														/* pointer to ch									*/
				deo->type = EVENT_SYSTEM;									/* action type										*/
				deo->time = 1;														/* event time * PULSE_SKILL				*/
				GET_PLAYER_EVENT(ch, EVENT_SYSTEM) = event_create(sysintensive_delay, deo, 15 RL_SEC);
			}
		}
		sprintf(buf, "Saving %s and aliases.\r\n", GET_NAME(ch));
		send_to_char(buf, ch);
	}

	write_aliases(ch);
	save_char(ch, loadroom, FALSE);
	crash_datasave(ch, 0, RENT_CRASH);
	house_crashsave_objects(find_house_owner(ch));
}


/* generic function for commands which are normally overridden by
	 special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
	send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}


ACMD(do_practice)
{
	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (*arg)
		send_to_char("You can only practice skills at the practice grounds or in your guild.\r\n", ch);
	else
		send_to_char("Usage: practice <skillname>|<spellname>\r\n", ch);
}


ACMD(do_visible)
{
	if (IS_IMMORTAL(ch)) {
		perform_immort_vis(ch);
		return;
	}

	if AFF_FLAGGED(ch, AFF_INVISIBLE) {
		appear(ch);
		send_to_char("You break the spell of invisibility.\r\n", ch);
	} else
		send_to_char("You are already visible.\r\n", ch);
}


ACMD(do_title)
{
	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (IS_NPC(ch))
		send_to_char("Your title is fine... go away.\r\n", ch);
	else if (PLR_FLAGGED(ch, PLR_NOTITLE))
		send_to_char("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
	else if (strstr(argument, "(") || strstr(argument, ")"))
		send_to_char("Titles can't contain the ( or ) characters.\r\n", ch);
	else if (strlen(argument) > MAX_TITLE_LENGTH) {
		sprintf(buf, "Sorry, titles can't be longer than %d characters.\r\n",
			MAX_TITLE_LENGTH);
		send_to_char(buf, ch);
	} else {
		set_title(ch, argument);
		sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
		send_to_char(buf, ch);
	}
}


int	perform_group(struct char_data *ch, struct char_data *vict)
{
	if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
		return (0);

	SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
	if (ch != vict)
		act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
	act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
	act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
	return (1);
}


void print_group(struct char_data *ch)
{
	struct char_data *k;
	struct follow_type *f;

	if (!AFF_FLAGGED(ch, AFF_GROUP))
		send_to_char("But you are not the member of a group!\r\n", ch);
	else {
		send_to_char("Your group consists of:\r\n", ch);

		k = (ch->master ? ch->master : ch);

		if (AFF_FLAGGED(k, AFF_GROUP)) {
			sprintf(buf, "     [%3dH %3dM %3dV] $N (Head of group)",
							GET_HIT(k), GET_MANA(k), GET_MOVE(k));
			act(buf, FALSE, ch, 0, k, TO_CHAR);
		}

		for (f = k->followers; f; f = f->next) {
			if (!AFF_FLAGGED(f->follower, AFF_GROUP))
				continue;

			sprintf(buf, "     [%3dH %3dM %3dV] $N", GET_HIT(f->follower),
							GET_MANA(f->follower), GET_MOVE(f->follower));
			act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
		}
	}
}


ACMD(do_group)
{
	struct char_data *vict;
	struct follow_type *f;
	int found;

	one_argument(argument, buf);

	if (!*buf) {
		print_group(ch);
		return;
	}

	if (ch->master) {
		act("You can not enroll group members without being head of a group.",
				FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!str_cmp(buf, "all")) {
		perform_group(ch, ch);
		for (found = 0, f = ch->followers; f; f = f->next)
			found += perform_group(ch, f->follower);
		if (!found)
			send_to_char("Everyone following you is already in your group.\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM, 0)))
		send_to_char(NOPERSON, ch);
	else if ((vict->master != ch) && (vict != ch))
		act("&W$N &nmust follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
	else {
		if (!AFF_FLAGGED(vict, AFF_GROUP))
			perform_group(ch, vict);
		else {
			if (ch != vict)
				act("&W$N&n is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
			act("You have been kicked out of &W$n's&n group!", FALSE, ch, 0, vict, TO_VICT);
			act("&W$N&n has been kicked out of &W$n's&n group!", FALSE, ch, 0, vict, TO_NOTVICT);
			REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
		}
	}
}


ACMD(do_ungroup)
{
	struct follow_type *f, *next_fol;
	struct char_data *tch;

	one_argument(argument, buf);

	if (!*buf) {
		if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
			send_to_char("But you lead no group!\r\n", ch);
			return;
		}
		sprintf(buf2, "&W%s&n has disbanded the group.\r\n", GET_NAME(ch));
		for (f = ch->followers; f; f = next_fol) {
			next_fol = f->next;
			if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
				REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
				send_to_char(buf2, f->follower);
				if (!AFF_FLAGGED(f->follower, AFF_CHARM))
					stop_follower(f->follower);
			}
		}

		REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
		send_to_char("You disband the group.\r\n", ch);
		return;
	}
	if (!(tch = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM, 0))) {
		send_to_char("There is no such person!\r\n", ch);
		return;
	}
	if (tch->master != ch) {
		send_to_char("That person is not following you!\r\n", ch);
		return;
	}

	if (!AFF_FLAGGED(tch, AFF_GROUP)) {
		send_to_char("That person isn't in your group.\r\n", ch);
		return;
	}

	REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

	act("&W$N&n is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
	act("You have been kicked out of &W$n's&n group!", FALSE, ch, 0, tch, TO_VICT);
	act("&W$N&n has been kicked out of &W$n's&n group!", FALSE, ch, 0, tch, TO_NOTVICT);

	if (!AFF_FLAGGED(tch, AFF_CHARM))
		stop_follower(tch);
}


ACMD(do_report)
{
	struct char_data *k;
	struct follow_type *f;

	if (!AFF_FLAGGED(ch, AFF_GROUP)) {
		send_to_char("But you are not a member of any group!\r\n", ch);
		return;
	}
	sprintf(buf, "&W%s&n reports: &W%d&n/&Y%d&nH, &W%d&n/&Y%d&nM, &W%d&n/&Y%d&nV\r\n",
					GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
					GET_MANA(ch), GET_MAX_MANA(ch),
					GET_MOVE(ch), GET_MAX_MOVE(ch));

	CAP(buf);

	k = (ch->master ? ch->master : ch);

	for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
			send_to_char(buf, f->follower);
	if (k != ch)
		send_to_char(buf, k);
	send_to_char("You report to the group.\r\n", ch);
}


ACMD(do_split)
{
	int amount, num, share, rest;
	struct char_data *k;
	struct follow_type *f;

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (is_number(buf)) {
		amount = atoi(buf);
		if (amount <= 0) {
			send_to_char("Sorry, you can't do that.\r\n", ch);
			return;
		}
		if (amount > GET_GOLD(ch)) {
			send_to_char("You don't seem to have that much gold to split.\r\n", ch);
			return;
		}
		k = (ch->master ? ch->master : ch);

		if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
			num = 1;
		else
			num = 0;

		for (f = k->followers; f; f = f->next)
			if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
					(!IS_NPC(f->follower)) &&
					(IN_ROOM(f->follower) == IN_ROOM(ch)))
				num++;

		if (num && AFF_FLAGGED(ch, AFF_GROUP)) {
			share = amount / num;
			rest = amount % num;
		} else {
			send_to_char("With whom do you wish to share your gold?\r\n", ch);
			return;
		}

		GET_GOLD(ch) -= share * (num - 1);

		sprintf(buf, "&W%s&n splits &W%d&n coins; you receive &Y%d&n.\r\n", GET_NAME(ch),
						amount, share);
		if (rest) {
			sprintf(buf + strlen(buf), "&Y%d&n coin%s %s not splitable, so &W%s&n "
							"keeps the money.\r\n", rest,
							(rest == 1) ? "" : "s",
							(rest == 1) ? "was" : "were",
							GET_NAME(ch));
		}
		if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch))
				&& !(IS_NPC(k)) && k != ch) {
			GET_GOLD(k) += share;
			send_to_char(buf, k);
		}
		for (f = k->followers; f; f = f->next) {
			if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
					(!IS_NPC(f->follower)) &&
					(IN_ROOM(f->follower) == IN_ROOM(ch)) &&
					f->follower != ch) {
				GET_GOLD(f->follower) += share;
				send_to_char(buf, f->follower);
			}
		}
		sprintf(buf, "You split &Y%d&n coins among &W%d&n members -- &W%d&n coins each.\r\n",
						amount, num, share);
		if (rest) {
			sprintf(buf + strlen(buf), "&Y%d&n coin%s %s not splitable, so you keep "
																 "the money.\r\n", rest,
																 (rest == 1) ? "" : "s",
																 (rest == 1) ? "was" : "were");
			GET_GOLD(ch) += rest;
		}
		send_to_char(buf, ch);
	} else {
		send_to_char("How many coins do you wish to split with your group?\r\n", ch);
		return;
	}
}


ACMD(do_use)
{
	struct obj_data *mag_item;
	int i, spellnum;

	half_chop(argument, arg, buf);
	if (!*arg) {
		sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
		send_to_char(buf2, ch);
		return;
	}
	mag_item = GET_EQ(ch, WEAR_HOLD);

	if (!mag_item || !isname(arg, mag_item->name)) {
		switch (subcmd) {
		case SCMD_RECITE:
		case SCMD_QUAFF:
			if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
				sprintf(buf2, "You don't seem to have %s %s.\r\n", AN(arg), arg);
				send_to_char(buf2, ch);
				return;
			}
			break;
		case SCMD_STUDY:
		case SCMD_USE:
			sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
			send_to_char(buf2, ch);
			return;
		default:
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unknown subcmd %d passed to do_use.", subcmd);
			return;
		}
	}
	switch (subcmd) {
	case SCMD_QUAFF:
		if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
			send_to_char("You can only quaff potions.\r\n", ch);
			return;
		}
		break;
	case SCMD_RECITE:
		if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
			send_to_char("You can only recite scrolls.\r\n", ch);
			return;
		}
		break;
	case SCMD_STUDY:
		if (GET_OBJ_TYPE(mag_item) != ITEM_SPELLBOOK) {
			send_to_char("You can only study spell books.\r\n", ch);
			return;
		}
		for (i = 1; i <= 3; i++) {
			spellnum = GET_OBJ_VAL(mag_item, i);
			if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE) continue;
		}
		break;
	case SCMD_USE:
		if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
				(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
			send_to_char("You can't seem to figure out how to use it.\r\n", ch);
			return;
		}
		break;
	}

	mag_objectmagic(ch, mag_item, buf);
}


ACMD(do_wimpy)
{
	int wimp_lev;

	/* 'wimp_level' is a player_special. -gg 2/25/98 */
	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		if (GET_WIMP_LEV(ch)) {
			sprintf(buf, "Your current wimp level is %d hit points.\r\n",
							GET_WIMP_LEV(ch));
			send_to_char(buf, ch);
			return;
		} else {
			send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
			return;
		}
	}
	if (isdigit(*arg)) {
		if ((wimp_lev = atoi(arg)) != 0) {
			if (wimp_lev < 0)
				send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
			else if (wimp_lev > GET_MAX_HIT(ch))
				send_to_char("That doesn't make much sense, now does it?\r\n", ch);
			else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
				send_to_char("You can't set your wimp level above half your hit points.\r\n", ch);
			else {
				sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n",
								wimp_lev);
				send_to_char(buf, ch);
				GET_WIMP_LEV(ch) = wimp_lev;
			}
		} else {
			send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
			GET_WIMP_LEV(ch) = 0;
		}
	} else
		send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);
}


ACMD(do_display)
{
	size_t i;

	if (IS_NPC(ch)) {
		send_to_char("Monsters don't need displays.  Go away.\r\n", ch);
		return;
	}
	skip_spaces(&argument);

	if (!*argument) {
		send_to_char("Usage: prompt { { H | M | V } | all | none | on | off }\r\n", ch);
		return;
	}
	if (!str_cmp(argument, "all"))
		SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_HASPROMPT);
	else if (!str_cmp(argument, "none"))
		REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
	else if (!str_cmp(argument, "off"))
		REMOVE_BIT(PRF_FLAGS(ch), PRF_HASPROMPT);
	else if (!str_cmp(argument, "on"))
		SET_BIT(PRF_FLAGS(ch), PRF_HASPROMPT);
	else {
		REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);

		for (i = 0; i < strlen(argument); i++) {
			switch (LOWER(argument[i])) {
			case 'h':
				SET_BIT(PRF_FLAGS(ch), PRF_DISPHP);
				break;
			case 'm':
				SET_BIT(PRF_FLAGS(ch), PRF_DISPMANA);
				break;
			case 'v':
				SET_BIT(PRF_FLAGS(ch), PRF_DISPMOVE);
				break;
			default:
				send_to_char("Usage: prompt { { H | M | V } | all | none | on | off }\r\n", ch);
				return;
			}
		}
	}

	send_to_char(OK, ch);
}

ACMD(do_gen_write)
{
	char *header;
	const char *filename;
	char *list;
	
	if (IS_NPC(ch)) {
		send_to_char("You're a monster.  Go away.\r\n", ch);
		return;
	}

	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument) {
		send_to_char("Please supply an argument.\r\n", ch);
		return;
	}

	header = get_buffer(256);
	switch (subcmd) {
	case SCMD_BUG:
		filename = BUG_FILE;
		list = bugs;
	  	sprintf(header, "&Y*&n%-8s:", GET_NAME(ch));
		break;
	case SCMD_TYPO:
		filename = TYPO_FILE;
	  	sprintf(header, "&Y*&n%-8s [%5d]:", 
			GET_NAME(ch), 
			GET_ROOM_VNUM(IN_ROOM(ch)));
		list = typos;
		break;
	case SCMD_IDEA:
		filename = IDEA_FILE;
	  	sprintf(header, "&Y*&n%-8s:", GET_NAME(ch));
		list = ideas;
		break;
	default:
		release_buffer(header);
		extended_mudlog(NRM, SYSL_GENERAL, TRUE, "%s [%i] %s",
				"Unhandled Subcommand", subcmd,
				"in function do_gen_write");
		return;
	}

	extended_mudlog(NRM, SYSL_GENERAL, TRUE, "%s %s: %s", 
			GET_NAME(ch), CMD_NAME, argument);

	if(!list) list = str_dup(" ");
	int bufsize=strlen(header)+strlen(argument)+ strlen(list) + 4;
	char *new = get_buffer(bufsize);
	sprintf(new, "%s%s %s\r\n",list,header,argument);
	bti_add(ch, new, filename);
	release_buffer(header);
	release_buffer(new);
	
	send_to_char("Okay.  Thanks!\r\n", ch);
	return;
} 

#define	TOG_OFF 0
#define	TOG_ON  1

void gen_tog(struct char_data *ch, int subcmd)
{
	bitvector_t result;

	const char *tog_messages[][2] = {
		{"You are now safe from summoning by other players.\r\n",
		"You may now be summoned by other players.\r\n"},
		{"Nohassle disabled.\r\n",
		"Nohassle enabled.\r\n"},
		{"Brief mode off.\r\n",
		"Brief mode on.\r\n"},
		{"Compact mode off.\r\n",
		"Compact mode on.\r\n"},
		{"You can now hear tells.\r\n",
		"You are now deaf to tells.\r\n"},
		{"You can now hear shouts.\r\n",
		"You are now deaf to shouts.\r\n"},
		{"You can now hear the OOC-channel.\r\n",
		"You can no longer hear the OOC-channel.\r\n"},
		{"You can now hear the Wiz-channel.\r\n",
		"You are now deaf to the Wiz-channel.\r\n"},
		{"You are no longer part of the Quest.\r\n",
		"Okay, you are part of the Quest!\r\n"},
		{"You will no longer see the room flags.\r\n",
		"You will now see the room flags.\r\n"},
		{"You will now have your communication repeated.\r\n",
		"You will no longer have your communication repeated.\r\n"},
		{"HolyLight mode off.\r\n",
		"HolyLight mode on.\r\n"},
		{"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
		"Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
		{"Autoexits disabled.\r\n",
		"Autoexits enabled.\r\n"},
		{"Will no longer track through doors.\r\n",
		"Will now track through doors.\r\n"},
		{"You will no longer Auto-Assist.\r\n",
		"You will now Auto-Assist.\r\n"},
		{"AutoSaccing disabled.\r\n",
		"AutoSaccing enabled.\r\n"},
		{"AutoSplit disabled.\r\n",
		"AutoSplit enabled.\r\n"},
		{"AutoLooting disabled.\r\n",
		"AutoLooting enabled.\r\n"},
		{"Autogold disabled.\r\n",
		"Autogold enabled.\r\n"},
		{"Meterbar disabled.\r\n",
		"Meterbar enabled.\r\n"},
		{"You can now hear the newbie channel.\r\n",
		"You can no longer hear the newbie channel.\r\n"},
		{"!ROLEPLAY OFF!",
		"!ROLEPLAY ON!"},
		{"You can no longer see the tip messages.\r\n",
		"You can now see the tip messages.\r\n"},
		{"VNUM display turned off.\r\n",
		"VNUM display turned on.\r\n"},
		{"You will no longer receive email.\r\n",
		"You will now receive email newsletters.\r\n"},
		{"You will now hear the sing channel.\r\n",
		"You will no longer hear the sing channel.\r\n"},
		{"You will now hear the obscene channel.\r\n",
		"You will no longer hear the obscene channel.\r\n"},
		{"No longer displaying skill gain messages.\r\n",
		"Skill gain messages will be displayed.\r\n"},
		{"No longer displaying timestamps.\r\n",
		"Timestamps will now be displayed.\r\n"},
		{"You will now see skill values with bonuses applied.\r\n",
		"Real skill values will now be displayed.\r\n"},
		{"No longer displaying the pose ID.\r\n",
		"Pose ID will now be displayed.\r\n"}
	};


	if (IS_NPC(ch))
		return;

	switch (subcmd) {
	case SCMD_NOSUMMON:
		result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);     /* 0 */
		break;
	case SCMD_NOHASSLE:
		result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
		break;
	case SCMD_BRIEF:
		result = PRF_TOG_CHK(ch, PRF_BRIEF);
		break;
	case SCMD_COMPACT:
		result = PRF_TOG_CHK(ch, PRF_COMPACT);
		break;
	case SCMD_NOTELL:
		result = PRF_TOG_CHK(ch, PRF_NOTELL);
		break;
	case SCMD_DEAF:
		result = PRF_TOG_CHK(ch, PRF_DEAF);
		break;
	case SCMD_NOSOC:
		result = PRF_TOG_CHK(ch, PRF_NOSOC);
		break;
	case SCMD_NOWIZ:
		result = PRF_TOG_CHK(ch, PRF_NOWIZ);
		break;
	case SCMD_QUEST:
		result = PRF_TOG_CHK(ch, PRF_QUEST);
		break;
	case SCMD_ROOMFLAGS:
		result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
		break;
	case SCMD_NOREPEAT:															/* 10 */
		result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
		break;
	case SCMD_HOLYLIGHT:
		result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
		break;
	case SCMD_SLOWNS:
		result = (nameserver_is_slow = !nameserver_is_slow);
		break;
	case SCMD_AUTOEXIT:
		result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
		break;
	case SCMD_TRACK:
		result = (track_through_doors = !track_through_doors);
		break;
	case SCMD_AUTOASSIST:
		result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
		break;
	case SCMD_AUTOSAC:
		result = PRF_TOG_CHK(ch, PRF_AUTOSAC);
		break;
	case SCMD_AUTOSPLIT:
		result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
		break;
	case SCMD_AUTOLOOT:
		result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
		break;
	case SCMD_AUTOGOLD:
		result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
		break;
	case SCMD_METERBAR:														/* 20 */
		result = PRF_TOG_CHK(ch, PRF_METERBAR);
		break;
	case SCMD_NONEWBIE:
		result = PRF_TOG_CHK(ch, PRF_NONEWBIE);
		break;
	case SCMD_IC:
		tog_rp(ch);
		return;
	case SCMD_NOTIPCHAN:
		result = PRF_TOG_CHK(ch, PRF_TIPCHANNEL);
		break;
	case SCMD_SHOWVNUMS:
		result = PRF_TOG_CHK(ch, PRF_SHOWVNUMS);
		break;
	case SCMD_EMAIL:
		result = PRF_TOG_CHK(ch, PRF_EMAIL);
		break;
	case SCMD_NOSING:
		result = PRF_TOG_CHK(ch, PRF_NOSING);
		break;
	case SCMD_NOOBSCENE:
		result = PRF_TOG_CHK(ch, PRF_NOOBSCENE);
		break;
	case SCMD_SKILLGAINS:
		result = PRF_TOG_CHK(ch, PRF_SKILLGAINS);
		break;
	case SCMD_TIMESTAMPS:
		result = PRF_TOG_CHK(ch, PRF_TIMESTAMPS);
		break;
	case SCMD_REALSKILLS:													/* 30 */
		result = PRF_TOG_CHK(ch, PRF_REALSKILLS);
		break;
  case SCMD_POSEID:
		result = PRF_TOG_CHK(ch, PRF_POSEID);
		break;
	default:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unknown subcmd %d in gen_tog.", subcmd);
		return;
	}

	if (result)
		send_to_char(tog_messages[subcmd][TOG_ON], ch);
	else
		send_to_char(tog_messages[subcmd][TOG_OFF], ch);

	return;
}


ACMD(do_become)
{
	if (IS_NPC(ch) || !ch->desc)
	return;

	if (GET_POS(ch) == POS_FIGHTING)
		send_to_char("No way!  You're fighting for your life!\r\n", ch);
	else if (GET_POS(ch) < POS_STUNNED) {
		send_to_char("You die before your time...\r\n", ch);
		die(ch, NULL);
	} else {
		int loadroom = GET_ROOM_VNUM(IN_ROOM(ch));

		if (GET_TRAVELS(ch) && GET_TRAVELS(ch)->gout)
			strcpy(buf, GET_TRAVELS(ch)->gout);
		else
			strcpy(buf, travel_defaults[TRAV_GOUT]);
		act(buf, TRUE, ch, 0, 0, TO_ROOM);


		if (free_rent)
			crash_datasave(ch, 0, RENT_RENTED);

		save_char(ch, loadroom, FALSE);
		write_aliases(ch);
		house_crashsave_objects(find_house_owner(ch));
		extended_mudlog(BRF, SYSL_LOGINS, TRUE, "%s@%s is becoming...", GET_NAME(ch), GET_HOST(ch));
		send_to_char("Whom do you wish to become?  ", ch);
		STATE(ch->desc) = CON_GET_BECOME;
		extract_char_final(ch);
	}
}


ACMD(do_recall)
{
	room_rnum to_room;
	int who, troom;
	who = number(1,5);

	/*
	 * You cannot move around and do skills at the same time.
	 * Torgny Bjers, 2002-06-13.
	 */
	if (GET_PLAYER_EVENT(ch, EVENT_SKILL) || GET_PLAYER_EVENT(ch, EVENT_SPELL)) {
		send_to_char("You are occupied with a task, and cannot move.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	troom = atoi(arg);

	if (!strncmp(arg, "start", 5))
		to_room = real_room(mortal_start_room[start_zone_index]);
	else if (!strncmp(arg, "board", 5) || !strncmp(arg, "ooc", 3))
		to_room = real_room(mortal_start_room[ooc_lounge_index]);
	else
		to_room = real_room(mortal_start_room[ooc_lounge_index]);

	if (to_room == NOWHERE) {
		send_to_char("You cannot recall there right now.\r\n" ,ch);
		return;
	}

	if (!IS_SET(PLR_FLAGS(ch), PLR_NEWBIE)) {
		send_to_char("You can only recall during your first 24 game hours.\r\n", ch);
		return;
	}

	if (!IS_OOC(ch)) {
		send_to_char("You can only recall when you are OOC.\r\n", ch);
		return;
	}

	if (ZONE_FLAGGED(IN_ZONE(ch), ZONE_NORECALL) && IS_MORTAL(ch)) {
		send_to_char("\r\n&REldritch wizardry obstructs thee from using the holy Recall...&n\r\n"
			"  Perhaps it is better if you just remain where you are.\r\n",ch);
		return;
	}

	if (IS_IC(ch) || FIGHTING(ch)) {
		send_to_char("You cannot recall right now.\r\n" ,ch);
		return;
	}

	if (GET_POS(ch) < POS_RESTING) {
		send_to_char("You cannot recall right now.\r\n" ,ch);
		return;
	}

	if (IN_ROOM(ch) == to_room) {
		send_to_char("But you are already there!\r\n", ch);
		return;
	}

	switch(who) {
	 case 1:
		send_to_char("\r\n&MShameriseth peers through the Mists of Time and Space...&n\r\n",ch);
		send_to_char("  &c'&CYour prayer has been heard, mortal.&c'&n\r\n\r\n",ch);
		break;
	 case 2:
		send_to_char("\r\n&MAetheon quickly snags you up, mumbling through his teeth,&m\r\n",ch);
		send_to_char("  &c'&CAs if I didn't have enough to do building the place...&c'&n\r\n\r\n",ch);
		break;
	 case 3:
		send_to_char("\r\n&MBalor grabs your torso with a clawed fist,\r\n",ch);
		send_to_char("  &c'&CCare to join my ranks, mortal? All I have to do is drop you a little sooner.&c'&n\r\n\r\n",ch);
		break;
	 case 4:
		send_to_char("\r\n&MAramil dashes down from Heaven and grabs you in his arms...\r\n",ch);
		send_to_char("  &c'&CThe battle is over now.&c'&n\r\n\r\n",ch);
		break;
	 case 5:
		send_to_char("\r\n&MCheron swoops down from Heaven and carries you to safety...\r\n", ch);
		send_to_char("  &c'&CRest now, young one.&c'&n\r\n\r\n", ch);
		break;
	}

	act("&M$n disappears in a puff of smoke!&n", TRUE, ch, 0, 0, TO_ROOM);
	char_from_room(ch);
	char_to_room(ch, to_room);
	act("&M$n appears in the middle of the room.&n", TRUE, ch, 0, 0, TO_ROOM);
	look_at_room(ch, 0);
	entry_memory_mtrigger(ch);
	greet_mtrigger(ch, -1);
	greet_memory_mtrigger(ch);
}


ACMD(do_rolldie)
{
	int die1, die2, result;

	two_arguments(argument, buf1, buf2);

	if (!*buf1 || !*buf2) {
		send_to_char("Usage: rolldie <number> <size>\r\n", ch);
		return;
	}

	die1=atoi(buf1);
	die2=atoi(buf2);

	result = number(die1, (die1*die2));
	sprintf(buf, "You roll %dd%d and the result is %d.\r\n", die1, die2, result);
	send_to_char(buf, ch);
}


ACMD(do_cycletip)
{
	if (IS_GRGOD(ch))
		send_tip_message();
	else
		send_to_char("You are not godly enough to cycle tip messages.\r\n", ch);
}


/* do_colorpref written by Zach Ibach (Zaquenin)
 * Many thanks, Zach!
 */
ACMD(do_colorpref)
{
	char arg1[MAX_INPUT_LENGTH];
	char *code;

	code = one_arg_case_sen(argument, arg1);

	if (!*arg1) {
		send_to_char("Please specify the command whose color you wish to change.\r\n", ch);
		return;
	}

	skip_spaces(&code);
	if (!*code) {
		send_to_char("Please specify the color code you wish.\r\n",ch);
		return;
	}

	switch(*code) {
		case 'c': case 'C':
		case 'r': case 'R':
		case 'g': case 'G':
		case 'm': case 'M':
		case 'b': case 'B':
		case 'y': case 'Y':
		case 'w': case 'W':
		case 'n': case 'D':
			break;
		default:
			send_to_char("You must select a proper color code.\r\n",ch);
			return;
	}

	if (!strcasecmp(arg1,"all")) {
		GET_COLORPREF_EMOTE(ch) = *code;
		GET_COLORPREF_OSAY(ch) = *code;
		GET_COLORPREF_SAY(ch) = *code;
		GET_COLORPREF_POSE(ch) = *code;
		GET_COLORPREF_ECHO(ch) = *code;
	} else if(!strcasecmp(arg1,"osay"))
		GET_COLORPREF_OSAY(ch) = *code;
	else if(is_abbrev(arg1,"emote"))
		GET_COLORPREF_EMOTE(ch) = *code;
	else if(!strcasecmp(arg1,"pose"))
		GET_COLORPREF_POSE(ch) = *code;
	else if(!strcasecmp(arg1,"say"))
		GET_COLORPREF_SAY(ch) = *code;
	else if(!strcasecmp(arg1, "echo"))
		GET_COLORPREF_ECHO(ch) = *code;
	else {
		send_to_char("You must specify a command to change.\r\n",ch);
		send_to_char("Say, osay, emote, pose, or all.\r\n",ch);
		return;
	}
	sprintf(buf, "Your color preference for %s has been set to %c.\r\n", arg1, *code);
	send_to_char(buf,ch);
}


ACMD(do_lines)
{
	int lines;
	char *buf;

	if (IS_NPC(ch)) {
		send_to_char("Mobs do not need page lengths. Scram!\r\n", ch);
		return;
	}

	one_argument(argument, buf1);

	if (!*buf1) {
		buf = get_buffer(256);
		sprintf(buf,
			"Current page length: &W%d&n.\r\n"
			"Usage: lines <number>\r\n", GET_PAGE_LENGTH(ch));
		send_to_char(buf, ch);
		release_buffer(buf);
		return;
	}

	lines = atoi(buf1);

	if (lines < 10 || lines > 200) {
		send_to_char("You have to supply a number between 10 and 200.\r\n", ch);
		return;
	}

	GET_PAGE_LENGTH(ch) = lines;

	buf = get_buffer(256);
	sprintf(buf, "Page length now set to &W%d&n.\r\n", lines);
	send_to_char(buf, ch);
	release_buffer(buf);

}


ACMD(do_test_parse)
{
	struct wild_spell_info w;

	sprintf(buf,"Argument: %s\r\n",argument);
	send_to_char(buf,ch);
	parse_wild_spell(argument,&w);
	sprintf(buf,"Technique: %d\r\n",w.tech);
	send_to_char(buf,ch);
	sprintf(buf,"Form: %d\r\n",w.form);
	send_to_char(buf,ch);
	sprintf(buf,"Range: %d\r\n",w.range);
	send_to_char(buf,ch);
	sprintf(buf, "Duration: %d\r\n",w.duration);
	send_to_char(buf,ch);
	sprintf(buf, "Target: %d\r\n",w.target_group);
	send_to_char(buf,ch);
	sprintf(buf, "Seed: %d\r\n",w.seed);
	send_to_char(buf,ch);

}


ACMD(do_away)
{
	char *msg = get_buffer(256), *awaymsg;
	bitvector_t mode;
	const char *modes[][2] = {
		{	"AFK",	"keyboard"	},
		{	"AFW",	"window"		}
	};

	/* Switched immortals can be forgetful. */
	if (IS_NPC(ch)) {
		sprintf(msg, "Mobs don't have %ss, go away.\r\n", modes[subcmd][1]);
		send_to_char(msg, ch);
		release_buffer(msg);
		return;
	}

	skip_spaces(&argument);

	switch (subcmd) {
		case SCMD_AWAY_AFW:
			/* Cannot be AFK and AFW at the same time: */
			if (SESS_FLAGGED(ch, SESS_AFK)) {
				send_to_char("You are already AFK!\r\n", ch);
				release_buffer(msg);
				return;
			}
			mode = SESS_AFW;
			break;
		default:
			/* Cannot be AFW and AFK at the same time: */
			if (SESS_FLAGGED(ch, SESS_AFW)) {
				send_to_char("You are already AFW!\r\n", ch);
				release_buffer(msg);
				return;
			}
			mode = SESS_AFK;
			break;
	}

	awaymsg = get_buffer(MAX_INPUT_LENGTH);

	if (!*argument)
		sprintf(awaymsg, "%s", AWAY_DEFAULT);
	else
		sprintf(awaymsg, "%s", argument);

	if (strlen(awaymsg) > MAX_ALIAS_LENGTH) {
		sprintf(msg, "%s message too long, using default.\r\n", modes[subcmd][0]);
		send_to_char(msg, ch);
		sprintf(awaymsg, "%s", AWAY_DEFAULT);
	}

	if (GET_AWAY(ch))
		free(GET_AWAY(ch));
	GET_AWAY(ch) = str_dup(awaymsg);

	/* Toggle the flag, and make sure we get the right message */
	if (SESS_FLAGGED(ch, mode)) {
		sprintf(msg, "$n returns from %s!", modes[subcmd][0]);
		act(msg, FALSE, ch, 0, 0, TO_ROOM);
		sprintf(msg, "You return from %s!", modes[subcmd][0]);
		act(msg, FALSE, ch, 0, 0, TO_CHAR);
		REMOVE_BIT(SESSION_FLAGS(ch), mode);
	} else {
		sprintf(msg, "$n has just gone %s: %s", modes[subcmd][0], GET_AWAY(ch));
		act(msg, FALSE, ch, 0, 0, TO_ROOM);
		sprintf(msg, "You go %s: %s", modes[subcmd][0], GET_AWAY(ch));
		act(msg, FALSE, ch, 0, 0, TO_CHAR);
		SET_BIT(SESSION_FLAGS(ch), mode);
	}

	release_buffer(msg);
	release_buffer(awaymsg);

}


#define RPXP_RATE			200

ACMD(do_rpconvert)
{
	char *msg = get_buffer(256);
	int rpxp, qp;

	/* Switched immortals can be forgetful. */
	if (IS_NPC(ch)) {
		send_to_char("Mobs don't have Quest Points, and they certainly do not role play, go away.\r\n", ch);
		return;
	}

	if (!*argument) {
		sprintf(msg, "The current conversion rate is set at %d RPxp per Qp.\r\nUsage: rpconvert <quest points>\r\n", RPXP_RATE);
		send_to_char(msg, ch);
		release_buffer(msg);
		return;
	}

	if (GET_RPXP(ch) < RPXP_RATE) {
		send_to_char("You do not have enough RPxp to convert to Quest Points.\r\n", ch);
		release_buffer(msg);
		return;
	}

	qp = atoi(argument);
	rpxp = (int)(qp * RPXP_RATE);

	if (qp <= 0) {
		send_to_char("You cannot convert a negative amount of points, nice try.\r\n", ch);
		release_buffer(msg);
		return;
	}

	if (rpxp > GET_RPXP(ch)) {
		sprintf(msg, "You do not have enough RPxp to convert to %d Qp.\r\n", qp);
		send_to_char(msg, ch);
		release_buffer(msg);
		return;
	}

	GET_RPXP(ch) -= rpxp;
	GET_QP(ch) += qp;

	sprintf(msg, "Converted %d RPxp to %d Qp.\r\n", rpxp, qp);
	send_to_char(msg, ch);

	release_buffer(msg);

}


ACMD(do_qptransfer)
{
	struct char_data *vict;
	char *victbuf, *amountbuf;
	int amount;

	if (!(*argument)) {
		send_to_charf(ch, "Usage: qptransfer <amount> <player>\r\n");
		return;
	}
	
	amountbuf = get_buffer(MAX_INPUT_LENGTH);
	victbuf = get_buffer(MAX_INPUT_LENGTH);
	two_arguments(argument, amountbuf, victbuf);

	if (!(amount=atoi(amountbuf))|| !(*victbuf)) {
		send_to_charf(ch, "Usage: qptransfer <amount> <player>\r\n");
		release_buffer(amountbuf);
		release_buffer(victbuf);
		return;
	}

	/* Make sure amount is > 0.  no stealing qp! */
	if (amount <= 0) {
		send_to_charf(ch, "Amount must be greater than 0.\r\n");
		release_buffer(amountbuf);
		release_buffer(victbuf);
		return;
	}

	/* Okay, our amount is set, let's make sure ch has enough */
	if (GET_QP(ch) < amount) {
		send_to_charf(ch, "You don't have that many QPs!\r\n");
		release_buffer(amountbuf);
		release_buffer(victbuf);
		return;
	}

	/* ch has enough, let's find the recipient, who has to be online. */
	if (!(vict = get_char_vis(ch, victbuf, NULL, FIND_CHAR_WORLD, 1))) {
		send_to_charf(ch, NOPERSON);
		release_buffer(amountbuf);
		release_buffer(victbuf);
		return;
	}

	/* we've got a vict, we've got an amount, let's transfer some qp */
	GET_QP(ch) -= amount;
	GET_QP(vict) += amount;
	send_to_charf(ch, "You transfer %d qp to %s.\r\n", amount, GET_NAME(vict));
	send_to_charf(vict, "%s transfers %d qp to you.\r\n", GET_NAME(ch), amount);
	extended_mudlog(NRM, SYSL_QUESTING, TRUE, "%s transferred %d quest point%s to %s.", 
		GET_NAME(ch), amount, (amount != 1 ? "s" : ""), GET_NAME(vict));
	save_char(ch, NOWHERE, FALSE);
	save_char(vict, NOWHERE, FALSE);
	release_buffer(amountbuf);
	release_buffer(victbuf);
}

void bti_add(struct char_data *ch, char *str, const char *filename)
{
	char *name=NULL;
	char *description=NULL;
	char *replace = "REPLACE INTO %s ("
		"name, "
		"text) "
		"VALUES ('%s', '%s');";
	SQL_MALLOC(filename, name);
	SQL_ESC(filename, name);
	SQL_MALLOC(str, description);
	SQL_ESC(str, description);
	if (!(mysqlWrite(replace, TABLE_TEXT, name, description))) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Can't write text "
		"'%s' to database table %s.", filename, TABLE_TEXT);
		extended_mudlog(NRM, SYSL_SQL, TRUE, "Query error "
		"(%s)%s:%d: %s", __FILE__, __FUNCTION__, __LINE__, 
		mysql_error(&mysql));
		return;
	}
	if (*name)
		SQL_FREE(name);
	if (*description)
		SQL_FREE(description);
	do_reload(0, (char*)filename,0,0);

}
