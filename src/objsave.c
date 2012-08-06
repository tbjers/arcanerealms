/* ************************************************************************
*   File: objsave.c                                    Part of CircleMUD  *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.         See license.doc for complete information. *
*                                                                         *
*  This file altered by Anton Graham <darkimage@programmer.net> using     *
*  code from Patrick Dughi's Xapobjs patch, kudos to these guys!          *
*                                                                         *
*  Former functions for rent save moved into Crash_datasave, coded by     *
*  Torgny Bjers, <artovil@arcanerealms.org>, 2002-07-09                   *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001, Torgny Bjers.                                       *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************* */
/* $Id: objsave.c,v 1.38 2004/04/20 16:30:13 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "specset.h"
#include "diskio.h"           /* Provided by the ascii pfiles patch */

/* these factors should be unique integers */
#define	RENT_FACTOR    1
#define	CRYO_FACTOR    4

#define	LOC_INVENTORY  0
#define	MAX_BAG_ROWS   5

/* internal variables */
int obj_order_num = 0;

/* external variables */
extern struct room_data *world;
extern struct obj_data *obj_proto;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int rent_file_timeout, crash_file_timeout;
extern int free_rent;
extern int min_rent_cost;
extern int max_obj_save;  /* change in config.c */
extern obj_rnum top_of_objt;
extern mob_rnum top_of_mobt;

/* External functions */
ACMD(do_action);
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
int	invalid_class(struct char_data *ch, struct obj_data *obj);
bitvector_t	asciiflag_conv(char *flag);
int	sprintascii(char *out, bitvector_t bits);
int handle_obj(struct obj_data *obj, struct char_data *ch, int locate, struct obj_data **cont_rows);
int	check_object(struct obj_data *obj);

/* local functions */
void Crash_extract_norent_eq(struct char_data *ch);
void auto_equip(struct char_data *ch, struct obj_data *obj, int location);
int	Crash_offer_rent(struct char_data *ch, struct char_data *receptionist, int display, int factor);
int	Crash_report_unrentables(struct char_data *ch, struct char_data *recep, struct obj_data *obj);
void Crash_report_rent(struct char_data *ch, struct char_data *recep, struct obj_data *obj, long *cost, long *nitems, int display, int factor);
void update_obj_file(void);
int	gen_receptionist(struct char_data *ch, struct char_data *recep, int cmd, char *arg, int mode);
int	Crash_save(struct obj_data *obj, int id, int location);
int	Crash_write_rentcode(struct char_data *ch, struct rent_info rent);
int	Obj_to_store(struct obj_data *obj, int id, int location, int type);
void Crash_rent_deadline(struct char_data *ch, struct char_data *recep, long cost);
void Crash_restore_weight(struct obj_data *obj);
void Crash_extract_objs(struct obj_data *obj);
int	Crash_is_unrentable(struct obj_data *obj);
void Crash_extract_norents(struct obj_data *obj);
void Crash_extract_expensive(struct obj_data *obj);
void Crash_calculate_rent(struct obj_data *obj, int *cost);
int	Crash_load_objs(struct char_data *ch);

/*
 * Utility functions
 */

/*
 * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>
 */
void auto_equip(struct char_data *ch, struct obj_data *obj, int location)
{
	int j;

	/* Lots of checks... */
	if (location > 0) {  /* Was wearing it. */
		switch (j = (location - 1)) {
		case WEAR_LIGHT:
			break;
		case WEAR_FINGER_R:
		case WEAR_FINGER_L:
			if (!CAN_WEAR(obj, ITEM_WEAR_FINGER)) /* not fitting :( */
				location = LOC_INVENTORY;
			break;
		case WEAR_NECK:
			if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
				location = LOC_INVENTORY;
			break;
		case WEAR_BODY:
			if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
				location = LOC_INVENTORY;
			break;
		case WEAR_HEAD:
			if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
				location = LOC_INVENTORY;
			break;
		case WEAR_LEGS:
			if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
				location = LOC_INVENTORY;
			break;
		case WEAR_FEET:
			if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
				location = LOC_INVENTORY;
			break;
		case WEAR_HANDS:
			if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
				location = LOC_INVENTORY;
			break;
		case WEAR_ARMS:
			if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
				location = LOC_INVENTORY;
			break;
		case WEAR_SHIELD:
			if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
				location = LOC_INVENTORY;
			break;
		case WEAR_ABOUT:
			if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
				location = LOC_INVENTORY;
			break;
		case WEAR_WAIST:
			if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
				location = LOC_INVENTORY;
			break;
		case WEAR_WRIST_R:
		case WEAR_WRIST_L:
			if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
				location = LOC_INVENTORY;
			break;
		case WEAR_WIELD:
			if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
				location = LOC_INVENTORY;
			break;
		case WEAR_HOLD:
			if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
				break;
			if (IS_WARRIOR(ch) && CAN_WEAR(obj, ITEM_WEAR_WIELD) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
				break;
			location = LOC_INVENTORY;
			break;
		case WEAR_DWIELD:
			if (!CAN_WEAR(obj, ITEM_WEAR_DWIELD))
				location = LOC_INVENTORY;
			break;
		case WEAR_FACE:
			if (!CAN_WEAR(obj, ITEM_WEAR_FACE))
				location = LOC_INVENTORY;
			break;
		case WEAR_FLOAT:
			if (!CAN_WEAR(obj, ITEM_WEAR_FLOAT))
				location = LOC_INVENTORY;
			break;
		case WEAR_BACK:
			if (!CAN_WEAR(obj, ITEM_WEAR_BACK))
				location = LOC_INVENTORY;
			break;
		case WEAR_BELT_1:
		case WEAR_BELT_2:
			if (!CAN_WEAR(obj, ITEM_WEAR_BELT))
				location = LOC_INVENTORY;
			break;
		case WEAR_OUTSIDE:
			if (!CAN_WEAR(obj, ITEM_WEAR_OUTSIDE))
				location = LOC_INVENTORY;
			break;
		case WEAR_THROAT_1:
		case WEAR_THROAT_2:
			if (!CAN_WEAR(obj, ITEM_WEAR_THROAT))
				location = LOC_INVENTORY;
			break;
		case WEAR_WINGS:
			if (!CAN_WEAR(obj, ITEM_WEAR_WINGS))
				location = LOC_INVENTORY;
			break;
		case WEAR_HORNS:
			if (!CAN_WEAR(obj, ITEM_WEAR_HORNS))
				location = LOC_INVENTORY;
			break;
		case WEAR_TAIL:
			if (!CAN_WEAR(obj, ITEM_WEAR_TAIL))
				location = LOC_INVENTORY;
			break;
		default:
			location = LOC_INVENTORY;
		}

		if (location > 0) {      /* Wearable. */
			if (!GET_EQ(ch,j)) {
				/*
				 * Check the characters's alignment to prevent them from being
				 * zapped through the auto-equipping.
				 */
				if (invalid_align(ch, obj) || !HAS_BODY(ch, (location - 1)) || invalid_size(ch, obj))
					location = LOC_INVENTORY;
				else
					equip_char(ch, obj, j);
			} else {  /* Oops, saved a player with double equipment? */
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch), location);
				location = LOC_INVENTORY;
			}
		}
	}
	if (location <= 0)  /* Inventory */
		obj_to_char(obj, ch);
}


