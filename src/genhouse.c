/************************************************************************
 * Generic OLC Library - Houses / genhouse.c                       v1.0 *
 * Copyright 1996 by Harvey Gilpin                                      *
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)          *
 * Copyright 2002-2003 by Torgny Bjers (artovil@arcanerealms.org)       *
 *                                                                      *
 * MySQL C API connection for world files and various former disk based *
 * storage/loading functions made by Torgny Bjers for Arcane Realms MUD.*
 * Copyright (C)2001-2002, Torgny Bjers.                                *
 *                                                                      *
 * MYSQL COPYRIGHTS                                                     *
 * The client library, and the GNU getopt library, are covered by the   *
 * GNU LIBRARY GENERAL PUBLIC LICENSE.                                  *
 ************************************************************************/
/* $Id: genhouse.c,v 1.3 2002/12/28 16:17:02 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "house.h"
#include "genolc.h"
#include "genhouse.h"
#include "genzon.h"

extern struct house_data *house_index;
extern int top_of_houset;

/*-------------------------------------------------------------------*/

void copy_house(struct house_data *thouse, struct house_data *fhouse)
{
	/*
	 * Copy basic information over.
	 */
	H_NUM(thouse) = H_NUM(fhouse);
	H_ATRIUM(thouse) = H_ATRIUM(fhouse);
	H_EXIT(thouse) = H_EXIT(fhouse);
	H_BUILT(thouse) = H_BUILT(fhouse);
	H_USED(thouse) = H_USED(fhouse);
	H_PAID(thouse) = H_PAID(fhouse);
	H_OWNER(thouse) = H_OWNER(fhouse);
	H_PRUNE_SAFE(thouse) = H_PRUNE_SAFE(fhouse);
	H_MODE(thouse) = H_MODE(fhouse);
	H_COST(thouse) = H_COST(fhouse);
	H_MAX_SECURE(thouse) = H_MAX_SECURE(fhouse);
	H_MAX_LOCKED(thouse) = H_MAX_LOCKED(fhouse);
	/*
	 * Copy lists over.
	 */
	copy_house_list(&(H_ROOMS(thouse)), H_ROOMS(fhouse));
	copy_house_list(&(H_COWNERS(thouse)), H_COWNERS(fhouse));
	copy_house_list(&(H_GUESTS(thouse)), H_GUESTS(fhouse));
}

/*-------------------------------------------------------------------*/

/*
 * Copy a -1 terminated integer array list.
 */
void copy_house_list(long **tlist, long *flist)
{
	int num_items, i;

	if (*tlist)
		free(*tlist);

	/*
	 * Count number of entries.
	 */
	for (i = 0; flist[i] != -1; i++);
	num_items = i + 1;

	/*
	 * Make space for entries.
	 */
	CREATE(*tlist, long, num_items);

	/*
	 * Copy entries over.
	 */
	for (i = 0; i < num_items; i++)
		(*tlist)[i] = flist[i];
}

/*-------------------------------------------------------------------*/

void add_to_house_list(long **list, long newi)
{
	long i, num_items, *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i] != -1; i++);
	num_items = i;

	/*
	 * Make a new list and slot in the new entry.
	 */
	CREATE(nlist, long, num_items + 2);

	for (i = 0; i < num_items; i++)
		nlist[i] = (*list)[i];
	nlist[num_items] = newi;
	nlist[num_items + 1] = -1;

	/*
	 * Out with the old, in with the new.
	 */
	free(*list);
	*list = nlist;
}

/*-------------------------------------------------------------------*/

void remove_from_house_list(long **list, long num)
{
	long i, num_items, *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i] != -1; i++);

	if (num >= i || num < 0)
		return;
	num_items = i;

	CREATE(nlist, long, num_items);

	for (i = 0; i < num_items; i++)
		nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

	free(*list);
	*list = nlist;
}

/*-------------------------------------------------------------------*/

/*
 * Free up the whole house structure and it's content.
 */
void free_house(struct house_data *house)
{
	free(H_ROOMS(house));
	free(house);
}

/*-------------------------------------------------------------------*/

/*
 * Ew, linear search, O(n)
 */
int	real_house(int vhouse_num)
{
	int rhouse_num;

	if (!house_index)
		return (NOWHERE);

	for (rhouse_num = 0; rhouse_num <= top_of_houset; rhouse_num++)
		if (HOUSE_NUM(rhouse_num) == vhouse_num)
			return (rhouse_num);

	return (NOWHERE);
}

