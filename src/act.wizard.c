/* ************************************************************************
*		File: act.wizard.c                                  Part of CircleMUD *
*	 Usage: Player-level god commands and other goodies                     *
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
/* $Id: act.wizard.c,v 1.179 2004/05/10 17:31:14 cheron Exp $ */

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
#include "house.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "genolc.h"
#include "genwld.h"
#include "genzon.h"
#include "genmob.h"
#include "dg_scripts.h"
#include "loadrooms.h"
#include "specset.h"
#include "characters.h"
#include "roleplay.h"
#include "commands.h"

/*	 external vars  */
extern FILE *player_fl;
extern socket_t mother_desc;
extern ush_int port;
extern int boot_high;
extern int total_hotboots;
extern time_t boot_time;
extern time_t copyover_time;
extern obj_rnum top_of_objt;
extern mob_rnum top_of_mobt;
extern zone_rnum top_of_zone_table;
extern int top_of_trigt;
extern int top_of_p_table;
extern int top_of_socialt;
extern int top_of_aquestt;
extern int top_of_houset;
extern int top_shop;
extern long g_lNumAssemblies;
extern int circle_shutdown, circle_reboot;
extern int circle_restrict;
extern int load_into_inventory;
extern int buf_switches, buf_largecount, buf_overflows;
extern int is_name(const char *str, const char *namelist);
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern struct house_data *house_index;
extern struct shop_data *shop_index;
extern struct attack_hit_type attack_hit_text[];
extern struct index_data **trig_index;
extern struct social_messg *soc_mess_list;
extern struct player_index_element *player_table;
struct char_data *find_char(int n);
extern int    top_of_commandt;
extern struct command_info *cmd_info;
extern struct command_list_info command_list[];

extern time_t boot_high_time;
extern int autosave_time;
extern int crash_file_timeout;

/* (FIDO) Added allowed extern variables here for ACMD(do_world) */
extern int pk_allowed;
extern int sleep_allowed;
extern int charm_allowed;
extern int summon_allowed;
extern int roomaffect_allowed;
extern int rent_file_timeout;
extern int bautosave_time;
extern int min_rent_cost;
extern int max_obj_save;
extern int holler_move_cost;
extern int max_exp_gain;
extern int max_exp_loss;
extern int max_npc_corpse_time;
extern int max_pc_corpse_time;
extern int idle_void;
extern int idle_rent_time;

extern int max_playing;
extern int max_filesize;
extern int max_bad_pws;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern room_vnum donation_room_1;
extern room_vnum donation_room_2;
extern room_vnum donation_room_3;
extern int siteok_everyone;
extern int dts_are_dumps;
extern int load_into_inventory;
extern int track_through_doors;
extern int free_rent;
extern int auto_save;
extern int bautosave;
extern int auto_pwipe;
extern int selfdelete_fastwipe;
extern int nameserver_is_slow;
extern int zone_reset;
extern int magic_enabled;

extern int bautosave;
extern int mini_mud;
extern int no_rent_check;
extern int no_specials;

extern const char *lib_dir;
extern const char *mysql_db;

struct skill_info_type skill_info[TOP_SKILL_DEFINE + 1];
extern sh_int mortal_start_room[NUM_STARTROOMS +1];

extern int in_copyover;

/* for chars */
extern int top_of_race_list;
extern struct race_list_element *race_list;
extern int top_of_culture_list;
extern struct culture_list_element *culture_list;
extern const char *npc_class_types[];
extern const char *npc_race_types[];
extern char *combat_skills[];

/* extern functions */
int	find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
int	parse_class(struct char_data *ch, char arg);
int	parse_menu(char arg);
int	save_all(void);
int	zdelete_check(int zone);
void appear(struct char_data *ch);
void assemblyListToChar(struct char_data *pCharacter);
void do_start(struct char_data *ch);
void find_uid_name(char *uid, char *name);
void hcontrol_list_houses(struct char_data *ch);
void ispell_done(void);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where, int show);
void Read_Invalid_List(void);
void roll_real_abils(struct char_data *ch);
void show_shops(struct char_data *ch, char *value);
void load_config(void);
long get_ptable_by_name(char *name);
void extract_char_final(struct char_data *ch);
bitvector_t	asciiflag_conv(char *flag);
void master_parser (char *parsebuf, struct char_data *ch, struct room_data *room, struct obj_data *obj);
void clean_up (char *in);
void remove_player(int pfilepos);
void race_menu(struct char_data *ch, bool full, bool error);
void culture_menu(struct char_data *ch, bool full, bool error);
void write_aliases(struct char_data *ch);
int	Valid_Name(char *newname);
void clean_players(int preview);
int	_parse_name(char *arg, char *name);
void save_all_guilds(void);
void save_commands(void);
int update_attribs(struct char_data *ch);
const char *display_config_vars (void);
void write_mud_date_to_file(void);

/* local functions */
bool check_same_room(int zone, int subcmd, room_rnum rnum);
int newbie_equip(struct char_data *ch);
int set_contact(struct char_data *ch, char *name);
int	perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
room_rnum	find_target_room(struct char_data *ch, char *rawroomstr);
void do_stat_character(struct char_data *ch, struct char_data *k);
void do_stat_object(struct char_data *ch, struct obj_data *j);
void do_stat_room(struct char_data *ch);
void do_zstat_room(struct char_data *ch, zone_rnum rnum);
void perform_immort_invis(struct char_data *ch, bitvector_t rights);
void perform_immort_vis(struct char_data *ch);
void print_zone_to_buf(char *bufptr, zone_rnum zone);
void send_to_imms(char *msg);
void stop_snooping(struct char_data *ch);
void disp_bitvector_menu(struct descriptor_data *d, const char *names[], int length);
void disp_attributes_menu(struct descriptor_data *d, const char *attribute[], int length);
void do_stat_command(struct char_data *ch, int command);

ACMD(do_send);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_stat);
ACMD(do_shutdown);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_purge);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_world);
ACMD(do_syslog);
ACMD(do_advance);
ACMD(do_restore);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
ACMD(do_show);
ACMD(do_set);
ACMD(do_file);
ACMD(do_copyover);
ACMD(do_newbie);
ACMD(do_xname);
ACMD(do_approve);
ACMD(do_rename);
ACMD(do_pending);
ACMD(do_xsyslog);

int set_contact(struct char_data *ch, char *name)
{
  if (!strcmp(name, "off")) {
    free(GET_CONTACT(ch));
		GET_CONTACT(ch) = NULL;
    return TRUE;
  }

  if (get_id_by_name(name) < 0)
    return FALSE;

  if (GET_CONTACT(ch) != NULL)
    free(GET_CONTACT(ch));

  GET_CONTACT(ch) = str_dup(name);
  return TRUE;
}

ACMD(do_send)
{
	struct char_data *vict;
  int result;

	half_chop(argument, arg, buf);

	if (!*argument || !*arg || !*buf) {
		send_to_char("Send what to who?\r\n", ch);
		return;
	}
	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD, 1))) {
		send_to_char(NOPERSON, ch);
		return;
	}
	send_to_char(buf, vict);
	send_to_char("\r\n", vict);
  if (IS_MORTAL(vict)) {
    if (GET_CONTACT(vict) != NULL) {
      if (strcmp(GET_CONTACT(vict), GET_NAME(ch)))
        result = set_contact(vict, GET_NAME(ch));
    } else
      result = set_contact(vict, GET_NAME(ch));
  }

	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char("Sent.\r\n", ch);
	else {
		sprintf(buf2, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
		send_to_char(buf2, ch);
	}
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum	find_target_room(struct char_data *ch, char *rawroomstr)
{
	room_rnum location = NOWHERE;
	char roomstr[MAX_INPUT_LENGTH];

	one_argument(rawroomstr, roomstr);

	if (!*roomstr) {
		send_to_char("You must supply a room number or name.\r\n", ch);
		return (NOWHERE);
	}

	if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
		if ((location = real_room((room_vnum)atoi(roomstr))) == NOWHERE) {
			send_to_char("No room exists with that number.\r\n", ch);
			return (NOWHERE);
		}
	 } else {
		struct char_data *target_mob;
		struct obj_data *target_obj;
		char *mobobjstr = roomstr;
		int num;

		num = get_number(&mobobjstr);
		if ((target_mob = get_char_vis(ch, mobobjstr, &num, FIND_CHAR_WORLD, 1)) != NULL) {
			if ((location = IN_ROOM(target_mob)) == NOWHERE) {
				send_to_char("That character is currently lost.\r\n", ch);
				return (NOWHERE);
			}
		} else if ((target_obj = get_obj_vis(ch, mobobjstr, &num)) != NULL) {
			if (IN_ROOM(target_obj) != NOWHERE)
				location = IN_ROOM(target_obj);
			else if (target_obj->carried_by && IN_ROOM(target_obj->carried_by) != NOWHERE)
				location = IN_ROOM(target_obj->carried_by);
			else if (target_obj->worn_by && IN_ROOM(target_obj->worn_by) != NOWHERE)
				location = IN_ROOM(target_obj->worn_by);

			if (location == NOWHERE) {
				send_to_char("That object is currently not in a room.\r\n", ch);
				return (NOWHERE);
			}
		}

		if (location == NOWHERE) {
			send_to_char("Nothing exists by that name.\r\n", ch);
			return (NOWHERE);
		}
	}

	/* a location has been found -- if you're >= GRGOD, no restrictions. */
	if (IS_GRGOD(ch))
		return (location);

	if (ROOM_FLAGGED(location, ROOM_GODROOM))
		send_to_char("You are not godly enough to use that room!\r\n", ch);
	else if (ROOM_FLAGGED(location, ROOM_PRIVATE) && world[location].people && world[location].people->next_in_room)
		send_to_char("There's a private conversation going on in that room.\r\n", ch);
	else if (ROOM_FLAGGED(location, ROOM_HOUSE) && !house_can_enter(ch, GET_ROOM_VNUM(location)))
		send_to_char("That's private property -- no trespassing!\r\n", ch);
	else
		return (location);

	return (NOWHERE);
}



ACMD(do_at)
{
	char command[MAX_INPUT_LENGTH];
	room_rnum location, original_loc;

	half_chop(argument, buf, command);
	if (!*buf) {
		send_to_char("You must supply a room number or a name.\r\n", ch);
		return;
	}

	if (!*command) {
		send_to_char("What do you want to do there?\r\n", ch);
		return;
	}

	if ((location = find_target_room(ch, buf)) < 0)
		return;

	/* a location has been found. */
	original_loc = IN_ROOM(ch);
	char_from_room(ch);
	char_to_room(ch, location);
	command_interpreter(ch, command);

	/* check if the char is still there */
	if (IN_ROOM(ch) == location) {
		char_from_room(ch);
		char_to_room(ch, original_loc);
	}
}


ACMD(do_goto)
{
	room_rnum location;

	if ((location = find_target_room(ch, argument)) < 0)
		return;

	if (GET_TRAVELS(ch) && GET_TRAVELS(ch)->tout)
		strcpy(buf, GET_TRAVELS(ch)->tout);
	else
		strcpy(buf, travel_defaults[TRAV_TOUT]);

	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	char_from_room(ch);
	char_to_room(ch, location);

	if (GET_TRAVELS(ch) && GET_TRAVELS(ch)->tin)
		strcpy(buf, GET_TRAVELS(ch)->tin);
	else
		strcpy(buf, travel_defaults[TRAV_TIN]);

	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	look_at_room(ch, 0);
}



ACMD(do_trans)
{
	struct descriptor_data *i;
	struct char_data *victim;

	one_argument(argument, buf);
	if (!*buf)
		send_to_char("Whom do you wish to transfer?\r\n", ch);
	else if (str_cmp("all", buf)) {
		if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD, 1)))
			send_to_char(NOPERSON, ch);
		else if (victim == ch)
			send_to_char("That doesn't make much sense, does it?\r\n", ch);
		else {
			if ((compare_rights(ch, victim) == -1) && !IS_NPC(victim)) {
				send_to_char("Go transfer someone your own size.\r\n", ch);
				return;
			}
			act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
			char_from_room(victim);
			char_to_room(victim, IN_ROOM(ch));
			act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
			act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
			look_at_room(victim, 0);
		}
	} else {                        /* Trans All */
		if (!IS_GRGOD(ch)) {
			send_to_char("I think not.\r\n", ch);
			return;
		}

		for (i = descriptor_list; i; i = i->next)
			if ((STATE(i) == CON_PLAYING || STATE(i) == CON_COPYOVER) && i->character && i->character != ch) {
				victim = i->character;
				if (compare_rights(ch, victim) <= 0)
					continue;
				act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
				char_from_room(victim);
				char_to_room(victim, IN_ROOM(ch));
				act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
				act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
				look_at_room(victim, 0);
			}
		send_to_char(OK, ch);
	}
}



ACMD(do_teleport)
{
	struct char_data *victim;
	room_rnum target;

	two_arguments(argument, buf, buf2);

	if (!*buf)
		send_to_char("Whom do you wish to teleport?\r\n", ch);
	else if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD, 1)))
		send_to_char(NOPERSON, ch);
	else if (victim == ch)
		send_to_char("Use 'goto' to teleport yourself.\r\n", ch);
	else if (compare_rights(ch, victim) <= 0)
		send_to_char("Maybe you shouldn't do that.\r\n", ch);
	else if (!*buf2)
		send_to_char("Where do you wish to send this person?\r\n", ch);
	else if ((target = find_target_room(ch, buf2)) >= 0) {
		send_to_char(OK, ch);
		act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
		char_from_room(victim);
		char_to_room(victim, target);
		act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
		act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
		look_at_room(victim, 0);
	}
}


void do_stat_room(struct char_data *ch)
{
	struct extra_descr_data *desc;
	struct room_data *rm = &world[IN_ROOM(ch)];
	int i, found;
	struct obj_data *j;
	struct char_data *k;

	sprintf(buf, "Room name: &y%s&n\r\n", rm->name);
	send_to_char(buf, ch);

	sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
	sprintf(buf, "Zone: [&c%3d&n], VNum: [&c%5d&n], RNum: [&c%5d&n], Type: &c%s&n\r\n",
					zone_table[rm->zone].number, rm->number,
					IN_ROOM(ch), buf2);
	send_to_char(buf, ch);

	sprinttype(rm->magic_flux, magic_flux_types, buf2, sizeof(buf2));
	sprintf(buf, "Flux Type: &c%s&n, Available Flux: &c%d&n\r\n",
					buf2, rm->available_flux);
	send_to_char(buf, ch);

	sprintbit(rm->room_flags, room_bits, buf2, sizeof(buf2));
	sprintf(buf, "SpecProc: &c%s&n, Flags: &c%s&n\r\n",
					(rm->func == NULL) ? "None" : "Exists", buf2);
	send_to_char(buf, ch);

	sprintbit(rm->resources.resources, resource_bits, buf2, sizeof(buf2));
	sprintf(buf, "Resources : &c%s&n\r\n", buf2);
	sprintf(buf + strlen(buf), "Ore  : &y%3d (%3d)&n Gems  : &y%3d (%3d)&n\r\n", rm->resources.current[RESOURCE_ORE], rm->resources.max[RESOURCE_ORE], rm->resources.current[RESOURCE_GEMS], rm->resources.max[RESOURCE_GEMS]);
	sprintf(buf + strlen(buf), "Wood : &y%3d (%3d)&n Stone : &y%3d (%3d)&n\r\n", rm->resources.current[RESOURCE_WOOD], rm->resources.current[RESOURCE_WOOD], rm->resources.current[RESOURCE_STONE], rm->resources.max[RESOURCE_STONE]);
	send_to_char(buf, ch);

	send_to_char("Description:\r\n&y", ch);
	if (rm->description)
		write_to_output(ch->desc, FALSE, "%s", rm->description);
	else
		send_to_char("  None.&n\r\n", ch);

	if (rm->ex_description) {
		sprintf(buf, "&nExtra descs:&c");
		for (desc = rm->ex_description; desc; desc = desc->next) {
			strcat(buf, " ");
			strcat(buf, desc->keyword);
		}
		send_to_char(strcat(buf, "&n\r\n"), ch);
	}
	sprintf(buf, "&nChars present:&c");
	for (found = 0, k = rm->people; k; k = k->next_in_room) {
		if (!CAN_SEE(ch, k))
			continue;
		sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
						(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
		strcat(buf, buf2);
		if (strlen(buf) >= 62) {
			if (k->next_in_room)
				send_to_char(strcat(buf, ",\r\n"), ch);
			else
				send_to_char(strcat(buf, "&n\r\n"), ch);
			*buf = found = 0;
		}
	}

	if (*buf)
		send_to_char(strcat(buf, "&n\r\n"), ch);

	if (rm->contents) {
		sprintf(buf, "&nContents:&g");
		for (found = 0, j = rm->contents; j; j = j->next_content) {
			if (!CAN_SEE_OBJ(ch, j))
				continue;
			sprintf(buf2, "%s %s", found++ ? "," : "", j->short_description);
			strcat(buf, buf2);
			if (strlen(buf) >= 62) {
				if (j->next_content)
					send_to_char(strcat(buf, ",\r\n"), ch);
				else
					send_to_char(strcat(buf, "\r\n"), ch);
				*buf = found = 0;
			}
		}

		if (*buf)
			send_to_char(strcat(buf, "&n\r\n"), ch);
		send_to_char(CCNRM(ch, C_NRM), ch);
	}
	for (i = 0; i < NUM_OF_DIRS; i++) {
		if (rm->dir_option[i]) {
			if (rm->dir_option[i]->to_room == NOWHERE)
				strcpy(buf1, "&cNONE&n");
			else
				sprintf(buf1, "&c%5d&n", GET_ROOM_VNUM(rm->dir_option[i]->to_room));
			sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2, sizeof(buf2));
			sprintf(buf, "Exit &c%-10s&n:  To: [&c%s&n], Key: [&c%5d&n], Keywrd: &c%s&n, Type: &c%s&n\r\n ",
							dirs[i], buf1, rm->dir_option[i]->key,
					 rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None",
							buf2);
			send_to_char(buf, ch);
			if (rm->dir_option[i]->general_description)
				strcpy(buf, rm->dir_option[i]->general_description);
			else
				strcpy(buf, "  No exit description.&n\r\n");
			send_to_char(buf, ch);
		}
	}

	/* check the room for a script */
	do_sstat_room(ch);
}


