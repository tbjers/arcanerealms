/* ************************************************************************
*  File: act.build.c                                    Part of CircleMUD *
*	 Usage: Builder-rights commands                                         *
*																																					*
*	 Various builder/OLC related functions have been moved from             *
*	 act.wizard.c into this new file in order to clean up and simplify for  *
*	 coders that only wish to change something in their OLC commands and    *
*	 not wish to recompile all 3000-4000 lines of act.wizard.c.             *
*	 Done by: Torgny Bjers <artovil@arcanerealms.org> for Arcane Realms MUD *
*	 Many of the functions are by L. Raymond: <lraymond@brokersys.com>      *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: act.build.c,v 1.53 2003/06/11 02:42:36 cheron Exp $ */

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
#include "oasis.h"
#include "genolc.h"
#include "genwld.h"
#include "genzon.h"
#include "dg_scripts.h"
#include "tutor.h"

/*	 external vars  */
extern struct room_data *world;
extern struct zone_data *zone_table;
extern struct char_data *character_list;
extern struct obj_data *obj_proto;
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct index_data *obj_index;
extern struct social_messg *soc_mess_list;
extern int load_into_inventory;
extern int buf_switches, buf_largecount, buf_overflows;
extern int top_of_p_table;
extern int top_of_socialt;
extern int is_name(const char *str, const char *namelist);
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern zone_rnum top_of_zone_table;
extern obj_vnum object_name_to_num(char *namestr);
extern mob_vnum mobile_name_to_num(char *namestr);
struct char_data *find_char(int n);
extern struct spell_info_type *spell_info;
extern struct tutor_info_type *tutor_info;
extern int make_player_corpse;
extern int soft_player_death;

/* (FIDO) Added allowed extern variables here for ACMD(do_world) */
extern int pk_allowed;
extern int sleep_allowed;
extern int charm_allowed;
extern int summon_allowed;
extern int roomaffect_allowed;

/* for chars */
extern struct class_data classes[NUM_CLASSES];
extern struct race_list_element *race_list;
extern const char *npc_class_types[];
extern const char *npc_race_types[];

/* extern functions */
void show_shops(struct char_data *ch, char *value);
void reset_zone(zone_rnum zone);
void roll_real_abils(struct char_data *ch);
int	parse_class(struct char_data *ch, char arg);
int	parse_race(char arg);
int	save_all(void);
void Crash_rentsave(struct char_data *ch, int cost);
int	zdelete_check(int zone);
void make_corpse(struct char_data *ch);

/* local functions */
room_rnum	find_target_room(struct char_data *ch, char *rawroomstr);
room_vnum	redit_find_new_vnum(zone_rnum zone);
void load_to_recipient(struct char_data *ch, struct char_data *recipient, struct obj_data *obj, int dolog);
void print_zone_to_buf(char *bufptr, zone_rnum zone);
ACMD(do_astat);
ACMD(do_cleanse);
ACMD(do_copyto);
ACMD(do_dig);
ACMD(do_liblist);
ACMD(do_oload);
ACMD(do_purgemob);
ACMD(do_purgeobj);
ACMD(do_saveall);
ACMD(do_slay);
ACMD(do_undig);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_zdelete);
ACMD(do_zreset);
ACMD(do_mdump);
ACMD(do_odump);


ACMD(do_zreset)
{
	char *args = get_buffer(MAX_INPUT_LENGTH), *printbuf = get_buffer(MAX_INPUT_LENGTH);
	zone_rnum i;
	zone_vnum j=0;

	one_argument(argument, args);
	if (!*args) {
		send_to_char("You must specify a zone.\r\n", ch);
		release_buffer(args);
		release_buffer(printbuf);
		return;
	}
	if (*args == '*') {
		if (!IS_GRGOD(ch)) {
			send_to_char("You do not have permission to reset the entire world.\r\n", ch);
			release_buffer(args);
			release_buffer(printbuf);
			return;
		}
		for (i = 0; i <= top_of_zone_table; i++)
			reset_zone(i);
		send_to_char("Reset world.\r\n", ch);
		extended_mudlog(BRF, SYSL_RESETS, TRUE, "%s reset entire world.", GET_NAME(ch));
		release_buffer(args);
		release_buffer(printbuf);
		return;
	} else if (*args == '.') {
		i = world[IN_ROOM(ch)].zone;
		if (!CAN_EDIT_ZONE(ch, i)) {
			sprintf(printbuf, "You do not have permission to edit zone %d.\r\n", (IN_ROOM_VNUM(ch)/100));
			send_to_char(printbuf, ch);
			release_buffer(args);
			release_buffer(printbuf);
			return;
		}
	} else {
		if (!is_number(args)) {
			send_to_char("That's not a zone number.\r\n", ch);
			return;
		}
		j = atoi(args);
		for (i = 0; i <= top_of_zone_table; i++)
			if (zone_table[i].number == j)
				break;
	}
	release_buffer(args);
	if (i <= top_of_zone_table) {
		if (!CAN_EDIT_ZONE(ch, i)) {
			sprintf(printbuf, "You do not have permission to edit zone %d.\r\n", j);
			send_to_char(printbuf, ch);
			release_buffer(printbuf);
			return;
		}
		reset_zone(i);
		sprintf(printbuf, "Reset zone %d (#%d): %s.\r\n", i, zone_table[i].number,
						zone_table[i].name);
		send_to_char(printbuf, ch);
		release_buffer(printbuf);
		extended_mudlog(BRF, SYSL_RESETS, FALSE, "%s reset zone %d (%d): %s", GET_NAME(ch), i, zone_table[i].number, zone_table[i].name);
	} else
		send_to_char("Invalid zone number.\r\n", ch);
}


