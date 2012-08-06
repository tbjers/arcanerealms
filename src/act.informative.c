/* ************************************************************************
*		File: act.informative.c                             Part of CircleMUD *
*	 Usage: Player-rights commands of an informative nature                 *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: act.informative.c,v 1.179 2004/04/23 15:18:10 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "time.h"
#include "quest.h"
#include "characters.h"
#include "guild.h"
#include "guild_parser.h"
#include "mail.h"
#include "color.h"

/* extern variables */
extern int top_of_tip_table;
extern struct time_info_data time_info;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct aq_data *aquest_table;
extern struct race_list_element *race_list;
extern int top_of_culture_list;
extern struct culture_list_element *culture_list;
extern struct index_data *obj_index;
extern struct index_data *mob_index;
extern int nameserver_is_slow;
extern int track_through_doors;
extern struct guild_info *guilds_data;
extern obj_rnum top_of_objt;
extern mob_rnum top_of_mobt;
extern int tasks_interval;
extern int events_interval;

extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *meeting;
extern char *changes;
extern char *buildercreds;
extern char *guidelines;
extern char *roleplay;
extern char *snooplist;
extern char *namepolicy;
extern char *ideas;
extern char *typos;
extern char *bugs;

extern char *combat_skills[];
extern const char *languages[];
extern struct tip_index_element *tip_table;

/* global */
int	boot_high = 0;
time_t boot_high_time = 0;
 
/* extern functions */
ACMD(do_action);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
int	compute_armor_class(struct char_data *ch);
int	thaco(struct char_data *ch);
void master_parser (char *parsebuf, struct char_data *ch, struct room_data *room, struct obj_data *obj);
void clean_up (char *in);
bitvector_t asciiflag_conv(char *flag);
void gen_tog(struct char_data *ch, int subcmd);
void tog_rp(struct char_data *ch);
int share_guild(struct char_data *ch1, struct char_data *ch2);
int get_guild(struct char_data *ch, int num);
extern int is_name(const char *str, const char *namelist);
extern int isname(const char *str, const char *namelist);

/* local functions */
char *find_exdesc(char *word, struct extra_descr_data *list);
void diag_char_to_char(struct char_data *i, struct char_data *ch);
void do_auto_exits(struct char_data *ch);
void list_char_to_char(struct char_data *list, struct char_data *ch);
void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, bool show);
void list_one_char(struct char_data *i, struct char_data *ch);
void look_at_char(struct char_data *i, struct char_data *ch);
void look_at_target(struct char_data *ch, char *arg);
void look_in_direction(struct char_data *ch, int dir);
void look_in_obj(struct char_data *ch, char *arg);
void meter_bar(struct char_data *ch, int meter_flag, char ** pbuf);
void perform_immort_where(struct char_data *ch, char *arg);
void perform_mortal_where(struct char_data *ch, char *arg);
void print_object_location(int num, struct obj_data *obj, struct char_data *ch, int recur);
void show_obj_to_char(struct obj_data * object, struct char_data *ch, int mode);
void sort_commands(void);
void send_tip_message(void);
void check_pending_tasks(struct char_data *ch, int subcmd);
int imm_titles (struct char_data *ch);
void extract_char_final(struct char_data *ch);
int sort_commands_helper(const void *a, const void *b);
void show_obj_modifiers(struct obj_data *obj, struct char_data *ch);

/* Local globals */
struct sort_struct {
	int sort_pos;
	byte is_social;
}	*cmd_sort_info = NULL;

ACMD(do_calendar);
ACMD(do_color);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exits);
ACMD(do_gen_ps);
ACMD(do_gold);
ACMD(do_inventory);
ACMD(do_look);
ACMD(do_scan);
ACMD(do_score);
ACMD(do_toggle);
ACMD(do_users);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_wprof);
ACMD(do_whois);
ACMD(do_attributes);

/* For show_obj_to_char 'mode'.	/-- arbitrary */
#define SHOW_OBJ_LONG			0
#define SHOW_OBJ_SHORT		1
#define SHOW_OBJ_ACTION		2


void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode)
{
	char objstring[1024];

	if (!obj || !ch) {
		mlog("SYSERR: NULL pointer in show_obj_to_char(): obj=%p ch=%p", obj, ch);
		return;
	}

	switch (mode) {
	case SHOW_OBJ_LONG:
		strncpy(objstring, obj->description, sizeof(objstring));
		proc_color(objstring, COLOR_LEV(ch), TRUE, GET_OBJ_COLOR(obj), GET_OBJ_RESOURCE(obj));
		send_to_char(objstring, ch);
		break;

	case SHOW_OBJ_SHORT:
		strncpy(objstring, obj->short_description, sizeof(objstring));
		proc_color(objstring, COLOR_LEV(ch), TRUE, GET_OBJ_COLOR(obj), GET_OBJ_RESOURCE(obj));
		send_to_char(objstring, ch);
		break;

	case SHOW_OBJ_ACTION:
		switch (GET_OBJ_TYPE(obj)) {
		case ITEM_NOTE:
			if (obj->action_description) {
				char *notebuf = get_buffer(MAX_STRING_LENGTH);
				sprintf(notebuf, "There is something written on it:\r\n\r\n%.*s", MAX_MAIL_SIZE, obj->action_description);
				page_string(ch->desc, notebuf, TRUE);
				release_buffer(notebuf);
			} else
				send_to_char("It's blank.\r\n", ch);
			return;

		case ITEM_DRINKCON:
			if (GET_OBJ_VAL(obj, 2) == LIQ_COLOR)
				send_to_char("It looks like a dye container.", ch);
			else
				send_to_char("It looks like a drink container.", ch);
			break;

		default:
			sprintf(buf, "When you look at %s you do not see anything special.", obj->short_description);
			send_to_char(buf, ch);
			break;
		}
		break;

	default:
		mlog("SYSERR: Bad display mode (%d) in show_obj_to_char().", mode);
		return;
	}
  show_obj_modifiers(obj, ch);
  send_to_char("\r\n", ch);
}


void show_obj_modifiers(struct obj_data *obj, struct char_data *ch)
{
	if (OBJ_FLAGGED(obj, ITEM_INVISIBLE))
		send_to_char(" (invisible)", ch);

	if (OBJ_FLAGGED(obj, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
		send_to_char(" ..It glows blue!", ch);

	if (OBJ_FLAGGED(obj, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
		send_to_char(" ..It glows yellow!", ch);

	if (OBJ_FLAGGED(obj, ITEM_GLOW))
		send_to_char(" ..It has a soft glowing aura!", ch);

	if (OBJ_FLAGGED(obj, ITEM_HUM))
		send_to_char(" ..It emits a faint humming sound!", ch);
}


void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, bool show)
{
	struct obj_data *i, *j;
	char *printbuf = get_buffer(32);
	bool found;
	int num;

	found = FALSE;
	for (i = list; i; i = i->next_content) {
		num = 0;
		for (j = list; j != i; j = j->next_content)
			if (GET_OBJ_PROTOVNUM(j) == NOTHING) {
				if (strcmp(j->short_description, i->short_description) == 0 && GET_OBJ_COLOR(j) == GET_OBJ_COLOR(i) && GET_OBJ_RESOURCE(j) == GET_OBJ_RESOURCE(i))
					break;
			} else if (GET_OBJ_PROTOVNUM(j) == GET_OBJ_PROTOVNUM(i) && GET_OBJ_COLOR(j) == GET_OBJ_COLOR(i) && GET_OBJ_RESOURCE(j) == GET_OBJ_RESOURCE(i))
				break;
		if (j != i)
			continue;
		for (j = i; j; j = j->next_content)
			if (GET_OBJ_PROTOVNUM(j) == NOTHING) {
				if(strcmp(j->short_description, i->short_description) == 0 && GET_OBJ_COLOR(j) == GET_OBJ_COLOR(i) && GET_OBJ_RESOURCE(j) == GET_OBJ_RESOURCE(i))
					num++;
			} else if (GET_OBJ_PROTOVNUM(j) == GET_OBJ_PROTOVNUM(i) && GET_OBJ_COLOR(j) == GET_OBJ_COLOR(i) && GET_OBJ_RESOURCE(j) == GET_OBJ_RESOURCE(i))
				num++;

		if (CAN_SEE_OBJ(ch, i)) {
			if (PRF_FLAGGED(ch, PRF_SHOWVNUMS) && (!OBJ_FLAGGED(i, ITEM_NODISPLAY) || PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
				sprintf(printbuf, "[%4d] ", GET_OBJ_VNUM(i));
				send_to_char(printbuf, ch);
			}
			if (num!=1) {
				sprintf(printbuf,"(%2i) ", num);
				send_to_char(printbuf, ch);
			}
			if (!(mode == 0 && OBJ_FLAGGED(i, ITEM_NODISPLAY) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
				show_obj_to_char(i, ch, mode);
			found = TRUE;
		}
	}
	if (!found && show)
		send_to_char(" Nothing.\r\n", ch);
	release_buffer(printbuf);
}


void diag_char_to_char(struct char_data * i, struct char_data *ch)
{
	char *printbuf;
	int percent;

	if (!IS_NPC(ch) && !IS_NPC(i)) {
		if (IN_OLC(ch->desc))
			return;
	}

	if (GET_MAX_HIT(i) > 0)
		percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
	else
		percent = -1;                /* How could MAX_HIT be < 1?? */

	printbuf = get_buffer(512);

	if (!IS_NPC(i)) {
		if (GET_SEX(i) == 1)
			strcpy(printbuf, "he");
		else if (GET_SEX(i) == 2)
			strcpy(printbuf, "she");
		else
			strcpy(printbuf, "it");
	} else {
		strcpy(printbuf, PERS(i, ch, 0));
		CAP(printbuf);
	}

	if (percent >= 100)
		strcat(printbuf, " is in excellent condition.\r\n");
	else if (percent >= 90)
		strcat(printbuf, " has a few scratches.\r\n");
	else if (percent >= 75)
		strcat(printbuf, " has some small wounds and bruises.\r\n");
	else if (percent >= 50)
		strcat(printbuf, " has quite a few wounds.\r\n");
	else if (percent >= 30)
		strcat(printbuf, " has some big nasty wounds and scratches.\r\n");
	else if (percent >= 15)
		strcat(printbuf, " looks pretty hurt.\r\n");
	else if (percent >= 0)
		strcat(printbuf, " is in awful condition.\r\n");
	else
		strcat(printbuf, " is bleeding awfully from big wounds.\r\n");

	strcat(printbuf, "&n");

	send_to_char(printbuf, ch);
	release_buffer(printbuf);
}


void look_at_char(struct char_data * i, struct char_data *ch)
{
	int j, found;
	char *printbuf = get_buffer(MAX_STRING_LENGTH);

	if (!ch->desc)
		return;

	if (!IS_NPC(i)) {
		char *name = get_buffer(64);
		sprintf(name, "(%s) ", GET_NAME(i));
		sprintf(printbuf, "&c%s%s%s%s&n%s", IS_IC(i) && find_recognized(ch, i) ? name : "" , IS_IC(i) ? "" : i->player.name, IS_IC(i) ? "" : " ", IS_IC(i) ? (GET_LDESC(i) ? GET_LDESC(i) : "Somebody <L-Desc not set>\r\n") : GET_TITLE(i), IS_IC(i) ? "" : "\r\n");
		release_buffer(name);
		if ((IS_IMMORTAL(ch) || IS_IC(i)) && GET_RPDESCRIPTION(i, GET_ACTIVEDESC(i))) { 
			sprintf(printbuf, "%s%s", CAP(printbuf), GET_RPDESCRIPTION(i, GET_ACTIVEDESC(i)));
			send_to_char(printbuf, ch);
		} else {
			send_to_char(printbuf, ch);
			if (!IS_NPC(i) && GET_RACE(i) < RACE_DRAGON) {
				sprintf(printbuf, "&yA %s %s with %s %s hair and %s eyes,\r\nand $s skin is of %s %s tone.&n", genders[(int)GET_SEX(i)], race_list[(int)GET_RACE(i)].name, 
					hair_style[(int)GET_HAIRSTYLE(i)], hair_color[(int)GET_HAIRCOLOR(i)], eye_color[(int)GET_EYECOLOR(i)], AN(skin_tone[(int)GET_SKINTONE(i)]), skin_tone[(int)GET_SKINTONE(i)]);
				act(printbuf, FALSE, i, 0, ch, TO_VICT);
			} else if (!IS_NPC(ch) && GET_RACE(ch) >= RACE_DRAGON) {
				sprintf(printbuf, "&yYou see something what appears to be %s %s.&n", AN(race_list[(int)GET_RACE(i)].name), race_list[(int)GET_RACE(i)].name);
				act(printbuf, FALSE, i, 0, ch, TO_VICT);
			}
			else {
				act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);
			}
		}
	} else {
		if (i->player.description) {
			sprintf(printbuf, "&g%s&n", i->player.description);
			send_to_char(printbuf, ch);
		} else
			act("&gYou see nothing special about $m.&n", FALSE, i, 0, ch, TO_VICT);
	}

	if (IS_NPC(i))
		diag_char_to_char(i, ch);

	found = FALSE;
	for (j = 0; !found && j < NUM_WEARS; j++)
		if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
			found = TRUE;

	if (found) {
		send_to_char("\r\n", ch);  /* act() does capitalization. */
		act("$n is using:", FALSE, i, 0, ch, TO_VICT);
		for (j = 0; j < NUM_WEARS; j++)
			if (GET_EQ(i, wear_slots[j]) && CAN_SEE_OBJ(ch, GET_EQ(i, wear_slots[j]))) {
				send_to_char(wear_sort_where[j], ch);
				show_obj_to_char(GET_EQ(i, wear_slots[j]), ch, SHOW_OBJ_SHORT);
			}
	}
	if (ch != i && IS_IMMORTAL(ch)) {
		act("\r\nYou peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
		list_obj_to_char(i->carrying, ch, SHOW_OBJ_SHORT, TRUE);
	}
	release_buffer(printbuf);
}


void list_one_char(struct char_data * i, struct char_data *ch)
{
	char *fightbuf;
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	const char *positions[] = {
		" lies here, dead.",
		" lies here, mortally wounded.",
		" lies here, incapacitated.",
		" lies here, stunned.",
		" lies here.",
		" is meditating here.",
		" rests here.",
		" sits here.",
		"!FIGHTING!",
		"!DODGING!",
		"!DEFENDING!",
		" stands here.",
		" stands here on watch.",
		" is in need of repair here."
	};

	if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i)) {
		char *printbuf1 = get_buffer(MAX_STRING_LENGTH);

		if (AFF_FLAGGED(i, AFF_INVISIBLE))
			strcpy(printbuf, "!");
		
		if (PRF_FLAGGED(ch, PRF_SHOWVNUMS)) {
			sprintf(printbuf1, "[%4d] ", GET_MOB_VNUM(i));
			strcat(printbuf, printbuf1);
		}

		if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
			if (IS_EVIL(i))
				strcat(printbuf, "&r[&RRed Aura&r]&n ");
			else if (IS_GOOD(i))
				strcat(printbuf, "&b[&BBlue Aura&b]&n ");
		}
		strcat(printbuf, i->player.long_descr);
		send_to_char(printbuf, ch);

		if (AFF_FLAGGED(i, AFF_SANCTUARY))
			act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
		if (AFF_FLAGGED(i, AFF_ORB))
			act("...$e glows with a pale white light!", FALSE, i, 0, ch, TO_VICT);
		if (AFF_FLAGGED(i, AFF_BLIND))
			act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);

		release_buffer(printbuf);
		release_buffer(printbuf1);
		return;
	}

	if (IS_NPC(i)) {
		if (PRF_FLAGGED(ch, PRF_SHOWVNUMS))
			sprintf(printbuf, "[%4d] %s", GET_MOB_VNUM(i), i->player.short_descr);
		else
			strcpy(printbuf, i->player.short_descr);
		CAP(printbuf);
	} else {
		if (IS_IC(i)) {
			char *name = get_buffer(64);
			sprintf(name, "(&W%s&n) ", GET_NAME(i));
			if (IS_IC(ch) && GET_DOING(i) != NULL)
				sprintf(printbuf, "%s%s is %s", find_recognized(ch, i) ? name : "", GET_SDESC(i) ? GET_SDESC(i) : "Somebody <No S-Desc set>", GET_DOING(i));
			else if (GET_POS(i) == POS_STANDING && GET_LDESC(i)) {
				send_to_charf(ch, "%s%s", find_recognized(ch, i) ? name : "", GET_LDESC(i));
				release_buffer(name);
				release_buffer(printbuf);
				return;
			}	else
				sprintf(printbuf, "%s%s", find_recognized(ch, i) ? name : "", GET_SDESC(i) ? GET_SDESC(i) : GET_NAME(i));
			release_buffer(name);
		} else {
			sprintf(printbuf, "%s%s %s&n", (IS_IC(i)?"":"*"), i->player.name, GET_TITLE(i));
		}
	}

	if (AFF_FLAGGED(i, AFF_INVISIBLE))
		strcat(printbuf, " (invisible)");
	if (AFF_FLAGGED(i, AFF_HIDE))
		strcat(printbuf, " (hidden)");
	if (!IS_NPC(i)) {
		if (!i->desc)
			strcat(printbuf, " (Linkless)");
		else if (PLR_FLAGGED(i, PLR_OLC))
			strcat(printbuf, " (OLC)");
		else if (EDITING(i->desc))
			sprintf(printbuf, "%s (%s)", printbuf, edit_modes[EDITING(i->desc)->mode]);
	}

	if (!IS_NPC(i) && SESS_FLAGGED(i, SESS_AFK))
		strcat(printbuf, " (afk)");
	if (!IS_NPC(i) && SESS_FLAGGED(i, SESS_AFW))
		strcat(printbuf, " (afw)");

	if (IS_IC(ch) && IS_IC(i) && GET_DOING(i)) {
		strcat(printbuf, "\r\n");
		send_to_char(printbuf, ch);
		release_buffer(printbuf);
		return;
	}

	fightbuf = get_buffer(MAX_INPUT_LENGTH);
	
	if (FIGHTING(i)) {
		if (FIGHTING(i) == ch)
			strcpy(fightbuf, "YOU!");
		else {
			if (IN_ROOM(i) == IN_ROOM(FIGHTING(i)))
				strcpy(fightbuf, PERS(FIGHTING(i), ch, 0));
			else
				strcat(fightbuf, "someone who has already left");
			strcat(fightbuf, "!");
		}
	}

	switch (GET_POS(i)) {
		case POS_FIGHTING:
			if (FIGHTING(i))
				sprintf(printbuf + strlen(printbuf), " is here, fighting %s", fightbuf);
			else
				strcat(printbuf, " is here struggling with thin air.");
			break;
		case POS_DODGE:
			if (FIGHTING(i))
				sprintf(printbuf + strlen(printbuf), " is here, dodging the attacks of %s", fightbuf);
			else
				strcat(printbuf, " is here, dodging the attacks of the air.");
			break;
		case POS_DEFEND:
			if (FIGHTING(i))
				sprintf(printbuf + strlen(printbuf), " is here, defending against %s", fightbuf);
			else
				strcat(printbuf, " is here, defending against the world.");
			break;
		default:
			strcat(printbuf, positions[(int) GET_POS(i)]);
			break;
	}
	
	release_buffer(fightbuf);
	
	if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
		if (IS_EVIL(i))
			strcat(printbuf, " (Red Aura)");
		else if (IS_GOOD(i))
			strcat(printbuf, " (Blue Aura)");
	}

	strcat(printbuf, "\r\n");
	send_to_char(printbuf, ch);
	release_buffer(printbuf);

}


void list_char_to_char(struct char_data * list, struct char_data *ch)
{
	struct char_data *i;

	for (i = list; i; i = i->next_in_room)
		if (ch != i) {
			if (CAN_SEE(ch, i))
				list_one_char(i, ch);
			else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) &&
							 AFF_FLAGGED(i, AFF_INFRAVISION))
				send_to_char("You see a pair of glowing red eyes looking your way.\r\n", ch);
		}
}