void do_stat_object(struct char_data *ch, struct obj_data *j)
{
	int i, found;
	obj_vnum vnum;
	struct obj_data *j2;
	struct extra_descr_data *desc;

	vnum = GET_OBJ_VNUM(j);
	sprintf(buf, "Name: &y\\c98%s\\c99&n, Aliases: &y%s&n\r\n",
					((j->short_description) ? j->short_description : "<None>"),
					j->name);
	send_to_char(buf, ch);
	sprinttype(GET_OBJ_TYPE(j), item_types, buf1, sizeof(buf1));
	sprintf(buf, "VNum: [&c%5d&n], RNum: [&c%5d&n], Type: &c%s&n\r\n",
		vnum, GET_OBJ_RNUM(j), buf1);
	send_to_char(buf, ch);
	if (GET_OBJ_RNUM(j) == NOTHING) {
		sprintf(buf, "Proto VNum: [&c%5d&n]\r\n",
			GET_OBJ_PROTOVNUM(j));
		send_to_char(buf, ch);
	}

	if (obj_index[GET_OBJ_RNUM(j)].func!=NULL)
			get_spec_name(GET_OBJ_RNUM(j), buf2, 'o');
	else sprintf(buf2, "None");      
	sprintf(buf, "SpecProc: (&c%s&n)\r\n", buf2);
	send_to_char(buf, ch);

	sprintf(buf, "L-Des: &y\\c98%s\\c99&n\r\n", ((j->description) ? j->description : "None"));
	send_to_char(buf, ch);

	if (j->ex_description) {
		sprintf(buf, "Extra descs:&c");
		for (desc = j->ex_description; desc; desc = desc->next) {
			strcat(buf, " ");
			strcat(buf, desc->keyword);
		}
		strcat(buf, "&n\r\n");
		send_to_char(buf, ch);
	}
	send_to_char("Can be worn on: &c", ch);
	sprintbit(GET_OBJ_WEAR(j), wear_bits, buf, sizeof(buf));
	strcat(buf, "&n\r\n");
	send_to_char(buf, ch);

	send_to_char("Set char bits : &c", ch);
	sprintbit(GET_OBJ_AFFECT(j), affected_bits, buf, sizeof(buf));
	strcat(buf, "&n\r\n");
	send_to_char(buf, ch);

	send_to_char("Extra flags   : &c", ch);
	sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf, sizeof(buf));
	strcat(buf, "&n\r\n");
	send_to_char(buf, ch);

	sprintf(buf, "Weight: &c%d&n, Value: &c%d&n, Cost/day: &c%d&n, Timer: &c%d&n, Size: &y%s&n\r\n",
		 GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j), race_size[(int)GET_OBJ_SIZE(j) + 1]);
	send_to_char(buf, ch);

	strcpy(buf, "In room: &c");
	if (IN_ROOM(j) == NOWHERE)
		strcat(buf, "Nowhere");
	else {
		sprintf(buf2, "%d", GET_ROOM_VNUM(IN_ROOM(j)));
		strcat(buf, buf2);
	}
	/*
	 * NOTE: In order to make it this far, we must already be able to see the
	 *       character holding the object. Therefore, we do not need CAN_SEE().
	 */
	strcat(buf, "&n, In object: &c");
	strcat(buf, j->in_obj ? j->in_obj->short_description : "None");
	strcat(buf, "&n, Carried by: &c");
	strcat(buf, j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
	strcat(buf, "&n, Worn by: &c");
	strcat(buf, j->worn_by ? GET_NAME(j->worn_by) : "Nobody");
	strcat(buf, "&n\r\n");
	send_to_char(buf, ch);

	switch (GET_OBJ_TYPE(j)) {
	case ITEM_LIGHT:
		if (GET_OBJ_VAL(j, 2) == -1)
			strcpy(buf, "Hours left: &cInfinite&n");
		else
			sprintf(buf, "Hours left: [&c%d&n]", GET_OBJ_VAL(j, 2));
		break;
	case ITEM_SCROLL:
	case ITEM_POTION:
		sprintf(buf, "Spells: (Level &c%d&n) &c%s&n, &c%s&n, &c%s&n", GET_OBJ_VAL(j, 0),
						skill_name(GET_OBJ_VAL(j, 1)), skill_name(GET_OBJ_VAL(j, 2)),
						skill_name(GET_OBJ_VAL(j, 3)));
		break;
	case ITEM_SPELLBOOK:
		sprintf(buf, "Spells: (Learn %d%%) %s, %s, %s", GET_OBJ_VAL(j, 0),
						skill_name(GET_OBJ_VAL(j, 1)), skill_name(GET_OBJ_VAL(j, 2)),
						skill_name(GET_OBJ_VAL(j, 3)));
		break;
	case ITEM_WAND:
	case ITEM_STAFF:
		sprintf(buf, "Spell: &c%s&n at level &c%d&n, &c%d&n (of &c%d&n) charges remaining",
						skill_name(GET_OBJ_VAL(j, 3)), GET_OBJ_VAL(j, 0),
						GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 1));
		break;
	case ITEM_WEAPON:
		sprintf(buf, "Todam: &c%dd%d&n, Message type: &c%d&n",
						GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
		break;
	case ITEM_ARMOR:
		sprintf(buf, "AC-apply: [&c%d&n]", GET_OBJ_VAL(j, 0));
		break;
	case ITEM_TRAP:
		sprintf(buf, "Spell: &c%d&n, - Hitpoints: &c%d&n",
						GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
		break;
	case ITEM_CONTAINER:
		sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf2, sizeof(buf2));
		sprintf(buf, "Weight capacity: &c%d&n, Lock Type: &c%s&n, Key Num: &c%d&n, Corpse: &c%s&n",
						GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2),
						YESNO(GET_OBJ_VAL(j, 3)));
		break;
	case ITEM_DRINKCON:
	case ITEM_FOUNTAIN:
		sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2, sizeof(buf2));
		sprintf(buf, "Capacity: &c%d&n, Contains: &c%d&n, Poisoned: &c%s&n, Liquid: &c%s&n",
						GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), YESNO(GET_OBJ_VAL(j, 3)),
						buf2);
		break;
	case ITEM_NOTE:
		sprintf(buf, "Tongue: &c%d&n", GET_OBJ_VAL(j, 0));
		break;
	case ITEM_KEY:
		strcpy(buf, "");
		break;
	case ITEM_FOOD:
		sprintf(buf, "Makes full: &c%d&n, Poisoned: &c%s&n", GET_OBJ_VAL(j, 0),
						YESNO(GET_OBJ_VAL(j, 3)));
		break;
	case ITEM_MONEY:
		sprintf(buf, "Coins: &c%d&n", GET_OBJ_VAL(j, 0));
		break;
	case ITEM_SHEATH:
		sprintf(buf, "Weapon type: &c%s&n", combat_skills[GET_OBJ_VAL(j, 0)]);
		break;
	default:
		sprintf(buf, "Values 0-3: [&c%d&n] [&c%d&n] [&c%d&n] [&c%d&n]",
						GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
						GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
		break;
	}
	send_to_char(strcat(buf, "\r\n"), ch);

	/*
	 * I deleted the "equipment status" code from here because it seemed
	 * more or less useless and just takes up valuable screen space.
	 */

	if (j->contains) {
		strcat(buf, "\r\nContents:&g");
		for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
			sprintf(buf2, "%s %s", found++ ? "," : "", j2->short_description);
			strcat(buf, buf2);
			if (strlen(buf) >= 62) {
				if (j2->next_content)
					send_to_char(strcat(buf, ",\r\n"), ch);
				else
					send_to_char(strcat(buf, "&n\r\n"), ch);
				*buf = found = 0;
			}
		}

		if (*buf)
			send_to_char(strcat(buf, "\r\n"), ch);
		send_to_char(CCNRM(ch, C_NRM), ch);
	}
	found = 0;
	send_to_char("Affections:&g", ch);
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		if (j->affected[i].modifier) {
			sprinttype(j->affected[i].location, apply_types, buf2, sizeof(buf2));
			sprintf(buf, "%s %+d to %s", found++ ? "," : "",
							j->affected[i].modifier, buf2);
			send_to_char(buf, ch);
		}
	if (!found)
		send_to_char(" &gNone", ch);

	send_to_char("&n\r\n", ch);

	/* check the object for a script */
	do_sstat_object(ch, j);
}


void do_stat_character(struct char_data *ch, struct char_data *k)
{
	int i, i2, found = 0, weekday = 0, day = 0;
	struct follow_type *fol;
	struct affected_type *aff;
	char *printbuf, sbuf[MAX_INPUT_LENGTH];

	day = k->player_specials->saved.bday_day + 1;        /* day in [1..28] */
	
	if (!(k->player_specials->saved.bday_month == 12 && day == 28)) {
		weekday = ((28 * k->player_specials->saved.bday_month) + day) % 7;
	} else {
		weekday = 7;
	}
	if (k->player_specials->saved.bday_day == 0)
		weekday = 0;

printbuf = get_buffer(MAX_STRING_LENGTH);

sprinttype(GET_SEX(k), genders, sbuf, sizeof(sbuf));
sprintf(printbuf, "%s [&g%5ld&n] [&g%s&n] %s\r\n", GET_NAME(k), IS_NPC(k) ? GET_MOB_VNUM(k) : GET_IDNUM(k), IS_NPC(k) ? "NPC" : "PC", sbuf);
if (!IS_NPC(k)) {
	sprintf(printbuf + strlen(printbuf), "True Name   : &y%s&n\r\n", (GET_TRUENAME(k) ? GET_TRUENAME(k) : "<None>"));
	sprintf(printbuf + strlen(printbuf), "Title       : &y%s&n\r\n", (GET_TITLE(k) ? GET_TITLE(k) : "<None>"));
	sprintf(printbuf + strlen(printbuf), "Doing       : &y%s&n\r\n", (GET_DOING(k) ? GET_DOING(k) : "<None>"));
	sprintf(printbuf + strlen(printbuf), "Email       : &y%s&n, OK to mail: &y%s&n\r\n", GET_EMAIL(k) ? GET_EMAIL(k) : "<Not Set>", (GET_MAILINGLIST(k)?"YES":"NO"));
	sprintf(printbuf + strlen(printbuf), "Keywords    : &y%s&n\r\n", (GET_KEYWORDS(k) ? GET_KEYWORDS(k) : "<None>"));
	sprintf(printbuf + strlen(printbuf), "S-Des       : &y%s&n\r\n", (GET_SDESC(k) ? GET_SDESC(k) : "<None>"));
} else {
	sprintf(printbuf + strlen(printbuf), "Alias       : &y%s&n\r\n", (GET_ALIAS(k) ? GET_ALIAS(k) : "<None>"));
}

sprintf(printbuf + strlen(printbuf), "L-Des       : &y%s&n%s", (GET_LDESC(k) ? GET_LDESC(k) : "<None>\r\n"), (k->desc) ? "" : (IS_NPC(k) ? "" : "\r\n"));

if (!IS_NPC(k)) {
	int count = 0;
	for (i=0; i < NUM_DESCS; i++) {
		if (GET_RPDESCRIPTION(k, i))
			count++;
	}
	sprintf(printbuf + strlen(printbuf), "RP-Desc     : [&g%d&n] (Use STAT DESC to view)\r\n", count);
	sprintf(printbuf + strlen(printbuf), "Hometown    : [&g%5d&n]                     In room     : [&g%5d&n]\r\n", mortal_start_room[GET_HOME(k)], GET_ROOM_VNUM(IN_ROOM(k)));
	sprintf(printbuf + strlen(printbuf), "Race        : [&g%-20s&n]      Language    : [&g%s&n]\r\n", race_list[(int)GET_RACE(k)].name, skill_info[SPEAKING(k)].name);
} else {
	sprintf(printbuf + strlen(printbuf), "Mobile Race : [&g%-20s&n]      Difficulty  : [&g%s&n]\r\n", race_list[(int)GET_RACE(k)].name, mob_difficulty_type[(int)GET_DIFFICULTY(k)]);
}

sprintf(printbuf + strlen(printbuf), "Social Rank : [&g%-16s&n]          Piety Rank  : [&g%-16s&n]\r\n", social_ranks[GET_SOCIAL_RANK(k)], piety_ranks[GET_PIETY(k)]);
sprintf(printbuf + strlen(printbuf), "Reputation  : [&g%-16s&n]          Align       : [&g%4d&n]\r\n", (GET_REPUTATION(k) >= 0 ? reputation_ranks[GET_REPUTATION(k)][0] : reputation_ranks[0 - GET_REPUTATION(k)][1]), GET_ALIGNMENT(k));
sprintf(sbuf, "%s %s", hair_style[GET_HAIRSTYLE(k)], hair_color[GET_HAIRCOLOR(k)]);
sprintf(printbuf + strlen(printbuf), "Hair        : [&g%-22s&n]    Eyes        : [&g%s&n]\r\n", sbuf, eye_color[GET_EYECOLOR(k)]);
sprintf(printbuf + strlen(printbuf), "Skin        : [&g%-22s&n]    Size        : [&g%s&n]\r\n", skin_tone[GET_SKINTONE(k)], race_size[(int)GET_SIZE(k) + 1]);
sprintf(printbuf + strlen(printbuf), "Weight      : [&g%3d&n] (%-10s)          Height      : [&g%3d&n] (%-10s)\r\n", GET_WEIGHT(k), short_character_weights[(int)weight_class(k)][(int)GET_SEX(k)], GET_HEIGHT(k), short_character_heights[(int)height_class(k)][(int)GET_SEX(k)]);

if (!IS_NPC(k)) {
	sprintf(sbuf, "%s, %d %s %d", weekdays[weekday], day, month_name[k->player_specials->saved.bday_month], k->player_specials->saved.bday_year);
	sprintf(printbuf + strlen(printbuf), "Birthday    : [&g%-24s&n]  Age         : [&g%d&n]\r\n", sbuf, age(k)->year);
	strcpy(sbuf, (char *) asctime(localtime(&(k->player.time.birth))));
	sbuf[24] = '\0';
	sprintf(printbuf + strlen(printbuf), "Created     : [&g%-24s&n]  ", sbuf);
	strcpy(sbuf, (char *) asctime(localtime(&(k->player.time.logon))));
	sbuf[24] = '\0';
	sprintf(printbuf + strlen(printbuf), "Last Logon  : [&g%-24s&n]\r\n", sbuf);
	sprintf(printbuf + strlen(printbuf), "Played      : [&g%5dh %2dm&n]                Idle Timer  : [&g%5d&n] (in tics)\r\n", k->player.time.played / 3600, (k->player.time.played % 3600) / 60, k->char_specials.timer);
	sprintf(printbuf + strlen(printbuf), "Str: [&g%.2f&n]  Agl: [&g%.2f&n]  Pre: [&g%.2f&n]  Per: [&g%.2f&n]  Hea: [&g%.2f&n]\r\n",
		((double)GET_STRENGTH(k)) / 100, ((double)GET_AGILITY(k)) / 100, ((double)GET_PRECISION(k)) / 100, ((double)GET_PERCEPTION(k)) / 100, ((double)GET_HEALTH(k)) / 100);
	sprintf(printbuf + strlen(printbuf), "Wil: [&g%.2f&n]  Int: [&g%.2f&n]  Cha: [&g%.2f&n]  Luc: [&g%.2f&n]  Ess: [&g%.2f&n]\r\n",
		((double)GET_WILLPOWER(k)) / 100, ((double)GET_INTELLIGENCE(k)) / 100, ((double)GET_CHARISMA(k)) / 100, ((double)GET_LUCK(k)) / 100, ((double)GET_ESSENCE(k)) / 100);
}

sprintf(printbuf + strlen(printbuf), "Hit p.      : [&g%5d/%-5d+%2d&n]            Mana p.     : [&g%5d/%-5d+%2d&n]\r\n", GET_HIT(k), GET_MAX_HIT(k), hit_gain(k), GET_MANA(k), GET_MAX_MANA(k), mana_gain(k));
sprintf(printbuf + strlen(printbuf), "Move p.     : [&g%5d/%-5d+%2d&n]            Flux p.     : [&g%5d/%-5d   &n]\r\n", GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k), GET_FLUX(k), GET_MAX_FLUX(k));

if (!IS_NPC(k)) {
	sprintf(printbuf + strlen(printbuf), "Hunger      : [&g%3d&n]     Thirst :[&g%3d&n]     Drunk : [&g%3d&n]     Fatigue: [&g%3d&n]\r\n", GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK), GET_FATIGUE(k));
	sprintf(printbuf + strlen(printbuf), "RPxp        : [&g%6d&n]                    QPs         : [&g%6d&n]\r\n", GET_RPXP(k), GET_QP(k));
	sprintf(printbuf + strlen(printbuf), "Skillcap    : [&g%8d&n]                  SP Used     : [&g%8d&n]\r\n", GET_SKILLCAP(k), skill_cap(k, 0, 0));
}

sprintf(printbuf + strlen(printbuf), "Coins       : [&g%9d&n]                 Bank        : [&g%9d&n] \r\n", GET_GOLD(k), GET_BANK_GOLD(k)); 
sprintf(printbuf + strlen(printbuf), "Passive Def : [&g%3d&n]                       Dam Reduc   : [&g%3d&n]\r\n", GET_PD(k), GET_REDUCTION(k));
sprintf(printbuf + strlen(printbuf), "Hitroll     : [&g%3d&n]                       Damroll     : [&g%3d&n]\r\n", GET_HITROLL(k), GET_DAMROLL(k));
for (i = 0, i2 = 0; i < NUM_WEARS; i++)
	if (HAS_BODY(k, i) && GET_EQ(k, i))
		i2++;
sprintf(printbuf + strlen(printbuf), "Carried     : [&g%3d&n] (weight: %3d)         Items worn  : [&g%3d&n]\r\n", IS_CARRYING_N(k), IS_CARRYING_W(k), i2);
sprintf(printbuf + strlen(printbuf), "Approved by : [&g%-20s&n]      Contact     : [&g%s&n]\r\n", (GET_APPROVEDBY(k) ? GET_APPROVEDBY(k) : "Nobody"), (GET_CONTACT(k) ? GET_CONTACT(k) : "Nobody"));

sprintf(sbuf, "%s", "\0");
for (fol = k->followers; fol; fol = fol->next)
	sprintf(sbuf + strlen(sbuf), "%s %s", found++ ? "," : "", PERS(fol->follower, ch, 1));
sprintf(printbuf + strlen(printbuf), "Master is   : [&g%-20s&n], Followers are: &y%s&n\r\n", ((k->master) ? GET_NAME(k->master) : "<none>"), found ? sbuf : "<none>");
sprinttype(GET_POS(k), position_types, sbuf, sizeof(sbuf));
sprintf(printbuf + strlen(printbuf), "Position    : [&g%-10s&n]                ", sbuf);
sprinttype((k->mob_specials.default_pos), position_types, sbuf, sizeof(sbuf));
sprintf(printbuf + strlen(printbuf), "Default     : [&g%-10s&n]\r\n", sbuf); 

if (!IS_NPC(k)) {
	if (k->desc) {
		sprinttype(STATE(k->desc), connected_types, sbuf, sizeof(sbuf));
	} else {
		sprintf(sbuf, "Offline");
	}
	sprintf(printbuf + strlen(printbuf), "Conn. State : [&g%-20s&n]      Fighting    : [&g%s&n]\r\n", sbuf, (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody"));
	sprintbit(USER_RIGHTS(k), user_rights, sbuf, sizeof(sbuf));
	sprintf(printbuf + strlen(printbuf), "RIGHTS      : &y%s&n\r\n", sbuf);
	sprintbit(EXTENDED_SYSLOG(k), extended_syslogs, sbuf, sizeof(sbuf));
	sprintf(printbuf + strlen(printbuf), "SYSLOGS     : &y%s&n\r\n", sbuf); 
	sprintbit(PLR_FLAGS(k), player_bits, sbuf, sizeof(sbuf));
	sprintf(printbuf + strlen(printbuf), "PLR FLAGS   : &y%s&n\r\n", sbuf);
	sprintbit(PRF_FLAGS(k), preference_bits, sbuf, sizeof(sbuf));
	sprintf(printbuf + strlen(printbuf), "PRF FLAGS   : &y%s&n\r\n", sbuf);
} else {
	sprintf(printbuf + strlen(printbuf), "Spec Proc   : [&g%-6s&n]                    Fighting    : [&g%s&n]\r\n", (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"), (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody"));
	sprintf(printbuf + strlen(printbuf), "Attack type : [&g%-15s&n]           Bare Hand   : [&g%3dd%3d&n]\r\n", attack_hit_text[k->mob_specials.attack_type].singular, k->mob_specials.damnodice, k->mob_specials.damsizedice);
	sprintbit(MOB_FLAGS(k), action_bits, sbuf, sizeof(sbuf));
	sprintf(printbuf + strlen(printbuf), "MOB FLAGS   : &y%s&n\r\n", sbuf);
}

sprintbit(AFF_FLAGS(k), affected_bits, sbuf, sizeof(sbuf));
sprintf(printbuf + strlen(printbuf), "AFF FLAGS   : &y%s&n\r\n", sbuf);

send_to_char(printbuf, ch);

if (k->affected) {
	release_buffer(printbuf);
	printbuf = get_buffer(MAX_STRING_LENGTH);
	for (aff = k->affected; aff; aff = aff->next) {
		sprintf(printbuf + strlen(printbuf), "SPL: (&g%3d ticks&n) &y%-21s&n ", aff->duration + 1, spell_name(aff->type));
		if (aff->modifier)
			sprintf(printbuf + strlen(printbuf), "&y%+d to %s&n", aff->modifier, apply_types[(int) aff->location]);

		if (aff->bitvector) {
			sprintbit(aff->bitvector, affected_bits, sbuf, sizeof(sbuf));
			sprintf(printbuf + strlen(printbuf), "&y%ssets %s&n", aff->modifier ? ", " : "", sbuf);
		}
		sprintf(printbuf + strlen(printbuf), "\r\n");
	}
	send_to_char(printbuf, ch);
}

release_buffer(printbuf);
printbuf = get_buffer(MAX_STRING_LENGTH);

if (IS_NPC(k)) {
	do_sstat_character(ch, k);
	if (SCRIPT_MEM(k)) {
		struct script_memory *mem = SCRIPT_MEM(k);
		sprintf(printbuf, "\r\nScript memory:\r\n  Remember             Command\r\n");
		while (mem) {
			struct char_data *mc = find_char(mem->id);
			if (!mc) sprintf(printbuf + strlen(printbuf), "&y  ** Corrupted!&n\r\n");
			else {
				if (mem->cmd) sprintf(printbuf + strlen(printbuf), "  &y%-20.20s%s&n\r\n", GET_NAME(mc), mem->cmd);
				else sprintf(printbuf + strlen(printbuf), "  &y%-20.20s <default>&n\r\n", GET_NAME(mc));
			}
		mem = mem->next;
		}
	}
} else {
	/* this is a PC, display their global variables */
	if (k->script && k->script->global_vars) {
		struct trig_var_data *tv;
		char name[MAX_INPUT_LENGTH];

		sprintf(printbuf, "Global Variables:\r\n");

		/* currently, variable context for players is always 0, so it is */
		/* not displayed here. in the future, this might change */
		for (tv = k->script->global_vars; tv; tv = tv->next) {
			if (*(tv->value) == UID_CHAR) {
				find_uid_name(tv->value, name);
				sprintf(printbuf + strlen(printbuf), "    %10s:  [&gUID&n]: %s\r\n", tv->name, name);
			} else
				sprintf(printbuf + strlen(printbuf), "    %10s:  %s\r\n", tv->name, tv->value);
		}
	}
}

send_to_char(printbuf, ch);
release_buffer(printbuf);

}


void do_stat_background(struct char_data *ch, struct char_data *k)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);

	sprintf(printbuf, "Background for %s:\r\n\r\n%s", GET_NAME(k), strlen(k->player.description) > 2 ? k->player.description : "No background written yet.\r\n");

	page_string(ch->desc, printbuf, TRUE);
	release_buffer(printbuf);
}

void do_stat_description(struct char_data *ch, struct char_data *k)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	int i, found=0;

	sprintf(printbuf, "Descriptions for %s:\r\n\r\n&WS-Desc:&n %s\r\n\r\n&WL-Desc:&n %s\r\n&WRP Descs (&C*&n = active):&n\r\n", GET_NAME(k), GET_SDESC(k), GET_LDESC(k));

	for (i=0; i < NUM_DESCS; i++) {
		if (GET_RPDESCRIPTION(k, i)) {
			sprintf(printbuf, "%s%s&W%d)&n %s\r\n", printbuf, i == GET_ACTIVEDESC(k) ? "&C*&n" : "", i, GET_RPDESCRIPTION(k, i));
			found++;
		}
	}

	if (!found) {
		sprintf(printbuf, "%sNo RP Desc set.\r\n", printbuf);
	}

	page_string(ch->desc, printbuf, TRUE);
	release_buffer(printbuf);
}


