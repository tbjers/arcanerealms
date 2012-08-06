/* ************************************************************************
*	 File: reports.c                                                        *
*	 Usage: Misc. functions for on-line status reports                      *
*																																					*
***************************************************************************/
/* $Id: reports.c,v 1.14 2003/04/10 18:03:39 cheron Exp $ */
/*

 These are several functions created to assist me in taking inventory
of new areas and spot-checking my own work.  There are several functions
in here, but only do_wlist needs to be added to the command interpreter.
The	functions in this file seem a little fragmented since I am still 
trying to decide exactly what I personally want to display.
	After the function list I have included a paragraph that can be added
to your Wizhelp file.


Main function ACMD (do_wlist)

sub	functions:

void do_olist (struct char_data *ch, char *input);
	List all objs in specified zone.  If no arg, it defaults to your current 
	zone.  It includes type; for Armor/worn, it shows where and AC; for weapons
	it shows damage dice and +dam applies.
		calls: 
		 void list_object_zone  (struct char_data *ch, int whichzone);
			int get_weapon_dam (struct obj_data *o);
			int get_armor_ac (struct obj_data *o);
	
void do_mlist (struct char_data *ch, char *input);
	List all mobs in specified zone, plus their levels.  If no arg, it
	defaults to your current zone.
		calls:
			void list_mob_zone  (struct char_data *ch, int whichzone);

void list_wands_staves (struct char_data *ch, char *input);
void list_scrolls_potions (struct char_data *ch, char *input);
 List each specified item, giving spell and charge information


WLIST
(World list)

Options
	Wlist obj <zone> | mob <zone>
	Wlist S   (scrolls)
	Wlist P   (potions)
	Wlist T   (staves)
	Wlist W   (wands)
	Wlist #   (wear position)
	

Wlist	o | m  <zone>

	This will list all of the mobs or objects available to you, whether
loaded in the game or not.  If no zone is specified, you will see all
mobs/objects in your current zone.  Both will display the vnum and short
description.	For a mob, you'll be given its level.  For objects, you'll
see	the wear position.  Also, Armor will show normal and magical AC, and
weapons	will include damage dice and +damroll apply.
	If you specify a zone that has no objects because it is the second half
of another zone, objects from the earlier zone will be displayed.

Wlist	S | P | W | T

	This will list all the magical items available in the game.  All four will 
give the vnum and short description. Wand and sTaff will include the spell 
it casts and the number of charges it holds.  Potion and Scroll will 
list all the spells of the item.

Wlist	# N | ?

	This will list all items which can be worn in a specific position.  To see a 
list of the positions available, use WLIST # ?, then replace the ? with the proper number.

Ex:	 WLIST # ?   (List shows WRIST is position 14)
		 WLIST # 14  (displays all wrist-wear in game)
#


*/


#include "conf.h"
#include "sysdep.h"

#include "structs.h"
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
#include "olc.h"        /*to avoid repeating NUM_ITEM_WEARS*/
#include "specset.h"
#include "buffer.h"

/*	 external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto; 
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct char_data *mob_proto;
extern zone_rnum top_of_zone_table;
extern struct player_index_element *player_table;
extern int top_of_p_file; 
extern const int MAX_PLAYERS;
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern room_rnum top_of_world;
extern void get_spec_name (int i, char *specname, char type);
extern struct char_data *read_mobile(mob_vnum nr, int type);
extern struct attack_hit_type attack_hit_text[];
void char_to_room(struct char_data * ch, room_rnum room);

/*Used by REPORT command*/
extern const char *wear_bits[];
extern const char *room_spec_procs[];
extern const char *mob_spec_procs[];
extern const char *obj_spec_procs[];
extern const char *affect_bits[];
extern const char *action_bits[];
extern const char *extra_bits[];
extern const char *apply_types[];
extern const char *room_bits[];
extern const char *zone_bits[];
extern const char *sector_types[];
extern const char *npc_class_types[];
struct help_index_element *help_table;
struct help_index_element *wizhelp_table;
struct help_index_element *olchelp_table;

extern int top_of_helpt;
extern int top_of_olchelpt;
extern int top_of_wizhelpt;

extern int get_weapon_dam (struct obj_data *o);

