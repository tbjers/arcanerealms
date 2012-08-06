/* ************************************************************************
*  File: gedit.c                                        Part of CircleMUD *
*	 Usage: guild editor for setting guild and guildie information          *
*																																					*
*  Created: Fri Jun 28 2002 by Torgny Bjers (artovil@arcanerealms.org)    *
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
/* $Id: gedit.c,v 1.7 2002/11/06 18:55:47 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "events.h"
#include "guild.h"
#include "guild_parser.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "boards.h"
#include "genolc.h"
#include "oasis.h"

/* External Declarations */

extern struct zone_data *zone_table;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern zone_rnum top_of_zone_table;
extern char *class_abbrevs[];
extern struct spell_info_type spell_info[];
extern ACMD(do_look);
struct obj_data *obj_proto;

extern struct guild_info *guilds_data;
extern void GuildToSQL(struct guild_info *g);
void extract_char_final(struct char_data *ch);
extern const char *guild_types[17];

/* External utility functions */
zone_rnum	real_zone(zone_vnum vnum);

/* Declarations from guild.c */

void save_guild(struct guild_info *g);
void save_all_guilds(void);

/*-------------------------------------------------------------------*/

/*
 * Copy strings over so bad things don't happen.  We do not free the
 * existing strings here because copy_guild() did a shallow copy previously
 * and we'd be freeing the very strings we're copying.  If this function
 * is used elsewhere, be sure to free_room_strings() the 'dest' room first.
 */
int	copy_guild_strings(struct guild_info *dest, struct guild_info *source)
{
	if (dest == NULL || source == NULL) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "GenOLC: copy_guild_strings: NULL values passed.");
		return FALSE;
	}

	dest->name = str_dup(source->name);
	if (source->gl_title)
		dest->gl_title = str_dup(source->gl_title);
	if (source->guildie_titles)
		dest->guildie_titles = str_dup(source->guildie_titles);
	if (source->gossip_name)
		dest->gossip_name = str_dup(source->gossip_name);
	dest->gchan_name = str_dup(source->gchan_name);
	if (source->gchan_color)
		dest->gchan_color = str_dup(source->gchan_color);
	else
		dest->gchan_color = str_dup("n");
	dest->description = str_udup(source->description);
	dest->requirements = str_udup(source->requirements);
	dest->gossip = str_udup(source->gossip);

	return TRUE;
}


int	free_guild_strings(struct guild_info *guild)
{
	/* Free descriptions. */
	if (guild->name)
		free(guild->name);
	if (guild->gl_title)
		free(guild->gl_title);
	if (guild->guildie_titles)
		free(guild->guildie_titles);
	if (guild->gossip_name)
		free(guild->gossip_name);
	if (guild->gchan_color)
		free(guild->gchan_color);
	if (guild->description)
		free(guild->description);
	if (guild->requirements)
		free(guild->requirements);
	if (guild->gossip)
		free(guild->gossip);

	return TRUE;
}


int	copy_guild(struct guild_info *to, struct guild_info *from)
{
	free_guild_strings(to);
	*to = *from;
	if (copy_guild_strings(to, from))
		return TRUE;
	return FALSE;
}


struct guild_info *change_guild(struct guild_info *guild, int id)
{
	struct guild_info *g;
	g = guilds_data;
	while (g) {
		if (g->id == id) break;
		g = g->next;
	}	
	if (!(copy_guild(g, guild)))
		return (NULL);
	else
		return (g);
}


void save_guild_internally(struct descriptor_data *d)
{
	struct guild_info *g;
	if ((g = change_guild(OLC_GUILD(d), OLC_GUILD(d)->id)))
		save_guild(g);
	else {
		write_to_output(d, TRUE, "Could not create copy of guild in change_guild()\r\n");
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Could not create copy of guild in change_guild()");
	}
}