/*-------------------------------------------------------------------*/

int	add_house(struct house_data *nhouse)
{
	room_rnum rroom = NOWHERE, ratrium = NOWHERE;
	house_rnum rhouse;
	int found = 0, count = 0;

	if ((ratrium = real_room(H_ATRIUM(nhouse))) == NOWHERE) {
		extended_mudlog(NRM, SYSL_HOUSES, TRUE, "House %d not added because house atrium does not exist.", H_NUM(nhouse));
		return (NOWHERE);	/* house doesn't have an atrium -- skip */
	}

	if (H_EXIT(nhouse) < 0 || H_EXIT(nhouse) >= NUM_OF_DIRS) {
		extended_mudlog(NRM, SYSL_HOUSES, TRUE, "House %d not added due to an invalid exit number.", H_NUM(nhouse));
		return (NOWHERE);	/* invalid exit num -- skip */
	}

	if ((rroom = real_room(H_ROOM(nhouse, 0))) == NOWHERE) {
		extended_mudlog(NRM, SYSL_HOUSES, TRUE, "House %d not added because house room does not exist.", H_NUM(nhouse));
		return (NOWHERE);	/* this vnum doesn't exist -- skip */
	}

	if (TOROOM(rroom, H_EXIT(nhouse)) != ratrium) {
		extended_mudlog(NRM, SYSL_HOUSES, TRUE, "House %d not added because exit #%d did not lead to atrium.", H_EXIT(nhouse), H_NUM(nhouse));
		return (NOWHERE);	/* exit num mismatch -- skip */
	}

	SET_BIT(ROOM_FLAGS(ratrium), ROOM_ATRIUM);

	for (count = 0; H_ROOM(nhouse, count) != NOWHERE; count++) {
		if ((rroom = real_room(H_ROOM(nhouse, count))) == NOWHERE)
			continue;	/* this vnum doesn't exist -- skip */
		SET_BIT(ROOM_FLAGS(rroom), ROOM_HOUSE | ROOM_PRIVATE);
	}

	/*
	 * The house already exists, just update it.
	 */
	if ((rhouse = real_house(H_NUM(nhouse))) != NOWHERE) {
		copy_house(&house_index[rhouse], nhouse);
		return rhouse;
	}

	if (!house_index) {
		CREATE(house_index, struct house_data, top_of_houset + 1);
	} else {
		top_of_houset++;
		RECREATE(house_index, struct house_data, top_of_houset + 1);
	}

	for (rhouse = top_of_houset; rhouse > 0; rhouse--) {
		if (nhouse->vnum > HOUSE_NUM(rhouse - 1)) {
			found = rhouse;

			/* Make a "nofree" variant and remove these later. */
			house_index[rhouse].rooms = NULL;
			house_index[rhouse].cowners = NULL;
			house_index[rhouse].guests = NULL;

			copy_house(&house_index[rhouse], nhouse);
			break;
		}
		house_index[rhouse] = house_index[rhouse - 1];
	}

	if (!found) {
		/* Make a "nofree" variant and remove these later. */
		house_index[rhouse].rooms = NULL;
		house_index[rhouse].cowners = NULL;
		house_index[rhouse].guests = NULL;

		copy_house(&house_index[0], nhouse);
	}

	return rhouse;
}

/*-------------------------------------------------------------------*/