ACMD(do_liblist)
{
	int first, last, nr, found = 0;
  struct spell_info_type *sptr;
  struct tutor_info_type *tptr;
	char *list = get_buffer(65536);
	char *printbuf = get_buffer(MAX_INPUT_LENGTH), *printbuf1 = get_buffer(MAX_INPUT_LENGTH), *printbuf2 = get_buffer(MAX_INPUT_LENGTH);

	two_arguments(argument, printbuf, printbuf2);
	
	if (!*printbuf || !*printbuf2) {
		switch (subcmd) {
		 case SCMD_RLIST:
		 case SCMD_OLIST:
		 case SCMD_MLIST:
		 case SCMD_ZLIST:
		 case SCMD_SPLIST:
		 case SCMD_TUTORLIST:
			send_to_charf(ch, "Usage: %s <begining number> <ending number>\r\n", CMD_NAME);
			break;
		 default:
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SYSERR: invalid SCMD passed to ACMDdo_build_list!");
			break;
		}
		release_buffer(printbuf);
		release_buffer(printbuf1);
		release_buffer(printbuf2);
		release_buffer(list);
		return;
	}
	
	first = atoi(printbuf);
	last = atoi(printbuf2);
	
	if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
		send_to_char("Values must be between 0 and 99999.\r\n", ch);
		release_buffer(printbuf);
		release_buffer(printbuf1);
		release_buffer(printbuf2);
		release_buffer(list);
		return;
	}
	
	if (last - first > 2500 && subcmd != SCMD_ZLIST) {
		send_to_char("You may only list up to 25 zones at a time.\r\n", ch);
		release_buffer(printbuf);
		release_buffer(printbuf1);
		release_buffer(printbuf2);
		release_buffer(list);
		return;
	}
	
	if (first >= last) {
		send_to_char("Second value must be greater than first.\r\n", ch);
		release_buffer(printbuf);
		release_buffer(printbuf1);
		release_buffer(printbuf2);
		release_buffer(list);
		return;
	}
	
	switch (subcmd) {
	case SCMD_RLIST:
		sprintf(list,	"&c------------------------------------------------------------------------------&n\r\n"
									"&gRoom List From Vnum &G%d&g to &G%d&n\r\n"
									"&c==============================================================================&n\r\n"
									"    #    Vnum  Zone  Name\r\n"
									"&c------------------------------------------------------------------------------&n\r\n", first, last);
		for (nr = 0; nr <= top_of_world && (world[nr].number <= last); nr++) {
			if (world[nr].number >= first) {
				sprintf(list, "%s%5d. [&c%5d&n] (&y%3d&n) &y%s&n\r\n", list, ++found,
								world[nr].number, world[nr].zone,
								world[nr].name);
			}
		}
		break;
	case SCMD_OLIST:
		sprintf(list,	"&c------------------------------------------------------------------------------&n\r\n"
									"&gObject List From Vnum &G%d&g to &G%d&n\r\n"
									"&c==============================================================================&n\r\n"
									"    #    Vnum  Name\r\n"
									"&c------------------------------------------------------------------------------&n\r\n", first, last);
		for (nr = 0; nr <= top_of_objt && (obj_index[nr].vnum <= last); nr++)
		 {
			 if (obj_index[nr].vnum >= first) {
				 sprintf(list, "%s%5d. [&c%5d&n] &y%s&n\r\n", list, ++found,
								 obj_index[nr].vnum,
								 obj_proto[nr].short_description);
			 }
		 }
		break;
	case SCMD_MLIST:
		sprintf(list,	"&c------------------------------------------------------------------------------&n\r\n"
									"&gMobile List From Vnum &G%d&g to &G%d&n\r\n"
									"&c==============================================================================&n\r\n"
									"    #    Vnum  Name\r\n"
									"&c------------------------------------------------------------------------------&n\r\n", first, last);
		for (nr = 0; nr <= top_of_mobt && (mob_index[nr].vnum <= last); nr++)
		 {
			 if (mob_index[nr].vnum >= first) {
				 sprintf(list, "%s%5d. [&c%5d&n] &y%s&n (%s)\r\n", list, ++found,
					 mob_index[nr].vnum,
					 mob_proto[nr].player.short_descr,
					 race_list[(int)mob_proto[nr].player.race].name);
			 }
		 }
		break;
	case SCMD_ZLIST:
		sprintf(list,	"&c------------------------------------------------------------------------------&n\r\n"
									"&gZone List From Vnum &G%d&g to &G%d&n\r\n"
									"&c==============================================================================&n\r\n"
									"&CZone  Open Reset Builders                                                Range&n\r\n"
									"&c------------------------------------------------------------------------------&n\r\n", first, last);
		for (nr = 0; nr <= top_of_zone_table && (zone_table[nr].number <= last);nr++) {
			if (zone_table[nr].number >= first) {
				found++;
				sprintf(list, "%s[&c%3d&n] %-4.4s (&g%3d&n) &y%-47.47s&n [&c%5d&n-&c%5d&n]\r\n", list,
					zone_table[nr].number,
					YESNO(IS_SET(zone_table[nr].zone_flags, ZONE_OPEN)),
					zone_table[nr].lifespan,
					zone_table[nr].builders,
					zone_table[nr].bot,
					zone_table[nr].top
				);
			}
		}
		break;
	case SCMD_SPLIST:
		if( first == 0)
			first = 1;
		sprintf(list,	"&c------------------------------------------------------------------------------&n\r\n"
									"&gSpell List From Spellnum &G%d&g to &G%d&n\r\n"
									"&c==============================================================================&n\r\n", first, last);
		for (sptr = spell_info; sptr; sptr = sptr->next) {
			if (sptr->number > last)
				break;
			if (sptr->number >= first) {
				sprintf(list, "%s%5d. [&c%5d&n] &y%s&n (&c%s&n)&n\r\n",
					list,
					++found,
					sptr->number,
					sptr->name,
					YESNO(sptr->learned)
				);
			}
		}
		break;
	case SCMD_TUTORLIST:
		if( first == 0)
			first = 1;
		sprintf(list,	"&c------------------------------------------------------------------------------&n\r\n"
									"&gTutor List From tutornum &G%d&g to &G%d&n\r\n"
									"&c==============================================================================&n\r\n", first, last);
		for (tptr = tutor_info; tptr; tptr = tptr->next) {
			if (tptr->number > last)
				break;
			if (tptr->number >= first) {
				sprintf(list, "%s%5d. [&c%5d&n] &y%s&n (&c%d&n)&n\r\n",
					list,
					++found,
					tptr->number,
					real_mobile(tptr->vnum) != NOTHING ? mob_proto[real_mobile(tptr->vnum)].player.short_descr : "Nobody",
					tptr->vnum
				);
			}
		}
		break;
	default:
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "SYSERR: invalid SCMD passed to ACMDdo_build_list!");
		release_buffer(printbuf);
		release_buffer(printbuf1);
		release_buffer(printbuf2);
		release_buffer(list);
		return;
	}
	
	if (!found) {
		switch (subcmd) {
		case SCMD_RLIST:
			send_to_char("No rooms found within those parameters.\r\n", ch);
			break;
		case SCMD_OLIST:
			send_to_char("No objects found within those parameters.\r\n", ch);
			break;
		case SCMD_MLIST:
			send_to_char("No mobiles found within those parameters.\r\n", ch);
			break;
		case SCMD_ZLIST:
			send_to_char("No zones found within those parameters.\r\n", ch);
			break;
		case SCMD_SPLIST:
			send_to_char("No spells found within those parameters.\r\n", ch);
			break;
		case SCMD_TUTORLIST:
			send_to_char("No tutors found within those parameters.\r\n", ch);
			break;
		default:
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SYSERR: invalid SCMD passed to do_liblist!");
			break;
		}
		release_buffer(printbuf);
		release_buffer(printbuf1);
		release_buffer(printbuf2);
		release_buffer(list);
		return;
	}
	
	page_string(ch->desc, list, TRUE);
	release_buffer(printbuf);
	release_buffer(printbuf1);
	release_buffer(printbuf2);
	release_buffer(list);
	
}


ACMD(do_astat)
{
	char *printbuf = get_buffer(MAX_INPUT_LENGTH);
	int i, real = FALSE;
	
	if (IS_NPC(ch))
		return;
	
	one_argument(argument, printbuf);
	
	if(!*printbuf) {
		 send_to_char("Astat which social?\r\n", ch);
		 release_buffer(printbuf);
		 return;
	}
	
	for (i = 0; i <= top_of_socialt; i++) {
		if (is_abbrev(printbuf, soc_mess_list[i].command)) {
			real = TRUE;
			break;
		}
	}
	
	if (!real) {
		send_to_char("No such social.\r\n", ch);
		release_buffer(printbuf);
		return;
	}
	
	sprintf(printbuf,
					"\r\n"
					"Command         : %-15.15s\r\n"
					"Sort as Command : %-15.15s\r\n"
					"Min Position[CH]: %-8.8s\r\n"
					"Min Position[VT]: %-8.8s\r\n"
					"Show if Invis   : %s\r\n"
					"Char    [NO ARG]: %s\r\n"
					"Others  [NO ARG]: %s\r\n"
					"Char [NOT FOUND]: %s\r\n"
					"Char  [ARG SELF]: %s\r\n"
					"Others[ARG SELF]: %s\r\n"
					"Char      [VICT]: %s\r\n"
					"Others    [VICT]: %s\r\n"
					"Victim    [VICT]: %s\r\n"
					"Char  [BODY PRT]: %s\r\n"
					"Others[BODY PRT]: %s\r\n"
					"Victim[BODY PRT]: %s\r\n"
					"Char       [OBJ]: %s\r\n"
					"Others     [OBJ]: %s\r\n",
					
					soc_mess_list[i].command,
					soc_mess_list[i].sort_as,
					position_types[soc_mess_list[i].min_char_position],
					position_types[soc_mess_list[i].min_victim_position],
					(soc_mess_list[i].hide ? "HIDDEN" : "NOT HIDDEN"),
					soc_mess_list[i].char_no_arg ? soc_mess_list[i].char_no_arg : "",
					soc_mess_list[i].others_no_arg ? soc_mess_list[i].others_no_arg : "",
					soc_mess_list[i].not_found ? soc_mess_list[i].not_found : "",
					soc_mess_list[i].char_auto ? soc_mess_list[i].char_auto : "",
					soc_mess_list[i].others_auto ? soc_mess_list[i].others_auto : "",
					soc_mess_list[i].char_found ? soc_mess_list[i].char_found : "",
					soc_mess_list[i].others_found ? soc_mess_list[i].others_found : "",
					soc_mess_list[i].vict_found ? soc_mess_list[i].vict_found : "",
					soc_mess_list[i].char_body_found ? soc_mess_list[i].char_body_found : "",
					soc_mess_list[i].others_body_found ? soc_mess_list[i].others_body_found : "",
					soc_mess_list[i].vict_body_found ? soc_mess_list[i].vict_body_found : "",
					soc_mess_list[i].char_obj_found ? soc_mess_list[i].char_obj_found : "",
					soc_mess_list[i].others_obj_found ? soc_mess_list[i].others_obj_found : "");
	
	send_to_char(printbuf, ch);
	release_buffer(printbuf);
}