void do_auto_exits(struct char_data *ch)
{
	char *printbuf = get_buffer(256), *exits = get_buffer(256);
	int door, slen = 0;
	
	for (door = 0; door < NUM_OF_DIRS; door++) {
		if (PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
			if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE && !PRF_FLAGGED(ch, PRF_SHOWVNUMS))
				slen += sprintf(printbuf+slen, "%s%s ",
												(EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) ? "!" : ""),
												abbr_dirs[door]);
			else if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
				slen += sprintf(printbuf+slen, "%s%s:%d&W ",
												(EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) ? "!" : ""),
												abbr_dirs[door], GET_ROOM_VNUM(EXIT(ch, door)->to_room));
		}
		else if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
			!EXIT_FLAGGED(EXIT(ch, door), EX_FAERIE) &&
			!EXIT_FLAGGED(EXIT(ch, door), EX_INFERNAL) &&
			!EXIT_FLAGGED(EXIT(ch, door), EX_DIVINE) &&
			!EXIT_FLAGGED(EXIT(ch, door), EX_ANCIENT) &&
			!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
			slen += sprintf(printbuf+slen, "%s ", abbr_dirs[door]);
	}	
	sprintf(exits, "&G[ Exits: &W%s&G]&n\r\n", *printbuf ? printbuf : "None! ");
	release_buffer(printbuf);
	send_to_char(exits, ch);
	release_buffer(exits);
}


ACMD(do_exits)
{
	char *printbuf, *exit, *exlist;
	int door;
	
	if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
		return;
	}

	printbuf = get_buffer(2048);
	exit = get_buffer(256);
	exlist = get_buffer(2048);

	for (door = 0; door < NUM_OF_DIRS; door++)
		if (PRF_FLAGGED(ch, PRF_HOLYLIGHT) && EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
			!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)) {
			if (IS_IMMORTAL(ch))
				sprintf(exit, "%-5s - [%5d] %s\r\n", dirs[door],
				GET_ROOM_VNUM(EXIT(ch, door)->to_room),
				world[EXIT(ch, door)->to_room].name);
			else {
				sprintf(exit, "%-5s - ", dirs[door]);
				if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
					strcat(exit, "Too dark to tell\r\n");
				else {
					strcat(exit, world[EXIT(ch, door)->to_room].name);
					strcat(exit, "\r\n");
				}
			}
			strcat(exlist, CAP(exit));
		}

		else if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
			!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
			(!(EXIT_FLAGGED(EXIT(ch, door), EX_FAERIE) && !CAN_SEE_FAERIE(ch))) &&
			(!(EXIT_FLAGGED(EXIT(ch, door), EX_INFERNAL) && !CAN_SEE_INFERNAL(ch))) &&
			(!(EXIT_FLAGGED(EXIT(ch, door), EX_DIVINE) && !CAN_SEE_DIVINE(ch))) &&
			(!(EXIT_FLAGGED(EXIT(ch, door), EX_ANCIENT) && !CAN_SEE_ANCIENT(ch)))) {
			if (IS_IMMORTAL(ch))
				sprintf(exit, "%-5s - [%5d] %s\r\n", dirs[door],
				GET_ROOM_VNUM(EXIT(ch, door)->to_room),
				world[EXIT(ch, door)->to_room].name);
			else {
				sprintf(exit, "%-5s - ", dirs[door]);
				if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
					strcat(exit, "Too dark to tell\r\n");
				else {
					strcat(exit, world[EXIT(ch, door)->to_room].name);
					strcat(exit, "\r\n");
				}
			}
			strcat(exlist, CAP(exit));
		}
		release_buffer(exit);

		sprintf(printbuf, "Obvious exits:\r\n%s", *exlist ? exlist : " None.\r\n");		
		send_to_char(printbuf, ch);
		release_buffer(printbuf);
		release_buffer(exlist);
}


void look_at_room(struct char_data *ch, int ignore_brief)
{
	char *parsed_title, *parsed_desc = '\0', *title, *bits;
				
	if (!ch->desc)
		return;

	if (!IS_LIGHT(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
		send_to_char("It is pitch black...\r\n", ch);
		return;
	} else if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char("You see nothing but infinite darkness...\r\n", ch);
		return;
	}

	parsed_title = get_buffer(1024);

	sprintf(parsed_title, "%s", world[IN_ROOM(ch)].name);

	title = get_buffer(1024);
	bits = get_buffer(1024);

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PARSED))                       
		master_parser (parsed_title, ch, NULL, NULL);

	strcpy(title, "&c");

	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
		sprintbit(ROOM_FLAGS(IN_ROOM(ch)), room_bits, bits, 1024);
		sprintf(title, "%s[&C%5d&c ] %s [ &G%s&c]", title, GET_ROOM_VNUM(IN_ROOM(ch)), parsed_title, bits);
	} else {
		sprintf(title, "%s%s", title, parsed_title);
	}

	strcat(title, "&n\r\n&g");
	send_to_char(title, ch);

	release_buffer(parsed_title);
	release_buffer(title);
	release_buffer(bits);

	parsed_desc = get_buffer(32768);

	sprintf(parsed_desc, "%s", world[IN_ROOM(ch)].description);

	if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief ||
			ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH)) {
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PARSED))
			master_parser(parsed_desc, ch, NULL, NULL);
		send_to_char(parsed_desc, ch);
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PARSED)) {
			send_to_char("\r\n", ch);
		}
	}
	send_to_char("&n", ch); /* people */
	
	/* autoexits */
	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
		do_auto_exits(ch);
	
	/* now list characters & objects */
	send_to_char("&y", ch); /* people */
	list_obj_to_char(world[IN_ROOM(ch)].contents, ch, SHOW_OBJ_LONG, FALSE);
	send_to_char("&n", ch); /* people */
	list_char_to_char(world[IN_ROOM(ch)].people, ch);
	send_to_char("&n", ch);

	release_buffer(parsed_desc);	
}


void look_in_direction(struct char_data *ch, int dir)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	if (EXIT(ch, dir)) {
		if (EXIT(ch, dir)->general_description)
			send_to_char(EXIT(ch, dir)->general_description, ch);
		else
			send_to_char("You see nothing special.\r\n", ch);

		if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && EXIT(ch, dir)->keyword) {
			sprintf(printbuf, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
			send_to_char(printbuf, ch);
		} else if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) && EXIT(ch, dir)->keyword) {
			sprintf(printbuf, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
			send_to_char(printbuf, ch);
		}
	} else
		send_to_char("Nothing special there...\r\n", ch);
	release_buffer(printbuf);
}