void do_stat_command(struct char_data *ch, int command)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH), *rbuf = get_buffer(80);

	sprintbit(cmd_info[command].rights, user_rights, rbuf, 80);
	
	sprintf(printbuf, "Command Name  : &y%s&n\r\n", cmd_info[command].command);
	sprintf(printbuf + strlen(printbuf), "Sort as       : &y%s&n\r\n", (cmd_info[command].sort_as ? cmd_info[command].sort_as : cmd_info[command].command));
	sprintf(printbuf + strlen(printbuf), "Min. Position : &y%s&n\r\n", position_types[(int)cmd_info[command].minimum_position]);
	sprintf(printbuf + strlen(printbuf), "Command Func  : &y%s&n\r\n", command_list[cmd_info[command].command_num].name);
	sprintf(printbuf + strlen(printbuf), "Rights        : &y%s&n\r\n", rbuf);
	sprintf(printbuf + strlen(printbuf), "Subcommand    : &y%d&n\r\n", cmd_info[command].subcmd);
	sprintf(printbuf + strlen(printbuf), "Copyover      : &y%s&n\r\n", (cmd_info[command].copyover ? "ALLOWED" : "NOT ALLOWED"));
	sprintf(printbuf + strlen(printbuf), "Enabled       : &y%s&n\r\n", (cmd_info[command].enabled ? "ENABLED" : "DISABLED"));

	page_string(ch->desc, printbuf, TRUE);
	release_buffer(rbuf);
	release_buffer(printbuf);
}


ACMD(do_stat)
{
	struct char_data *victim;
	struct obj_data *object;
	int tmp, command;
	char *inputbuf1 = get_buffer(MAX_INPUT_LENGTH + 1), *inputbuf2 = get_buffer(MAX_INPUT_LENGTH + 1);

	half_chop(argument, inputbuf1, inputbuf2);

	if (!*inputbuf1) {
		send_to_char("Stats on who or what?\r\n", ch);
		release_buffer(inputbuf1);
		release_buffer(inputbuf2);
		return;
	} else if (is_abbrev(inputbuf1, "room")) {
		do_stat_room(ch);
	} else if (is_abbrev(inputbuf1, "zone")) {
		do_zstat_room(ch, IN_ROOM(ch));
	} else if (is_abbrev(inputbuf1, "mob")) {
		if (!*inputbuf2)
			send_to_char("Stats on which mobile?\r\n", ch);
		else {
			if ((victim = get_char_vis(ch, inputbuf2, NULL, FIND_CHAR_WORLD, 1)) != NULL)
				do_stat_character(ch, victim);
			else
				send_to_char("No such mobile around.\r\n", ch);
		}
	} else if (is_abbrev(inputbuf1, "player")) {
		if (!*inputbuf2) {
			send_to_char("Stats on which player?\r\n", ch);
		} else {
			if ((victim = get_player_vis(ch, inputbuf2, NULL, FIND_CHAR_WORLD)) != NULL)
				do_stat_character(ch, victim);
			else
				send_to_char("No such player around.\r\n", ch);
		}
	} else if (is_abbrev(inputbuf1, "file")) {
		if (!*inputbuf2) {
			send_to_char("Stats on which player?\r\n", ch);
		} else {
			CREATE(victim, struct char_data, 1);
			clear_char(victim);
			CREATE(victim->player_specials, struct player_special_data, 1);
			if (load_char(inputbuf2, victim) > -1) {
				if (compare_rights(ch, victim) == -1 && !IS_IMPL(ch))
					send_to_char("Sorry, you can't do that.\r\n", ch);
				else
					do_stat_character(ch, victim);
				free_char(victim);
			} else {
				send_to_char("There is no such player.\r\n", ch);
				free_char(victim);
			}
		}
	} else if (is_abbrev(inputbuf1, "background")) {
		if (!IS_RPSTAFF(ch)) {
			send_to_char("You are not allowed to view backgrounds.\r\n", ch);
		} else {
			if (!*inputbuf2) {
				MYSQL_RES *playerindex;
				MYSQL_ROW row;
				if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT Name FROM %s WHERE LENGTH(Background) > 10;", TABLE_PLAYER_INDEX))) {
					extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading player records from [%s].", TABLE_PLAYER_INDEX);
				} else {
					int rec_count;
					rec_count = mysql_num_rows(playerindex);
					if(rec_count == 0) {
						send_to_char("There are no players with written backgrounds.\r\n", ch);
					} else {
						char *printbuf = get_buffer(MAX_STRING_LENGTH);
						int i = 0;
						strcpy(printbuf, "The following players have written backgrounds:\r\n");
						for(i = 0; i < rec_count; i++)
						{
							row = mysql_fetch_row(playerindex);
							sprintf(printbuf, "%s%s\r\n", printbuf, row[0]);
						}
						page_string(ch->desc, printbuf, TRUE);
						mysql_free_result(playerindex);
						release_buffer(printbuf);
					}
				}
			} else {
				CREATE(victim, struct char_data, 1);
				clear_char(victim);
				CREATE(victim->player_specials, struct player_special_data, 1);
				if (load_char(inputbuf2, victim) > -1) {
					do_stat_background(ch, victim);
					free_char(victim);
				} else {
					send_to_char("There is no such player.\r\n", ch);
					free_char(victim);
				}
			}
		}
	} else if (is_abbrev(inputbuf1, "object")) {
		if (!*inputbuf2)
			send_to_char("Stats on which object?\r\n", ch);
		else {
			if ((object = get_obj_vis(ch, inputbuf2, NULL)) != NULL)
				do_stat_object(ch, object);
			else
				send_to_char("No such object around.\r\n", ch);
		}
	} else if (is_abbrev(inputbuf1, "description")) {
		if (!IS_RPSTAFF(ch)) {
			send_to_char("You are not allowed to view descriptions.\r\n", ch);
		} else {
			if (!*inputbuf2) {
				send_to_char("View descriptions on what player?\r\n", ch);
				release_buffer(inputbuf1);
				release_buffer(inputbuf2);
				return;
			} else {
				CREATE(victim, struct char_data, 1);
				clear_char(victim);
				CREATE(victim->player_specials, struct player_special_data, 1);
				if (load_char(inputbuf2, victim) > -1) {
					do_stat_description(ch, victim);
					free_char(victim);
				} else {
					send_to_char("There is no such player.\r\n", ch);
					free_char(victim);
				}
			}
		}
	} else if (is_abbrev(inputbuf1, "command")) {
		if (!*inputbuf2)
			send_to_char("Stats on which command?\r\n", ch);
		else {
			for (command = 0;(command <= top_of_commandt); command++)
				if (is_abbrev(inputbuf2, cmd_info[command].command)) break;
			if (command > top_of_commandt) {
				if (find_social(inputbuf2) > NOTHING)
					send_to_char("This command exists as a social.  Use ASTAT instead.\r\n", ch);
				else
					send_to_char("No such command.\r\n", ch);
			}	else
				do_stat_command(ch, command);
		}
	} else {
		char *name = inputbuf1;
		int number = get_number(&name);

		if ((object = get_obj_in_equip_vis(ch, inputbuf1, &tmp, ch->equipment)) != NULL)
			do_stat_object(ch, object);
		else if ((object = get_obj_in_list_vis(ch, inputbuf1, &number, ch->carrying)) != NULL)
			do_stat_object(ch, object);
		else if ((victim = get_char_vis(ch, inputbuf1, &number, FIND_CHAR_ROOM, 1)) != NULL)
			do_stat_character(ch, victim);
		else if ((object = get_obj_in_list_vis(ch, inputbuf1, &number, world[IN_ROOM(ch)].contents)) != NULL)
			do_stat_object(ch, object);
		else if ((victim = get_char_vis(ch, inputbuf1, &number, FIND_CHAR_WORLD, 1)) != NULL)
			do_stat_character(ch, victim);
		else if ((object = get_obj_vis(ch, inputbuf1, &number)) != NULL)
			do_stat_object(ch, object);
		else
			send_to_char("Nothing around by that name.\r\n", ch);
	}
	release_buffer(inputbuf1);
	release_buffer(inputbuf2);
}


ACMD(do_shutdown)
{
	if (subcmd != SCMD_SHUTDOWN) {
		send_to_char("If you want to shut something down, say so!\r\n", ch);
		return;
	}
	one_argument(argument, arg);

	if (!*arg) {
		extended_mudlog(BRF, SYSL_GENERAL, TRUE, "Shutdown by %s.", GET_NAME(ch));
		send_to_all("Shutting down.\r\n");
		circle_shutdown = 1;
	} else if (!str_cmp(arg, "now")) {
		extended_mudlog(BRF, SYSL_GENERAL, TRUE, "Shutdown NOW by %s.", GET_NAME(ch));
		send_to_all("Rebooting... come back in 10-30 seconds.\r\n");
		circle_shutdown = 1;
		circle_reboot = 2;
	} else if (!str_cmp(arg, "emergency") && IS_IMPL(ch)) {
		extended_mudlog(BRF, SYSL_GENERAL, TRUE, "Shutdown EMERGENCY by %s.", GET_NAME(ch));
		send_to_all("Emergency shutdown... come back in 10-30 seconds.\r\n");
		circle_shutdown = 1;
		circle_reboot = 3;
	} else if (!str_cmp(arg, "reboot")) {
		extended_mudlog(BRF, SYSL_GENERAL, TRUE, "Reboot by %s.", GET_NAME(ch));
		send_to_all("Rebooting... come back in 10-30 seconds.\r\n");
		touch(FASTBOOT_FILE);
		circle_shutdown = circle_reboot = 1;
	} else if (!str_cmp(arg, "die")) {
		extended_mudlog(BRF, SYSL_GENERAL, TRUE, "Shutdown by %s.", GET_NAME(ch));
		send_to_all("Shutting down for maintenance.\r\n");
		touch(KILLSCRIPT_FILE);
		circle_shutdown = 1;
	} else if (!str_cmp(arg, "pause")) {
		extended_mudlog(BRF, SYSL_GENERAL, TRUE, "Shutdown by %s.", GET_NAME(ch));
		send_to_all("Shutting down for maintenance.\r\n");
		touch(PAUSE_FILE);
		circle_shutdown = 1;
	} else
		send_to_char("Unknown shutdown option.\r\n", ch);
}


void snoop_check(struct char_data *ch)
{
	/*  This short routine is to ensure that characters that happen
	 *  to be snooping (or snooped) and get advanced/demoted will
	 *  not be snooping/snooped someone of a higher/lower level (and
	 *  thus, not entitled to be snooping.
	 */
	if (ch->desc && ch->desc->snooping &&
			(compare_rights(ch, ch->desc->snooping->character) <= 0)) {
		ch->desc->snooping->snoop_by = NULL;
		ch->desc->snooping = NULL;
	}

	if (ch->desc && ch->desc->snoop_by &&
			(compare_rights(ch->desc->snooping->character, ch) <= 0)) {
		ch->desc->snoop_by->snooping = NULL;
		ch->desc->snoop_by = NULL;
	}
}


void stop_snooping(struct char_data *ch)
{
	if (!ch->desc->snooping)
		send_to_char("You aren't snooping anyone.\r\n", ch);
	else {
		extended_mudlog(BRF, SYSL_SNOOPS, TRUE, "%s stops snooping.", GET_NAME(ch));
		send_to_char("You stop snooping.\r\n", ch);
		ch->desc->snooping->snoop_by = NULL;
		ch->desc->snooping = NULL;
	}
}


ACMD(do_snoop)
{
	struct char_data *victim, *tch;

	if (!ch->desc)
		return;

	one_argument(argument, arg);

	if (!*arg)
		stop_snooping(ch);
	else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD, 1)))
		send_to_char("No such person around.\r\n", ch);
	else if (!victim->desc)
		send_to_char("There's no link.. nothing to snoop.\r\n", ch);
	else if (victim == ch)
		stop_snooping(ch);
	else if (victim->desc->snoop_by)
		send_to_char("Busy already. \r\n", ch);
	else if (victim->desc->snooping == ch->desc)
		send_to_char("Don't be stupid.\r\n", ch);
	else {
		if (victim->desc->original)
			tch = victim->desc->original;
		else
			tch = victim;

		if (compare_rights(ch, tch) <= 0) {
			send_to_char("You can't.\r\n", ch);
			return;
		}
		send_to_char(OK, ch);

		if (ch->desc->snooping)
			ch->desc->snooping->snoop_by = NULL;

		ch->desc->snooping = victim->desc;
		victim->desc->snoop_by = ch->desc;
		extended_mudlog(BRF, SYSL_SNOOPS, TRUE, "%s starts snooping %s.", GET_NAME(ch), GET_NAME(victim));
	}
}



ACMD(do_switch)
{
	struct char_data *victim;

	one_argument(argument, arg);

	if (ch->desc->original)
		send_to_char("You're already switched.\r\n", ch);
	else if (!*arg)
		send_to_char("Switch with who?\r\n", ch);
	else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD, 1)))
		send_to_char("No such character.\r\n", ch);
	else if (ch == victim)
		send_to_char("Hee hee... we are jolly funny today, eh?\r\n", ch);
	else if (victim->desc)
		send_to_char("You can't do that, the body is already in use!\r\n", ch);
	else if (!IS_IMPL(ch) && !IS_NPC(victim))
		send_to_char("You aren't holy enough to use a mortal's body.\r\n", ch);
	else if (!IS_GRGOD(ch) && ROOM_FLAGGED(IN_ROOM(victim), ROOM_GODROOM))
		send_to_char("You are not godly enough to use that room!\r\n", ch);
	else if (!IS_GRGOD(ch) && ROOM_FLAGGED(IN_ROOM(victim), ROOM_HOUSE)
								&& !house_can_enter(ch, GET_ROOM_VNUM(IN_ROOM(victim))))
		send_to_char("That's private property -- no trespassing!\r\n", ch);
	else {
		send_to_char(OK, ch);

		ch->desc->character = victim;
		ch->desc->original = ch;

		victim->desc = ch->desc;
		ch->desc = NULL;
	}
}


ACMD(do_return)
{
	if (ch->desc && ch->desc->original) {
		send_to_char("You return to your original body.\r\n", ch);

		/*
		 * If someone switched into your original body, disconnect them.
		 *   - JE 2/22/95
		 *
		 * Zmey: here we put someone switched in our body to disconnect state
		 * but we must also NULL his pointer to our character, otherwise   
		 * close_socket() will damage our character's pointer to our descriptor
		 * (which is assigned below in this function). 12/17/99
		 */
		if (ch->desc->original->desc) {
			ch->desc->original->desc->character = NULL;
			STATE(ch->desc->original->desc) = CON_DISCONNECT;
		}

		/* Now our descriptor points to our original body. */
		ch->desc->character = ch->desc->original;
		ch->desc->original = NULL;

		/* And our body's pointer to descriptor now points to our descriptor. */
		ch->desc->character->desc = ch->desc;
		ch->desc = NULL;
	}
}



ACMD(do_load)
{
	struct char_data *mob;
	struct obj_data *obj;
	mob_vnum number;
	mob_rnum r_num;

	two_arguments(argument, buf, buf2);

	if (!*buf || !*buf2 || !isdigit(*buf2)) {
		send_to_char("Usage: load { obj | mob } <number>\r\n", ch);
		return;
	}
	if ((number = atoi(buf2)) < 0) {
		send_to_char("A NEGATIVE number??\r\n", ch);
		return;
	}
	if (is_abbrev(buf, "mob")) {
		if ((r_num = real_mobile(number)) < 0) {
			send_to_char("There is no monster with that number.\r\n", ch);
			return;
		}
		mob = read_mobile(r_num, REAL);
		char_to_room(mob, IN_ROOM(ch));

		act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
				0, 0, TO_ROOM);
		act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
		act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
		load_mtrigger(mob);
	} else if (is_abbrev(buf, "obj")) {
		if ((r_num = real_object(number)) < 0) {
			send_to_char("There is no object with that number.\r\n", ch);
			return;
		}
		obj = read_object(r_num, REAL);
		if (load_into_inventory)
			obj_to_char(obj, ch);
		else
			obj_to_room(obj, IN_ROOM(ch));
		act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
		act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
		act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
		load_otrigger(obj);
	} else
		send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}


/* Called from vstat - show the zone reset information for
 * the specified room
 */
void do_zstat_room(struct char_data *ch, zone_rnum rnum)
{
	int zone = world[rnum].zone;
	int subcmd = 0, counter = 0;
	room_vnum rvnum = GET_ROOM_VNUM(rnum);
	char output[MAX_STRING_LENGTH];
	bool found;

	sprintbit(zone_table[zone].zone_flags, zone_bits, buf1, sizeof(buf1));

	sprintf(buf,
		"Zone number : &c%d&n [&c%4d&n]\r\n"
		"Zone name   : &c%s&n\r\n"
		"Builders    : &c%s&n\r\n"
		"Zone Flags  : &c%s&n\r\n",
		zone_table[zone].number, rvnum, zone_table[zone].name ? zone_table[zone].name : "<NONE!>",
		zone_table[zone].builders ? zone_table[zone].builders : "<NONE!>", buf1);
		send_to_char(buf, ch);

	if (!(ZCMD(zone, subcmd).command)){
		send_to_char("No zone loading information.\r\n", ch);
		return;
	}

	sprintf(output, "--------------------------------------------------\r\n");
	while ((ZCMD(zone, subcmd).command) && (ZCMD(zone, subcmd).command != 'S')) {
		found = FALSE;
		/*
		 * Translate what the command means.
		 */

	 /* If this is a "then do this" command, we need to check that the previous
		* command was issued in this room
		*/
		if (ZCMD(zone, subcmd).if_flag == TRUE){
			if (check_same_room(zone, subcmd, rnum) == FALSE) {
				subcmd++;
				continue; /* not this room */
			}
		}
		switch (ZCMD(zone, subcmd).command) {
		case 'M':
			if (ZCMD(zone, subcmd).arg3 != rnum)
				break; /* not this room */
			sprintf(buf2, "&y%sLoad %s [&c%d&y], Max : %d",
							ZCMD(zone, subcmd).if_flag ? " then " : "",
							mob_proto[ZCMD(zone, subcmd).arg1].player.short_descr,
							mob_index[ZCMD(zone, subcmd).arg1].vnum, ZCMD(zone, subcmd).arg2);
			found = TRUE;
			break;
		case 'G':
			sprintf(buf2, "&y%sGive it %s [&c%d&y], Max : %d",
							ZCMD(zone, subcmd).if_flag ? " then " : "",
							obj_proto[ZCMD(zone, subcmd).arg1].short_description,
							obj_index[ZCMD(zone, subcmd).arg1].vnum,
							ZCMD(zone, subcmd).arg2);
			found = TRUE;
			break;
		case 'O':
			if (ZCMD(zone, subcmd).arg3 != rnum)
							break; /* not this room */
			sprintf(buf2, "&y%sLoad %s [&c%d&y], Max : %d",
							ZCMD(zone, subcmd).if_flag ? " then " : "",
							obj_proto[ZCMD(zone, subcmd).arg1].short_description,
							obj_index[ZCMD(zone, subcmd).arg1].vnum,
							ZCMD(zone, subcmd).arg2);
			found = TRUE;
			break;
		case 'E':
			sprintf(buf2, "&y%sEquip with %s [&c%d&y], %s, Max : %d",
							ZCMD(zone, subcmd).if_flag ? " then " : "",
							obj_proto[ZCMD(zone, subcmd).arg1].short_description,
							obj_index[ZCMD(zone, subcmd).arg1].vnum,
							equipment_types[ZCMD(zone, subcmd).arg3],
							ZCMD(zone, subcmd).arg2);

			found = TRUE;
			break;
		case 'P':
			sprintf(buf2, "&y%sPut %s [&c%d&y] in %s [&c%d&y], Max : %d",
							ZCMD(zone, subcmd).if_flag ? " then " : "",
							obj_proto[ZCMD(zone, subcmd).arg1].short_description,
							obj_index[ZCMD(zone, subcmd).arg1].vnum,
							obj_proto[ZCMD(zone, subcmd).arg3].short_description,
							obj_index[ZCMD(zone, subcmd).arg3].vnum,
							ZCMD(zone, subcmd).arg2);
			found = TRUE;
			break;
		case 'R':
			if (ZCMD(zone, subcmd).arg1 != rnum)
				break; // not this room
			sprintf(buf2, "&y%sRemove %s [&c%d&y] from room.",
							ZCMD(zone, subcmd).if_flag ? " then " : "",
							obj_proto[ZCMD(zone, subcmd).arg2].short_description,
							obj_index[ZCMD(zone, subcmd).arg2].vnum);
			found = TRUE;
			break;
		case 'D':
			if (ZCMD(zone, subcmd).arg1 != rnum)
							 break; /* not this room */
			sprintf(buf2, "&y%sSet door %s as %s.",
							ZCMD(zone, subcmd).if_flag ? " then " : "",
							dirs[ZCMD(zone, subcmd).arg2],
							ZCMD(zone, subcmd).arg3 ? ((ZCMD(zone, subcmd).arg3 == 1) ? ((ZCMD(zone, subcmd).arg3 == 2) ? "hidden" : "closed") : "locked") : "open");
			found = TRUE;
			break;
		case 'T': /* Omit this case if the latest version of DGScripts is not installed */
			if (ZCMD(zone, subcmd).arg3 != rnum)
				break; /* not this room */
			sprintf(buf2, "&y%sAttach trigger %s [&c%d&y] to %s",
				ZCMD(zone, subcmd).if_flag ? " then " : "",
				trig_index[ZCMD(zone, subcmd).arg2]->proto->name,
				trig_index[ZCMD(zone, subcmd).arg2]->vnum,
				((ZCMD(zone, subcmd).arg1 == MOB_TRIGGER) ? "mobile" :
					((ZCMD(zone, subcmd).arg1 == OBJ_TRIGGER) ? "object" :
						((ZCMD(zone, subcmd).arg1 == WLD_TRIGGER)? "room" : "????"))));
			found = TRUE;
			break;
		case 'V':  /* Omit this case if the latest version of DGScripts is not installed */
			if (ZCMD(zone, subcmd).arg1 != WLD_TRIGGER)
							break; /* not a world trigger then it doesn't concern us. */
			if (ZCMD(zone, subcmd).arg3 != rnum)
							break; /* not this room */
			sprintf(buf2, "&y%sAssign global %s:%d to %s = %s",
				ZCMD(zone, subcmd).if_flag ? " then " : "",
				ZCMD(zone, subcmd).sarg1, ZCMD(zone, subcmd).arg2,
				((ZCMD(zone, subcmd).arg1 == MOB_TRIGGER) ? "mobile" :
					((ZCMD(zone, subcmd).arg1 == OBJ_TRIGGER) ? "object" :
						((ZCMD(zone, subcmd).arg1 == WLD_TRIGGER)? "room" : "????"))),
				ZCMD(zone, subcmd).sarg2);
			found = TRUE;
			break;
		default: /* Catch all */
			sprintf(buf2, "<Unknown Command>");
			found = TRUE;
			break;
		}
		/*
		 * Build the display buffer for this command.  
		 * Only show things that apply to this room.
		 */
		if (found == TRUE){  
						sprintf(buf1, "&n%d - %s&n\r\n", counter++, buf2);
						strcat(output, buf1);
		}
		subcmd++;
	}
	send_to_char(output, ch);

}