void update_obj_file(void)
{
	int i;

	for (i = 0; i <= top_of_p_table; i++)
		if (*player_table[i].name)
			Crash_clean_file(player_table[i].id);
}


void Crash_restore_weight(struct obj_data *obj)
{
	if (obj) {
		Crash_restore_weight(obj->contains);
		Crash_restore_weight(obj->next_content);
		if (obj->in_obj)
			GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
	}
}


/*
 * Get !RENT items from equipment to inventory and
 * extract !RENT out of worn containers.
 */
void Crash_extract_norent_eq(struct char_data *ch)
{
	int j;

	for (j = 0; j < NUM_WEARS; j++) {
		if (GET_EQ(ch, j) == NULL)
			continue;
			 
		if (Crash_is_unrentable(GET_EQ(ch, j)))
			obj_to_char(unequip_char(ch, j), ch);
		else
			Crash_extract_norents(GET_EQ(ch, j));
	}
}


void Crash_extract_objs(struct obj_data *obj)
{
	if (obj) {
		Crash_extract_objs(obj->contains);
		Crash_extract_objs(obj->next_content);
		extract_obj(obj);
	}
}


int	Crash_is_unrentable(struct obj_data *obj)
{
	if (!obj)
		return (0);

	if (OBJ_FLAGGED(obj, ITEM_NORENT) ||
			GET_OBJ_RENT(obj) < 0 ||
			(GET_OBJ_RNUM(obj) <= NOTHING &&
			!IS_SET(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE))) {
		return (1);
	}

	return (0);
}


void Crash_extract_norents(struct obj_data *obj)
{
	if (obj) {
		Crash_extract_norents(obj->contains);
		Crash_extract_norents(obj->next_content);
		if (Crash_is_unrentable(obj))
			extract_obj(obj);
	}
}


void Crash_extract_expensive(struct obj_data *obj)
{
	struct obj_data *tobj, *max;

	max = obj;
	for (tobj = obj; tobj; tobj = tobj->next_content)
		if (GET_OBJ_RENT(tobj) > GET_OBJ_RENT(max))
			max = tobj;
	extract_obj(max);
}


void Crash_calculate_rent(struct obj_data *obj, int *cost)
{
	if (obj) {
		*cost += MAX(0, GET_OBJ_RENT(obj));
		Crash_calculate_rent(obj->contains, cost);
		Crash_calculate_rent(obj->next_content, cost);
	}
}


int	handle_obj(struct obj_data *temp, struct char_data *ch, int locate, struct obj_data **cont_row)
{
	int j;
	struct obj_data *obj1;

	if (!temp)  /* this should never happen, but.... */
		return (0);

	auto_equip(ch, temp, locate);

	/* 
		 what to do with a new loaded item:

		 if there's a list with <locate> less than 1 below this:
		 (equipped items are assumed to have <locate>==0 here) then its
		 container has disappeared from the file   *gasp*
		 -> put all the list back to ch's inventory
		 if there's a list of contents with <locate> 1 below this:
		 check if it's a container
		 - if so: get it from ch, fill it, and give it back to ch (this way the
		 container has its correct weight before modifying ch)
		 - if not: the container is missing -> put all the list to ch's inventory

		 for items with negative <locate>:
		 if there's already a list of contents with the same <locate> put obj to it
		 if not, start a new list

		 Confused? Well maybe you can think of some better text to be put here ...

		 since <locate> for contents is < 0 the list indices are switched to
		 non-negative
	*/

	if (locate > 0) { /* item equipped */

		for (j = MAX_BAG_ROWS-1;j > 0;j--)
			if (cont_row[j]) { /* no container -> back to ch's inventory */
				for (;cont_row[j];cont_row[j] = obj1) {
					obj1 = cont_row[j]->next_content;
					obj_to_char(cont_row[j], ch);
				}
				cont_row[j] = NULL;
			}
		if (cont_row[0]) { /* content list existing */
			if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER || GET_OBJ_TYPE(temp) == ITEM_SHEATH) {
				/* rem item ; fill ; equip again */
				temp = unequip_char(ch, locate-1);
				temp->contains = NULL; /* should be empty - but who knows */
				for (;cont_row[0];cont_row[0] = obj1) {
					obj1 = cont_row[0]->next_content;
					obj_to_obj(cont_row[0], temp);
				}
				equip_char(ch, temp, locate-1);
			} else { /* object isn't container -> empty content list */
				for (;cont_row[0];cont_row[0] = obj1) {
					obj1 = cont_row[0]->next_content;
					obj_to_char(cont_row[0], ch);
				}
				cont_row[0] = NULL;
			}
		}
	} else { /* locate <= 0 */
		for (j = MAX_BAG_ROWS-1;j > -locate;j--)
			if (cont_row[j]) { /* no container -> back to ch's inventory */
				for (;cont_row[j];cont_row[j] = obj1) {
					obj1 = cont_row[j]->next_content;
					obj_to_char(cont_row[j], ch);
				} 
				cont_row[j] = NULL;
			}

		if (j == -locate && cont_row[j]) { /* content list existing */
			if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER || GET_OBJ_TYPE(temp) == ITEM_SHEATH) {
				/* take item ; fill ; give to char again */
				obj_from_char(temp);
				temp->contains = NULL;
				for (;cont_row[j];cont_row[j] = obj1) {
					obj1 = cont_row[j]->next_content;
					obj_to_obj(cont_row[j], temp);
				}
				obj_to_char(temp, ch); /* add to inv first ... */
			} else { /* object isn't container -> empty content list */
				for (;cont_row[j];cont_row[j] = obj1) {
					obj1 = cont_row[j]->next_content;
					obj_to_char(cont_row[j], ch);
				}
				cont_row[j] = NULL;
			}
		}

		if (locate < 0 && locate >= -MAX_BAG_ROWS) {
			/* let obj be part of content list
				 but put it at the list's end thus having the items
				 in the same order as before renting */
			obj_from_char(temp);
			if ((obj1 = cont_row[-locate-1])) {
				while (obj1->next_content)
					obj1 = obj1->next_content;
				obj1->next_content = temp;
			} else
				cont_row[-locate-1] = temp;
		}
	} /* locate less than zero */

	return (1);
}