void look_in_obj(struct char_data *ch, char *arg)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char *printbuf2 = get_buffer(MAX_STRING_LENGTH);
	struct obj_data *obj = NULL;
	struct char_data *dummy = NULL;
	int amt, bits;
	
	if (!*arg)
		send_to_char("Look in what?\r\n", ch);
	else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
		FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
		sprintf(printbuf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
		send_to_char(printbuf, ch);
	} else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
		(GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
		(GET_OBJ_TYPE(obj) != ITEM_CONTAINER) && 
		(GET_OBJ_TYPE(obj) != ITEM_SHEATH))
		send_to_char("There's nothing inside that!\r\n", ch);
	else {
		if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
			if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
				send_to_char("It is closed.\r\n", ch);
			else {
				send_to_char(fname(obj->name), ch);
				switch (bits) {
				case FIND_OBJ_INV:
					send_to_char(" (carried): \r\n", ch);
					break;
				case FIND_OBJ_ROOM:
					send_to_char(" (here): \r\n", ch);
					break;
				case FIND_OBJ_EQUIP:
					send_to_char(" (used): \r\n", ch);
					break;
				}
				
				list_obj_to_char(obj->contains, ch, SHOW_OBJ_SHORT, TRUE);
			}
		} else if (GET_OBJ_TYPE(obj) == ITEM_SHEATH) {
			list_obj_to_char(obj->contains, ch, SHOW_OBJ_SHORT, TRUE);
		} else {                /* item must be a fountain or drink container */
			if (GET_OBJ_VAL(obj, 1) <= 0)
				send_to_char("It is empty.\r\n", ch);
			else {
				if (GET_OBJ_VAL(obj,0) <= 0 || GET_OBJ_VAL(obj,1)>GET_OBJ_VAL(obj,0)) {
					sprintf(printbuf, "Its contents seem somewhat murky.\r\n"); /* BUG */
				} else {
					amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
					sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, printbuf2, MAX_STRING_LENGTH);
					sprintf(printbuf, "It's %sfull of a %s liquid.\r\n", fullness[amt], printbuf2);
				}
				send_to_char(printbuf, ch);
			}
		}
	}
	release_buffer(printbuf);
	release_buffer(printbuf2);
}


char *find_exdesc(char *word, struct extra_descr_data *list)
{
	struct extra_descr_data *i;

	for (i = list; i; i = i->next)
		if (isname(word, i->keyword))
			return (i->description);

	return (NULL);
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
void look_at_target(struct char_data *ch, char *arg)
{
	int bits, found = FALSE, j, fnum, i = 0;
	struct char_data *found_char = NULL;
	struct obj_data *obj, *found_obj = NULL;
	char *desc;
	char objstring[MAX_STRING_LENGTH];

	if (!ch->desc)
		return;

	if (!*arg) {
		send_to_char("Look at what?\r\n", ch);
		return;
	}

	bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
											FIND_CHAR_ROOM, ch, &found_char, &found_obj);

	/* Is the target a character? */
	if (found_char != NULL) {
		look_at_char(found_char, ch);
		if (ch != found_char) {
			if (CAN_SEE(found_char, ch))
				act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
			act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
		}
		return;
	}

	/* Strip off "number." from 2.foo and friends. */
	if (!(fnum = get_number(&arg))) {
		send_to_char("Look at what?\r\n", ch);
		return;
	}

	/* Does the argument match an extra desc in the room? */
	if ((desc = find_exdesc(arg, world[IN_ROOM(ch)].ex_description)) != NULL && ++i == fnum) {
		page_string(ch->desc, desc, FALSE);
		return;
	}

	/* Does the argument match an extra desc in the char's equipment? */
	for (j = 0; j < NUM_WEARS && !found; j++) {
		if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j))) {
			if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL && ++i == fnum) {
				strncpy(objstring, desc, sizeof(objstring));
				proc_color(objstring, COLOR_LEV(ch), TRUE, GET_OBJ_COLOR(GET_EQ(ch, j)), GET_OBJ_RESOURCE(GET_EQ(ch, j)));
				clean_up(objstring);
				send_to_charf(ch, "%s\r\n", objstring);
				found = TRUE;
			}
		}
	}

	/* Does the argument match an extra desc in the char's inventory? */
	for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
		if (CAN_SEE_OBJ(ch, obj)) {
			if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
				strncpy(objstring, desc, sizeof(objstring));
				proc_color(objstring, COLOR_LEV(ch), TRUE, GET_OBJ_COLOR(obj), GET_OBJ_RESOURCE(obj));
				clean_up(objstring);
				send_to_charf(ch, "%s\r\n", objstring);
				found = TRUE;
			}
		}
	}

	/* Does the argument match an extra desc of an object in the room? */
	for (obj = world[IN_ROOM(ch)].contents; obj && !found; obj = obj->next_content) {
		if (CAN_SEE_OBJ(ch, obj)) {
			if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
				strncpy(objstring, desc, sizeof(objstring));
				proc_color(objstring, COLOR_LEV(ch), TRUE, GET_OBJ_COLOR(obj), GET_OBJ_RESOURCE(obj));
				clean_up(objstring);
				send_to_charf(ch, "%s\r\n", objstring);
				found = TRUE;
			}
		}
	}

	/* If an object was found back in generic_find */
	if (bits) {
		if (!found)
			show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION);
		else {
			show_obj_modifiers(found_obj, ch);
			send_to_char("\r\n", ch);
		}
	} else if (!found)
		send_to_char("You do not see that here.\r\n", ch);
}