/* local functions */
int get_armor_ac (struct obj_data *o);
void list_object_zone (struct char_data *ch, int whichzone);
void list_mob_zone (struct char_data *ch, int whichzone);
void list_room_zone (struct char_data *ch, int whichzone);
void do_olist (struct char_data *ch, char *input);
void do_mlist (struct char_data *ch, char *input);
void do_rlist (struct char_data *ch, char *input);
void list_wands_staves (struct char_data *ch, char *input);
void list_scrolls_potions (struct char_data *ch, char *input);
void do_list_wear (struct char_data *ch, char *input);
void do_zone_report(struct char_data *ch, char *arg);
void do_help_report (struct char_data *ch);
void do_obj_report (struct char_data *ch);
void do_mob_report (struct char_data *ch);
void do_list_report (struct char_data *ch);
ACMD (do_wlist);


void list_room_zone (struct char_data *ch, int whichzone)
{
	int i=0;
	int j=0;
	room_vnum first_z_room=0; /*vnum of first obj in zone list*/
	room_vnum last_z_room=0; /*vnum of last obj in zone list */
	int char_zone = ((GET_ROOM_VNUM(IN_ROOM(ch)))/100); /*initialize to current zone    */
	char *rbuf = get_buffer(MAX_STRING_LENGTH);

	rbuf[0]='\0'; 
	if (whichzone > (-1))
		char_zone = whichzone; /*see if specific zone was passed as arg*/

	while (j!= char_zone) {
		first_z_room++;
		j=(world[i].number/100);
		i++;

		/*
		 * some zones have 100+ rooms but not 100+ items/objs.
		 * To find the needed objs/objects,
		 * search backwards for previous zone.
		 */
		if (j>char_zone) {
			if (char_zone>0) {
				char_zone--;
				first_z_room=0;
				i=0;
				j=0; 
			} else {
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "rlist looked for a zone < 0");
				release_buffer(rbuf);
				return;
			}
		}
	}

	first_z_room --;
	last_z_room=first_z_room;


	while (j == char_zone) {
		j=(world[i].number/100);
		last_z_room++;
		i++; 
	}

	if (first_z_room<0)
		first_z_room ++;
	for (i=first_z_room;i<last_z_room;i++) {
		sprintf(rbuf+strlen(rbuf),"[%5d]  %s\r\n",
		world[i].number,
		world[i].name);
	}
	
	if (strlen(rbuf)<5)
		send_to_char ("There are no rooms in the zone you requested.\r\n",ch);
	else
		page_string (ch->desc, rbuf, 1);

	release_buffer(rbuf);
	return;
}


void do_rlist (struct char_data *ch, char *input)
{
	if (!(*input))
		list_room_zone(ch,(-1)); 
	else {
		skip_spaces(&input);
		if (is_number(input))
			list_room_zone(ch,atoi(input));
		else
			list_room_zone(ch,((GET_ROOM_VNUM(IN_ROOM(ch))) /100)); 
	}
	return; 
}