ACMD(do_copyto)
{
	char buf2[10];
	char buf[80];
	int iroom = 0, rroom = 0;
	struct room_data *room;
	
	one_argument(argument, buf2);
	/* buf2 is room to copy to */
	
	CREATE (room, struct room_data, 1);
	iroom = atoi(buf2);
	rroom = real_room(atoi(buf2));
	*room = world[rroom];
	
	if (!*buf2) {
		send_to_char("Usage: copyto <room number>\r\n", ch);
		return; }
	if (rroom <= 0) {
		sprintf(buf, "There is no room with the number %d.\r\n", iroom);
		send_to_char(buf, ch);
		return; }
	
	/* Main stuff */
	
	if (!CAN_EDIT_ZONE(ch, world[rroom].zone)) {
		sprintf(buf, "You do not have permission to edit zone %d.\r\n", (iroom/100));
		send_to_char(buf, ch);
		return;
	}

	if (world[IN_ROOM(ch)].description) {
		world[rroom].description = str_dup(world[IN_ROOM(ch)].description);
		
		add_to_save_list(zone_table[real_zone_by_thing(iroom)].number, SL_WLD);
	 
		sprintf(buf, "You copy the description to room &W%d&n.\r\n", iroom);
		send_to_char(buf, ch);
	} else
		send_to_char("This room has no description!\r\n", ch);
}


/* Find the next free vnum in the zone
	 Adapted from D. Tyler Barnes' BuildWalk snippet */
room_vnum	redit_find_new_vnum(zone_rnum zone) {

	room_vnum vnum;
	room_rnum rnum = real_room((vnum = genolc_zone_bottom(zone)));

	if (rnum != NOWHERE) {
		for(;;) {
			 if (vnum > zone_table[zone].top)
					return(NOWHERE);
			 if (rnum > top_of_world || world[rnum].number > vnum)
					break;
			 rnum++;
			 vnum++;
		}
	}
	return(vnum);
}


ACMD(do_dig)
{
	struct room_data *room;
	char buf2[10];
	char buf3[10];
	char buf[80];
	room_vnum vnum = NOWHERE;
	int iroom = 0, rroom = 0;
	int dir = 0;
	
	two_arguments(argument, buf2, buf3);
	/* buf2 is the direction, buf3 is the room */
	
	iroom = atoi(buf3);
	rroom = real_room(iroom);
	
	if (!*buf2) {
		send_to_char("Usage: dig <dir> <room number>\r\n", ch);
		return;
	}
	if (*buf3 && rroom <= 0) {
		sprintf(buf, "There is no room with the number %d.\r\nAttempting to create instead.\r\n", iroom);
		send_to_char(buf, ch);
	}

	/* Main stuff */
	if(!strcmp(buf2, "n") || !strcmp(buf2, "north"))
		dir = NORTH;
	else if(!strcmp(buf2, "e") || !strcmp(buf2, "east"))
		dir = EAST;
	else if(!strcmp(buf2, "s") || !strcmp(buf2, "south"))
		dir = SOUTH;
	else if(!strcmp(buf2, "w") || !strcmp(buf2, "west"))
		dir = WEST;
	else if(!strcmp(buf2, "u") || !strcmp(buf2, "up"))
		dir = UP;
	else if(!strcmp(buf2, "d") || !strcmp(buf2, "down"))
		dir = DOWN;
	else if(!strcmp(buf2, "ne") || !strcmp(buf2, "northeast"))
		dir = NORTHEAST;
	else if(!strcmp(buf2, "nw") || !strcmp(buf2, "northwest"))
		dir = NORTHWEST;
	else if(!strcmp(buf2, "se") || !strcmp(buf2, "southeast"))
		dir = SOUTHEAST;
	else if(!strcmp(buf2, "sw") || !strcmp(buf2, "southwest"))
		dir = SOUTHWEST;
	else if(!strcmp(buf2, "in"))
		dir = INDIR;
	else if(!strcmp(buf2, "out"))
		dir = OUTDIR;
	else {
		send_to_char("What direction? (n s e w u d ne nw se sw in out)\r\n", ch);
		return;
	}
	
	if (!CAN_EDIT_ZONE(ch, IN_ZONE(ch))) {
		sprintf(buf, "You do not have permission to edit zone %d (err0).\r\n", (IN_ROOM_VNUM(ch)/100));
		send_to_char(buf, ch);
		return;
	}

	if ((rroom > 0) && !CAN_EDIT_ZONE(ch, world[rroom].zone)) {
		sprintf(buf, "You do not have permission to edit zone %d (err1).\r\n", (iroom/100));
		send_to_char(buf, ch);
		return;
	}

	if (!*buf3 && (vnum = redit_find_new_vnum(world[IN_ROOM(ch)].zone)) == NOWHERE) {
		sprintf(buf, "No free vnums are available in zone #%d!\r\n", zone_table[IN_ZONE(ch)].number);
		send_to_char(buf, ch);
		return;
	}

	/* Either we have no argument, or the room didn't exist. */
	if (rroom <= 0) {
		/* If we have an vnum argument, we want to create a room with that vnum. */
		if (*buf3 && rroom <= 0)
			vnum = iroom;
		/* Otherwise we just pick the next free vnum. */
		else
			iroom = vnum;

		if (real_zone_by_thing(iroom) == NOWHERE) {
			sprintf(buf, "\r\n&RZone #%d does not exist, aborting creation.&n\r\n", (iroom/100));
			send_to_char(buf, ch);
			return;
		}

		if (!CAN_EDIT_ZONE(ch, real_zone_by_thing(iroom))) {
			sprintf(buf, "You do not have permission to edit zone %d (err2).\r\n", (iroom/100));
			send_to_char(buf, ch);
			return;
		}

		/* Set up data for add_room function */
		CREATE(room, struct room_data, 1);
		room->name = str_dup("New Room");
		sprintf(buf, "This unfinished room was created by %s.\r\n", GET_NAME(ch));
		room->description = str_dup(buf);
		room->number = iroom;
		room->zone = real_zone_by_thing(iroom);
		room->magic_flux = 2;
		room->available_flux = 0;
		room->sector_type = 1;

		/* Add the room */
		add_room(room);

		/* Memory cleanup */
		free(room->name);
		free(room->description);
		free(room);

		rroom = real_room(iroom);

		/* Report room creation to user */
		sprintf(buf, "\r\n&YNew room #%d created.&n\r\n\r\n", iroom);
		send_to_char(buf, ch);
	}

	CREATE(world[rroom].dir_option[rev_dir[dir]], struct room_direction_data,1);
	world[rroom].dir_option[rev_dir[dir]]->general_description = NULL;
	world[rroom].dir_option[rev_dir[dir]]->keyword = NULL;
	world[rroom].dir_option[rev_dir[dir]]->to_room = IN_ROOM(ch);
	
	CREATE(world[IN_ROOM(ch)].dir_option[dir], struct room_direction_data,1);
	world[IN_ROOM(ch)].dir_option[dir]->general_description = NULL;
	world[IN_ROOM(ch)].dir_option[dir]->keyword = NULL;
	world[IN_ROOM(ch)].dir_option[dir]->to_room = rroom;

	add_to_save_list(zone_table[IN_ZONE(ch)].number, SL_WLD);
	add_to_save_list(zone_table[real_zone_by_thing(iroom)].number, SL_WLD);
	
	send_to_char("EXIT CREATED\r\n", ch);

	sprintf(buf, "Direction: &W%s&n\r\n"
							 "From room: &W%d&n (%d)\r\n"
							 "  To room: &W%d&n (%d)\r\n",
							 dirs[dir],
							 IN_ROOM_VNUM(ch), zone_table[IN_ZONE(ch)].number,
							 iroom, zone_table[real_zone_by_thing(iroom)].number);

	send_to_char(buf, ch);

	sprintf(buf, "%s gestures and an exit to the %s appears!", GET_NAME(ch), dirs[dir]);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
}