ACMD(do_look)
{
	int look_type;

	if (!ch->desc)
		return;

	if (GET_POS(ch) < POS_SLEEPING)
		send_to_char("You can't see anything but stars!\r\n", ch);
	else if (AFF_FLAGGED(ch, AFF_BLIND))
		send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
	else if (!IS_LIGHT(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
		send_to_char("It is pitch black...\r\n", ch);
		list_char_to_char(world[IN_ROOM(ch)].people, ch);        /* glowing red eyes */
	} else {
		char *inputarg = get_buffer(MAX_INPUT_LENGTH);
		char *inputarg2 = get_buffer(MAX_INPUT_LENGTH);
		half_chop(argument, inputarg, inputarg2);

		if (subcmd == SCMD_READ) {
			if (!*inputarg)
				send_to_char("Read what?\r\n", ch);
			else
				look_at_target(ch, inputarg);
			release_buffer(inputarg);
			release_buffer(inputarg2);
			return;
		}
		if (!*inputarg)                        /* "look" alone, without an argument at all */
			look_at_room(ch, 1);
		else if ((is_abbrev(inputarg, "in")) && (*inputarg2))
			look_in_obj(ch, inputarg2);
		/* did the char type 'look <direction>?' */
		else if ((look_type = search_block(inputarg, dirs, FALSE)) >= 0 ||
						 (look_type = search_block(inputarg, abbr_dirs, FALSE)) >= 0)
			look_in_direction(ch, look_type);
		else if (is_abbrev(inputarg, "at"))
			look_at_target(ch, inputarg2);
		else
			look_at_target(ch, inputarg);
		release_buffer(inputarg);
		release_buffer(inputarg2);
	}
}



ACMD(do_examine)
{
	char *inputarg = get_buffer(MAX_INPUT_LENGTH);
	char *tempsave;
	struct char_data *tmp_char;
	struct obj_data *tmp_object;
	
	one_argument(argument, inputarg);
	
	if (!*inputarg) {
		send_to_char("Examine what?\r\n", ch);
		release_buffer(inputarg);
		return;
	}

	tempsave = get_buffer(MAX_INPUT_LENGTH);

	/* look_at_target() eats the number. */
	look_at_target(ch, strcpy(tempsave, inputarg));

	release_buffer(tempsave);
	
	generic_find(inputarg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
	
	if (tmp_object) {
		if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) || 
			(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) || 
			(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
			if (GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER) {
				if (OBJVAL_FLAGGED(tmp_object, CONT_CLOSED))
					send_to_char("It is closed.\r\n", ch);
				else {
					send_to_char("When you look inside, you see:\r\n", ch);
					look_in_obj(ch, inputarg);
				}
			} else {
				send_to_char("When you look inside, you see:\r\n", ch);
				look_in_obj(ch, inputarg);
			}
		} else if (GET_OBJ_TYPE(tmp_object) == ITEM_SHEATH) {
			send_to_charf(ch, "It seems to be a sheath for %s.\r\n", skill_name(GET_OBJ_VAL(tmp_object, 0)));
			if (tmp_object->contains) {
				send_to_char("Sheathed:\r\n", ch);
				look_in_obj(ch, inputarg);
			}
		}
	}
	release_buffer(inputarg);
}



ACMD(do_gold)
{
	char *printbuf = get_buffer(256);
	if (GET_GOLD(ch) == 0)
		send_to_char("You're broke!\r\n", ch);
	else if (GET_GOLD(ch) == 1)
		send_to_char("You have one miserable little gold coin.\r\n", ch);
	else {
		sprintf(printbuf, "You have %d gold coins.\r\n", GET_GOLD(ch));
		send_to_char(printbuf, ch);
	}
	release_buffer(printbuf);
}


void meter_bar(struct char_data *ch, int meter_flag, char **pbuf)
{	
	char *printbuf = get_buffer(MAX_STRING_LENGTH), *printbuf2 = get_buffer(MAX_STRING_LENGTH);
	int i, i_percent, max = 0, current = 0, chars = 0;
	float percent = 0;
	const char *meter_name [] = {
		"Hit ",
		"Mana",
		"Move"
	};
	
	switch (meter_flag) {
		
	case 0:
		percent = ((float)GET_HIT(ch)) / ((float)GET_MAX_HIT(ch));
		current=GET_HIT(ch);
		max=GET_MAX_HIT(ch);
		break;
		
	case 1:
		percent = ((float)GET_MANA(ch)) / ((float)GET_MAX_MANA(ch));
		current=GET_MANA(ch);
		max=GET_MAX_MANA(ch);
		break;
		
	case 2:
		percent = ((float)GET_MOVE(ch)) / ((float)GET_MAX_MOVE(ch));
		current=GET_MOVE(ch);
		max=GET_MAX_MOVE(ch);
		break;
		
	default:
		break;
	}
	
	i_percent = ((float)percent) * 100.0;
	chars = 50 * percent;
	
	if (chars > 50)
		chars = 50;
	
	for(i=0; i < chars; ++i)
		*(printbuf2 + i) = '#';
	*(printbuf2 + i) = '\0';

	for(; i < 50; ++i)
		*(printbuf2 + i) = '-';
	*(printbuf2 + i) = '\0';
	
	sprintf(printbuf + strlen(printbuf), "[&r%-50s&n] %s &W%4d&n/&Y%4d&n", printbuf2, meter_name[meter_flag], current, max);
	*pbuf = str_dup(printbuf);
	release_buffer(printbuf);
	release_buffer(printbuf2);
}


ACMD(do_score)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	struct time_info_data playing_time;
	char *meterbuf;
	
	int tmp_hit_clr;
	int tmp_mana_clr;
	int tmp_move_clr;
	int weekday = 0, day = 0, gotAffects = 0;

	const char *suf[] = {
		"!UNUSED!",
		"st",
		"nd",
		"rd"
	};

	const char *score_hmm_color[] = {
		"&W",
		"&G",
		"&R",
	};

	if (IS_NPC(ch)) {
		send_to_char("You hardly need a scoresheet. Off with you.\r\n", ch);
		release_buffer(printbuf);
		return;
	}
	
	day = ch->player_specials->saved.bday_day + 1;        /* day in [1..28] */
	
	if (!(ch->player_specials->saved.bday_month == 12 && day == 28)) {
		weekday = ((28 *ch->player_specials->saved.bday_month) + day) % 7;
	} else {
		weekday = 7;
	}
	if (ch->player_specials->saved.bday_day == 0)
		weekday = 0;

	playing_time = *real_time_passed((time(0) - ch->player.time.logon) + (ch->player.time.played), 0);
	
	sprintf(printbuf, "&c==[ &CGeneral Info &c]==========================================================&n\r\n");
	
	sprintf(printbuf,   "%s&wName  : &Y%s %s&n\r\n", printbuf, GET_NAME(ch), GET_TITLE(ch));

	if (GET_DOING(ch) != NULL)
		sprintf(printbuf, "%s&wDoing : &g%s&n\r\n", printbuf, GET_DOING(ch));

	if (GET_SDESC(ch) != NULL)
		sprintf(printbuf, "%s&wShort Description : &y%s&n\r\n", printbuf, GET_SDESC(ch));

	if (GET_LDESC(ch) != NULL)
		sprintf(printbuf, "%s&wLong Description  :-\r\n&y%s&n", printbuf, GET_LDESC(ch));

	sprintf(printbuf, "%s&wRace    : &Y%-15s  &wAge  : &Y%d&n\r\n", printbuf, race_list[(int)GET_RACE(ch)].name, 
		GET_AGE(ch));

	if (GET_CULTURE(ch) != CULTURE_UNDEFINED)
		sprintf(printbuf, "%s&wCulture : &Y%-30s&n\r\n", printbuf, culture_list[(int)GET_CULTURE(ch)].name);

	if ((age(ch)->month == time_info.month) && (age(ch)->day == time_info.day))    
		sprintf(printbuf, "%s (It's your birthday today.)\r\n", printbuf);

	sprintf(printbuf, "%sYou were born on the day of %s, the %d%s Day of the month %s,\r\nIn the glorious Year %d after" 
		" the First Cataclysm.\r\n", printbuf, weekdays[weekday], day, (((day % 10) < 4 && (day % 10) > 0 && (day < 10 || day > 20))?
			suf[(day % 10)]:"th"), month_name[ch->player_specials->saved.bday_month], ch->player_specials->saved.bday_year);
	
	if (PRF_FLAGGED(ch, PRF_METERBAR)) {
		sprintf(printbuf, "%s&c==[ &CVitals &c]================================================================&n\r\n", printbuf);
		meter_bar(ch, 0, &meterbuf);       /* Hit Point */
		sprintf(printbuf, "%s%s\r\n", printbuf, meterbuf);
		meter_bar(ch, 1, &meterbuf);       /* Mana Point */
		sprintf(printbuf, "%s%s\r\n", printbuf, meterbuf);
		meter_bar(ch, 2, &meterbuf);       /* Move Point */
		sprintf(printbuf, "%s%s\r\n", printbuf, meterbuf);
	}
	
	sprintf(printbuf, "%s&c==[ &CCharacteristics &c]=======================================================&n\r\n", printbuf);
	
	/*check to see if the stat is below 2/3 and above 1/4, if so, */
	if ( (GET_HIT(ch) < ((GET_MAX_HIT(ch) / 3) * 2)) && (GET_HIT(ch) > (GET_MAX_HIT(ch) / 4)) )
		tmp_hit_clr = 1; /* use color green */
	else
		tmp_hit_clr = 0; /* else use color white */
	/*check to see if the stat is below 1/4 if so, */
	if (GET_HIT(ch) < (GET_MAX_HIT(ch) / 4))
		tmp_hit_clr = 2; /* use color bright red */
	
	/*check to see if the stat is below 2/3 and above 1/4,
	 if so, use color green; else use color white */
	if ( (GET_MANA(ch) < ((GET_MAX_MANA(ch) / 3) * 2)) && (GET_MANA(ch) > (GET_MAX_MANA(ch) / 4)) )
		tmp_mana_clr = 1; /* use color green */
	else
		tmp_mana_clr = 0; /* else use color white */
	/*check to see if the stat is below 1/4 if so, */
	if (GET_MANA(ch) < (GET_MAX_MANA(ch) / 4))
		tmp_mana_clr = 2; /* use color bright red */
	
	/*check to see if the stat is below 2/3 and above 1/4,
	 if so, use color green; */
	if ( (GET_MOVE(ch) < ((GET_MAX_MOVE(ch) / 3) * 2)) && (GET_MOVE(ch) > (GET_MAX_MOVE(ch) / 4)) )
		tmp_move_clr = 1; /* use color green */
	else
		tmp_move_clr = 0; /* else use color white */
	/*check to see if the stat is below 1/4 if so, */
	if (GET_MOVE(ch) < (GET_MAX_MOVE(ch) / 4))
		tmp_move_clr = 2; /* use color bright red */
	
	sprintf(printbuf, "%s&wHitp : %s%4d&w/&Y%4d    &wStr: &Y%-2d    &wAgl: &Y%-2d    &wPrc: &Y%-2d    &wPer: &Y%-2d&n\r\n", printbuf, score_hmm_color[tmp_hit_clr],
					GET_HIT(ch), GET_MAX_HIT(ch), GET_STRENGTH(ch) / 100, GET_AGILITY(ch) / 100, GET_PRECISION(ch) / 100, GET_PERCEPTION(ch) / 100);
	sprintf(printbuf, "%s&wMana : %s%4d&w/&Y%4d    &wHea: &Y%-3d   &wWil: &Y%-2d    &wInt: &Y%-2d    &wCha: &Y%-2d&n\r\n", printbuf, score_hmm_color[tmp_mana_clr],
					GET_MANA(ch), GET_MAX_MANA(ch), GET_HEALTH(ch) / 100, GET_WILLPOWER(ch) / 100, GET_INTELLIGENCE(ch) / 100, GET_CHARISMA(ch) / 100);
	sprintf(printbuf, "%s&wMoves: %s%4d&w/&Y%4d    &wLck: &Y%-2d    &wEss: &Y%-2d&n\r\n", printbuf, score_hmm_color[tmp_move_clr],
					GET_MOVE(ch), GET_MAX_MOVE(ch), GET_LUCK(ch) / 100, GET_ESSENCE(ch) / 100);

	sprintf(printbuf, "%s&c==[ &CInformation and Statistics &c]============================================&n\r\n", printbuf);

	sprintf(printbuf, "%s&wGold: &Y%9d     &wPlayTime: &Y%2d &wD &Y%2d &wH&n     &wRPxp: &Y%d&n\r\n", printbuf, GET_GOLD(ch), playing_time.day, playing_time.hours, GET_RPXP(ch));

/* Have to release here, otherwise it'll go over when a player gets hungry/thirsty and lose Quest Info */
	page_string(ch->desc, printbuf, TRUE);
	release_buffer(printbuf);
	printbuf = get_buffer(MAX_STRING_LENGTH);

	sprintf(printbuf, "&c==[ &CStatus and Affects &c]====================================================&n\r\n");
	
	sprintf(printbuf, "%s&wPassive Defense: &w[&Y%3d&w]     Damage Reduction: &w[&Y%3d&w]\r\n", printbuf, GET_PD(ch), GET_REDUCTION(ch));
	
	sprintf(printbuf, "%s&wEnchantments: &c", printbuf);
	//Cheron: Had to move outside of if (AFF_FLAGS(ch)) because Armor and Bless do not set aff flags,
  // and therefore do not show up in score unless an AFF is ALSO there.
  if (affected_by_spell(ch, SPELL_ARMOR)) {
	  gotAffects = 1;
		strcat(printbuf, "&wArmor  ");
	}
	if (affected_by_spell(ch, SPELL_BLESS)) {
		gotAffects = 1;
		strcat(printbuf, "&WBless  ");
	}

  if (AFF_FLAGS(ch)) {
		sprintbit(AFF_FLAGS(ch), affected_flags, buf2, sizeof(buf2));
		sprintf(printbuf + strlen(printbuf), "%s", buf2);
		gotAffects = 1;
	}

	if (gotAffects == 0)
		strcat(printbuf, "none.&n");
	strcat(printbuf, "&n\r\n");
	
	switch (GET_POS(ch)) {
		case POS_DEAD:
			strcat(printbuf, "&RYou are DEAD!&n\r\n");
			break;
		case POS_MORTALLYW:
			strcat(printbuf, "&RYou are mortally wounded!  You should seek help!&n\r\n");
			break;
		case POS_INCAP:
			strcat(printbuf, "&YYou are incapacitated, slowly fading away...&n\r\n");
			break;
		case POS_STUNNED:
			strcat(printbuf, "&MYou are stunned!  You can't move!&n\r\n");
			break;
		case POS_SLEEPING:
			strcat(printbuf, "&gYou are sleeping.&n\r\n");
			break;
		case POS_MEDITATING:
			strcat(printbuf, "&gYou are meditating.&n\r\n");
			break;
		case POS_RESTING:
			strcat(printbuf, "&gYou are resting.&n\r\n");
			break;
		case POS_SITTING:
			strcat(printbuf, "&gYou are sitting.&n\r\n");
			break;
		case POS_FIGHTING:
			if (FIGHTING(ch))
				sprintf(printbuf, "%s&rYou are fighting &R%s&r.&n\r\n", printbuf, PERS(FIGHTING(ch), ch, 0));
			else
				strcat(printbuf, "&rYou are fighting thin air.&n\r\n");
			break;
		case POS_DODGE:
			if (FIGHTING(ch))
				sprintf(printbuf, "%s&rYou are dodging the attacks of &R%s&r.&n\r\n", printbuf, PERS(FIGHTING(ch), ch, 0));
			else
				strcat(printbuf, "&rYou are dodging the air.&n\r\n");
			break;
		case POS_DEFEND:
			if (FIGHTING(ch))
				sprintf(printbuf, "%s&rYou are defending yourself against &R%s&r.&n\r\n", printbuf, PERS(FIGHTING(ch), ch, 0));
			else
				strcat(printbuf, "&rYou are defending yourself against the world.&n\r\n");
			break;
		case POS_STANDING:
			 strcat(printbuf, "&wYou are standing.&n\r\n");
			 break;
		case POS_WATCHING:
			strcat(printbuf, "&wYou are standing on watch.&n\r\n");
			break;
		case POS_REPLACE:
			strcat(printbuf, "&WYou are in need of replacement!&n\r\n");
			break;
		default:
			strcat(printbuf, "&wYou are floating.&n\r\n");
		break;
	}
	
	if (GET_COND(ch, DRUNK) > 10)
		strcat(printbuf, "&MYou are drunk.&n\r\n");
	if (GET_COND(ch, FULL) == 0)
		strcat(printbuf, "&MYou are hungry.&n\r\n");
	if (GET_COND(ch, THIRST) == 0)
		strcat(printbuf, "&MYou are thirsty.&n\r\n");
	
	if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
		strcat(printbuf, "&wYou are summonable by other players.&n\r\n");

	sprintf(printbuf, "%s&c==[ &CQuest Information &c]=====================================================&n\r\n", printbuf);
	sprintf(printbuf, "%s&wQuestpoints : &Y%-9d&n\r\n", printbuf, GET_QP(ch));
	if (GET_QUEST(ch) > 0)
		sprintf(printbuf, "%sQuest       : &Y%s&n\r\n", printbuf, aquest_table[real_quest((int)GET_QUEST(ch))].desc);
	
	page_string(ch->desc, printbuf, TRUE);
	release_buffer(printbuf);

	if (IS_MORTAL(ch))
		list_skill_titles(ch, TRUE);
}


ACMD(do_inventory)
{
	send_to_char("You are carrying:\r\n", ch);
	list_obj_to_char(ch->carrying, ch, SHOW_OBJ_SHORT, TRUE);
}


ACMD(do_equipment)
{
	int i, found = 0;

	send_to_char("You are using:\r\n", ch);
	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, wear_slots[i])) {
			if (CAN_SEE_OBJ(ch, GET_EQ(ch, wear_slots[i]))) {
				send_to_char(wear_sort_where[i], ch);
				show_obj_to_char(GET_EQ(ch, wear_slots[i]), ch, SHOW_OBJ_SHORT);
				found = TRUE;
			} else {
				send_to_char(wear_sort_where[i], ch);
				send_to_char("Something.\r\n", ch);
				found = TRUE;
			}
		}
	}
	if (!found) {
		send_to_char(" Nothing.\r\n", ch);
	}
}


ACMD(do_weather)
{
	char *printbuf = get_buffer(256);
	const char *sky_look[] = {
		"cloudless",
		"cloudy",
		"rainy",
		"lit by flashes of lightning"
	};

	if (OUTSIDE(ch)) {
		sprintf(printbuf, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
						(weather_info.change >= 0 ? "you feel a warm wind from south" :
						 "your foot tells you bad weather is due"));
		send_to_char(printbuf, ch);
	} else
		send_to_char("You have no feeling about the weather at all.\r\n", ch);
	release_buffer(printbuf);
}


/*
 * New 'do_who' by Daniel Koepke [aka., "Argyle Macleod"] of The Keep
 * Modified and butchered by Torgny Bjers (artovil@arcanerealms.org) for Arcane Realms.
 */
int imm_titles (struct char_data *ch)
{	
	if (IS_OWNER(ch))
		return (8);
	else if (IS_IMPLEMENTOR(ch))
		return (7);
	else if (IS_HEADBUILDER(ch))
		return (6);
	else if (IS_DEVELOPER(ch))
		return (5);
	else if (IS_ADMIN(ch))
		return (4);
	else if (IS_BUILDER(ch))
		return (3);
	else if (IS_RPSTAFF(ch))
		return (2);
	else if (IS_QUESTOR(ch))
		return (1);
	else
		return (0);
}


const char *WHO_USAGE =
	"Usage: who [-n name] [-rzqimo] [-g guild]\r\n"
	"\r\n"
	" Switches: \r\n"
	"\r\n"
	"  -r = who is in the current room\r\n"
	"  -z = who is in the current zone\r\n"
	"\r\n"
	"  -q = only show questers\r\n"
	"  -i = only show immortals\r\n"
	"  -m = only show mortals\r\n"
	"  -o = only show outlaws\r\n"
	"  -g = only show guild mates (displays first guild without argument)\r\n";