ACMD(do_vstat)
{
	struct char_data *mob;
	struct obj_data *obj;
	mob_vnum number;        /* or obj_vnum ... */
	mob_rnum r_num;        /* or obj_rnum ... */

	two_arguments(argument, buf, buf2);

	if (!*buf || !*buf2 || !isdigit(*buf2)) {
		send_to_char("Usage: vstat { obj | mob | room } <number>\r\n", ch);
		return;
	}
	if ((number = atoi(buf2)) < 0) {
		send_to_char("A NEGATIVE number??\r\n", ch);
		return;
	}
	if (is_abbrev(buf, "mob")) {
		if ((r_num = real_mobile(number)) < 0) {
			send_to_char("There is no monster with that number.\r\n", ch);
			return;
		}
		mob = read_mobile(r_num, REAL);
		char_to_room(mob, 0);
		do_stat_character(ch, mob);
		extract_char(mob);
	} else if (is_abbrev(buf, "obj")) {
		if ((r_num = real_object(number)) < 0) {
			send_to_char("There is no object with that number.\r\n", ch);
			return;
		}
		obj = read_object(r_num, REAL);
		do_stat_object(ch, obj);
		extract_obj(obj);
	} else if (is_abbrev(buf, "room")){
		if ((r_num = real_room(number)) < 0) {
			send_to_char("That room does not exist.\r\n", ch);
			return;
		}
		do_zstat_room(ch, r_num);
	} else
		send_to_char("That'll have to be either 'obj', 'mob', or 'room'.\r\n", ch);
}


/* Check the previous command in the list to determine whether
 * it was used in the same room or not 
 */
bool check_same_room(int zone, int subcmd, room_rnum rnum)
{
	/* sprintf(buf, "subcmd is %d", subcmd);
	mlog(buf); */
	
	if (subcmd <= 0)  /* Looks like we're already at the bottom, tough luck */
		return FALSE;
	
	if (ZCMD(zone, (subcmd-1)).if_flag) {  /* Need to go back another command */
		if (check_same_room(zone, (subcmd - 1), rnum) != TRUE)
			return FALSE; /* Checking previous command found no match to the rnum */
		else
			return TRUE;  /* Found it! */
	} else {
		switch (ZCMD(zone, (subcmd-1)).command) {
		case 'M':   /* Mobile */
		case 'V':                /* Variable - Used with DG Scripts, omit this case if you don't use them*/
		case 'O':                /* Object */
			if (ZCMD(zone, (subcmd-1)).arg3 == rnum)
				return TRUE;
			break;
		case 'R':        /* Room */
		case 'D':        /* Door */
			if (ZCMD(zone, (subcmd-1)).arg1 == rnum)
				return TRUE;
			break;
		default: break;
		}
	}
	return FALSE;
}


/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
	struct char_data *vict, *next_v;
	struct obj_data *obj, *next_o;

	one_argument(argument, buf);

	if (*buf) { /* argument supplied. destroy single object
							 * or char */
		if ((vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM, 1)) != NULL) {
			if (vict == ch) { // Can't purge yourself, hello.
				send_to_char("Fuuu!\r\n", ch);
				extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "%s tried suicide with the PURGE command.", GET_NAME(ch));
				return;
			}
			if (!IS_NPC(vict) && !GOT_RIGHTS(ch, RIGHTS_PLAYERS)) {
				send_to_char("You are not allowed to purge players!\r\n", ch);
				extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "%s tried to purge %s.", GET_NAME(ch), GET_NAME(vict));
				return;
			}
			if (!IS_NPC(vict) && (compare_rights(ch, vict) == -1)) {
				send_to_char("Fuuuuuuuuu!\r\n", ch);
				extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "%s tried to purge %s.", GET_NAME(ch), GET_NAME(vict));
				return;
			}
			act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

			if (!IS_NPC(vict)) {
				extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "%s has purged %s.", GET_NAME(ch), GET_NAME(vict));
				if (vict->desc) {
					STATE(vict->desc) = CON_CLOSE;
					vict->desc->character = NULL;
					vict->desc = NULL;
				}
			}
			extract_char(vict);
		} else if ((obj = get_obj_in_list_vis(ch, buf, NULL, world[IN_ROOM(ch)].contents)) != NULL) {
			act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
			extract_obj(obj);
		} else {
			send_to_char("Nothing here by that name.\r\n", ch);
			return;
		}

		send_to_char(OK, ch);
	} else {                        /* no argument. clean out the room */
		act("$n gestures... You are surrounded by scorching flames!",
				FALSE, ch, 0, 0, TO_ROOM);
		send_to_room("The world seems a little cleaner.\r\n", IN_ROOM(ch));

		for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v) {
			next_v = vict->next_in_room;
			if (IS_NPC(vict))
				extract_char(vict);
		}

		for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_o) {
			next_o = obj->next_content;
			extract_obj(obj);
		}
	}
}



const	char *logtypes[] = {
	"off", "brief", "normal", "complete", "\n"
};

ACMD(do_syslog)
{
	int tp;

	one_argument(argument, arg);

	if (!*arg) {
		tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
					(PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
		sprintf(buf, "Your syslog is currently %s.\r\n", logtypes[tp]);
		send_to_char(buf, ch);
		return;
	}
	if (((tp = search_block(arg, logtypes, FALSE)) == -1)) {
		send_to_char("Usage: syslog { Off | Brief | Normal | Complete }\r\n", ch);
		return;
	}
	REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
	SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

	sprintf(buf, "Your syslog is now %s.\r\n", logtypes[tp]);
	send_to_char(buf, ch);
}


#define	EXE_FILE "bin/circle" /* maybe use argv[0] but it's not reliable */

void initiate_copyover(bool panic) {
	FILE *fp;
	struct descriptor_data *d, *d_next;
	char buf[1024];

	*buf = '\0';

	fp = fopen (COPYOVER_FILE, "w");

	if (!fp) {
		mlog("Copyover file (%s) not writeable.", COPYOVER_FILE);
		return;
	}

	if (panic)
		strcpy(buf, "\r\n\x1B[1;31m \007\007\007PANIC! Unrecoverable MySQL error, emergency copyover initiated!\x1B[0;0m\r\n");
	else
		strcpy(buf, "\r\n           \x1B[1;31m \007\007\007Reality stops for a moment as space and time folds...\x1B[0;0m\r\n");
	mlog("COPYOVER: Printing descriptor name and host of connected players.");
	/*
	 * Write down copyover information needed at boot, including
	 * uptime, maxon, and what time we copyovered. Torgny Bjers, 2002-06-19
	 */
	fprintf(fp, "#%d\n"							// port
							"%d %d %d %d\n"			// no_specials, mini_mud, no_rent_check, bautosave
							"%lu %lu %d %d\n"		// uptime, maxontime, maxon, panic
							"%s%c\n"						// lib_dir
							"%s%c\n",						// mysql_db
							port,
							no_specials, mini_mud, no_rent_check, bautosave,
							boot_time, boot_high_time, boot_high, total_hotboots,
							lib_dir, STRING_TERMINATOR,
							mysql_db, STRING_TERMINATOR);
	/* For each playing descriptor, save its state */
	for (d = descriptor_list; d ; d = d_next) {
		struct char_data *och = d->character;
		d_next = d->next; /* We delete from the list , so need to save this */
		if (!d->character || (d->connected > CON_PLAYING && d->connected != CON_COPYOVER)) /* drop those logging on */ {
			write_to_descriptor (d->descriptor, "\r\nSorry, we are rebooting. Come back in a few seconds.\r\n");
			close_socket (d); /* throw'em out */
		} else {
			fprintf(fp, "%d %s %s\n", d->descriptor, GET_NAME(och), d->host);
			/* save och, and -all- items, even the unrentables */
			if (!panic) {
				crash_datasave(och, 0, RENT_CRASH);
				save_char(och, GET_ROOM_VNUM(IN_ROOM(och)), FALSE);
			}
			write_to_descriptor (d->descriptor, buf);
		}
	}

	fprintf(fp, "-1\n");
	fclose(fp);

	ispell_done();

	/* We do this during a shutdown, why not a copyover?
	 * Added house saving, mud time saving, crash_save_all
	 * 6.27.03 - Cheron
	 * Oops... looks like my Crash_save_all is what made the
	 * eq go missing...
	 * 8.13.03 - Cheron
	 */
	mlog("Saving player houses.");
	house_save_all();
	
	mlog("Saving all guilds.");
	save_all_guilds();
	
	mlog("Saving commands.");
	save_commands();

	mlog("Saving current MUD time.");
	write_mud_date_to_file();

	kill_mysql();

	/* exec - descriptors are inherited */

	*buf = '\0';

	sprintf(buf, "-C%d ", mother_desc);
	sprintf(buf + strlen(buf), "-d %s -D %s %d", lib_dir, mysql_db, port);
	mlog("COPYOVER: bin/circle %s", buf);

	exit_buffers();

	/* Ugh, seems it is expected we are 1 step above lib - this may be dangerous! */
	chdir("..");
	execl(EXE_FILE, "circle", buf, (char *) NULL);
	/* Failed - sucessful exec will not return */

	perror("do_copyover: execl");
	mlog("PANIC! Copyover failed!");

	exit (1); /* too much trouble to try to recover! */
}


/* (c) 1996-97 Erwin S. Andreasen <erwin@pip.dknet.dk> */
ACMD(do_copyover)
{
	struct descriptor_data *d;

	skip_spaces(&argument);
	if (!*argument || !strcmp(argument, "wait")) {
		for (d = descriptor_list; d; d=d->next) {
			if (STATE(d) == CON_PLAYING && !EDITING(d))
				send_to_char("Copyover initiated...\r\n", d->character);
		}
		in_copyover = 1;
	} else if (!strcmp(argument, "abort")) {
		for (d = descriptor_list; d; d=d->next) {
			if (STATE(d) == CON_COPYOVER && !EDITING(d)) {
				STATE(d) = CON_PLAYING;
				send_to_char("Copyover aborted.\r\n", d->character);
			}
		}
		in_copyover = 0;
	}	else if (!strcmp(argument, "now"))
		initiate_copyover(FALSE);
	else
		send_to_char("Usage: copyover [wait | now | abort]\r\n", ch);
}


void send_to_imms(char *msg)
{
	struct descriptor_data *pt;
	
	for (pt = descriptor_list; pt; pt = pt->next)
		if (!pt->connected && pt->character && IS_GOD(pt->character))
			send_to_char(msg, pt->character);
		
}

ACMD(do_restore)
{
	struct char_data *vict;
	int i;
  struct descriptor_data *d;

	one_argument(argument, buf);
	if (!*buf)
		send_to_char("Whom do you wish to restore?\r\n", ch);
  else if (!strcmp(buf, "all")) {
    for (d=descriptor_list; d; d = d->next) {
			if (STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) {
				GET_HIT(d->character) = GET_MAX_HIT(d->character);
				GET_MANA(d->character) = GET_MAX_MANA(d->character);
				GET_MOVE(d->character) = GET_MAX_MOVE(d->character);
				if (IS_IMMORTAL(d->character)) {
					GET_COND(d->character, THIRST) = -1;
					GET_COND(d->character, FULL) = -1;
					GET_COND(d->character, DRUNK) = -1;
				} else {
					if (GET_COND(d->character, THIRST) != -1) GET_COND(d->character, THIRST) = 24;
					if (GET_COND(d->character, FULL) != -1) GET_COND(d->character, FULL) = 24;
					if (GET_COND(d->character, DRUNK) != -1) GET_COND(d->character, DRUNK) = 0;
				}
				GET_FATIGUE(d->character) = FAT_ENERGIZED;
				send_to_charf(d->character, "You have been fully restored by %s!\r\n", GET_NAME(ch));
				}
		}
    send_to_char(OK, ch);
  } else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD, 1)))
		send_to_char(NOPERSON, ch);
	else {
		GET_HIT(vict) = GET_MAX_HIT(vict);
		GET_MANA(vict) = GET_MAX_MANA(vict);
		GET_MOVE(vict) = GET_MAX_MOVE(vict);
		GET_COND(vict, THIRST) = -1;
		GET_COND(vict, FULL) = -1;
		GET_COND(vict, DRUNK) = -1;
		GET_FATIGUE(vict) = FAT_ENERGIZED;

		if (IS_IMMORTAL(ch) && IS_IMMORTAL(vict)) {
			for (i = 1; i <= MAX_SKILLS; i++)
				SET_SKILL(vict, i, 10000);

			if (IS_IMMORTAL(vict)) {
				vict->real_abils.strength = 2500;
				vict->real_abils.agility = 2500;
				vict->real_abils.precision = 2500;
				vict->real_abils.perception = 2500;
				vict->real_abils.health = 32000;
				vict->real_abils.willpower = 2500;
				vict->real_abils.intelligence = 2500;
				vict->real_abils.charisma = 2500;
				vict->real_abils.luck = 2500;
				vict->real_abils.essence = 2500;
			}
		}
		update_pos(vict);
		update_groups(vict);
		affect_total(vict);

		send_to_char(OK, ch);
		send_to_charf(vict, "You have been fully restored by %s!\r\n", GET_NAME(ch));
	}
}


void perform_immort_vis(struct char_data *ch)
{
	if (GET_INVIS_LEV(ch) == RIGHTS_NONE && !AFF_FLAGGED(ch, AFF_HIDE | AFF_INVISIBLE)) {
		send_to_char("You are already fully visible.\r\n", ch);
		return;
	}
	 
	GET_INVIS_LEV(ch) = RIGHTS_NONE;
	appear(ch);
	send_to_char("You are now fully visible.\r\n", ch);
}


void perform_immort_invis(struct char_data *ch, bitvector_t rights)
{
	struct char_data *tch;

	for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
		if (tch == ch)
			continue;
		if (GOT_RIGHTS(tch, GET_INVIS_LEV(ch)) && !GOT_RIGHTS(tch, rights)) {
			 if (GET_TRAVELS(ch) && GET_TRAVELS(ch)->iout)
					strcpy(buf, GET_TRAVELS(ch)->iout);
			 else
					strcpy(buf, travel_defaults[TRAV_IOUT]);
			 act(buf, FALSE, ch, 0, tch, TO_VICT);
		}
		if ((rights == RIGHTS_NONE) || (!GOT_RIGHTS(tch, GET_INVIS_LEV(ch)) && GOT_RIGHTS(tch, rights))) {
			 if (GET_TRAVELS(ch) && GET_TRAVELS(ch)->iin)
					strcpy(buf, GET_TRAVELS(ch)->iin);
			 else
					strcpy(buf, travel_defaults[TRAV_IIN]);
			 act(buf, FALSE, ch, 0, tch, TO_VICT);
		}
	}

	GET_INVIS_LEV(ch) = rights;
	sprintf(buf, "You are now invisibile at (%s)!\r\n", wiz_char_rights(GET_INVIS_LEV(ch)));
	send_to_char(buf, ch);
}
	

ACMD(do_invis)
{
	bitvector_t rights;

	if (IS_NPC(ch)) {
		send_to_char("You can't do that!\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (!*arg) {
		if (GET_INVIS_LEV(ch) != RIGHTS_NONE)
			perform_immort_vis(ch);
		else
			perform_immort_invis(ch, get_max_rights(ch));
	} else {
		rights = wiz_bit_rights(arg);
		if (!GOT_RIGHTS(ch, rights))
			send_to_char("You can't go invisible above your own rights.\r\n", ch);
		else if (rights == RIGHTS_NONE)
			perform_immort_vis(ch);
		else
			perform_immort_invis(ch, rights);
	}
}


ACMD(do_gecho)
{
	struct descriptor_data *pt;
	char color = (GET_COLORPREF_ECHO(ch) ? GET_COLORPREF_ECHO(ch) : 'c');
	char *buf;

	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument)
		send_to_char("That must be a mistake...\r\n", ch);
	else {
		buf = get_buffer(MAX_STRING_LENGTH);
		parse_emote(argument, buf, color, 1); 
		for (pt = descriptor_list; pt; pt = pt->next)
			if ((STATE(pt) == CON_PLAYING || STATE(pt) == CON_COPYOVER) && !EDITING(pt) && pt->character && pt->character != ch)
				send_to_char(buf, pt->character);
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else
			send_to_char(buf, ch);
		release_buffer(buf);
	}
}


ACMD(do_dc)
{
	struct descriptor_data *d;
	int num_to_dc;

	one_argument(argument, arg);
	if (!(num_to_dc = atoi(arg))) {
		send_to_char("Usage: DC <user number> (type USERS for a list)\r\n", ch);
		return;
	}
	for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

	if (!d) {
		send_to_char("No such connection.\r\n", ch);
		return;
	}
	if (d->character && compare_rights(ch, d->character) <= 0) {
		if (!CAN_SEE(ch, d->character))
			send_to_char("No such connection.\r\n", ch);
		else
			send_to_char("Umm.. maybe that's not such a good idea...\r\n", ch);
		return;
	}

	/* We used to just close the socket here using close_socket(), but
	 * various people pointed out this could cause a crash if you're
	 * closing the person below you on the descriptor list.  Just setting
	 * to CON_CLOSE leaves things in a massively inconsistent state so I
	 * had to add this new flag to the descriptor. -je
	 *
	 * It is a much more logical extension for a CON_DISCONNECT to be used
	 * for in-game socket closes and CON_CLOSE for out of game closings.
	 * This will retain the stability of the close_me hack while being
	 * neater in appearance. -gg 12/1/97
	 *
	 * For those unlucky souls who actually manage to get disconnected
	 * by two different immortals in the same 1/10th of a second, we have
	 * the below 'if' check. -gg 12/17/99
	 */
	if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
		send_to_char("They're already being disconnected.\r\n", ch);
	else {  
		/*   
		 * Remember that we can disconnect people not in the game and
		 * that rather confuses the code when it expected there to be
		 * a character context.
		 */
		if (STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER)
			STATE(d) = CON_DISCONNECT;
		else
			STATE(d) = CON_CLOSE;

		sprintf(buf, "Connection #%d closed.\r\n", num_to_dc);
		send_to_char(buf, ch);
		extended_mudlog(BRF, SYSL_LINKS, TRUE, "Connection #%d @%s disconnected by %s.", num_to_dc, d->host, GET_NAME(ch));
	}
}



ACMD(do_wizlock)
{
	int value;
	const char *when;

	one_argument(argument, arg);
	if (*arg) {
		value = atoi(arg);
		if (value < 0 || value > (NUM_USER_RIGHTS-1)) {
			send_to_char("Invalid wizlock value.\r\n", ch);
			return;
		}
		circle_restrict = value;
		when = "now";
	} else
		when = "currently";

	switch (circle_restrict) {
	case 0:
		sprintf(buf, "The game is %s completely open.\r\n", when);
		break;
	case 1:
		sprintf(buf, "The game is %s closed to mortals.\r\n", when);
		break;
	default:
		sprintf(buf, "Only those with %s rights may enter the game %s.\r\n",
						user_rights[circle_restrict], when);
		break;
	}
	send_to_char(buf, ch);
}


ACMD(do_date)
{
	char *tmstr;
	time_t mytime;
	int d, h, m;

	if (subcmd == SCMD_DATE)
		mytime = time(0);
	else
		mytime = boot_time;

	tmstr = (char *) asctime(localtime(&mytime));
	*(tmstr + strlen(tmstr) - 1) = '\0';

	if (subcmd == SCMD_DATE)
		sprintf(buf, "Current machine time: %s\r\n", tmstr);
	else {
		mytime = time(0) - boot_time;
		d = mytime / 86400;
		h = (mytime / 3600) % 24;
		m = (mytime / 60) % 60;

		sprintf(buf, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d,
						((d == 1) ? "" : "s"), h, m);
	}

	send_to_char(buf, ch);
}



ACMD(do_last)
{
	struct char_data *vict = NULL;

	one_argument(argument, arg);
	if (!*arg) {
		send_to_char("For whom do you wish to search?\r\n", ch);
		return;
	}
	CREATE(vict, struct char_data, 1);
	clear_char(vict);
	CREATE(vict->player_specials, struct player_special_data, 1);
	if (load_char(arg, vict) <  0) {
		send_to_char("There is no such player.\r\n", ch);
		free_char(vict);
		return;
	}
	if ((compare_rights(ch, vict) == -1) && !IS_IMPL(ch)) {
		send_to_char("You are not sufficiently godly for that!\r\n", ch);
		return;
	}
	sprintf(buf, "[%5ld] [%s] %-12s : %-18s : %-20s",
				GET_IDNUM(vict), race_list[(int) GET_RACE(vict)].abbr, 
				GET_NAME(vict),	vict->player_specials->host && *vict->player_specials->host
				? vict->player_specials->host : "(NOHOST)",
				ctime(&vict->player.time.logon));
	send_to_char(buf, ch);
	free_char(vict);
}