ACMD(do_undig)
{
	char buf2[10];
	char buf[80];
	int dir = 0;
	struct room_direction_data *exit1 = NULL, *exit2 = NULL;
	room_rnum toroom;
	
	/* buf2 is the direction */
	one_argument(argument, buf2);
	
	if (!*buf2) {
		send_to_char("Format: undig <direction>\r\n", ch);
		return;
	}
	
	if(!strcmp(buf2, "n") || !strcmp(buf2, "north"))
		dir = NORTH;
	else if(!strcmp(buf2, "e") || !strcmp(buf2, "east"))
		dir = EAST;
	else if(!strcmp(buf2, "s") || !strcmp(buf2, "south"))
		dir = SOUTH;
	else if(!strcmp(buf2, "w") || !strcmp(buf2, "west"))
		dir = WEST;
	else if(!strcmp(buf2, "u") || !strcmp(buf2, "up"))
		dir = UP;
	else if(!strcmp(buf2, "d") || !strcmp(buf2, "down"))
		dir = DOWN;
	else if(!strcmp(buf2, "ne") || !strcmp(buf2, "northeast"))
		dir = NORTHEAST;
	else if(!strcmp(buf2, "nw") || !strcmp(buf2, "northwest"))
		dir = NORTHWEST;
	else if(!strcmp(buf2, "se") || !strcmp(buf2, "southeast"))
		dir = SOUTHEAST;
	else if(!strcmp(buf2, "sw") || !strcmp(buf2, "southwest"))
		dir = SOUTHWEST;
	else if(!strcmp(buf2, "in"))
		dir = INDIR;
	else if(!strcmp(buf2, "out"))
		dir = OUTDIR;
	else {
		send_to_char("What direction? (n s e w u d ne nw se sw in out)\r\n", ch);
		return;
	}
	
	exit1 = world[IN_ROOM(ch)].dir_option[dir];
	if (exit1) {
		toroom = exit1->to_room;
		exit2 = world[toroom].dir_option[rev_dir[dir]];
	}
	
	if (exit1) {
		if (!CAN_EDIT_ZONE(ch, IN_ZONE(ch))) {
			sprintf(buf, "You do not have permission to edit zone %d.\r\n", (IN_ROOM_VNUM(ch)/100));
			send_to_char(buf, ch);
			return;
		}
		
		if (exit1->general_description)
			free(exit1->general_description);
		if (exit1->keyword)
			free(exit1->keyword);
		free(exit1);
		world[IN_ROOM(ch)].dir_option[dir] = NULL;
	}
	else {
		send_to_char("There is no exit in that direction!\r\n", ch);
		return;
	}

	if (exit2 && !CAN_EDIT_ZONE(ch, world[toroom].zone)) {
		send_to_char("Other side remains due to zone permission!\r\n", ch);
		sprintf(buf, "You do not have permission to edit zone %d.\r\n", (GET_ROOM_VNUM(toroom) / 100));
		send_to_char(buf, ch);
	}
	else if (exit2) {
		if (exit2->general_description)
			free(exit2->general_description);
		if (exit2->keyword)
			free(exit2->keyword);
		free(exit2);
		world[toroom].dir_option[rev_dir[dir]] = NULL;
	}
	else {
		send_to_char("No returning exit from the other side, only one side deleted!\r\n", ch);
	}
	
	/* Only works if you have Oasis OLC */
	add_to_save_list(zone_table[real_zone_by_thing(GET_ROOM_VNUM(toroom))].number, SL_WLD);
	add_to_save_list(zone_table[IN_ZONE(ch)].number, SL_WLD);
	
	send_to_char("EXIT PURGED\r\n", ch);

	sprintf(buf, "Direction: &W%s&n\r\n"
							 "From room: &W%d&n (%d)\r\n"
							 "  To room: &W%d&n (%d)\r\n", dirs[dir],
							 IN_ROOM_VNUM(ch), (zone_table[IN_ZONE(ch)].number),
							 GET_ROOM_VNUM(toroom), zone_table[real_zone_by_thing(GET_ROOM_VNUM(toroom))].number);

	send_to_char(buf, ch);

	sprintf(buf, "%s gestures to the %s...what once was an exit has now vanished.", GET_NAME(ch), dirs[dir]);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
}


ACMD(do_saveall)
{
	char *input = get_buffer(MAX_INPUT_LENGTH);
	char *printbuf = get_buffer(MAX_INPUT_LENGTH);
	zone_rnum i;
	zone_vnum j = 0;

	one_argument(argument, input);

	if (!*input) {
		i = world[IN_ROOM(ch)].zone;
		if (!CAN_EDIT_ZONE(ch, i)) {
			sprintf(printbuf, "You do not have permission to save zone %d.\r\n", (IN_ROOM_VNUM(ch)/100));
			send_to_char(printbuf, ch);
			release_buffer(input);
			release_buffer(printbuf);
			return;
		}
	} else if (*input == '*') {
		if (!IS_GRGOD(ch)) {
			send_to_char("You do not have permission to save all world files.\r\n", ch);
			release_buffer(printbuf);
			release_buffer(input);
			return;
		}
		save_all();
		send_to_char ("All world files saved.\r\n", ch);
		extended_mudlog(BRF, SYSL_OLC, TRUE, "%s saved all world files.", GET_NAME(ch));
		release_buffer(printbuf);
		release_buffer(input);
		return;
	} else if (*input == '.') {
		i = world[IN_ROOM(ch)].zone;
		if (!CAN_EDIT_ZONE(ch, i)) {
			sprintf(printbuf, "You do not have permission to save zone %d.\r\n", (IN_ROOM_VNUM(ch)/100));
			send_to_char(printbuf, ch);
			release_buffer(input);
			release_buffer(printbuf);
			return;
		}
	}
	else {
		j = atoi(input);
		for (i = 0; i <= top_of_zone_table; i++)
			if (zone_table[i].number == j)
				break;
	}

	release_buffer(input);

	if (i >= 0 && i <= top_of_zone_table) {
		if (!CAN_EDIT_ZONE(ch, i)) {
			sprintf(printbuf, "You do not have permission to save zone %d.\r\n", j);
			send_to_char(printbuf, ch);
			release_buffer(printbuf);
			return;
		}
		save_all_zone(zone_table[i].number);
		sprintf(printbuf, "Saved zone %d (#%d): %s.\r\n", i, zone_table[i].number,
						zone_table[i].name);
		send_to_char(printbuf, ch);
		release_buffer(printbuf);
		extended_mudlog(BRF, SYSL_OLC, TRUE, "%s saved zone %d (#%d): %s", GET_NAME(ch), i, zone_table[i].number, zone_table[i].name);
	} else send_to_char("Invalid zone number.\r\n", ch);
}


