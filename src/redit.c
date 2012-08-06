/************************************************************************
 *  OasisOLC - Rooms / redit.c  v2.0                                    *
 *  Original author: Levork                                             *
 *  Copyright 1996 Harvey Gilpin                                        *
 *  Copyright 1997-1999 George Greer (greerga@circlemud.org)            *
 ************************************************************************/
/* $Id: redit.c,v 1.30 2003/03/17 01:46:50 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "boards.h"
#include "genolc.h"
#include "genwld.h"
#include "oasis.h"
#include "dg_olc.h"
#include "specset.h"
#include "constants.h"

/*------------------------------------------------------------------------*/

/*
 * External data structures.
 */
extern struct room_data *world;
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct zone_data *zone_table;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern struct descriptor_data *descriptor_list;

/*------------------------------------------------------------------------*/


/*------------------------------------------------------------------------*\
	Utils and exported functions.
\*------------------------------------------------------------------------*/

void redit_setup_new(struct descriptor_data *d)
{
	CREATE(OLC_ROOM(d), struct room_data, 1);

	OLC_ROOM(d)->name = str_dup("An unfinished room");
	OLC_ROOM(d)->description = str_dup("You are in an unfinished room.\r\n");
	OLC_ROOM(d)->number = NOWHERE;
	OLC_ROOM(d)->magic_flux = 2;
	OLC_ROOM(d)->sector_type = 1;
	OLC_ITEM_TYPE(d) = WLD_TRIGGER;
	redit_disp_menu(d);
	OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void redit_setup_existing(struct descriptor_data *d, int real_num)
{
	struct room_data *room;
	int counter;
	struct trig_proto_list *proto, *fproto;

	/*
	 * Build a copy of the room for editing.
	 */
	CREATE(room, struct room_data, 1);

	*room = world[real_num];
	/*
	 * Allocate space for all strings.
	 */
	room->name = str_udup(world[real_num].name);
	room->description = str_udup(world[real_num].description);
	room->description = str_udup(world[real_num].description);

	/*
	 * Exits - We allocate only if necessary.
	 */
	for (counter = 0; counter < NUM_OF_DIRS; counter++) {
		if (world[real_num].dir_option[counter]) {
			CREATE(room->dir_option[counter], struct room_direction_data, 1);

			/*
			 * Copy the numbers over.
			 */
			*room->dir_option[counter] = *world[real_num].dir_option[counter];
			/*
			 * Allocate the strings.
			 */
			if (world[real_num].dir_option[counter]->general_description)
				room->dir_option[counter]->general_description = str_dup(world[real_num].dir_option[counter]->general_description);
			if (world[real_num].dir_option[counter]->keyword)
				room->dir_option[counter]->keyword = str_dup(world[real_num].dir_option[counter]->keyword);
		}
	}

	/*
	 * Extra descriptions, if necessary.
	 */
	if (world[real_num].ex_description) {
		struct extra_descr_data *tdesc, *temp, *temp2;
		CREATE(temp, struct extra_descr_data, 1);

		room->ex_description = temp;
		for (tdesc = world[real_num].ex_description; tdesc; tdesc = tdesc->next) {
			temp->keyword = str_dup(tdesc->keyword);
			temp->description = str_dup(tdesc->description);
			if (tdesc->next) {
				CREATE(temp2, struct extra_descr_data, 1);
				temp->next = temp2;
				temp = temp2;
			} else
				temp->next = NULL;
		}
	}

	if (SCRIPT(&world[real_num]))
		script_copy(room, &world[real_num], WLD_TRIGGER);

	proto = world[real_num].proto_script;
	while (proto) {
		CREATE(fproto, struct trig_proto_list, 1);
		fproto->vnum = proto->vnum;
		if (room->proto_script==NULL)
			room->proto_script = fproto;
		proto = proto->next;
		fproto = fproto->next; /* NULL */
	}

	/*
	 * Attach copy of room to player's descriptor.
	 */
	OLC_ROOM(d) = room;
	OLC_VAL(d) = 0;
	OLC_ITEM_TYPE(d) = WLD_TRIGGER;
	dg_olc_script_copy(d);
	redit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void redit_save_internally(struct descriptor_data *d)
{
	int j, room_num, new_room = FALSE;
	struct descriptor_data *dsc;

	if (OLC_ROOM(d)->number == NOWHERE) {
		new_room = TRUE;
		OLC_ROOM(d)->number = OLC_NUM(d);
	}
	/* FIXME: Why is this not set elsewhere? */
	OLC_ROOM(d)->zone = OLC_ZNUM(d);

	if ((room_num = add_room(OLC_ROOM(d))) == NOWHERE) {
		write_to_output(d, TRUE, "Something went wrong...\r\n");
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "redit_save_internally: Something failed! (%d)", room_num);
		return;
	}

	/* Update script info for this room */
	/* Free old proto list */
	if (world[room_num].proto_script &&
			world[room_num].proto_script != OLC_SCRIPT(d)) {
		struct trig_proto_list *proto, *fproto;
		proto = world[room_num].proto_script;
		while (proto) {
			fproto = proto;
			if (proto->next)
				proto = proto->next;
			else
				break;
			free(fproto);
		}
	}    
			 
	world[room_num].proto_script = OLC_SCRIPT(d);
	assign_triggers(&world[room_num], WLD_TRIGGER);
  
	/* Don't adjust numbers on a room update. */
	if (!new_room)
		return;

	/* Idea contributed by C.Raehl 4/27/99 */
	for (dsc = descriptor_list; dsc; dsc = dsc->next) {
		if (dsc == d)
			continue;

		if (STATE(dsc) == CON_ZEDIT) {
			for (j = 0; OLC_ZONE(dsc)->cmd[j].command != 'S'; j++)
				switch (OLC_ZONE(dsc)->cmd[j].command) {
					case 'O':
					case 'M':
					case 'T':
					case 'V':
						OLC_ZONE(dsc)->cmd[j].arg3 += (OLC_ZONE(dsc)->cmd[j].arg3 >= room_num);
						break;
					case 'D':
						OLC_ZONE(dsc)->cmd[j].arg2 += (OLC_ZONE(dsc)->cmd[j].arg2 >= room_num);
						/* Fall through */
					case 'R':
						OLC_ZONE(dsc)->cmd[j].arg1 += (OLC_ZONE(dsc)->cmd[j].arg1 >= room_num);
						break;
					}
		} else if (STATE(dsc) == CON_REDIT) {
			for (j = 0; j < NUM_OF_DIRS; j++)
				if (OLC_ROOM(dsc)->dir_option[j])
					if (OLC_ROOM(dsc)->dir_option[j]->to_room >= room_num)
						OLC_ROOM(dsc)->dir_option[j]->to_room++;
		}
	}
}

/*------------------------------------------------------------------------*/

void redit_save_to_disk(zone_vnum zone_num)
{
	save_rooms(zone_num);                /* :) */
}

/*------------------------------------------------------------------------*/

void free_room(struct room_data *room)
{
	int i;
	struct extra_descr_data *tdesc, *next;

	if (room->name)
		free(room->name);
	if (room->description)
		free(room->description);

	/*
	 * Free exits.
	 */
	for (i = 0; i < NUM_OF_DIRS; i++) {
		if (room->dir_option[i]) {
			if (room->dir_option[i]->general_description)
				free(room->dir_option[i]->general_description);
			if (room->dir_option[i]->keyword)
				free(room->dir_option[i]->keyword);
			free(room->dir_option[i]);
		}
	}

	/*
	 * Free extra descriptions.
	 */
	for (tdesc = room->ex_description; tdesc; tdesc = next) {
		next = tdesc->next;
		if (tdesc->keyword)
			free(tdesc->keyword);
		if (tdesc->description)
			free(tdesc->description);
		free(tdesc);
	}
	free(room);        /* XXX ? */
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * Display a list of Special Procedures
 */
void redit_disp_specproc(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i < NUM_ROOM_SPECS; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s  %s", grn, i, nrm, room_specproc_info[i].name,
			!(++columns % 2) ? "\r\n" : "");

	write_to_output(d, TRUE, "\r\nEnter Spec Proc number : ");
}

/*
 * For extra descriptions.
 */
void redit_disp_extradesc_menu(struct descriptor_data *d)
{
	struct extra_descr_data *extra_desc = OLC_DESC(d);

	clear_screen(d);
	sprintf(buf,
					"%s1%s) Keyword: %s%s\r\n"
					"%s2%s) Description:\r\n%s\\c98%s\\c99\r\n"
					"%s3%s) Goto next description: ",

					grn, nrm, yel, extra_desc->keyword ? extra_desc->keyword : "<NONE>",
					grn, nrm, yel, extra_desc->description ? extra_desc->description : "<NONE>",
					grn, nrm
					);

	strcat(buf, !extra_desc->next ? "<NOT SET>\r\n" : "Set.\r\n");
	strcat(buf, "Enter choice (0 to quit) : ");
	write_to_output(d, TRUE, "%s", buf);
	OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}

/*
 * For exits.
 */
void redit_disp_exit_menu(struct descriptor_data *d)
{
	/*
	 * if exit doesn't exist, alloc/create it 
	 */
	if (OLC_EXIT(d) == NULL)
		CREATE(OLC_EXIT(d), struct room_direction_data, 1);

	/*
	 * Weird door handling! 
	 */
	if (IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR)) {
		if (IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF))
			strcpy(buf2, "Pickproof");
		else
			strcpy(buf2, "Is a door");
	}
	else if (IS_SET(OLC_EXIT(d)->exit_info, EX_FAERIE))
			strcpy(buf2, "Faerie Entrance");
	else if (IS_SET(OLC_EXIT(d)->exit_info, EX_INFERNAL))
			strcpy(buf2, "Infernal Entrance");
	else if (IS_SET(OLC_EXIT(d)->exit_info, EX_DIVINE))
			strcpy(buf2, "Divine Entrance");
	else if (IS_SET(OLC_EXIT(d)->exit_info, EX_ANCIENT))
			strcpy(buf2, "Ancient Entrance");
	else
			strcpy(buf2, "No door");
	
	get_char_colors(d->character);
	clear_screen(d);
	write_to_output(d, TRUE,
					"%s1%s) Exit to     : %s%d\r\n"
					"%s2%s) Description :-\r\n%s%s\r\n"
					"%s3%s) Door name   : %s%s\r\n"
					"%s4%s) Key         : %s%d\r\n"
					"%s5%s) Door flags  : %s%s\r\n"
					"%s6%s) Purge exit.\r\n"
					"Enter choice, 0 to quit : ",

					grn, nrm, cyn, OLC_EXIT(d)->to_room != -1 ? world[OLC_EXIT(d)->to_room].number : -1,
					grn, nrm, yel, OLC_EXIT(d)->general_description ? OLC_EXIT(d)->general_description : "<NONE>",
					grn, nrm, yel, OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>",
					grn, nrm, cyn, OLC_EXIT(d)->key,
					grn, nrm, cyn, buf2, grn, nrm
					);

	OLC_MODE(d) = REDIT_EXIT_MENU;
}