ACMD(do_force)
{
	char buf1[MAX_STRING_LENGTH];
	struct descriptor_data *i, *next_desc;
	struct char_data *vict, *next_force;
	char to_force[MAX_INPUT_LENGTH + 2];

	half_chop(argument, arg, to_force);

	sprintf(buf1, "$n has forced you to '%s'.", to_force);

	if (!*arg || !*to_force)
		send_to_char("Whom do you wish to force do what?\r\n", ch);
	else if (!IS_GRGOD(ch) || (str_cmp("all", arg) && str_cmp("room", arg))) {
		if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD, 1)))
			send_to_char(NOPERSON, ch);
		else if (!IS_NPC(vict) && (compare_rights(ch, vict) <= 0))
			send_to_char("No, no, no!\r\n", ch);
		else {
			send_to_char(OK, ch);
			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			if (!IS_NPC(vict))
				extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
			command_interpreter(vict, to_force);
		}
	} else if (!str_cmp("room", arg)) {
		send_to_char(OK, ch);
		extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s forced room %d to %s",
								GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);

		for (vict = world[IN_ROOM(ch)].people; vict; vict = next_force) {
			next_force = vict->next_in_room;
			if (!IS_NPC(vict) && (compare_rights(ch, vict) <= 0))
				continue;
			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			command_interpreter(vict, to_force);
		}
	} else { /* force all */
		send_to_char(OK, ch);
		extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s forced all to %s", GET_NAME(ch), to_force);

		for (i = descriptor_list; i; i = next_desc) {
			next_desc = i->next;

			if (!(STATE(i) == CON_PLAYING || STATE(i) == CON_COPYOVER) || !(vict = i->character) || (!IS_NPC(vict) && (compare_rights(ch, vict) <= 0)))
				continue;
			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			command_interpreter(vict, to_force);
		}
	}
}


ACMD(do_wiznet)
{
	struct descriptor_data *d;
	char any = FALSE;
	bitvector_t rights = RIGHTS_IMMORTAL;

	if (get_max_rights(ch) == RIGHTS_NONE || get_max_rights(ch) == RIGHTS_NEWBIES) {
		send_to_char("Huh?!?\r\n", ch);
		return;
	}

	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument) {
		send_to_char("Usage: wiznet <text> | #<rights> <text> | wiz @<rights>\r\n", ch);
		return;
	}
	switch (*argument) {
	case '#':
		one_argument(argument + 1, buf1);
		half_chop(argument+1, buf1, argument);
		rights = MAX(wiz_bit_rights(buf1), RIGHTS_IMMORTAL);
		if (!GOT_RIGHTS(ch, rights)) {
			send_to_char("You can't wizline above your own rights.\r\n", ch);
			return;
		}
		break;
	case '@':
		for (d = descriptor_list; d; d = d->next) {
			if ((STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) && IS_IMMORTAL(d->character) &&
					!PRF_FLAGGED(d->character, PRF_NOWIZ) &&
					(CAN_SEE(ch, d->character) || IS_IMPL(ch))) {
				if (!any) {
					strcpy(buf1, "Gods online:\r\n");
					any = TRUE;
				}
				sprintf(buf1 + strlen(buf1), "  %s", GET_NAME(d->character));
				if (PLR_FLAGGED(d->character, PLR_OLC))
					strcat(buf1, " (OLC)\r\n");
				if (EDITING(d))
					strcat(buf1, " (Writing)\r\n");
				else
					strcat(buf1, "\r\n");

			}
		}
		any = FALSE;
		for (d = descriptor_list; d; d = d->next) {
			if ((STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) && IS_IMMORTAL(d->character) &&
					PRF_FLAGGED(d->character, PRF_NOWIZ) &&
					CAN_SEE(ch, d->character)) {
				if (!any) {
					strcat(buf1, "Gods offline:\r\n");
					any = TRUE;
				}
				sprintf(buf1 + strlen(buf1), "  %s\r\n", GET_NAME(d->character));
			}
		}
		send_to_char(buf1, ch);
		return;
	case '\\':
		++argument;
		break;
	default:
		if (!GOT_RIGHTS(ch, rights)){
		  send_to_char("You must specify a rights channel to wiznet on.", ch);
		  return;
		}
		break;
	}
	if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
		send_to_char("You are offline!\r\n", ch);
		return;
	}
	skip_spaces(&argument);

	if (!*argument) {
		send_to_char("Don't bother the gods like that!\r\n", ch);
		return;
	}
	if (rights != RIGHTS_IMMORTAL) {
		sprintf(buf1, "%s: <%s> %s\r\n", GET_NAME(ch), wiz_char_rights(rights), argument);
		sprintf(buf2, "Someone: <%s> %s\r\n", wiz_char_rights(rights), argument);
	} else {
		sprintf(buf1, "%s: %s\r\n", GET_NAME(ch), argument);
		sprintf(buf2, "Someone: %s\r\n", argument);
	}
	
	for (d = descriptor_list; d; d = d->next) {
		if ((STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) && (GOT_RIGHTS(d->character, rights)) &&
				(!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&
				!(EDITING(d) || PLR_FLAGGED(d->character, PLR_OLC))
				&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
			send_to_char(CCCYN(d->character, C_NRM), d->character);
			if (CAN_SEE(d->character, ch))
				send_to_char(buf1, d->character);
			else
				send_to_char(buf2, d->character);
			send_to_char(CCNRM(d->character, C_NRM), d->character);
		}
	}

	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
}



/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
	struct char_data *vict;
	long result;

	one_argument(argument, arg);

	if (!*arg)
		send_to_char("Yes, but for whom?!?\r\n", ch);
	else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD, 1)))
		send_to_char("There is no such player.\r\n", ch);
	else if (IS_NPC(vict))
		send_to_char("You can't do that to a mob!\r\n", ch);
	else if (compare_rights(ch, vict) == -1)
		send_to_char("Hmmm...you'd better not.\r\n", ch);
	else {
		switch (subcmd) {
		case SCMD_REROLL:
			send_to_char("Rerolled...\r\n", ch);
			roll_real_abils(vict);
			update_attribs(vict);
			extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
			sprintf(buf, "New stats: Str %d, Agl %d, Pre %d, Per %d, Hea %d\r\n"
				"Wil %d, Int %d, Cha %d, Luc %d, Ess %d\r\n",
					GET_STRENGTH(vict) / 100, GET_AGILITY(vict) / 100, GET_PRECISION(vict) / 100, 
					GET_PERCEPTION(vict) / 100, GET_HEALTH(vict) / 100, GET_WILLPOWER(vict) / 100, GET_INTELLIGENCE(vict) / 100, 
					GET_CHARISMA(vict) / 100, GET_LUCK(vict) / 100, GET_ESSENCE(vict) / 100);
			send_to_char(buf, ch);
			break;
		case SCMD_PARDON:
			if (!PLR_FLAGGED(vict, PLR_THIEF | PLR_KILLER)) {
				send_to_char("Your victim is not flagged.\r\n", ch);
				return;
			}
			REMOVE_BIT(PLR_FLAGS(vict), PLR_THIEF | PLR_KILLER);
			send_to_char("Pardoned.\r\n", ch);
			send_to_char("You have been pardoned by the Gods!\r\n", vict);
			extended_mudlog(NRM, SYSL_PENALTIES, TRUE, "%s pardoned by %s.", GET_NAME(vict), GET_NAME(ch));
			break;
		case SCMD_NOTITLE:
			result = PLR_TOG_CHK(vict, PLR_NOTITLE);
			extended_mudlog(NRM, SYSL_PENALTIES, TRUE, "Notitle %s for %s by %s.", ONOFF(result),
							GET_NAME(vict), GET_NAME(ch));
			break;
		case SCMD_SQUELCH:
			result = PLR_TOG_CHK(vict, PLR_MUTED);
		  if (result)
				send_to_char("You feel your voice taken away...\r\n", vict);
			else
				send_to_char("You feel your voice return.\r\n", vict);
			extended_mudlog(NRM, SYSL_PENALTIES, TRUE, "Squelch %s for %s by %s.", ONOFF(result),
							GET_NAME(vict), GET_NAME(ch));
			break;
		case SCMD_FREEZE:
			if (ch == vict) {
				send_to_char("Oh, yeah, THAT'S real smart...\r\n", ch);
				return;
			}
			if (PLR_FLAGGED(vict, PLR_FROZEN)) {
				send_to_char("Your victim is already pretty cold.\r\n", ch);
				return;
			}
			SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
			send_to_char("A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n", vict);
			send_to_char("Frozen.\r\n", ch);
			act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
			extended_mudlog(NRM, SYSL_PENALTIES, TRUE, "%s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
			break;
		case SCMD_THAW:
			if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
				send_to_char("Sorry, your victim is not morbidly encased in ice at the moment.\r\n", ch);
				return;
			}
			if (!IS_ADMIN(ch) && !IS_RPSTAFF(ch)) {
				sprintf(buf, "Sorry, you can't unfreeze %s.\r\n", GET_NAME(vict));
				send_to_char(buf, ch);
				return;
			}
			extended_mudlog(NRM, SYSL_PENALTIES, TRUE, "%s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
			REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
			send_to_char("A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n", vict);
			send_to_char("Thawed.\r\n", ch);
			act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
			break;
		case SCMD_UNAFFECT:
			if (vict->affected) {
				while (vict->affected)
					affect_remove(vict, vict->affected);
				send_to_char("There is a brief flash of light!\r\n"
										 "You feel slightly different.\r\n", vict);
				send_to_char("All spells removed.\r\n", ch);
			} else {
				send_to_char("Your victim does not have any affections!\r\n", ch);
				return;
			}
			break;
		case SCMD_STRIPRIGHTS:
			if (!GOT_RIGHTS(vict, RIGHTS_NONE)) {
				USER_RIGHTS(vict) = RIGHTS_NONE;
				SET_BIT(PLR_FLAGS(vict), PLR_NORIGHTS);
				save_char(vict, NOWHERE, FALSE);
				send_to_char("Player stripped of rights.\r\n", ch);
				send_to_char("You suddenly feel rather useless.\r\n", vict);
			} else {
				USER_RIGHTS(vict) = RIGHTS_MEMBER;
				REMOVE_BIT(PLR_FLAGS(vict), PLR_NORIGHTS);
				save_char(vict, NOWHERE, FALSE);
				send_to_char("Player rights restored.\r\n", ch);
				send_to_char("You feel useful again.\r\n", vict);
			}
			break;
		default:
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
			break;
		}
		save_char(vict, NOWHERE, FALSE);
	}
}


/* single zone printing fn used by "show zone" so it's not repeated in the
	 code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, zone_rnum zone)
{
	sprintf(bufptr, "%s%3d %-30.30s Age: %3d; Reset: %3d (%1d); Range: %5d-%5d\r\n",
					bufptr, zone_table[zone].number, zone_table[zone].name,
					zone_table[zone].age, zone_table[zone].lifespan,
					zone_table[zone].reset_mode,
					zone_table[zone].bot, zone_table[zone].top);
}


char *human_numbers(long value)
{
	char numbers[5];

	if (value < 1024)
		sprintf(numbers, "%ld", value);
	else if (value >= 1024 && value < 1024000)
		sprintf(numbers, "%ldk", value / 1024);
	else if (value >= 1024000)
		sprintf(numbers, "%ldM", value / 1024 / 1024);
	else
		strcpy(numbers, "0");

	return (strdup(numbers));
}


void show_player_list(struct char_data *ch)
{
	int i = 0, found = 0, num_players = 0;
	MYSQL_RES *playerindex;
	MYSQL_ROW row;
	char *printbuf = get_buffer(MAX_STRING_LENGTH);

	send_to_char("Registered players (&Rred&n denotes not in database):\r\n-------------------------------------------------------------------------------\r\n", ch);
	for (i = 0; i <= top_of_p_table; i++) {
		if (!*player_table[i].name) continue; /* player name is null - they've been deleted */
		if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT ID FROM %s WHERE Name='%s';", TABLE_PLAYER_INDEX, player_table[i].name))) {
			extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not load player index from table [%s].", TABLE_PLAYER_INDEX);
		}
		if ((row = mysql_fetch_row(playerindex))) found = 1;
		else found = 0;

		sprintf(printbuf + strlen(printbuf), "[%5ld] %s%-20s&n : %-20s", player_table[i].id, !found ? "&R" : "&w", player_table[i].name, ctime(&player_table[i].last));
		mysql_free_result(playerindex);
		num_players++;
	}
	
	sprintf(printbuf + strlen(printbuf), "%d players registered.\r\n", num_players);
	page_string(ch->desc, printbuf, TRUE);
	release_buffer(printbuf);
	return;
}


#define	SCR_WIDTH 79
#define	RETURN_CHAR '|'

ACMD(do_show)
{
	int i, j, k, l, con, in_game;                /* i, j, k to specifics? */
	zone_rnum zrn;
	zone_vnum zvn;
	char self = 0;
	struct char_data *vict = NULL;
	struct obj_data *obj;
	struct descriptor_data *d;
	char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], birth[80];
	char *printbuf;
	char *mysqlbuf1, *mysqlbuf2, *mysqlbuf3, *mysqlbuf4;
	MYSQL_RES *playerindex;
	MYSQL_ROW row;
	int pending_count, newbie_count;
	char *tmstr, *timebuf;
	time_t mytime;
	int day, hour, minute;
	int imm_count = 0, list_count = 0, bkg_count = 0;
	bitvector_t immrights = 0;

	struct show_struct {
		const char *cmd;
		const bitvector_t rights;
	} fields[] = {
		{	"nothing",		RIGHTS_NONE					},	/* 0 */
		{	"zones",			RIGHTS_MEMBER				},	/* 1 */
		{	"player",			RIGHTS_PLAYERS			},
		{	"rent",				RIGHTS_ADMIN				},
		{	"stats",			RIGHTS_IMMORTAL			},
		{	"errors",			RIGHTS_HEADBUILDER	},	/* 5 */
		{	"death",			RIGHTS_ADMIN				},
		{	"godrooms",		RIGHTS_ADMIN				},
		{	"shops",			RIGHTS_BUILDING			},
		{	"houses",			RIGHTS_BUILDING			},
		{	"snoop",			RIGHTS_ADMIN				},	/* 10 */
		{	"assemblies",	RIGHTS_HEADBUILDER	},
		{	"linkdead",		RIGHTS_ADMIN				},
		{	"buffers",		RIGHTS_DEVELOPER		},
		{	"imms",				RIGHTS_IMMORTAL			},
		{ "players",		RIGHTS_PLAYERS			},
		{	"\n",					RIGHTS_NONE					}
	};

	skip_spaces(&argument);

	if (!*argument) {
		strcpy(buf, "Show options:\r\n-------------------------------------------------------------------------------\r\n");
		for (j = 0, i = 1; fields[i].rights != RIGHTS_NONE; i++)
			if (GOT_RIGHTS(ch, fields[i].rights))
				sprintf(buf + strlen(buf), "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
		return;
	}

	strcpy(arg, two_arguments(argument, field, value));

	for (l = 0; *(fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, fields[l].cmd, strlen(field)))
			break;

	if (!GOT_RIGHTS(ch, fields[l].rights) && (fields[l].rights != RIGHTS_NONE)) {
		send_to_char("You are not godly enough for that!\r\n", ch);
		return;
	}
	if (!strcmp(value, "."))
		self = 1;
	buf[0] = '\0';
	switch (l) {

	case 1:	/* zone */
		/* tightened up by JE 4/6/93 */
		if (self)
			print_zone_to_buf(buf, world[IN_ROOM(ch)].zone);
		else if (*value && is_number(value)) {
			for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++);
			if (zrn <= top_of_zone_table)
				print_zone_to_buf(buf, zrn);
			else {
				send_to_char("That is not a valid zone.\r\n", ch);
				return;
			}
		} else
			for (zrn = 0; zrn <= top_of_zone_table; zrn++)
				print_zone_to_buf(buf, zrn);
			page_string(ch->desc, buf, TRUE);
			break;

	case 2:	/* player */
		if (!*value) {
			send_to_char("A name would help.\r\n", ch);
			return;
		}

		CREATE(vict, struct char_data, 1);
		clear_char(vict);
		CREATE(vict->player_specials, struct player_special_data, 1);
		if (load_char(value, vict) < 0) {
			send_to_char("There is no such player.\r\n", ch);
			free_char(vict);
			return;
		}
		sprintf(buf, "Player: %-12s (%s) [%s %s]\r\n", GET_NAME(vict),
			genders[(int) GET_SEX(vict)], wiz_char_rights(get_max_rights(vict)), race_list[(int)GET_RACE(vict)].abbr);
		sprintf(buf + strlen(buf),
			"Au: %-8d  Bal: %-8d  Exp: %-8d  Align: %-5d  Lessons: %-3d\r\n",
						GET_GOLD(vict), GET_BANK_GOLD(vict), GET_EXP(vict),
						GET_ALIGNMENT(vict), GET_PRACTICES(vict));
		strcpy(birth, ctime(&vict->player.time.birth));
		sprintf(buf + strlen(buf),
			"Started: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n",
						birth, ctime(&vict->player.time.logon),
						(int) (vict->player.time.played / 3600),
						(int) (vict->player.time.played / 60 % 60));
		send_to_char(buf, ch);
		free_char(vict);
		break;

	case 3:	/* rent */
		if (!*value) {
			send_to_char("A name would help.\r\n", ch);
			return;
		}
		Crash_listrent(ch, value);
		break;

	case 4:	/* stats */
		i = 0;
		j = 0;
		k = 0;
		con = 0;
		for (vict = character_list; vict; vict = vict->next) {
			if (IS_NPC(vict))
				j++;
			else if (CAN_SEE(ch, vict)) {
				i++;
				if (vict->desc)
					con++;
			}
		}

		in_game = i;

    if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT COUNT(ID) FROM %s WHERE active = 0;", TABLE_PLAYER_INDEX))) {
      extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading player records from [%s].", TABLE_PLAYER_INDEX);
      return;
    }
    row = mysql_fetch_row(playerindex);
    pending_count = atoi(row[0]);
    mysql_free_result(playerindex);
    
    if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT COUNT(Played) FROM %s WHERE Played < 86400;", TABLE_PLAYER_INDEX))) {
      extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading player records from [%s].", TABLE_PLAYER_INDEX);
      return;
    }
    row = mysql_fetch_row(playerindex);
    newbie_count = atoi(row[0]);
    mysql_free_result(playerindex);

    if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT COUNT(OKtoMail) FROM %s WHERE OKtoMail = 1;", TABLE_PLAYER_INDEX))) {
      extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading player records from [%s].", TABLE_PLAYER_INDEX);
      return;
    }
    row = mysql_fetch_row(playerindex);
    list_count = atoi(row[0]);
    mysql_free_result(playerindex);

    if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT COUNT(Name) FROM %s WHERE LENGTH(Background) > 10;", TABLE_PLAYER_INDEX))) {
      extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading player records from [%s].", TABLE_PLAYER_INDEX);
      return;
    }
    row = mysql_fetch_row(playerindex);
    bkg_count = atoi(row[0]);
    mysql_free_result(playerindex);

		for (obj = object_list; obj; obj = obj->next)
			k++;

		for (i = 0; i < top_of_p_table; i++)
			if (player_table[i].rights > RIGHTS_MEMBER)
				imm_count++;

		printbuf = get_buffer(MAX_STRING_LENGTH);
		//                 1        10        20        30        40        50        60        70       79
		strcpy(printbuf,  "&c.------------------------[ &CArcane Realms Statistics &c]------------------------.\r\n");
		sprintf(printbuf, "%s| &GWorld Information        Player Stats                Buffer Information    &c|\r\n", printbuf);
		sprintf(printbuf, "%s| &gZones          : &Y%5d   &gRegistered players : &Y%4d   &gLarge buffers : &Y%5d &c|\r\n", printbuf, top_of_zone_table + 1, top_of_p_table + 1, buf_largecount);
		sprintf(printbuf, "%s| &gRooms          : &Y%5d   &gConnected players  : &Y%4d   &gSwitches      : &Y%5d &c|\r\n", printbuf, top_of_world + 1, con, buf_switches);
		sprintf(printbuf, "%s| &gUnique mobiles : &Y%5d   &gPlayers in game    : &Y%4d   &gOverflows     : &Y%5d &c|\r\n", printbuf, top_of_mobt + 1, in_game, buf_overflows);
		sprintf(printbuf, "%s| &gMobiles in use : &Y%5d   &gTotal newbie count : &Y%4d   &gCache hits    : &Y%5.5s &c|\r\n", printbuf, j, newbie_count, human_numbers(buffer_cache_stat(BUFFER_CACHE_HITS)));
		sprintf(printbuf, "%s| &gUnique objects : &Y%5d   &gUnapproved players : &Y%4d   &gCache misses  : &Y%5.5s &c|\r\n", printbuf, top_of_objt + 1, pending_count, human_numbers(buffer_cache_stat(BUFFER_CACHE_MISSES)));
		sprintf(printbuf, "%s| &gObjects in use : &Y%5d   &gImmortal players   : &Y%4d                         &c|\r\n", printbuf, k, imm_count);
		sprintf(printbuf, "%s| &gTriggers       : &Y%5d   &gPlayers ok to mail : &Y%4d                         &c|\r\n", printbuf, top_of_trigt + 1, list_count);
		sprintf(printbuf, "%s| &gAuto-Quests    : &Y%5d   &gWith backgrounds   : &Y%4d                         &c|\r\n", printbuf, top_of_aquestt + 1, bkg_count);
		sprintf(printbuf, "%s| &gAssemblies     : &Y%5ld   &gPlayer Houses      : &Y%4d                         &c|\r\n", printbuf, g_lNumAssemblies + 1, house_index ? top_of_houset + 1 : 0);
		sprintf(printbuf, "%s| &gShops          : &Y%5d                                                     &c|\r\n", printbuf, shop_index ? top_shop + 1 : 0);
		sprintf(printbuf, "%s|----------------------------[ &CServer Statistics &c]---------------------------|\r\n", printbuf);
		sprintf(printbuf, "%s| &gThe server accepts incoming telnet connections to port &G%-5d               &c|\r\n", printbuf, port);
		timebuf = get_buffer(256);

		mytime = time(0);
		tmstr = (char *) asctime(localtime(&mytime));
		*(tmstr + strlen(tmstr) - 1) = '\0';
		sprintf(timebuf, "&gThe current system time is &G%s", tmstr);
		sprintf(printbuf, "%s| %-74.74s     &c|\r\n", printbuf, timebuf);

		mytime = boot_time;
		tmstr = (char *) asctime(localtime(&mytime));
		*(tmstr + strlen(tmstr) - 1) = '\0';
		mytime = time(0) - boot_time;
		day = mytime / 86400;
		hour = (mytime / 3600) % 24;
		minute = (mytime / 60) % 60;
		sprintf(timebuf, "&gThe server last booted up &G%s", tmstr);
		sprintf(printbuf, "%s| %-74.74s     &c|\r\n", printbuf, timebuf);
		sprintf(timebuf, "&gWhich was &G%d &gday%s, &G%d &ghour%s, and &G%d &gminute%s ago,", day, ((day == 1) ? "" : "s"), hour, ((hour == 1) ? "" : "s"), minute, ((minute == 1) ? "" : "s"));
		sprintf(printbuf, "%s| %-74.74s               &c|\r\n", printbuf, timebuf);
		sprintf(timebuf, "&gsince then the max players on has been &G%-4d", boot_high);
		sprintf(printbuf, "%s| %-74.74s     &c|\r\n", printbuf, timebuf);

		mytime = boot_high_time;
		tmstr = (char *) asctime(localtime(&mytime));
		*(tmstr + strlen(tmstr) - 1) = '\0';
		mytime = time(0) - boot_high_time;
		day = mytime / 86400;
		hour = (mytime / 3600) % 24;
		minute = (mytime / 60) % 60;
		sprintf(timebuf, "&gThis was &G%d &gday%s, &G%d &ghour%s, and &G%d &gminute%s ago", day, ((day == 1) ? "" : "s"), hour, ((hour == 1) ? "" : "s"), minute, ((minute == 1) ? "" : "s"));
		sprintf(printbuf, "%s| %-74.74s               &c|\r\n", printbuf, timebuf);
		sprintf(timebuf, "&gat &G%s&g system time.", tmstr);
		sprintf(printbuf, "%s| %-74.74s       &c|\r\n", printbuf, timebuf);

		if (copyover_time > 0) {
			mytime = copyover_time;
			tmstr = (char *) asctime(localtime(&mytime));
			*(tmstr + strlen(tmstr) - 1) = '\0';
			mytime = time(0) - copyover_time;
			day = mytime / 86400;
			hour = (mytime / 3600) % 24;
			minute = (mytime / 60) % 60;
			sprintf(timebuf, "&gThe server last hotbooted &G%s&g, &G%d&g total hotboots", tmstr, total_hotboots);
			sprintf(printbuf, "%s| %-79.79s      &c|\r\n", printbuf, timebuf);
			sprintf(timebuf, "&gWhich was &G%d &gday%s, &G%d &ghour%s, and &G%d &gminute%s ago.", day, ((day == 1) ? "" : "s"), hour, ((hour == 1) ? "" : "s"), minute, ((minute == 1) ? "" : "s"));
			sprintf(printbuf, "%s| %-74.74s               &c|\r\n", printbuf, timebuf);
		}

		release_buffer(timebuf);

		sprintf(printbuf, "%s|----------------------------[ &CMySQL Statistics &c]----------------------------|\r\n", printbuf);
		mysqlbuf1 = get_buffer(256);
		mysqlbuf2 = get_buffer(256);
		mysqlbuf3 = get_buffer(256);
		mysqlbuf4 = get_buffer(256);
		sprintf(mysqlbuf1, "&gMySQL connection established with &G%s&g,", mysql_get_host_info(&mysql));
		sprintf(mysqlbuf2, "&gusing MySQL C API library &G%s&g.", mysql_get_client_info());		
		sprintf(mysqlbuf3, "&gUsing database &G%s&g.", mysql_db);		
		sprintf(mysqlbuf4, "&g%s", mysql_stat(&mysql));		
		sprintf(printbuf, "%s| %-74.74s       &c|\r\n", printbuf, mysqlbuf1);
		sprintf(printbuf, "%s| %-74.74s       &c|\r\n", printbuf, mysqlbuf2);
		sprintf(printbuf, "%s| %-74.74s       &c|\r\n", printbuf, mysqlbuf3);
		sprintf(printbuf, "%s| %-74.74s   &c|\r\n", printbuf, mysqlbuf4);
		sprintf(printbuf, "%s'----------------------------------------------------------------------------'&n\r\n", printbuf);
		page_string(ch->desc, printbuf, TRUE);
		release_buffer(printbuf);
		release_buffer(mysqlbuf1);
		release_buffer(mysqlbuf2);
		release_buffer(mysqlbuf3);
		release_buffer(mysqlbuf4);
		break;

	case 5:	/* room errors */
		strcpy(buf, "Errant Rooms\r\n-------------------------------------------------------------------------------\r\n");
		for (i = 0, k = 0; i <= top_of_world; i++)
			for (j = 0; j < NUM_OF_DIRS; j++)
				if (world[i].dir_option[j] && world[i].dir_option[j]->to_room == 0)
					sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k, GET_ROOM_VNUM(i),
									world[i].name);
		page_string(ch->desc, buf, TRUE);
		break;

	case 6:	/* death traps */
		strcpy(buf, "Death Traps\r\n-------------------------------------------------------------------------------\r\n");
		for (i = 0, j = 0; i <= top_of_world; i++)
			if (ROOM_FLAGGED(i, ROOM_DEATH))
				sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j,
								GET_ROOM_VNUM(i), world[i].name);
		page_string(ch->desc, buf, TRUE);
		break;

	case 7:	/* godrooms */
		strcpy(buf, "Godrooms\r\n-------------------------------------------------------------------------------\r\n");
		for (i = 0, j = 0; i <= top_of_world; i++)
		if (ROOM_FLAGGED(i, ROOM_GODROOM))
			sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n",
								++j, GET_ROOM_VNUM(i), world[i].name);
		page_string(ch->desc, buf, TRUE);
		break;

	case 8:	/* shops */
		show_shops(ch, value);
		break;

	case 9:	/* houses */
		hcontrol_list_houses(ch);
		break;

	case 10: /* snoops */
		*buf = '\0';
		send_to_char("People currently snooping:\r\n-------------------------------------------------------------------------------\r\n", ch);
		for (d = descriptor_list; d; d = d->next) {
			if (d->snooping == NULL || d->character == NULL)
				continue;
			if (!(STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) || (compare_rights(ch, d->character) == -1))
				continue;
			if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
				continue;
			sprintf(buf + strlen(buf), "%-10s - snooped by %s.\r\n",
							 GET_NAME(d->snooping->character), GET_NAME(d->character));
		}
		send_to_char(*buf ? buf : "No one is currently snooping.\r\n", ch);
		break;

	case 11: /* assemblies */
		assemblyListToChar(ch);
		break;

	case 12: /* linkdead */
		send_to_char("Players currently linkdead:\r\n-------------------------------------------------------------------------------\r\n", ch);
		i = 0;
		for (vict = character_list; vict; vict = vict->next) {
			if (!IS_NPC(vict) && !vict->desc) 
				sprintf(buf + strlen(buf), "%2d) [%5d] %s\r\n", ++i, world[IN_ROOM(vict)].number, GET_NAME(vict));
		}
		sprintf(buf + strlen(buf), "%d currently linkdead.\r\n", i);
		page_string(ch->desc, buf, TRUE);
		break;

	case 13: /* buffers */
		show_buffers(ch, -1, 1);
		show_buffers(ch, -1, 2);
		show_buffers(ch, -1, 0);
		break;

	case 14: /* imms */
		immrights = RIGHTS_MEMBER;
		immrights |= RIGHTS_IMMORTAL;
		send_to_char("Players with Immortal rights:\r\n-------------------------------------------------------------------------------\r\n", ch);
		for (i = 0; i < top_of_p_table; i++) {
			if (player_table[i].rights >= immrights) {
				send_to_charf(ch, "%s\r\n", player_table[i].name);
				imm_count++;
			}
		}
		send_to_charf(ch, "%d immortals registered.\r\n", imm_count);
		break;
	
	case 15: /* players */
		show_player_list(ch);
		break;

	default: /* not a known show method */
		send_to_char("Sorry, I don't understand that.\r\n", ch);
		break;
	}
}