void list_object_zone (struct char_data *ch, int whichzone)
{
	int i=0;
	int j=0;
	obj_vnum first_z_obj=0; /*vnum of first obj in zone list*/
	obj_vnum last_z_obj=0; /*vnum of last obj in zone list */
	int char_zone = ((GET_ROOM_VNUM(IN_ROOM(ch)))/100); /*initialize to current zone    */
	char *obuf = get_buffer(MAX_STRING_LENGTH);

	obuf[0]='\0'; 
	if (whichzone > (-1)) char_zone = whichzone; /*see if specific zone was passed as arg*/

	while (j!= char_zone) {
		first_z_obj++;
		j=(GET_OBJ_VNUM(&obj_proto[i])/100);
		i++;

		/*
		 * some zones have 100+ rooms but not 100+ items/objs.
		 * To find the needed objs/objects, 
		 * search backwards for previous zones w/them.
		 */
		if (j>char_zone) {
			if (char_zone>0) {
				char_zone--;
				first_z_obj=0;
				i=0;
				j=0; 
			} else {
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "olist looked for a zone < 0");
				release_buffer(obuf);
				return;
			}
		}
	}

	first_z_obj --;
	last_z_obj=first_z_obj;

	while (j == char_zone) {
		j=(GET_OBJ_VNUM(&obj_proto[i])/100);
		last_z_obj++;
		i++; 
	}

	if (first_z_obj<0)
		first_z_obj ++;
	for (i=first_z_obj;i<last_z_obj;i++) {
		sprintf(obuf+strlen(obuf),"[%5d] %-32s",
		GET_OBJ_VNUM(&obj_proto[i]),
		obj_proto[i].short_description);

		j=obj_proto[i].obj_flags.type_flag;
		sprintf(obuf+strlen(obuf)," %s ", item_types[(int)obj_proto[i].obj_flags.type_flag]);
		if ((j == ITEM_WORN) || (j== ITEM_ARMOR)) {
			sprintf(obuf+strlen(obuf)," (-%d AC 1) ",GET_OBJ_VAL(&obj_proto[i],0));
			j=get_armor_ac (&obj_proto[i]);
			if (j!=0)
				sprintf(obuf + strlen(obuf), "(-%d AC 2)",j); 
		}

		if (j==ITEM_WEAPON) {
			sprintf(obuf+strlen(obuf),"(Ave dam: %.1f) ", 
			(((GET_OBJ_VAL(&obj_proto[i], 2) + 1) /2.0) * GET_OBJ_VAL(&obj_proto[i], 1))); 
			j = get_weapon_dam(&obj_proto[i]);
			if (j!=0) sprintf(buf+strlen(buf), " +%d DAM ",j); 
		}
		sprintbit(GET_OBJ_WEAR(&obj_proto[i]), wear_bits, obuf+strlen(obuf), sizeof(obuf));
		sprintf(obuf+strlen(obuf),"\r\n");
	}
	if (strlen(obuf)<5) send_to_char ("There are no objects in the zone you requested.\r\n",ch);
	else page_string (ch->desc,obuf,1);
	release_buffer(obuf);
	return;
}


void do_olist (struct char_data *ch, char *input)
{
	if (!(*input))
		list_object_zone(ch,(-1)); 
	else {
		skip_spaces(&input);
		if (is_number(input))
			list_object_zone(ch,atoi(input));
		else
			list_object_zone(ch,((GET_ROOM_VNUM(IN_ROOM(ch))) /100)); 
	}
	return; 
}


void list_mob_zone (struct char_data *ch, int whichzone)
{

	int i=0;
	int j=0;
	mob_vnum first_z_mob=0; /*vnum of first mob in zone list*/
	mob_vnum last_z_mob=0; /*vnum of last mob in zone list */
	int char_zone = ((GET_ROOM_VNUM(IN_ROOM(ch)))/100); /*initialize to current zone    */
	char *mbuf = get_buffer(MAX_STRING_LENGTH);

	mbuf[0]='\0'; 
	if (whichzone > (-1))
		char_zone = whichzone; /*see if specific zone was passed as arg*/

	while (j!= char_zone) {
		first_z_mob++;
		j=(GET_MOB_VNUM(&mob_proto[i])/100);
		i++;

		/*
		 * some zones have 100+ rooms but not 100+ items/mobs.
		 * To find the needed mobs/objects, 
		 * search backwards for previous zone w/them.
		 */
		if (j>char_zone) {
			if (char_zone>0) {
				char_zone--;
				first_z_mob=0;
				i=0;
				j=0; 
			} else {
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "mlist looked for a zone < 0");
				release_buffer(mbuf);
				return;
			}
		}
	}

	first_z_mob --;
	last_z_mob=first_z_mob;


	while (j == char_zone) {
		j=(GET_MOB_VNUM(&mob_proto[i])/100);
		last_z_mob++;
		i++; 
	}

	if (first_z_mob<0)
		first_z_mob ++;
	for (i=first_z_mob;i<last_z_mob;i++) {
		sprintf(mbuf+strlen(mbuf),"[%5d] %-30s  (difficulty %d)\r\n",
		GET_MOB_VNUM(&mob_proto[i]),
		mob_proto[i].player.short_descr,
		mob_proto[i].mob_specials.difficulty); 
	}
	if (strlen(mbuf)<5)
		send_to_char ("There are no mobs in the zone you requested.\r\n",ch);
	else
		page_string (ch->desc,mbuf,1);

	release_buffer(mbuf);
	return;
}


void do_mlist (struct char_data *ch, char *input)
{
	if (!(*input))
		list_mob_zone(ch,(-1)); 
	else {
		skip_spaces(&input);
		if (is_number(input))
			list_mob_zone(ch,atoi(input));
		else
			list_mob_zone(ch,((GET_ROOM_VNUM(IN_ROOM(ch))) /100)); 
	}
	return; 
}


