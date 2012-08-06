/* ************************************************************************
*		File: house.c                                       Part of CircleMUD *
*	 Usage: Handling of player houses                                       *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001, Torgny Bjers.                                       *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
*                                                                         *
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: house.c,v 1.24 2004/04/20 16:30:13 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "house.h"
#include "genhouse.h"
#include "constants.h"

#define	LOC_ROOM				0
#define	MAX_BAG_ROWS		5

/* external functions */
int	Obj_to_store(struct obj_data *obj, int id, int locate, int type);
bitvector_t	asciiflag_conv(char *flag);

/* external variables */
extern int mini_mud;
extern struct house_data *house_index;
extern int top_of_houset;
extern int obj_order_num;

/* local functions */
int ok_house_room(house_vnum vnum, room_vnum room);
int	handle_house_obj(struct obj_data *temp, room_rnum rnum, int locate, struct obj_data **cont_row);
int	load_house_objects(house_vnum vnum);
void house_restore_weight(struct obj_data *obj);
void house_delete_file(house_vnum vnum);
int	find_house(room_vnum vnum);
int	find_house_owner(struct char_data *ch);
int house_can_enter(struct char_data *ch, room_vnum house);
int house_can_take(struct char_data *ch, room_vnum house);
void hcontrol_list_houses(struct char_data *ch);
void hcontrol_build_house(struct char_data *ch, char *arg);
void hcontrol_destroy_house(struct char_data *ch, char *arg);
void hcontrol_pay_house(struct char_data *ch, char *arg);
ACMD(do_hcontrol);
ACMD(do_house);

/*
 * Utility functions
 */

int	find_house(room_vnum vnum)
{
	int house_nr;

	if (!house_index)
		return (NOWHERE);

	for (house_nr = 0; house_nr <= top_of_houset; house_nr++)
		if (ok_house_room(house_nr, vnum))
			return (house_nr);

	return (NOWHERE);
}


int	find_house_owner(struct char_data *ch)
{
	int house_nr;

	if (!house_index)
		return (NOWHERE);

	for (house_nr = 0; house_nr <= top_of_houset; house_nr++)
		if (HOUSE_OWNER(house_nr) == GET_IDNUM(ch))
			return (HOUSE_NUM(house_nr));

	return (NOWHERE);
}


int	find_cowner(struct char_data *ch, int house_nr)
{
	int count;

	if (!house_index)
		return (0);

	for (count = 0; HOUSE_COWNER(house_nr, count) != NOWHERE; count++)
		if (HOUSE_COWNER(house_nr, count) == GET_IDNUM(ch))
			return (1);

	return (0);
}


int	find_guest(struct char_data *ch, int house_nr)
{
	int count;

	if (!house_index)
		return (0);

	for (count = 0; HOUSE_GUEST(house_nr, count) != NOWHERE; count++)
		if (HOUSE_GUEST(house_nr, count) == GET_IDNUM(ch))
			return (1);

	return (0);
}


int ok_house_room(house_rnum rnum, room_vnum room)
{
	int index;

	if (!house_index)
		return (NOWHERE);

	for (index = 0; HOUSE_ROOM(rnum, index) != NOWHERE; index++)
		if (HOUSE_ROOM(rnum, index) == room)
			return (TRUE);
	return (FALSE);
}


int	house_can_enter(struct char_data *ch, room_vnum room)
{
	int house_nr;

	if (IS_GRGOD(ch) || (house_nr = find_house(room)) == NOWHERE)
		return (1);

	switch (house_index[house_nr].mode) {
	case HOUSE_PRIVATE:
		if (GET_IDNUM(ch) == HOUSE_OWNER(house_nr)) {
			HOUSE_USED(house_nr) = time(0);
			return (1);
		}
		if (find_cowner(ch, house_nr)) {
			HOUSE_USED(house_nr) = time(0);
			return (1);
		}
		if (find_guest(ch, house_nr))
			return (1);
		break;
	case HOUSE_PUBLIC:
		return (1);
	}

	return (0);
}