#undef SCR_WIDTH
#undef RETURN_CHAR


/***************** The do_set function ***********************************/

#define	PC						1
#define	NPC						2
#define	BOTH					3

#define	MISC					0
#define	BINARY        1
#define	NUMBER        2

#define	SET_OR_REMOVE(flagset, flags) { \
				if (on) SET_BIT(flagset, flags); \
				else if (off) REMOVE_BIT(flagset, flags); }

#define	RANGE(low, high) (value = MAX((low), MIN((high), (value))))


/* The set options available */
	struct set_struct {
		const char *cmd;
		const bitvector_t rights;
		const char pcnpc;
		const char type;
	} set_fields[] = {
	 { "brief",					RIGHTS_PLAYERS,			PC,			BINARY },	/* 0 */
	 { "invstart",			RIGHTS_PLAYERS,			PC,			BINARY },	/* 1 */
	 { "title",					RIGHTS_PLAYERS,			PC,     MISC },
	 { "nosummon",			RIGHTS_ADMIN,				PC,			BINARY },
	 { "maxhit",				RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "maxmana",				RIGHTS_PLAYERS,			BOTH,		NUMBER },	/* 5 */
	 { "maxmove",				RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "hit",						RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "mana",					RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "move",					RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "align",					RIGHTS_PLAYERS,			BOTH,		NUMBER },	/* 10 */
	 { "strength",			RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "questpoints",		RIGHTS_PLAYERS,			PC,			NUMBER },
	 { "agility",				RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "precision",			RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "perception",		RIGHTS_PLAYERS,			BOTH,		NUMBER },	/* 15 */
	 { "health",				RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "willpower",			RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "intelligence",	RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "charisma",			RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "luck",					RIGHTS_PLAYERS,			BOTH,		NUMBER },	/* 20 */
	 { "essence",				RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "ac",						RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "gold",					RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "bank",					RIGHTS_PLAYERS,			PC,			NUMBER },
	 { "exp",						RIGHTS_PLAYERS,			BOTH,		NUMBER },	/* 25 */
	 { "hitroll",				RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "damroll",				RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "invis",					RIGHTS_IMPLEMENTOR,	PC,			MISC },
	 { "nohassle",			RIGHTS_ADMIN,				PC,			BINARY },
	 { "frozen",				RIGHTS_PLAYERS,			PC,			BINARY },	/* 30 */
	 { "practices",			RIGHTS_ADMIN,				PC,			NUMBER },
	 { "lessons",				RIGHTS_ADMIN,				PC,			NUMBER },
	 { "drunk",					RIGHTS_PLAYERS,			BOTH,		MISC },
	 { "hunger",				RIGHTS_PLAYERS,			BOTH,		MISC },
	 { "thirst",				RIGHTS_PLAYERS,			BOTH,		MISC },		/* 35 */
	 { "killer",				RIGHTS_PLAYERS,			PC,			BINARY },
	 { "thief",					RIGHTS_PLAYERS,			PC,			BINARY },
	 { "room",					RIGHTS_IMPLEMENTOR,	BOTH,		NUMBER },  
	 { "roomflag",			RIGHTS_ADMIN,				PC,			BINARY },
	 { "siteok",				RIGHTS_ADMIN,				PC,			BINARY },	/* 40 */
	 { "deleted",				RIGHTS_IMPLEMENTOR,	PC,			BINARY },
	 { "culture",				RIGHTS_PLAYERS,			BOTH,		MISC },
	 { "nowizlist",			RIGHTS_IMPLEMENTOR,	PC,			BINARY },
	 { "quest",					RIGHTS_PLAYERS,			PC,			BINARY },
	 { "loadroom",			RIGHTS_ADMIN,				PC,			MISC },		/* 45 */
	 { "color",					RIGHTS_PLAYERS,			PC,			BINARY },
	 { "idnum",					RIGHTS_IMPLEMENTOR,	PC,			NUMBER },
	 { "passwd",				RIGHTS_IMPLEMENTOR,	PC,			MISC },
	 { "nodelete",			RIGHTS_PLAYERS,			PC,			BINARY },
	 { "sex",						RIGHTS_PLAYERS,			BOTH,		MISC },		/* 50 */
	 { "age",						RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "height",				RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "weight",				RIGHTS_PLAYERS,			BOTH,		NUMBER }, 
	 { "olc",						RIGHTS_HEADBUILDER,	PC,			NUMBER },
	 { "race",					RIGHTS_ADMIN,				BOTH,		MISC },		/* 55 */
	 { "email",					RIGHTS_PLAYERS,			PC,			MISC },
	 { "hometown",			RIGHTS_PLAYERS,			PC,			NUMBER },
   { "contact",				RIGHTS_PLAYERS,			PC,			MISC },
   { "forcerename",		RIGHTS_PLAYERS,			PC,			BINARY },
   { "eyecolor",			RIGHTS_PLAYERS,			PC,			NUMBER },	/* 60 */
   { "haircolor",			RIGHTS_PLAYERS,			PC,			NUMBER },
   { "hairstyle",			RIGHTS_PLAYERS,			PC,			NUMBER },
	 { "skintone",			RIGHTS_PLAYERS,			PC,			NUMBER },
	 { "rights",				RIGHTS_IMPLEMENTOR,	PC,			MISC },
	 { "xsyslog",				RIGHTS_ADMIN,				PC,			MISC },		/* 65 */
	 { "newbie",				RIGHTS_PLAYERS,			PC,			BINARY },
	 { "truename",			RIGHTS_PLAYERS,			PC,			MISC },
	 { "rpxp",					RIGHTS_PLAYERS,			PC,			NUMBER },
	 { "fatigue",				RIGHTS_PLAYERS,			BOTH,		MISC },
	 { "flux",					RIGHTS_PLAYERS,			BOTH,		NUMBER },	/* 70 */
	 { "maxflux",				RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "piety",					RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "reputation",		RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "socialrank",		RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "militaryrank",	RIGHTS_PLAYERS,			BOTH,		NUMBER },	/* 75 */
	 { "sanity",				RIGHTS_PLAYERS,			BOTH,		NUMBER },
	 { "sdesc",					RIGHTS_PLAYERS,			PC,			MISC },
	 { "ldesc",					RIGHTS_PLAYERS,			PC,			MISC },
	 { "skillcap",		  RIGHTS_PLAYERS,			PC,			NUMBER },
	 { "\n",						RIGHTS_NONE,				BOTH,		MISC }
	};


int	perform_set(struct char_data *ch, struct char_data *vict, int mode,
								char *val_arg)
{
	int i, on = 0, off = 0, value = 0;
	room_rnum rnum;
	room_vnum rvnum;
	char output[MAX_STRING_LENGTH];

	/* Check to make sure all the levels are correct */
	if (!IS_IMPL(ch)) {
		if (!IS_NPC(vict) && (compare_rights(ch, vict) <= 0) && vict != ch) {
			send_to_char("Maybe that's not such a great idea...\r\n", ch);
			return (0);
		}
	}
	if (!(GOT_RIGHTS(ch, set_fields[mode].rights) || IS_IMPL(ch))) {
		send_to_char("You are not godly enough for that!\r\n", ch);
		return (0);
	}

	/* Make sure the PC/NPC is correct */
	if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
		send_to_char("You can't do that to a beast!\r\n", ch);
		return (0);
	} else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
		send_to_char("That can only be done to a beast!\r\n", ch);
		return (0);
	}

	/* Find the value of the argument */
	if (set_fields[mode].type == BINARY) {
		if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
			on = 1;
		else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
			off = 1;
		if (!(on || off)) {
			send_to_char("Value must be 'on' or 'off'.\r\n", ch);
			return (0);
		}
		sprintf(output, "%s %s for %s.", set_fields[mode].cmd, ONOFF(on),
						GET_NAME(vict));
	} else if (set_fields[mode].type == NUMBER) {
		value = atoi(val_arg);
		sprintf(output, "%s's %s set to %d.", GET_NAME(vict),
						set_fields[mode].cmd, value);
	} else {
		strcpy(output, "Okay.");  /* can't use OK macro here 'cause of \r\n */
	}

	switch (mode) {
	case 0:
		SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
		break;
	case 1:
		SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
		break;
	case 2:
		set_title(vict, val_arg);
		sprintf(output, "%s's title is now: %s", GET_NAME(vict), GET_TITLE(vict));
		break;
	case 3:
		SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
		sprintf(output, "Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
		break;
	case 4:
		vict->points.max_hit = RANGE(1, 5000);
		affect_total(vict);
		break;
	case 5:
		vict->points.max_mana = RANGE(1, 5000);
		affect_total(vict);
		break;
	case 6:
		vict->points.max_move = RANGE(1, 5000);
		affect_total(vict);
		break;
	case 7:
		vict->points.hit = RANGE(-9, vict->points.max_hit);
		affect_total(vict);
		break;
	case 8:
		vict->points.mana = RANGE(0, vict->points.max_mana);
		affect_total(vict);
		break;
	case 9:
		vict->points.move = RANGE(0, vict->points.max_move);
		affect_total(vict);
		break;
	case 10:
		GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
		affect_total(vict);
		break;
	case 11:
		if (IS_NPC(vict) || IS_IMMORTAL(vict))
			vict->real_abils.strength = RANGE(300, 10000);
		else
			vict->real_abils.strength = RANGE(300, 5000);
		affect_total(vict);
		break;
	case 12:
		if (value < 0) {
			send_to_char("You have to supply a number.\r\n", ch);
			return (0);
		}
		GET_QP(vict) = RANGE(0, 100000000);
		break;
	case 13:
		if (IS_NPC(vict) || IS_IMMORTAL(vict))
			vict->real_abils.agility = RANGE(300, 10000);
		else
			vict->real_abils.agility = RANGE(300, 5000);
		affect_total(vict);
		break;
	case 14:
		if (IS_NPC(vict) || IS_IMMORTAL(vict))
			vict->real_abils.precision = RANGE(300, 10000);
		else
			vict->real_abils.precision = RANGE(300, 5000);
		affect_total(vict);
		break;
	case 15:
		if (IS_NPC(vict) || IS_IMMORTAL(vict))
			vict->real_abils.perception = RANGE(300, 10000);
		else
			vict->real_abils.perception = RANGE(300, 5000);
		affect_total(vict);
		break;
	case 16:
		if (IS_NPC(vict) || IS_IMMORTAL(vict))
			vict->real_abils.health = RANGE(300, 32000);
		else
			vict->real_abils.health = RANGE(300, 32000);
		break;
	case 17:
		if (IS_NPC(vict) || IS_IMMORTAL(vict))
			vict->real_abils.willpower = RANGE(300, 10000);
		else
			vict->real_abils.willpower = RANGE(300, 5000);
		affect_total(vict);
		break;
	case 18:
		if (IS_NPC(vict) || IS_IMMORTAL(vict))
			vict->real_abils.intelligence = RANGE(300, 10000);
		else
			vict->real_abils.intelligence = RANGE(300, 5000);
		affect_total(vict);
		break;
	case 19:
		if (IS_NPC(vict) || IS_IMMORTAL(vict))
			vict->real_abils.charisma = RANGE(300, 10000);
		else
			vict->real_abils.charisma = RANGE(300, 5000);
		affect_total(vict);
		break;
	case 20:
		if (IS_NPC(vict) || IS_IMMORTAL(vict))
			RANGE(300, 10000);
		else
			RANGE(300, 5000);
		vict->real_abils.luck = value;
		affect_total(vict);
		break;
	case 21:
		if (IS_NPC(vict) || IS_IMMORTAL(vict))
			vict->real_abils.essence = RANGE(300, 10000);
		else
			vict->real_abils.essence = RANGE(300, 5000);
		affect_total(vict);
		break;
	case 22:
		vict->points.armor = RANGE(-100, 100);
		affect_total(vict);
		break;
	case 23:
		GET_GOLD(vict) = RANGE(0, 100000000);
		break;
	case 24:
		GET_BANK_GOLD(vict) = RANGE(0, 100000000);
		break;
	case 25:
		if(!IS_IMPL(vict))
			vict->points.exp = RANGE(0, EXP_MAX - 50000000);
		else
			vict->points.exp = RANGE(0, EXP_MAX);
		break;
	case 26:
		vict->points.hitroll = RANGE(-20, 20);
		affect_total(vict);
		break;
	case 27:
		vict->points.damroll = RANGE(-20, 20);
		affect_total(vict);
		break;
	case 28:
		if (!IS_IMPLEMENTOR(ch) && ch != vict) {
			send_to_char("You aren't godly enough for that!\r\n", ch);
			return (0);
		}
		GET_INVIS_LEV(vict) = wiz_bit_rights(val_arg);
		break;
	case 29:
		if (!IS_IMPL(ch) && ch != vict) {
			send_to_char("You aren't godly enough for that!\r\n", ch);
			return (0);
		}
		SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
		break;
	case 30:
		if (ch == vict && on) {
			send_to_char("Better not -- could be a long winter!\r\n", ch);
			return (0);
		}
		SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
		break;
	case 31:
	case 32:
		GET_PRACTICES(vict) = RANGE(0, 100);
		break;
	case 33:
	case 34:
	case 35:
		if (!str_cmp(val_arg, "off")) {
			GET_COND(vict, (mode - 33)) = (char) -1; /* warning: magic number here */
			sprintf(output, "%s's %s now off.", GET_NAME(vict), set_fields[mode].cmd);
		} else if (is_number(val_arg)) {
			value = atoi(val_arg);
			RANGE(0, 24);
			GET_COND(vict, (mode - 33)) = (char) value; /* and here too */
			sprintf(output, "%s's %s set to %d.", GET_NAME(vict),
							set_fields[mode].cmd, value);
		} else {
			send_to_char("Must be 'off' or a value from 0 to 24.\r\n", ch);
			return (0);
		}
		break;
	case 36:
		SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
		break;
	case 37:
		SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
		break;
	case 38:
		if ((rnum = real_room(value)) < 0) {
			send_to_char("No room exists with that number.\r\n", ch);
			return (0);
		}
		if (IN_ROOM(vict) != NOWHERE)        /* Another Eric Green special. */
			char_from_room(vict);
		char_to_room(vict, rnum);
		break;
	case 39:
		SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
		break;
	case 40:
		SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
		break;
	case 41:
		SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
		break;
	case 42:
		if (!*val_arg) {
			culture_menu(ch, TRUE, FALSE);
			return (0);
		}
		i = parse_menu(*val_arg);
		if (i == NOTHING) {
			culture_menu(ch, TRUE, TRUE);
			return (0);
		}
		GET_CULTURE(vict) = i;
		break;
	case 43:
		SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
		break;
	case 44:
		SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
		break;
	case 45:
		if (!str_cmp(val_arg, "off")) {
			REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
		} else if (is_number(val_arg)) {
			rvnum = atoi(val_arg);
			if (real_room(rvnum) != NOWHERE) {
				SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
				GET_LOADROOM(vict) = rvnum;
				sprintf(output, "%s will enter at room #%d.", GET_NAME(vict),
								GET_LOADROOM(vict));
			} else {
				send_to_char("That room does not exist!\r\n", ch);
				return (0);
			}
		} else {
			send_to_char("Must be 'off' or a room's virtual number.\r\n", ch);
			return (0);
		}
		break;
	case 46:
		SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1 | PRF_COLOR_2));
		break;
	case 47:
		if (IS_OWNER(vict) || IS_NPC(vict))
			return (0);
		GET_IDNUM(vict) = value;
		break;
	case 48:
