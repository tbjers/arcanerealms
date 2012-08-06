/************************************************************************
 * OasisOLC - Houses / hedit.c                                     v2.0 *
 * Copyright 1996 Harvey Gilpin                                         *
 * Copyright 1997-1999 George Greer (greerga@circlemud.org)             *
 * Copyright 2002-2003 by Torgny Bjers (artovil@arcanerealms.org)       *
*************************************************************************/
/* $Id: hedit.c,v 1.9 2002/12/28 16:17:02 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "house.h"
#include "genolc.h"
#include "genhouse.h"
#include "oasis.h"
#include "constants.h"

/*-------------------------------------------------------------------*/

/*
 * External variable declarations.
 */
extern struct house_data *house_index;
extern int top_of_houset;

/*-------------------------------------------------------------------*/

/*
 * Should check more things.
 */
void hedit_save_internally(struct descriptor_data *d)
{
	OLC_HOUSE(d)->vnum = OLC_NUM(d);
	add_house(OLC_HOUSE(d));
}

void hedit_save_to_disk(void)
{
	save_houses();
}

/*-------------------------------------------------------------------*\
	utility functions 
\*-------------------------------------------------------------------*/

void hedit_setup_new(struct descriptor_data *d)
{
	struct house_data *house;

	/*
	 * Allocate a scratch house structure.
	 */
	CREATE(house, struct house_data, 1);

	/*
	 * Fill in some default values.
	 */
	H_OWNER(house) = NOBODY;
	H_ATRIUM(house) = NOWHERE;
	H_EXIT(house) = NOWHERE;
	H_MODE(house) = HOUSE_PRIVATE;
	H_PRUNE_SAFE(house) = FALSE;
	H_BUILT(house) = time(0);
	H_USED(house) = time(0);
	H_PAID(house) = 0;
	H_COST(house) = 0;
	H_MAX_SECURE(house) = 2;
	H_MAX_LOCKED(house) = 50;
	/*
	 * Stir the lists lightly.
	 */
	CREATE(H_ROOMS(house), long, 1);
	H_ROOM(house, 0) = NOWHERE;

	CREATE(H_COWNERS(house), long, 1);
	H_COWNER(house, 0) = NOBODY;

	CREATE(H_GUESTS(house), long, 1);
	H_GUEST(house, 0) = NOBODY;

	/*
	 * Presto! A house.
	 */
	OLC_HOUSE(d) = house;

	hedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void hedit_setup_existing(struct descriptor_data *d, int rhouse_num)
{
	/*
	 * Create a scratch house structure.
	 */
	CREATE(OLC_HOUSE(d), struct house_data, 1);

	copy_house(OLC_HOUSE(d), house_index + rhouse_num);
	hedit_disp_menu(d);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

void hedit_exits_menu(struct descriptor_data *d)
{
	struct house_data *house;
	int i;

	house = OLC_HOUSE(d);
	get_char_colors(d->character);

	clear_screen(d);
	for (i = 0; *dirs[i] != '\n'; i++)
		write_to_output(d, TRUE, "%s%2d%s) %s%s%s\r\n", grn, i, nrm, yel, dirs[i], nrm);
	write_to_output(d, TRUE, "\r\nEnter Exit number : ");
}

/*-------------------------------------------------------------------*/

void hedit_modes_menu(struct descriptor_data *d)
{
	struct house_data *house;
	int i;

	house = OLC_HOUSE(d);
	get_char_colors(d->character);

	clear_screen(d);
	for (i = 0; *house_types[i] != '\n'; i++)
		write_to_output(d, TRUE, "%s%2d%s) %s%s%s\r\n", grn, i, nrm, cyn, house_types[i], nrm);
	write_to_output(d, TRUE, "\r\nEnter House Type : ");
}

/*-------------------------------------------------------------------*/

void hedit_prune_safe_menu(struct descriptor_data *d)
{
	struct house_data *house;
	int i;

	house = OLC_HOUSE(d);
	get_char_colors(d->character);

	clear_screen(d);
	for (i = 0; i < 2; i++)
		write_to_output(d, TRUE, "%s%2d%s) %s%s%s\r\n", grn, i, nrm, cyn, YESNO(i), nrm);
	write_to_output(d, TRUE, "\r\nShould the house be prune-safe? : ");
}

/*-------------------------------------------------------------------*/

void hedit_compact_rooms_menu(struct descriptor_data *d)
{
	struct house_data *house;
	int i, count = 0;

	house = OLC_HOUSE(d);
	get_char_colors(d->character);

	clear_screen(d);
	for (i = 0; H_ROOM(house, i) != -1; i++)
		write_to_output(d, TRUE, "%2d - [%s%5d%s]  | %s", i, cyn, H_ROOM(house, i), nrm,
			!(++count % 5) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new room.\r\n"
		"%sD%s) Delete a room.\r\n"
		"%sL%s) Long display.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = HEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void hedit_rooms_menu(struct descriptor_data *d)
{
	struct house_data *house;
	int i;

	house = OLC_HOUSE(d);
	get_char_colors(d->character);

	clear_screen(d);
	write_to_output(d, TRUE, "##     VNUM     Room\r\n\r\n");
	for (i = 0; H_ROOM(house, i) != -1; i++)
		write_to_output(d, TRUE, "%2d - [%s%5d%s] - %s%s%s\r\n", i, cyn, H_ROOM(house, i), nrm,
			yel, world[real_room(H_ROOM(house, i))].name, nrm);
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new room.\r\n"
		"%sD%s) Delete a room.\r\n"
		"%sC%s) Compact Display.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = HEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void hedit_owners_menu(struct descriptor_data *d)
{
	struct house_data *house;
	int i;

	house = OLC_HOUSE(d);
	get_char_colors(d->character);

	clear_screen(d);
	write_to_output(d, TRUE, "##     ID       Player\r\n\r\n");
	for (i = 0; H_COWNER(house, i) != NOBODY; i++)
		write_to_output(d, TRUE, "%2d - [%s%5d%s] - %s%s%s\r\n", i, cyn, H_COWNER(house, i), nrm,
			yel, *get_name_by_id(H_COWNER(house, i)) == NOBODY ? "<Deleted>" : get_name_by_id(H_COWNER(house, i)), nrm);
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new co-owner.\r\n"
		"%sD%s) Delete a co-owner.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = HEDIT_COWNERS_MENU;
}

/*-------------------------------------------------------------------*/

void hedit_guests_menu(struct descriptor_data *d)
{
	struct house_data *house;
	int i;

	house = OLC_HOUSE(d);
	get_char_colors(d->character);

	clear_screen(d);
	write_to_output(d, TRUE, "##     ID       Player\r\n\r\n");
	for (i = 0; H_GUEST(house, i) != NOBODY; i++)
		write_to_output(d, TRUE, "%2d - [%s%5d%s] - %s%s%s\r\n", i, cyn, H_GUEST(house, i), nrm,
			yel, *get_name_by_id(H_GUEST(house, i)) == NOBODY ? "<Deleted>" : get_name_by_id(H_GUEST(house, i)), nrm);
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new guest.\r\n"
		"%sD%s) Delete a guest.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = HEDIT_GUESTS_MENU;
}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void hedit_disp_menu(struct descriptor_data *d)
{
	struct house_data *house;
	char *name;

	house = OLC_HOUSE(d);
	get_char_colors(d->character);

	if ((name = get_name_by_id(H_OWNER(house))) == NULL)
		name = str_dup("nobody");
	else
		name = str_dup(get_name_by_id(H_OWNER(house)));

	clear_screen(d);
	write_to_output(d, TRUE,
		"-- House Number : [%s%d%s]\r\n"
		"%s0%s) Owner                 : %s%s\r\n"
		"%s1%s) Atrium                : [%s%d%s] %s%s\r\n"
		"%s2%s) Exit                  : [%s%d%s] %s%s\r\n"
		"%s3%s) House type            : %s%s\r\n"
		"%s4%s) Prune-safe            : %s%s\r\n"
		"%s5%s) Cost                  : [%s%d%s]\r\n"
		"%s6%s) Max Secure containers : [%s%d%s]\r\n"
		"%s7%s) Max Locked-down objs  : [%s%d%s]\r\n"
		"%sR%s) Rooms Menu\r\n"
		"%sC%s) Co-Owners Menu\r\n"
		"%sG%s) Guests Menu\r\n"
		"%sD%s) Delete this house\r\n"
		"%sQ%s) Quit\r\n"
		"Enter Choice : ",
		cyn, OLC_NUM(d), nrm,
		grn, nrm, yel, CAP(name),
		grn, nrm, cyn, real_room(H_ATRIUM(house)) == NOWHERE ? -1 : world[real_room(H_ATRIUM(house))].number,
		nrm, yel, real_room(H_ATRIUM(house)) == NOWHERE ? "Nowhere" : world[real_room(H_ATRIUM(house))].name,
		grn, nrm, cyn, H_EXIT(house) < 0 || H_EXIT(house) >= NUM_OF_DIRS ? -1 : H_EXIT(house),
		nrm, yel, H_EXIT(house) < 0 || H_EXIT(house) >= NUM_OF_DIRS ? "Nowhere" : dirs[H_EXIT(house)],
		grn, nrm, cyn, house_types[H_MODE(house)],
		grn, nrm, cyn, YESNO(H_PRUNE_SAFE(house)),
		grn, nrm, cyn, H_COST(house), nrm,
		grn, nrm, cyn, H_MAX_SECURE(house), nrm,
		grn, nrm, cyn, H_MAX_LOCKED(house), nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm
	);

	release_buffer(name);

	OLC_MODE(d) = HEDIT_MAIN_MENU;
}

/**************************************************************************
	The GARGANTUAN event handler
 **************************************************************************/

void hedit_parse(struct descriptor_data *d, char *arg)
{
	int i, j;

	if (OLC_MODE(d) > HEDIT_NUMERICAL_RESPONSE) {
		if (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))) {
			write_to_output(d, TRUE, "Field must be numerical, try again : ");
			return;
		}
	}
	switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
	case SEDIT_CONFIRM_SAVESTRING:
		switch (*arg) {
		case 'y':
		case 'Y':
			write_to_output(d, TRUE, "Saving house to memory.\r\n");
			hedit_save_internally(d);
			hedit_save_to_disk();
			extended_mudlog(BRF, SYSL_OLC, TRUE, "%s edits house %d.", GET_NAME(d->character), OLC_NUM(d));
			cleanup_olc(d, CLEANUP_STRUCTS);
			return;
		case 'n':
		case 'N':
			cleanup_olc(d, CLEANUP_ALL);
			return;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\nDo you wish to save the house? : ");
			return;
		}
		break;

/*-------------------------------------------------------------------*/
	case HEDIT_MAIN_MENU:
		i = 0;
		switch (*arg) {
		case 'q':
		case 'Q':
			if (OLC_VAL(d)) {		/* Anything been changed? */
				write_to_output(d, TRUE, "Do you wish to save the changes to the house? (y/n) : ");
				OLC_MODE(d) = HEDIT_CONFIRM_SAVESTRING;
			} else
				cleanup_olc(d, CLEANUP_ALL);
			return;
		case '0':
			OLC_MODE(d) = HEDIT_OWNER;
			write_to_output(d, TRUE, "Enter the name of the house owner : ");
			i--;
			return;
		case '1':
			OLC_MODE(d) = HEDIT_ATRIUM;
			write_to_output(d, TRUE, "Enter the room number of the atrium : ");
			i++;
			return;
		case '2':
			OLC_MODE(d) = HEDIT_EXIT;
			hedit_exits_menu(d);
			return;
		case '3':
			OLC_MODE(d) = HEDIT_MODE;
			hedit_modes_menu(d);
			return;
		case '4':
			OLC_MODE(d) = HEDIT_PRUNE_SAFE;
			hedit_prune_safe_menu(d);
			return;
		case '5':
			OLC_MODE(d) = HEDIT_COST;
			write_to_output(d, TRUE, "Enter the house cost : ");
			i++;
			return;
		case '6':
			OLC_MODE(d) = HEDIT_MAX_SECURE;
			write_to_output(d, TRUE, "Enter maximum number of secure containers : ");
			i++;
			return;
		case '7':
			OLC_MODE(d) = HEDIT_MAX_LOCKED;
			write_to_output(d, TRUE, "Enter maximum number of locked-down items : ");
			i++;
			return;
		case 'c':
		case 'C':
			hedit_owners_menu(d);
			return;
		case 'g':
		case 'G':
			hedit_guests_menu(d);
			return;
		case 'r':
		case 'R':
			hedit_rooms_menu(d);
			return;
		case 'd':
		case 'D':
			delete_house(d, OLC_NUM(d));
			cleanup_olc(d, CLEANUP_ALL);
			return;
		default:
			hedit_disp_menu(d);
			return;
		}

		if (i == 0)
			break;
		else if (i == 1)
			write_to_output(d, TRUE, "\r\nEnter new value : ");
		else if (i == -1)
			write_to_output(d, TRUE, "\r\nEnter new text :\r\n] ");
		else
			write_to_output(d, TRUE, "Oops...\r\n");
		return;
/*-------------------------------------------------------------------*/
	case HEDIT_ROOMS_MENU:
		switch (*arg) {
		case 'a':
		case 'A':
			write_to_output(d, TRUE, "\r\nEnter new room vnum number : ");
			OLC_MODE(d) = HEDIT_NEW_ROOM;
			return;
		case 'c':
		case 'C':
			hedit_compact_rooms_menu(d);
			return;
		case 'l':
		case 'L':
			hedit_rooms_menu(d);
			return;
		case 'd':
		case 'D':
			write_to_output(d, TRUE, "\r\nDelete which room? : ");
			OLC_MODE(d) = HEDIT_DELETE_ROOM;
			return;
		case 'q':
		case 'Q':
			break;
		}
		break;
/*-------------------------------------------------------------------*/
	case HEDIT_COWNERS_MENU:
		switch (*arg) {
		case 'a':
		case 'A':
			write_to_output(d, TRUE, "\r\nEnter player name : ");
			OLC_MODE(d) = HEDIT_NEW_COWNER;
			return;
		case 'l':
		case 'L':
			hedit_owners_menu(d);
			return;
		case 'd':
		case 'D':
			write_to_output(d, TRUE, "\r\nDelete which co-owner? : ");
			OLC_MODE(d) = HEDIT_DELETE_COWNER;
			return;
		case 'q':
		case 'Q':
			break;
		}
		break;
/*-------------------------------------------------------------------*/
	case HEDIT_GUESTS_MENU:
		switch (*arg) {
		case 'a':
		case 'A':
			write_to_output(d, TRUE, "\r\nEnter player name : ");
			OLC_MODE(d) = HEDIT_NEW_GUEST;
			return;
		case 'l':
		case 'L':
			hedit_guests_menu(d);
			return;
		case 'd':
		case 'D':
			write_to_output(d, TRUE, "\r\nDelete which guest? : ");
			OLC_MODE(d) = HEDIT_DELETE_GUEST;
			return;
		case 'q':
		case 'Q':
			break;
		}
		break;
/*-------------------------------------------------------------------*/
		/*
		 * Numerical responses.
		 */
/*-------------------------------------------------------------------*/
	case HEDIT_OWNER:
		if ((i = get_id_by_name(arg)) == NOBODY) {
			write_to_output(d, TRUE, "No such player, try again : ");
			return;
		}
		H_OWNER(OLC_HOUSE(d)) = i;
		break;
/*-------------------------------------------------------------------*/
	case HEDIT_ATRIUM:
		if ((i = real_room(atoi(arg))) == NOWHERE) {
			write_to_output(d, TRUE, "No such room, try again : ");
			return;
		}
		if ((find_house(atoi(arg))) != NOWHERE) {
			write_to_output(d, TRUE, "That's a house room, try again : ");
			return;
		}
		H_ATRIUM(OLC_HOUSE(d)) = atoi(arg);
		break;
/*-------------------------------------------------------------------*/
	case HEDIT_EXIT:
		/* Check for a valid exit. */
		if (atoi(arg) < 0 || atoi(arg) >= NUM_OF_DIRS) {
			write_to_output(d, TRUE, "\r\n&RThat's not a valid exit.&n\r\n\r\n");
			hedit_exits_menu(d);
			return;
		}
		/* Make sure we have a start room (first in list). */
		if ((i = real_room(H_ROOM(OLC_HOUSE(d), 0))) == NOWHERE) {
			write_to_output(d, TRUE, "\r\n&RYou have to define the starting room first.&n\r\n\r\n");
			break;
		}
		/* Make sure we have an atrium. */
		if ((j = real_room(H_ATRIUM(OLC_HOUSE(d)))) == NOWHERE) {
			write_to_output(d, TRUE, "\r\n&RYou have to define the atrium room first.&n\r\n\r\n");
			break;
		}
		/* Check for a valid exit leading out from start room. */
		if (TOROOM(i, atoi(arg)) == NOWHERE) {
			write_to_output(d, TRUE, "\r\n&RThere is no exit %s from room %d.&n\r\n\r\n", dirs[atoi(arg)], H_ROOM(OLC_HOUSE(d), 0));
			hedit_exits_menu(d);
			return;
		}
		/* Check for a valid return exit from the atrium. */
		if (TOROOM(j, rev_dir[atoi(arg)]) != i) {
			write_to_output(d, TRUE, "\r\n&RThe returning exit %s does not come from the atrium %d.&n\r\n\r\n", dirs[rev_dir[atoi(arg)]], H_ATRIUM(OLC_HOUSE(d)));
			hedit_exits_menu(d);
			return;
		}
		H_EXIT(OLC_HOUSE(d)) = atoi(arg);
		break;
/*-------------------------------------------------------------------*/
	case HEDIT_MODE:
		i = atoi(arg);
		if (i < 0 || i >= NUM_HOUSE_FLAGS) {
			write_to_output(d, TRUE, "\r\n&RThat's not a valid house type.&n\r\n\r\n");
			hedit_modes_menu(d);
			return;
		}
		H_MODE(OLC_HOUSE(d)) = i;
		break;
/*-------------------------------------------------------------------*/
	case HEDIT_PRUNE_SAFE:
		i = atoi(arg);
		if (i < 0 || i > 1) {
			write_to_output(d, TRUE, "\r\n&RValid choices are 0 or 1.&n\r\n\r\n");
			hedit_prune_safe_menu(d);
			return;
		}
		H_PRUNE_SAFE(OLC_HOUSE(d)) = i;
		break;
/*-------------------------------------------------------------------*/
	case HEDIT_COST:
		H_COST(OLC_HOUSE(d)) = atoi(arg);
		break;
/*-------------------------------------------------------------------*/
	case HEDIT_MAX_SECURE:
		if ((i = atoi(arg)) > HOUSE_MAX_SECURES) {
			write_to_output(d, TRUE, "Valid ranges are 0-%d : ", HOUSE_MAX_SECURES);
			return;
		}
		H_MAX_SECURE(OLC_HOUSE(d)) = i;
		break;
/*-------------------------------------------------------------------*/
	case HEDIT_MAX_LOCKED:
		if ((i = atoi(arg)) > HOUSE_MAX_LOCKS) {
			write_to_output(d, TRUE, "Valid ranges are 0-%d : ", HOUSE_MAX_LOCKS);
			return;
		}
		H_MAX_LOCKED(OLC_HOUSE(d)) = i;
		break;
/*-------------------------------------------------------------------*/
	case HEDIT_NEW_ROOM:
		if ((i = atoi(arg)) != -1)
			if ((i = real_room(i)) < 0) {
				write_to_output(d, TRUE, "That room does not exist, try again : ");
				return;
			}
		if (i >= 0)
			add_to_house_list(&(H_ROOMS(OLC_HOUSE(d))), atoi(arg));
		hedit_rooms_menu(d);
		return;
	case HEDIT_DELETE_ROOM:
		remove_from_house_list(&(H_ROOMS(OLC_HOUSE(d))), atoi(arg));
		hedit_rooms_menu(d);
		return;

/*-------------------------------------------------------------------*/
	case HEDIT_NEW_COWNER:
		if ((i = get_id_by_name(arg)) == NOBODY) {
			write_to_output(d, TRUE, "No such player, try again : ");
			return;
		}
		if (i == H_OWNER(OLC_HOUSE(d))) {
			write_to_output(d, TRUE, "That player already owns the house, try again : ");
			return;
		}
		for (j = 0; H_COWNER(OLC_HOUSE(d), j) != NOBODY; j++) {
			if (i == H_COWNER(OLC_HOUSE(d), j)) {
				write_to_output(d, TRUE, "That player has already been added, try again : ");
				return;
			}
		}
		for (j = 0; H_GUEST(OLC_HOUSE(d), j) != NOBODY; j++) {
			if (i == H_GUEST(OLC_HOUSE(d), j)) {
				write_to_output(d, TRUE, "That player is listed as a guest, try again : ");
				return;
			}
		}
		if (i >= 0)
			add_to_house_list(&(H_COWNERS(OLC_HOUSE(d))), i);
		hedit_owners_menu(d);
		return;
	case HEDIT_DELETE_COWNER:
		remove_from_house_list(&(H_COWNERS(OLC_HOUSE(d))), atoi(arg));
		hedit_owners_menu(d);
		return;
/*-------------------------------------------------------------------*/
	case HEDIT_NEW_GUEST:
		if ((i = get_id_by_name(arg)) == NOBODY) {
			write_to_output(d, TRUE, "No such player, try again : ");
			return;
		}
		if (i == H_OWNER(OLC_HOUSE(d))) {
			write_to_output(d, TRUE, "That player already owns the house, try again : ");
			return;
		}
		for (j = 0; H_COWNER(OLC_HOUSE(d), j) != NOBODY; j++) {
			if (i == H_COWNER(OLC_HOUSE(d), j)) {
				write_to_output(d, TRUE, "That player is a co-owner, try again : ");
				return;
			}
		}
		for (j = 0; H_GUEST(OLC_HOUSE(d), j) != NOBODY; j++) {
			if (i == H_GUEST(OLC_HOUSE(d), j)) {
				write_to_output(d, TRUE, "That player has already been added, try again : ");
				return;
			}
		}
		if (i >= 0)
			add_to_house_list(&(H_GUESTS(OLC_HOUSE(d))), i);
		hedit_guests_menu(d);
		return;
	case HEDIT_DELETE_GUEST:
		remove_from_house_list(&(H_GUESTS(OLC_HOUSE(d))), atoi(arg));
		hedit_guests_menu(d);
		return;
/*-------------------------------------------------------------------*/
	default:
		/*
		 * We should never get here.
		 */
		cleanup_olc(d, CLEANUP_ALL);
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: hedit_parse(): Reached default case!");
		write_to_output(d, TRUE, "Oops...\r\n");
		break;
	}
/*-------------------------------------------------------------------*/

/*
 * END OF CASE 
 * If we get here, we have probably changed something, and now want to
 * return to main menu.  Use OLC_VAL as a 'has changed' flag.
 */
	OLC_VAL(d) = 1;
	hedit_disp_menu(d);
}