ACMD(do_zdelete)
{
	zone_vnum i;
	zone_vnum zone;
	zone_vnum zbot, ztop;
	int real = 0, rec_count = 0;
	char *printbuf = get_buffer(MAX_INPUT_LENGTH), *printbuf2 = get_buffer(MAX_INPUT_LENGTH);
	MYSQL_RES *bootindex;

	if (subcmd != SCMD_ZDELETE) {
		send_to_char("If you want to delete a zone, say so!\r\n", ch);
		release_buffer(printbuf);
		release_buffer(printbuf2);
		return;
	}

	two_arguments(argument, printbuf, printbuf2);

	if (!*printbuf) {
		send_to_char("You have to supply a zone number for deletion!\r\n", ch);
		release_buffer(printbuf);
		release_buffer(printbuf2);
		return;
	}
	
	zone = atoi(printbuf);

	for (i = 0; i <= top_of_zone_table; i++) {
		if (zone_table[i].number == zone) {
			real = TRUE;
			break;
		}
	}

	if (!real) {
		send_to_char("No such zone.\r\n", ch);
		release_buffer(printbuf);
		release_buffer(printbuf2);
		return;
	}

	zbot = genolc_zone_bottom(i);
	ztop = zone_table[i].top;

	/* Delete all the zone's wld trigger assignments from the database. */
	if (!(bootindex = mysqlGetResource(TABLE_BOOT_INDEX, "SELECT mini FROM %s WHERE znum = %d AND mini = 0;", TABLE_BOOT_INDEX, zone))) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "%s:%s(%d) Error selecting zone index.", __FILE__, __FUNCTION__, __LINE__);
		release_buffer(printbuf);
		release_buffer(printbuf2);
		return;
	}

	rec_count = mysql_num_rows(bootindex);
	mysql_free_result(bootindex);

	if(rec_count == 0 && !IS_IMPLEMENTOR(ch)) {
		extended_mudlog(BRF, SYSL_SECURE, TRUE, "ZDEL: %s tried to delete required zone #%d.", GET_NAME(ch), zone);
		release_buffer(printbuf);
		release_buffer(printbuf2);
		return;
	}
	if (rec_count == 0 && strncmp(printbuf2, "yes", 3)) {
		send_to_char("You are trying to delete a required zone.  Please confirm.\r\n", ch);
		release_buffer(printbuf);
		release_buffer(printbuf2);
		return;
	}

	// Delete all the zone's rooms from the database.
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_WLD_INDEX, zone);

	// Delete all the zone's wld exits from the database.
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_WLD_EXITS, zone);

	// Delete all the zone's wld extra descriptions from the database.
	mysqlWrite("DELETE FROM %s WHERE znum BETWEEN %d AND %d;", TABLE_WLD_EXTRADESCS, zbot, ztop);

	// Delete all the zone's wld trigger assignments from the database.
	mysqlWrite("DELETE FROM %s WHERE znum BETWEEN %d AND %d AND type = %d;", TABLE_TRG_ASSIGNS, zbot, ztop, WLD_TRIGGER);

	// Delete all the zone's wld trigger assignments from the database.
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_BOOT_INDEX, zone);

	// Delete all the zone's mobs from the database.
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_MOB_INDEX, zone);

	// Delete all the zone's mob trigger assignments from the database.
	mysqlWrite("DELETE FROM %s WHERE znum BETWEEN %d AND %d AND type = %d;", TABLE_TRG_ASSIGNS, zbot, ztop, MOB_TRIGGER);

	// Delete all the zone's objects from the database.
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_OBJ_INDEX, zone);

	// Delete all the zone's obj extra descriptions from the database.
	mysqlWrite("DELETE FROM %s WHERE znum BETWEEN %d AND %d;", TABLE_OBJ_EXTRADESCS, zbot, ztop);

	// Delete all the zone's obj affects from the database.
	mysqlWrite("DELETE FROM %s WHERE znum BETWEEN %d AND %d;", TABLE_OBJ_AFFECTS, zbot, ztop);

	// Delete all the zone's obj trigger assignments from the database.
	mysqlWrite("DELETE FROM %s WHERE znum BETWEEN %d AND %d AND type = %d;", TABLE_TRG_ASSIGNS, zbot, ztop, OBJ_TRIGGER);

	// Delete all the zone's shops from the database.
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_SHP_INDEX, zone);

	// Delete all the zones's shop products from the database.
	mysqlWrite("DELETE FROM %s WHERE znum BETWEEN %d AND %d;", TABLE_SHP_PRODUCTS, zbot, ztop);

	// Delete all the zones's shop keywords from the database.
	mysqlWrite("DELETE FROM %s WHERE znum BETWEEN %d AND %d;", TABLE_SHP_KEYWORDS, zbot, ztop);

	// Delete all the zones's shop rooms from the database.
	mysqlWrite("DELETE FROM %s WHERE znum BETWEEN %d AND %d;", TABLE_SHP_ROOMS, zbot, ztop);

	// Delete the zone's triggers.
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_TRG_INDEX, zone);

	// Delete the zone's quests.
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_QST_INDEX, zone);

	// Delete all the zone commands.
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_ZON_COMMANDS, zone);

	// Delete the zone.
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_ZON_INDEX, zone);

	// Delete the zone from the boot table.
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_BOOT_INDEX, zone);

	extended_mudlog(BRF, SYSL_OLC, TRUE, "Zone #%d (%d-%d) deleted by %s.", zone, zbot, ztop, GET_NAME(ch));
	send_to_char("Zone Deletion Successful.\r\n", ch);

	release_buffer(printbuf);
	release_buffer(printbuf2);
}


/*
 mdump <what> <qty> <object> <pct>
 ex. 
 mdump 6112 100       -> 100 green dragons will be randomly scattered around the world
 mdump puff 20        -> 20 Puffs will be scattered
 mdump puff 15 99 10  -> 15 Puffs will be scattered and 10% of them will have a Scythe
 */
ACMD(do_mdump)
{
	room_rnum toroom;
	char numstr[20], *mobstr = get_buffer(64), *temp = get_buffer(64), *printbuf;
	char itemnum[40]={0}, percent[4]={0};
	int i=0, j=0;
	int nummob, mobvnum, mobrnum, itemvnum=0, itemrnum=0;
	struct obj_data *obj;
	struct char_data *mob;
	
	if (!*argument) {
		send_to_char("MDUMP <name|vnum> <qty (max 100)>\r\n",ch);
		send_to_char("MDUMP <name|vnum> <qty (max 100)> <item vnum> <percent>\r\n",ch);    
		send_to_char(" where <item vnum>  = what item should be randomly loaded on mob\r\n",ch);
		send_to_char("  and  <percent> = what portion should receive item.\r\n",ch);
		release_buffer(mobstr);
		release_buffer(temp);
		return;
	}
	skip_spaces (&argument);
	half_chop(argument, mobstr, numstr);    
	if ((argument=strchr(numstr,' '))) {      
		skip_spaces(&argument);
		two_arguments(argument,itemnum, percent);
		strcpy(temp, numstr);
		half_chop(temp,numstr,argument);
	}
	if (strlen(mobstr) <1) {
		send_to_char("MDUMP <name|vnum> <qty (max 100)>\r\n",ch);
		send_to_char("MDUMP <name|vnum> <qty (max 100)> <item vnum> <percent>\r\n",ch);    
		send_to_char(" where <item vnum>  = what item should be randomly loaded on mob\r\n",ch);
		send_to_char("  and  <percent> = what portion should receive item.\r\n",ch);    
		release_buffer(mobstr);
		release_buffer(temp);
		return;
	}
	
	if (!isdigit(*mobstr)) 
		mobvnum=mobile_name_to_num(mobstr);
	else mobvnum=atoi(mobstr);
	
	if (itemnum[0]) {  // what item should be loaded to these mobs?
		if (!isdigit(*itemnum)) 
			itemvnum=object_name_to_num(itemnum);
		else
			itemvnum=atoi(itemnum);            
	}
	
	if ((mobrnum = real_mobile(mobvnum)) < 0) {
		send_to_char("That mob does not exist.\r\n", ch);
		release_buffer(mobstr);
		release_buffer(temp);
		return;
	}
	if (itemnum[0]) {
		if ((itemrnum = real_object(itemvnum)) < 0) {
			send_to_char("The object you want to load does not exist.\r\n", ch);
			release_buffer(mobstr);
			release_buffer(temp);
			return;
		}
	}
	nummob=atoi(numstr);
	if (nummob<=0) nummob=1;
	else if (nummob > 100) nummob=100;   
	if (itemnum[0]) {
		i=atoi(percent);
		if (!i) (j=(int)(nummob/95));  // default to 5% load on any item
		else (j=(i*nummob)/100);
	}
	for (i=0;i<nummob;i++) {    
		do {
			toroom = number(0, top_of_world);
		} while (ROOM_FLAGGED(toroom, ROOM_PRIVATE | ROOM_DEATH) || ZONE_FLAGGED(world[toroom].zone,ZONE_NOSUMMON));
		mob = read_mobile(mobrnum,REAL);
		char_to_room(mob, toroom);
		/*       
		Since the mobs are randomly scattered, loading the first X% will also randomly
		scatter the objects.  Note:  a cleric could conceivably cast "summon 100.dragon", 
		"summon 99.dragon"..."summon 1.dragon"  knowing that the first few mobs he gets will 
		have the item, so if this is used as part of a quest be sure to scramble the mobs 
		that receive the item.        
		*/
		if (j) {
			obj=read_object(itemrnum, REAL);
			obj_to_char (obj,mob);
			j--;
		}
		load_mtrigger(mob);
		strcpy(mobstr, mob->player.short_descr);
	}
	printbuf = get_buffer(MAX_PROMPT_LENGTH);
	sprintf (printbuf,"Loaded %d cop%s of %s",i, nummob>1?"ies":"y", mobstr);
	if (itemnum[0]) 
		sprintf(printbuf+strlen(printbuf), " and a %s%% load of Obj. %s [%s].\r\n", percent[0]?percent:"5", itemnum, obj_proto[itemrnum].short_description);
	else 
		sprintf(printbuf+strlen(printbuf),".\r\n");
	send_to_char (printbuf,ch);
	release_buffer(printbuf);
}