ACMD(do_who)
{
	struct descriptor_data *d;
	struct char_data *wch;
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char *printbuf1 = get_buffer(MAX_STRING_LENGTH);
	char *Imm_buf = get_buffer(MAX_STRING_LENGTH);
	char *Mort_buf = get_buffer(MAX_STRING_LENGTH);
	char *name_search = get_buffer(MAX_INPUT_LENGTH);
	char *guild_search = get_buffer(MAX_INPUT_LENGTH);
	char *inputarg = get_buffer(MAX_INPUT_LENGTH);
	char mode;
	char *title = NULL;

	const char *titles[] = {
		"   Immortal    ",
		"    Questor    ",
		"    RPStaff    ",
		"    Builder    ",
		"     Admin     ",
		"*  Developer  *",
		"* HeadBuilder *",
		"* Implementor *",
		"**   Owner   **"
	};

	bool who_room = FALSE, who_zone = FALSE, who_quest = 0;
	bool outlaws = FALSE, noimm = FALSE, nomort = FALSE, guildies = FALSE;
	bool guild_header = FALSE;
	
	int guild = 0, guild_num = 0;
	
	int Wizards = 0, Mortals = 0;
	
	skip_spaces(&argument);
	strcpy(printbuf, argument);
	
	/* the below is from stock CircleMUD -- found no reason to rewrite it */
	while (*printbuf) {
		half_chop(printbuf, inputarg, printbuf1);
		if (*inputarg == '-') {
			mode = *(inputarg + 1);       /* just in case; we destroy arg in the switch */
			switch (mode) {
			case 'o':
			case 'k':
				outlaws = TRUE;
				strcpy(printbuf, printbuf1);
				break;
			case 'z':
				who_zone = TRUE;
				strcpy(printbuf, printbuf1);
				break;
			case 'q':
				who_quest = TRUE;
				strcpy(printbuf, printbuf1);
				break;
			case 'n':
				half_chop(printbuf1, name_search, printbuf);
				break;
			case 'r':
				who_room = TRUE;
				strcpy(printbuf, printbuf1);
				break;
			case 'i':
				nomort = TRUE;
				strcpy(printbuf, printbuf1);
				break;
			case 'm':
				noimm = TRUE;
				strcpy(printbuf, printbuf1);
				break;
			case 'g':
				if (!GET_GUILD(ch)) {
					send_to_char("You are not guilded!\r\n", ch);
					release_buffer(printbuf);
					release_buffer(printbuf1);
					release_buffer(Imm_buf);
					release_buffer(Mort_buf);
					release_buffer(name_search);
					release_buffer(guild_search);
					release_buffer(inputarg);
					return;
				}
				guildies = TRUE;
				half_chop(printbuf1, guild_search, printbuf);
				guild_num = atoi(guild_search);
				break;
			default:
				send_to_char(WHO_USAGE, ch);
				release_buffer(printbuf);
				release_buffer(printbuf1);
				release_buffer(Imm_buf);
				release_buffer(Mort_buf);
				release_buffer(name_search);
				release_buffer(guild_search);
				release_buffer(inputarg);
				return;
			}                                /* end of switch */
			
		} else {                        /* endif */
			send_to_char(WHO_USAGE, ch);
			release_buffer(printbuf);
			release_buffer(printbuf1);
			release_buffer(Imm_buf);
			release_buffer(Mort_buf);
			release_buffer(name_search);
			release_buffer(guild_search);
			release_buffer(inputarg);
			return;
		}
	}                                /* end while (parser) */

	sprintf(printbuf, "\r\nType 'who help' to get instructions on how to sort the list.\r\n==============================================================================\r\n");
	
	strcpy(Imm_buf, "");
	strcpy(Mort_buf,"");
	
	for (d = descriptor_list; d; d = d->next) {
		if (!(IS_PLAYING(d)))
			continue;
		
		if (d->original)
			wch = d->original;
		else if (!(wch = d->character))
			continue;

		if (!CAN_SEE_WHO(ch, wch))
			continue;
		if ((noimm && IS_IMMORTAL(wch)) || (nomort && IS_MORTAL(wch)))
			continue;
		if (*name_search && str_cmp(GET_NAME(wch), name_search) && !strstr(GET_TITLE(wch), name_search))
			continue;
		if (outlaws && !PLR_FLAGGED(wch, PLR_KILLER) && !PLR_FLAGGED(wch, PLR_THIEF))
			continue;
		if (who_quest && !PRF_FLAGGED(wch, PRF_QUEST))
			continue;
		if (who_zone && world[IN_ROOM(ch)].zone != world[IN_ROOM(wch)].zone)
			continue;
		if (who_room && (IN_ROOM(wch) != IN_ROOM(ch)))
			continue;
		if (guildies && !guild_num && !(guild = share_guild(ch, wch)))
			continue;
		if (guildies && guild_num && !get_guild(wch, guild_num))
			continue;

		if (guildies) {
			struct guild_info *g;
			struct guildie_info *guildie;
			g = guilds_data;
			while (g) {
				if (!guild_num && g->id == guild) break;
				if (g->id == guild_num) break;
				g = g->next;
			}
			if (g) {
				if (!guild_header) {
					sprintf(printbuf, "%sGuild members in &W%s:&n\r\n", printbuf, guild_num ? g->name : "all your guilds");
					strcat(printbuf, "------------------------------------------------------------------------------\r\n");
					guild_header = TRUE;
				}
				guildie = g->guildies;
				while (guildie) {
					if (STATUS_FLAGGED(guildie, STATUS_MEMBER) && guildie->idnum == GET_IDNUM(wch)) {
						if (IS_IMMORTAL(guildie->ch))
							Wizards++;
						else
							Mortals++;
						if (!guildie->subrank && !guildie->rank_num)
							sprintf(printbuf1, "&c[&C%-3s&c]&n (%s) %s %s",
								((IS_IC(guildie->ch))?"IC":"OOC"),
								g->name,
								GET_NAME(guildie->ch),
								GET_TITLE(guildie->ch));
						else
							sprintf(printbuf1, "&c[&C%-3s&c]&n (%s) %s%s%s%s%s%s %s",
								((IS_IC(guildie->ch))?"IC":"OOC"),
								g->name,
								(STATUS_FLAGGED(guildie, STATUS_SUBRANK) ? "&W[&n" : ""),
								(STATUS_FLAGGED(guildie, STATUS_SUBRANK) ? guildie->subrank : ""),
								(STATUS_FLAGGED(guildie, STATUS_SUBRANK) ? "&W]&n " : ""),
								((guildie->rank_num != 0) ? guildie->rank->name : ""),
								((guildie->rank_num != 0) ? " " : ""),
								GET_NAME(guildie->ch),
								GET_TITLE(guildie->ch));
					}
					guildie = guildie->next;
				}
			}
		} else {
			if (IS_IMMORTAL(wch)) {
				title = get_title(wch);
				sprintf(printbuf1, "&c[&C%s&c]%s%s%s &C%s &c%s&n", 
					titles[imm_titles(wch)], title ? " &n{" : "", title ? title : "", title ? "&n}" : "", GET_NAME(wch), GET_TITLE(wch));
				Wizards++;
			} else {
				title = get_title(wch);
				sprintf(printbuf1, "&c[&C%-3s&c]%s%s%s &G%s &g%s&n", (IS_IC(wch)?"IC":"OOC"),
					title ? " &n{" : "", title ? title : "", title ? "&n}" : "", GET_NAME(wch), GET_TITLE(wch));
				Mortals++;
			}
		}

		if (GET_INVIS_LEV(wch) != RIGHTS_NONE)
			sprintf(printbuf1, "%s &n(&Wi-%s&n)", printbuf1, wiz_char_rights(GET_INVIS_LEV(wch)));
		else if (IS_AFFECTED(wch, AFF_INVISIBLE))
			strcat(printbuf1, " &n(&Winvis&n)");
		
		if (PLR_FLAGGED(wch, PLR_OLC))
			sprintf(printbuf1, "%s &y(&Y%s&y)", printbuf1, connected_types[STATE(d)]);
		else if (EDITING(d))
			sprintf(printbuf1, "%s &y(&Y%s&y)", printbuf1, edit_modes[EDITING(d)->mode]);
		
		if (PRF_FLAGGED(wch, PRF_DEAF))
			strcat(printbuf1, " &n(deaf)");
		if (PRF_FLAGGED(wch, PRF_NOTELL))
			strcat(printbuf1, " &n(notell)");
		if (PRF_FLAGGED(wch, PRF_QUEST))
			strcat(printbuf1, " &n(quest)");
		if (PLR_FLAGGED(wch, PLR_THIEF))
			strcat(printbuf1, " &r(THIEF)");
		if (PLR_FLAGGED(wch, PLR_KILLER))
			strcat(printbuf1, " &R(KILLER)");
		if (SESS_FLAGGED(wch, SESS_AFK))
			strcat(printbuf1, " &n(afk)");
		if (SESS_FLAGGED(wch, SESS_AFW))
			strcat(printbuf1, " &n(afw)");
		strcat(printbuf1, "&n\r\n");
		
		if (IS_IMMORTAL(wch))
			strcat(Imm_buf, printbuf1);
		else
			strcat(Mort_buf, printbuf1);
	} /* end of for */
	
	if (Wizards)
		strcat(printbuf, Imm_buf);

	if (Mortals && Wizards)
		strcat(printbuf, "------------------------------------------------------------------------------\r\n");
	
	if (Mortals)
		strcat(printbuf, Mort_buf);
	
	if ((Wizards + Mortals) == 0)
		strcat(printbuf, "There are no other players online right now.\r\n");

	if ((Wizards + Mortals) > boot_high) {
		boot_high = Wizards + Mortals;
		boot_high_time = time(0);
	}

	sprintf(printbuf, "%s------------------------------------------------------------------------------\r\nThere %s &W%d &ncharacter%s on, the most on today was &W%d&n%s.&n\r\n", printbuf,
					(Wizards+Mortals == 1 ? "is" : "are"), Wizards+Mortals, (Wizards+Mortals == 1 ? "" : "s"), boot_high, (boot_high > 1 ? "" : " measly character"));

	page_string(ch->desc, printbuf, TRUE);

	release_buffer(printbuf);
	release_buffer(printbuf1);
	release_buffer(Imm_buf);
	release_buffer(Mort_buf);
	release_buffer(name_search);
	release_buffer(guild_search);
	release_buffer(inputarg);
}


#define	USERS_FORMAT \
"format: users [-n name] [-h host] [-g guild] [-o] [-p]\r\n"

ACMD(do_users)
{
	const char *format = "%3d %-5s %-14.14s %-14s %-3s %-8s ";
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char *printbuf1 = get_buffer(MAX_STRING_LENGTH);
	char *line = get_buffer(1024);
	char *idletime = get_buffer(64);
	char *title = get_buffer(128);
	char *state = get_buffer(128);
	char *timeptr, mode;
	char *name_search = get_buffer(MAX_INPUT_LENGTH);
	char *host_search = get_buffer(MAX_INPUT_LENGTH);
	char *inputarg = get_buffer(MAX_INPUT_LENGTH);
	struct char_data *tch;
	struct descriptor_data *d;
	int num_can_see = 0;
	int outlaws = 0, playing = 0, deadweight = 0;

	const char *abbr_titles[] = {
		"Imm",
		"Qst",
		"Stf",
		"Bld",
		"Adm",
		"Dev",
		"HBl",
		"Imp",
		"Own"
	};

	strcpy(printbuf, argument);
	while (*printbuf) {
		half_chop(printbuf, inputarg, printbuf1);
		if (*inputarg == '-') {
			mode = *(inputarg + 1);  /* just in case; we destroy arg in the switch */
			switch (mode) {
			case 'o':
			case 'k':
				outlaws = 1;
				playing = 1;
				strcpy(printbuf, printbuf1);
				break;
			case 'p':
				playing = 1;
				strcpy(printbuf, printbuf1);
				break;
			case 'd':
				deadweight = 1;
				strcpy(printbuf, printbuf1);
				break;
			case 'n':
				playing = 1;
				half_chop(printbuf1, name_search, printbuf);
				break;
			case 'h':
				playing = 1;
				half_chop(printbuf1, host_search, printbuf);
				break;
			default:
				send_to_char(USERS_FORMAT, ch);
				release_buffer(printbuf);
				release_buffer(printbuf1);
				release_buffer(line);
				release_buffer(idletime);
				release_buffer(title);
				release_buffer(name_search);
				release_buffer(host_search);
				release_buffer(state);
				release_buffer(inputarg);
				return;
			}                                /* end of switch */

		} else {                        /* endif */
			send_to_char(USERS_FORMAT, ch);
			release_buffer(printbuf);
			release_buffer(printbuf1);
			release_buffer(line);
			release_buffer(idletime);
			release_buffer(title);
			release_buffer(name_search);
			release_buffer(host_search);
			release_buffer(state);
			release_buffer(inputarg);
			return;
		}
	}                                /* end while (parser) */
	sprintf(printbuf, "&CNum Rank  Name           State          Idl Login@   Site&n\r\n");
	sprintf(printbuf, "%s&c--- ----- -------------- -------------- --- -------- -------------------------&n\r\n", printbuf);

	one_argument(argument, inputarg);

	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) != CON_PLAYING && playing)
			continue;
		if (STATE(d) == CON_PLAYING && deadweight)
			continue;
		if (IS_PLAYING(d)) {
			if (d->original)
				tch = d->original;
			else if (!(tch = d->character))
				continue;

			if (*host_search && !strstr(d->host, host_search))
				continue;
			if (*name_search && str_cmp(GET_NAME(tch), name_search))
				continue;
			if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
					!PLR_FLAGGED(tch, PLR_THIEF))
				continue;
			if (!CAN_SEE(ch, tch))
				continue;

			if (d->original)
				sprintf(title, "&y[&Y%s&y]", ((IS_IMMORTAL(d->original))?abbr_titles[imm_titles(d->original)]:"   "));
			else
				sprintf(title, "&y[&Y%s&y]", ((IS_IMMORTAL(d->character))?abbr_titles[imm_titles(d->character)]:"   "));
		} else
			strcpy(title, "&y[&Y---&y]");

		timeptr = asctime(localtime(&d->login_time));
		timeptr += 11;
		*(timeptr + 8) = '\0';

		if (STATE(d) == CON_PLAYING && d->original)
			strcpy(state, "&MSwitched&y");
		else
			strcpy(state, connected_types[STATE(d)]);

		if (d->character && STATE(d) == CON_PLAYING && !(IS_BUILDER(d->character) || IS_GRGOD(d->character)))
			sprintf(idletime, "&Y%3d&y", d->character->char_specials.timer);
		else
			strcpy(idletime, "");

		if (d->character && d->character->player.name) {
			if (d->original)
				sprintf(line, format, d->desc_num, title,
								d->original->player.name, state, idletime, timeptr);
			else
				sprintf(line, format, d->desc_num, title,
								d->character->player.name, state, idletime, timeptr);
		} else
			sprintf(line, format, d->desc_num, title, "UNDEFINED",
							state, idletime, timeptr);

		if (d->host && *d->host)
			sprintf(line, "%s&y[&Y%s&y]\r\n", line, d->host);
		else
			strcat(line, "&y[&YHostname unknown&y]\r\n");

		if (STATE(d) != CON_PLAYING ||
								(STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character))) {
			num_can_see++;
			if (!(IS_PLAYING(d)))
				sprintf(printbuf, "%s&R%s&n", printbuf, line);
			else
				sprintf(printbuf, "%s&W%s&n", printbuf, line);
		}
	}

	sprintf(printbuf, "%s&n\r\n&W%d &wvisible socket%s connected.&n\r\n", printbuf, num_can_see, (num_can_see > 1)?"s":"");

	page_string(ch->desc, printbuf, TRUE);

	release_buffer(printbuf);
	release_buffer(printbuf1);
	release_buffer(line);
	release_buffer(idletime);
	release_buffer(title);
	release_buffer(name_search);
	release_buffer(host_search);
	release_buffer(state);
	release_buffer(inputarg);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);

	switch (subcmd) {
	case SCMD_CREDITS:
		page_string(ch->desc, credits, 0);
		break;
	case SCMD_NEWS:
		page_string(ch->desc, news, 0);
		break;
	case SCMD_INFO:
		page_string(ch->desc, info, 0);
		break;
	case SCMD_WIZLIST:
		page_string(ch->desc, wizlist, 0);
		break;
	case SCMD_IMMLIST:
		page_string(ch->desc, immlist, 0);
		break;
	case SCMD_HANDBOOK:
		page_string(ch->desc, handbook, 0);
		break;
	case SCMD_POLICIES:
		page_string(ch->desc, policies, 0);
		break;
	case SCMD_MOTD:
		page_string(ch->desc, motd, 0);
		break;
	case SCMD_IMOTD:
		page_string(ch->desc, imotd, 0);
		break;
	case SCMD_MEETING:                   
		page_string(ch->desc, meeting, 0);
		break;
	case SCMD_CHANGES:                   
		page_string(ch->desc, changes, 0);
		break;
	case SCMD_BUILDERCREDS:                   
		page_string(ch->desc, buildercreds, 0);
		break;
	case SCMD_GUIDELINES:                   
		page_string(ch->desc, guidelines, 0);
		break;
	case SCMD_CLEAR:
		send_to_char("\033[H\033[J", ch);
		break;
	case SCMD_VERSION:
		send_to_char(strcat(strcpy(printbuf, circlemud_version), "\r\n"), ch);
		send_to_char(strcat(strcpy(printbuf, DG_SCRIPT_VERSION), "\r\n"), ch);
		break;
	case SCMD_WHOAMI:
		send_to_char(strcat(strcpy(printbuf, GET_NAME(ch)), "\r\n"), ch);
		break;
	case SCMD_ROLEPLAY:
		page_string(ch->desc, roleplay, 0);
		break;
	case SCMD_SNOOPLIST:
		page_string(ch->desc, snooplist, 0);
		break;
	case SCMD_NAMEPOLICY:
		page_string(ch->desc, namepolicy, 0);
		break;
	case SCMD_IDEAS:
		send_to_char("\nThe following is the list of proposed "
			     "ideas. \nLegend: &Y*&n Undecided  &R*&n "
			     "Discarded  &G*&n Approved\nIf you don't see "
			     "your idea here, check the code changes " 
			     "board.  It's been \nimplemented.  Discarded "
			     "ideas will be removed from the list after "
			     "about a \nmonth from the decision date.\n"
			  
			     "--------------------------------------------"
			     "------------------------------\n\n", ch);
		page_string(ch->desc, ideas, 0);
		break;
	case SCMD_BUGS:
		send_to_char("\nThe following is the list of reported "
			     "bugs.\nLegend: &Y*&n New &R*&n Acknowledged " 
			     "&G*&n Fixed\nFixed bugs will be posted on "
			     "the code changes board and removed from this "
			     "\nlist within a month of the fixing date.  "
			     "If you do not see your bug here, \ncheck the "
			     "code changes board as it has been addressed.\n"
			     "---------------------------------------------"
			     "-----------------------------\n\n", ch);
		page_string(ch->desc,bugs,0);
		break;
	case SCMD_TYPOS:
		send_to_char("\nThe following is the list of reported "
			     "typos.\nLegend: &Y*&n New &R*&n Acknowledged "
			     "&G*&n Fixed\nFixed typos will be removed from "
			     "this list within a month from the fix date.\n"
			     "If you do not see your typo here, it has been "
			     "fixed.\n"
			     "---------------------------------------------"
			     "-----------------------------\n\n", ch);
		page_string(ch->desc,typos,0);
		break;
	default:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unhandled case in do_gen_ps. (%d)", subcmd);
		break;
	}

	release_buffer(printbuf);
}