void gedit_setup_existing(struct descriptor_data *d, struct guild_info *guild)
{
	struct guild_info *g;

	/*
	 * Build a copy of the guild for editing.
	 */
	CREATE(g, struct guild_info, 1);

	/*
	 * Allocate space for all strings.
	 */
	g->name = str_udup(guild->name);
	if (guild->gl_title)
		g->gl_title = str_dup(guild->gl_title);
	if (guild->guildie_titles)
		g->guildie_titles = str_dup(guild->guildie_titles);
	if (guild->gossip_name)
		g->gossip_name = str_udup(guild->gossip_name);
	g->gchan_name = str_udup(guild->gchan_name);
	g->gchan_color = str_udup(guild->gchan_color);
	g->description = str_udup(guild->description);
	g->requirements = str_udup(guild->requirements);
	g->gossip = str_udup(guild->gossip);
	g->gchan_type = guild->gchan_type;
	g->id = guild->id;
	g->type = guild->type;
	g->gold = guild->gold;
	g->guildwalk_room = guild->guildwalk_room;
	g->gflags = guild->gflags;
	g->next = guild->next;
	g->guildies = guild->guildies;
	g->gequip = guild->gequip;
	g->gskills = guild->gskills;
	g->gzones = guild->gzones;
	g->ranks = guild->ranks;
	g->ghelp = guild->ghelp;

	/*
	 * Attach copy of guild to player's descriptor.
	 */
	OLC_GUILD(d) = g;
	OLC_VAL(d) = 0;
	gedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void gedit_disp_guild_types(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (counter = 0; counter < MAX_GUILD_TYPES; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
								guild_types[counter], !(++columns % 2) ? "\r\n" : "");
	/* Add the club at the end since it doesn't come in order */
	write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, 15, nrm,
							guild_types[15], !(++columns % 2) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\nEnter guild type : ");
	OLC_MODE(d) = GEDIT_TYPE;
}


void gedit_disp_channel_colors(struct descriptor_data *d)
{
	int counter, columns = 0;
	const char *colors = "nbcgmrwyBCDGMRWY";

	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; colors[counter] != '\0'; counter++)
		write_to_output(d, TRUE, "%s%c%s) %-20.20s %s", grn, colors[counter], nrm,
								color_name(colors[counter]), !(++columns % 2) ? "\r\n" : "");

	write_to_output(d, TRUE, "\r\nEnter guild channel color : ");
	OLC_MODE(d) = GEDIT_GCHAN_COLOR;
}