/*
	odump <what> <qty>
		ex. 
			odump 99 5      -> randomly scatter 5 Scythes of Death around the world.
			odump signet 1  -> load 1 signet in a random room
*/
ACMD(do_odump)
{
	room_rnum toroom;
	char numstr[20], objstr[10], *printbuf;
	int i=0;
	int numobj, objvnum, objrnum;
	struct obj_data *obj;

	if (!*argument) {
		send_to_char("ODUMP <name | vnum> <quantity (max 100)> >\r\n",ch);    
		return;
	}
	skip_spaces (&argument);
	half_chop(argument, objstr, numstr);
	if (strlen(objstr) <1) {
		send_to_char("ODUMP <name | vnum> <quantity (max 100)> >\r\n",ch);
		return;
	}

	if (!isdigit(objstr[0]))
		objvnum=object_name_to_num(objstr);
	else
		objvnum=atoi(objstr);

	if ((objrnum = real_object(objvnum)) < 0) {
		send_to_char("That was an invalid object.\r\n", ch);
		return;
	}

	numobj=atoi(numstr);      
	if (numobj<=0)
		numobj=1;
	else if (numobj > 100)
		numobj=100;

	for (i=0;i<numobj;i++) {
		do {
			toroom = number(0, top_of_world);
		} while (ROOM_FLAGGED(toroom, ROOM_PRIVATE | ROOM_DEATH) || ZONE_FLAGGED(world[toroom].zone,ZONE_NOSUMMON));        
		obj = read_object(objrnum,REAL);
		obj_to_room(obj, toroom);   
		load_otrigger(obj);
	}
	printbuf = get_buffer(MAX_PROMPT_LENGTH);
	sprintf(printbuf, "OK.  Loaded %d copies of %s.\r\n",numobj, objstr);
	send_to_char (printbuf,ch);
	release_buffer(printbuf);
}


/*
 PURGEMOB <vnum | name> - purge all copies of a mob in the world
 Be careful when using names...
 */
ACMD(do_purgemob)
{
	char mobstr[50];
	unsigned int number=0;
	mob_rnum r_num;
	struct char_data *i=NULL;    
	struct char_data *mob=NULL;      
	char name[50];
	int count=0;
	char *printbuf = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument,printbuf);
	
	if(!*printbuf) {
		send_to_char("You need to give me a VNUM or NAME.\r\n", ch);
		release_buffer(printbuf);
		return;
	}

	/*If buf is a digit, change to name*/ 
	if(isdigit(*printbuf)) {
		if((number = atoi(printbuf)) < 0) {
			send_to_char("A NEGATIVE number??\r\n", ch);
			release_buffer(printbuf);
			return;
		}    
		if (((r_num = real_mobile(number)) < 0) || (number==0)) {
			send_to_char("There is no mob with that name or number.\r\n", ch);            
			release_buffer(printbuf);
			return;
		}
		/*change vnum to name*/
		r_num = real_mobile(number);  /*tap into mob prototypes to get a name*/
		for (i=character_list; i;) {
			if (i->nr == r_num) {
				strcpy(mobstr, i->player.name);
				break;
			} else
				i=i->next;          
		}
	} else
		strcpy(mobstr,printbuf);

	/*at this point, mobstr=name of mob to purge*/

	for (i = character_list; i;) {
		if ((isname(mobstr, i->player.name) || (!strcmp(mobstr,i->player.name))) && IS_NPC(i)) {
			if (i->player.short_descr)
				strcpy(name, i->player.short_descr);
			else
				strcpy(name, "Unknown");
			mob=i->next;
			extract_char(i);
			i=mob;
			count++;
		}
		else
			i=i->next;
	}  

	if (count > 0) {
		sprintf(printbuf, "OK.  Purged %d cop%s of %s.\r\n", count, count>1||count==0? "ies":"y", name);
		send_to_char (printbuf,ch);
		extended_mudlog(BRF, SYSL_GENERAL, TRUE, "%s Purged %d cop%s of %s.", GET_NAME(ch), count, count>1||count==0? "ies":"y", name);
	} else {
		send_to_char ("No mobs left to be purged.\r\n", ch);
	}
	release_buffer(printbuf);
}


/*
	purgeobj <what>
	Purge all copies of an object in the world; use vnum or name
*/
ACMD(do_purgeobj)
{
	char *printbuf = get_buffer(MAX_INPUT_LENGTH);
	char objstr[50];
	unsigned int number=0;
	mob_rnum r_num;
	struct obj_data *i=NULL;    
	struct obj_data *obj=NULL;      
	char name[50];
	int count=0;
	
	one_argument(argument,printbuf);
	
	if(!*printbuf) {
		send_to_char("You need to give me a VNUM or NAME.\r\n", ch);
		release_buffer(printbuf);
		return;
	}
	
	/*If buf is a digit, get name*/ 
	if(isdigit(*printbuf)) {
		if((number = atoi(printbuf)) < 0) {
			send_to_char("A NEGATIVE number?\r\n", ch);
			release_buffer(printbuf);
			return;
		}    
		if (((r_num = real_object(number)) < 0) || (number==0)) {
			send_to_char("There is no object with that name or number.\r\n", ch);            
			release_buffer(printbuf);
			return;
		}
		/*change vnum to name*/
		r_num = real_object(number);  /*tap into obj prototypes to get a name*/
		for (i=obj_proto; i;) {
			if (i->item_number == r_num) {
				strcpy(objstr, i->name);
				break;
			} else
				i=i->next;          
		}
	} else strcpy(objstr,printbuf);
	
	/*at this point, objstr=name of obj to purge*/
	
	for (i = obj_proto; i;) {
		if (isname(objstr, i->name) || (!strcmp(objstr,i->name))) {
			strcpy (name, i->short_description);
			obj=i->next;
			extract_obj(i);
			i=obj;
			count++;
		} else
			i=i->next;
	}   
	sprintf(printbuf,"OK.  Purged %d cop%s of %s.\r\n",count, count>1 ? "ies":"y", name);
	send_to_char (printbuf,ch);
	release_buffer(printbuf);
}


/*
dolog	- if this is called by a player (magic etc), set to 0, otherwise
it'll	be logged as the Imm oload command
*/
void load_to_recipient(struct char_data *ch, struct char_data *recipient, struct obj_data *obj, int dolog)
{
	char *printbuf = get_buffer(MAX_INPUT_LENGTH);
	char *printbuf2 = get_buffer(MAX_INPUT_LENGTH);
	char tooheavy=0;
	char toomany=0;
	
	if ((IS_CARRYING_W(recipient) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(recipient))
		tooheavy=1;
	
	if (IS_CARRYING_N(recipient) + 1 > CAN_CARRY_N(recipient)) toomany=1;
	
	/*allow Imp to oload !Take items for own use*/
	if ((tooheavy || toomany || (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)))) && !IS_IMPL(ch)) {
		if (dolog) {
			sprintf(printbuf2, "Oload: obj #%i by %s into %s\'s inventory",
			GET_OBJ_VNUM(obj), GET_NAME(ch), GET_NAME(recipient));                
		}
		if ((tooheavy) || (toomany)) {
			if (dolog)
				sprintf(printbuf2+strlen(printbuf2), " (too heavy/many).");
			send_to_char("That item was too heavy or one too many items.\r\n",ch);
		} else {
			sprintf(printbuf2+strlen(printbuf2), " (!TAKE).");
			send_to_char("That item is !TAKE.\r\n",ch);
		}
		if (dolog)
			send_to_char("Do NOT fool around with this command; it is always logged.\r\n",ch);
	} else {
		if (dolog) {
			sprintf(printbuf2, "Oload: obj #%i loaded by %s into %s\'s inventory.", 
			GET_OBJ_VNUM(obj), GET_NAME(ch), GET_NAME(recipient));                    
		}
		obj_to_char(obj,recipient); 
		send_to_charf(ch, "You place %s in %s\'s inventory.\r\n",
			obj->short_description, GET_NAME(recipient));
		act("$p appears in your inventory.", TRUE, ch, obj, recipient, TO_VICT);
	}
	if (dolog)
		extended_mudlog(BRF, SYSL_PLAYERS, TRUE, printbuf2);
	release_buffer(printbuf);
	release_buffer(printbuf2);
}


/*
 *  Oload vnum <recipient>
 *  If no recipient, load obj VNUM into user's inventory.  
 *  If recipient specificed and in game, load item into his inventory.
 *  If recipient cannot carry weight of item, it is aborted. 
 */