void perform_mortal_where(struct char_data *ch, char *arg)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	struct char_data *i;
	struct descriptor_data *d;
				char *parsed_desc;

	if (!*arg) {
		send_to_char("Players in your Zone\r\n--------------------\r\n", ch);
		for (d = descriptor_list; d; d = d->next) {
			if (!(STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) || d->character == ch)
				continue;
			if ((i = (d->original ? d->original : d->character)) == NULL)
				continue;
			if (IN_ROOM(i) == NOWHERE || !CAN_SEE(ch, i))
				continue;
			if (world[IN_ROOM(ch)].zone != world[IN_ROOM(i)].zone)
				continue;
			parsed_desc=strdup(world[IN_ROOM(i)].name);
			if (ROOM_FLAGGED(IN_ROOM(i),ROOM_PARSED))
				master_parser(parsed_desc, ch, NULL, NULL);
			if (IS_IMMORTAL(i))
				sprintf(printbuf, "&W%-20s&n - &y%s&n\r\n", (GET_NAME(i)), parsed_desc);
			else
				sprintf(printbuf, "&W%-20s&n - &y%s&n\r\n", GET_NAME(i), parsed_desc);
			send_to_char(printbuf, ch);
												
		}
	} else { /* print only FIRST char, not all. */
		for (i = character_list; i; i = i->next) {
			if (IN_ROOM(i) == NOWHERE || i == ch)
				continue;
			if (!CAN_SEE(ch, i) || world[IN_ROOM(i)].zone != world[IN_ROOM(ch)].zone)
				continue;
			if (!isname(arg, i->player.name))
				continue;
			parsed_desc=strdup(world[IN_ROOM(i)].name);
												
			if (ROOM_FLAGGED(IN_ROOM(i),ROOM_PARSED))
				master_parser(parsed_desc, ch, NULL, NULL);
												
			sprintf(printbuf, "&W%-25s&n - &y%s&n\r\n", GET_NAME(i), world[IN_ROOM(i)].name);
			send_to_char(printbuf, ch);
			release_buffer(printbuf);
			return;
		}
		send_to_char("No-one around by that name.\r\n", ch);
	}
	release_buffer(printbuf);
}


void print_object_location(int num, struct obj_data * obj, struct char_data *ch,
																																																				 int recur)
{
	char *parsed_desc;
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
				
	if (num > 0)
		sprintf(printbuf, "O%3d. %-25s - ", num, obj->short_description);
	else
		sprintf(printbuf, "%33s", " - ");

	if (IN_ROOM(obj) > NOWHERE) {
		
		parsed_desc = strdup(world[IN_ROOM(obj)].name);
		if (ROOM_FLAGGED(IN_ROOM(obj),ROOM_PARSED))
										master_parser(parsed_desc, ch, NULL, NULL);
								
		sprintf(printbuf, "%s[%5d] %s\r\n", printbuf,
			GET_ROOM_VNUM(IN_ROOM(obj)), parsed_desc);
		
		send_to_char(printbuf, ch);
								
	} else if (obj->carried_by) {
		sprintf(printbuf, "%scarried by %s\r\n", printbuf,
																								PERS(obj->carried_by, ch, 0));
		send_to_char(printbuf, ch);
	} else if (obj->worn_by) {
		sprintf(printbuf, "%sworn by %s\r\n", printbuf,
																								PERS(obj->worn_by, ch, 0));
		send_to_char(printbuf, ch);
	} else if (obj->in_obj) {
		sprintf(printbuf, "%sinside %s%s\r\n", printbuf,
			obj->in_obj->short_description, (recur ? ", which is" : " "));
		send_to_char(printbuf, ch);
		if (recur)
			print_object_location(0, obj->in_obj, ch, recur);
	} else {
		strcat(printbuf, "in an unknown location\r\n");
		send_to_char(printbuf, ch);
	}
	release_buffer(printbuf);
}


void perform_immort_where(struct char_data *ch, char *arg)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	struct char_data *i;
	struct obj_data *k;
	struct descriptor_data *d;
	int num = 0, found = 0;

	if (!*arg) {
		send_to_char("Players\r\n-------\r\n", ch);
		for (d = descriptor_list; d; d = d->next)
			if (STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) {
				i = (d->original ? d->original : d->character);
				if (i && CAN_SEE(ch, i) && (IN_ROOM(i) != NOWHERE)) {
					if (d->original)
						sprintf(printbuf, "&W%-20s&n - [&c%5d&n] &y%s&n (in %s)\r\n",
										GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(d->character)),
								 world[IN_ROOM(d->character)].name, GET_NAME(d->character));
					else
						sprintf(printbuf, "&W%-20s&n - [&c%5d&n] &y%s&n\r\n", GET_NAME(i),
										GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
					send_to_char(printbuf, ch);
				}
			}
	} else {
		for (i = character_list; i; i = i->next)
			if (CAN_SEE(ch, i) && IN_ROOM(i) != NOWHERE && isname(arg, i->player.name)) {
				found = 1;
				sprintf(printbuf, "M%3d. &W%-25s&n - [&c%5d&n] &y%s&n\r\n", ++num, GET_NAME(i),
								GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
				send_to_char(printbuf, ch);
			}
		for (num = 0, k = object_list; k; k = k->next)
			if (k->name)
				if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
					found = 1;
					print_object_location(++num, k, ch, TRUE);
				}
		if (!found)
			send_to_char("Couldn't find any such thing.\r\n", ch);
	}
	release_buffer(printbuf);
}


ACMD(do_where)
{
	char *inputarg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, inputarg);

	if (IS_IMMORTAL(ch))
		perform_immort_where(ch, inputarg);
	else
		perform_mortal_where(ch, inputarg);
	release_buffer(inputarg);
}


ACMD(do_consider)
{
	char *inputarg = get_buffer(MAX_INPUT_LENGTH);
	struct char_data *victim;
	int diff;

	one_argument(argument, inputarg);

	if (!(victim = get_char_vis(ch, inputarg, NULL, FIND_CHAR_ROOM, 0))) {
		send_to_char("Consider killing who?\r\n", ch);
		release_buffer(inputarg);
		return;
	}
	if (victim == ch) {
		send_to_char("Easy!  Very easy indeed!\r\n", ch);
		release_buffer(inputarg);
		return;
	}
	if (!IS_NPC(victim)) {
		send_to_char("Would you like to borrow a cross and a shovel?\r\n", ch);
		release_buffer(inputarg);
		return;
	}
	diff = (GET_DAMROLL(victim) - GET_DAMROLL(ch));

	if (diff <= -10)
		send_to_char("Now where did that chicken go?\r\n", ch);
	else if (diff <= -5)
		send_to_char("You could do it with a needle!\r\n", ch);
	else if (diff <= -2)
		send_to_char("Easy.\r\n", ch);
	else if (diff <= -1)
		send_to_char("Fairly easy.\r\n", ch);
	else if (diff == 0)
		send_to_char("The perfect match!\r\n", ch);
	else if (diff <= 1)
		send_to_char("You would need some luck!\r\n", ch);
	else if (diff <= 2)
		send_to_char("You would need a lot of luck!\r\n", ch);
	else if (diff <= 3)
		send_to_char("You would need a lot of luck and great equipment!\r\n", ch);
	else if (diff <= 5)
		send_to_char("Do you feel lucky, punk?\r\n", ch);
	else if (diff <= 10)
		send_to_char("Are you mad!?\r\n", ch);
	else if (diff <= 100)
		send_to_char("You ARE mad!\r\n", ch);

	release_buffer(inputarg);
}


ACMD(do_diagnose)
{
	char *inputarg = get_buffer(MAX_INPUT_LENGTH);
	struct char_data *vict;

	one_argument(argument, inputarg);

	if (*inputarg) {
		if (!(vict = get_char_vis(ch, inputarg, NULL, FIND_CHAR_ROOM, 0)))
			send_to_char(NOPERSON, ch);
		else
			diag_char_to_char(vict, ch);
	} else {
		if (FIGHTING(ch))
			diag_char_to_char(FIGHTING(ch), ch);
		else
			send_to_char("Diagnose who?\r\n", ch);
	}
	release_buffer(inputarg);
}


const	char *ctypes[] = {
	"off", "sparse", "normal", "complete", "\n"
};

ACMD(do_color)
{
	char *inputarg, *printbuf;
	int tp;

	if (IS_NPC(ch))
		return;

	inputarg = get_buffer(MAX_INPUT_LENGTH);
	printbuf = get_buffer(512);

	one_argument(argument, inputarg);

	if (!*inputarg) {
		sprintf(printbuf, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
		send_to_char(printbuf, ch);
		release_buffer(inputarg);
		release_buffer(printbuf);
		return;
	}
	if (((tp = search_block(inputarg, ctypes, FALSE)) == -1)) {
		send_to_char("Usage: color { Off | Sparse | Normal | Complete }\r\n", ch);
		release_buffer(inputarg);
		release_buffer(printbuf);
		return;
	}
	REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
	SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (tp & 1)) | (PRF_COLOR_2 * (tp & 2) >> 1));

	sprintf(printbuf, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_SPR),
					CCNRM(ch, C_SPR), ctypes[tp]);
	send_to_char(printbuf, ch);
	release_buffer(inputarg);
	release_buffer(printbuf);
}