int house_can_take(struct char_data *ch, room_vnum room)
{
	int house_nr;

	if (IS_GRGOD(ch) || (house_nr = find_house(room)) == NOWHERE)
		return (1);

	switch (house_index[house_nr].mode) {
	case HOUSE_PRIVATE:
	case HOUSE_PUBLIC:
		if (GET_IDNUM(ch) == HOUSE_OWNER(house_nr))
			return (1);
		if (find_cowner(ch, house_nr))
			return (1);
		break;
	}

	return (0);
}


int	handle_house_obj(struct obj_data *temp, room_rnum rnum, int locate, struct obj_data **cont_row)
{
	int j;
	struct obj_data *obj1;

	if (!temp)
		return (0);

	obj_to_room(temp, rnum);

	for (j = MAX_BAG_ROWS-1;j > -locate;j--)
		if (cont_row[j]) { /* no container -> back to room */
			for (;cont_row[j];cont_row[j] = obj1) {
				obj1 = cont_row[j]->next_content;
				obj_to_room(cont_row[j], rnum);
			} 
			cont_row[j] = NULL;
		}

	if (j == -locate && cont_row[j]) { /* content list existing */
		if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER || GET_OBJ_TYPE(temp) == ITEM_SHEATH) {
			/* take item ; fill ; put in room again */
			obj_from_room(temp);
			temp->contains = NULL;
			for (;cont_row[j];cont_row[j] = obj1) {
				obj1 = cont_row[j]->next_content;
				obj_to_obj(cont_row[j], temp);
			}
			obj_to_room(temp, rnum); /* add to room first ... */
		} else { /* object isn't container -> empty content list */
			for (;cont_row[j];cont_row[j] = obj1) {
				obj1 = cont_row[j]->next_content;
				obj_to_room(cont_row[j], rnum);
			}
			cont_row[j] = NULL;
		}
	}

	if (locate < 0 && locate >= -MAX_BAG_ROWS) {
		/*
		 * let obj be part of content list
		 * but put it at the list's end thus having the items
		 * in the same order as before renting
		 */
		obj_from_room(temp);
		if ((obj1 = cont_row[-locate-1])) {
			while (obj1->next_content)
				obj1 = obj1->next_content;
			obj1->next_content = temp;
		} else
			cont_row[-locate-1] = temp;
	}

	return (1);
}


/*
 * Load all objects for a house
 * New MySQL version for MySQL objsave.c functions.
 * by Torgny Bjers <artovil@arcanerealms.org>, 2002-12-21
 */
int	load_house_objects(room_vnum vnum)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	struct obj_data *temp = NULL;
	struct obj_data *cont_row[MAX_BAG_ROWS];
	unsigned long *fieldlength;
	int num_objs = 0, locate = 0;
	obj_vnum ovnum;
	room_rnum rnum;
	obj_vnum proto_number;
	int i;

	if ((rnum = real_room(vnum)) == NOWHERE)
		return (0);

	for (i = 0; i < MAX_BAG_ROWS; i++)
		cont_row[i] = NULL;

	if (!(result = mysqlGetResource(TABLE_HOUSE_RENT_OBJECTS, "SELECT * FROM %s WHERE room_id = %ld ORDER BY obj_order ASC;", TABLE_HOUSE_RENT_OBJECTS, vnum))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "(%s)%s:%d Could not load house rent files from table [%s].", __FILE__, __FUNCTION__, __LINE__, TABLE_HOUSE_RENT_OBJECTS);
		return (0);
	}

	num_objs = mysql_num_rows(result);

	if (!num_objs) {
		mysql_free_result(result);
		return (0);
	}

	if (num_objs > 0) {
		while ((row = mysql_fetch_row(result)))
		{
			fieldlength = mysql_fetch_lengths(result);
			temp = NULL;

			locate = atoi(row[1]);
			ovnum = atoi(row[2]);
			proto_number = atoi(row[21]);

			if (ovnum == NOTHING) {

				temp = create_obj();
				temp->item_number = ovnum;
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

				for (i = 0; i < MAX_OBJ_AFFECT; i++) {
					temp->affected[i].location = APPLY_NONE;
					temp->affected[i].modifier = 0;
				}

			} else {

				if (real_object(ovnum) != NOTHING) {
					temp = read_object(ovnum, VIRTUAL);
					GET_OBJ_COLOR(temp) = atoi(row[20]);
					GET_OBJ_RESOURCE(temp) = atoi(row[22]);
				} else {
					extended_mudlog(BRF, SYSL_RENT, TRUE, "Nonexistent object %d found in rent file for house %d.", ovnum, vnum);
				}

			}

			handle_house_obj(temp, rnum, locate, cont_row);

		}
	}

	return (1);
}