/*
 * Crash loading functions
 */

int Crash_load_rentcode(long id, int *rentcode, int *timed, int *netcost, int *gold, int *account, int *nitems)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int rec_count;

	if (!(result = mysqlGetResource(TABLE_RENT_PLAYERS, "SELECT RentCode, Timed, NetCost, Gold, Account, NumItems FROM %s WHERE PlayerID = %ld;", TABLE_RENT_PLAYERS, id))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "(%s)%s:%d Could not access %s.", __FILE__, __FUNCTION__, __LINE__, TABLE_RENT_PLAYERS);
		return (0);
	}

	rec_count = mysql_num_rows(result);

	if (rec_count == 1) {
		row = mysql_fetch_row(result);
		*rentcode = atoi(row[0]);
		*timed = atoi(row[1]);
		*netcost = atoi(row[2]);
		*gold = atoi(row[3]);
		*account = atoi(row[4]);
		*nitems = atoi(row[5]);
		mysql_free_result(result);
		return (1);
	}

	mysql_free_result(result);
	return (0);
}


int	Crash_load_objs(struct char_data *ch) {
	MYSQL_RES *result, *result2;
	MYSQL_ROW row, row2;
	unsigned long *fieldlength;
	int i, vnum = 0, proto_number = 0, num_of_days;
	int locate = 0, cost, num_objs = 0, num_affects = 0;
	struct obj_data *cont_row[MAX_BAG_ROWS];
	int rentcode = 0, timed = 0, netcost = 0, gold = 0, account = 0, nitems = 0;
	struct obj_data *temp = NULL;

	for (i = 0; i < MAX_BAG_ROWS; i++)
		cont_row[i] = NULL;

	if (!Crash_load_rentcode(GET_IDNUM(ch), &rentcode, &timed, &netcost, &gold, &account, &nitems))
		return (0);

	if (rentcode == RENT_RENTED || rentcode == RENT_TIMEDOUT) {
		char str[64];
		sprintf(str, "%d", SECS_PER_REAL_DAY);
		num_of_days = (int)((float) (time(0) - timed) / atoi(str));
		cost = (int) (netcost *num_of_days);
		if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
			extended_mudlog(BRF, SYSL_LOGINS, TRUE, "%s has joined, rented equipment lost (no $).", GET_NAME(ch));
			crash_datasave(ch, 0, RENT_CRASH);
			return (2);
		} else {
			GET_BANK_GOLD(ch) -= MAX(cost - GET_GOLD(ch), 0);
			GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
			save_char(ch, NOWHERE, FALSE);
		}
	}

	switch (rentcode) {
	case RENT_RENTED:
		extended_mudlog(BRF, SYSL_RENT, TRUE, "%s@%s un-renting and joins game.", GET_NAME(ch), ch->desc->host);
		break;
	case RENT_CRASH:
		extended_mudlog(BRF, SYSL_RENT, TRUE, "%s@%s retrieving crash-saved items and joins game.", GET_NAME(ch), ch->desc->host);
		break;
	case RENT_CRYO:
		extended_mudlog(BRF, SYSL_RENT, TRUE, "%s@%s un-cryo'ing and joins game.", GET_NAME(ch), ch->desc->host);
		break;
	case RENT_FORCED:
	case RENT_TIMEDOUT:
		extended_mudlog(BRF, SYSL_RENT, TRUE, "%s@%s retrieving force-saved items and joins game.", GET_NAME(ch), ch->desc->host);
		break;
	default:
		extended_mudlog(BRF, SYSL_RENT, TRUE, "%s@%s has joined with undefined rent code.", GET_NAME(ch), ch->desc->host);
		break;
	}

	if (!(result = mysqlGetResource(TABLE_RENT_OBJECTS, "SELECT * FROM %s WHERE player_id = %ld ORDER BY obj_order ASC;", TABLE_RENT_OBJECTS, GET_IDNUM(ch)))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "(%s)%s:%d Could not load rent files from table [%s].", __FILE__, __FUNCTION__, __LINE__, TABLE_RENT_OBJECTS);
		return (0);
	}

	num_objs = mysql_num_rows(result);

	if (!num_objs) {
		extended_mudlog(BRF, SYSL_LOGINS, TRUE, "%s@%s has joined with no equipment.", GET_NAME(ch), ch->desc->host);
		mysql_free_result(result);
		return (1);
	}

	if (num_objs > 0) {
		while ((row = mysql_fetch_row(result)))
		{
			fieldlength = mysql_fetch_lengths(result);
			temp = NULL;

			locate = atoi(row[1]);
			vnum = atoi(row[2]);
			proto_number = atoi(row[21]);

			if (vnum == NOTHING) {

				temp = create_obj();
				temp->item_number = vnum;
				temp->proto_number = proto_number;

				temp->unique_id = atoi(row[3]);
				temp->name = str_dup(row[4]);
				temp->short_description = str_dup(row[5]);
				temp->description = str_dup(row[6]);
				temp->action_description = str_dup(row[7]);

				/* Object flags checked in check_object(). */
				GET_OBJ_TYPE(temp) = atoi(row[8]);
				GET_OBJ_EXTRA(temp) = asciiflag_conv(row[9]);
				GET_OBJ_WEAR(temp) = asciiflag_conv(row[10]);
				temp->obj_flags.bitvector = atoi(row[11]);

				GET_OBJ_VAL(temp, 0) = atoi(row[12]);
				GET_OBJ_VAL(temp, 1) = atoi(row[13]);
				GET_OBJ_VAL(temp, 2) = atoi(row[14]);
				GET_OBJ_VAL(temp, 3) = atoi(row[15]);

				GET_OBJ_WEIGHT(temp) = atoi(row[16]);
				GET_OBJ_COST(temp) = atoi(row[17]);
				GET_OBJ_RENT(temp) = atoi(row[18]);

				GET_OBJ_SIZE(temp) = atoi(row[19]);
				GET_OBJ_COLOR(temp) = atoi(row[20]);
				GET_OBJ_RESOURCE(temp) = atoi(row[22]);
				
				/* Get the object affects */
				if (!(result2 = mysqlGetResource(TABLE_RENT_AFFECTS, "SELECT * FROM %s WHERE player_id = %ld and unique_id = %ld;", TABLE_RENT_AFFECTS, GET_IDNUM(ch), temp->unique_id))) {
					extended_mudlog(BRF, SYSL_SQL, TRUE, "(%s)%s:%d Could not load rent files from table [%s].", __FILE__, __FUNCTION__, __LINE__, TABLE_RENT_AFFECTS);
					return (0);
				}
				num_affects = mysql_num_rows(result2);

				for (i = 0; i < MAX_OBJ_AFFECT; i++) {
					if (i < num_affects && (row2 = mysql_fetch_row(result2))) {
						temp->affected[i].location = atoi(row2[1]);
						temp->affected[i].modifier = atoi(row2[2]);
					} else {
						temp->affected[i].location = APPLY_NONE;
						temp->affected[i].modifier = 0;
					}
				}
				mysql_free_result(result2);

				/* Get the object extradescs */


			} else {

				if (real_object(vnum) != NOTHING) {
					temp = read_object(vnum, VIRTUAL);
					GET_OBJ_VAL(temp, 0) = atoi(row[12]);
					GET_OBJ_VAL(temp, 1) = atoi(row[13]);
					GET_OBJ_VAL(temp, 2) = atoi(row[14]);
					GET_OBJ_VAL(temp, 3) = atoi(row[15]);
					GET_OBJ_COLOR(temp) = atoi(row[20]);
					GET_OBJ_RESOURCE(temp) = atoi(row[22]);
				} else {
					extended_mudlog(BRF, SYSL_RENT, TRUE, "Nonexistent object %d found in rent file %s.", vnum, GET_NAME(ch));
				}

			}

			handle_obj(temp, ch, locate, cont_row);

		}
	}

	/* Little hoarding check. -gg 3/1/98 */
	extended_mudlog(NRM, SYSL_LOGINS, TRUE, "%s@%s has joined with %d objects (max %d).", GET_NAME(ch), ch->desc->host, num_objs, max_obj_save);

	mysql_free_result(result);

	if ((rentcode == RENT_RENTED) || (rentcode == RENT_CRYO))
		return (0);
	else
		return (1);
}