void list_wands_staves (struct char_data *ch, char *input)
{
	int type=0;
	int i=0;
	int j=0;
	int k=0;
	char *wsbuf = get_buffer(MAX_STRING_LENGTH);

	skip_spaces(&input);
	switch (input[0]) {
	case 'T':
	case 't':
		type = ITEM_STAFF; 
		break;
	case 'W':
	case 'w':
		type = ITEM_WAND; 
		break;
	default:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Default reached in list_scrolls_potions (arg = %s)", input);
		release_buffer(wsbuf);
		return; 
	}	/*switch...*/

	wsbuf[0]='\0';
	for (i=0;i<top_of_objt;i++) {
		j=obj_proto[i].obj_flags.type_flag; /*look for specific sort of item*/
		if (j == type) { /*found one*/
			sprintf(wsbuf+strlen(wsbuf),"[%5d] %-30s", /*print vnum, short description*/
			GET_OBJ_VNUM(&obj_proto[i]),
			obj_proto[i].short_description);

			/*
			 * values 0-3:
			 * Potion, Scroll - up to three spells [values 1-3]
			 */

			sprintf(wsbuf+strlen(wsbuf), " Spells: ");
			if (type==ITEM_STAFF) { /*staves have only one spell*/
				if ((GET_OBJ_VAL(&obj_proto[i], 3)) != (-1))
					sprintf(wsbuf+strlen(wsbuf), "%s ", skill_name(GET_OBJ_VAL(&obj_proto[i], 3)));
			} else {
				for (k=1; k < 4; k++) {
					if ((GET_OBJ_VAL(&obj_proto[i], k)) != (-1))
						sprintf(wsbuf+strlen(wsbuf), "%s ", skill_name(GET_OBJ_VAL(&obj_proto[i], k)));
				}
				sprintf(wsbuf+strlen(wsbuf), "\r\n");
			}
		}	/*if j == type*/
	}	/*for i...*/
	page_string (ch->desc, wsbuf, 1);
	release_buffer(wsbuf);
}



void list_scrolls_potions (struct char_data *ch, char *input)
{
	int type=0;
	int i=0;
	int j=0;
	int k=0;
	char *spbuf = get_buffer(MAX_STRING_LENGTH);

	skip_spaces(&input);
	switch (input[0]) {
	case 'S':
	case 's':
		type = ITEM_SCROLL; 
		break;
	case 'P':
	case 'p':
		type = ITEM_POTION; 
		break;
	default :
		mlog("SYSERR:  Default reached in list_scrolls_potions (arg = %s)", input);
		release_buffer(spbuf);
		return;
	}/*switch...*/

	spbuf[0]='\0';
	for (i=0;i<top_of_objt;i++) {
		j=obj_proto[i].obj_flags.type_flag; /*look for specific sort of item*/
		if (j == type) { /*found one*/
			sprintf(spbuf+strlen(spbuf),"[%5d] %-20s", /*print vnum, short description*/
			GET_OBJ_VNUM(&obj_proto[i]),
			obj_proto[i].short_description);

			/*
			 * values 0-3:
			 * Potion, Scroll - up to three spells [values 1-3]
			 */

			sprintf(spbuf+strlen(spbuf), " Spells: ");
			for (k=1;k<4;k++) {
				if ((GET_OBJ_VAL(&obj_proto[i], k)) != (-1))
					sprintf(spbuf+strlen(spbuf), "%s ", skill_name(GET_OBJ_VAL(&obj_proto[i], k)));
			}


			sprintf(spbuf+strlen(spbuf), "\r\n");

		}/*if j == type*/
	}/*for i...*/
	page_string (ch->desc, spbuf, 1);
	release_buffer(spbuf);
}