/*
 * Save all objects for a house (recursive; initial call must
 * be followed by a call to house_restore_weight)
 * Now takes bag rows into account just like objsave.c.
 * Torgny Bjers <artovil@arcanerealms.org>, 2002-12-21.
 */
int	save_house_objects(struct obj_data *obj, house_vnum vnum, int location)
{
	struct obj_data *tmp;
	int result;

	if (obj) {
		save_house_objects(obj->next_content, vnum, location);
		save_house_objects(obj->contains, vnum, MIN(LOC_ROOM, location) - 1);
		result = Obj_to_store(obj, vnum, location, CRASH_HOUSE);

		for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
			GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

		if (!result)
			return (0);
	}
	return (1);
}


/* restore weight of containers after house_save has changed them for saving */
void house_restore_weight(struct obj_data *obj)
{
	if (obj) {
		house_restore_weight(obj->contains);
		house_restore_weight(obj->next_content);
		if (obj->in_obj)
			GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
	}
}


/* Save all objects in a house (recurses through all rooms) */
void house_crashsave_objects(house_vnum vnum)
{
	house_rnum hrnum;
	room_rnum rnum;
	int index;

	if ((hrnum = real_house(vnum)) == NOWHERE)
		return;

	/* Save down each room in the house separately. */
	for (index = 0; HOUSE_ROOM(hrnum, index) != NOWHERE; index++) {
		if ((rnum = real_room(HOUSE_ROOM(hrnum, index))) == NOWHERE)
			continue;
		if (!ROOM_FLAGGED(rnum, ROOM_HOUSE_CRASH))
			continue;
		/* Delete all the house room's objects from the database. */
		mysqlWrite("DELETE FROM %s WHERE room_id = %ld;", TABLE_HOUSE_RENT_OBJECTS, HOUSE_ROOM(hrnum, index));
		obj_order_num = 0;
		if (!save_house_objects(world[rnum].contents, HOUSE_ROOM(hrnum, index), LOC_ROOM))
			continue;
		house_restore_weight(world[rnum].contents);
		REMOVE_BIT(ROOM_FLAGS(rnum), ROOM_HOUSE_CRASH);
	}
}


/* Delete saved house objects in one fell swoop. */
void house_delete_file(house_vnum vnum)
{
	house_rnum hrnum;
	int index;

	if ((hrnum = real_house(vnum)) == NOWHERE)
		return;

	if (!(mysqlWrite("DELETE FROM %s WHERE vnum = %ld;", TABLE_HOUSE_ROOMS, HOUSE_NUM(hrnum))))
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error removing rooms for house %d.", HOUSE_NUM(hrnum));

	if (!(mysqlWrite("DELETE FROM %s WHERE vnum = %ld;", TABLE_HOUSE_COWNERS, HOUSE_NUM(hrnum))))
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error removing co-owners for house %d.", HOUSE_NUM(hrnum));

	if (!(mysqlWrite("DELETE FROM %s WHERE vnum = %ld;", TABLE_HOUSE_GUESTS, HOUSE_NUM(hrnum))))
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error removing guests for house %d.", HOUSE_NUM(hrnum));

	if (!(mysqlWrite("DELETE FROM %s WHERE vnum = %ld;", TABLE_HOUSE_INDEX, HOUSE_NUM(hrnum))))
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error removing house %d.", HOUSE_NUM(hrnum));

	for (index = 0; HOUSE_ROOM(hrnum, index) != NOWHERE; index++) {
		/* Delete all the house objects from the database. */
		if (!(mysqlWrite("DELETE FROM %s WHERE room_id = %ld;", TABLE_HOUSE_RENT_OBJECTS, HOUSE_ROOM(hrnum, index))))
			extended_mudlog(BRF, SYSL_SQL, TRUE, "Error removing rent objects for house %d, room %ld.", HOUSE_NUM(hrnum), HOUSE_ROOM(hrnum, index));
		if (!(mysqlWrite("DELETE FROM %s WHERE room_id = %ld;", TABLE_HOUSE_RENT_AFFECTS, HOUSE_ROOM(hrnum, index))))
			extended_mudlog(BRF, SYSL_SQL, TRUE, "Error removing rent objects affects for house %d, room %ld.", HOUSE_NUM(hrnum), HOUSE_ROOM(hrnum, index));
		if (!(mysqlWrite("DELETE FROM %s WHERE room_id = %ld;", TABLE_HOUSE_RENT_EXTRADESCS, HOUSE_ROOM(hrnum, index))))
			extended_mudlog(BRF, SYSL_SQL, TRUE, "Error removing rent objects extradescs for house %d, room %ld.", HOUSE_NUM(hrnum), HOUSE_ROOM(hrnum, index));
	}
}