void Crash_listrent(struct char_data *ch, char *name)
{
	send_to_char("This function has not been converted to MySQL yet.\r\n", ch);
}


/*
 * Return values:
 *  0 - successful load, keep char in rent room.
 *  1 - load failure or load of crash items -- put char in temple.
 *  2 - rented equipment lost (no $)
 */
int	Crash_load(struct char_data *ch)
{
	return (Crash_load_objs(ch));
}


/*
 * Crash deletion functions
 */

int	Crash_delete_file(int id)
{
	/* Delete rent items from the database. */
	if (!(mysqlWrite("DELETE FROM %s WHERE PlayerID = %ld;", TABLE_RENT_PLAYERS, id))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error removing rent data for player id #%d.", id);
		return (0);
	}
	/* Delete all the player's objects from the database. */
	if (!(mysqlWrite("DELETE FROM %s WHERE player_id = %ld;", TABLE_RENT_OBJECTS, id))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error removing rent objects for player id #%d.", id);
		return (0);
	}
	if (!(mysqlWrite("DELETE FROM %s WHERE player_id = %ld;", TABLE_RENT_AFFECTS, id))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error removing rent objects affects for player id #%d.", id);
		return (0);
	}
	if (!(mysqlWrite("DELETE FROM %s WHERE player_id = %ld;", TABLE_RENT_EXTRADESCS, id))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error removing rent objects extradescs for player id #%d.", id);
		return (0);
	}
	return (1);
}


int	Crash_delete_crashfile(struct char_data *ch)
{
	MYSQL_RES *result;

	if (!(result = mysqlGetResource(TABLE_RENT_PLAYERS, "SELECT PlayerID FROM %s WHERE PlayerID = %ld AND RentCode = %d;", TABLE_RENT_PLAYERS, GET_IDNUM(ch), RENT_CRASH))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "(%s)%s:%d Could not access %s.", __FILE__, __FUNCTION__, __LINE__, TABLE_RENT_PLAYERS);
		return (0);
	}
	
	if (mysql_num_rows(result) == 1)
		Crash_delete_file(GET_IDNUM(ch));

	mysql_free_result(result);

	return (1);
}


int	Crash_clean_file(int id)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int rentcode, timed;

	if (!(result = mysqlGetResource(TABLE_RENT_PLAYERS, "SELECT RentCode, Timed FROM %s WHERE PlayerID = %ld;", TABLE_RENT_PLAYERS, id))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "(%s)%s:%d Could not access %s.", __FILE__, __FUNCTION__, __LINE__, TABLE_RENT_PLAYERS);
		return (0);
	}

	if (mysql_num_rows(result) != 1) {
		mysql_free_result(result);
		return (0);
	} else {
		row = mysql_fetch_row(result);
		rentcode = atoi(row[0]);
		timed = atoi(row[1]);
		mysql_free_result(result);
		if ((rentcode == RENT_CRASH) ||
				(rentcode == RENT_FORCED) || (rentcode == RENT_TIMEDOUT)) {
			if (timed < time(0) - (crash_file_timeout * SECS_PER_REAL_DAY)) {
				char *rent_type = get_buffer(256);
				Crash_delete_file(id);
				switch (rentcode) {
				case RENT_CRASH:
					strcpy(rent_type, "crash");
					break;
				case RENT_FORCED:
					strcpy(rent_type, "forced rent");
					break;
				case RENT_TIMEDOUT:
					strcpy(rent_type, "idlesave");
					break;
				default:
					strcpy(rent_type, "UNKNOWN!");
					break;
				}
				extended_mudlog(BRF, SYSL_RENT, TRUE, "Deleting %s data for player id #%d.", rent_type, id);
				release_buffer(rent_type);
				return (1);
			}
			/* Must retrieve rented items w/in 30 days */
		} else if (rentcode == RENT_RENTED) {
			if (timed < time(0) - (rent_file_timeout * SECS_PER_REAL_DAY)) {
				Crash_delete_file(id);
				extended_mudlog(BRF, SYSL_RENT, TRUE, "Deleting rent data for player id #%d.", id);
				return (1);
			}
		}
	}
	
	return (0);
}