int	save_houses(void)
{
	int house_nr, j;
	struct house_data *house;
	char *house_replace = "REPLACE INTO %s ("
		"vnum, "
		"atrium, "
		"exit_num, "
		"owner, "
		"built_on, "
		"last_used, "
		"prune_safe, "
		"mode, "
		"last_payment, "
		"cost, "
		"max_secure, "
		"max_locked) "
		"VALUES (%d, %d, %d, %d, %ld, %ld, %d, %d, %ld, %d, %d, %d);";
	char *room_insert = "INSERT INTO %s ("
		"vnum, "
		"room) "
		"VALUES (%d, %d);";
	char *player_insert = "INSERT INTO %s ("
		"vnum, "
		"player) "
		"VALUES (%d, %d);";

	for (house_nr = 0; house_nr <= top_of_houset; house_nr++) {

		if (real_house(HOUSE_NUM(house_nr)) != -1) {
	
			house = house_index + house_nr;
	
			/* Delete all the house's rooms from the database. */
			mysqlWrite("DELETE FROM %s WHERE vnum = %d;", TABLE_HOUSE_ROOMS, H_NUM(house));
			/* Delete all the house's co-owners from the database. */
			mysqlWrite("DELETE FROM %s WHERE vnum = %d;", TABLE_HOUSE_COWNERS, H_NUM(house));
			/* Delete all the house's guests from the database. */
			mysqlWrite("DELETE FROM %s WHERE vnum = %d;", TABLE_HOUSE_GUESTS, H_NUM(house));
	
			if (!(mysqlWrite(
				house_replace,
				TABLE_HOUSE_INDEX,
				H_NUM(house),
				H_ATRIUM(house),
				H_EXIT(house),
				H_OWNER(house),
				H_BUILT(house),
				H_USED(house),
				H_PRUNE_SAFE(house),
				H_MODE(house),
				H_PAID(house),
				H_COST(house),
				H_MAX_SECURE(house),
				H_MAX_LOCKED(house)
			))) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing house #%d to database.", H_NUM(house));
				return FALSE;
			}
	
			/*
			 * Save the rooms.
			 */
			for (j = 0; H_ROOM(house, j) != NOWHERE; j++) {
				if (!(mysqlWrite(room_insert, TABLE_HOUSE_ROOMS, H_NUM(house), H_ROOM(house, j)))) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing rooms for house #%d to database.", H_NUM(house));
					return FALSE;
				}
			}
	
			/*
			 * Save the co-owners.
			 */
			for (j = 0; H_COWNER(house, j) != NOBODY; j++) {
				if (!(mysqlWrite(player_insert, TABLE_HOUSE_COWNERS, H_NUM(house), H_COWNER(house, j)))) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing co-owners for house #%d to database.", H_NUM(house));
					return FALSE;
				}
			}
	
			/*
			 * Save the guests.
			 */
			for (j = 0; H_GUEST(house, j) != NOBODY; j++) {
				if (!(mysqlWrite(player_insert, TABLE_HOUSE_GUESTS, H_NUM(house), H_GUEST(house, j)))) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing guests for house #%d to database.", H_NUM(house));
					return FALSE;
				}
			}

		}

	}

	return TRUE;
}


void delete_house(struct descriptor_data *d, house_vnum vnum)
{
	int house_nr, j, count;
	room_rnum ratrium, rroom;

	if ((house_nr = real_house(vnum)) == NOWHERE) {
		write_to_output(d, TRUE, "Unknown house.\r\n");
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Tried to delete non-existant house!");
		return;
	}

	if ((ratrium = real_room(HOUSE_ATRIUM(house_nr))) == NOWHERE)
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "House %d had invalid atrium %d!", vnum, HOUSE_ATRIUM(house_nr));
	else
		REMOVE_BIT(ROOM_FLAGS(ratrium), ROOM_ATRIUM);

	for (count = 0; HOUSE_ROOM(house_nr, count) != NOWHERE; count++) {
		if ((rroom = real_room(HOUSE_ROOM(house_nr, count))) == NOWHERE)
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "House %d had invalid room %ld!", atoi(arg), HOUSE_ROOM(house_nr, count));
		else
			REMOVE_BIT(ROOM_FLAGS(rroom), ROOM_HOUSE | ROOM_PRIVATE | ROOM_HOUSE_CRASH);
		house_delete_file(HOUSE_NUM(house_nr));
	}

	extended_mudlog(NRM, SYSL_HOUSES, TRUE, "%s deleted house %d.", GET_NAME(d->character), HOUSE_NUM(house_nr));

	/* move the houses down in the list */
	for (j = house_nr; j < top_of_houset; j++)
		house_index[j] = house_index[j + 1];

	top_of_houset--;

	write_to_output(d, TRUE, "House deleted.\r\n");

	/*
	 * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums,
	 * just in case the house we just deleted shared an atrium with another
	 * house.  --JE 9/19/94
	 */
	for (house_nr = 0; house_nr < top_of_houset; house_nr++)
		if ((ratrium = real_room(HOUSE_ATRIUM(house_nr))) != NOWHERE)
			SET_BIT(ROOM_FLAGS(ratrium), ROOM_ATRIUM);
}