/* New version of house loading for MySQL by Torgny Bjers, 2002-12-27. */
void parse_house(MYSQL_ROW house_row, unsigned long *fieldlength, int vnum)
{
	MYSQL_RES *rooms;
	MYSQL_RES *owners;
	MYSQL_RES *guests;
	MYSQL_ROW rooms_row;
	MYSQL_ROW owners_row;
	MYSQL_ROW guests_row;
	static int house_nr = 0;
	room_rnum rhouse = NOWHERE, ratrium = NOWHERE;
	int row_count, count = 0;
	bool error = FALSE;

	HOUSE_NUM(house_nr) = vnum;
	HOUSE_ATRIUM(house_nr) = atoi(house_row[1]);
	HOUSE_EXIT(house_nr) = atoi(house_row[2]);
	HOUSE_OWNER(house_nr) = atoi(house_row[3]);
	HOUSE_BUILT(house_nr) = atoi(house_row[4]);
	HOUSE_USED(house_nr) = atoi(house_row[5]);
	HOUSE_PRUNE_SAFE(house_nr) = atoi(house_row[6]);
	HOUSE_MODE(house_nr) = atoi(house_row[7]);
	HOUSE_PAID(house_nr) = atoi(house_row[8]);
	HOUSE_COST(house_nr) = atoi(house_row[9]);
	HOUSE_MAX_SECURE(house_nr) = atoi(house_row[10]);
	HOUSE_MAX_LOCKED(house_nr) = atoi(house_row[11]);

	if (get_name_by_id(HOUSE_OWNER(house_nr)) == NULL)
		extended_mudlog(NRM, SYSL_HOUSES, TRUE, "House %d has an invalid owner.", HOUSE_NUM(house_nr));

	if ((ratrium = real_room(HOUSE_ATRIUM(house_nr))) == NOWHERE) {
		extended_mudlog(NRM, SYSL_HOUSES, TRUE, "House %d not loaded because house atrium does not exist.", HOUSE_NUM(house_nr));
		return;	/* house doesn't have an atrium */
	}

	if (HOUSE_EXIT(house_nr) < 0 || HOUSE_EXIT(house_nr) >= NUM_OF_DIRS) {
		extended_mudlog(NRM, SYSL_HOUSES, TRUE, "House %d not loaded due to an invalid exit number.", HOUSE_NUM(house_nr));
		return;	/* invalid exit num */
	}

	/* open MySQL table for house rooms */
	if (!(rooms = mysqlGetResource(TABLE_HOUSE_ROOMS, "SELECT * FROM %s WHERE vnum = %d ORDER BY id ASC;", TABLE_HOUSE_ROOMS, vnum)))
		error = TRUE;

	if (!(owners = mysqlGetResource(TABLE_HOUSE_COWNERS, "SELECT * FROM %s WHERE vnum = %d ORDER BY id ASC;", TABLE_HOUSE_COWNERS, vnum)))
		error = TRUE;

	if (!(guests = mysqlGetResource(TABLE_HOUSE_GUESTS, "SELECT * FROM %s WHERE vnum = %d ORDER BY id ASC;", TABLE_HOUSE_GUESTS, vnum)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	count = 0;

	/*
	 * Load the room list from MySQL,
	 * this list has to be terminated with -1.
	 */
	row_count = mysql_num_rows(rooms);
	CREATE(house_index[house_nr].rooms, long, row_count + 1);
	while ((rooms_row = mysql_fetch_row(rooms)))
		HOUSE_ROOM(house_nr, count++) = atoi(rooms_row[2]);
	HOUSE_ROOM(house_nr, count) = NOWHERE;

	mysql_free_result(rooms);

	count = 0;

	/*
	 * Load the owner list from MySQL,
	 * this list has to be terminated with -1.
	 */
	row_count = mysql_num_rows(owners);
	CREATE(house_index[house_nr].cowners, long, row_count + 1);
	while ((owners_row = mysql_fetch_row(owners)))
		HOUSE_COWNER(house_nr, count++) = atoi(owners_row[2]);
	HOUSE_COWNER(house_nr, count) = NOBODY;

	mysql_free_result(owners);

	count = 0;

	/*
	 * Load the guest list from MySQL,
	 * this list has to be terminated with -1.
	 */
	row_count = mysql_num_rows(guests);
	CREATE(house_index[house_nr].guests, long, row_count + 1);
	while ((guests_row = mysql_fetch_row(guests)))
		HOUSE_GUEST(house_nr, count++) = atoi(guests_row[2]);
	HOUSE_GUEST(house_nr, count) = NOBODY;

	mysql_free_result(guests);

	if ((rhouse = real_room(HOUSE_ROOM(house_nr, 0))) == NOWHERE) {
		extended_mudlog(NRM, SYSL_HOUSES, TRUE, "House %d not loaded because house room does not exist.", HOUSE_NUM(house_nr));
		return;	/* this vnum doesn't exist -- skip */
	}

	if (TOROOM(rhouse, HOUSE_EXIT(house_nr)) != ratrium) {
		extended_mudlog(NRM, SYSL_HOUSES, TRUE, "House %d not loaded because exit #%d did not lead to atrium.", HOUSE_EXIT(house_nr), HOUSE_NUM(house_nr));
		return;	/* exit num mismatch -- skip */
	}

	SET_BIT(ROOM_FLAGS(ratrium), ROOM_ATRIUM);

	for (count = 0; HOUSE_ROOM(house_nr, count) != NOWHERE; count++) {
		if ((rhouse = real_room(HOUSE_ROOM(house_nr, count))) == NOWHERE) {
			extended_mudlog(NRM, SYSL_HOUSES, TRUE, "Room %ld skipped because it does not exist.", HOUSE_ROOM(house_nr, count));
			continue;	/* this vnum doesn't exist -- skip */
		}
		SET_BIT(ROOM_FLAGS(rhouse), ROOM_HOUSE | ROOM_PRIVATE);
		load_house_objects(HOUSE_ROOM(house_nr, count));
	}

	top_of_houset = house_nr++;
}


const	char *HCONTROL_FORMAT =
"Usage: hcontrol build <house vnum> <exit direction> <player name>\r\n"
"       hcontrol destroy <house vnum>\r\n"
"       hcontrol pay <house vnum>\r\n"
"       hcontrol show\r\n";

void hcontrol_list_houses(struct char_data *ch)
{
	int house_nr, j;
	char *timestr, *temp;
	char built_on[128], last_pay[128], last_used[128], own_name[128];
	char *printbuf = get_buffer(65536);

	if (!house_index) {
		send_to_char("No houses have been defined.\r\n", ch);
		release_buffer(printbuf);
		return;
	}

	for (house_nr = 0; house_nr <= top_of_houset; house_nr++) {

		/* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
		if ((temp = get_name_by_id(HOUSE_OWNER(house_nr))) == NULL)
			continue;

		if (!(house_nr % (GET_PAGE_LENGTH(ch) - 2))) {
			strcat(printbuf, "Vnum    Atrium  Build Date  Guests  Owner             Last Paymt   Last Used\r\n");
			strcat(printbuf, "------  ------  ----------  ------  ----------------  -----------  -----------\r\n");
		}

		if (HOUSE_BUILT(house_nr)) {
			timestr = asctime(localtime(&(HOUSE_BUILT(house_nr))));
			*(timestr + 10) = '\0';
			strcpy(built_on, timestr);
		} else
			strcpy(built_on, "Unknown");

		if (HOUSE_PAID(house_nr)) {
			timestr = asctime(localtime(&(HOUSE_PAID(house_nr))));
			*(timestr + 10) = '\0';
			strcpy(last_pay, timestr);
		} else
			strcpy(last_pay, "None");

		if (HOUSE_USED(house_nr)) {
			timestr = asctime(localtime(&(HOUSE_USED(house_nr))));
			*(timestr + 10) = '\0';
			strcpy(last_used, timestr);
		} else
			strcpy(last_used, "None");

		/* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
		strcpy(own_name, temp);

		for (j = 0; HOUSE_GUEST(house_nr, j) != NOBODY; j++)
			; /* Loop does the trick */

		sprintf(printbuf, "%s%6d  %6d  %-10.10s      %2d  %-16.16s  %-11.11s  %-11.11s\r\n",
			printbuf, HOUSE_NUM(house_nr), HOUSE_ATRIUM(house_nr), built_on,
			j, CAP(own_name), last_pay, last_used);
	}

	if (!*printbuf)
		strcpy(printbuf, "No houses have been defined.\r\n");

	page_string(ch->desc, printbuf, TRUE);
	release_buffer(printbuf);
}


void hcontrol_pay_house(struct char_data *ch, char *arg)
{
	int i;

	if (!*arg)
		send_to_char(HCONTROL_FORMAT, ch);
	else if ((i = find_house(atoi(arg))) == NOWHERE)
		send_to_char("Unknown house.\r\n", ch);
	else {
		extended_mudlog(NRM, SYSL_HOUSES, TRUE, "Payment for house %s collected by %s.", arg, GET_NAME(ch));

		house_index[i].last_payment = time(0);
		send_to_char("Payment recorded.\r\n", ch);
	}
}


/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	half_chop(argument, arg1, arg2);

	if (is_abbrev(arg1, "pay"))
		hcontrol_pay_house(ch, arg2);
	else if (is_abbrev(arg1, "show"))
		hcontrol_list_houses(ch);
	else
		send_to_char(HCONTROL_FORMAT, ch);
}


/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int house_nr, count, id;
	
	half_chop(argument, arg1, arg2);
	
	if (is_abbrev(arg1, "guests")) {

		if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
			send_to_char("You must be in your house to set guests.\r\n", ch);
		else if ((house_nr = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) == NOWHERE)
			send_to_char("Um.. this house seems to be screwed up.\r\n", ch);
		else if (GET_IDNUM(ch) != HOUSE_OWNER(house_nr) && !find_cowner(ch, house_nr))
			send_to_char("Only owners can set guests.\r\n", ch);
		else if (!*arg2)
			house_list_guests(ch, house_nr, FALSE);
		else if ((id = get_id_by_name(arg2)) < 0)
			send_to_char("No such player.\r\n", ch);
		else if (id == GET_IDNUM(ch))
			send_to_char("It's your house!\r\n", ch);
		else {
			for (count = 0; HOUSE_GUEST(house_nr, count) != NOWHERE; count++)
				if (HOUSE_GUEST(house_nr, count) == id) {
					remove_from_house_list(&(HOUSE_GUESTS(house_nr)), count);
					send_to_char("Guest deleted.\r\n", ch);
					save_houses();
					return;
				}
			if (count == HOUSE_MAX_GUESTS) {
				send_to_char("You have too many guests.\r\n", ch);
				return;
			}
			add_to_house_list(&(HOUSE_GUESTS(house_nr)), id);
			send_to_char("Guest added.\r\n", ch);
			save_houses();
		}

	} else if (is_abbrev(arg1, "cowners")) {

		if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
			send_to_char("You must be in your house to set co-owners.\r\n", ch);
		else if ((house_nr = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) == NOWHERE)
			send_to_char("Um.. this house seems to be screwed up.\r\n", ch);
		else if (GET_IDNUM(ch) != HOUSE_OWNER(house_nr))
			send_to_char("Only the primary owner can set co-owners.\r\n", ch);
		else if (!*arg2)
			house_list_cowners(ch, house_nr, FALSE);
		else if ((id = get_id_by_name(arg2)) < 0)
			send_to_char("No such player.\r\n", ch);
		else if (id == GET_IDNUM(ch))
			send_to_char("It's your house!\r\n", ch);
		else {
			for (count = 0; HOUSE_COWNER(house_nr, count) != NOWHERE; count++)
				if (HOUSE_COWNER(house_nr, count) == id) {
					remove_from_house_list(&(HOUSE_COWNERS(house_nr)), count);
					send_to_char("Guest deleted.\r\n", ch);
					save_houses();
					return;
				}
			if (count == HOUSE_MAX_COWNERS) {
				send_to_char("You have too many co-owners.\r\n", ch);
				return;
			}
			add_to_house_list(&(HOUSE_COWNERS(house_nr)), id);
			send_to_char("Co-owner added.\r\n", ch);
			save_houses();
		}

	} else {

		send_to_charf(ch, "Usage: %s { guests | cowners } <name>\r\n", CMD_NAME);

	}

}