/*
 * Crash saving functions
 */

int	Obj_to_store(struct obj_data *obj, int id, int locate, int type)
{
	char *description=NULL;
	char *name=NULL;
	char *short_description=NULL;
	char *action_description=NULL;
	char *keyword=NULL;
	char *obj_table=NULL;
	char *aff_table=NULL;
	char *desc_table=NULL;
	int counter2 = 0;
	struct extra_descr_data *ex_desc;
	char *exdescs_insert = "REPLACE INTO "
		"%s ("
			"unique_id, "
			"keyword, "
			"description, "
			"%s_id"
		") VALUES ("
			"%ld, "
			"'%s', "
			"'%s', "
			"%ld"
		");";
	char *affects_insert = "REPLACE INTO "
		"%s ("
			"unique_id, "
			"location, "
			"modifier, "
			"%s_id"
		") VALUES ("
			"%ld, "
			"%d, "
			"%d, "
			"%ld"
		");";
	char *unique_insert = "INSERT INTO "
		"%s ("									// Table Name
			"%s_id, "							// 1 (player_id / house_id)
			"location, "
			"vnum, "
			"obj_order, "
			"name, "							// 5
			"short_description, "
			"description, "
			"action_description, "
			"type, "
			"extra, "							// 10
			"wear, "
			"perm, "
			"val0, "
			"val1, "
			"val2, "							// 15
			"val3, "
			"weight, "
			"cost, "
			"rent, "
			"size, "							// 20
			"color, "
			"proto_vnum, "
			"resource"
		") VALUES ("
			"%ld, "								// 1
			"%d, "
			"%d, "
			"%d, "
			"'%s', "							// 5
			"'%s', "
			"'%s', "
			"'%s', "
			"%d, "
			"%llu, "							// 10
			"%llu, "
			"%llu, "
			"%d, "
			"%d, "
			"%d, "								// 15
			"%d, "
			"%d, "
			"%d, "
			"%d, "
			"%d, "								// 20
			"%d, "
			"%d, "
			"%d"
		");";
	char *prototype_insert = "INSERT INTO "
		"%s ("									// Table Name
			"%s_id, "							// 1 (player_id / house_id)
			"location, "
			"vnum, "
			"obj_order, "
			"val0, "							// 5
			"val1, "
			"val2, "
			"val3, "
			"color, "
			"resource"						// 10
		") VALUES ("
			"%ld, "								// 1
			"%d, "
			"%d, "
			"%d, "
			"%d, "								// 5
			"%d, "
			"%d, "
			"%d, "
			"%d, "
			"%d"									// 10
		");";

	/*
	 * Type can be either CRASH_PLAYER or CRASH_HOUSE.
	 * We are using different tables for houses and
	 * players, simpler to distinguish between the two
	 * that way.
	 * Torgny Bjers <artovil@arcanerealms.org>, 2002-12-21
	 */
	switch (type) {
	case CRASH_PLAYER:
		obj_table = TABLE_RENT_OBJECTS;
		aff_table = TABLE_RENT_AFFECTS;
		desc_table = TABLE_RENT_EXTRADESCS;
		break;
	case CRASH_HOUSE:
		obj_table = TABLE_HOUSE_RENT_OBJECTS;
		aff_table = TABLE_HOUSE_RENT_AFFECTS;
		desc_table = TABLE_HOUSE_RENT_EXTRADESCS;
		break;
	}

	obj_order_num++;

	if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE)) {

		/* We're dealing with a unique object */

		if (obj->action_description) {
			buf1[MAX_STRING_LENGTH - 1] = '\0';
			strncpy(buf1, obj->action_description, MAX_STRING_LENGTH - 1);
			SQL_MALLOC(buf1, action_description);
			SQL_ESC(buf1, action_description);
		} else
			SQL_MALLOC("", action_description);

		strcpy(buf, (obj->name && *obj->name) ? obj->name : "undefined");
		SQL_MALLOC(buf, name);
		SQL_ESC(buf, name);

		strcpy(buf, (obj->short_description && *obj->short_description) ? obj->short_description : "undefined");
		SQL_MALLOC(buf, short_description);
		SQL_ESC(buf, short_description);

		strcpy(buf, (obj->description && *obj->description) ?	obj->description : "undefined");
		SQL_MALLOC(buf, description);
		SQL_ESC(buf, description);

		if (!(mysqlWrite(
			unique_insert,
			obj_table,						// Table Name
			(type == CRASH_PLAYER ? "player" : "room"),
			id,										// 1
			locate,
			NOTHING,
			obj_order_num,
			name,
			short_description,		// 5
			description,
			action_description,
			GET_OBJ_TYPE(obj),
			GET_OBJ_EXTRA(obj),
			GET_OBJ_WEAR(obj),		// 10
			GET_OBJ_PERM(obj),
			GET_OBJ_VAL(obj, 0),
			GET_OBJ_VAL(obj, 1),
			GET_OBJ_VAL(obj, 2),
			GET_OBJ_VAL(obj, 3),	// 15
			GET_OBJ_WEIGHT(obj),
			GET_OBJ_COST(obj),
			GET_OBJ_RENT(obj),
			GET_OBJ_SIZE(obj),
			GET_OBJ_COLOR(obj),		// 20
			GET_OBJ_PROTOVNUM(obj),
			GET_OBJ_RESOURCE(obj)
		))) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "(%s)%s:%d Error writing rent object.", __FILE__, __FUNCTION__, __LINE__);
			return FALSE;
		}

		obj->unique_id = obj_order_num;

		SQL_FREE(name);
		SQL_FREE(short_description);
		SQL_FREE(description);
		SQL_FREE(action_description);

		/*
		 * Do we have extra descriptions? 
		 */
		if (obj->ex_description) {	// Yes, save them too.
			for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
				/*
				 * Sanity check to prevent nasty protection faults.
				 */
				if (!ex_desc->keyword || !ex_desc->description || !*ex_desc->keyword || !*ex_desc->description) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "(%s)%s:%d Corrupt ex_desc!", __FILE__, __FUNCTION__, __LINE__);
					continue;
				}
				buf1[MAX_STRING_LENGTH - 1] = '\0';
				strncpy(buf1, ex_desc->description, MAX_STRING_LENGTH - 1);
				SQL_MALLOC(buf1, description);
				SQL_ESC(buf1, description);
				/* copy keyword and allocate */
				buf1[MAX_STRING_LENGTH - 1] = '\0';
				strcpy(buf1, ex_desc->keyword);
				SQL_MALLOC(buf1, keyword);
				SQL_ESC(buf1, keyword);

				if (!(mysqlWrite(
					exdescs_insert,
					desc_table,
					(type == CRASH_PLAYER ? "player" : "room"),
					obj->unique_id,
					keyword,
					description,
					id
				))) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "(%s)%s:%d Error writing object extra description to database.", __FILE__, __FUNCTION__, __LINE__);
					SQL_FREE(keyword);
					SQL_FREE(description);
					return (0);
				}

				SQL_FREE(keyword);
				SQL_FREE(description);
			}
		}
		/*
		 * Do we have affects? 
		 */
		for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++) {
			if (obj->affected[counter2].modifier) {

				if (!(mysqlWrite(
					affects_insert,
					aff_table,
					(type == CRASH_PLAYER ? "player" : "room"),
					obj->unique_id,
					obj->affected[counter2].location,
					obj->affected[counter2].modifier,
					id
				))) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "(%s)%s:%d Error writing object modifier to database.", __FILE__, __FUNCTION__, __LINE__);
					return (0);
				}

			}
		}

	} else {
		/* Object is prototyped */

		if (!(mysqlWrite(
			prototype_insert,
			obj_table,
			(type == CRASH_PLAYER ? "player" : "room"),
			id,
			locate,
			GET_OBJ_VNUM(obj),
			obj_order_num,
			GET_OBJ_VAL(obj, 0),
			GET_OBJ_VAL(obj, 1),
			GET_OBJ_VAL(obj, 2),
			GET_OBJ_VAL(obj, 3),
			GET_OBJ_COLOR(obj),
			GET_OBJ_RESOURCE(obj)
		))) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "(%s)%s:%d Error writing rent object.", __FILE__, __FUNCTION__, __LINE__);
			return FALSE;
		}
	}

	return (1);
}