void do_list_wear (struct char_data *ch, char *input)
{
	char j = atoi(input);
	int i=0;
	char *wbuf = get_buffer(MAX_STRING_LENGTH);

	if (input[0] == '?') {
		j=0; 
		send_to_char ("Wear positions:\r\n", ch);
		for (i = 0; i < NUM_ITEM_WEARS; i++) {
			sprintf(buf, "(%2d) %-20.20s %s", 
			i + 1, wear_bits[i], 
			!(++j % 2) ? "\r\n" : "");
			send_to_char(buf, ch); 
		}
		send_to_char("\r\nIf you choose TAKE, you will be shown item that are !Take\r\n",ch);
		release_buffer(wbuf);
		return;
	}

	wbuf[0]='\0';

	j--; /*to be used with NAMES array*/

	if (j==0) { /*Show ony !Take items for this option*/
		for (i=0;i<top_of_objt;i++) /*cycle through every obj*/
			if (!(CAN_WEAR(&obj_proto[i], (1<<j)))) { /*check exact bit for requested position*/
				sprintf(wbuf+strlen(wbuf),"[%5d] %-32s !TAKE\r\n",
				GET_OBJ_VNUM(&obj_proto[i]),
				obj_proto[i].short_description); 
			}
		page_string (ch->desc, wbuf, 1); 
		release_buffer(wbuf);
		return;
	}

	for (i=0;i<top_of_objt;i++) { /*cycle through every obj*/
		if (CAN_WEAR(&obj_proto[i], (1<<j))) { /*check exact bit for requested position*/
			sprintf(wbuf+strlen(wbuf),"[%5d] %-32s ",
			GET_OBJ_VNUM(&obj_proto[i]),
			obj_proto[i].short_description); 
			sprintf(wbuf+strlen(wbuf),"%s\r\n", wear_bits[(int)j]); /*repeat position*/
		}
	}
	if (!buf)
		send_to_char("There are no items of that type in the object files.c",ch);
	page_string (ch->desc, wbuf, 1); 

	release_buffer(wbuf);
}


ACMD (do_wlist)
{
	char *message="Usage: wlist [ o | m | s | t | w | p | r | # <number> ] <zone>\r\nType HELP WLIST for details\r\n";
	char *obuf = get_buffer(MAX_INPUT_LENGTH); /* option buffer */
	char *zbuf = get_buffer(MAX_INPUT_LENGTH); /* zone buffer */

	two_arguments(argument, obuf, zbuf);

	if (!*obuf) {
		send_to_char(message,ch);
		release_buffer(obuf);
		release_buffer(zbuf);
		return;
	}

	switch (obuf[0]) {
	case 'O':
	case 'o':
		do_olist(ch, zbuf); 
		break;
	case 'M':
	case 'm':
		do_mlist(ch, zbuf); 
		break;
	case 'T':
	case 't':
	case 'W':
	case 'w':
		list_wands_staves(ch, obuf); 
		break;
	case 'P':
	case 'p':
	case 'S':
	case 's':
		list_scrolls_potions(ch, obuf); 
		break;
	case 'R':
	case 'r':
		do_rlist(ch, zbuf); 
		break;
	case '#':
		do_list_wear(ch, zbuf); 
		break;
	default:
		send_to_char (message, ch);
		break;
	}/*switch...*/

	release_buffer(obuf);
	release_buffer(zbuf);
	return;
}


/**********************************************************************************/
/*****************************		Mobs  *******************************************/
/**********************************************************************************/

void do_mob_report (struct char_data *ch)
{
	struct char_data *mob; 
	FILE *reportfile; 
	int i;

	if (!(reportfile = fopen("report.mob", "w"))) {
		mlog("SYSERR:  Mob report file unavailable.");
		send_to_char ("Report.mob could not be generated.\r\n",ch);
		return;
	}
	sprintf(buf, "MOBS\n----\n");
	for (i=0; i<top_of_mobt;i++) {
		mob=read_mobile(i, REAL);
		char_to_room(mob, 0);
		sprintf(buf+strlen(buf), "[%5d] %s  Spec Proc: ",
		GET_MOB_VNUM(mob), GET_NAME(mob));

		if (mob_index[GET_MOB_RNUM(mob)].func!=NULL)
			get_spec_name(GET_MOB_RNUM(mob), buf2,'m');
		else sprintf(buf2, "none"); 
		sprintf(buf+strlen(buf), "%s\n",buf2);

		sprintf(buf+strlen(buf),mob->player.description);
		sprintf(buf+strlen(buf),"Difficulty: %d   XP: %d  HP: %d  Mana: %d  Gold %d\n",
		GET_DIFFICULTY(mob), GET_EXP(mob), GET_MAX_HIT(mob), GET_MAX_MANA(mob), GET_GOLD(mob));
		sprintf(buf+strlen(buf),"Passive Defense: %d   Damage Reduction: %d  ", 
		GET_PD(mob), GET_REDUCTION(mob));
		sprintf(buf+strlen(buf), "Attack Type: %s\n",
		attack_hit_text[mob->mob_specials.attack_type].singular);

		sprintbit(MOB_FLAGS(mob), action_bits, buf2, sizeof(buf2));
		sprintf(buf+strlen(buf),"Flags:  %s\n", buf2);

		sprintbit(AFF_FLAGS(mob), affected_bits, buf2, sizeof(buf2));
		sprintf(buf+strlen(buf),"Affects:  %s\n\n-------\n", buf2);
		extract_char(mob);
		fprintf(reportfile, buf);
		buf[0]='\0';
	}/*for i=0...*/
	fclose (reportfile);
	send_to_char ("report.mob printed\r\n",ch);
}