/* crash-save all the houses */
void house_save_all(void)
{
	int house_nr;

	if (!house_index)
		return;

	for (house_nr = 0; house_nr <= top_of_houset; house_nr++)
		house_crashsave_objects(HOUSE_NUM(house_nr));
}


void house_list_guests(struct char_data *ch, int house_nr, int quiet)
{
	int j, guests = 0;
	char *temp;
	char buf[MAX_STRING_LENGTH], buf2[MAX_NAME_LENGTH + 2];

	strcpy(buf, "  Guests: ");

	for (j = 0; HOUSE_GUEST(house_nr, j) != NOBODY; j++) {
		if ((temp = get_name_by_id(HOUSE_GUEST(house_nr, j))) == NULL)
			continue;
		guests++;
		sprintf(buf2, "%s ", temp);
		strcat(buf, CAP(buf2));
	}

	if (!guests)
		strcat(buf, "None");

	strcat(buf, "\r\n");
	send_to_char(buf, ch);
}


void house_list_cowners(struct char_data *ch, int house_nr, int quiet)
{
	int j, cowners = 0;
	char *temp;
	char buf[MAX_STRING_LENGTH], buf2[MAX_NAME_LENGTH + 2];

	strcpy(buf, "  Co-owners: ");

	for (j = 0; HOUSE_COWNER(house_nr, j) != NOBODY; j++) {
		if ((temp = get_name_by_id(HOUSE_COWNER(house_nr, j))) == NULL)
			continue;
		cowners++;
		sprintf(buf2, "%s ", temp);
		strcat(buf, CAP(buf2));
	}

	if (!cowners)
		strcat(buf, "None");

	strcat(buf, "\r\n");
	send_to_char(buf, ch);
}