void disp_toggles (struct char_data *ch)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH), *printbuf2 = get_buffer(256);

	if (GET_WIMP_LEV(ch) == 0)
		strcpy(printbuf2, "OFF");
	else
		sprintf(printbuf2, "%-3d", GET_WIMP_LEV(ch));

	strcpy(printbuf, "&c.=[ &Ctoggles &c]==============================================================.&n\r\n");

	if (IS_IMPLEMENTOR(ch)) {
		sprintf(printbuf,
					"%s&c|&W Implementor Preferences&c                                                  |&n\r\n"
					"&c|&y          Slowns: &Y%-3s    &n"
					"&y        Trackthru: &Y%-3s      &n"
					"&y                     &c|&n\r\n", printbuf,

				YESNO(nameserver_is_slow),
				ONOFF(track_through_doors)
		);
	}

	if (IS_IMMORTAL(ch)) {
		sprintf(printbuf,
					"%s&c|&W Immortal Preferences&c                                                     |&n\r\n"
					"&c|&y        Nohassle: &Y%-3s    &n"
					"&y        Holylight: &Y%-3s    &n"
					"&y      Roomflags: &Y%-3s   &c|&n\r\n"

					"&c|&y         Wizchan: &Y%-3s    &n"
					"&y            Vnums: &Y%-3s    &n"
					"&y                       &c|&n\r\n", printbuf,

				ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
				ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
				ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS)),

				ONOFF(!PRF_FLAGGED(ch, PRF_NOWIZ)),
				ONOFF(PRF_FLAGGED(ch, PRF_SHOWVNUMS))
		);
	}

	sprintf(printbuf,
		"%s&c|&W Member Preferences&c                                                       |&n\r\n"
		"&c|&y Hit Pnt Display: &Y%-3s    "
		"&y       Brief Mode: &Y%-3s    &n"
		"&y Summon Protect: &Y%-3s   &c|&n\r\n"

		"&c|&y    Move Display: &Y%-3s    "
		"&y     Compact Mode: &Y%-3s    &n"
		"&y       On Quest: &Y%-3s   &c|&n\r\n"

		"&c|&y    Mana Display: &Y%-3s    "
		"&y           NoTell: &Y%-3s    &n"
		"&y   Repeat Comm.: &Y%-3s   &c|&n\r\n"

		"&c|&y   Show Autoexit: &Y%-3s    "
		"&y             Deaf: &Y%-3s    &n"
		"&y     Wimp Level: &Y%-3s   &c|&n\r\n"

		"&c|&y     OOC Channel: &Y%-3s    "
		"&y   Newbie channel: &Y%-3s    &n"
		"&yObscene channel: &Y%-3s   &c|&n\r\n"

		"&c|&y      Autoassist: &Y%-3s    "
		"&y    Autosacrifice: &Y%-3s    &n"
		"&y    Autolooting: &Y%-3s   &c|&n\r\n"

		"&c|&y        Autogold: &Y%-3s    "
		"&y    Autosplitting: &Y%-3s    &n"
		"&y       Meterbar: &Y%-3s   &c|&n\r\n"

		"&c|&y     Color Level: &Y%-8s   "
		"&y      RP mode: &Y%-3s    &n"
		"&y          Email: &Y%-3s   &c|&n\r\n"
		
		"&c|&y    Tip messages: &Y%-3s    "
		"&y     Sing channel: &Y%-3s    &n"
		"&y    Skill gains: &Y%-3s   &c|&n\r\n"

		"&c|&y      Timestamps: &Y%-3s    "
		"&y     real skill %%: &Y%-3s    &n"
		"&y        Pose ID: &Y%-3s   &c|&n\r\n", printbuf,

		ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
		ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
		ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),

		ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
		ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
		YESNO(PRF_FLAGGED(ch, PRF_QUEST)),

		ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
		ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
		YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

		ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
		YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
		printbuf2,

		ONOFF(!PRF_FLAGGED(ch, PRF_NOSOC)),
		ONOFF(!PRF_FLAGGED(ch, PRF_NONEWBIE)),
		ONOFF(!PRF_FLAGGED(ch, PRF_NOOBSCENE)),

		YESNO(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
		YESNO(PRF_FLAGGED(ch, PRF_AUTOSAC)),
		YESNO(PRF_FLAGGED(ch, PRF_AUTOLOOT)),

		YESNO(PRF_FLAGGED(ch, PRF_AUTOGOLD)),
		YESNO(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
		ONOFF(PRF_FLAGGED(ch, PRF_METERBAR)),
 
		ctypes[COLOR_LEV(ch)],
		ICOOC(SESS_FLAGGED(ch, SESS_IC)),
		YESNO(PRF_FLAGGED(ch, PRF_EMAIL)),

		ONOFF(PRF_FLAGGED(ch, PRF_TIPCHANNEL)),
		ONOFF(!PRF_FLAGGED(ch, PRF_NOSING)),
		ONOFF(PRF_FLAGGED(ch, PRF_SKILLGAINS)),

		ONOFF(PRF_FLAGGED(ch, PRF_TIMESTAMPS)),
		ONOFF(PRF_FLAGGED(ch, PRF_REALSKILLS)),
		ONOFF(PRF_FLAGGED(ch, PRF_POSEID))
		);


	strcat(printbuf, "&c'=========================================================================='&n\r\n");
	send_to_char(printbuf, ch);

	release_buffer(printbuf);
	release_buffer(printbuf2);

}


struct tog_struct {
	const char *cmd;
	bitvector_t rights;
	int subcmd;
} tog_fields[] = {
	{ "autoassist",	RIGHTS_MEMBER			, SCMD_AUTOASSIST},	/* 0 */
	{	"autoexit"	,	RIGHTS_MEMBER			, SCMD_AUTOEXIT },
	{	"autogold"	,	RIGHTS_MEMBER			, SCMD_AUTOGOLD },
	{	"autoloot"	, RIGHTS_MEMBER			, SCMD_AUTOLOOT },
	{	"autosac"		, RIGHTS_MEMBER			, SCMD_AUTOSAC },
	{ "autosplit"	, RIGHTS_MEMBER			, SCMD_AUTOSPLIT },	/* 5 */
	{ "brief"			, RIGHTS_MEMBER			, SCMD_BRIEF },
	{ "compact"		, RIGHTS_MEMBER			, SCMD_COMPACT },
	{ "email"			, RIGHTS_MEMBER			, SCMD_EMAIL },
	{ "gossip"		, RIGHTS_MEMBER			, SCMD_NOSOC },
	{ "holylight"	, RIGHTS_IMMORTAL		, SCMD_HOLYLIGHT },	/* 10 */
	{ "ic"				, RIGHTS_MEMBER			, SCMD_IC },
	{ "meterbar"	, RIGHTS_MEMBER			, SCMD_METERBAR },
	{ "newbie"		, RIGHTS_MEMBER			, SCMD_NONEWBIE },
	{ "nohassle"	, RIGHTS_IMMORTAL		, SCMD_NOHASSLE },
	{ "obscene"   , RIGHTS_MEMBER     , SCMD_NOOBSCENE  },	/* 15 */
	{ "quest"			, RIGHTS_MEMBER			, SCMD_QUEST },		
	{ "repeat"		, RIGHTS_MEMBER			, SCMD_NOREPEAT },
	{ "rp"				, RIGHTS_MEMBER			, SCMD_IC },
	{ "roleplay"	, RIGHTS_MEMBER			, SCMD_IC },
	{ "roomflags"	, RIGHTS_IMMORTAL		, SCMD_ROOMFLAGS },
	{ "social"		, RIGHTS_MEMBER			, SCMD_NOSOC },			/* 20 */
	{ "shout"			, RIGHTS_MEMBER			, SCMD_DEAF },
	{ "sing"			, RIGHTS_MEMBER			, SCMD_NOSING },
	{ "slowns"		, RIGHTS_IMPLEMENTOR, SCMD_SLOWNS },
	{ "summon"		, RIGHTS_MEMBER			, SCMD_NOSUMMON },	/* 25 */
	{ "tell"			, RIGHTS_MEMBER			, SCMD_NOTELL },
	{ "tips"			, RIGHTS_MEMBER			, SCMD_NOTIPCHAN },	
	{ "trackthru"	, RIGHTS_IMPLEMENTOR, SCMD_TRACK },
	{ "wizchan"		, RIGHTS_IMMORTAL		, SCMD_NOWIZ },
	{ "vnums"			, RIGHTS_IMMORTAL		, SCMD_SHOWVNUMS },	/* 30 */
	{ "skillgains", RIGHTS_MEMBER			, SCMD_SKILLGAINS },
	{ "timestamps", RIGHTS_MEMBER			, SCMD_TIMESTAMPS },
	{ "realskills", RIGHTS_MEMBER			, SCMD_REALSKILLS },
	{ "poseid"		, RIGHTS_MEMBER			, SCMD_POSEID },
	{ "\n"				, RIGHTS_NONE				, 0 }
};


ACMD(do_toggle)
{
	int i;
	char *printbuf, *args;

	if (IS_NPC(ch))
		return;

	args = get_buffer(MAX_NAME_LENGTH);
	one_argument(argument, args);

	if (!*argument || !*args) {
		disp_toggles(ch);
		release_buffer(args);
		return;
	}
	
	printbuf = get_buffer(MAX_STRING_LENGTH);

	for (i=0; *(tog_fields[i].cmd) != '\n'; i++)
		if (isname(args, tog_fields[i].cmd)) break;
	
	if ((tog_fields[i].rights == RIGHTS_NONE) || (!(GOT_RIGHTS(ch, tog_fields[i].rights)) && !IS_IMPL(ch))) {
		sprintf(printbuf, "Could not toggle %s.\r\n", args);
		send_to_char(printbuf, ch);
		release_buffer(args);
		release_buffer(printbuf);
		return;
	}
	
	gen_tog(ch, tog_fields[i].subcmd);

	release_buffer(args);
	release_buffer(printbuf);
}


int	num_of_cmds;


void sort_commands(void)
{
	int a, b, tmp;

	num_of_cmds = 0;

	/*
	 * first, count commands (num_of_commands is actually one greater than the
	 * number of commands; it inclues the '\n'.
	 */
	/* while (*cmd_info[num_of_cmds].command != '\n') */
	while (*complete_cmd_info[num_of_cmds].command != '\n')
		num_of_cmds++;

	/*check if there was an old sort info.. then free it -- aedit -- M. Scott*/
	if (cmd_sort_info) free(cmd_sort_info);

	/* create data array */
	CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

	/* initialize it */
	for (a = 1; a < num_of_cmds; a++) {
		cmd_sort_info[a].sort_pos = a;
		cmd_sort_info[a].is_social = (complete_cmd_info[a].command_pointer == do_action);
	}
	/* the infernal special case */
	cmd_sort_info[find_command("insult")].is_social = TRUE;

	/* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
	for (a = 1; a < num_of_cmds - 1; a++) {
		for (b = a + 1; b < num_of_cmds; b++) {
			/* if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
								 cmd_info[cmd_sort_info[b].sort_pos].command) > 0) { */
			if (strcmp(complete_cmd_info[cmd_sort_info[a].sort_pos].command,
					complete_cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
				tmp = cmd_sort_info[a].sort_pos;
				cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
				cmd_sort_info[b].sort_pos = tmp;
			}
		}
	}
}



ACMD(do_commands)
{
	char *inputarg = get_buffer(MAX_INPUT_LENGTH);
	char *printbuf;
	int no, i, cmd_num;
	int wizhelp = 0, socials = 0;
	struct char_data *vict;

	one_argument(argument, inputarg);

	if (*inputarg) {
		if (!(vict = get_char_vis(ch, inputarg, NULL, FIND_CHAR_WORLD, 1)) || IS_NPC(vict)) {
			send_to_char("Who is that?\r\n", ch);
			release_buffer(inputarg);
			return;
		}
		if (compare_rights(ch, vict) == -1) {
			send_to_char("You can't see the commands of people with more rights than you.\r\n", ch);
			release_buffer(inputarg);
			return;
		}
	} else
		vict = ch;

	if (subcmd == SCMD_SOCIALS)
		socials = 1;
	else if (subcmd == SCMD_WIZHELP)
		wizhelp = 1;

	printbuf = get_buffer(MAX_STRING_LENGTH);

	sprintf(printbuf, "The following %s%s are available to %s:\r\n",
					wizhelp ? "privileged " : "",
					socials ? "socials" : "commands",
					vict == ch ? "you" : GET_NAME(vict));

	/* cmd_num starts at 1, not 0, to remove 'RESERVED' */
	for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
		i = cmd_sort_info[cmd_num].sort_pos;

		if (complete_cmd_info[i].rights == RIGHTS_NONE)
			continue;

		if (!GOT_RIGHTS(vict, complete_cmd_info[i].rights))
			continue;

		if (!wizhelp && socials != cmd_sort_info[i].is_social)
			continue;

		if ((complete_cmd_info[i].rights != RIGHTS_MEMBER) != wizhelp)
			continue;

		sprintf(printbuf, "%s%-13.13s%s", printbuf, complete_cmd_info[i].command, no++ % 6 == 0 ? "\r\n" : "");
	}

	strcat(printbuf, "\r\n");
	page_string(ch->desc, printbuf, 1);
	release_buffer(inputarg);
	release_buffer(printbuf);
}