int	Crash_write_rentcode(struct char_data *ch, struct rent_info rent)
{
	const char *rent_players = "REPLACE INTO "
		"%s "						// Table name
		"("
			"PlayerID, "	// 1
			"RentCode, "
			"Timed, "
			"NetCost, "
			"Gold, "			// 5
			"Account, "
			"NumItems"
		") "
		"VALUES("
			"%ld, "				// 1
			"%ld, "
			"%ld, "
			"%ld, "
			"%ld, "				// 5
			"%ld, "
			"%d"
		");";

	if (!(mysqlWrite(
		rent_players,
		TABLE_RENT_PLAYERS,			// Table name
		GET_IDNUM(ch),					// 1
		rent.rentcode,
		rent.time,
		rent.net_cost_per_diem,
		rent.gold,							// 5
		rent.account,
		rent.nitems
	))) {
		extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "(%s)%s:%d Error writing player rent code to %s for %s.", __FILE__, __FUNCTION__, __LINE__, TABLE_RENT_PLAYERS, GET_NAME(ch));
		return (0);
	}

	return (1);
}


int	Crash_save(struct obj_data *obj, int id, int location)
{
	struct obj_data *tmp;
	int result;

	if (obj) {
		Crash_save(obj->next_content, id, location);
		Crash_save(obj->contains, id, MIN(0, location) - 1);
		result = Obj_to_store(obj, id, location, CRASH_PLAYER);

		for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
			GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

		if (!result)
			return (0);
	}
	return (1);
}