/*		if (GET_IDNUM(ch) > 1) {
			send_to_char("Please don't use this command, yet.\r\n", ch);
			return (0);
		}
*/		if (IS_GRGOD(vict)) {
			send_to_char("You cannot change that.\r\n", ch);
			return (0);
		}
		strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);
		*(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
		sprintf(output, "Password changed to '%s'.", val_arg);
		break;
	case 49:
		SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
		break;
	case 50:
		if ((i = search_block(val_arg, genders, FALSE)) < 0) {
			send_to_char("Must be 'male', 'female', or 'neutral'.\r\n", ch);
			return (0);
		}
		GET_SEX(vict) = i;
		break;
	case 51:        /* set age */
		if (value < 2 || value > 200) {        /* Arbitrary limits. */
			send_to_char("Ages 2 to 200 accepted.\r\n", ch);
			return (0);
		}
		/*
		 * NOTE: May not display the exact age specified due to the integer
		 * division used elsewhere in the code.  Seems to only happen for
		 * some values below the starting age (17) anyway. -gg 5/27/98
		 */
		vict->player.time.birth = time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
		break;

	case 52:        /* Blame/Thank Rick Glover. :) */
		GET_HEIGHT(vict) = value;
		affect_total(vict);
		break;

	case 53:
		GET_WEIGHT(vict) = value;
		affect_total(vict);
		break;

	case 54:
		GET_OLC_ZONE(vict) = value;
		break;

	case 55:
		if (!*val_arg) {
			race_menu(ch, TRUE, FALSE);
			return (0);
		}
		i = parse_menu(*val_arg);
		if (i == NOTHING) {
			race_menu(ch, TRUE, TRUE);
			return (0);
		} else if (i >= RACE_SERAPHI && !IS_IMPL(ch)) {
			race_menu(ch, TRUE, TRUE);
			return (0);
		}
		set_race(vict, i);
		break;

	case 56:
		/* Set email address */
		strncpy(GET_EMAIL(vict), val_arg, MAX_INPUT_LENGTH);
		*(GET_EMAIL(vict) + MAX_INPUT_LENGTH) = '\0';
		sprintf(output, "Email changed to '%s'.", val_arg);
		break;

	case 57:
		/* hometown */
		if (!(*val_arg)) {
			send_to_char("NUM ROOM    LOCATION\r\n", ch);
			for (i = 0; i < NUM_STARTROOMS; i++) {
				sprintf(buf, "%2d) [%5d] %s\r\n", i, mortal_start_room[i], hometowns[i]);
				send_to_char(buf, ch);
			}
			return (0);
		}
		if (!IS_NPC(vict)) {
			RANGE(0, NUM_STARTROOMS);
			GET_HOME(vict) = value;
			send_to_char("NUM ROOM    LOCATION\r\n", ch);
			for (i = 0; i < NUM_STARTROOMS; i++) {
				sprintf(buf, "%2d) [%5d] %s\r\n", i, mortal_start_room[i], hometowns[i]);
				send_to_char(buf, ch);
			}
			sprintf(buf2, "New hometown: %d out of %d choices.\n\r",
							GET_HOME(vict), NUM_STARTROOMS);
			send_to_char(buf2, ch);
		}
		break;

  case 58:
		if (IS_IMMORTAL(vict)) {
			send_to_char("Can't set that on Immortals!\r\n", ch);
			return (0);
		}
		if (set_contact(vict, val_arg))
			sprintf(output, "%s's contact is now: %s", GET_NAME(vict), ((GET_CONTACT(vict) != NULL) ? GET_CONTACT(vict) : "off"));
		else {
			send_to_char("Invalid contact name.\r\n", ch);
			return (0);
		}
		break;
  
	case 59:
		SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FORCE_RENAME);
		break;
		
	case 60:
		if (!*val_arg) {
			disp_attributes_menu(ch->desc, eye_color, NUM_EYECOLORS);
			return (0);
		}
		GET_EYECOLOR(vict) = RANGE(0, NUM_EYECOLORS);
		break;
		
	case 61:
		if (!*val_arg) {
			disp_attributes_menu(ch->desc, hair_color, NUM_HAIRCOLORS);
			return (0);
		}
		GET_HAIRCOLOR(vict) = RANGE(0, NUM_HAIRCOLORS);
		break;
		
	case 62:
		if (!*val_arg) {
			disp_attributes_menu(ch->desc, hair_style, NUM_HAIRSTYLES);
			return (0);
		}
		GET_HAIRSTYLE(vict) = RANGE(0, NUM_HAIRSTYLES);
		break;
		
	case 63:
		if (!*val_arg) {
			disp_attributes_menu(ch->desc, skin_tone, NUM_SKINTONES);
			return (0);
		}
		GET_SKINTONE(vict) = RANGE(0, NUM_SKINTONES);
		break;
		
	case 64:
		if (!*val_arg) {
			disp_bitvector_menu(ch->desc, user_rights, NUM_USER_RIGHTS);
			return (0);
		}
		USER_RIGHTS(vict) = asciiflag_conv(val_arg); /*Please work*/
		/* update the player table so that it matches */
		i = get_ptable_by_name(GET_NAME(vict));
		player_table[i].rights = USER_RIGHTS(vict);
		extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s set %s's rights to: %s", GET_NAME(ch), GET_NAME(vict), val_arg);
		snoop_check(vict);
		break;
		
	case 65:
		if (!*val_arg) {
			disp_bitvector_menu(ch->desc, extended_syslogs, NUM_SYSL_FLAGS);
			return (0);
		}
		EXTENDED_SYSLOG(vict) = asciiflag_conv(val_arg); /*Please work*/
		extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s set %s's xsyslog to: %s", GET_NAME(ch), GET_NAME(vict), val_arg);
		break;
		
	case 66:
		SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NEWBIE);
		break;
		
	case 67:
		GET_TRUENAME(vict) = str_dup(true_name(GET_ID(vict)));
		extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s set %s's True name to: %s", GET_NAME(ch), GET_NAME(vict), GET_TRUENAME(vict));
		break;
		
	case 68:
		GET_RPXP(vict) = value;
		break;

	case 69:
		if (!str_cmp(val_arg, "off")) {
			GET_FATIGUE(vict) = -1;
			sprintf(output, "%s's %s now off.", GET_NAME(vict), set_fields[mode].cmd);
		} else if (is_number(val_arg)) {
			value = atoi(val_arg);
			RANGE(0, NUM_FATIGUE_LEVELS);
			GET_FATIGUE(vict) = value;
			sprintf(output, "%s's %s set to %d.", GET_NAME(vict), set_fields[mode].cmd, value);
		} else {
			send_to_char("Must be 'off' or a value from 0 to 5.\r\n", ch);
			return (0);
		}
		break;

	case 70:
		GET_FLUX(vict) = value;
		break;

	case 71:
		GET_MAX_FLUX(vict) = value;
		break;

	case 72:
		GET_PIETY(vict) = RANGE(0, NUM_PIETY_RANKS - 1);
		break;

	case 73:
		GET_REPUTATION(vict) = RANGE(0 - NUM_REPUTATION_RANKS + 1, NUM_REPUTATION_RANKS - 1);
		break;

	case 74:
		GET_SOCIAL_RANK(vict) = RANGE(0, NUM_SOCIAL_RANKS - 1);
		break;

	case 75:
		GET_MILITARY_RANK(vict) = value;
		break;

	case 76:
		GET_SANITY(vict) = RANGE(0, NUM_SANITY_RANKS - 1);
		break;

	case 77:
		if (*val_arg) {
			if (GET_SDESC(vict))
				free(GET_SDESC(vict));
			GET_SDESC(vict) = str_dup(LOWERALL(val_arg));
			sprintf(output, "%s's S-Desc is now: &y%s&n", GET_NAME(vict), ((GET_SDESC(vict) != NULL) ? GET_SDESC(vict) : "<None>"));
		} else {
			send_to_char("Invalid S-Desc.\r\n", ch);
			return (0);
		}
		break;

	case 78:
		if (*val_arg && val_arg[strlen(val_arg) - 1] == '.' && isalpha(val_arg[strlen(val_arg) - 2])) {
			if (GET_LDESC(vict))
				free(GET_LDESC(vict));
			if (GET_KEYWORDS(vict))
				free (GET_KEYWORDS(vict));
			GET_KEYWORDS(vict) = str_dup(create_keywords(val_arg));
			/* have to concatenate linefeed to ldesc */
			char *dbuf = get_buffer(MAX_INPUT_LENGTH);
			sprintf(dbuf, "%s\r\n", val_arg);
			GET_LDESC(vict) = str_dup(CAP(dbuf));
			release_buffer(dbuf);
			sprintf(output, "%s's L-Desc is now :-\r\n&y%s&n", GET_NAME(vict), ((GET_LDESC(vict) != NULL) ? GET_LDESC(vict) : "<None>"));
		} else {
			send_to_char("Invalid L-Desc.\r\n", ch);
			return (0);
		}
		break;
	
	case 79:
		GET_SKILLCAP(vict) = value;
		break;

  default:
		send_to_char("Can't set that!\r\n", ch);
		return (0);
	}

	strcat(output, "\r\n");
	send_to_char(CAP(output), ch);
	return (1);
}


ACMD(do_set)
{
	struct char_data *vict = NULL, *cbuf = NULL;
	char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH],
				val_arg[MAX_INPUT_LENGTH];
	int mode, len, player_i = 0, retval;
	char is_file = 0, is_player = 0;
	int i, row = 0;

	half_chop(argument, name, buf);

	if (!strcmp(name, "file")) {
		is_file = 1;
		half_chop(buf, name, buf);
	} else if (!str_cmp(name, "player")) {
		is_player = 1;
		half_chop(buf, name, buf);
	} else if (!str_cmp(name, "mob"))
		half_chop(buf, name, buf);

	half_chop(buf, field, buf);
	strcpy(val_arg, buf);

	if (!*name || !*field) {
		char *list = get_buffer(MAX_STRING_LENGTH);
		sprintf(list, "\r\nSet fields available to &W%s&n:\r\n\r\n", GET_NAME(ch));
		for (i = 0; *set_fields[i].cmd != '\n'; i++) {
			if (!GOT_RIGHTS(ch, set_fields[i].rights) || set_fields[i].rights == RIGHTS_NONE)
				continue;
			sprintf(list, "%s%-13.13s ", list, set_fields[i].cmd);
			if (row++ % 5 == 4)
				strcat(list, "\r\n");
		}
		if (row % 5)
			strcat(list, "\r\n");
		sprintf(list, "%s\r\nUsage: set <victim> <field> <value>\r\n", list);
		page_string(ch->desc, list, TRUE);
		release_buffer(list);
		return;
	}

	/* find the target */
	if (!is_file) {
		if (is_player) {
			if (!(vict = get_player_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
				send_to_char("There is no such player.\r\n", ch);
				return;
			}
		} else { /* is_mob */
			if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD, 1))) {
				send_to_char("There is no such creature.\r\n", ch);
				return;
			}
		}
	} else if (is_file) {
		/* try to load the player off disk */
		CREATE(cbuf, struct char_data, 1);
		clear_char(cbuf);
		CREATE(cbuf->player_specials, struct player_special_data, 1);
		if ((player_i = load_char(name, cbuf)) > -1) {
			if (compare_rights(ch, cbuf) <= 0 && !IS_IMPL(ch)) {
				free_char(cbuf);
				send_to_char("Sorry, you can't do that.\r\n", ch);
				return;
			}
			vict = cbuf;
		} else {
			free_char(cbuf);
			send_to_char("There is no such player.\r\n", ch);
			return;
		}
	}

	/* find the command in the list */
	len = strlen(field);
	for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
		if (!strncmp(field, set_fields[mode].cmd, len))
			break;

	/* perform the set */
	retval = perform_set(ch, vict, mode, val_arg);

	/* save the character if a change was made */
	if (retval) {
		if (!is_file && !IS_NPC(vict))
			save_char(vict, NOWHERE, FALSE);
		if (is_file) {
			GET_PFILEPOS(cbuf) = player_i;
			save_char(cbuf, GET_LOADROOM(cbuf), TRUE);
			send_to_char("Saved in file.\r\n", ch);
		}
	}

	/* free the memory if we allocated it earlier */
	if (is_file)
		free_char(cbuf);
}


void disp_bitvector_menu(struct descriptor_data *d, const char *names[], int length)
{
	char *menu = get_buffer(MAX_STRING_LENGTH);
	int i, columns = 0;
  char *bit="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ:;<=>?[\\]^_`";

	get_char_colors(d->character);

	for (i = 0; i < length; i++) {
		sprintf(menu + strlen(menu), "%s%-1.1s%s) %-20.20s  %s", grn, &bit[i], nrm, names[i],
			!(++columns % 3) ? "\r\n" : "");
	}
	sprintf(menu, "%s%s", menu, (columns % 3) ? "\r\n" : "");
	write_to_output(d, TRUE, "%s", menu);
	release_buffer(menu);
}


void disp_attributes_menu(struct descriptor_data *d, const char *attribute[], int length)
{
	char *menu = get_buffer(MAX_STRING_LENGTH);
	int i, columns = 0;

	get_char_colors(d->character);

	for (i = 0; i < length; i++) {
		sprintf(menu + strlen(menu), "%s%2d%s) %-20.20s  %s", grn, i, nrm, attribute[i],
			!(++columns % 2) ? "\r\n" : "");
	}
	sprintf(menu, "%s%s", menu, (columns % 3) ? "\r\n" : "");
	write_to_output(d, TRUE, "%s", menu);
	release_buffer(menu);
}


ACMD(do_world)
{
	int allows = 0;

	const char *allow[] = {
		"disallowed",
		"allowed",
		"\n"
	};

	const char *allowtype[] = {
		"Player Killing",
		"PK Sleep",
		"PK Summon",
		"PK Charm",
		"PK Room Affects",
		"\n"
	};

	switch (subcmd) {
	case SCMD_PK:
		if (pk_allowed == 1) {
			allows = 0;
			pk_allowed = 0;
		} else {
			allows = 1;
			pk_allowed = 1;
		}
		break;
	case SCMD_PKSLEEP:
		if (sleep_allowed == 1) {
			allows = 0;
			sleep_allowed = 0;
		} else {
			allows = 1;
			sleep_allowed = 1;
		}
		break;
	case SCMD_PKSUMMON:
		if (summon_allowed == 1) {
			allows = 0;
			summon_allowed = 0;
		} else {
			allows = 1;
			summon_allowed = 1;
		}
		break;
	case SCMD_PKCHARM:
		if (charm_allowed == 1) {
			allows = 0;
			charm_allowed = 0;
		} else {
			allows = 1;
			charm_allowed = 1;
		}
		break;
	case SCMD_PKROOMAFFECTS:
		if (roomaffect_allowed == 1) {
			allows = 0;
			roomaffect_allowed = 0;
		} else {
			allows = 1;
			roomaffect_allowed = 1;
		}
		break;
	case SCMD_PKMAIN:
	default:
		send_to_charf(ch, "%s", display_config_vars());
		return;
	}

	if (subcmd >= 0 && subcmd < SCMD_PKMAIN) {
		sprintf(buf, "You have %s %s.\r\n", allow[allows], allowtype[subcmd]);
		send_to_char(buf, ch);

		extended_mudlog(NRM, SYSL_GENERAL, TRUE, "%s %s %s.", CAP(GET_NAME(ch)), allow[allows], allowtype[subcmd]);
	}

}


int newbie_equip(struct char_data *ch)
{
	struct obj_data *obj;
	int item, where;

	return(1);

	int give_obj[]    = { 103, 104, 120, -1 }; /* Global objects */
	int magi_obj[]    = { 110, -1 };
	int thief_obj[]   = { 110, 113, -1 };
	int cleric_obj[]  = { 114, -1 };
	int warrior_obj[] = { 100, 101, 102, 105, 106, 107, 108, 109, 110, 111, -1 };

	if (IN_ROOM(ch) == NOWHERE)
		IN_ROOM(ch) = 0;

	for(item = 0; give_obj[item] != -1; item++) {
		obj = read_object(give_obj[item], VIRTUAL);
		if (obj == NULL)
			continue;
		obj_to_char(obj, ch);
		if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
			perform_wear(ch, obj, where, 0);
		}
	}

	if(IS_MAGI_TYPE(ch)) {
		for(item = 0; magi_obj[item] != -1; item++) {
			obj = read_object(magi_obj[item], VIRTUAL);
			if (obj == NULL)
				continue;
			obj_to_char(obj, ch);
			if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
				perform_wear(ch, obj, where, 0);
			}
		}
	} else if(IS_THIEF_TYPE(ch)) {
		for(item = 0; thief_obj[item] != -1; item++) {
			obj = read_object(thief_obj[item], VIRTUAL);
			if (obj == NULL)
				continue;
			obj_to_char(obj, ch);
			if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
				perform_wear(ch, obj, where, 0);
			}
		}
	} else if(IS_CLERIC_TYPE(ch)) {
		for(item = 0; cleric_obj[item] != -1; item++) {
			obj = read_object(cleric_obj[item], VIRTUAL);
			if (obj == NULL)
				continue;
			obj_to_char(obj, ch);
			if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
				perform_wear(ch, obj, where, 0);
			}
		}
	} else if(IS_WARRIOR_TYPE(ch)) {
		for(item = 0; warrior_obj[item] != -1; item++) {
			obj = read_object(warrior_obj[item], VIRTUAL);
			if (obj == NULL)
				continue;
			obj_to_char(obj, ch);
			if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
				perform_wear(ch, obj, where, 0);
			}
		}
	}

	if (IN_ROOM(ch) == 0)
		IN_ROOM(ch) = NOWHERE;

	return(1);

}


ACMD(do_newbie)
{
	struct char_data *victim;

	one_argument(argument, buf);
	
	if(IS_NPC(ch)) {
		send_to_char("Monsters can't help newbies, go away!\r\n", ch);
		return;
	}
	if (!*buf)
		send_to_char("Who would you like to newbie?\r\n", ch);
	else if (str_cmp("all", buf)) {
		if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD, 1)))
			send_to_char("That person is not here.\r\n", ch);
		else if (victim == ch && !IS_IMPL(ch))
			send_to_char("You're just a LITTLE to high to be newbied!\r\n", ch);
		else {
			if (IS_IMMORTAL(victim) && !IS_IMPL(victim)) {
				send_to_char("That victim is too experienced to newbie!\r\n", ch);
				return;
			}
			if(IS_NPC(victim)) {
				send_to_char("Monsters don't need no newbie equipment!\r\n", ch);
				return;
			}

			if(newbie_equip(victim)) {
				extended_mudlog(NRM, SYSL_NEWBIES, TRUE, "%s newbie-equipped %s.", GET_NAME(ch),
								GET_NAME(victim));
				act("You newbie-equipped $N.", FALSE, ch, NULL, victim, TO_CHAR);
				act("$n newbie-equipped you.", FALSE, ch, NULL, victim, TO_VICT);
				act("$n newbie-equipped $N.", TRUE, ch, NULL, victim, TO_NOTVICT);
			} else {
				sprintf(buf, "Something went wrong when you tried to newbie equip %s.\r\n", GET_NAME(victim));
				send_to_char(buf, ch);
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Newbie-equip of %s failed.", GET_NAME(victim));
			}
		}
	}
}


ACMD(do_xname)
{
	char tempname[MAX_INPUT_LENGTH];
	int i = 0;
	FILE *fp;
	*buf = '\0';

	one_argument(argument, buf);

	if(!*buf) {
		send_to_char("Xname which name?\r\n", ch);
		return;
	}
	if(!(fp = fopen(XNAME_FILE, "a"))) {
		perror("Problems opening xname file for do_xname");
		return;
	}
	strcpy(tempname, buf);
	for (i = 0; tempname[i]; i++)
		tempname[i] = LOWER(tempname[i]);
	fprintf(fp, "%s\n", tempname);
	fclose(fp);
	sprintf(buf1, "%s has been xnamed!\r\n", tempname);
	send_to_char(buf1, ch);
	Read_Invalid_List();
}


