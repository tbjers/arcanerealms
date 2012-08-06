/* ************************************************************************
*  File: guild_parser.c                                 Part of CircleMUD *
*	 Usage: header file for database handling                               *
*																																					*
*  Created: Sat May 13 2000 by Yohay Etsion (yohay_e@netvision.net.il)    *
*  This is part of Dragons Fang, which is based on CircleMUD              *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  MySQL C API connection for world files and various former XML base d   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001-2002, Torgny Bjers.                                  *
*                                                                         *
*  Converted: 2002-06-26, by Torgny Bjers (artovil@arcanerealms.org)      *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
************************************************************************ */
/* $Id: guild_parser.c,v 1.4 2002/06/29 18:52:22 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "events.h"
#include "db.h"
#include "guild.h"
#include "guild_parser.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "skills.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "boards.h"

/****** Globals ***************/

struct guild_info *guilds_data;


static void fetchGzoneInfo(struct guild_info *g, int guild)
{
	MYSQL_RES *zones;
	MYSQL_ROW row;
	struct gzone_info *gzone;
	bool error = FALSE;

	/* open MySQL table for extra descriptions */
	if (!(zones = mysqlGetResource(TABLE_GUILD_ZONES, "SELECT znum FROM %s WHERE guild = %d ORDER BY znum ASC;", TABLE_GUILD_ZONES, guild)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	while ((row = mysql_fetch_row(zones)))
	{
		CREATE(gzone, struct gzone_info, 1);
		gzone->next = g->gzones;
		g->gzones = gzone;
		gzone->zone = atoi(row[0]);
	}
	mysql_free_result(zones);

}


static void fetchGhelpInfo(struct guild_info *g, int guild)
{
	MYSQL_RES *help;
	MYSQL_ROW row;
	struct ghelp_info *ghelp;
	bool error = FALSE;

	/* open MySQL table for extra descriptions */
	if (!(help = mysqlGetResource(TABLE_GUILD_HELP, "SELECT keyword, entry FROM %s WHERE guild = %d ORDER BY keyword ASC;", TABLE_GUILD_HELP, guild)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	while ((row = mysql_fetch_row(help)))
	{
		CREATE(ghelp, struct ghelp_info, 1);
		ghelp->next = g->ghelp;
		g->ghelp = ghelp;
		ghelp->keyword = str_dup(row[0]);
		ghelp->entry = str_dup(row[1]);
	}
	mysql_free_result(help);

}


static void fetchGskillInfo(struct guild_info *g, int guild)
{
	MYSQL_RES *skills;
	MYSQL_ROW row;
	struct gskill_info *gskill;
	bool error = FALSE;

	/* open MySQL table for extra descriptions */
	if (!(skills = mysqlGetResource(TABLE_GUILD_SKILLS, "SELECT skill, maximum_set FROM %s WHERE guild = %d ORDER BY skill ASC;", TABLE_GUILD_SKILLS, guild)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	while ((row = mysql_fetch_row(skills)))
	{
		CREATE(gskill, struct gskill_info, 1);
		gskill->next = g->gskills;
		g->gskills = gskill;
		gskill->skill = atoi(row[0]);
		gskill->maximum_set = atoi(row[1]);
	}
	mysql_free_result(skills);

}


static void fetchRankInfo(struct guild_info *g, int guild)
{
	MYSQL_RES *ranks;
	MYSQL_ROW row;
	struct rank_info *rank, *rank2;
	bool error = FALSE;

	/* open MySQL table for extra descriptions */
	if (!(ranks = mysqlGetResource(TABLE_GUILD_RANKS, "SELECT num, name FROM %s WHERE guild = %d ORDER BY num ASC;", TABLE_GUILD_RANKS, guild)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	while ((row = mysql_fetch_row(ranks)))
	{
		CREATE(rank, struct rank_info, 1);
		if (!g->ranks) {
			g->ranks = rank;
		} else {
			rank2 = g->ranks;
			while (rank2->next != NULL)
				rank2 = rank2->next;
			rank2->next = rank;
		}
		rank->num = atoi(row[0]);
		rank->name = str_dup(row[1]);
	}
	mysql_free_result(ranks);

}


static void fetchGequipInfo(struct guild_info *g, int guild)
{
	struct gequip_info *gequip;
	MYSQL_RES *equipment;
	MYSQL_ROW row;
	bool error = FALSE;

	/* open MySQL table for extra descriptions */
	if (!(equipment = mysqlGetResource(TABLE_GUILD_EQUIPMENT, "SELECT vnum FROM %s WHERE guildie = 0 AND guild = %d ORDER BY vnum ASC;", TABLE_GUILD_EQUIPMENT, guild)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	while ((row = mysql_fetch_row(equipment)))
	{
		CREATE(gequip, struct gequip_info, 1);
		gequip->next = g->gequip;
		g->gequip = gequip;
		gequip->vnum = atoi(row[0]);
	}
	mysql_free_result(equipment);
}


static void fetchGuildieEquipInfo(struct guildie_info *guildie)
{
	struct gequip_info *gequip;
	MYSQL_RES *equipment;
	MYSQL_ROW row;
	bool error = FALSE;

	/* open MySQL table for extra descriptions */
	if (!(equipment = mysqlGetResource(TABLE_GUILD_EQUIPMENT, "SELECT vnum FROM %s WHERE guildie = %d ORDER BY vnum ASC;", TABLE_GUILD_EQUIPMENT, guildie->idnum)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	while ((row = mysql_fetch_row(equipment)))
	{
		CREATE(gequip, struct gequip_info, 1);
		gequip->next = guildie->gequipsent;
		guildie->gequipsent = gequip;
		gequip->vnum = atoi(row[0]);
	}
	mysql_free_result(equipment);
}


static void fetchSponsorerInfo(struct guildie_info *guildie)
{
	struct sponsorer_info *s;
	MYSQL_RES *sponsorer;
	MYSQL_ROW row;
	bool error = FALSE;

	/* open MySQL table for extra descriptions */
	if (!(sponsorer = mysqlGetResource(TABLE_GUILD_SPONSORER, "SELECT idnum, name FROM %s WHERE guildie = %d ORDER BY idnum ASC;", TABLE_GUILD_SPONSORER, guildie->idnum)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	while ((row = mysql_fetch_row(sponsorer)))
	{
		CREATE(s, struct sponsorer_info, 1);
		s->next = guildie->sponsorers;
		guildie->sponsorers = s;
		s->idnum = atoi(row[0]);
		s->name = str_dup(row[1]);
	}
	mysql_free_result(sponsorer);
}


static void fetchGuildieInfo(struct guild_info *g, int guild)
{
	MYSQL_RES *guildies;
	MYSQL_ROW row;
	struct guildie_info *guildie;
	struct rank_info *rank;
	bool error = FALSE;

	/* open MySQL table for extra descriptions */
	if (!(guildies = mysqlGetResource(TABLE_GUILD_GUILDIES, "SELECT * FROM %s WHERE guild = %d ORDER BY idnum ASC;", TABLE_GUILD_GUILDIES, guild)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	while ((row = mysql_fetch_row(guildies)))
	{
		CREATE(guildie, struct guildie_info, 1);
		guildie->next = g->guildies;
		g->guildies = guildie;
		guildie->idnum = atoi(row[1]);
		guildie->name = str_dup(row[3]);
		guildie->rank_num = atoi(row[4]);
		guildie->subrank = str_dup(row[5]);
		guildie->perm = atoi(row[6]);
		guildie->status = atoi(row[7]);
		guildie->deposited = atoi(row[8]);
		guildie->withdrew = atoi(row[9]);
		fetchSponsorerInfo(guildie);
		fetchGuildieEquipInfo(guildie);
		rank = g->ranks;
		while (rank) {
			if (guildie->rank_num == rank->num) {
				guildie->rank = rank;
				rank = 0;
			} else rank = rank->next;
		}
	}
	mysql_free_result(guildies);
}


void GuildToSQL(struct guild_info *g)
{
	struct gskill_info *gskills;
	struct guildie_info *guildies;
	struct gzone_info *gzones;
	struct rank_info *ranks;
	struct gequip_info *gequip;
	struct ghelp_info *ghelp;
	struct sponsorer_info *sponsors;
	int num_sponsors = 0;

	char *esc_name;
	char *esc_subrank;
	char *esc_gossip_name;
	char *esc_gl_title;
	char *esc_guildie_pretitle;
	char *esc_description;
	char *esc_requirements;
	char *esc_gossip;
	char *esc_gchan_name;
	
	const char *index_query = "REPLACE INTO %s "
		"("
			"name, "							// 0
			"gossip_name, "
			"gl_title, "
			"guildie_pretitle, "
			"type, "
			"id, "								// 5
			"gflags, "
			"guildwalk_room, "
			"gold, "
			"description, "
			"requirements, "			// 10
			"gossip, "
			"gchan_name, "
			"gchan_color, "
			"gchan_type"					// 14
		") VALUES ("
			"'%s', "							// 0
			"'%s', "
			"'%s', "
			"'%s', "
			"%d, "
			"%d, "								// 5
			"%d, "
			"%d, "
			"%d, "
			"'%s', "
			"'%s', "							// 10
			"'%s', "
			"'%s', "
			"'%s', "
			"%d"									// 14
		");";
	const char *guildie_query = "INSERT INTO %s "
		"("
			"guild, "
			"idnum, "
			"name, "
			"rank_num, "
			"subrank, "
			"perm, "
			"status, "
			"deposited, "
			"withdrew, "
			"sponsors"
		") VALUES ("
			"%d, "
			"%d, "
			"'%s', "
			"%d, "
			"'%s', "
			"%d, "
			"%d, "
			"%d, "
			"%d, "
			"%d"
		");";
	const char *rank_query = "INSERT INTO %s (guild, num, name) "
		"VALUES (%d, %d, '%s');";
	const char *skills_query = "INSERT INTO %s (guild, skill, maximum_set) "
		"VALUES (%d, %d, %d);";
	const char *help_query = "INSERT INTO %s (guild, keyword, entry) "
		"VALUES (%d, '%s', '%s');";
	const char *zones_query = "INSERT INTO %s (guild, znum) "
		"VALUES (%d, %d);";
	const char *equipment_query = "INSERT INTO %s (guildie, vnum, guild) "
		"VALUES (%d, %d, %d);";
	const char *sponsors_query = "INSERT INTO %s (guildie, idnum, name) "
		"VALUES (%d, %d, '%s');";

	/*
	 * Write down the basic guild information first.
	 */

	SQL_MALLOC(g->name, esc_name);
	SQL_MALLOC(g->gossip_name, esc_gossip_name);
	SQL_MALLOC(g->gl_title, esc_gl_title);
	SQL_MALLOC(g->guildie_titles, esc_guildie_pretitle);
	SQL_MALLOC(g->description, esc_description);
	SQL_MALLOC(g->requirements, esc_requirements);
	SQL_MALLOC(g->gossip, esc_gossip);
	SQL_MALLOC(g->gchan_name, esc_gchan_name);

	SQL_ESC(g->name, esc_name);
	SQL_ESC(g->gossip_name, esc_gossip_name);
	SQL_ESC(g->gl_title, esc_gl_title);
	SQL_ESC(g->guildie_titles, esc_guildie_pretitle);
	SQL_ESC(g->description, esc_description);
	SQL_ESC(g->requirements, esc_requirements);
	SQL_ESC(g->gossip, esc_gossip);
	SQL_ESC(g->gchan_name, esc_gchan_name);

	mysqlWrite(index_query, TABLE_GUILD_INDEX,
		esc_name,
		esc_gossip_name,
		esc_gl_title,
		esc_guildie_pretitle,
		g->type,
		g->id,
		g->gflags,
		g->guildwalk_room,
		g->gold,
		esc_description,
		esc_requirements,
		esc_gossip,
		esc_gchan_name,
		g->gchan_color,
		g->gchan_type
	);

	SQL_FREE(esc_name);
	SQL_FREE(esc_gossip_name);
	SQL_FREE(esc_gl_title);
	SQL_FREE(esc_guildie_pretitle);
	SQL_FREE(esc_description);
	SQL_FREE(esc_requirements);
	SQL_FREE(esc_gossip);
	SQL_FREE(esc_gchan_name);

	gskills = g->gskills;
	guildies = g->guildies;
	gzones = g->gzones;
	ranks = g->ranks;
	gequip = g->gequip;
	ghelp = g->ghelp;

	mysqlWrite("DELETE FROM %s WHERE guild = %d;", TABLE_GUILD_RANKS, g->id);
	while (ranks) {
		SQL_MALLOC(ranks->name, esc_name);
		SQL_ESC(ranks->name, esc_name);
		mysqlWrite(rank_query, TABLE_GUILD_RANKS, g->id, ranks->num, esc_name);
		SQL_FREE(esc_name);
		ranks = ranks->next;
	}

	mysqlWrite("DELETE FROM %s WHERE guild = %d AND guildie = 0;", TABLE_GUILD_EQUIPMENT, g->id);
	while (gequip) {
		mysqlWrite(equipment_query, TABLE_GUILD_EQUIPMENT, 0, gequip->vnum, g->id);
		gequip = gequip->next;
	}

	mysqlWrite("DELETE FROM %s WHERE guild = %d;", TABLE_GUILD_GUILDIES, g->id);
	while (guildies) {
		/*
		 * Write down the guildie sponsors
		 */
		num_sponsors = 0;
		sponsors = guildies->sponsorers;
		mysqlWrite("DELETE FROM %s WHERE guildie = %d;", TABLE_GUILD_SPONSORER, guildies->idnum);
		while (sponsors) {
			SQL_MALLOC(sponsors->name, esc_name);
			SQL_ESC(sponsors->name, esc_name);
			mysqlWrite(sponsors_query, TABLE_GUILD_SPONSORER, guildies->idnum, sponsors->idnum, esc_name);
			SQL_FREE(esc_name);
			sponsors = sponsors->next;
			num_sponsors++;
		}
		/*
		 * Write down basic guildie information
		 */
		SQL_MALLOC(guildies->name, esc_name);
		SQL_MALLOC(guildies->subrank, esc_subrank);
		SQL_ESC(guildies->name, esc_name);
		SQL_ESC(guildies->subrank, esc_subrank);
		mysqlWrite(guildie_query, TABLE_GUILD_GUILDIES,
			g->id,
			guildies->idnum,
			esc_name,
			guildies->rank_num,
			esc_subrank,
			guildies->perm,
			guildies->status,
			guildies->deposited,
			guildies->withdrew,
			num_sponsors
		);
		SQL_FREE(esc_name);
		SQL_FREE(esc_subrank);
		/*
		 * Write down guildie equipment
		 */
		mysqlWrite("DELETE FROM %s WHERE guild = 0 AND guildie = %d;", TABLE_GUILD_EQUIPMENT, guildies->idnum);
		gequip = guildies->gequipsent;
		while (gequip) {
			mysqlWrite(equipment_query, TABLE_GUILD_EQUIPMENT, guildies->idnum, gequip->vnum, 0);
			gequip = gequip->next;
		}
		/*
		 * Move to the next guildie
		 */
		guildies = guildies->next;
	}

	mysqlWrite("DELETE FROM %s WHERE guild = %d;", TABLE_GUILD_SKILLS, g->id);
	while (gskills) {
		mysqlWrite(skills_query, TABLE_GUILD_SKILLS, g->id, gskills->skill, gskills->maximum_set);
		gskills = gskills->next;
	}

	mysqlWrite("DELETE FROM %s WHERE guild = %d;", TABLE_GUILD_ZONES, g->id);
	while (gzones) {
		mysqlWrite(zones_query, TABLE_GUILD_ZONES, g->id, gzones->zone);
		gzones = gzones->next;
	}

	mysqlWrite("DELETE FROM %s WHERE guild = %d;", TABLE_GUILD_HELP, g->id);
	while (ghelp) {
		SQL_MALLOC(ghelp->keyword, esc_name);
		SQL_MALLOC(ghelp->keyword, esc_description);
		SQL_ESC(ghelp->keyword, esc_name);
		SQL_ESC(ghelp->entry, esc_description);
		mysqlWrite(help_query, TABLE_GUILD_HELP, g->id, esc_name, esc_description);
		SQL_FREE(esc_name);		
		SQL_FREE(esc_description);		
		ghelp = ghelp->next;
	}

}


void boot_guilds(MYSQL_RES *content)
{
	MYSQL_ROW content_row;
	struct guild_info *g;
	unsigned long *fieldlength;

	/*
	 * Read through the content result set from query
	 */
	while ((content_row = mysql_fetch_row(content)))
	{
		/*
		 * Allocate memory for the guild and point to the next:
		 */
		CREATE(g, struct guild_info, 1);
		g->next = guilds_data;
		guilds_data = g;
		/*
		 * Basic Guild information
		 */
		fieldlength = mysql_fetch_lengths(content);
		g->name = str_dup(content_row[0]);
		g->gossip_name = NULL_STR(fieldlength[1], content_row[1]);
		g->gl_title = NULL_STR(fieldlength[2], content_row[2]);
		g->guildie_titles = NULL_STR(fieldlength[3], content_row[3]);
		g->type = atoi(content_row[4]);
		g->id = atoi(content_row[5]);
		g->gflags = atoi(content_row[6]);
		g->guildwalk_room = atoi(content_row[7]);
		g->gold = atoi(content_row[8]);
		g->description = NULL_STR(fieldlength[9], content_row[9]);
		g->requirements = NULL_STR(fieldlength[10], content_row[10]);
		g->gossip = NULL_STR(fieldlength[11], content_row[11]);
		g->gchan_name = NULL_STR(fieldlength[12], content_row[12]);
		g->gchan_color = NULL_STR(fieldlength[13], content_row[13]);
		g->gchan_type = atoi(content_row[14]);
		/*
		 * Extended guild information
		 */
		fetchRankInfo(g, g->id);
		fetchGuildieInfo(g, g->id);
		fetchGequipInfo(g, g->id);
		fetchGskillInfo(g, g->id);
		fetchGzoneInfo(g, g->id);
		fetchGhelpInfo(g, g->id);
	}

}