/**********************************************************************************/
/****************************	 Objects  *******************************************/
/**********************************************************************************/

void do_obj_report (struct char_data *ch)
{
	struct obj_data *obj, *key;
	int i=0, j=0, found=0;

	FILE *reportfile; 

	if (!(reportfile = fopen("report.obj", "w"))) {
		mlog("SYSERR:  Object report file unavailable.");
		send_to_char ("Report.obj could not be generated.\r\n",ch);
		return;
	}
	sprintf(buf, "OBJECTS\n-------\n");
	for (i=0; i<top_of_objt;i++) {
		obj=read_object(i, REAL);
		sprintf(buf+strlen(buf), "[%5d] %s\nSpec Proc: ",
		GET_OBJ_VNUM(obj), obj->short_description);

		if (obj_index[GET_OBJ_RNUM(obj)].func!=NULL)
			get_spec_name(GET_OBJ_RNUM(obj), buf2,'o');
		else sprintf(buf2, "none");

		sprintf(buf+strlen(buf), "%s  Aliases:  %s\n", buf2, GET_OBJ_NAME(obj));

		sprinttype(GET_OBJ_TYPE(obj), item_types, buf2, sizeof(buf2));
		sprintf (buf+strlen(buf),"Type:  %s    Worn on: ",buf2); 
		sprintbit(obj->obj_flags.wear_flags, wear_bits, buf2, sizeof(buf2));
		sprintf(buf+strlen(buf), "%s\n", buf2); 

		sprintf(buf+strlen(buf), "Weight: %d, Value: %d, Cost/day: %d, Timer: %d\n",
		GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_TIMER(obj));

		sprintbit(obj->obj_flags.bitvector, affected_bits, buf2, sizeof(buf2));
		sprintf(buf+strlen(buf), "Affects player: %s\n",buf2);

		sprintbit(obj->obj_flags.bitvector, affected_bits, buf2, sizeof(buf2));
		sprintf(buf+strlen(buf), "Extra bits:  %s\n",buf2); 


		switch (GET_OBJ_TYPE(obj)) {
		case ITEM_LIGHT:
			if (GET_OBJ_VAL(obj, 2) == -1) strcpy(buf, "Hours left: Infinite\n");
			else sprintf(buf+strlen(buf), "Hours left: [%d]\n", GET_OBJ_VAL(obj, 2));
			break;
		case ITEM_SCROLL:
		case ITEM_POTION:
			sprintf(buf+strlen(buf), "Spells: (Level %d) %s, %s, %s\n", GET_OBJ_VAL(obj, 0),
			skill_name(GET_OBJ_VAL(obj, 1)), skill_name(GET_OBJ_VAL(obj, 2)),
			skill_name(GET_OBJ_VAL(obj, 3)));
			break;
		case ITEM_WAND:
		case ITEM_STAFF:
			sprintf(buf+strlen(buf), "Spell: %s at level %d, %d (of %d) charges remaining\n",
			skill_name(GET_OBJ_VAL(obj, 3)), GET_OBJ_VAL(obj, 0),
			GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
			break;
		case ITEM_WEAPON:
			sprintf(buf+strlen(buf), "Ave. Dam: %d, Message type: %d\n",
			get_weapon_dam(obj), GET_OBJ_VAL(obj, 3));
			break;
		case ITEM_ARMOR:
			sprintf(buf+strlen(buf), "Passive Defense: [%d]\n", GET_OBJ_VAL(obj, 0));
			sprintf(buf+strlen(buf), "Damage Reduction: [%d]\n", GET_OBJ_VAL(obj, 1));
			break;
		case ITEM_TRAP:
			sprintf(buf+strlen(buf), "Spell: %d, - Hitpoints: %d\n",
			GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1));
			break;
		case ITEM_CONTAINER:
			sprintbit(GET_OBJ_VAL(obj, 1), container_bits, buf2, sizeof(buf2));
			sprintf(buf+strlen(buf), "Weight capacity: %d, Lock Type: %s, Key Num: %d ",
			GET_OBJ_VAL(obj, 0), buf2, GET_OBJ_VAL(obj, 2)); 
			if (GET_OBJ_VAL(obj, 2) > 0) {
				key=read_object(GET_OBJ_VAL(obj,2), VIRTUAL);
				if (key) {
					sprintf(buf+strlen(buf), "(%s)", GET_OBJ_NAME(key));
					extract_obj(key); 
				}
				else
					sprintf(buf+strlen(buf), "(Error: Key does not exist!)");
			}
			sprintf(buf+strlen(buf), "\n");
			break;
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:
			sprinttype(GET_OBJ_VAL(obj, 2), drinks, buf2, sizeof(buf2));
			sprintf(buf+strlen(buf), "Capacity: %d, Contains: %d, Poisoned: %s, Liquid: %s\n",
			GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), YESNO(GET_OBJ_VAL(obj, 3)),buf2);
			break; 
		case ITEM_FOOD:
			sprintf(buf+strlen(buf), "Makes full: %d, Poisoned: %s\n", 
			GET_OBJ_VAL(obj, 0),YESNO(GET_OBJ_VAL(obj, 3)));
			break;
		case ITEM_MONEY:
			sprintf(buf, "Coins: %d\n", GET_OBJ_VAL(obj, 0));
			break;
		case ITEM_PORTAL:
			sprintf(buf, "To room: %d\n", GET_OBJ_VAL(obj, 0));
			break;
		default:
			sprintf(buf+strlen(buf), "Values 0-3: [%d] [%d] [%d] [%d]\n",
			GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1),
			GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3));
			break;
		}

		found = 0;
		sprintf(buf+strlen(buf), "Affects on player stats:\n");
		for (j = 0; j < MAX_OBJ_AFFECT; j++)
			if (obj->affected[j].modifier) {
				sprinttype(obj->affected[j].location, apply_types, buf2, sizeof(buf2));
				sprintf(buf+strlen(buf), "    %+d to %s\n",obj->affected[j].modifier, buf2); 
				found=1;
			}
		if (!found)
			sprintf(buf+strlen(buf),"    None\n"); 
		sprintf(buf+strlen(buf), "\n--------\n");
		extract_obj(obj);
		fprintf(reportfile, buf);
		buf[0]='\0';
	}/*for i=0...*/
	fclose (reportfile);
	send_to_char ("report.obj printed\r\n",ch);
}