void gedit_disp_channel_types(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (counter = 1; *guild_chantypes[counter] != '\n'; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
								guild_chantypes[counter], !(++columns % 2) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\nEnter channel type : ");
	OLC_MODE(d) = GEDIT_GCHAN_TYPE;
}


void gedit_disp_flag_menu(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (counter = 0; counter < NUM_GUILD_FLAGS; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
								guild_flags[counter], !(++columns % 2) ? "\r\n" : "");
	sprintbit(OLC_GUILD(d)->gflags, guild_flags, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nGuild flags: %s%s%s\r\n"
					"Enter guild flags, 0 to quit : ", cyn, buf1, nrm);
	OLC_MODE(d) = GEDIT_FLAGS;
}


void gedit_ranks_menu(struct descriptor_data *d)
{
	struct guild_info *g;
	struct rank_info *rank;
	int i = 0;

	g = OLC_GUILD(d);
	rank = g->ranks;

	get_char_colors(d->character);
	clear_screen(d);

	write_to_output(d, TRUE, "\r\nRANK - TITLE\r\n");

	while (rank) {
		write_to_output(d, TRUE, "[%s%2ld%s] - %s%s%s\r\n",
			cyn, rank->num, nrm,
			yel, rank->name, nrm);
		i++;
		rank = rank->next;
	}
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new rank.\r\n"
		"%sD%s) Delete a rank.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = GEDIT_RANKS_MENU;
}


void gedit_zones_menu(struct descriptor_data *d)
{
	struct guild_info *g;
	struct gzone_info *gzone;
	int i = 0, zone;

	g = OLC_GUILD(d);
	gzone = g->gzones;

	get_char_colors(d->character);
	clear_screen(d);

	write_to_output(d, TRUE, "\r\nZONE  - TITLE\r\n");

	while (gzone) {
		zone = real_zone(gzone->zone);
		write_to_output(d, TRUE, "[%s%3d%s] - %s%s%s\r\n",
			cyn, gzone->zone, nrm,
			yel, zone != NOWHERE ? (zone_table[zone].name ? zone_table[zone].name : "<NONE!>") : "<NONE!>", nrm);
		i++;
		gzone = gzone->next;
	}
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new zone.\r\n"
		"%sD%s) Delete a zone.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = GEDIT_ZONES_MENU;
}


void gedit_equipment_menu(struct descriptor_data *d)
{
	struct guild_info *g;
	struct gequip_info *gequip;
	int i = 0, object;

	g = OLC_GUILD(d);
	gequip = g->gequip;

	get_char_colors(d->character);
	clear_screen(d);

	write_to_output(d, TRUE, "\r\nZONE  - TITLE\r\n");

	while (gequip) {
		object = real_object(gequip->vnum);
		write_to_output(d, TRUE, "[%s%3d%s] - %s%s%s\r\n",
			cyn, gequip->vnum, nrm,
			yel, object != NOTHING ? (obj_proto[object].name ? obj_proto[object].name : "<NONE!>") : "<NONE!>", nrm);
		i++;
		gequip = gequip->next;
	}
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new object.\r\n"
		"%sD%s) Delete a object.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = GEDIT_EQUIPMENT_MENU;
}

/*-------------------------------------------------------------------*/

void gedit_disp_menu(struct descriptor_data * d)
{
	char *color;

	sprintbit(OLC_GUILD(d)->gflags, guild_flags, buf1, sizeof(buf1));
	color = str_dup(color_name(*OLC_GUILD(d)->gchan_color));
	
	get_char_colors(d->character);

	write_to_output(d, TRUE,
		"-- Guild Number : [%s%d%s]\r\n"
		"%s1%s) Name                : %s%s\r\n"
		"%s2%s) GL Title            : %s%s\r\n"
		"%s3%s) Guildies pre-title  : %s%s\r\n"
		"%s4%s) Guild type          : [%s%s%s ^]\r\n"
		"%s5%s) Gossip command      : %s%s\r\n"
		"%s6%s) Guild channel name  : %s%s\r\n"
		"%s7%s) Guild channel color : &%s%s\r\n"
		"%s8%s) Guild channel type  : [%s%s%s ^]\r\n"
		"%s9%s) Guild gold in bank  : [%s%d%s]\r\n"
		"%sA%s) Guildwalk room      : [%s%d%s]\r\n"
		"%sB%s) Guild flags         : %s%s\r\n"
		"%sC%s) Edit Requirements\r\n"
		"%sD%s) Edit Description\r\n"
		"%sE%s) Edit Equipment\r\n"
		"%sG%s) Edit Gossip\r\n"
		/* "%sH%s) Edit Helpfiles\r\n" */
		"%sR%s) Edit Ranks\r\n"
		/* "%sS%s) Edit Skills\r\n" */
		"%sZ%s) Edit Zones\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ",
		
		cyn, OLC_GUILD(d)->id, nrm,
		grn, nrm, yel, OLC_GUILD(d)->name,
		grn, nrm, yel, OLC_GUILD(d)->gl_title ? OLC_GUILD(d)->gl_title : "<None>",
		grn, nrm, yel, OLC_GUILD(d)->guildie_titles ? OLC_GUILD(d)->guildie_titles : "<None>",
		grn, nrm, cyn, guild_types[OLC_GUILD(d)->type], nrm,
		grn, nrm, yel, OLC_GUILD(d)->gossip_name ? OLC_GUILD(d)->gossip_name : "<None>",
		grn, nrm, yel, OLC_GUILD(d)->gchan_name ? OLC_GUILD(d)->gchan_name : "<None>",
		grn, nrm, OLC_GUILD(d)->gchan_color, CAP(color),
		grn, nrm, cyn, guild_chantypes[OLC_GUILD(d)->gchan_type], nrm,
		grn, nrm, cyn, OLC_GUILD(d)->gold, nrm,
		grn, nrm, yel, OLC_GUILD(d)->guildwalk_room, nrm,
		grn, nrm, cyn, buf1,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		/* grn, nrm, */
		grn, nrm,
		/* grn, nrm, */
		grn, nrm,
		grn, nrm
	);

	release_buffer(color);

	OLC_MODE(d) = GEDIT_MAIN_MENU;
}


void gedit_parse(struct descriptor_data *d, char *arg)
{
	int i = -1, number;

	if (OLC_MODE(d) > SPEDIT_NUMERICAL_RESPONSE) {
		i = atoi(arg);
		if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && !isdigit(arg[1])))) {
			write_to_output(d, TRUE, "Field must be numerical, try again : ");
			return;
		}
	} else { /* String response. */
		if (!genolc_checkstring(d, arg))
			return;
	}
	switch (OLC_MODE(d)) {
	case SPEDIT_CONFIRM_SAVE:
		switch (*arg) {
		case 'y':
		case 'Y':
			save_guild_internally(d);
			extended_mudlog(BRF, SYSL_OLC, TRUE, "%s edits guild %d", GET_NAME(d->character), OLC_GUILD(d)->id);
			write_to_output(d, TRUE, "Guild saved to disk and memory.\r\n");
			cleanup_olc(d, CLEANUP_STRUCTS);
			break;
		case 'n':
		case 'N':
			/* free everything up, including strings etc */
			cleanup_olc(d, CLEANUP_ALL);
			break;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\n");
			write_to_output(d, TRUE, "Do you wish to save this guild? : ");
			break;
		}
		return;
		
		case SPEDIT_MAIN_MENU:
			i = 0;
			switch (*arg) {
			case 'q':
			case 'Q':
				if (OLC_VAL(d)) {
					/*. Something has been modified .*/
					write_to_output(d, TRUE, "Do you wish to save this guild? : ");
					OLC_MODE(d) = GEDIT_CONFIRM_SAVE;
				} else
					cleanup_olc(d, CLEANUP_STRUCTS);
				return;
			case '1':
				i--;
				write_to_output(d, TRUE, "Enter guild name:-\r\n| ");
				OLC_MODE(d) = GEDIT_NAME;
				break;
			case '2':
				i--;
				write_to_output(d, TRUE, "Enter guild leader title:-\r\n| ");
				OLC_MODE(d) = GEDIT_GLTITLE;
				break;
			case '3':
				i--;
				write_to_output(d, TRUE, "Enter guildie pre-title:-\r\n| ");
				OLC_MODE(d) = GEDIT_GUILDIES_PRETITLE;
				break;
			case '4':
				i++;
				gedit_disp_guild_types(d);
				break;
			case '5':
				i--;
				write_to_output(d, TRUE, "Enter the command name of the gossip:-\r\n| ");
				OLC_MODE(d) = GEDIT_GOSSIP_NAME;
				break;
			case '6':
				i--;
				write_to_output(d, TRUE, "Enter the name of the guild channel:-\r\n| ");
				OLC_MODE(d) = GEDIT_GCHAN_NAME;
				break;
			case '7':
				i--;
				gedit_disp_channel_colors(d);
				break;
			case '8':
				i++;
				gedit_disp_channel_types(d);
				break;
			case '9':
				i++;
				write_to_output(d, TRUE, "Enter amount of gold in bank:-\r\n| ");
				OLC_MODE(d) = GEDIT_GOLD;
				break;
			case 'a':
			case 'A':
				i++;
				write_to_output(d, TRUE, "Enter guildwalk room:-\r\n| ");
				OLC_MODE(d) = GEDIT_GUILDWALK;
				break;
			case 'b':
			case 'B':
				i++;
				gedit_disp_flag_menu(d);
				break;
			case 'c':
			case 'C':
				i--;
				OLC_MODE(d) = GEDIT_REQUIREMENTS;
				clear_screen(d);
				write_to_output(d, TRUE, "%s", stredit_header);
				if (OLC_GUILD(d)->requirements)
					write_to_output(d, FALSE, "%s", OLC_GUILD(d)->requirements);
				string_write(d, &OLC_GUILD(d)->requirements, MAX_STRING_LENGTH, 0, STATE(d));
				OLC_VAL(d) = 1;
				break;
			case 'd':
			case 'D':
				i--;
				OLC_MODE(d) = GEDIT_DESCRIPTION;
				clear_screen(d);
				write_to_output(d, TRUE, "%s", stredit_header);
				if (OLC_GUILD(d)->description)
					write_to_output(d, FALSE, "%s", OLC_GUILD(d)->description);
				string_write(d, &OLC_GUILD(d)->description, MAX_STRING_LENGTH, 0, STATE(d));
				OLC_VAL(d) = 1;
				break;
			case 'e':
			case 'E':
				i--;
				gedit_equipment_menu(d);
				break;
			case 'g':
			case 'G':
				i--;
				OLC_MODE(d) = GEDIT_GOSSIP;
				clear_screen(d);
				write_to_output(d, TRUE, "%s", stredit_header);
				if (OLC_GUILD(d)->gossip)
					write_to_output(d, FALSE, "%s", OLC_GUILD(d)->gossip);
				string_write(d, &OLC_GUILD(d)->gossip, MAX_STRING_LENGTH, 0, STATE(d));
				OLC_VAL(d) = 1;
				break;
			case 'r':
			case 'R':
				i--;
				gedit_ranks_menu(d);
				break;
			case 'z':
			case 'Z':
				i--;
				gedit_zones_menu(d);
				break;
			default:
				write_to_output(d, TRUE, "Invalid choice!\r\n");
				gedit_disp_menu(d);
				break;
			}
			return;
/*-------------------------------------------------------------------*/
		case GEDIT_NAME:
			if (OLC_GUILD(d)->name)
				free(OLC_GUILD(d)->name);
			OLC_GUILD(d)->name = str_dup(arg);
			break;				
		case GEDIT_GLTITLE:
			if (OLC_GUILD(d)->gl_title)
				free(OLC_GUILD(d)->gl_title);
			if (*arg)
				OLC_GUILD(d)->gl_title = str_dup(arg);
			else
				OLC_GUILD(d)->gl_title = NULL;
			break;				
		case GEDIT_GUILDIES_PRETITLE:
			if (OLC_GUILD(d)->guildie_titles)
				free(OLC_GUILD(d)->guildie_titles);
			if (*arg)
				OLC_GUILD(d)->guildie_titles = str_dup(arg);
			else
				OLC_GUILD(d)->guildie_titles = NULL;
			break;				
/*-------------------------------------------------------------------*/
		case GEDIT_TYPE:
			if (atoi(arg) != 15)
				OLC_GUILD(d)->type = LIMIT(atoi(arg), 0, MAX_GUILD_TYPES);
			else
				OLC_GUILD(d)->type = 15;
			break;				
/*-------------------------------------------------------------------*/
		case GEDIT_GOSSIP_NAME:
			if (OLC_GUILD(d)->gossip_name)
				free(OLC_GUILD(d)->gossip_name);
			if (*arg)
				OLC_GUILD(d)->gossip_name = str_dup(arg);
			else
				OLC_GUILD(d)->gossip_name = NULL;
			break;				
		case GEDIT_GCHAN_NAME:
			if (!*arg) {
				write_to_output(d, TRUE, "\r\n&RYou have to give the guild channel a name.&n\r\n\r\n");
				write_to_output(d, TRUE, "Enter the name of the guild channel:-\r\n| ");
				OLC_MODE(d) = GEDIT_GCHAN_NAME;
				return;
			}
			if (OLC_GUILD(d)->gchan_name)
				free(OLC_GUILD(d)->gchan_name);
			OLC_GUILD(d)->gchan_name = str_dup(arg);
			break;				
		case GEDIT_GCHAN_COLOR:
			if (!*arg || !*color_name(*arg)) {
				gedit_disp_channel_colors(d);
				return;
			}
			if (OLC_GUILD(d)->gchan_color)
				free(OLC_GUILD(d)->gchan_color);
			OLC_GUILD(d)->gchan_color = str_dup(arg);
			break;				
		case GEDIT_GCHAN_TYPE:
			OLC_GUILD(d)->gchan_type = LIMIT(atoi(arg), 1, MAX_GCHAN_TYPE);
			break;				
/*-------------------------------------------------------------------*/
		case GEDIT_GOLD:
			OLC_GUILD(d)->gold = atoi(arg);
			break;				
		case GEDIT_GUILDWALK:
			if ((number = real_room(atoi(arg))) >= 0 && *arg) {
				OLC_GUILD(d)->guildwalk_room = atoi(arg);
			} else {
				write_to_output(d, TRUE, "That room does not exist, try again : ");
				return;
			}
			break;				
		case GEDIT_FLAGS:
			number = atoi(arg);
			if (number < 0 || number > NUM_GUILD_FLAGS) {
				write_to_output(d, TRUE, "That is not a valid choice!\r\n");
				gedit_disp_flag_menu(d);
			} else if (number == 0)
				break;
			else {
				/*
				 * Toggle the bit.
				 */
				TOGGLE_BIT(OLC_GUILD(d)->gflags, 1ULL << (number - 1));
				gedit_disp_flag_menu(d);
			}
			return;
/*-------------------------------------------------------------------*/
		case GEDIT_REQUIREMENTS:
			/*
			 * We will NEVER get here, we hope.
			 */
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached GEDIT_REQUIREMENTS case in parse_gedit().");
			write_to_output(d, TRUE, "Oops, in GEDIT_REQUIREMENTS.\r\n");
			break;
		case GEDIT_DESCRIPTION:
			/*
			 * We will NEVER get here, we hope.
			 */
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached GEDIT_DESCRIPTION case in parse_gedit().");
			write_to_output(d, TRUE, "Oops, in GEDIT_DESCRIPTION.\r\n");
			break;
		case GEDIT_GOSSIP:
			/*
			 * We will NEVER get here, we hope.
			 */
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached GEDIT_GOSSIP case in parse_gedit().");
			write_to_output(d, TRUE, "Oops, in GEDIT_GOSSIP.\r\n");
			break;
/*-------------------------------------------------------------------*/
		case GEDIT_RANKS_MENU:
			switch (*arg) {
			case 'a':
			case 'A':
				write_to_output(d, TRUE, "\r\nEnter new rank number : ");
				OLC_MODE(d) = GEDIT_NEW_RANK_NUMBER;
				return;
			case 'd':
			case 'D':
				write_to_output(d, TRUE, "\r\nDelete which rank? : ");
				OLC_MODE(d) = GEDIT_DELETE_RANK;
				return;
			case 'q':
			case 'Q':
				break;
			default:
				gedit_ranks_menu(d);
				return;
			}
			break;
/*-------------------------------------------------------------------*/
		case GEDIT_NEW_RANK_NUMBER:
			if (atoi(arg) <= 0) {
				write_to_output(d, TRUE, "\r\nIllegal rank number, please try again.\r\n\r\nEnter new rank number : ");
				return;
			} else {
				OLC_VAL(d) = atoi(arg);
				write_to_output(d, TRUE, "\r\nEnter new rank name : ");
				OLC_MODE(d) = GEDIT_NEW_RANK_NAME;
				return;
			}
			break;
		case GEDIT_NEW_RANK_NAME:
			if (!*arg) {
				write_to_output(d, TRUE, "\r\nYou have to give the rank a name.\r\n\r\nEnter new rank name : ");
				return;
			} else {
				struct guild_info *g;
				struct rank_info *rank, *r;
				struct guildie_info *guildie;
				/*
				 * Everything went fine, go ahead and add the rank.
				 * Torgny Bjers (artovil@arcanerealms.org), 2002-06-29.
				 */
				g = OLC_GUILD(d);
				CREATE(rank, struct rank_info, 1);
				rank->num = OLC_VAL(d);
				rank->name = str_dup(arg);
				if (!g->ranks)
					g->ranks = rank;
				else {
					r = g->ranks;
					while (r) {
						if (r->num >= OLC_VAL(d)) break;
						r = r->next;
					}
					if (!r) {
						r = g->ranks;
						while (r->next)
							r = r->next;
						r->next = rank;
					}
					else if (g->ranks == r) {
						rank->next = r;
						g->ranks = rank;
					}
					else {
						rank->next = r;
						r = g->ranks;
						while (r->next != rank->next)
							r = r->next;
						r->next = rank;
					}
				}
				r = rank->next;
				while (r) {
					r->num += 1;
					r = r->next;
				}
				guildie = g->guildies;
				while (guildie) {
					if (guildie->rank_num >= OLC_VAL(d))
						guildie->rank_num += 1;
					guildie = guildie->next;
				}
				gedit_ranks_menu(d);
				return;
			}
			break;
		case GEDIT_DELETE_RANK:
			if (atoi(arg) <= 0) {
				write_to_output(d, TRUE, "\r\nIllegal rank number, please try again.\r\n\r\nDelete which rank? : ");
				return;
			} else {
				struct guild_info *g;
				struct rank_info *rank, *r, *r2;
				struct guildie_info *guildie;
				int value;
				/*
				 * Everything went fine, go ahead and delete the rank.
				 * There has to be a memory leak down here, since no call
				 * free() are made anywhere when the rank is deleted.
				 * Torgny Bjers (artovil@arcanerealms.org), 2002-06-29.
				 */
				g = OLC_GUILD(d);
				value = atoi(arg);
				rank = g->ranks;
				while (rank) {
					if (rank->num == value) break;
					rank = rank->next;
				}
				if (!rank) {
					write_to_output(d, TRUE, "\r\nNo such rank.\r\n\r\n");
					gedit_ranks_menu(d);
					return;
				}
				if (g->ranks == rank) {
					g->ranks = g->ranks->next;
					r = g->ranks;
				}
				else {
					r = g->ranks;
					while (r->next != rank)
						r = r->next;
					r->next = rank->next;
					r = r->next;
				}
				r2 = r;
				while (r2) {
					r2->num -= 1;
					r2 = r2->next;
				}
				guildie = g->guildies;
				while (guildie) {
					if (guildie->rank_num >= value)
						guildie->rank_num -= 1;
					if (guildie->rank == rank) {
						if (r) {
							guildie->rank = r;
							guildie->rank_num = r->num;
						}
						else guildie->rank_num = 0;
					}
					guildie = guildie->next;
				}
				gedit_ranks_menu(d);
				return;
			}
			break;
/*-------------------------------------------------------------------*/
		case GEDIT_ZONES_MENU:
			switch (*arg) {
			case 'a':
			case 'A':
				write_to_output(d, TRUE, "\r\nEnter new zone number : ");
				OLC_MODE(d) = GEDIT_NEW_ZONE;
				return;
			case 'd':
			case 'D':
				write_to_output(d, TRUE, "\r\nDelete which zone? : ");
				OLC_MODE(d) = GEDIT_DELETE_ZONE;
				return;
			case 'q':
			case 'Q':
				break;
			default:
				gedit_zones_menu(d);
				return;
			}
			break;
/*-------------------------------------------------------------------*/
		case GEDIT_NEW_ZONE:
			if ((real_zone(atoi(arg))) != NOWHERE) {
				struct guild_info *g;
				struct gzone_info *gzone;
				g = OLC_GUILD(d);
				CREATE(gzone, struct gzone_info, 1);
				gzone->next = g->gzones;
				g->gzones = gzone;
				gzone->zone = atoi(arg);
				gedit_zones_menu(d);
				return;
			} else {
				write_to_output(d, TRUE, "\r\nThat's not a zone!\r\n\r\nEnter new zone number : ");
				return;
			}
			break;
		case GEDIT_DELETE_ZONE:
			if ((atoi(arg)) >= 0) {
				struct guild_info *g;
				struct gzone_info *gzone;
				int value = atoi(arg);
				g = OLC_GUILD(d);
				gzone = g->gzones;
				while (gzone) {
					if (gzone->zone == value) break;
					gzone = gzone->next;
				}
				if (!gzone) {
					write_to_output(d, TRUE, "\r\nNo such zone.\r\n");
					gedit_zones_menu(d);
					return;
				}
				if (g->gzones->zone == value)
					g->gzones = g->gzones->next;
				else {
					gzone = g->gzones; 
					while (gzone->next->zone != value)
						gzone = gzone->next;
					gzone->next = gzone->next->next;
				}
				gedit_zones_menu(d);
				return;
			} else {
				write_to_output(d, TRUE, "\r\nNo such zone.\r\n");
				return;
			}
			break;
/*-------------------------------------------------------------------*/
		case GEDIT_EQUIPMENT_MENU:
			switch (*arg) {
			case 'a':
			case 'A':
				write_to_output(d, TRUE, "\r\nEnter new object number : ");
				OLC_MODE(d) = GEDIT_NEW_EQUIPMENT;
				return;
			case 'd':
			case 'D':
				write_to_output(d, TRUE, "\r\nDelete which object? : ");
				OLC_MODE(d) = GEDIT_DELETE_EQUIPMENT;
				return;
			case 'q':
			case 'Q':
				break;
			default:
				gedit_equipment_menu(d);
				return;
			}
			break;
/*-------------------------------------------------------------------*/
		case GEDIT_NEW_EQUIPMENT:
			if ((real_object(atoi(arg))) != NOTHING) {
				struct guild_info *g;
				struct gequip_info *gequip;
				int value = atoi(arg);
				g = OLC_GUILD(d);
				CREATE(gequip, struct gequip_info, 1);
				gequip->next = g->gequip;
				g->gequip = gequip;
				gequip->vnum = value;
				gedit_equipment_menu(d);
				return;
			} else {
				write_to_output(d, TRUE, "\r\nThat's not an object!\r\n\r\nEnter new object number : ");
				return;
			}
			break;
		case GEDIT_DELETE_EQUIPMENT:
			if ((atoi(arg)) >= 0) {
				struct guild_info *g;
				struct gequip_info *gequip;
				int value = atoi(arg);
				g = OLC_GUILD(d);
				gequip = g->gequip;
				while (gequip) {
					if (gequip->vnum == value) break;
					gequip = gequip->next;
				}
				if (!gequip) {
					write_to_output(d, TRUE, "\r\nNo such object.\r\n");
					return;
				}
				if (g->gequip->vnum == value)
					g->gequip = g->gequip->next;
				else {
					gequip = g->gequip;
					while (gequip->next->vnum != value)
						gequip = gequip->next;
					gequip->next = gequip->next->next;
				}
				gedit_equipment_menu(d);
				return;
			} else {
				write_to_output(d, TRUE, "\r\nNo such object.\r\n");
				return;
			}
			break;
/*-------------------------------------------------------------------*/
		default:
			cleanup_olc(d, CLEANUP_ALL);
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: gedit_parse(): Reached default case!");
			write_to_output(d, TRUE, "Oops...\r\n");
			break;
	}
/*-------------------------------------------------------------------*/

/*
 * END OF CASE 
 * If we get here, we have probably changed something, and now want to
 * return to main menu.  Use OLC_VAL as a 'has changed' flag  
 */

	OLC_VAL(d) = TRUE;
	gedit_disp_menu(d);
}

void gedit_string_cleanup(struct descriptor_data *d, int terminator)
{
	switch (OLC_MODE(d)) {
	case GEDIT_REQUIREMENTS:
	case GEDIT_DESCRIPTION:
	case GEDIT_GOSSIP:
		gedit_disp_menu(d);
		break;
	}
}