ACMD(do_calendar)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	int month;

	sprintf(printbuf, "MONTHS OF THE YEAR:\r\n");

	for (month=0;month<13;month++) {
		sprintf(printbuf, "%s%s%2d. %s&n\r\n", printbuf, ((month == time_info.month)?"&Y":""), month+1, month_name[(int) month]);
	}

	page_string(ch->desc, printbuf, TRUE);

}


/*
 * Need to redefine these here, since they are only defined in parser.c.
 * This is to enable nice wrapping of the tip messages.  We could of
 * course do this manually, but that would not be very much fun, would it?
 * This would later enable us to read these from the database instead.
 */
#define	SCR_WIDTH 79
#define	RETURN_CHAR '|'

void send_tip_message(void)
{
	char *printbuf, *printbuf2;
	struct char_data *i, *next_char;
	int to_sleeping = 1;
	static int tiprow;
	char title[40];

	/* Added sanity check. */
	if (top_of_tip_table <= 0)
		return;

	if (tiprow < top_of_tip_table)
		tiprow++;
	else
		tiprow = 0;

	printbuf = get_buffer(MAX_STRING_LENGTH);
	printbuf2 = get_buffer(MAX_STRING_LENGTH);

	sprintf(title, "[&WTIP&n] %s", tip_table[tiprow].title);
	sprintf(printbuf, "%s", tip_table[tiprow].tip);
	clean_up(printbuf);

	sprintf(printbuf2, "\r\n%s\r\n&y%s&n\r\n", title, printbuf);

	/*characters */
	for (i = character_list; i; i = next_char) {
		next_char = i->next;
		if (!IS_NPC(i) && SENDOK(i) && PRF_FLAGGED(i, PRF_TIPCHANNEL) && GOT_RIGHTS(i, tip_table[tiprow].rights)) {
			send_to_char(printbuf2, i);
		}
	}

	release_buffer(printbuf);
	release_buffer(printbuf2);
}


/*
 * Check for tasks, overdue and pending. Added 2002-01-14 by Torgny Bjers.
 */
void check_pending_tasks(struct char_data *ch, int subcmd)
{
	MYSQL_RES *events;
	MYSQL_ROW row;
	int rec_count = 0, i;
	unsigned long *fieldlength;
	char *esc_name;
	char *printbuf, *printbuf2;

	SQL_MALLOC(GET_NAME(ch), esc_name);
	SQL_ESC(GET_NAME(ch), esc_name);

	switch (subcmd) {
		case SCMD_TASKS:
			if (!(events = mysqlGetResource(TABLE_EVENTS, "SELECT REPLACE(title, '&', '&&') AS title, REPLACE(short_desc, '&', '&&') AS short_desc, enddate, assigned FROM %s WHERE assigned LIKE '%%%s%%' AND Active = 1 AND Approved = 1 AND (enddate < ADDDATE(NOW(), INTERVAL 7 DAY) OR startdate BETWEEN ADDDATE(NOW(), INTERVAL 1 DAY) AND ADDDATE(NOW(), INTERVAL %d DAY)) ORDER BY enddate ASC;", TABLE_EVENTS, esc_name, tasks_interval))) {
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading tasks.");
				SQL_FREE(esc_name);
				return;
			}
			break;
		case SCMD_EVENTS:
			if (!(events = mysqlGetResource(TABLE_EVENTS, "SELECT REPLACE(title, '&', '&&') AS title, REPLACE(short_desc, '&', '&&') AS short_desc, startdate FROM %s WHERE Assigned = '' AND Active = 1 AND Approved = 1 AND (startdate BETWEEN ADDDATE(NOW(), INTERVAL 1 DAY) AND ADDDATE(NOW(), INTERVAL %d DAY)) ORDER BY startdate ASC;", TABLE_EVENTS, events_interval))) {
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading events.");
				SQL_FREE(esc_name);
				return;
			}
			break;
		default:
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached default case in check_pending_tasks (%d).", __LINE__);
			SQL_FREE(esc_name);
			return;
			break;
	}

	SQL_FREE(esc_name);

	rec_count = mysql_num_rows(events);

	if(rec_count == 0)
		return;

	printbuf = get_buffer(MAX_STRING_LENGTH);
	printbuf2 = get_buffer(MAX_STRING_LENGTH);

	sprintf(printbuf, "\007\007\r\n&REVENT NOTIFICATION:&n\r\n");

	for(i = 0; i < rec_count; i++)
	{
		row = mysql_fetch_row(events);
		fieldlength = mysql_fetch_lengths(events);
		switch (subcmd) {
			case SCMD_TASKS:
				sprintf(printbuf2, "%s (%s)", row[1], (row[3]?row[3]:"None"));
				break;
			case SCMD_EVENTS:
				sprintf(printbuf2, "%s", row[1]);
				break;
			default:
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached default case in check_pending_tasks (%d).", __LINE__);
				return;
				break;
		}
		clean_up(printbuf2);
		sprintf(printbuf, "%s\r\n&r[&Y%s %s&r]&W %s&n\r\n&y%s&n\r\n", printbuf, event_modes[subcmd], row[2], row[0], printbuf2);
	}

	page_string(ch->desc, printbuf, TRUE);

	/* Free up result sets to conserve memory. */
	mysql_free_result(events);

	release_buffer(printbuf);
	release_buffer(printbuf2);
}

#undef SCR_WIDTH
#undef RETURN_CHAR


ACMD(do_whois)
{
	char *display;
	char *inputarg = get_buffer(MAX_INPUT_LENGTH);
	struct char_data *vict = NULL;
	const char *titles[] = {
		"&gImmortal&n",
		"&GQuestor&n",
		"&GRPStaff&n",
		"&GBuilder&n",
		"&GAdmin&n",
		"&CDeveloper&n",
		"&CHeadBuilder&n",
		"&WImplementor&n",
		"&WOwner&n"
	};

	one_argument(argument, inputarg);
	if (!*inputarg) {
		send_to_char("For whom do you wish to search?\r\n", ch);
		release_buffer(inputarg);
		return;
	}
	CREATE(vict, struct char_data, 1);
	clear_char(vict);
	CREATE(vict->player_specials, struct player_special_data, 1);
	if (load_char(inputarg, vict) <  0) {
		send_to_char("There is no such player.\r\n", ch);
		free_char(vict);
		release_buffer(inputarg);
		return;
	}
	display = get_buffer(MAX_STRING_LENGTH);

	sprintf(display, "&c=====[&C%s&c]=============================================\r\n", GET_NAME(vict));
	if (IS_IMMORTAL(vict))
		sprintf(display+strlen(display), "%s\r\n", titles[imm_titles(vict)]);

	sprintf(display+strlen(display), "&W%s &w%s&n\r\n", GET_NAME(vict), GET_TITLE(vict));
	sprintf(display+strlen(display), "&YLast on: &y%s&n\r\n", ctime(&vict->player.time.logon));

	/* If they have no LDESC set, we create one for them. */
	if (!vict->player.long_descr || !*vict->player.long_descr) {
		if (GET_RACE(vict) < RACE_DRAGON)
			sprintf(display+strlen(display), "&y%s is a %s %s with %s %s hair and %s eyes,\r\nand %s skin is of %s %s tone.&n\r\n", GET_NAME(vict), genders[(int)GET_SEX(vict)], race_list[(int)GET_RACE(vict)].name, 
																				hair_style[(int)GET_HAIRSTYLE(vict)], hair_color[(int)GET_HAIRCOLOR(vict)], eye_color[(int)GET_EYECOLOR(vict)], HSHR(vict), AN(skin_tone[(int)GET_SKINTONE(vict)]), skin_tone[(int)GET_SKINTONE(vict)]);
		else
			sprintf(display+strlen(display), "&y%s appears to be %s %s.&n\r\n", GET_NAME(vict), AN(race_list[(int)GET_RACE(vict)].name), race_list[(int)GET_RACE(vict)].name);
	} else {
		sprintf(display+strlen(display), "&y%s&n", GET_LDESC(vict));
	}
	
	sprintf(display+strlen(display), "\r\n&YContact Information:&n\r\n&y%s&n\r\n", GET_CONTACTINFO(vict) ? GET_CONTACTINFO(vict) : "Not set");

	send_to_char(display, ch);
	release_buffer(display);
	free_char(vict);
	release_buffer(inputarg);
}


ACMD(do_attributes)
{
	char *printbuf;

	if (IS_NPC(ch)) {
		send_to_char("When are you mobs going to learn?!\r\n", ch);
		return;
	}

	printbuf = get_buffer(MAX_STRING_LENGTH);

	sprintf(printbuf, "&c.----------[ &CPLAYER ATTRIBUTES &c]----------.\r\n");
	sprintf(printbuf, "%s|  &nStrength     : &Y%3d   &nAgility    : &Y%3d  &c|\r\n", printbuf, GET_STRENGTH(ch) / 100, GET_AGILITY(ch) / 100);
	sprintf(printbuf, "%s|  &nPrecision    : &Y%3d   &nPerception : &Y%3d  &c|\r\n", printbuf, GET_PRECISION(ch) / 100, GET_PERCEPTION(ch) / 100);
	sprintf(printbuf, "%s|  &nHealth       : &Y%3d   &nWill Power : &Y%3d  &c|\r\n", printbuf, GET_HEALTH(ch) / 100, GET_WILLPOWER(ch) / 100);
	sprintf(printbuf, "%s|  &nIntelligence : &Y%3d   &nCharisma   : &Y%3d  &c|\r\n", printbuf, GET_INTELLIGENCE(ch) / 100, GET_CHARISMA(ch) / 100);
	sprintf(printbuf, "%s|  &nLuck         : &Y%3d   &nEssence    : &Y%3d  &c|\r\n", printbuf, GET_LUCK(ch) / 100, GET_ESSENCE(ch) / 100);
	sprintf(printbuf, "%s&c'-----------------------------------------'&n\r\n", printbuf);
	sprintf(printbuf, "%sUse SCORE for the full character sheet.\r\n", printbuf);

	page_string(ch->desc, printbuf, TRUE);
	release_buffer(printbuf);
}

/* 
 * Ch: You
 * Victim: The person you want to be notified about.
 * Returns true if their name appears on your list of people,
 * and you can see them.
 */
bool find_notify(struct char_data *ch, struct char_data *vict)
{
	int i;

	if (ch == NULL || vict == NULL || !ch->desc || !vict->desc || ch == vict)
		return (FALSE);

	for (i = 0; i < MAX_NOTIFY; i++)
		if (GET_NOTIFY(ch, i) == GET_IDNUM(vict) && CAN_SEE(ch, vict))
			return (TRUE);

	return (FALSE);
}


ACMD(do_notifylist)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	int vict_id = 0;
	bool found = FALSE;
	int i;

	if (ch->desc == NULL)
		return;

	if (!*argument) {
		for (i = 0; i < MAX_NOTIFY; i++) {
			if (GET_NOTIFY(ch, i) > 0 && get_name_by_id(GET_NOTIFY(ch, i)) != NULL) {
				send_to_charf(ch, "%s\r\n", CAP(strdup(get_name_by_id(GET_NOTIFY(ch, i)))));
				found = TRUE;
			}
		}
		send_to_charf(ch, "%sUsage: %s [ add <name> | remove <name> ]\r\n", found ? "\r\n" : "", CMD_NAME);
		return;
	}

	skip_spaces(&argument);

	two_arguments(argument, arg1, arg2);

	if (!(strn_cmp(arg1, "remove", 6))) {
		if (!*arg2)
			send_to_charf(ch, "Usage: %s [ add <name> | remove <name> ]\r\n", CMD_NAME);
		else if ((vict_id = get_id_by_name(arg2)) == NOBODY)
			send_to_char(NOPERSON, ch);
		else {
			for (i = 0; i < MAX_NOTIFY; i++) {
				if (GET_NOTIFY(ch, i) == vict_id) {
					GET_NOTIFY(ch, i) = 0;
					if (!found)
						send_to_charf(ch, "Removed %s from your notify list.\r\n", CAP(strdup(get_name_by_id(vict_id))));
					found = TRUE;
				}
			}
			if (!found)
				send_to_charf(ch, "No such person on your notify list.\r\n");
		}
	} else if (!(strn_cmp(arg1, "add", 3))) {
		if ((vict_id = get_id_by_name(arg2)) == NOBODY)
			send_to_char(NOPERSON, ch);
		else if (GET_IDNUM(ch) == vict_id)
			send_to_charf(ch, "You should know when you log in, don't you think?\r\n");
		else {
			for (i = 0; i < MAX_NOTIFY; i++) {
				if (GET_NOTIFY(ch, i) == vict_id) {
					found = TRUE;
					send_to_charf(ch, "You are already notified when %s logs in.\r\n", CAP(strdup(get_name_by_id(vict_id))));
				}
			}
			if (!found) {
				for (i = 0; i < MAX_NOTIFY; i++) {
					if (GET_NOTIFY(ch, i) == 0) {
						GET_NOTIFY(ch, i) = vict_id;
						send_to_charf(ch, "You will now be notified when %s logs in.\r\n", CAP(strdup(get_name_by_id(vict_id))));
						found = TRUE;
						break;
					}
				}
				if (!found) {
					for (i = 0; i < MAX_NOTIFY; i++)
						if (GET_NOTIFY(ch, i - 1))
							GET_NOTIFY(ch, i - 1) = GET_NOTIFY(ch, i);
					GET_NOTIFY(ch, MAX_NOTIFY - 1) = vict_id;
					send_to_charf(ch, "You will now be notified when %s logs in.\r\n", CAP(strdup(get_name_by_id(vict_id))));
				}
			}
		}
	} else
		send_to_charf(ch, "Usage: %s [ add <name> | remove <name> ]\r\n", CMD_NAME);
}