ACMD(do_oload)	
{
	char *printbuf = get_buffer(MAX_INPUT_LENGTH);
	int number=0, r_num;
	struct obj_data *obj;   
	struct char_data *recipient;
	char rec_buf[100];
	char *k;
	char name[50];
	char found_by_name=0;
	unsigned int temp=0,j=0;
	int i=0;

	two_arguments(argument, printbuf, rec_buf);

	if(!*printbuf) {
		send_to_char("You need to give me a VNUM or NAME.\r\n", ch);
		release_buffer(printbuf);
		return;
	}

	/*If 2.sword is Oloaded, these lines separate 2 (temp) from sword (buf)*/
	k = strchr(printbuf, '.');
	if (k) {
		for (j = 0; j < strlen(k); j++)
			name[j] = k[j+1];              
		name[50] = '\0';
		temp = (strlen(printbuf) - strlen(k));
		printbuf[temp] = '\0';
		temp=(atoi(printbuf)-1);         /*Number of item to find...*/       
		strcpy (printbuf,name);       
	}

	/*If buf is a digit, skip down to load by VNUM*/ 
	if(!isdigit(*printbuf)) {
		obj = obj_proto;
		for (i = 0; i < top_of_objt; i++) {
			if (!isname(printbuf, obj[i].name))
				continue;
			if (temp-- > 0)
				continue; 
			else {
				number = GET_OBJ_VNUM(&obj[i]);         
				found_by_name = 1;
				break;
			}
		}   /*loading by object name*/
	}  /*if !isdigit(printbuf)*/ 


	if (!found_by_name) {
		if ((number = atoi(printbuf)) < 0) {
			send_to_char("A NEGATIVE number??\r\n", ch);
			release_buffer(printbuf);
			return;
		}
	}

	if (((r_num = real_object(number)) < 0) || (number == 0)) {
		send_to_char("There is no object with that name or number.\r\n", ch);
		skip_spaces(&argument);
		extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "Oload: %s tried to load a non-existent object (oload %s).", GET_NAME(ch), argument);
		release_buffer(printbuf);
		return;
	}


	if (!*rec_buf) { /* no target for load */
		obj = read_object(r_num, REAL);  
		obj_to_char(obj, ch);
		send_to_charf(ch, "Poof!  You create %s.\r\n", obj->short_description);
	} else { /* there is a target for load */
		if (!(recipient = get_char_vis(ch, rec_buf, NULL, FIND_CHAR_WORLD, 1))) {
			send_to_charf(ch, "%s is not in the world.\r\n",rec_buf);
			release_buffer(printbuf);
			return;
		} else {               
			obj = read_object(r_num, REAL);  
			load_to_recipient(ch, recipient, obj, 0);
		}                              
	}  /* there is a load target */
	release_buffer(printbuf);
}


/* Just purging a player, but with a more interesting message */
ACMD(do_slay)
{
	char *printbuf = get_buffer(MAX_INPUT_LENGTH);
	struct char_data *vict;
	
	one_argument(argument, printbuf);
	
	if (*printbuf) {
		if ((vict = get_char_vis(ch, printbuf, NULL, FIND_CHAR_ROOM, 1))) {
			if (vict == ch) { // Can't purge yourself, hello.
				send_to_char("Fuuu!\r\n", ch);
				extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "%s tried suicide with the SLAY command.", GET_NAME(ch));
				release_buffer(printbuf);
				return;
			}
			if (!IS_NPC(vict) && (compare_rights(ch, vict) == -1)) {
				 send_to_char("You can't slay someone with more rights than you.\r\n", ch);
				 release_buffer(printbuf);
				 return;
			}
			if (!IS_ADMIN(ch)) {
				send_to_char("\r\nDon't play with this command.\r\n", ch);                    
				send_to_char("\r\nUnimaginable pain sears your body.\r\n",ch);
				sprintf(printbuf,"%s screams in agony and collapses.\r\n",GET_NAME(ch));
				act(printbuf, FALSE, ch, 0, vict, TO_NOTVICT);                                           
				extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "%s has tried to use the SLAY command on %s.", GET_NAME(ch), GET_NAME(vict));
				if (make_player_corpse)
					make_corpse(ch);
				if (soft_player_death)
					extract_char_soft(ch);
				else
					extract_char(ch);
				release_buffer(printbuf);
				return;
			}      
			if (!IS_NPC(vict)) {                    
				if (vict->desc) {
					act("$N screams in agony and collapses.", FALSE, ch, 0, vict, TO_NOTVICT);
					act("$N screams in agony and collapses.", FALSE, ch, 0, vict, TO_CHAR);
					sprintf(printbuf,"\r\nUnimaginable pain sears your body.\r\nThe last thought you have is the name ... %s.\r\n",GET_NAME(ch));
					send_to_char(printbuf,vict);
					extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "%s has slain %s.", GET_NAME(ch), GET_NAME(vict));
					if (make_player_corpse) 
						make_corpse(vict);
					if (soft_player_death)
						extract_char_soft(vict);
					else {
						extract_char(vict);
						STATE(vict->desc) = CON_CLOSE;
						vict->desc->character = NULL;
						vict->desc = NULL;
					}
					release_buffer(printbuf);
					return; 
				}
			}
			act("$N screams in agony and collapses.", FALSE, ch, 0, vict, TO_NOTVICT);
			act("$N screams in agony and collapses.", FALSE, ch, 0, vict, TO_CHAR);
			extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "(GC) %s has slain %s.", GET_NAME(ch), GET_NAME(vict));
			if (make_player_corpse)
				make_corpse(vict);
			extract_char(vict);
			
		}  /*cannot find target*/
		else {
			send_to_char("No one here by that name.\r\n", ch);
			release_buffer(printbuf);
			return;
		}
	}  /*if *printbuf...*/
	else
		send_to_char ("You need a target to slay.\r\n",ch);
	release_buffer(printbuf);
}


ACMD(do_freevnums)
{
	char *list = get_buffer(65536);
	char *type = get_buffer(MAX_INPUT_LENGTH);
	char *printbuf = get_buffer(MAX_INPUT_LENGTH);
	int nr, num = 0;
	zone_vnum vznum;
	zone_rnum rznum;

	two_arguments(argument, type, printbuf);

	if (!*type) {
		send_to_char("Usage: freevnums { room | object | mobile } <znum>\r\n", ch);
		release_buffer(list);
		release_buffer(type);
		release_buffer(printbuf);
		return;
	}

	if (!*printbuf) {
		vznum = zone_table[IN_ZONE(ch)].number;
		rznum = IN_ZONE(ch);
	} else {
		vznum = atoi(printbuf);
		for (rznum = 0; rznum <= top_of_zone_table; rznum++)
			if (zone_table[rznum].number == vznum)
				break;
		if (rznum < 0 || rznum > top_of_zone_table) {
			send_to_char("No such zone.\r\n", ch);
			release_buffer(list);
			release_buffer(type);
			release_buffer(printbuf);
			return;
		}
	}

	if (!strncmp(type, "room", strlen(type))) {
		strcpy(type, "rooms");
		sprintf(list, "\r\nFree rooms between &W%d&n and &W%d&n:\r\n\r\n", genolc_zone_bottom(rznum), zone_table[rznum].top);
		for (nr = genolc_zone_bottom(rznum); nr <= zone_table[rznum].top; nr++)
			if (real_room(nr) == NOWHERE)
				sprintf(list, "%s%-9d%s", list, nr, (num++ % 8 == 7) ? "\r\n" : "");
	} else if (!strncmp(type, "object", strlen(type))) {
		strcpy(type, "objects");
		sprintf(list, "\r\nFree objects between &W%d&n and &W%d&n:\r\n\r\n", genolc_zone_bottom(rznum), zone_table[rznum].top);
		for (nr = genolc_zone_bottom(rznum); nr <= zone_table[rznum].top; nr++)
			if (real_object(nr) == NOTHING)
				sprintf(list, "%s%-9d%s", list, nr, (num++ % 8 == 7) ? "\r\n" : "");
	} else if (!strncmp(type, "mobile", strlen(type))) {
		strcpy(type, "mobiles");
		sprintf(list, "\r\nFree mobiles between &W%d&n and &W%d&n:\r\n\r\n", genolc_zone_bottom(rznum), zone_table[rznum].top);
		for (nr = genolc_zone_bottom(rznum); nr <= zone_table[rznum].top; nr++)
			if (real_mobile(nr) == NOBODY)
				sprintf(list, "%s%-9d%s", list, nr, (num++ % 8 == 7) ? "\r\n" : "");
				num++;
	} else {
		send_to_char("Usage: freevnums { room | object | mobile } <znum>\r\n", ch);
		release_buffer(list);
		release_buffer(type);
		release_buffer(printbuf);
		return;
	}

	sprintf(list, "%s%s%sTotal free %s between %d and %d: &W%d&n\r\n", list, ((num > 0) ? "\r\n" : ""), ((num % 8 < 8) ? "\r\n" : ""), type, genolc_zone_bottom(rznum), zone_table[rznum].top, num);
	page_string(ch->desc, list, TRUE);

	release_buffer(list);
	release_buffer(type);
	release_buffer(printbuf);
}