void Crash_datasave(struct char_data *ch, int cost, int type, const char *function)
{
	struct rent_info rent;
	int j, cost_eq = 0;

	if (IS_NPC(ch))
		return;

	switch (type) {
	case RENT_CRASH:
		break;
	case RENT_RENTED:
	case RENT_CRYO:
		Crash_extract_norent_eq(ch);
		Crash_extract_norents(ch->carrying);
		rent.gold = GET_GOLD(ch);
		rent.account = GET_BANK_GOLD(ch);
		rent.net_cost_per_diem = cost;
		break;
	case RENT_TIMEDOUT:
		Crash_extract_norent_eq(ch);
		Crash_extract_norents(ch->carrying);
		rent.gold = GET_GOLD(ch);
		rent.account = GET_BANK_GOLD(ch);
		cost = 0;
		Crash_calculate_rent(ch->carrying, &cost);
		cost_eq = 0;
		for (j = 0; j < NUM_WEARS; j++)
			Crash_calculate_rent(GET_EQ(ch, j), &cost_eq);
		cost += cost_eq;
		cost *= 2;    /* forcerent cost is 2x normal rent */
		rent.net_cost_per_diem = cost;
		break;
	default:
		extended_mudlog(NRM, SYSL_RENT, TRUE, "(%s)%s:%d called with undefined rent type by %s.", __FILE__, __FUNCTION__, __LINE__, function);
		return;
	}

	obj_order_num = 0;

	rent.rentcode = type;
	rent.time = time(0);
	Crash_write_rentcode(ch, rent);

	/* Delete all the player's objects from the database. */
	mysqlWrite("DELETE FROM %s WHERE player_id = %ld;", TABLE_RENT_OBJECTS, GET_IDNUM(ch));

	switch (type) {

	case RENT_CRASH:
		for (j = 0; j < NUM_WEARS; j++)
			if (GET_EQ(ch, j)) {
				if (!Crash_save(GET_EQ(ch, j), GET_IDNUM(ch), j + 1)) {
					extended_mudlog(NRM, SYSL_RENT, TRUE, "(%s)%s:%d could not Crash_save object %d, called from %s.", __FILE__, __FUNCTION__, __LINE__, GET_OBJ_PROTOVNUM(GET_EQ(ch, j)), function);
					return;
				}
				Crash_restore_weight(GET_EQ(ch, j));
			}
		if (!Crash_save(ch->carrying, GET_IDNUM(ch), 0)) {
			extended_mudlog(NRM, SYSL_RENT, TRUE, "(%s)%s:%d could not Crash_save ch->carrying, called from %s.", __FILE__, __FUNCTION__, __LINE__, function);
			return;
		}
		Crash_restore_weight(ch->carrying);
		REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
		break;

	case RENT_TIMEDOUT:
		if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
			for (j = 0; j < NUM_WEARS; j++)  /* Unequip players with low gold. */
				if (GET_EQ(ch, j))
					obj_to_char(unequip_char(ch, j), ch);
			while ((cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) && ch->carrying) {
				Crash_extract_expensive(ch->carrying);
				cost = 0;
				Crash_calculate_rent(ch->carrying, &cost);
				cost *= 2;
			}
		}
		if (ch->carrying == NULL) {
			for (j = 0; j < NUM_WEARS && GET_EQ(ch, j) == NULL; j++) /* Nothing */ ;
			if (j == NUM_WEARS) { /* No equipment or inventory. */
				Crash_delete_file(GET_IDNUM(ch));
				return;
			}
		}
		for (j = 0; j < NUM_WEARS; j++) {
			if (GET_EQ(ch, j)) {
				if (!Crash_save(GET_EQ(ch, j), GET_IDNUM(ch), j + 1)) {
					extended_mudlog(NRM, SYSL_RENT, TRUE, "(%s)%s:%d could not Crash_save object %d, called from %s.", __FILE__, __FUNCTION__, __LINE__, GET_OBJ_PROTOVNUM(GET_EQ(ch, j)), function);
					return;
				}
				Crash_restore_weight(GET_EQ(ch, j));
				Crash_extract_objs(GET_EQ(ch, j));
			}
		}
		if (!Crash_save(ch->carrying, GET_IDNUM(ch), 0)) {
			extended_mudlog(NRM, SYSL_RENT, TRUE, "(%s)%s:%d could not Crash_save ch->carrying, called from %s.", __FILE__, __FUNCTION__, __LINE__, function);
			return;
		}
		Crash_extract_objs(ch->carrying);
		break;

	case RENT_RENTED:
		for (j = 0; j < NUM_WEARS; j++)
			if (GET_EQ(ch, j)) {
				if (!Crash_save(GET_EQ(ch, j), GET_IDNUM(ch), j + 1)) {
					extended_mudlog(NRM, SYSL_RENT, TRUE, "(%s)%s:%d could not Crash_save object %d, called from %s.", __FILE__, __FUNCTION__, __LINE__, GET_OBJ_PROTOVNUM(GET_EQ(ch, j)), function);
					return;
				}
				Crash_restore_weight(GET_EQ(ch, j));
				Crash_extract_objs(GET_EQ(ch, j));
			}
		if (!Crash_save(ch->carrying, GET_IDNUM(ch), 0)) {
			extended_mudlog(NRM, SYSL_RENT, TRUE, "(%s)%s:%d could not Crash_save ch->carrying, called from %s.", __FILE__, __FUNCTION__, __LINE__, function);
			return;
		}
		Crash_extract_objs(ch->carrying);
		break;

	case RENT_CRYO:
		GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - cost);
		for (j = 0; j < NUM_WEARS; j++)
			if (GET_EQ(ch, j)) {
				if (!Crash_save(GET_EQ(ch, j), GET_IDNUM(ch), j + 1)) {
					extended_mudlog(NRM, SYSL_RENT, TRUE, "(%s)%s:%d could not Crash_save object %d, called from %s.", __FILE__, __FUNCTION__, __LINE__, GET_OBJ_PROTOVNUM(GET_EQ(ch, j)), function);
					return;
				}
				Crash_restore_weight(GET_EQ(ch, j));
				Crash_extract_objs(GET_EQ(ch, j));
			}
		if (!Crash_save(ch->carrying, GET_IDNUM(ch), 0)) {
			extended_mudlog(NRM, SYSL_RENT, TRUE, "(%s)%s:%d could not Crash_save ch->carrying, called from %s.", __FILE__, __FUNCTION__, __LINE__, function);
			return;
		}
		Crash_extract_objs(ch->carrying);
		SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
		break;

	}

}


void Crash_save_all(void)
{
	struct descriptor_data *d;
	for (d = descriptor_list; d; d = d->next) {
		if ((STATE(d) == CON_PLAYING || STATE(d) == CON_COPYOVER) && !IS_NPC(d->character)) {
			if (PLR_FLAGGED(d->character, PLR_CRASH)) {
				crash_datasave(d->character, 0, RENT_CRASH);
				save_char(d->character, NOWHERE, FALSE);
			}
		}
	}
}


/*
 * Routines used for the receptionist
 */

void Crash_rent_deadline(struct char_data *ch, struct char_data *recep,
												 long cost)
{
	long rent_deadline;

	if (!cost)
		return;

	rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / cost);
	sprintf(buf,
					"$n tells you, 'You can rent for %ld day%s with the gold you have\r\n"
					"on hand and in the bank.'\r\n",
					rent_deadline, (rent_deadline > 1) ? "s" : "");
	act(buf, FALSE, recep, 0, ch, TO_VICT);
}


int	Crash_report_unrentables(struct char_data *ch, struct char_data *recep,
														 struct obj_data *obj)
{
	char buf[128];
	int has_norents = 0;

	if (obj) {
		if (Crash_is_unrentable(obj)) {
			has_norents = 1;
			sprintf(buf, "$n tells you, 'You cannot store %s.'", OBJS(obj, ch));
			act(buf, FALSE, recep, 0, ch, TO_VICT);
		}
		has_norents += Crash_report_unrentables(ch, recep, obj->contains);
		has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
	}
	return (has_norents);
}