ACMD(do_file)
{
	FILE *req_file;
	int cur_line = 0,
			num_lines = 0,
			req_lines = 0,
			i,
			j;
	int l;
	char field[MAX_INPUT_LENGTH],
			 value[MAX_INPUT_LENGTH];
	struct file_struct {
		char *cmd;
		bitvector_t rights;
		char *file;
	} fields[] = {
		{	"none",			RIGHTS_IMPLEMENTOR,	"Does Nothing"	},
		{	"xnames",		RIGHTS_ADMIN,				"../lib/misc/xnames"},
		{	"help",			RIGHTS_HELPFILES,		"../log/help"},
		{	"olc",			RIGHTS_DEVELOPER,		"../log/olc"	},
		{	"sql",			RIGHTS_DEVELOPER,		"../log/sql"	},
		{	"errors",		RIGHTS_DEVELOPER,		"../log/errors" },
		{	"restarts",	RIGHTS_ADMIN,				"../log/restarts" },
		{	"pclean",		RIGHTS_IMMORTAL,		"../log/pclean" },
		{	"players",	RIGHTS_IMMORTAL,		"../log/newplayers" },
		{	"rentgone",	RIGHTS_IMMORTAL,		"../log/rentgone" },
		{	"godcmds",	RIGHTS_ADMIN,				"../log/godcmds"	},
		{	"syslog",		RIGHTS_ADMIN,				"../syslog" },
		{	"crash",		RIGHTS_ADMIN,				"../syslog.CRASH" },
		{	"quest",		RIGHTS_QUESTOR,			"../log/quest" },
		{	"\n",				0,									"\n"	}
	};

	skip_spaces(&argument);

	if (!*argument)
	{
		strcpy(buf, "USAGE: view <option> <num lines>\r\n\r\nFile options:\r\n");
		for (j = 0, i = 1; fields[i].rights; i++)
			if (GOT_RIGHTS(ch, fields[i].rights))
				sprintf(buf, "%s&W%-15s&n%s\r\n", buf, fields[i].cmd, fields[i].file);
			send_to_char(buf, ch);
			return;
	}

	strcpy(arg, two_arguments(argument, field, value));

	for (l = 0; *(fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, fields[l].cmd, strlen(field)))
			break;

	if(*(fields[l].cmd) == '\n')
	{
		send_to_char("That is not a valid option!\r\n", ch);
		return;
	}

	if (!GOT_RIGHTS(ch, fields[l].rights))
	{
		send_to_char("You are not godly enough to view that file!\r\n", ch);
		return;
	}

	if(!*value)
		 req_lines = GET_PAGE_LENGTH(ch); /* default is the last 22 lines */
	else
		 req_lines = atoi(value);

	/* open the requested file */
	if (!(req_file=fopen(fields[l].file,"r")))
	{
		 extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error opening file %s using 'view' command.",
						 fields[l].file);
		 return;
	}

	/* count lines in requested file */
	get_line(req_file,buf);
	while (!feof(req_file))
	{
		 num_lines++;
		 get_line(req_file,buf);
	}
	fclose(req_file);


	/* Limit # of lines printed to # requested or # of lines in file or
		 150 lines */
	if(req_lines > num_lines) req_lines = num_lines;
	if(req_lines > 500) req_lines = 500;


	/* close and re-open */
	if (!(req_file=fopen(fields[l].file,"r")))
	{
		 extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error opening file %s using 'file' command.",
						 fields[l].file);
		 return;
	}

	buf2[0] = '\0';

	/* and print the requested lines */
	get_line(req_file,buf);
	while (!feof(req_file))
	{
		 cur_line++;
		 if(cur_line > (num_lines - req_lines))
		 {
				sprintf(buf2,"%s%s\r\n",buf2, buf);
		 }
		 get_line(req_file,buf);
	 }
	 
	 
	 sprintf(buf, "%s",buf2);
	 page_string(ch->desc, buf, 1);

	 fclose(req_file);
}

ACMD(do_approve)
{
	MYSQL_RES *playerindex;
	MYSQL_ROW row;
	int rec_count = 0;
	char player[MAX_STRING_LENGTH];
	char *name=NULL, *approvedby=NULL;
  struct char_data *vict = '\0';
	const char *approve_type[] = {
		"unapprove",
		"approve"
	};

	one_argument(argument, player);

	if (subcmd != SCMD_APPROVE && subcmd != SCMD_UNAPPROVE) {
		send_to_char("An error has occured.\r\n", ch);
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unknown subcmd (%d) passed to do_approve.", subcmd);
		return;
	}

	if (!*player) {
		sprintf(buf, "You want to %s who?\r\n", approve_type[subcmd]);
    send_to_char(buf, ch);
    return;
  }

	SQL_MALLOC(LOWERALL(player), name);
	SQL_ESC(LOWERALL(player), name);

	if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT ID, Name, Perms, Preferences, Last, Active FROM %s WHERE name = '%s';", TABLE_PLAYER_INDEX, name))) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading player record of [%s] from [%s].", TABLE_PLAYER_INDEX, name);
		return;
	}

	SQL_FREE(name);

	rec_count = mysql_num_rows(playerindex);

	if (rec_count == 1) {
		row = mysql_fetch_row(playerindex);
		if (!is_name(row[1], GET_NAME(ch)) && atoi(row[5]) != subcmd) {
			SQL_MALLOC(GET_NAME(ch), approvedby);
			SQL_ESC(GET_NAME(ch), approvedby);
			mysqlWrite("UPDATE %s SET Active = %d, ApprovedBy = '%s' WHERE ID = %d;", TABLE_PLAYER_INDEX, subcmd, ((subcmd == SCMD_APPROVE) ? approvedby : ""), atoi(row[0]));
			extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s %sd %s for roleplay.", GET_NAME(ch), approve_type[subcmd], CAP(player));
			SQL_FREE(approvedby);
		}
	} else {
		/* If we didn't find exactly one char, then we abort. */
		mysql_free_result(playerindex);
		send_to_char("No such player in the database.\r\n", ch);
		return;
	}

	mysql_free_result(playerindex);	

	if (!(vict = get_player_vis(ch, player, NULL, FIND_CHAR_WORLD))) {
		sprintf(buf, "%s not online, %sd directly in database.\r\n", CAP(player), approve_type[subcmd]);
		send_to_char(buf, ch);
		return;
	} else {
		if ((IS_ACTIVE(vict) && subcmd == SCMD_APPROVE) || (!IS_ACTIVE(vict) && subcmd == SCMD_UNAPPROVE)) {
			sprintf(buf, "%s has already been %sd.\r\n", GET_NAME(vict), approve_type[subcmd]);
			send_to_char(buf, ch);
			return;
		}	
		IS_ACTIVE(vict) = subcmd;
		if (GET_APPROVEDBY(vict)) {
			free(GET_APPROVEDBY(vict));
			GET_APPROVEDBY(vict) = NULL;
		}
		switch (subcmd) {
			case SCMD_APPROVE:
				GET_APPROVEDBY(vict) = str_dup(GET_NAME(ch));
				sprintf(buf, "You have been approved for roleplay by %s.\r\nWelcome to the Arcane Realms, we hope you will enjoy your stay!\r\nType HELP NEWBIE to get a quick tour of the system.\r\n", GET_NAME(ch));
				send_to_char(buf, vict);	
				break;
			case SCMD_UNAPPROVE:
				GET_APPROVEDBY(vict) = NULL;
				sprintf(buf, "Your roleplaying rights have been revoked by %s.\r\n", GET_NAME(ch));
				send_to_char(buf, vict);	
				break;
			default:
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached default case in do_approve.");
				return;
				break;
		}
		save_char(vict, NOWHERE, FALSE);
	}

	sprintf(buf, "%s has been %sd for roleplay.\r\n", CAP(player), approve_type[subcmd]);
	send_to_char(buf, ch);
}


ACMD(do_rename)
{
	int i;
  struct char_data *vict;
  char old_name[MAX_NAME_LENGTH], new_name[MAX_NAME_LENGTH];
	char *tmp_name;

  half_chop(argument, old_name, new_name);

  if (!*old_name || !*new_name) {
    send_to_char("Rename who to what?\r\n", ch);
    return;
  }

	if (!(vict = get_player_vis(ch, old_name, NULL, FIND_CHAR_WORLD))) {
		send_to_char(NOPERSON, ch);
		return;
	}
	
	if (vict == ch)	{
		send_to_char("You cannot rename yourself, sorry.\r\n", ch);
		return;
	}

	if (get_ptable_by_name(new_name) != -1) {
		send_to_char("A player already exists with that name.\r\n", ch);
		return;
	}

	tmp_name = get_buffer(MAX_INPUT_LENGTH);
	if (!Valid_Name(new_name) || _parse_name(new_name, tmp_name)) {
		sprintf(buf, "%s cannot be allowed as a name, please try another.\r\n", CAP(new_name));
		send_to_char(buf, ch);
		release_buffer(tmp_name);
		return;
	}

	if ((i = get_ptable_by_name(old_name)) != -1) {
		remove_player(i);
		player_table[i].name = str_dup(tmp_name);
	}

	free(GET_NAME(vict));
	GET_NAME(vict) = str_dup(tmp_name);
	if (GET_APPROVEDBY(vict))
		free(GET_APPROVEDBY(vict));
	GET_APPROVEDBY(vict) = str_dup(GET_NAME(ch));

	sprintf(buf, "%s renamed to \"%s\" and saved.\r\n", CAP(old_name), GET_NAME(vict));
	send_to_char(buf, ch);
	sprintf(buf, "You have been renamed to \"%s\" by %s.\r\n", GET_NAME(vict), GET_NAME(ch));
	send_to_char(buf, vict);

	save_char(vict, NOWHERE, FALSE);
  write_aliases(vict);
  crash_datasave(vict, 0, RENT_CRASH);
	
	extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s renamed %s to \"%s\".", GET_NAME(ch), CAP(old_name), tmp_name);

	release_buffer(tmp_name);

}


ACMD(do_pending)
{
	MYSQL_RES *playerindex;
	MYSQL_ROW row;
	int rec_count = 0, i;
	time_t last;
	bitvector_t flags;
	unsigned long *fieldlength;

	if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT ID, LCASE(Name), Perms, Preferences, Last, Active, Host, Flags FROM %s WHERE Active = 0 AND Verified = 1 ORDER BY ID ASC;", TABLE_PLAYER_INDEX))) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading player records from [%s].", TABLE_PLAYER_INDEX);
		return;
	}

	rec_count = mysql_num_rows(playerindex);

	if(rec_count == 0) {
		send_to_char("All players in database have been approved.\r\n", ch);
		return;
	}

	send_to_char("THE FOLLOWING PLAYERS ARE STILL NOT APPROVED:\r\n", ch);
	send_to_char("Players marked with &Rred&n have been set to force rename.\r\n", ch);

	for(i = 0; i < rec_count; i++)
	{
		row = mysql_fetch_row(playerindex);
		fieldlength = mysql_fetch_lengths(playerindex);
		last = atoi(row[4]);
		flags = atoi(row[7]);
		if (!(atoi(row[5]))) {
			sprintf(buf, "[%5d] [%s%-20s&n] [%-15s] : %-20s\r", 
				atoi(row[0]), IS_SET(flags, PLR_FORCE_RENAME) ? "&R" : "", CAP(row[1]), row[6], ctime(&last));
			send_to_char(buf, ch);
		}
	}

	/* Free up result sets to conserve memory. */
	mysql_free_result(playerindex);

}


ACMD(do_xsyslog)
{
  bitvector_t result;
	int	i = 0, j, l, columns = 0;
	char field[MAX_INPUT_LENGTH];
	char *menu = get_buffer(MAX_STRING_LENGTH);
	char *name = get_buffer(64);

	skip_spaces(&argument);

	if (!*argument)
	{
		send_to_char("\r\nEXTENDED SYSLOGS:\r\n\r\n", ch);
		for (j = 0, i = 0; log_fields[i].rights; i++) {
			if (GOT_RIGHTS(ch, log_fields[i].rights)) {
				strcpy(name, log_fields[i].cmd);
				strcpy(name, LOWERALL(name));
				sprintf(menu + strlen(menu), "&W%-15.15s&n %-3s      %s", CAP(name), ONOFF(GOT_SYSLOG(ch, 1ULL << i)),
					!(++columns % 3) ? "\r\n" : "");
			}
		}
		sprintf(menu, "%s%s\r\nUSAGE: xsyslog { <option> | All | Off }\r\n", menu, (columns % 3) ? "\r\n" : "");
		send_to_char(menu, ch);
		release_buffer(menu);
		release_buffer(name);
		return;
	}

	strcpy(arg, one_argument(argument, field));

	if (!strncmp(field, "all", 3)) {
		for (l = 0; *(log_fields[l].cmd) != '\n'; l++)
			if (GOT_RIGHTS(ch, log_fields[l].rights))
				SET_BIT(EXTENDED_SYSLOG(ch), 1ULL << l);
		release_buffer(menu);
		release_buffer(name);
		send_to_char("All extended syslogs activated.\r\n", ch);
		return;
	} else if (!strncmp(field, "off", 3)) {
		for (l = 0; *(log_fields[l].cmd) != '\n'; l++)
			if (GOT_RIGHTS(ch, log_fields[l].rights))
				REMOVE_BIT(EXTENDED_SYSLOG(ch), 1ULL << l);
		release_buffer(menu);
		release_buffer(name);
		send_to_char("All extended syslogs have been turned off.\r\n", ch);
		return;
	}

	for (l = 0; *(log_fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, log_fields[l].cmd, strlen(field)))
			break;

	if(*(log_fields[l].cmd) == '\n')
	{
		send_to_char("That is not a valid syslog option!\r\n", ch);
		release_buffer(menu);
		release_buffer(name);
		return;
	}

	if (!GOT_RIGHTS(ch, log_fields[l].rights))
	{
		send_to_char("You do not have the rights to see that syslog!\r\n", ch);
		if (GOT_SYSLOG(ch, 1ULL << l))
			result = REMOVE_BIT(EXTENDED_SYSLOG(ch), 1ULL << l);
		release_buffer(menu);
		release_buffer(name);
		return;
	}

	result = SYSL_TOG_CHK(ch, 1ULL << l);

	strcpy(name, log_fields[l].cmd);

  if (result)
		sprintf(menu, "%s syslog turned on.\r\n", CAP(name));
	else
		sprintf(menu, "%s syslog turned off.\r\n", CAP(name));
	send_to_char(menu, ch);
	release_buffer(menu);
	release_buffer(name);
}


ACMD(do_wipeplayer)
{
	int i;
	char name[MAX_NAME_LENGTH];

	if (!*argument) {
		send_to_char("Usage: wipeplayer { preview | old | <name> }\r\n", ch);
		return;
	}

	one_argument(argument, name);

	if (!strncmp(name, "preview", 7)) {
		send_to_char("Previewing old players in the database.\r\n", ch);
		clean_players(1);
		return;
	}

	if (!strncmp(name, "old", 3)) {
		send_to_char("Cleaning old players from the database.\r\n", ch);
		clean_players(0);
		return;
	}

	if ((i = get_ptable_by_name(name)) != -1) {
		if (player_table[i].rights < USER_RIGHTS(ch) || IS_OWNER(ch)) {
			if (player_table[i].id == GET_ID(ch)) {
				send_to_char("Not a smart way to commit suicide...\r\n", ch);
				return;
			} else if (IS_SET(player_table[i].rights, RIGHTS_OWNER)) {
				extended_mudlog(NRM, SYSL_SECURE, TRUE, "%s tried to delete %s from the player index.", GET_NAME(ch), player_table[i].name);
				send_to_char("You may not delete the owner.\r\n", ch);
				return;
			}
			sprintf(buf, "You have manually wiped player %s.\r\n", player_table[i].name);
			send_to_char(buf, ch);
			extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s has manually deleted %s from the player index.", GET_NAME(ch), player_table[i].name);
			remove_player(i);
		} else {
			send_to_charf(ch, "Your rights: %lld, %s's rights: %lld.\r\n", USER_RIGHTS(ch), player_table[i].name, player_table[i].rights);
			send_to_char("You cannot delete a player that has more rights than you.\r\n", ch);
			return;
		}
	} else {
		send_to_char("No such player in the player table.\r\n", ch);
		return;
	}

}


ACMD(do_award)
{
  char *player = get_buffer(MAX_INPUT_LENGTH);
  char *amount = get_buffer(MAX_INPUT_LENGTH);
  struct char_data *vict;
	int questpoints;

  half_chop(argument, player, amount);

  if (!*player || !*amount) {
    send_to_charf(ch, "Usage: %s { <player> | all | quest | ic } <points>\r\n", CMD_NAME);
    release_buffer(player);
    release_buffer(amount);
    return;
  }

	questpoints = atoi(amount);

	if (!str_cmp("all", player)) { // Award all online players
		struct descriptor_data *d;
    for (d=descriptor_list; d; d = d->next) {
			if (STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) {
				GET_QP(d->character) = LIMIT(GET_QP(d->character) + questpoints, 0, 1000000);
				send_to_charf(d->character, "You have been awarded %d quest point%s by %s.\r\n", questpoints, (questpoints != 1 ? "s" : ""), GET_NAME(ch));
			}
		}
		extended_mudlog(NRM, SYSL_QUESTING, TRUE, "Everybody were awarded %d quest point%s by %s.", questpoints, (questpoints != 1 ? "s" : ""), GET_NAME(ch));
		send_to_char(OK, ch);
	} else if (!str_cmp("ic", player)) { // Award all IC players
		struct descriptor_data *d;
    for (d=descriptor_list; d; d = d->next) {
			if ((STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) && IS_IC(d->character)) {
				GET_QP(d->character) = LIMIT(GET_QP(d->character) + questpoints, 0, 1000000);
				send_to_charf(d->character, "You have been awarded %d quest point%s by %s.\r\n", questpoints, (questpoints != 1 ? "s" : ""), GET_NAME(ch));
			}
		}
		extended_mudlog(NRM, SYSL_QUESTING, TRUE, "All IC players were awarded %d quest point%s by %s.", questpoints, (questpoints != 1 ? "s" : ""), GET_NAME(ch));
		send_to_char(OK, ch);
	} else if (!str_cmp("quest", player)) { // Award all players in quest
		struct descriptor_data *d;
    for (d=descriptor_list; d; d = d->next) {
			if ((STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) && PRF_FLAGGED(d->character, PRF_QUEST)) {
				GET_QP(d->character) = LIMIT(GET_QP(d->character) + questpoints, 0, 1000000);
				send_to_charf(d->character, "You have been awarded %d quest point%s by %s.\r\n", questpoints, (questpoints != 1 ? "s" : ""), GET_NAME(ch));
			}
		}
		extended_mudlog(NRM, SYSL_QUESTING, TRUE, "Quest-players were awarded %d quest point%s by %s.", questpoints, (questpoints != 1 ? "s" : ""), GET_NAME(ch));
		send_to_char(OK, ch);
	} else { // We are looking for a player...
		if (!(vict = get_player_vis(ch, player, NULL, FIND_CHAR_WORLD))) {
			send_to_char(NOPERSON, ch);
			release_buffer(player);
			release_buffer(amount);
			return;
		} else {
			GET_QP(vict) = LIMIT(GET_QP(vict) + questpoints, 0, 1000000);
			send_to_charf(vict, "You have been awarded %d quest point%s by %s.\r\n", questpoints, (questpoints != 1 ? "s" : ""), GET_NAME(ch));
			save_char(vict, NOWHERE, FALSE);
			extended_mudlog(NRM, SYSL_QUESTING, TRUE, "%s was awarded %d quest point%s by %s.", GET_NAME(vict), questpoints, (questpoints != 1 ? "s" : ""), GET_NAME(ch));
			send_to_char(OK, ch);
		}
	}

	release_buffer(player);
	release_buffer(amount);
}


ACMD(do_chown)
{
  struct char_data *victim;
  struct obj_data *obj;
	char *sbuf = get_buffer(1024);
  char *obuf = get_buffer(256);
  char *vbuf = get_buffer(256);
  int i, k = 0;

	skip_spaces(&argument);
  two_arguments(argument, obuf, vbuf);

  if (!*obuf)
    send_to_char("Syntax: chown <object> <character>.\r\n", ch);
  else if (!*vbuf)
    send_to_char("Syntax: chown <object> <character>.\r\n", ch);
  else if (!(victim = get_char_vis(ch, vbuf, NULL, FIND_CHAR_WORLD, 1)))
    send_to_char("No one by that name here.\r\n", ch);
  else if (victim == ch)
    send_to_char("Are you sure you're feeling ok?\r\n", ch);
  else if (get_max_rights(victim) >= get_max_rights(ch))
    send_to_char("That's really not such a good idea.\r\n", ch);
  else {
    for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(victim, i) && CAN_SEE_OBJ(ch, GET_EQ(victim, i)) && isname(obuf, GET_EQ(victim, i)->name)) {
        obj_to_char(unequip_char(victim, i), victim);
        k = 1;
      }
    }

		if (!(obj = get_obj_in_list_vis(victim, obuf, NULL, victim->carrying))) {
			if (!k && !(obj = get_obj_in_list_vis(victim, obuf, NULL, victim->carrying))) {
				sprintf(sbuf, "%s does not appear to have the %s.\r\n", GET_NAME(victim), obuf);
				send_to_char(sbuf, ch);
				release_buffer(sbuf);
      	release_buffer(obuf);
				release_buffer(vbuf);
				return;
			}
		}

		act("&n$n makes a magical gesture and $p&n flies from $N to $m.",FALSE,ch,obj,victim,TO_NOTVICT);
		act("&n$n makes a magical gesture and $p&n flies away from you to $m.",FALSE,ch,obj,victim,TO_VICT);
		act("&nYou make a magical gesture and $p&n flies away from $N to you.",FALSE,ch,obj, victim,TO_CHAR);

		obj_from_char(obj);
		obj_to_char(obj, ch);
		save_char(ch, NOWHERE, FALSE);
		save_char(victim, NOWHERE, FALSE);
	}

	release_buffer(sbuf);
	release_buffer(obuf);
	release_buffer(vbuf);

}