ACMD(do_checkdescs)
{
	char *list = get_buffer(65536);
	char *type = get_buffer(MAX_INPUT_LENGTH);
	char *printbuf = get_buffer(MAX_INPUT_LENGTH);
	MYSQL_RES *result;
	MYSQL_ROW row;
	int rec_count, i;
	int num = 0;
	zone_vnum vznum;
	zone_rnum rznum;

	two_arguments(argument, type, printbuf);

	if (!*type) {
		send_to_char("Usage: checkdescs { room | object | mobile } <znum>\r\n", ch);
		release_buffer(list);
		release_buffer(type);
		release_buffer(printbuf);
		return;
	}

	if (!*printbuf) {
		vznum = zone_table[IN_ZONE(ch)].number;
		rznum = IN_ZONE(ch);
	} else {
		vznum = atoi(printbuf);
		for (rznum = 0; rznum <= top_of_zone_table; rznum++)
			if (zone_table[rznum].number == vznum)
				break;
		if (rznum < 0 || rznum > top_of_zone_table) {
			send_to_char("No such zone.\r\n", ch);
			release_buffer(list);
			release_buffer(type);
			release_buffer(printbuf);
			return;
		}
	}

	if (!strncmp(type, "room", strlen(type))) {
		strcpy(type, "rooms");
		sprintf(list, "\r\nMissing room descriptions between &W%d&n and &W%d&n:\r\n\r\n", genolc_zone_bottom(rznum), zone_table[rznum].top);
		if (!(result = mysqlGetResource(TABLE_WLD_INDEX, "SELECT vnum FROM %s WHERE LENGTH(description) < 80 AND znum = %d ORDER BY vnum;", TABLE_WLD_INDEX, vznum))) {
			extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not load list of rooms from [%s].", TABLE_WLD_INDEX);
			return;
		}
		rec_count = mysql_num_rows(result);
		for (i = 0; i < rec_count; i++) {
			row = mysql_fetch_row(result);
			sprintf(list, "%s%-9d%s", list, atoi(row[0]), (num++ % 8 == 7) ? "\r\n" : "");
		}
	} else if (!strncmp(type, "object", strlen(type))) {
		strcpy(type, "objects");
		sprintf(list, "\r\nMissing object extra descriptions between &W%d&n and &W%d&n:\r\n\r\n", genolc_zone_bottom(rznum), zone_table[rznum].top);
		if (!(result = mysqlGetResource(TABLE_OBJ_INDEX, "SELECT oi.vnum FROM %s oi LEFT JOIN %s oe ON oi.vnum = oe.vnum WHERE oe.vnum IS NULL AND oi.znum = %d ORDER BY oi.vnum;", TABLE_OBJ_INDEX, TABLE_OBJ_EXTRADESCS, vznum))) {
			extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not load list of rooms from [%s].", TABLE_OBJ_INDEX);
			return;
		}
		rec_count = mysql_num_rows(result);
		for (i = 0; i < rec_count; i++) {
			row = mysql_fetch_row(result);
			sprintf(list, "%s%-9d%s", list, atoi(row[0]), (num++ % 8 == 7) ? "\r\n" : "");
		}
	} else if (!strncmp(type, "mobile", strlen(type))) {
		strcpy(type, "mobiles");
		sprintf(list, "\r\nMissing mobile descriptions between &W%d&n and &W%d&n:\r\n\r\n", genolc_zone_bottom(rznum), zone_table[rznum].top);
		if (!(result = mysqlGetResource(TABLE_MOB_INDEX, "SELECT vnum FROM %s WHERE LENGTH(ddesc) < 80 AND znum = %d ORDER BY vnum;", TABLE_MOB_INDEX, vznum))) {
			extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not load list of rooms from [%s].", TABLE_MOB_INDEX);
			return;
		}
		rec_count = mysql_num_rows(result);
		for (i = 0; i < rec_count; i++) {
			row = mysql_fetch_row(result);
			sprintf(list, "%s%-9d%s", list, atoi(row[0]), (num++ % 8 == 7) ? "\r\n" : "");
		}
	} else {
		send_to_char("Usage: checkdescs { room | object | mobile } <znum>\r\n", ch);
		release_buffer(list);
		release_buffer(type);
		release_buffer(printbuf);
		return;
	}

	sprintf(list, "%s%s%sTotal missing %s descriptions between %d and %d: &W%d&n\r\n", list, ((num > 0) ? "\r\n" : ""), ((num % 8 < 8) ? "\r\n" : ""), type, genolc_zone_bottom(rznum), zone_table[rznum].top, num);
	page_string(ch->desc, list, TRUE);

	release_buffer(list);
	release_buffer(type);
	release_buffer(printbuf);
}


ACMD(do_rescopy)
{
	char *args = get_buffer(MAX_INPUT_LENGTH);
	zone_rnum rzone = 0;
	room_rnum i = 0;

	one_argument(argument, args);

	if (!*args) {
		send_to_charf(ch, "Usage: %s { <vnum> | * }\r\n", CMD_NAME);
		release_buffer(args);
		return;
	}

	rzone = IN_ZONE(ch);

	if (!CAN_EDIT_ZONE(ch, rzone)) {
		send_to_charf(ch, "You do not have permission to edit zone %d.\r\n", zone_table[rzone].number);
		return;
	}

	if (*args == '*') {
		struct room_data *room;

		/*
		 * Loop through the entire zone and set all rooms with the
		 * same resources as the room that we are currently in.
		 */
		for (i = genolc_zone_bottom(rzone); i <= zone_table[rzone].top; i++) {
			int rnum;
			if ((rnum = real_room(i)) != NOWHERE) {
				room = (world + rnum);
				if (room->number != GET_ROOM_VNUM(IN_ROOM(ch))) {
					room->resources.resources = world[IN_ROOM(ch)].resources.resources;
					room->resources.max[RESOURCE_ORE] = world[IN_ROOM(ch)].resources.max[RESOURCE_ORE];
					room->resources.max[RESOURCE_GEMS] = world[IN_ROOM(ch)].resources.max[RESOURCE_GEMS];
					room->resources.max[RESOURCE_WOOD] = world[IN_ROOM(ch)].resources.max[RESOURCE_WOOD];
					room->resources.max[RESOURCE_STONE] = world[IN_ROOM(ch)].resources.max[RESOURCE_STONE];
					room->resources.max[RESOURCE_FABRIC] = world[IN_ROOM(ch)].resources.max[RESOURCE_FABRIC];
				}
			}
		}

		send_to_charf(ch, "Set all resources in zone %d (%d): %s.\r\n", rzone, zone_table[rzone].number, zone_table[rzone].name);
		extended_mudlog(BRF, SYSL_OLC, TRUE, "%s set all resources in zone %d (%d): %s.", GET_NAME(ch), rzone, zone_table[rzone].number, zone_table[rzone].name);
		release_buffer(args);
		return;

	} else {

		if (!is_number(args)) {
			send_to_char("That's not a room number.\r\n", ch);
			release_buffer(args);
			return;
		}

		if ((i = real_room(atoi(args))) == NOWHERE) {
			send_to_charf(ch, "No such room.\r\n");
			release_buffer(args);
			return;
		}

		if (world[i].number != GET_ROOM_VNUM(IN_ROOM(ch))) {
			world[i].resources.resources = world[IN_ROOM(ch)].resources.resources;
			world[i].resources.max[RESOURCE_ORE] = world[IN_ROOM(ch)].resources.max[RESOURCE_ORE];
			world[i].resources.max[RESOURCE_GEMS] = world[IN_ROOM(ch)].resources.max[RESOURCE_GEMS];
			world[i].resources.max[RESOURCE_WOOD] = world[IN_ROOM(ch)].resources.max[RESOURCE_WOOD];
			world[i].resources.max[RESOURCE_STONE] = world[IN_ROOM(ch)].resources.max[RESOURCE_STONE];
			world[i].resources.max[RESOURCE_FABRIC] = world[IN_ROOM(ch)].resources.max[RESOURCE_FABRIC];	
			send_to_charf(ch, "Copied resources from room %d to room %d.\r\n", world[IN_ROOM(ch)].number, world[i].number);
			extended_mudlog(BRF, SYSL_OLC, TRUE, "%s copied resources from room %d to room %d.", GET_NAME(ch), world[IN_ROOM(ch)].number, world[i].number);
		} else {
			send_to_charf(ch, "Now, what would be the point of that?\r\n");
		}

	}

	release_buffer(args);

}