void Crash_report_rent(struct char_data *ch, struct char_data *recep,
											 struct obj_data *obj, long *cost, long *nitems, 
											 int display, int factor)
{
	static char buf[256];

	if (obj) {
		if (!Crash_is_unrentable(obj)) {
			(*nitems)++;
			*cost += MAX(0, (GET_OBJ_RENT(obj) * factor));
			if (display) {
				sprintf(buf, "$n tells you, '%5d coins for %s..'",
								(GET_OBJ_RENT(obj) * factor), OBJS(obj, ch));
				act(buf, FALSE, recep, 0, ch, TO_VICT);
			}
		}
		Crash_report_rent(ch, recep, obj->contains, cost, nitems, display, factor);
		Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display, factor);
	}
}


int	Crash_offer_rent(struct char_data *ch, struct char_data *receptionist,
										 int display, int factor)
{
	char buf[MAX_INPUT_LENGTH];
	int i;
	long totalcost = 0, numitems = 0, norent;

	norent = Crash_report_unrentables(ch, receptionist, ch->carrying);
	for (i = 0; i < NUM_WEARS; i++)
		norent += Crash_report_unrentables(ch, receptionist, GET_EQ(ch, i));

	if (norent)
		return (0);

	totalcost = min_rent_cost *factor;

	Crash_report_rent(ch, receptionist, ch->carrying, &totalcost, &numitems, display, factor);

	for (i = 0; i < NUM_WEARS; i++)
		Crash_report_rent(ch, receptionist, GET_EQ(ch, i), &totalcost, &numitems, display, factor);

	if (!numitems) {
		act("$n tells you, 'But you are not carrying anything!  Just quit!'",
				FALSE, receptionist, 0, ch, TO_VICT);
		return (0);
	}
	if (numitems > max_obj_save) {
		sprintf(buf, "$n tells you, 'Sorry, but I cannot store more than %d items.'",
						max_obj_save);
		act(buf, FALSE, receptionist, 0, ch, TO_VICT);
		return (0);
	}
	if (display) {
		sprintf(buf, "$n tells you, 'Plus, my %d coin fee..'",
						min_rent_cost *factor);
		act(buf, FALSE, receptionist, 0, ch, TO_VICT);
		sprintf(buf, "$n tells you, 'For a total of %ld coins%s.'",
						totalcost, (factor == RENT_FACTOR ? " per day" : ""));
		act(buf, FALSE, receptionist, 0, ch, TO_VICT);
		if (totalcost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
			act("$n tells you, '...which I see you can't afford.'",
					FALSE, receptionist, 0, ch, TO_VICT);
			return (0);
		} else if (factor == RENT_FACTOR)
			Crash_rent_deadline(ch, receptionist, totalcost);
	}
	return (totalcost);
}


int	gen_receptionist(struct char_data *ch, struct char_data *recep,
										 int cmd, char *arg, int mode)
{
	int cost;
	room_rnum save_room;
	const char *action_table[] = { "smile", "dance", "sigh", "blush", "burp",
																 "cough", "fart", "twiddle", "yawn" };

	if (!ch->desc || IS_NPC(ch))
		return (FALSE);

	if (!cmd && !number(0, 5)) {
		do_action(recep, NULL, find_command(action_table[number(0, 8)]), 0);
		return (FALSE);
	}
	if (!CMD_IS("offer") && !CMD_IS("rent"))
		return (FALSE);
	if (!AWAKE(recep)) {
		sprintf(buf, "%s is unable to talk to you...\r\n", HSSH(recep));
		send_to_char(buf, ch);
		return (TRUE);
	}
	if (!CAN_SEE(recep, ch)) {
		act("$n says, 'I don't deal with people I can't see!'", FALSE, recep, 0, 0, TO_ROOM);
		return (TRUE);
	}
	if (free_rent) {
		act("$n tells you, 'Rent is free here.  Just quit, and your objects will be saved!'",
				FALSE, recep, 0, ch, TO_VICT);
		return (1);
	}
	if (CMD_IS("rent")) {
		if (!(cost = Crash_offer_rent(ch, recep, FALSE, mode)))
			return (TRUE);
		if (mode == RENT_FACTOR)
			sprintf(buf, "$n tells you, 'Rent will cost you %d gold coins per day.'", cost);
		else if (mode == CRYO_FACTOR)
			sprintf(buf, "$n tells you, 'It will cost you %d gold coins to be frozen.'", cost);
		act(buf, FALSE, recep, 0, ch, TO_VICT);
		if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
			act("$n tells you, '...which I see you can't afford.'",
					FALSE, recep, 0, ch, TO_VICT);
			return (TRUE);
		}
		if (cost && (mode == RENT_FACTOR))
			Crash_rent_deadline(ch, recep, cost);

		if (mode == RENT_FACTOR) {
			act("$n stores your belongings and helps you into your private chamber.",
					FALSE, recep, 0, ch, TO_VICT);
			crash_datasave(ch, cost, RENT_RENTED);
			sprintf(buf, "%s has rented (%d/day, %d tot.)", GET_NAME(ch),
							cost, GET_GOLD(ch) + GET_BANK_GOLD(ch));
		} else {    /* cryo */
			act("$n stores your belongings and helps you into your private chamber.\r\n"
					"A white mist appears in the room, chilling you to the bone...\r\n"
					"You begin to lose consciousness...",
					FALSE, recep, 0, ch, TO_VICT);
			crash_datasave(ch, cost, RENT_CRYO);
			sprintf(buf, "%s has cryo-rented.", GET_NAME(ch));
			SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
		}

		extended_mudlog(NRM, SYSL_RENT, TRUE, buf);
		act("$n helps $N into $S private chamber.", FALSE, recep, 0, ch, TO_NOTVICT);
		save_room = IN_ROOM(ch);
		extract_char(ch);
		save_char(ch, save_room, FALSE);
	} else {
		Crash_offer_rent(ch, recep, TRUE, mode);
		act("$N gives $n an offer.", FALSE, ch, 0, recep, TO_ROOM);
	}
	return (TRUE);
}


SPECIAL(receptionist)
{
	return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, RENT_FACTOR));
}


SPECIAL(cryogenicist)
{
	return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, CRYO_FACTOR));
}