void house_auto_prune(void)
{
	int i, j;
	room_rnum real_atrium, real_house;

	for (i = 0; i < top_of_houset; i++) {
		if ((real_house = real_room(house_index[i].vnum)) != NOWHERE) {
			if (!house_index[i].prune_safe && (time(0) - house_index[i].last_used > 14 * SECS_PER_REAL_DAY)) {

				if ((real_atrium = real_room(house_index[i].atrium)) == NOWHERE)
					extended_mudlog(NRM, SYSL_BUGS, TRUE, "House %d had invalid atrium %d!", atoi(arg), house_index[i].atrium);
				else
					REMOVE_BIT(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
			
				if ((real_house = real_room(house_index[i].vnum)) == NOWHERE)
					extended_mudlog(NRM, SYSL_BUGS, TRUE, "House %d had invalid vnum %d!", atoi(arg), house_index[i].vnum);
				else
					REMOVE_BIT(ROOM_FLAGS(real_house), ROOM_HOUSE | ROOM_PRIVATE | ROOM_HOUSE_CRASH);
			
				house_delete_file(house_index[i].vnum);

				extended_mudlog(NRM, SYSL_HOUSES, TRUE, "House %d auto-pruned.", house_index[i].vnum);
			
				for (j = i; j < top_of_houset - 1; j++)
					house_index[j] = house_index[j + 1];
			
				top_of_houset--;
			
				save_houses();
			
				/*
				 * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums,
				 * just in case the house we just deleted shared an atrium with another
				 * house.  --JE 9/19/94
				 */
				for (i = 0; i < top_of_houset; i++)
					if ((real_atrium = real_room(house_index[i].atrium)) != NOWHERE)
						SET_BIT(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

			}
		}
	}
}