/**********************************************************************************/
/****************************		 Lists   ******************************************/
/**********************************************************************************/

void do_list_report (struct char_data *ch)
{
	FILE *reportfile; 
	int i=0, cols=0; 

	if (!(reportfile = fopen("report.lst", "w"))) {
		mlog("SYSERR:  Miscellaneous report file unavailable.");
		send_to_char ("Report.lst could not be generated.\r\n",ch);
		return;
	}
	/*Add Quest stuff later*/
	sprintf(buf, "Mob and Player lists\n--------------------\n\n");
	strcat (buf, "Affect Flags\n");
	for (i=0; i<NUM_AFF_FLAGS; i++) {
		sprintf(buf+strlen(buf), "[%c] %2d) %-15s", i+'a', i, affected_bits[i]);
		cols++;
		if (cols%3==0)
			strcat(buf,"\n");
	}
	cols=0;
	strcat (buf, "\n\nMob Specs\n");
	for (i=0; i<NUM_MOB_SPECS; i++) {
		sprintf(buf+strlen(buf), "[%c] %2d) %-15s", i+'a', i, mob_specproc_info[i].name);
		cols++;
		if (cols%3==0)
			strcat(buf,"\n");
	}
	cols=0;
	strcat (buf, "\n\nCharacter Applies (for Items)\n");
	for (i=0; i<NUM_APPLIES; i++) {
		sprintf(buf+strlen(buf), "[%c] %2d) %-15s", i+'a', i, apply_types[i]);
		cols++;
		if (cols%3==0)
			strcat(buf,"\n");
	}
	cols=0;
	strcat (buf, "\n\nMob Flags\n");
	for (i=0; i<NUM_MOB_FLAGS; i++) {
		sprintf(buf+strlen(buf), "[%c] %2d) %-15s", i+'a', i, action_bits[i]);
		cols++;
		if (cols%3==0)
			strcat(buf,"\n");
	}
	/*
		cols=0;
		strcat (buf, "\n\nNPC Classes \n");
		for (i=0; i<NUM_NPC_CLASSES; i++)
		 {
			 sprintf(buf+strlen(buf), "[%c] %2d) %-15s", i+'a', i, npc_class_types[i]);
			 cols++;
			 if (cols%3==0) strcat(buf,"\n");
		 }
		*/
	cols=0;

	sprintf(buf+strlen(buf), "\nObj lists\n---------\n\n");
	strcat (buf, "Item Flags\n");
	for (i=0; i<NUM_ITEM_FLAGS; i++) {
		sprintf(buf+strlen(buf), "[%c] %2d) %-15s", i+'a', i, extra_bits[i]);
		cols++;
		if (cols%3==0)
			strcat(buf,"\n");
	}
	cols=0;
	strcat (buf, "\n\nObject Spec Procs\n");
	for (i=0; i<NUM_OBJ_SPECS; i++) {
		sprintf(buf+strlen(buf), "[%c] %2d) %-15s", i+'a', i, obj_specproc_info[i].name);
		cols++;
		if (cols%3==0)
			strcat(buf,"\n");
	}
	cols=0;

	sprintf(buf+strlen(buf), "\n\nRoom lists\n----------\n\n");
	strcat (buf, "Room Flags\n");
	for (i=0; i<NUM_ROOM_FLAGS; i++)
	{
		sprintf(buf+strlen(buf), "[%c] %2d) %-15s", i+'a', i, room_bits[i]);
		cols++;
		if (cols%3==0)
			strcat(buf,"\n");
	}
	cols=0;
	strcat (buf, "\n\nRoom Specs\n");
	for (i=0; i<NUM_ROOM_SPECS; i++) {
		sprintf(buf+strlen(buf), "[%c] %2d) %-15s", i+'a', i, room_specproc_info[i].name);
		cols++;
		if (cols%3==0)
				strcat(buf,"\n");
	}
	cols=0;
	strcat (buf, "\n\nRoom Sectors\n");
	for (i=0; i<NUM_ROOM_SECTORS; i++) {
		sprintf(buf+strlen(buf), "[%c] %2d) %-15s", i+'a', i, sector_types[i]);
		cols++;
		if (cols%3==0)
			strcat(buf,"\n");
	}
	cols=0;

	sprintf(buf+strlen(buf), "\n\nZone lists\n----------\n\n");
	strcat (buf, "Zone Flags\n");
	for (i=0; i<NUM_ZONE_FLAGS; i++) {
		sprintf(buf+strlen(buf), "[%c] %2d) %-15s", i+'a', i, zone_bits[i]);
		cols++;
		if (cols%3==0)
			strcat(buf,"\n");
	}
	cols=0;
	fprintf(reportfile, buf);
	fclose(reportfile);
	send_to_char ("report.lst printed\r\n",ch);
}


void do_zone_report(struct char_data *ch, char *arg)
{
	int i =0;

	skip_spaces(&arg);

	if (!*arg) {
		send_to_char ("Usage:  Report #\r\n",ch);
		send_to_char ("1) Mobs  2) Objs  3) Flag Lists  4) All\r\n",ch);
		return;
	}

	if (!isdigit(*arg)) {
		send_to_char ("Usage:  Report #\r\n",ch);
		send_to_char ("1) Mobs  2) Objs  3) Flag Lists  4) All\r\n",ch);
		return;
	}
	i=atoi(arg);

	switch (i) {
	case 1:
		do_mob_report(ch); 
		break;
	case 2:
		do_obj_report(ch); 
		break;
	case 3:
		do_list_report(ch); 
		break;
	case 4:
		do_mob_report(ch);
		do_obj_report(ch);
		do_list_report(ch);
		break;
	default:
		send_to_char ("Usage:  Report #",ch);
		send_to_char ("1) Mobs  2) Objs  3) Flag Lists  4) All",ch);
	}/*switch i*/
}