/*
 * For exit flags.
 */
void redit_disp_exit_flag_menu(struct descriptor_data *d)
{
	get_char_colors(d->character);
	write_to_output(d, TRUE,  "%s0%s) No door\r\n"
								"%s1%s) Closeable door\r\n"
								"%s2%s) Pickproof\r\n"
								"%s3%s) Faerie entrance\r\n"
								"%s4%s) Infernal entrance\r\n"
								"%s5%s) Divine entrance\r\n"
								"%s6%s) Ancient entrance\r\n"
								"Enter choice : ", grn, nrm, grn, nrm, grn, nrm,
																	 grn, nrm, grn, nrm, grn, nrm, grn, nrm);
}

/*
 * For room flags.
 */
void redit_disp_flag_menu(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (counter = 0; counter < NUM_ROOM_FLAGS; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
								room_bits[counter], !(++columns % 2) ? "\r\n" : "");
	sprintbit(OLC_ROOM(d)->room_flags, room_bits, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nRoom flags: %s%s%s\r\n"
					"Enter room flags, 0 to quit : ", cyn, buf1, nrm);
	OLC_MODE(d) = REDIT_FLAGS;
}

/*
 * For sector type.
 */
void redit_disp_sector_menu(struct descriptor_data *d)
{
	int counter, columns = 0;

	clear_screen(d);
	for (counter = 0; counter < NUM_ROOM_SECTORS; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
								sector_types[counter], !(++columns % 2) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\nEnter sector type : ");
	OLC_MODE(d) = REDIT_SECTOR;
}

/*
 * For magic flux type.
 */
void redit_disp_flux_menu(struct descriptor_data *d)
{
	int counter, columns = 0;

	clear_screen(d);
	for (counter = 0; counter < NUM_FLUX_TYPES; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
								magic_flux_types[counter], !(++columns % 2) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\nEnter flux type : ");
	OLC_MODE(d) = REDIT_FLUX;
}

/*
 * For resource menu.
 */
void redit_disp_resources_menu(struct descriptor_data *d)
{	
	struct room_data *room;

	get_char_colors(d->character);
	clear_screen(d);
	room = OLC_ROOM(d);

	sprintbit((long)room->resources.resources, resource_bits, buf2, sizeof(buf2));

	write_to_output(d, TRUE,
					"%s1%s) Resources : %s%s\r\n"
					"%s2%s) Ore       : %s%d\r\n"
					"%s3%s) Gems      : %s%d\r\n"
					"%s4%s) Wood      : %s%d\r\n"
					"%s5%s) Stone     : %s%d\r\n"
					"%s6%s) Fabric    : %s%d\r\n"
					"%s7%s) Reset resources.\r\n"
					"Enter choice, 0 to quit : ",

					grn, nrm, cyn, buf2,
					grn, nrm, yel, room->resources.max[RESOURCE_ORE],
					grn, nrm, yel, room->resources.max[RESOURCE_GEMS],
					grn, nrm, yel, room->resources.max[RESOURCE_WOOD],
					grn, nrm, yel, room->resources.max[RESOURCE_STONE],
					grn, nrm, yel, room->resources.max[RESOURCE_FABRIC],
					grn, nrm
					);

	OLC_MODE(d) = REDIT_RESOURCES_MENU;
}

/*
 * For resource flags.
 */
void redit_disp_resource_flag_menu(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (counter = 0; counter < NUM_RESOURCES; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
								resource_bits[counter], !(++columns % 2) ? "\r\n" : "");
	sprintbit(OLC_ROOM(d)->resources.resources, resource_bits, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nResources: %s%s%s\r\n"
					"Enter resources, 0 to quit : ", cyn, buf1, nrm);
	OLC_MODE(d) = REDIT_RESOURCE_FLAGS;
}

/*
 * The main menu.
 */
void redit_disp_menu(struct descriptor_data *d)
{
	char *printbuf1 = get_buffer(MAX_INPUT_LENGTH);
	char *printbuf2 = get_buffer(MAX_INPUT_LENGTH);
	char *printbuf3 = get_buffer(MAX_INPUT_LENGTH);
	struct room_data *room;

	get_char_colors(d->character);
	clear_screen(d);
	room = OLC_ROOM(d);

	sprintbit((long)room->room_flags, room_bits, printbuf1, MAX_INPUT_LENGTH);
	sprinttype(room->sector_type, sector_types, printbuf2, MAX_INPUT_LENGTH);
	sprinttype(room->magic_flux, magic_flux_types, printbuf3, MAX_INPUT_LENGTH);
	write_to_output(d, TRUE,
#if	defined(CLEAR_SCREEN)
		"^[[H^[[J"
#endif
		"-- Room number : [%s%d%s]    Room zone: [%s%d%s]\r\n"
		"%s1%s) Name           : %s\\c98%s\\c99\r\n"
		"%s2%s) Description    :\r\n%s\\c98%s\\c99"
		"%s3%s) Room flags     : %s%s\r\n"
		"%s4%s) Sector type    : %s%s\r\n"
		"%s5%s) Exit north     : %s%6d  %sB%s) Exit northeast : %s%d\r\n"
		"%s6%s) Exit east      : %s%6d  %sC%s) Exit southwest : %s%d\r\n"
		"%s7%s) Exit south     : %s%6d  %sD%s) Exit northwest : %s%d\r\n"
		"%s8%s) Exit west      : %s%6d  %sE%s) Exit southeast : %s%d\r\n"
		"%s9%s) Exit up        : %s%6d  %sF%s) Exit in        : %s%d\r\n"
		"%sA%s) Exit down      : %s%6d  %sG%s) Exit out       : %s%d\r\n"
		"%sH%s) Extra descriptions menu\r\n"
		"%sM%s) Magical Flux   : %s%s\r\n"
#ifdef DG_SCRIPT_VERSION
		"%sS%s) Script         : %s%s\r\n"
#endif
		"%sR%s) Edit Resources\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ",
		cyn, OLC_NUM(d), nrm,
		cyn, zone_table[OLC_ZNUM(d)].number, nrm,
		grn, nrm, yel, room->name,
		grn, nrm, grn, room->description,		
		grn, nrm, cyn, printbuf1,
		grn, nrm, cyn, printbuf2,
		grn, nrm, cyn,
		room->dir_option[NORTH] && room->dir_option[NORTH]->to_room != -1 ?
		world[room->dir_option[NORTH]->to_room].number : -1,
		grn, nrm, cyn,
		room->dir_option[NORTHEAST] && room->dir_option[NORTHEAST]->to_room != -1 ?
		world[room->dir_option[NORTHEAST]->to_room].number : -1,
		grn, nrm, cyn,
		room->dir_option[EAST] && room->dir_option[EAST]->to_room != -1 ?
		world[room->dir_option[EAST]->to_room].number : -1,
		grn, nrm, cyn,
		room->dir_option[SOUTHWEST] && room->dir_option[SOUTHWEST]->to_room != -1 ?
		world[room->dir_option[SOUTHWEST]->to_room].number : -1,
		grn, nrm, cyn,
		room->dir_option[SOUTH] && room->dir_option[SOUTH]->to_room != -1 ?
		world[room->dir_option[SOUTH]->to_room].number : -1,
		grn, nrm, cyn,
		room->dir_option[NORTHWEST] && room->dir_option[NORTHWEST]->to_room
		!= -1 ? world[room->dir_option[NORTHWEST]->to_room].number : -1,
		grn, nrm, cyn,
		room->dir_option[WEST] && room->dir_option[WEST]->to_room != -1 ?
		world[room->dir_option[WEST]->to_room].number : -1,
		grn, nrm, cyn,
		room->dir_option[SOUTHEAST] && room->dir_option[SOUTHEAST]->to_room != -1 ?
		world[room->dir_option[SOUTHEAST]->to_room].number : -1,
		grn, nrm, cyn,
		room->dir_option[UP] && room->dir_option[UP]->to_room != -1 ?
		world[room->dir_option[UP]->to_room].number : -1,
		grn, nrm, cyn,
		room->dir_option[INDIR] && room->dir_option[INDIR]->to_room != -1 ?
		world[room->dir_option[INDIR]->to_room].number : -1,
		grn, nrm, cyn,
		room->dir_option[DOWN] && room->dir_option[DOWN]->to_room != -1 ?
		world[room->dir_option[DOWN]->to_room].number : -1,
		grn, nrm, cyn,
		room->dir_option[OUTDIR] && room->dir_option[OUTDIR]->to_room != -1 ?
		world[room->dir_option[OUTDIR]->to_room].number : -1,
		grn, nrm,
		grn, nrm, cyn, printbuf3,
#ifdef DG_SCRIPT_VERSION
		grn, nrm, cyn, room->proto_script?"Set.":"Not Set.",
#endif
		grn, nrm,
		grn, nrm
		);

	release_buffer(printbuf1);
	release_buffer(printbuf2);
	release_buffer(printbuf3);

	OLC_MODE(d) = REDIT_MAIN_MENU;
}

/**************************************************************************
	The main loop
 **************************************************************************/

void redit_parse(struct descriptor_data *d, char *arg)
{
	int number;

	switch (OLC_MODE(d)) {
	case REDIT_CONFIRM_SAVESTRING:
		switch (*arg) {
		case 'y':
		case 'Y':
			redit_save_internally(d);
			redit_save_to_disk(OLC_ZNUM(d));
			extended_mudlog(BRF, SYSL_OLC, TRUE, "%s edits room %d.", GET_NAME(d->character), OLC_NUM(d));
			/*
			 * Do NOT free strings! Just the room structure. 
			 */
			cleanup_olc(d, CLEANUP_STRUCTS);
			write_to_output(d, TRUE, "Room saved to disk and memory.\r\n");
			break;
		case 'n':
		case 'N':
			/*
			 * Free everything up, including strings, etc.
			 */
			cleanup_olc(d, CLEANUP_ALL);
			break;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\nDo you wish to save this room? : ");
			break;
		}
		return;

	case REDIT_MAIN_MENU:
		switch (*arg) {
		case 'q':
		case 'Q':
			if (OLC_VAL(d)) { /* Something has been modified. */
				write_to_output(d, TRUE, "Do you wish to save this room? : ");
				OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
			} else
				cleanup_olc(d, CLEANUP_ALL);
			return;
		case '1':
			write_to_output(d, TRUE, "Enter room name:-\r\n] ");
			OLC_MODE(d) = REDIT_NAME;
			break;
		case '2':
			OLC_MODE(d) = REDIT_DESC;
			clear_screen(d);
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_ROOM(d)->description)
				write_to_output(d, FALSE, "%s", OLC_ROOM(d)->description);
			string_write(d, &OLC_ROOM(d)->description, MAX_ROOM_DESC, 0, STATE(d));
			OLC_VAL(d) = 1;
			break;
		case '3':
			redit_disp_flag_menu(d);
			break;
		case '4':
			redit_disp_sector_menu(d);
			break;
		case '5':
			OLC_VAL(d) = NORTH;
			redit_disp_exit_menu(d);
			break;
		case '6':
			OLC_VAL(d) = EAST;
			redit_disp_exit_menu(d);
			break;
		case '7':
			OLC_VAL(d) = SOUTH;
			redit_disp_exit_menu(d);
			break;
		case '8':
			OLC_VAL(d) = WEST;
			redit_disp_exit_menu(d);
			break;
		case '9':
			OLC_VAL(d) = UP;
			redit_disp_exit_menu(d);
			break;
		case 'a':
		case 'A':
			OLC_VAL(d) = DOWN;
			redit_disp_exit_menu(d);
			break;
		case 'b':
		case 'B':
			OLC_VAL(d) = NORTHEAST;
			redit_disp_exit_menu(d);
			break;
		case 'c':
		case 'C':
			OLC_VAL(d) = SOUTHWEST;
			redit_disp_exit_menu(d);
			break;
		case 'd':
		case 'D':
			OLC_VAL(d) = NORTHWEST;
			redit_disp_exit_menu(d);
			break;
		case 'e':
		case 'E':
			OLC_VAL(d) = SOUTHEAST;
			redit_disp_exit_menu(d);
			break;
		case 'f':
		case 'F':
			OLC_VAL(d) = INDIR;
			redit_disp_exit_menu(d);
			break;
		case 'g':
		case 'G':
			OLC_VAL(d) = OUTDIR;
			redit_disp_exit_menu(d);
			break;
		case 'h':
		case 'H':
			/*
			 * If the extra description doesn't exist.
			 */
			if (!OLC_ROOM(d)->ex_description)
				CREATE(OLC_ROOM(d)->ex_description, struct extra_descr_data, 1);
			OLC_DESC(d) = OLC_ROOM(d)->ex_description;
			redit_disp_extradesc_menu(d);
			break;
		case 'm':
		case 'M':
			redit_disp_flux_menu(d);
			break;
		case 'r':
		case 'R':
			redit_disp_resources_menu(d);
			break;
		case 's':
		case 'S':
			if (GOT_RIGHTS(d->character, RIGHTS_TRIGGERS)) {
				OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
				dg_script_menu(d);
				return;
			} else {
				write_to_output(d, TRUE, "\r\n&RYou do not have trigger editing rights.&n\r\n\r\n");
				redit_disp_menu(d);
				return;
			}
		default:
			write_to_output(d, TRUE, "\r\n&RInvalid choice!&n\r\n\r\n");
			redit_disp_menu(d);
			break;
		}
		return;

	case OLC_SCRIPT_EDIT:
		if (dg_script_edit_parse(d, arg)) return;
		break;

	case REDIT_NAME:
		if (!genolc_checkstring(d, arg))
			break;
		if (OLC_ROOM(d)->name)
			free(OLC_ROOM(d)->name);
		arg[MAX_ROOM_NAME - 1] = '\0';
		OLC_ROOM(d)->name = str_udup(arg);
		break;

	case REDIT_DESC:
		/*
		 * We will NEVER get here, we hope.
		 */
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached REDIT_DESC case in parse_redit().");
		write_to_output(d, TRUE, "Oops, in REDIT_DESC.\r\n");
		break;

	case REDIT_FLAGS:
		number = atoi(arg);
		if (number < 0 || number > NUM_ROOM_FLAGS) {
			write_to_output(d, TRUE, "That is not a valid choice!\r\n");
			redit_disp_flag_menu(d);
		} else if (number == 0)
			break;
		else {
			/*
			 * Toggle the bit.
			 */
			TOGGLE_BIT(OLC_ROOM(d)->room_flags, 1ULL << (number - 1));
			redit_disp_flag_menu(d);
		}
		return;

	case REDIT_SECTOR:
		number = atoi(arg);
		if (number < 0 || number >= NUM_ROOM_SECTORS) {
			write_to_output(d, TRUE, "Invalid choice!");
			redit_disp_sector_menu(d);
			return;
		}
		OLC_ROOM(d)->sector_type = number;
		break;

	case REDIT_FLUX:
		number = atoi(arg);
		if (number < 0 || number >= NUM_FLUX_TYPES) {
			write_to_output(d, TRUE, "Invalid choice!");
			redit_disp_flux_menu(d);
			return;
		}
		OLC_ROOM(d)->magic_flux = number;
		break;

	case REDIT_RESOURCE_FLAGS:
		number = atoi(arg);
		if (number < 0 || number > NUM_RESOURCES) {
			write_to_output(d, TRUE, "That is not a valid choice!\r\n");
			redit_disp_resource_flag_menu(d);
		} else if (number == 0) {
			OLC_MODE(d) = REDIT_RESOURCES_MENU;
			redit_disp_resources_menu(d);
			return;
		} else {
			/*
			 * Toggle the bit.
			 */
			TOGGLE_BIT(OLC_ROOM(d)->resources.resources, 1ULL << (number - 1));
			redit_disp_resource_flag_menu(d);
		}
		return;

	case REDIT_RESOURCES_MENU:
		switch (*arg) {
		case '0':
			break;
		case '1':
			OLC_MODE(d) = REDIT_RESOURCE_FLAGS;
			redit_disp_resource_flag_menu(d);
			return;
		case '2':
			OLC_MODE(d) = REDIT_RESOURCE_VAL0;
			write_to_output(d, TRUE, "Enter amount of ore : ");
			return;
		case '3':
			OLC_MODE(d) = REDIT_RESOURCE_VAL1;
			write_to_output(d, TRUE, "Enter amount of gems : ");
			return;
		case '4':
			OLC_MODE(d) = REDIT_RESOURCE_VAL2;
			write_to_output(d, TRUE, "Enter amount of wood : ");
			return;
		case '5':
			OLC_MODE(d) = REDIT_RESOURCE_VAL3;
			write_to_output(d, TRUE, "Enter amount of stone : ");
			return;
		case '6':
			OLC_MODE(d) = REDIT_RESOURCE_VAL4;
			write_to_output(d, TRUE, "Enter amount of fabric : ");
			return;
		case '7':
			OLC_ROOM(d)->resources.resources = 0;
			OLC_ROOM(d)->resources.max[RESOURCE_ORE] = 0;
			OLC_ROOM(d)->resources.max[RESOURCE_GEMS] = 0;
			OLC_ROOM(d)->resources.max[RESOURCE_WOOD] = 0;
			OLC_ROOM(d)->resources.max[RESOURCE_STONE] = 0;
			OLC_ROOM(d)->resources.max[RESOURCE_FABRIC] = 0;
			write_to_output(d, TRUE, "Resources reset.\r\n");
			redit_disp_resources_menu(d);
			return;
		default:
			write_to_output(d, TRUE, "Try again : ");
			return;
		}
		break;

	case REDIT_RESOURCE_VAL0:
		OLC_ROOM(d)->resources.max[RESOURCE_ORE] = atoi(arg);
		redit_disp_resources_menu(d);
		return;

	case REDIT_RESOURCE_VAL1:
		OLC_ROOM(d)->resources.max[RESOURCE_GEMS] = atoi(arg);
		redit_disp_resources_menu(d);
		return;

	case REDIT_RESOURCE_VAL2:
		OLC_ROOM(d)->resources.max[RESOURCE_WOOD] = atoi(arg);
		redit_disp_resources_menu(d);
		return;

	case REDIT_RESOURCE_VAL3:
		OLC_ROOM(d)->resources.max[RESOURCE_STONE] = atoi(arg);
		redit_disp_resources_menu(d);
		return;

	case REDIT_RESOURCE_VAL4:
		OLC_ROOM(d)->resources.max[RESOURCE_FABRIC] = atoi(arg);
		redit_disp_resources_menu(d);
		return;

	case REDIT_EXIT_MENU:
		switch (*arg) {
		case '0':
			break;
		case '1':
			OLC_MODE(d) = REDIT_EXIT_NUMBER;
			write_to_output(d, TRUE, "Exit to room number : ");
			return;
		case '2':
			OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_EXIT(d)->general_description)
				write_to_output(d, FALSE, "%s", OLC_EXIT(d)->general_description);
			string_write(d, &OLC_EXIT(d)->general_description, MAX_EXIT_DESC, 0, STATE(d));
			return;
		case '3':
			OLC_MODE(d) = REDIT_EXIT_KEYWORD;
			write_to_output(d, TRUE, "Enter keywords : ");
			return;
		case '4':
			OLC_MODE(d) = REDIT_EXIT_KEY;
			write_to_output(d, TRUE, "Enter key number : ");
			return;
		case '5':
			OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
			redit_disp_exit_flag_menu(d);
			return;
		case '6':
			/*
			 * Delete an exit.
			 */
			if (OLC_EXIT(d)->keyword)
				free(OLC_EXIT(d)->keyword);
			if (OLC_EXIT(d)->general_description)
				free(OLC_EXIT(d)->general_description);
			if (OLC_EXIT(d))
				free(OLC_EXIT(d));
			OLC_EXIT(d) = NULL;
			break;
		default:
			write_to_output(d, TRUE, "Try again : ");
			return;
		}
		break;

	case REDIT_EXIT_NUMBER:
		if ((number = atoi(arg)) != -1)
			if ((number = real_room(number)) < 0) {
				write_to_output(d, TRUE, "That room does not exist, try again : ");
				return;
			}
		OLC_EXIT(d)->to_room = number;
		redit_disp_exit_menu(d);
		return;

	case REDIT_EXIT_DESCRIPTION:
		/*
		 * We should NEVER get here, hopefully.
		 */
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached REDIT_EXIT_DESC case in parse_redit");
		write_to_output(d, TRUE, "Oops, in REDIT_EXIT_DESCRIPTION.\r\n");
		break;

	case REDIT_EXIT_KEYWORD:
		if (OLC_EXIT(d)->keyword)
			free(OLC_EXIT(d)->keyword);
		OLC_EXIT(d)->keyword = str_udup(arg);
		redit_disp_exit_menu(d);
		return;

	case REDIT_EXIT_KEY:
		OLC_EXIT(d)->key = atoi(arg);
		redit_disp_exit_menu(d);
		return;

	case REDIT_EXIT_DOORFLAGS:
		number = atoi(arg);
		if (number < 0 || number > 6) {
			write_to_output(d, TRUE, "That's not a valid choice!\r\n");
			redit_disp_exit_flag_menu(d);
		} else {
			/*
			 * Doors are a bit idiotic, don't you think? :) -- I agree. -gg
			 */
			OLC_EXIT(d)->exit_info = (number == 0 ? 0 :
																(number == 1 ? EX_ISDOOR :
																(number == 2 ? EX_ISDOOR | EX_PICKPROOF :
				(number == 3 ? EX_FAERIE :
				(number == 4 ? EX_INFERNAL :
				(number == 5 ? EX_DIVINE :
				(number == 6 ? EX_ANCIENT : 0)))))));
			/*
			 * Jump back to the menu system.
			 */
			redit_disp_exit_menu(d);
		}
		return;

	case REDIT_EXTRADESC_KEY:
		if (genolc_checkstring(d, arg))
			OLC_DESC(d)->keyword = str_dup(arg);
		redit_disp_extradesc_menu(d);
		return;

	case REDIT_EXTRADESC_MENU:
		switch ((number = atoi(arg))) {
		case 0:
			/*
			 * If something got left out, delete the extra description
			 * when backing out to the menu.
			 */
			if (OLC_DESC(d)->keyword == NULL || OLC_DESC(d)->description == NULL) {
				struct extra_descr_data **tmp_desc;
				if (OLC_DESC(d)->keyword)
					free(OLC_DESC(d)->keyword);
				if (OLC_DESC(d)->description)
					free(OLC_DESC(d)->description);

				/*
				 * Clean up pointers.
				 */
				for (tmp_desc = &(OLC_ROOM(d)->ex_description); *tmp_desc; tmp_desc = &((*tmp_desc)->next))
					if (*tmp_desc == OLC_DESC(d)) {
						*tmp_desc = NULL;
						break;
		}
		free(OLC_DESC(d));
			}
			break;
		case 1:
			OLC_MODE(d) = REDIT_EXTRADESC_KEY;
			write_to_output(d, TRUE, "Enter keywords, separated by spaces : ");
			return;
		case 2:
			OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_DESC(d)->description)
				write_to_output(d, FALSE, "%s", OLC_DESC(d)->description);
			string_write(d, &OLC_DESC(d)->description, MAX_MESSAGE_LENGTH, 0, STATE(d));
			return;
		case 3:
			if (OLC_DESC(d)->keyword == NULL || OLC_DESC(d)->description == NULL) {
				write_to_output(d, TRUE, "You can't edit the next extra description without completing this one.\r\n");
				redit_disp_extradesc_menu(d);
			} else {
				struct extra_descr_data *new_extra;

				if (OLC_DESC(d)->next)
					OLC_DESC(d) = OLC_DESC(d)->next;
				else {
					/*
					 * Make new extra description and attach at end.
					 */
					CREATE(new_extra, struct extra_descr_data, 1);
					OLC_DESC(d)->next = new_extra;
		OLC_DESC(d) = new_extra;
	}
	redit_disp_extradesc_menu(d);
			}
			return;
		}
		break;

	default:
		/*
		 * We should never get here.
		 */
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached default case in parse_redit");
		break;
	}
	/*
	 * If we get this far, something has been changed.
	 */
	OLC_VAL(d) = 1;
	redit_disp_menu(d);
}

void redit_string_cleanup(struct descriptor_data *d, int terminator)
{
	switch (OLC_MODE(d)) {
	case REDIT_DESC:
		redit_disp_menu(d);
		break;
	case REDIT_EXIT_DESCRIPTION:
		redit_disp_exit_menu(d);
		break;
	case REDIT_EXTRADESC_DESCRIPTION:
		redit_disp_extradesc_menu(d);
		break;
	}
}
