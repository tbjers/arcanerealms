/* ************************************************************************
*  File: guild.c                                        Part of CircleMUD *
*	 Usage: guild related commands and functions, loading and saving        *
*																																					*
*  Created: Sun May 14 2000 by Yohay Etsion (yohay_e@netvision.net.il)    *
*  This is part of Dragons Fang, which is based on CircleMUD              *
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
*  Converted: 2002-06-26, by Torgny Bjers (artovil@arcanerealms.org)      *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
************************************************************************ */
/* $Id: guild.c,v 1.18 2003/01/03 15:59:18 arcanere Exp $ */

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
#include "oasis.h"

/* External Declarations */

extern struct zone_data *zone_table;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern zone_rnum top_of_zone_table;
extern char *class_abbrevs[];
extern struct spell_info_type spell_info[];
extern obj_rnum top_of_objt;
extern mob_rnum top_of_mobt;

extern struct guild_info *guilds_data;
extern void GuildToSQL(struct guild_info *g);
void extract_char_final(struct char_data *ch);
ACMD(do_look);

/* Misc utilities */

const char *guild_types[17] =
{
	"Undefined",
	"Faction",
	"Organization",
	"Guild",
	"Group",
	"Other",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Undefined",
	"Club",
	"\n"
};

const char *guild_flags[] =
{
	"EXCLUSIVE",
	"IC",
	"GL_TITLE",
	"GUILDIE_TITLE",
	"HAS_GOSSIP",
	"!GCHAN",
	"SECRETIVE",
	"!GLIST",
	"!GWHO",
	"!GUILDWALK",
	"!BANK",
	"DARK",
	"\n"
};

const char *guild_chantypes[] =
{
	"Undefined",
	"Standard",
	"Someone|Subrank",
	"Someone|Subrank + rank",
	"\n"
};


void save_guild(struct guild_info *g)
{
	if (!g)
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "%s:%s(%d): NULL guild pointer.", __FILE__, __FUNCTION__, __LINE__); 
	else
		GuildToSQL(g);
}


void save_all_guilds(void)
{
	struct guild_info *g;

	g = guilds_data;

	while (g) {
		save_guild(g);
		g = g->next;
	}
}


void fetch_char_guild_data(struct char_data *ch)
{
	int r_num, needs_saving = 0;
	struct obj_data *obj;
	struct guild_info *g;
	struct guildie_info *guildie;
	struct char_guild_element *element;
	struct gequip_info *gequip;

	g = guilds_data;

	while (g) {
		guildie = g->guildies;
		while (guildie) {
			if (!strncasecmp(guildie->name, GET_NAME(ch), 
			strlen(guildie->name)) && (GET_IDNUM(ch) == guildie->idnum))
				break;
			guildie = guildie->next;
		}

		if (guildie) {
			needs_saving = 0;
			guildie->ch = ch;
			CREATE(element, struct char_guild_element, 1);
			element->guild = g;
			element->guildie = guildie;
			element->next = GET_CHAR_GUILDS(ch);
			GET_CHAR_GUILDS(ch) = element;
			GET_GUILD(ch) = 1;

			/* For sent gequip */
			gequip = guildie->gequipsent;

			while (gequip) {
				if ((r_num = real_object(gequip->vnum)) >= 0) {
					obj = read_object(r_num, REAL);
					obj_to_char(obj, ch);
					sprintf(buf, "&RYour GL has sent you %s while you were offline.&n\r\n", obj->short_description);
					send_to_char(buf, ch);
				}
				guildie->gequipsent = gequip->next;
				free(gequip);
				gequip = guildie->gequipsent;
				needs_saving = 1;
			}

			if (needs_saving)
				save_guild(g);
		}

		g = g->next;
	}
}


void remove_char_guild_data(struct char_data *ch)
{
	struct char_guild_element *element, *e;

	element = GET_CHAR_GUILDS(ch);

	while (element) {
		e = element;
		element->guildie->ch = NULL;
		element = element->next;
		free(e);
	}
	GET_CHAR_GUILDS(ch) = NULL;
}


int get_guild(struct char_data *ch, int num)
{
	struct char_guild_element *element;

	element = GET_CHAR_GUILDS(ch);
	while (element) {
		if (element->guild->id == num && STATUS_FLAGGED(element->guildie, STATUS_MEMBER)) 
			return 1;
		element = element->next;
	}
	return 0;
}


int get_guild_by_name(struct char_data *ch, char *name)
{
	struct char_guild_element *element = NULL;

	if (!ch || !name)
		return 0;

	for (element = GET_CHAR_GUILDS(ch); element; element = element->next)
		if (!strncasecmp(element->guild->name, name, strlen(name)) && 
			STATUS_FLAGGED(element->guildie, STATUS_MEMBER))
			return 1;

	return 0;
}


int get_guild_by_room(struct char_data *ch, int room) {
	struct char_guild_element *element = NULL;
	struct gzone_info *gz = NULL;

	if (!ch || room < 0 || room > top_of_world)
		return 0;

	for (element = GET_CHAR_GUILDS(ch); element; element = element->next)
		for (gz = element->guild->gzones; gz; gz = gz->next)
			if (zone_table[world[room].zone].number == gz->zone)
				if (STATUS_FLAGGED(element->guildie, STATUS_MEMBER))
					return 1;

	return 0;
}


void do_guild_channel(struct char_data *ch, struct char_guild_element *element, char *argument)
{
	char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
	struct guildie_info *guildie;

	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
		send_to_char("You are muted, therefore you cannot use public channels.\r\n", ch);
		return;
	}

	if (PLR_FLAGGED(ch, PLR_FROZEN) && !IS_IMPL(ch)) {
		send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
		return;
	}

	if (!STATUS_FLAGGED(element->guildie, STATUS_CHAN_ON)) {
		send_to_char("You cannot use this channel while it's off.\r\n", element->guildie->ch);
		return;
	}

	if (!*argument){
		send_to_char("What do you want to say on this channel?\r\n",ch);
		return;
	}

	if (element->guild->gchan_type == GCHAN_FULL) {
		if (!STATUS_FLAGGED(element->guildie, STATUS_SUBRANK)) { 
			sprintf(buf1,"%s %s: &%s%s&n\r\n", 
			((element->guildie->rank_num != 0) ? element->guildie->rank->name : "Unranked"),
			(GLD_FLAGGED(element->guild, GLD_SECRETIVE) ? "Someone" : GET_NAME(ch)), 
			element->guild->gchan_color,
			argument);
			sprintf(buf2,"%s %s: &%s%s&n\r\n", 
			((element->guildie->rank_num != 0) ? element->guildie->rank->name : "Unranked"),
			GET_NAME(ch),
			element->guild->gchan_color,
			argument);
		}
		else {
			sprintf(buf1,"%s [%s] %s: &%s%s&n\r\n", 
			((element->guildie->rank_num != 0) ? element->guildie->rank->name : "Unranked"),
			element->guildie->subrank,
			(GLD_FLAGGED(element->guild, GLD_SECRETIVE) ? "Someone" : GET_NAME(ch)), 
			element->guild->gchan_color, argument);
			sprintf(buf2,"%s [%s] %s: &%s%s&n\r\n", 
			((element->guildie->rank_num != 0) ? element->guildie->rank->name : "Unranked"),
			element->guildie->subrank,
			GET_NAME(ch),
			element->guild->gchan_color, argument); 
		}
	}
	else if (element->guild->gchan_type == GCHAN_SECRETIVE) {
		if (!STATUS_FLAGGED(element->guildie, STATUS_SUBRANK)) {
			sprintf(buf1,"Someone: &%s%s&n\r\n", 
			element->guild->gchan_color,
			argument);
			sprintf(buf2,"Someone: &%s%s&n\r\n", 
			element->guild->gchan_color,
			argument);
		}
		else {
			sprintf(buf1,"%s: &%s%s&n\r\n", 
			element->guildie->subrank,
			element->guild->gchan_color, argument);
			sprintf(buf2,"%s: &%s%s&n\r\n",
			element->guildie->subrank,
			element->guild->gchan_color, argument);
		}
	}
	else if (element->guild->gchan_type == GCHAN_SECRETRANKS) {
		if (!STATUS_FLAGGED(element->guildie, STATUS_SUBRANK)) {
			sprintf(buf1,"%s Someone: &%s%s&n\r\n", 
			((element->guildie->rank_num != 0) ? element->guildie->rank->name : "Unranked"),
			element->guild->gchan_color,
			argument);
			sprintf(buf2,"%s Someone: &%s%s&n\r\n", 
			((element->guildie->rank_num != 0) ? element->guildie->rank->name : "Unranked"),
			element->guild->gchan_color,
			argument);
		}
		else {
			sprintf(buf1,"%s %s: &%s%s&n\r\n", 
			((element->guildie->rank_num != 0) ? element->guildie->rank->name : "Unranked"),
			element->guildie->subrank,
			element->guild->gchan_color, argument);
			sprintf(buf2,"%s %s: &%s%s&n\r\n", 
			((element->guildie->rank_num != 0) ? element->guildie->rank->name : "Unranked"),
			element->guildie->subrank,
			element->guild->gchan_color, argument);
		}
	}
	else sprintf(buf1, "%s: &%s%s&n\r\n", GET_NAME(ch), element->guild->gchan_color, argument);

	guildie = element->guild->guildies;
	while (guildie) {
		buf3[0] = '\0';
		if (STATUS_FLAGGED(guildie, STATUS_TAG) && guildie->ch && !PLR_FLAGGED(guildie->ch, PLR_WRITING)
			&& STATUS_FLAGGED(guildie, STATUS_CHAN_ON) && STATUS_FLAGGED(guildie, STATUS_MEMBER)) {
			sprintf(buf3, "[%s] ", element->guild->gchan_name);
			send_to_char(buf3, guildie->ch);
		}
		if (STATUS_FLAGGED(guildie, STATUS_CHAN_ON) && guildie->ch && !PLR_FLAGGED(guildie->ch, PLR_WRITING)
			&& STATUS_FLAGGED(guildie, STATUS_MEMBER)) {
			if (IS_ADMIN(guildie->ch) || STATUS_FLAGGED(guildie, STATUS_GL))
				send_to_char(buf2, guildie->ch);
			else send_to_char(buf1, guildie->ch);
		}
		guildie = guildie->next;
	}
}


void do_guild_gossip(struct char_data *ch, struct char_guild_element *element, char *argument)
{
	char arg[MAX_INPUT_LENGTH+1];

	one_argument(argument, arg);

	if (!*arg)
		page_string(ch->desc, element->guild->gossip, 1);
	else if (is_abbrev(arg, "write")) {
		if (!PERM_FLAGGED(element->guildie, PERM_GOSSIP_WRITE)) {
			send_to_char("You do not have permission to edit gossip.\r\n", ch);
			return;
		}
		clear_screen(ch->desc);
		send_to_char("Normal editing options apply. Use 'saveguild' afterwards.\r\n", ch);
		write_to_output(ch->desc, TRUE, "%s", stredit_header);
		if (element->guild->gossip)
			write_to_output(ch->desc, FALSE, "%s", element->guild->gossip);
		string_write(ch->desc, &(element->guild->gossip), MAX_STRING_LENGTH, 0, EDIT_GUILD);
		SET_BIT(PLR_FLAGS(ch), PLR_WRITING); 
	}
}


void do_guild_walk(struct char_data *ch, struct char_guild_element *element)
{

	if (!GET_POS(ch) == POS_FIGHTING)
	{
		send_to_char("Oh no you don't! Not while you're fighting.\r\n", ch);
		return;
	}

	if (IS_IC(ch)) 
	{
		send_to_char("Sorry, guildwalking is not allowed when you are IC.\r\n", ch);
		return;
	}

	if (GLD_FLAGGED(element->guild, GLD_NOGUILDWALK)) {
		send_to_char("This guild has no guildwalk room.\r\n", ch);
		return;
	}

	if(element->guild->guildwalk_room == 0) { 
		send_to_char("No guildwalk room set. Contact your GL.\r\n", ch);
		return;
	}

	act("$n departs for $s homeland!",TRUE,ch,0,0,TO_ROOM);

	if ((GET_MOVE(ch)*0.2) > (GET_MOVE(ch) - 300))
		GET_MOVE(ch) = 0.2*GET_MOVE(ch);
	else GET_MOVE(ch) -= 300;
	char_from_room(ch);
	char_to_room(ch,real_room(element->guild->guildwalk_room));
	act("$n arrives from some distant land, weary and tired.",TRUE,ch,0,0,TO_ROOM);
	send_to_char("You have returned to your home, feeling deeply tired.\r\n",ch);
	do_look(ch,"\0",0,SCMD_LOOK);

}


int is_dark(struct char_data *ch)
{
	struct char_guild_element *element;

	element = GET_CHAR_GUILDS(ch);

	while (element) {
		if (GLD_FLAGGED(element->guild, GLD_DARK)) 
			return 1;
		element = element->next;
	}

	return 0;
}


int can_use_gskill(struct char_data *ch, int skill)
{
	struct char_guild_element *element;
	struct gskill_info *gskill;

	if (IS_IMMORTAL(ch))
		return 1;

	element = GET_CHAR_GUILDS(ch);

	while (element) {
		gskill = element->guild->gskills;
		while (gskill) {
			if (gskill->skill == skill)
				return 1;
			gskill = gskill->next;
		}
		element = element->next;
	}

	return 0;
}


int share_guild(struct char_data *ch1, struct char_data *ch2) {
	struct char_guild_element *el1, *el2;

	if (!ch1 || !ch2) return 0;

	for (el1 = GET_CHAR_GUILDS(ch1);el1;el1 = el1->next)
		for(el2 = GET_CHAR_GUILDS(ch2);el2;el2 = el2->next)
			if (el1->guild->id == el2->guild->id) return el1->guild->id;

	return 0;
}


char *get_title(struct char_data *ch)
{
	struct char_guild_element *element;

	/* Check for GL status */
		element = GET_CHAR_GUILDS(ch);
	while (element) {
		if (GLD_FLAGGED(element->guild, GLD_GLTITLE) &&
			STATUS_FLAGGED(element->guildie, STATUS_GL))
			return element->guild->gl_title;
		element = element->next;
	}

	/* Check for titles guildie status */
		element = GET_CHAR_GUILDS(ch);
	while (element) {
		if (GLD_FLAGGED(element->guild, GLD_GUILDIESTITLE) && 
			STATUS_FLAGGED(element->guildie, STATUS_MEMBER))
			return element->guild->guildie_titles;
		element = element->next;
	}

	/* No title is needed */
	return (NULL);
}


/*
 * Externalized the creation of guilds in order to adapt
 * it to the new gedit OLC.
 * Torgny Bjers (artovil@arcanerealms.org), 2002-06-28
 */
struct guild_info *add_guild(char *name)
{
	struct guild_info *g; 
	int newid; 

	g = guilds_data; 

	if (g) { 

		newid = -1;
		while (g) {
			if (g->id > newid)
				newid = g->id;
			g = g->next;
		}
		newid++;

		g = guilds_data;
		while (g->next) 
			g = g->next; 

		CREATE(g->next, struct guild_info, 1);
		g = g->next; 
	} else { 
		CREATE(guilds_data, struct guild_info, 1);
		g = guilds_data; 
		newid = 1; 
	}

	g->name = str_dup(name);
	g->description = str_dup("No description yet.\r\n"); 
	g->requirements = str_dup("No requirements set yet.\r\n");
	g->gossip = str_dup("No gossip yet.\r\n");
	g->type = 1;
	g->id = newid;
	g->guild_filename = g->name;
	g->gossip_name = NULL;
	g->gl_title = NULL;
	g->guildie_titles = NULL; 
	g->gchan_name = str_dup(name);
	g->gchan_color = str_dup("n");
	g->gchan_type = 1;

	return (g);
}


int rem_guild(int id)
{
	struct guild_info *g, *temp;
	struct descriptor_data *d;
	struct char_guild_element *temp1 = NULL, *temp2 = NULL;

	g = guilds_data;

	while (g) {
		if (g->id == id)
			break;
		if (!strncasecmp(g->name, arg, strlen(g->name)))
			break;
		g = g->next;
	}

	if (!g)
		return (-1);

	for (d = descriptor_list; d; d = d->next)
		if (!d->connected) {
			temp1 = GET_CHAR_GUILDS(d->character);
			if (temp1->guild == g) {
				GET_CHAR_GUILDS(d->character) = temp1->next;
			}
			else {
				temp2 = temp1;
				temp1 = temp1->next;
				while (temp1) {
					if (temp1->guild == g) {
						temp2->next = temp1->next;
						break;
					}
					temp2 = temp1;
					temp1 = temp1->next;
				}
			}
		}

	if (temp1)
		free(temp1);

	if (g == guilds_data) {
		mysqlWrite("DELETE FROM %s WHERE id = %d;", TABLE_GUILD_INDEX, g->id);
		extended_mudlog(BRF, SYSL_GUILDS, TRUE, "The guild \"%s\" was removed.", g->name);
		free(g);	
		return (1);
	}

	temp = guilds_data;
	while (temp->next != g)
		temp = temp->next;

	temp->next = g->next;

	mysqlWrite("DELETE FROM %s WHERE id = %d;", TABLE_GUILD_INDEX, g->id);
	extended_mudlog(BRF, SYSL_GUILDS, TRUE, "The guild \"%s\" was removed.", g->name);
	free(g);

	return (1);
}


/* ACMDs */

ACMD(do_addguild) 
{ 
	struct guild_info *g; 

	if (!*argument) {
		send_to_char("You have to give the new guild a name.\r\n", ch);
		return;
	}

	skip_spaces(&argument); 

	if (!(g = add_guild(argument))) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error adding guild \"%s\".", argument);
	} else {
		extended_mudlog(BRF, SYSL_GUILDS, TRUE, "%s added guild \"%s\".", GET_NAME(ch), g->name);
		send_to_char(OK, ch);
		save_all_guilds();
	}
}


ACMD(do_remguild)
{
	int result;

	argument = one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Which guild fo you want to remove?\r\nUse 'remguild <guild number> yes remove guild'.\r\n", ch);
		return;
	}

	if (strcasecmp(argument, " yes remove guild")) {
		send_to_char("We don't want any accidents here.. Use 'remguild <num> yes remove guild'.\r\n", ch);
		return;
	}

	result = rem_guild(atoi(arg));
	
	if (result == -1) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	send_to_char(OK, ch);
	save_all_guilds();
}


ACMD(do_allguilds)
{ 
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char *groupbuf = get_buffer(MAX_STRING_LENGTH);
	char *contentbuf = get_buffer(MAX_STRING_LENGTH);
	struct guild_info *g;
	int i, num_guilds;

	sprintf(printbuf, 	"&c.-----------------------------------------------------------------------------.\r\n");
	sprintf(printbuf, "%s| &CARCANE REALMS GUILD SYSTEM, listing of all guilds                           &c|\r\n", printbuf);
	sprintf(contentbuf, "&yGuild types:&Y ");
	for (i = 1; i <= MAX_GUILD_TYPES; i++)
		sprintf(contentbuf, "%s%ss%s", contentbuf, guild_types[i], ((i < MAX_GUILD_TYPES) ? ", " : ""));
	sprintf(printbuf, "%s| %-75.75s     &c|\r\n", printbuf, contentbuf);

	for (i = 1; i <= MAX_GUILD_TYPES; i++) {

		g = guilds_data; 

		num_guilds = 0;

		sprintf(groupbuf, "|-----------------------------------------------------------------------------|\r\n");
		sprintf(contentbuf, "&Y%ss:", guild_types[i]);
		sprintf(groupbuf, "%s| %-75.75s   &c|\r\n", groupbuf, contentbuf);

		while (g) {
			if (GLD_TYPE(g) == i && (!GLD_FLAGGED(g, GLD_SECRETIVE) || (GLD_FLAGGED(g, GLD_SECRETIVE) && IS_ADMIN(ch)))) {
				sprintf(contentbuf, "  %s [%d] [%s]", g->name, g->id, (GLD_FLAGGED(g, GLD_IC) ? "IC" : "OOC"));
				sprintf(groupbuf, "%s| &n%-75.75s &c|\r\n", groupbuf, contentbuf);
				num_guilds++;
			}
			g = g->next;
		}

		if (num_guilds > 0)
			strcat(printbuf, groupbuf);

	}

	sprintf(printbuf, "%s'-----------------------------------------------------------------------------'\r\n", printbuf);

	page_string(ch->desc, printbuf, 1);

	release_buffer(printbuf);
	release_buffer(groupbuf);
	release_buffer(contentbuf);

}


ACMD(do_clubs)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char *groupbuf = get_buffer(MAX_STRING_LENGTH);
	char *contentbuf = get_buffer(MAX_STRING_LENGTH);
	struct guild_info *g;
	int num_guilds;

	g = guilds_data;

	sprintf(printbuf, 	"&c.-----------------------------------------------------------------------------.\r\n");
	sprintf(printbuf, "%s| &CARCANE REALMS CLUBS                                                         &c|\r\n", printbuf);
	sprintf(printbuf, "%s|-----------------------------------------------------------------------------|\r\n", printbuf);

	num_guilds = 0;

	while (g) {
		if (GLD_TYPE(g) == GUILD_CLUB && (!GLD_FLAGGED(g, GLD_SECRETIVE) || (GLD_FLAGGED(g, GLD_SECRETIVE) && IS_ADMIN(ch)))) {
			sprintf(contentbuf, "  %s [%d]", g->name, g->id);
			sprintf(groupbuf, "%s| &n%-75.75s &c|\r\n", groupbuf, contentbuf);
			num_guilds++;
		}
		g = g->next;
	}

	if (num_guilds > 0) {
		strcat(printbuf, groupbuf);
	} else {
		sprintf(groupbuf, "| &n%-75.75s &c|\r\n", "There are no clubs at the moment.");
		strcat(printbuf, groupbuf);
	}

	sprintf(printbuf, "%s'-----------------------------------------------------------------------------'\r\n", printbuf);

	page_string(ch->desc, printbuf, 1);

	release_buffer(printbuf);
	release_buffer(groupbuf);
	release_buffer(contentbuf);

}


ACMD(do_myguilds)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char *groupbuf = get_buffer(MAX_STRING_LENGTH);
	char *contentbuf = get_buffer(MAX_STRING_LENGTH);
	struct char_guild_element *element;
	struct guild_info *g;
	struct guildie_info *guildie;
	int bool = 0, num_guilds = 0;

	element = GET_CHAR_GUILDS(ch);

	strcpy(printbuf, "\r\n&CMY GUILDS&n\r\n\r\n");

	num_guilds = 0;

	while (element) {

		num_guilds++;

		g = element->guild;
		guildie = element->guildie;

		sprintf(contentbuf, "%s [%s] [%d] [%s]", g->name, guild_types[g->type], g->id, (GLD_FLAGGED(g, GLD_IC) ? "IC" : "OOC"));
		if (STATUS_FLAGGED(guildie, STATUS_SEEKING)) 
			sprintf(contentbuf, "%s seeking", contentbuf);
		if (STATUS_FLAGGED(guildie, STATUS_GL)) 
			sprintf(contentbuf, "%s [GL]", contentbuf);
		if (STATUS_FLAGGED(guildie, STATUS_MEMBER))
			sprintf(contentbuf, "%s [&%s%s&n%s]", contentbuf, g->gchan_color, g->gchan_name,
			(STATUS_FLAGGED(guildie, STATUS_CHAN_ON) ? " (gchan on)" : ""));
		if (guildie->perm && !STATUS_FLAGGED(guildie, STATUS_GL)) {
			sprintf(contentbuf, "%s [ ", contentbuf);
			if (PERM_FLAGGED(guildie, PERM_GUILD))
				sprintf(contentbuf, "%sguild ", contentbuf);
			if (PERM_FLAGGED(guildie, PERM_WITHDRAW))
				sprintf(contentbuf, "%swithdraw ", contentbuf);
			if (PERM_FLAGGED(guildie, PERM_GOSSIP_WRITE))
				sprintf(contentbuf, "%sgossip ", contentbuf);
			if (PERM_FLAGGED(guildie, PERM_SPONSOR))
				sprintf(contentbuf, "%ssponsor ", contentbuf);
			if (PERM_FLAGGED(guildie, PERM_AUTHORIZE))
				sprintf(contentbuf, "%sauthorize ", contentbuf);
			if (PERM_FLAGGED(guildie, PERM_GSKILLSET))
				sprintf(contentbuf, "%sgskillset ", contentbuf);
			sprintf(contentbuf, "%s]", contentbuf);
		}
		sprintf(printbuf, "%s%s\r\n", printbuf, contentbuf);

		if (guildie->rank_num != 0) {
			sprintf(groupbuf, "    [Rank: %s (%ld)]", guildie->rank->name, guildie->rank_num);
			bool = 1;
		}

		if (STATUS_FLAGGED(guildie, STATUS_SUBRANK)) {
			sprintf(groupbuf, "    [Subrank: %s]", guildie->subrank);
			bool = 1;
		}

		if (guildie->deposited || guildie->withdrew) { 
			sprintf(groupbuf, "    [dep:%ld with:%ld]", guildie->deposited, guildie->withdrew);
			bool = 1;
		}

		if (bool == 1) {
			sprintf(contentbuf, "%s\r\n", groupbuf);
			strcat(printbuf, contentbuf);
		}

		strcat(printbuf, "\r\n");

		element = element->next;
	}

	if (num_guilds == 0) {
		strcat(printbuf, "You don't have any guilds.\r\n");
	}

	page_string(ch->desc, printbuf, 1);

	release_buffer(printbuf);
	release_buffer(groupbuf);
	release_buffer(contentbuf);

}


ACMD(do_granks)
{
	struct guild_info *g;
	struct rank_info *rank;
	int num;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Ranks of which guild?\r\nUse 'ranks <guild name/num>'.\r\n", ch);
		return;
	}

	num = atoi(arg);
	g = guilds_data;

	while (g) {
		if (g->id == num) break;
		if (!strncasecmp(g->name, arg, strlen(arg))) break;
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	if (GLD_FLAGGED(g, GLD_SECRETIVE) && !IS_ADMIN(ch)) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}	else {
		char *printbuf = get_buffer(MAX_STRING_LENGTH);
		rank = g->ranks;
		sprintf(printbuf, "\r\n&Y%s rank list&n\r\n\r\n", g->name);
		while (rank) {
			sprintf(printbuf, "%s&G%s &g[rank num: &G%ld&g]&n\r\n", printbuf, rank->name, rank->num);
			rank = rank->next;
		}
		page_string(ch->desc, printbuf, 1);
		release_buffer(printbuf);
	}
}


ACMD(do_guildinfo)
{
	struct guild_info *g = NULL;
	struct guildie_info *guildie;
	struct char_guild_element *element;
	int num;

	one_argument(argument, arg);

	if (!*arg) {
		element = GET_CHAR_GUILDS(ch);
		while (element) {
			g = element->guild;
			if (GLD_FLAGGED(g, GLD_EXCLUSIVE))
				break;
			element = element->next;
		}

		if (!element) {
			send_to_char("Guild information on which guild?\r\nUse 'guildinfo <guild number/name>'.\r\n", ch);
			return;
		}
	}

	if (*arg) {

		g = guilds_data;
		num = atoi(arg);

		while (g) {
			if (g->id == num) break;
			if (!strncasecmp(g->name, arg, strlen(arg))) break;
			g = g->next;
		}

		if (!g) {
			send_to_char("No such guild.\r\n", ch);
			return;
		}
	}

	if (GLD_FLAGGED(g, GLD_SECRETIVE) && !IS_ADMIN(ch)) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	guildie = g->guildies;

	sprintf(buf, "\r\n&C%s &cinformation&n\r\n\r\n", g->name);

	sprintf(buf, "%sNumber: %d, Type: %s/%s, GL title: %s, Guildies titles: %s\r\n",
	buf, g->id, 
	guild_types[g->type],
	(GLD_FLAGGED(g, GLD_IC) ? "IC" : "OOC"), 
	(GLD_FLAGGED(g, GLD_GLTITLE) ? g->gl_title : "None"),
	(GLD_FLAGGED(g, GLD_GUILDIESTITLE) ? g->guildie_titles : "None"));

	sprintf(buf, "%sGuild channel: %s, Exclusive: %s\r\n", buf, g->gchan_name,
	(GLD_FLAGGED(g, GLD_EXCLUSIVE) ? "&RYes&n" : "&GNo&n"));

	sprintf(buf, "%sGuild gossip: %s\r\n", buf,
	(GLD_FLAGGED(g, GLD_GOSSIP) ? g->gossip_name : "None"));

	sprintf(buf, "%sGuildleaders: ", buf);

	while (guildie) {
		if (STATUS_FLAGGED(guildie, STATUS_GL)) 
			sprintf(buf, "%s%s ", buf, guildie->name);
		guildie = guildie->next;
	}

	if (g->description) {
		sprintf(buf, "%s\r\n\r\nDescription:\r\n\r\n", buf);
		sprintf(buf, "%s%s", buf, g->description);
	}

	if (g->requirements) {
		sprintf(buf, "%s\r\n\r\nRequirements:\r\n\r\n", buf);
		sprintf(buf, "%s%s", buf, g->requirements);
	}

	page_string(ch->desc,buf,1);
}


ACMD(do_gadmininfo)
{
	struct guild_info *g = NULL;
	struct guildie_info *guildie;
	struct gskill_info *gskill;
	struct gequip_info *gequip;
	struct gzone_info *gzone;
	struct rank_info *rank;
	struct char_guild_element *element;
	int num, i, rnum;
	struct obj_data *obj;

	one_argument(argument, arg);

	if (!*arg) {

		element = GET_CHAR_GUILDS(ch);
		while (element) {
			guildie = element->guildie;
			if (STATUS_FLAGGED(guildie, STATUS_GL)) break;
			element = element->next;
		}

		if (!element) {
			send_to_char("Admin information on which guild?\r\nUse 'gadmininfo <guild number/name>'.\r\n", ch);
			return;
		}

		g = element->guild;
	}

	if (*arg) {

		num = atoi(arg);
		g = guilds_data;

		while (g) {
			if (g->id == num) break;
			if (!strncasecmp(g->name, arg, strlen(arg))) break;
			g = g->next;
		}

		if (!g) {
			send_to_char("No such guild.\r\n", ch);
			return;
		}

		if (!IS_ADMIN(ch)) {
			element = GET_CHAR_GUILDS(ch);
			while (element) {
				if (element->guild == g) break;
				element = element->next;
			}

			if (!element) {
				send_to_char("You are not a part of this guild.\r\n", ch);
				return;
			}

			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) {
				send_to_char("Are you a guildleader?!\r\n", ch);
				return;
			}
		}
	}

	rank = g->ranks;
	gskill = g->gskills;
	gequip = g->gequip;
	gzone = g->gzones;

	sprintf(buf, "\r\n&C%s &cadmin information&n\r\n\r\n", g->name);

	if (!GLD_FLAGGED(g, GLD_NOGCHAN))
		sprintf(buf, "%sChannel color: %s, Channel type: %d\r\n", buf, g->gchan_color, g->gchan_type);
	if (!GLD_FLAGGED(g, GLD_NOGUILDWALK))
		sprintf(buf, "%sGuildwalk room: %d\r\n", buf, g->guildwalk_room); 
	if (!GLD_FLAGGED(g, GLD_NOBANK))
		sprintf(buf, "%sMoney in bank account: %d\r\n", buf, g->gold); 

	sprintf(buf, "%s\r\n", buf);
	if (g->gflags) {
		sprintf(buf, "%s&CGuild flags:&n", buf);
		if (GLD_FLAGGED(g, GLD_EXCLUSIVE))
			sprintf(buf, "%s Exclusive", buf);
		if (GLD_FLAGGED(g, GLD_IC))
			sprintf(buf, "%s IC", buf);
		if (GLD_FLAGGED(g, GLD_GLTITLE))
			sprintf(buf, "%s GLtitle", buf);
		if (GLD_FLAGGED(g, GLD_GUILDIESTITLE))
			sprintf(buf, "%s Guildiestitle", buf);
		if (GLD_FLAGGED(g, GLD_GOSSIP))
			sprintf(buf, "%s Gossip", buf);
		if (GLD_FLAGGED(g, GLD_NOGCHAN))
			sprintf(buf, "%s Nogchan", buf);
		if (GLD_FLAGGED(g, GLD_SECRETIVE))
			sprintf(buf, "%s Secretive", buf);
		if (GLD_FLAGGED(g, GLD_NOGLIST))
			sprintf(buf, "%s Noglist", buf);
		if (GLD_FLAGGED(g, GLD_NOGWHO))
			sprintf(buf, "%s Nogwho", buf);
		if (GLD_FLAGGED(g, GLD_NOGUILDWALK))
			sprintf(buf, "%s Noguildwalk", buf);
		if (GLD_FLAGGED(g, GLD_NOBANK))
			sprintf(buf, "%s Nobank", buf);
		sprintf(buf, "%s\r\n\r\n", buf);
	}

	if (rank) { 
		sprintf(buf, "%s&CRanks:&n\r\n\r\n", buf);
		while (rank) {
			sprintf(buf, "%s%s [rank num: %ld]\r\n", buf, rank->name, rank->num);
			rank = rank->next;
		}
		sprintf(buf, "%s\r\n", buf);
	}

	if (gskill) { 
		sprintf(buf, "%s&CGskills:&n\r\n\r\n", buf);
		while (gskill) {
			sprintf(buf, "%s%s [skillnum: %d] [maximum set: %d]\r\n", buf, 
			spell_info[gskill->skill].name,
			gskill->skill, gskill->maximum_set);
			gskill = gskill->next;
		}
		sprintf(buf, "%s\r\n", buf);
	}

	if (gequip) { 
		sprintf(buf, "%s&CGequipment:&n\r\n\r\n", buf);
		while (gequip) {
			if ((rnum = real_object(gequip->vnum)) < 0) {
				send_to_char("There is no object with that number.\r\n", ch);
				return;
			}
			obj = read_object(rnum, REAL);
			sprintf(buf, "%s%s [vnum: %d]\r\n", buf, obj->short_description, gequip->vnum);
			gequip = gequip->next;
		}
		sprintf(buf, "%s\r\n", buf);
	}

	if (gzone) { 
		sprintf(buf, "%s&CGzones:&n\r\n\r\n", buf);
		while (gzone) {
			for (i = 0; zone_table[i].number != gzone->zone && i <= top_of_zone_table; i++); 
			if (i <= top_of_zone_table)
				sprintf(buf, "%s%s [zone num: %d]\r\n", buf, zone_table[i].name, gzone->zone); 
			else sprintf(buf, "%sUndefined [zone num: %d]\r\n", buf, gzone->zone);
			gzone = gzone->next;
		}
		sprintf(buf, "%s\r\n", buf);
	}

	page_string(ch->desc,buf,1);

}


ACMD(do_guildieinfo) 
{
	char guild_name[MAX_INPUT_LENGTH], guildie_name[MAX_INPUT_LENGTH];
	int guild_id, bool=0;
	struct guild_info *g, *g2;
	struct char_guild_element *element;
	struct guildie_info *guildie, *guildie2;
	struct gequip_info *gequip;
	struct char_data *vict = NULL;

	two_arguments(argument, guild_name, guildie_name);

	if (!*guild_name || !*guildie_name) {
		send_to_char("Usage: guildieinfo <guild number/name> <guildie name>\r\n", ch);
		return;
	}

	guild_id = atoi(guild_name);

	g = guilds_data;

	while (g) {
		if (g->id == guild_id) break;
		if (!strncasecmp(g->name, guild_name, strlen(guild_name))) break; 
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	if (!IS_ADMIN(ch)) {

		element = GET_CHAR_GUILDS(ch);
		while (element) {
			if (element->guild == g) break;
			element = element->next;
		}

		if (!element) {
			send_to_char("You are not a part of this guild.\r\n", ch);
			return;
		}

		if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) {
			send_to_char("You are not the guildleader of this guild.\r\n", ch);
			return;
		}

	}

	guildie = g->guildies;

	while (guildie) {
		if (!strncasecmp(guildie->name, guildie_name, strlen(guildie->name))) break;
		guildie = guildie->next;
	}

	if (!guildie) {
		send_to_char("No such person in the guild.\r\n", ch);
		return;
	}

	sprintf(buf, "\r\n&C%s &cguildie information&n\r\n", guildie->name);

	g2 = guilds_data;

	while (g2) {
		guildie2 = g2->guildies;
		while (guildie2) {
			if (guildie2->idnum == guildie->idnum) {

				sprintf(buf, "%s\r\n%s [%s] [%d] [%s]", buf, g2->name, 
				guild_types[g2->type], g2->id,
				(GLD_FLAGGED(g2, GLD_IC) ? "IC" : "OOC"));
				if (STATUS_FLAGGED(guildie2, STATUS_SEEKING)) 
					sprintf(buf, "%s &rseeking&n", buf);

				if (STATUS_FLAGGED(guildie2, STATUS_AUTHORIZED))
					sprintf(buf, "%s &Gapproved&n", buf);

				if (STATUS_FLAGGED(guildie2, STATUS_GL)) 
					sprintf(buf, "%s [GL]", buf);

				if (STATUS_FLAGGED(guildie2, STATUS_CHAN_ON))
					sprintf(buf, "%s [gchan on]", buf);

				if (g == g2 || IS_ADMIN(ch)) {
					if (guildie2->rank_num != 0) {
						sprintf(buf, "%s\r\n     [Rank: %s (%ld)]", buf, guildie2->rank->name, guildie2->rank_num);
						bool = 1;
					}
					if (STATUS_FLAGGED(guildie2, STATUS_SUBRANK)) {
						if (!bool)
							sprintf(buf, "%s\r\n    ", buf);
						sprintf(buf, "%s [Subrank: %s]", buf, guildie2->subrank);
					}
				}

				if ((guildie2->perm && g == g2 && !STATUS_FLAGGED(guildie2, STATUS_GL)) ||
					(guildie2->perm && !STATUS_FLAGGED(guildie2, STATUS_GL) && IS_ADMIN(ch))) {
					sprintf(buf, "%s\r\n     &c[ &G", buf);

					if (PERM_FLAGGED(guildie2, PERM_GUILD))
						sprintf(buf, "%sguild ", buf);
					if (PERM_FLAGGED(guildie2, PERM_WITHDRAW))
						sprintf(buf, "%swithdraw ", buf);
					if (PERM_FLAGGED(guildie2, PERM_GOSSIP_WRITE))
						sprintf(buf, "%swrite-gossip ", buf);
					if (PERM_FLAGGED(guildie2, PERM_SPONSOR))
						sprintf(buf, "%ssponsor ", buf);
					if (PERM_FLAGGED(guildie2, PERM_AUTHORIZE))
						sprintf(buf, "%sauthorize ", buf);
					if (PERM_FLAGGED(guildie2, PERM_GSKILLSET))
						sprintf(buf, "%sgskillset ", buf);
					sprintf(buf, "%s&c]&n", buf);
				}

				if ((guildie2->deposited || guildie2->withdrew) && 
					(g == g2 || IS_ADMIN(ch)))
					sprintf(buf, "%s\r\n [dep:%ld with:%ld]", buf, guildie2->deposited, guildie2->withdrew);

				if (guildie2->gequipsent && (g == g2 || IS_ADMIN(ch))) {
					sprintf(buf, "%s\r\n     &GGequipment sent:&n ", buf);
					gequip = guildie2->gequipsent;
					while (gequip) {
						sprintf(buf, "%s&R%d&n ", buf, gequip->vnum);
						gequip = gequip->next;
					}
				}

			}
			guildie2 = guildie2->next;
		}
		g2 = g2->next;
	}

	CREATE(vict, struct char_data, 1);
	clear_char(vict);
	CREATE(vict->player_specials, struct player_special_data, 1);
	if (load_char(guildie->name, vict) >= 0) {
		sprintf(buf, "%s\r\n     [%5ld] %-12s : %-18s : %-20s",
					buf, GET_IDNUM(vict), GET_NAME(vict),
					vict->player_specials->host && *vict->player_specials->host ? vict->player_specials->host : "(NOHOST)",
					ctime(&vict->player.time.logon));
		send_to_char(buf, ch);
		free_char(vict);
	} else {
		free_char(vict);
	}
}

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
if (on) SET_BIT(flagset, flags); \
 else if (off) REMOVE_BIT(flagset, flags); }


ACMD(do_guildset)
{
	int l;
	struct guild_info *g;
	char guild_num[MAX_INPUT_LENGTH];
	char field[MAX_INPUT_LENGTH], val_arg[MAX_INPUT_LENGTH], to_log[MAX_STRING_LENGTH];
	int guild_id, rank_num, gskill_num, on = 0, off = 0, value = 0, do_log = 0;
	char rank_name[MAX_INPUT_LENGTH], gskill_name[MAX_INPUT_LENGTH], to_char[MAX_INPUT_LENGTH];
	struct char_guild_element *element;
	struct gequip_info *gequip;
	struct gskill_info *gskill;
	struct gzone_info *gzone;
	struct rank_info *rank, *r, *r2;
	struct guildie_info *guildie;
	int i, row = 0;

	struct set_struct {
		const char *cmd;
		const bitvector_t rights;
		const char type;
	} fields[] = {
		{	"gl_title",					RIGHTS_ADMIN, MISC}, /* 0 */
		{	"guildie_titles",		RIGHTS_ADMIN, MISC},
		{	"gossip_name",			RIGHTS_ADMIN, MISC},
		{"id",								RIGHTS_ADMIN, NUMBER},
		{"name",							RIGHTS_ADMIN, MISC}, /* 5 */
		{"type",							RIGHTS_ADMIN, NUMBER},
		{"f_exclusive",				RIGHTS_ADMIN, BINARY},
		{"f_ic",							RIGHTS_ADMIN, BINARY},
		{"f_gltitle",					RIGHTS_ADMIN, BINARY},
		{"f_guildiestitle",		RIGHTS_ADMIN, BINARY}, /* 10 */
		{"f_gossip",					RIGHTS_ADMIN, BINARY},
		{"f_nogchan",					RIGHTS_ADMIN, BINARY},
		{"f_secretive",				RIGHTS_MEMBER, BINARY},
		{	"f_noglist",				RIGHTS_MEMBER, BINARY},
		{	"f_nogwho",					RIGHTS_MEMBER, BINARY}, /* 15 */
		{	"gold",							RIGHTS_ADMIN, NUMBER 	},
		{	"guildwalk",				RIGHTS_ADMIN, NUMBER 	},
		{	"gchan_name",				RIGHTS_MEMBER, MISC 	},
		{	"gchan_color",			RIGHTS_MEMBER, MISC 	},
		{	"gchan_type",				RIGHTS_MEMBER, NUMBER 	}, /* 20 */
		{	"gequipadd",				RIGHTS_ADMIN, NUMBER 	},
		{	"gequipremove",			RIGHTS_ADMIN, NUMBER 	},
		{	"gskilladd",				RIGHTS_ADMIN, MISC 	},
		{	"gskillremove",			RIGHTS_ADMIN, MISC 	},
		{	"gzoneadd",					RIGHTS_ADMIN, NUMBER 	}, /* 25 */
		{	"gzoneremove",			RIGHTS_ADMIN, NUMBER 	}, 
		{	"grankadd",					RIGHTS_MEMBER, MISC 	},
		{	"grankremove",			RIGHTS_MEMBER, NUMBER 	},
		{	"f_noguildwalk",		RIGHTS_ADMIN, BINARY 	},
		{	"f_nobank",					RIGHTS_ADMIN, BINARY 	}, /* 30 */
		{	"f_dark",						RIGHTS_ADMIN, BINARY 	},
		{	"grankchange",			RIGHTS_MEMBER, MISC 	},
		{	"\n",								RIGHTS_NONE, MISC 	}
	};

	sprintf(to_log, "%s has guildset ", GET_NAME(ch));
	half_chop(argument, guild_num, buf);
	half_chop(buf, field, buf);
	strcpy(val_arg, buf);

	if (!*guild_num || !*field) {
		char *list = get_buffer(MAX_STRING_LENGTH);
		sprintf(list, "\r\nGuild Set fields available to &W%s&n:\r\n\r\n", GET_NAME(ch));
		for (i = 0; *fields[i].cmd != '\n'; i++) {
			if (!GOT_RIGHTS(ch, fields[i].rights) || fields[i].rights == RIGHTS_NONE)
				continue;
			sprintf(list, "%s%-16.16s  ", list, fields[i].cmd);
			if (row++ % 4 == 3)
				strcat(list, "\r\n");
		}
		if (row % 5)
			strcat(list, "\r\n");
		sprintf(list, "%s\r\nUsage: guildset <guild number/name> <field> <value>\r\n", list);
		page_string(ch->desc, list, TRUE);
		release_buffer(list);
		return;
	}

	guild_id = atoi(guild_num);

	g = guilds_data;

	while (g) {
		if (g->id == guild_id) break;
		if (!strncasecmp(g->name, guild_num, strlen(guild_num))) break; 
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild!\r\n", ch);
		return;
	}

	sprintf(to_log, "%s%s's ", to_log, g->name);

	for (l = 0; *(fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, fields[l].cmd, strlen(field)))
			break;

	sprintf(to_log, "%s%s to", to_log, fields[l].cmd);

	if (!(GOT_RIGHTS(ch, fields[l].rights) || IS_IMPL(ch))) {
		send_to_char("You are not godly enough for that!\r\n", ch);
		return;
	}

	if (!IS_ADMIN(ch)) {

		element = GET_CHAR_GUILDS(ch);
		while (element) {
			if (element->guild == g) break;
			element = element->next;
		}

		if (!element) {
			send_to_char("You are not a part of this guild.\r\n", ch);
			return;
		}

		if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) {
			send_to_char("You are not a guildleader of this guild !\r\n", ch);
			return;
		}

	}

	if (fields[l].type == BINARY) {
		if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes")){
			on = 1;
			sprintf(to_log, "%s on.", to_log);
		}
		else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no")){
			off = 1;
			sprintf(to_log, "%s off.", to_log);
		}
		if (!(on || off)) {
			send_to_char("Value must be on or off.\r\n", ch);
			return;
		}
	}
	else if (fields[l].type == NUMBER) {
		value = atoi(val_arg);
	}

	strcpy(to_char, "Okay.\r\n"); /* can't use OK macro here 'cause of \r\n */
	switch (l) {
	case 0:
		if (val_arg[0] == '\0')
			strcpy(val_arg, "NONE");
		if (strlen(val_arg) > MAX_NAME_LENGTH)
			val_arg[MAX_TITLE_LENGTH] = '\0';
		if (g->gl_title)
			free(g->gl_title);
		g->gl_title = str_dup(val_arg); 
		sprintf(to_log, "%s %s.", to_log, val_arg);
		break;
	case 1:
		if (val_arg[0] == '\0')
			strcpy(val_arg, "NONE");
		if (strlen(val_arg) > MAX_NAME_LENGTH)
			val_arg[MAX_TITLE_LENGTH] = '\0';
		if (g->guildie_titles)
			free(g->guildie_titles);
		g->guildie_titles = str_dup(val_arg); 
		sprintf(to_log, "%s %s.", to_log, val_arg);
		break;
	case 2:
		if (strlen(val_arg) > MAX_NAME_LENGTH)
			val_arg[MAX_TITLE_LENGTH] = '\0';
		if (g->gossip_name)
			free(g->gossip_name);
		g->gossip_name = str_dup(val_arg); 
		sprintf(to_log, "%s %s.", to_log, val_arg);
		break;
	case 3:
		g->id = value;
		sprintf(to_log, "%s %d.", to_log, value);
		break;
	case 4:
		if (val_arg[0] == '\0')
			strcpy(val_arg, "NONE");
		if (strlen(val_arg) > MAX_NAME_LENGTH)
			val_arg[MAX_TITLE_LENGTH] = '\0';
		if (g->name)
			free(g->name);
		g->name = str_dup(val_arg); 
		sprintf(to_log, "%s %s.", to_log, val_arg);
		break;
	case 5:
		g->type = value;
		sprintf(to_log, "%s %d.", to_log, value);
		break;
	case 6:
		SET_OR_REMOVE(g->gflags, GLD_EXCLUSIVE);
		break;
	case 7:
		SET_OR_REMOVE(g->gflags, GLD_IC);
		break;
	case 8:
		SET_OR_REMOVE(g->gflags, GLD_GLTITLE);
		break;
	case 9:
		SET_OR_REMOVE(g->gflags, GLD_GUILDIESTITLE);
		break;
	case 10:
		SET_OR_REMOVE(g->gflags, GLD_GOSSIP);
		break;
	case 11:
		SET_OR_REMOVE(g->gflags, GLD_NOGCHAN);
		break;
	case 12:
		SET_OR_REMOVE(g->gflags, GLD_SECRETIVE);
		break;
	case 13:
		SET_OR_REMOVE(g->gflags, GLD_NOGLIST);
		break;
	case 14:
		SET_OR_REMOVE(g->gflags, GLD_NOGWHO);
		break;
	case 15:
		if (GLD_FLAGGED(g, GLD_NOBANK)) {
			send_to_char("This guild has no bank account.\r\n", ch);
			return;
		}
		g->gold = value;
		sprintf(to_log, "%s %d.", to_log, value);
		break;
	case 16:
		g->guildwalk_room = value;
		sprintf(to_log, "%s %d.", to_log, value);
		break;
	case 17:
		if (val_arg[0] == '\0')
			strcpy(val_arg, "NONE");
		if (strlen(val_arg) > MAX_NAME_LENGTH)
			val_arg[MAX_TITLE_LENGTH] = '\0';
		if (g->gchan_name)
			free(g->gchan_name);
		g->gchan_name = str_dup(val_arg);
		sprintf(to_log, "%s %s.", to_log, val_arg);
		break;
	case 18:
		if (val_arg[0] == '\0')
			strcpy(val_arg, "R");
		if (strlen(val_arg) > MAX_NAME_LENGTH)
			val_arg[MAX_TITLE_LENGTH] = '\0';
		if (g->gchan_color)
			free(g->gchan_color);
		g->gchan_color = str_dup(val_arg);
		sprintf(to_log, "%s %s.", to_log, val_arg);
		break;
	case 19:
		g->gchan_type = value;
		sprintf(to_log, "%s %d.", to_log, value);
		break;
	case 20:
		CREATE(gequip, struct gequip_info, 1);
		gequip->next = g->gequip;
		g->gequip = gequip;
		gequip->vnum = value;
		sprintf(to_log, "%s %d.", to_log, value);
		break;
	case 21:
		gequip = g->gequip;
		while (gequip) {
			if (gequip->vnum == value) break;
			gequip = gequip->next;
		}
		if (!gequip) {
			send_to_char("No such gequip.\r\n", ch);
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
		sprintf(to_log, "%s %d.", to_log, value);
		break;
	case 22:
		CREATE(gskill, struct gskill_info, 1);
		half_chop(val_arg, buf, gskill_name);
		gskill_num = find_skill_num(gskill_name);
		if (gskill_num <= 0) {
			send_to_char("Unrecognized skill.\r\n", ch);
			return;
		}
		gskill->maximum_set = atoi(buf);
		gskill->skill = gskill_num;
		gskill->next = g->gskills;
		g->gskills = gskill;
		sprintf(to_log, "%s %s.", to_log, gskill_name);
		break;
	case 23:
		gskill = g->gskills;
		gskill_num = find_skill_num(val_arg);
		while (gskill) {
			if (gskill->skill == gskill_num) break;
			gskill = gskill->next;
		}
		if (!gskill) {
			send_to_char("No such gskill.\r\n", ch);
			return;
		}
		if (g->gskills->skill == gskill_num) 
			g->gskills = g->gskills->next; 
		else {
			gskill = g->gskills;
			while (gskill->next->skill != gskill_num)
				gskill = gskill->next;
			gskill->next = gskill->next->next;
		}
		sprintf(to_log, "%s %s.", to_log, val_arg);
		break; 
	case 24:
		CREATE(gzone, struct gzone_info, 1);
		gzone->next = g->gzones;
		g->gzones = gzone;
		gzone->zone = value;
		sprintf(to_log, "%s %d.", to_log, value);
		break;
	case 25:
		gzone = g->gzones;
		while (gzone) {
			if (gzone->zone == value) break;
			gzone = gzone->next;
		}
		if (!gzone) {
			send_to_char("No such gzone.\r\n", ch);
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
		sprintf(to_log, "%s %d.", to_log, value);
		break;
	case 26:
		half_chop(val_arg, buf, rank_name);
		rank_num = atoi(buf);
		if (rank_num <= 0) {
			send_to_char("Illegal rank number.\r\n", ch);
			return;
		}
		CREATE(rank, struct rank_info, 1);
		rank->num = rank_num;
		rank->name = str_dup(rank_name);
		if (!g->ranks) 
			g->ranks = rank;
		else {
			r = g->ranks;
			while (r) {
				if (r->num >= rank_num) break;
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
			if (guildie->rank_num >= rank_num)
				guildie->rank_num += 1;
			guildie = guildie->next;
		}
		sprintf(to_log, "%s %s (%d).", to_log, rank_name, rank_num);
		break;
	case 27:
		rank = g->ranks;
		while (rank) {
			if (rank->num == value) break;
			rank = rank->next;
		}
		if (!rank) {
			send_to_char("No such rank.\r\n", ch);
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
		sprintf(to_log, "%s %d.", to_log, value);
		break;
	case 28:
		SET_OR_REMOVE(g->gflags, GLD_NOGUILDWALK);
		break;
	case 29:
		SET_OR_REMOVE(g->gflags, GLD_NOBANK);
		break;
	case 30:
		SET_OR_REMOVE(g->gflags, GLD_DARK);
		break;
	case 31:
		half_chop(val_arg, buf, rank_name);
		if (!*buf || !*rank_name) {
			send_to_char("Usage: guildset <guild> grankchange <rank number> <new rank name>.\r\n", ch);
			return;
		}
		rank_num = atoi(buf);
		if (rank_num <= 0) {
			send_to_char("Illegal rank number.\r\n", ch);
			return;
		}

		rank = g->ranks;
		while (rank) {
			if (rank->num == rank_num) break;
			rank = rank->next;
		}

		if (rank) {
			if (rank->name)
				free(rank->name);
			rank->name = str_dup(rank_name);
			sprintf(to_char, "Rank %ld, changed to %s.\r\n", rank->num, rank->name);
		}
		else sprintf(to_char, "Cannot find that guild rank!\r\n");
		break;
	default:
		sprintf(to_char, "Can't set that!\r\n");
		do_log++;
		break;
	}

	save_guild(g);

	if (!do_log)
		extended_mudlog(NRM, SYSL_GUILDS, TRUE, to_log);


	send_to_char(to_char, ch);
}


ACMD(do_guildieset)
{
	int l;
	struct guild_info *g;
	char guild_name[MAX_INPUT_LENGTH], guildie_name[MAX_INPUT_LENGTH], to_char[MAX_STRING_LENGTH];
	char field[MAX_INPUT_LENGTH], val_arg[MAX_INPUT_LENGTH], to_log[MAX_STRING_LENGTH];
	int guild_id, on = 0, off = 0, value = 0, do_log = 0;
	struct char_guild_element *element = NULL;
	struct guildie_info *guildie;
	struct rank_info *rank;
	struct gequip_info *gequip, *gequip2;
	int i, row = 0;

	struct set_struct {
		const char *cmd;
		const bitvector_t rights;
		const char type;
	} fields[] = {
		{"gchan",						RIGHTS_MEMBER, BINARY 	}, /* 0 */
		{"gl",							RIGHTS_ADMIN, BINARY 	},
		{"perm_guild",			RIGHTS_MEMBER, BINARY 	},
		{"perm_withdraw",		RIGHTS_MEMBER, BINARY 	},
		{"perm_gossip",			RIGHTS_MEMBER, BINARY 	},
		{"perm_sponsor",		RIGHTS_MEMBER, BINARY 	}, /* 5 */
		{"perm_authorize",	RIGHTS_MEMBER, BINARY 	},
		{"rank",						RIGHTS_MEMBER, NUMBER 	},
		{"perm_gskillset",	RIGHTS_MEMBER, BINARY 	},
		{"authorized",			RIGHTS_MEMBER, BINARY 	},
		{"subrank",					RIGHTS_MEMBER, MISC 	}, /* 10 */
		{"gequipsend",			RIGHTS_MEMBER, NUMBER 	},
		{"gequipunsend",		RIGHTS_MEMBER, NUMBER 	},
		{"\n",							RIGHTS_NONE, MISC 	}
	};

	sprintf(to_log, "%s has guildieset ", GET_NAME(ch));
	half_chop(argument, guild_name, buf);
	half_chop(buf, guildie_name, buf);
	half_chop(buf, field, buf);
	strcpy(val_arg, buf);

	if (!*guild_name || !*guildie_name || !*field) {
		char *list = get_buffer(MAX_STRING_LENGTH);
		sprintf(list, "\r\nGuildie Set fields available to &W%s&n:\r\n\r\n", GET_NAME(ch));
		for (i = 0; *fields[i].cmd != '\n'; i++) {
			if (!GOT_RIGHTS(ch, fields[i].rights) || fields[i].rights == RIGHTS_NONE)
				continue;
			sprintf(list, "%s%-16.16s  ", list, fields[i].cmd);
			if (row++ % 4 == 3)
				strcat(list, "\r\n");
		}
		if (row % 5)
			strcat(list, "\r\n");
		sprintf(list, "%s\r\nUsage: guildieset <guild number/name> <guildie name> <field> <value>\r\n", list);
		page_string(ch->desc, list, TRUE);
		release_buffer(list);
		return;
	}

	guild_id = atoi(guild_name);

	g = guilds_data;

	while (g) {
		if (g->id == guild_id) break;
		if (!strncasecmp(g->name, guild_name, strlen(guild_name))) break; 
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	if (!IS_ADMIN(ch)) {

		element = GET_CHAR_GUILDS(ch);
		while (element) {
			if (element->guild == g) break;
			element = element->next;
		}

		if (!element) {
			send_to_char("You are not a part of this guild.\r\n", ch);
			return;
		}

	}

	guildie = g->guildies;

	while (guildie) {
		if (!strncasecmp(guildie->name, guildie_name, strlen(guildie->name))) break;
		guildie = guildie->next;
	}

	if (!guildie) {
		send_to_char("No such person in the guild.\r\n", ch);
		return;
	}

	sprintf(to_log, "%s%s's ", to_log, guildie->name);

	for (l = 0; *(fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, fields[l].cmd, strlen(field)))
			break;

	sprintf(to_log, "%s%s to", to_log, fields[l].cmd);

	if (!(GOT_RIGHTS(ch, fields[l].rights) || IS_IMPL(ch))) {
		send_to_char("You are not godly enough for that!\r\n", ch);
		return;
	}

	if (fields[l].type == BINARY) {
		if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes")){
			on = 1;
			sprintf(to_log, "%s on.", to_log);
		}
		else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no")){
			off = 1;
			sprintf(to_log, "%s off.", to_log);
		}
		if (!(on || off)) {
			send_to_char("Value must be on or off.\r\n", ch);
			return;
		}
	}
	else if (fields[l].type == NUMBER) {
		value = atoi(val_arg);
	}

	strcpy(to_char, "Okay.\r\n"); /* can't use OK macro here 'cause of \r\n */
	switch (l) {
	case 0:
		if (!IS_ADMIN(ch))
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) {
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		SET_OR_REMOVE(guildie->status, STATUS_CHAN_ON);
		break;
	case 1:
		SET_OR_REMOVE(guildie->status, STATUS_GL);
		SET_OR_REMOVE(guildie->perm, PERM_GUILD);
		SET_OR_REMOVE(guildie->perm, PERM_WITHDRAW); 
		SET_OR_REMOVE(guildie->perm, PERM_GOSSIP_WRITE); 
		SET_OR_REMOVE(guildie->perm, PERM_SPONSOR); 
		SET_OR_REMOVE(guildie->perm, PERM_GSKILLSET); 
		SET_OR_REMOVE(guildie->perm, PERM_AUTHORIZE);
		break;
	case 2:
		if (!IS_ADMIN(ch))
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) { 
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		SET_OR_REMOVE(guildie->perm, PERM_GUILD);
		break;
	case 3:
		if (!IS_ADMIN(ch))
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) { 
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		SET_OR_REMOVE(guildie->perm, PERM_WITHDRAW);
		break;
	case 4:
		if (!IS_ADMIN(ch))
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) { 
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		SET_OR_REMOVE(guildie->perm, PERM_GOSSIP_WRITE);
		break;
	case 5:
		if (!IS_ADMIN(ch))
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) { 
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		SET_OR_REMOVE(guildie->perm, PERM_SPONSOR);
		break;
	case 6:
		if (!IS_ADMIN(ch))
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) { 
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		SET_OR_REMOVE(guildie->perm, PERM_AUTHORIZE);
		break;
	case 7:
		if (!IS_ADMIN(ch))
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) { 
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		if (!value) {
			guildie->rank = NULL;
			guildie->rank_num = 0;
			if (guildie->ch) {
				sprintf(buf, "&G%s has just removed your rank.&n\r\n", GET_NAME(ch));
				send_to_char(buf, guildie->ch);
			}
			break;
		}
		rank = g->ranks;
		while (rank) {
			if (rank->num == value) break;
			rank = rank->next;
		}
		if (!rank) {
			send_to_char("No such rank.\r\n", ch);
			return;
		}
		guildie->rank = rank;
		guildie->rank_num = rank->num;
		if (guildie->ch) {
			sprintf(buf, "&G%s has just set your rank to %s.&n\r\n", GET_NAME(ch), rank->name);
			send_to_char(buf, guildie->ch);
		}
		sprintf(to_log, "%s %d.", to_log, value);
		break;
	case 8:
		if (!IS_ADMIN(ch))
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) { 
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		SET_OR_REMOVE(guildie->perm, PERM_GSKILLSET);
		break;
	case 9:
		if (!IS_ADMIN(ch))
			if (!PERM_FLAGGED(element->guildie, PERM_AUTHORIZE)) {
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		SET_OR_REMOVE(guildie->status, STATUS_AUTHORIZED); 
		break;
	case 10:
		if (!IS_ADMIN(ch))
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) {
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		if (val_arg[0] == '\0') {
			strcpy(val_arg, "NONE");
			REMOVE_BIT(guildie->status, STATUS_SUBRANK);
			if (guildie->ch) {
				sprintf(buf, "&G%s has just removed your subrank.&n\r\n", GET_NAME(ch));
				send_to_char(buf, guildie->ch);
			}
			sprintf(to_log, "%s %s.", to_log, val_arg);
			break;
		}
		if (strlen(val_arg) > MAX_NAME_LENGTH)
			val_arg[MAX_TITLE_LENGTH] = '\0';
		guildie->subrank = str_dup(val_arg); 
		SET_BIT(guildie->status, STATUS_SUBRANK);
		if (guildie->ch) {
			sprintf(buf, "&G%s has just set your subrank to %s.&n\r\n", GET_NAME(ch), val_arg);
			send_to_char(buf, guildie->ch);
		}
		sprintf(to_log, "%s %s.", to_log, val_arg);
		break;
	case 11:
		if (!IS_ADMIN(ch))
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) { 
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		if (!value) {
			send_to_char("Yes, but which gequip ? Use 'guildieset <guild> <member> gequipsend <vnum>'.\r\n", ch);
			return;
		}

		gequip = g->gequip;
		while (gequip) {
			if (gequip->vnum == value)
				break;
			gequip = gequip->next;
		}
		if (!gequip) {
			send_to_char("No gequip with such a vnum in your guild.\r\n", ch);
			return;
		}

		CREATE(gequip, struct gequip_info, 1);
		gequip->vnum = value;
		gequip->next = guildie->gequipsent;
		guildie->gequipsent = gequip;

		extended_mudlog(BRF, SYSL_GUILDS, TRUE, "%s %d.", to_log, value);
		do_log = 1;
		break;
	case 12:
		if (!IS_ADMIN(ch))
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) { 
				send_to_char("You can't do that.\r\n", ch);
				return;
			}
		if (!value) {
			send_to_char("Yes, but which gequip ? Use 'guildieset <guild> <member> gequipunsend <vnum>'.\r\n", ch);
			return;
		}
		gequip = guildie->gequipsent;
		while (gequip)
		{
			if (gequip->vnum == value)
				break;
			gequip = gequip->next;
		}
		if (!gequip)
		{
			send_to_char("No such gequip was sent to this guildie.\r\n", ch);
			return;
		}

		if (guildie->gequipsent == gequip)
		{
			guildie->gequipsent = gequip->next;
			free(gequip);
		}
		else 
			{
			gequip2 = guildie->gequipsent;
			while (gequip2->next != gequip)
				gequip2 = gequip2->next;
			gequip2->next = gequip->next;
			free(gequip);
		}
	default:
		sprintf(to_char, "Can't set that!\r\n");
		do_log++;
		break;
	}

	save_guild(g);

	if (!do_log)
		extended_mudlog(NRM, SYSL_GUILDS, TRUE, to_log);

	send_to_char(to_char, ch);
}


ACMD(do_guild)
{
	char arg1[100], arg2[100];
	int num;
	struct guild_info *g, *g2;
	struct char_guild_element *element, *e2;
	struct guildie_info *guildie;
	struct char_data *vict;
	struct char_data *chdata = NULL;

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
	{ 
		send_to_char("Usage: guild <playername> <guild name/id>\r\n", ch);
		return;
	}

	g = guilds_data;
	num = atoi(arg2);

	while (g) {
		if (g->id == num) break;
		if (!strncasecmp(g->name, arg2, strlen(arg2))) break;
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	if (!IS_ADMIN(ch)) {

		element = GET_CHAR_GUILDS(ch);

		while (element) {
			if (element->guild == g) break;
			element = element->next;
		}

		if (!element) {
			send_to_char("You are not a member in this guild !\r\n", ch);
			return;
		}

		if (!PERM_FLAGGED(element->guildie, PERM_GUILD)) {
			send_to_char("You can't guild seekers.\r\n", ch);
			return;
		}
	}

	guildie = g->guildies;

	while (guildie) {
		if (!strncasecmp(guildie->name, arg1, strlen(guildie->name))) 
			break;
		guildie = guildie->next;
	}

	if (guildie) {

		if (STATUS_FLAGGED(guildie, STATUS_MEMBER)) {
			send_to_char("This character is already a member.\r\n", ch);
			return;
		}

		REMOVE_BIT(guildie->status, STATUS_SEEKING);
		REMOVE_BIT(guildie->status, STATUS_AUTHORIZED);
		SET_BIT(guildie->status, STATUS_MEMBER);
		SET_BIT(guildie->status, STATUS_CHAN_ON);

		save_guild(g);

		if (guildie->ch) {
			sprintf(buf, "&RCongratulations! You are now a member of %s.&n\r\n", g->name);
			send_to_char(buf, guildie->ch);
			GET_GUILD(guildie->ch) = 1;
		}

		sprintf(buf, "&R%s is now a member of %s.&n\r\n", guildie->name,
		g->name);
		send_to_char(buf, ch);

		return;
	}

	vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD, 1);
	if (vict) 
	if (!IS_MOB(vict)) {

		g2 = guilds_data;
		while (g2) {
			guildie = g2->guildies;
			while (guildie) {
				if (guildie->idnum == GET_IDNUM(vict) && GLD_FLAGGED(g2, GLD_EXCLUSIVE) && GLD_FLAGGED(g, GLD_EXCLUSIVE))
					if (!IS_ADMIN(ch)) {
						send_to_char("He/She is already a part of an exclusive guild.\r\n", ch);
						return;
					}
				guildie = guildie->next;
			}
			g2 = g2->next;
		}

		CREATE(guildie, struct guildie_info, 1);
		CREATE(e2, struct char_guild_element, 1);

		e2->guild = g; 
		e2->guildie = guildie; 
		e2->next = GET_CHAR_GUILDS(vict); 
		GET_CHAR_GUILDS(vict) = e2;

		guildie->next = g->guildies; 
		g->guildies = guildie;

		SET_BIT(guildie->status, STATUS_MEMBER); 
		SET_BIT(guildie->status, STATUS_CHAN_ON);
		guildie->idnum = GET_IDNUM(vict); 
		guildie->name = str_dup(GET_NAME(vict)); 
		guildie->ch = vict;

		if (guildie->ch) {
			sprintf(buf, "&RCongratulations! You are now a member of %s.&n\r\n", g->name);
			send_to_char(buf, guildie->ch);
			GET_GUILD(guildie->ch) = 1;
		}

		sprintf(buf, "&R%s is now a member of %s.&n\r\n", guildie->name,
		g->name);
		send_to_char(buf, ch);

		return;
	}

	CREATE(chdata, struct char_data, 1);
	clear_char(chdata);
	CREATE(chdata->player_specials, struct player_special_data, 1);
	if (load_char(arg1, chdata) >= 0) {
		char_to_room(chdata, 0);
		g2 = guilds_data;
		while (g2) {
			guildie = g2->guildies;
			while (guildie) {
				if (GLD_FLAGGED(g2, GLD_EXCLUSIVE) && GLD_FLAGGED(g, GLD_EXCLUSIVE))
					if (!IS_ADMIN(chdata)) {
						send_to_char("He/She is already a part of an exclusive guild.\r\n", ch);
						return;
					}
				guildie = guildie->next;
			}
			g2 = g2->next;
		}

		CREATE(guildie, struct guildie_info, 1);
		CREATE(e2, struct char_guild_element, 1);

		e2->guild = g; 
		e2->guildie = guildie; 
		e2->next = GET_CHAR_GUILDS(chdata); 
		GET_CHAR_GUILDS(chdata) = e2;

		guildie->next = g->guildies; 
		g->guildies = guildie;

		SET_BIT(guildie->status, STATUS_MEMBER); 
		SET_BIT(guildie->status, STATUS_CHAN_ON);
		guildie->idnum = GET_IDNUM(chdata); 
		guildie->name = str_dup(GET_NAME(chdata)); 
		guildie->ch = NULL;

		sprintf(buf, "&R%s is now a member of %s.&n\r\n", guildie->name,
		g->name);
		send_to_char(buf, ch);
		extract_char_final(chdata);
	} else {
		send_to_char("There is no such player.\r\n", ch);
		free_char(chdata);
	}
}


ACMD(do_deguild)
{
	char arg1[100], arg2[100];
	int num;
	struct guild_info *g;
	struct char_guild_element *element;
	struct guildie_info *guildie, *guildie2;

	two_arguments(argument, arg1, arg2);

	if (!*argument || !*arg2)
	{ 
		send_to_char("Usage: deguild <playername> <guild name/id>\r\n", ch);
		return;
	}

	g = guilds_data;
	num = atoi(arg2);

	while (g) {
		if (g->id == num) break;
		if (!strncasecmp(g->name, arg2, strlen(arg2))) break;
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	if (!IS_ADMIN(ch)) {

		element = GET_CHAR_GUILDS(ch);

		while (element) {
			if (element->guild == g) break;
			element = element->next;
		}

		if (!element) {
			send_to_char("You are not a member in this guild!\r\n", ch);
			return;
		}

		if (!PERM_FLAGGED(element->guildie, PERM_GUILD)) { 
			send_to_char("You can't deguild members \r\n", ch);
			return;
		}
	}

	guildie = g->guildies;

	while (guildie) {
		if (!strncasecmp(guildie->name, arg1, strlen(guildie->name))) 
			break;
		guildie = guildie->next;
	}

	if (!guildie) {
		send_to_char("No member with such a name.\r\n", ch);
		return;
	}

	if (g->guildies == guildie)
	g->guildies = g->guildies->next;
	else {
		guildie2 = g->guildies;
		while (guildie2->next != guildie)
			guildie2 = guildie2->next;
		guildie2->next = guildie->next;
	}

	save_guild(g);

	if (guildie->ch) {
		sprintf(buf, "&RYou have been deguilded from %s&n.\r\n", g->name);
		send_to_char(buf, guildie->ch);
		if ((GET_CHAR_GUILDS(guildie->ch))->guildie == guildie)
			GET_CHAR_GUILDS(guildie->ch) = (GET_CHAR_GUILDS(guildie->ch))->next;
		else {
			element = GET_CHAR_GUILDS(guildie->ch);
			while (element->next->guildie != guildie)
				element = element->next;
			element->next = element->next->next;
		}
	}

	sprintf(buf, "&R%s is deguilded from %s.&n\r\n", guildie->name,
	g->name);
	send_to_char(buf, ch);

}


ACMD(do_seekguild)
{
	char arg1[100], arg2[100];
	int num;
	struct guild_info *g;
	struct char_guild_element *element;
	struct guildie_info *guildie, *guildie2;

	two_arguments(argument, arg1, arg2);

	if (!*argument || !*arg1)
	{ 
		send_to_char("Usage: seekguild <guild name/id> [off]\r\n", ch);
		return;
	}

	g = guilds_data;
	num = atoi(arg1);

	while (g) {
		if (g->id == num) break;
		if (!strncasecmp(g->name, arg1, strlen(arg1))) break;
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	if (GLD_FLAGGED(g, GLD_SECRETIVE) && !IS_ADMIN(ch)) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	if (*arg2) {
		if (strncasecmp(arg2, "off", strlen("off")) == 0) {
			element = GET_CHAR_GUILDS(ch);
			while (element) {
				if (element->guild == g)
					break;
				element = element->next;
			}

			if (!element) {
				send_to_char("You are not seeking that guild.\r\n", ch);
				return;
			}

			if (!STATUS_FLAGGED(element->guildie, STATUS_SEEKING)) {
				send_to_char("You are already a member of this guild. Ask to be deguilded instead.\r\n", ch);
				return;
			}

			guildie = element->guildie;

			if (g->guildies == guildie)
				g->guildies = g->guildies->next;
			else {
				guildie2 = g->guildies;
				while (guildie2->next != guildie) 
					guildie2 = guildie2->next;
				guildie2->next = guildie->next;
			}

			save_guild(g);

			if ((GET_CHAR_GUILDS(ch))->guildie == guildie) 
				(GET_CHAR_GUILDS(ch)) = (GET_CHAR_GUILDS(ch))->next;
			else {
				element = GET_CHAR_GUILDS(ch);
				while (element->next->guildie != guildie)
					element = element->next;
				element->next = element->next->next;
			}
			sprintf(buf, "You are no longer seeking %s.\r\n", g->name);
			send_to_char(buf, ch);

			/* POSSIBLE MEMLEAK? What happens to all the elements we just put into nothing? */

			return;
		}
	}

	element = GET_CHAR_GUILDS(ch);

	while (element) {
		if (element->guild == g)
			break;
		if (GLD_FLAGGED(element->guild, GLD_EXCLUSIVE) && GLD_FLAGGED(g, GLD_EXCLUSIVE) &&
			!IS_ADMIN(ch)) {
			send_to_char("You can't join more than one exclusive guild.\r\n", ch);
			return;
		}
		element = element->next;
	}

	if (element) {
		if (STATUS_FLAGGED(element->guildie, STATUS_MEMBER)) 
			send_to_char("You are already a member.\r\n", ch);
		if (STATUS_FLAGGED(element->guildie, STATUS_SEEKING))
			send_to_char("You are already seeking.\r\n", ch);
		return;
	}

	CREATE(guildie, struct guildie_info, 1);
	CREATE(element, struct char_guild_element, 1);

	element->guild = g;
	element->guildie = guildie;
	element->next = GET_CHAR_GUILDS(ch);
	GET_CHAR_GUILDS(ch) = element;

	guildie->next = g->guildies;
	g->guildies = guildie;

	SET_BIT(guildie->status, STATUS_SEEKING);
	guildie->idnum = GET_IDNUM(ch);
	guildie->name = str_dup(GET_NAME(ch)); 
	guildie->ch = ch;

	save_guild(g);

	sprintf(buf, "&RYou are now seeking %s.&n\r\n", g->name);
	send_to_char(buf, ch);

}


ACMD(do_seeking)
{
	char guild_name[MAX_INPUT_LENGTH];
	int guild_id, num_seekers = 0;
	struct char_guild_element *element;
	struct guildie_info *guildie;
	struct sponsorer_info *sponsorer;

	one_argument(argument, guild_name);

	if (!*guild_name) {
		send_to_char("Usage: seeking <guild number/name>\r\n", ch);
		return;
	}

	guild_id = atoi(guild_name);

	element = GET_CHAR_GUILDS(ch);
	while (element) {
		if (element->guild->id == guild_id) break;
		if (!strncasecmp(element->guild->name, guild_name, strlen(guild_name))) break; 
		element = element->next;
	}

	if (!element) {
		send_to_char("You are not a part of this guild.\r\n", ch);
		return;
	}

	if (GLD_FLAGGED(element->guild, GLD_SECRETIVE) && !PERM_FLAGGED(element->guildie, PERM_SPONSOR)) {
		send_to_char("You haven't been authorized to sponsor seekers, so you can't.\r\n", ch);
		return;
	}

	sprintf(buf,"\r\n&cPlayers seeking &C%s&n\r\n\r\n", element->guild->name);

	guildie = element->guild->guildies;

	while (guildie) {
		if (STATUS_FLAGGED(guildie, STATUS_SEEKING)) {

			if(guildie->ch)
				sprintf(buf,"%s%s (&gonline)",buf,CAP(guildie->name));
			else
				sprintf(buf,"%s%s (&r01offline)",buf,CAP(guildie->name));

			if (!STATUS_FLAGGED(guildie, STATUS_AUTHORIZED))
				sprintf(buf, "%s (&RUnauthorized&n)", buf);
			else
				sprintf(buf, "%s (&GAuthorized&n)", buf);

			/* Is the player ready to be guilded level-wise or not? */ 
			/* I'll need this done sometimes :p */

			sponsorer = guildie->sponsorers;
			if (sponsorer) {
				sprintf(buf, "%s [ ", buf);
				while (sponsorer) {
					sprintf(buf, "%s%s ", buf, sponsorer->name);
					sponsorer = sponsorer->next;
				}
				sprintf(buf, "%s]", buf);
			}

			sprintf(buf, "%s\r\n", buf);
			num_seekers++;
		}

		guildie = guildie->next;

	}

	if (num_seekers == 0)
		strcat(buf, "No seekers at the moment.\r\n");

	page_string(ch->desc,buf,1);

}


ACMD(do_sponsor)
{
	char arg1[100], arg2[100];
	int num;
	struct guild_info *g;
	struct char_guild_element *element;
	struct guildie_info *guildie;
	struct sponsorer_info *sponsorer;

	two_arguments(argument, arg1, arg2);

	if (!*argument || !*arg2)
	{ 
		send_to_char("Usage: sponsor <playername> <guild name/id>\r\n", ch);
		return;
	}

	g = guilds_data;
	num = atoi(arg2);

	while (g) {
		if (g->id == num) break;
		if (!strncasecmp(g->name, arg2, strlen(arg2))) break;
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	element = GET_CHAR_GUILDS(ch);

	while (element) {
		if (element->guild == g) break;
		element = element->next;
	}

	if (!element) {
		send_to_char("You are not a member in this guild!\r\n", ch);
		return;
	}

	if (!PERM_FLAGGED(element->guildie, PERM_SPONSOR)) {
		send_to_char("You can't sponsor seekers.\r\n", ch);
		return;
	}

	guildie = g->guildies;

	while (guildie) {
		if (!strncasecmp(guildie->name, arg1, strlen(guildie->name))) 
			break;
		guildie = guildie->next;
	}

	if (!guildie) {
		send_to_char("No seeker with such a name.\r\n", ch);
		return;
	}

	if (!STATUS_FLAGGED(guildie, STATUS_SEEKING)) {
		sprintf(buf, "%s is already a member.\r\n", guildie->name);
		send_to_char(buf, ch);
		return;
	}

	CREATE(sponsorer, struct sponsorer_info, 1);
	sponsorer->idnum = GET_IDNUM(ch);
	sponsorer->name = str_dup(GET_NAME(ch));
	sponsorer->next = guildie->sponsorers;
	guildie->sponsorers = sponsorer;

	save_guild(g);

	if (guildie->ch) {
		sprintf(buf, "&RYou are now sponsored by %s.&n\r\n", GET_NAME(ch));
		send_to_char(buf, guildie->ch);
	}

	sprintf(buf, "&RYou are now sponsoring %s.&n\r\n", guildie->name);
	send_to_char(buf, ch);

}


ACMD(do_unsponsor)
{
	char arg1[100], arg2[100];
	int num;
	struct guild_info *g;
	struct char_guild_element *element;
	struct guildie_info *guildie;
	struct sponsorer_info *sponsorer, *spons_2;

	two_arguments(argument, arg1, arg2);

	if (!*argument || !*arg2)
	{ 
		send_to_char("Usage: unsponsor <playername> <guild name/id>\r\n", ch);
		return;
	}

	g = guilds_data;
	num = atoi(arg2);

	while (g) {
		if (g->id == num) break;
		if (!strncasecmp(g->name, arg2, strlen(arg2))) break;
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	element = GET_CHAR_GUILDS(ch);

	while (element) {
		if (element->guild == g) break;
		element = element->next;
	}

	if (!element) {
		send_to_char("You are not a member in this guild!\r\n", ch);
		return;
	}

	if (!PERM_FLAGGED(element->guildie, PERM_SPONSOR)) {
		send_to_char("You can't even sponsor seekers.\r\n", ch);
		return;
	}

	guildie = g->guildies;

	while (guildie) {
		if (!strncasecmp(guildie->name, arg1, strlen(guildie->name))) 
			break;
		guildie = guildie->next;
	}

	if (!guildie) {
		send_to_char("No seeker with such a name.\r\n", ch);
		return;
	}

	if (!STATUS_FLAGGED(guildie, STATUS_SEEKING)) {
		sprintf(buf, "%s is already a member.\r\n", guildie->name);
		send_to_char(buf, ch);
		return;
	}

	if (!(guildie->sponsorers)) {
		sprintf(buf, "%s doesn't have any sponsors!\r\n", guildie->name);
		send_to_char(buf, ch);
		return;
	}

	sponsorer = guildie->sponsorers;
	if (sponsorer->idnum == GET_IDNUM(ch))
	guildie->sponsorers = guildie->sponsorers->next;
	else {
		spons_2 = sponsorer;
		sponsorer = sponsorer->next;
		while (sponsorer) {
			if (sponsorer->idnum == GET_IDNUM(ch)) {
				spons_2->next = sponsorer->next;
				break;
			}
			spons_2 = sponsorer;
			sponsorer = sponsorer->next;
		}
	}

	if (!sponsorer) {
		sprintf(buf, "You are not sponsoring %s.\r\n", guildie->name);
		send_to_char(buf, ch);
		return;
	}

	free(sponsorer->name);
	free(sponsorer);

	save_guild(g);

	if (guildie->ch) {
		sprintf(buf, "&RYou are no longer sponsored by %s.&n\r\n", GET_NAME(ch));
		send_to_char(buf, guildie->ch);
	}

	sprintf(buf, "&RYou are no longer sponsoring %s.&n\r\n", guildie->name);
	send_to_char(buf, ch);

}


ACMD(do_glist)
{
	char guild_name[MAX_INPUT_LENGTH];
	int guild_id, is_gl = 0;
	struct char_guild_element *element;
	struct guildie_info *guildie;
	int not_really_a_member = 0;

	one_argument(argument, guild_name);

	if (!*guild_name) {
		send_to_char("Usage: glist <guild number/name>\r\n", ch);
		return;
	}

	guild_id = atoi(guild_name);

	element = GET_CHAR_GUILDS(ch);
	while (element) {
		if (element->guild->id == guild_id) break;
		if (!strncasecmp(element->guild->name, guild_name, strlen(guild_name))) break; 
		element = element->next;
	}

	/* if you're not in that guild, element == NULL. this will cause a
			 segfault further down... */
	if (!element) {
		if (!IS_ADMIN(ch)) {
			send_to_char("You are not a part of this guild.\r\n", ch);
			return;
		}
		else
			not_really_a_member = 1;
	}

	if (not_really_a_member) {
		do_seekguild(ch, argument, 0, 0);
		element = GET_CHAR_GUILDS(ch);
		while (element) {
			if (element->guild->id == guild_id) break;
			if (!strncasecmp(element->guild->name, guild_name, strlen(guild_name))) break; 
			element = element->next;
		}
	}

	if (element) {
		if (STATUS_FLAGGED(element->guildie, STATUS_GL) || IS_ADMIN(ch))
			is_gl = 1;
	}
	else {
		send_to_char("*oops* It seems something went wrong.\r\n", ch);
		return;
	}

	if (GLD_FLAGGED(element->guild, GLD_NOGLIST) && !is_gl) {
		send_to_char("You can't do that.\r\n", ch);
		return;
	}

	guildie = element->guild->guildies;

	sprintf(buf,"\r\n&C%s &cguildies&n\r\n\r\n", element->guild->name);

	while (guildie) {
		if (STATUS_FLAGGED(guildie, STATUS_MEMBER)) {
			if (!STATUS_FLAGGED(guildie, STATUS_SUBRANK))
				sprintf(buf,"%s%-30s %s\r\n", buf, 
				((GLD_FLAGGED(element->guild, GLD_SECRETIVE) && !is_gl) ? "Someone" : guildie->name),
				((guildie->rank_num != 0) ? guildie->rank->name : "Unranked")); 
			else
				sprintf(buf,"%s%-30s %s [%s]\r\n", buf,
			((GLD_FLAGGED(element->guild, GLD_SECRETIVE) && !is_gl)? "Someone" : guildie->name),
			((guildie->rank_num != 0) ? guildie->rank->name : "Unranked"), 
			guildie->subrank);
		}
		guildie = guildie->next;
	}

	page_string(ch->desc,buf,1);

	if (not_really_a_member) {
		sprintf(buf, "%s %s", (ch->player).name, argument);
		do_deguild(ch, buf, 0, 0);
	}
}


ACMD(do_gdesc)
{
	struct guild_info *g;
	char guild_name[MAX_INPUT_LENGTH];
	int guild_id;
	struct char_guild_element *element;

	one_argument(argument, guild_name);

	if (!*guild_name) {
		send_to_char("Usage: gdesc <guild number/name>\r\n", ch);
		return;
	}

	guild_id = atoi(guild_name);

	g = guilds_data;

	while (g) {
		if (g->id == guild_id) break;
		if (!strncasecmp(g->name, guild_name, strlen(guild_name))) break; 
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	if (!IS_ADMIN(ch)) {

		element = GET_CHAR_GUILDS(ch);
		while (element) {
			if (element->guild == g) break;
			element = element->next;
		}

		if (!element && GLD_FLAGGED(g, GLD_SECRETIVE)) {
			send_to_char("No such guild.\r\n", ch);
			return;
		}
		
		if (!element) {
			send_to_char("You are not a part of this guild.\r\n", ch);
			return;
		}

		if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) {
			send_to_char("You are not a guildleader of this guild!\r\n", ch);
			return;
		}

	}

	clear_screen(ch->desc);
	send_to_char("Normal editing options apply. Use 'saveguild' afterwards.\r\n", ch);
	write_to_output(ch->desc, TRUE, "%s", stredit_header);
	if (g->description)
		write_to_output(ch->desc, FALSE, "%s", g->description);
	string_write(ch->desc, &(g->description), MAX_STRING_LENGTH, 0, EDIT_GUILD);
	SET_BIT(PLR_FLAGS(ch), PLR_WRITING); 
}


ACMD(do_greq)
{
	struct guild_info *g;
	char guild_name[MAX_INPUT_LENGTH];
	int guild_id;
	struct char_guild_element *element;

	one_argument(argument, guild_name);

	if (!*guild_name) {
		send_to_char("Usage: greq <guild number/name>\r\n", ch);
		return;
	}

	guild_id = atoi(guild_name);

	g = guilds_data;

	while (g) {
		if (g->id == guild_id) break;
		if (!strncasecmp(g->name, guild_name, strlen(guild_name))) break; 
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	if (!IS_ADMIN(ch)) {

		element = GET_CHAR_GUILDS(ch);
		while (element) {
			if (element->guild == g) break;
			element = element->next;
		}

		if (!element && GLD_FLAGGED(g, GLD_SECRETIVE)) {
			send_to_char("No such guild.\r\n", ch);
			return;
		}
	
		if (!element) {
			send_to_char("You are not a part of this guild.\r\n", ch);
			return;
		}

		if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) {
			send_to_char("You are not a guildleader of this guild !\r\n", ch);
			return;
		}

	}

	clear_screen(ch->desc);
	send_to_char("Normal editing options apply. Use 'saveguild' afterwards.\r\n", ch);
	write_to_output(ch->desc, TRUE, "%s", stredit_header);
	if (g->requirements)
		write_to_output(ch->desc, FALSE, "%s", g->requirements);
	string_write(ch->desc, &(g->requirements), MAX_STRING_LENGTH, 0, EDIT_GUILD);
	SET_BIT(PLR_FLAGS(ch), PLR_WRITING); 
}


ACMD(do_ghelp)
{
	char guild_name[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH+1];
	char arg2[MAX_INPUT_LENGTH+1];
	int guild_id;
	struct char_guild_element *element;
	struct ghelp_info *ghelp, *ghelp2;

	half_chop(argument, guild_name, buf);
	half_chop(buf, arg1, arg2);

	if (!*guild_name) {
		send_to_char("Usages: 'ghelp <guild name/num>'.\r\n", ch);
		send_to_char("        'ghelp <guild name/num> <ghelp keyword>'.\r\n", ch);
		send_to_char("        'ghelp <guild name/num> write <existing/new ghelp keyword>'.\r\n", ch);
		send_to_char("        'ghelp <guild name/num> remove <existing ghelp keyword>'.\r\n", ch);
		return;
	}

	guild_id = atoi(guild_name);

	element = GET_CHAR_GUILDS(ch);
	while (element) {
		if (element->guild->id == guild_id) break;
		if (!strncasecmp(element->guild->name, guild_name, strlen(guild_name))) break;
		element = element->next;
	}

	if (!element || STATUS_FLAGGED(element->guildie, STATUS_SEEKING)) {
		send_to_char("You are not a part of such a guild.\r\n", ch);
		return;
	}

	ghelp = element->guild->ghelp;

	if (!*arg1) {

		sprintf(buf, "\r\n&C%s &cghelp entries list&n\r\n\r\n", element->guild->name);

		while (ghelp) {
			sprintf(buf1, "%s", ghelp->keyword);
			sprintf(buf, "%s&G%s&n\r\n", buf, ALLCAP(buf1));
			ghelp = ghelp->next;
		}
		page_string(ch->desc, buf, 1);
		return;
	}

	if (!*arg2) {
		while (ghelp) {
			if (!strncasecmp(ghelp->keyword, arg1, strlen(ghelp->keyword))) break;
			ghelp = ghelp->next;
		}
		if (!ghelp) {
			ghelp = element->guild->ghelp;
			while (ghelp) {
				if (!strncasecmp(ghelp->keyword, arg1, strlen(arg1))) break;
				ghelp = ghelp->next;
			}
		}
		if (!ghelp) {
			send_to_char("No such ghelp entry.\r\n", ch);
			return;
		}
		sprintf(buf1, "%s", ghelp->keyword);
		sprintf(buf, "\r\n&C%s&n\r\n\r\n%s&n", ALLCAP(buf1), ghelp->entry);
		page_string(ch->desc, buf, 1);
		return;
	}

	if (is_abbrev(arg1, "write")) {
		if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) {
			send_to_char("You can't write ghelp.\r\n", ch);
			return;
		}
		if (!*arg2) {
			send_to_char("Want to write which ghelp?\r\n", ch);
			return;
		}
		while (ghelp) {
			if (!strncasecmp(ghelp->keyword, arg2, strlen(arg2))) break;
			ghelp = ghelp->next;
		}
		if (ghelp) { 
			clear_screen(ch->desc);
			send_to_char("Normal editing options apply. Use 'saveguild' afterwards.\r\n", ch);
			write_to_output(ch->desc, TRUE, "%s", stredit_header);
			if (ghelp->entry)
				write_to_output(ch->desc, FALSE, "%s", ghelp->entry);
			string_write(ch->desc, &(ghelp->entry), MAX_STRING_LENGTH, 0, EDIT_GUILD);
			SET_BIT(PLR_FLAGS(ch), PLR_WRITING); 
			return;
		}
		CREATE(ghelp, struct ghelp_info, 1);
		ghelp->keyword = str_dup(arg2);
		ghelp->next = element->guild->ghelp;
		element->guild->ghelp = ghelp;
		clear_screen(ch->desc);
		send_to_char("Normal editing options apply. Use 'saveguild' afterwards.\r\n", ch);
		write_to_output(ch->desc, TRUE, "%s", stredit_header);
		if (ghelp->entry)
			write_to_output(ch->desc, FALSE, "%s", ghelp->entry);
		string_write(ch->desc, &(ghelp->entry), MAX_STRING_LENGTH, 0, EDIT_GUILD);
		SET_BIT(PLR_FLAGS(ch), PLR_WRITING); 
		return;
	}

	if (is_abbrev(arg1, "remove")) {
		if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) {
			send_to_char("You can't remove ghelp.\r\n", ch);
			return;
		}
		if (!*arg2) {
			send_to_char("Wanna remove which ghelp ?\r\n", ch);
			return;
		}
		while (ghelp) {
			if (!strncasecmp(ghelp->keyword, arg2, strlen(arg2))) break;
			ghelp = ghelp->next;
		}
		if (!ghelp) {
			send_to_char("No such ghelp in that guild, sorry.\r\n", ch);
			return;
		}
		if (element->guild->ghelp == ghelp) { 
			element->guild->ghelp = ghelp->next;
			send_to_char("Done.\r\n", ch);
			return;
		}
		ghelp2 = element->guild->ghelp;
		while (ghelp2->next != ghelp)
			ghelp2 = ghelp2->next;
		ghelp2->next = ghelp->next;
		send_to_char("Done.\r\n", ch);
		return;
	}
	send_to_char("Huh?!\r\n", ch);
}


ACMD(do_saveguild)
{
	char arg1[100];
	int num;
	struct guild_info *g;
	struct char_guild_element *element;

	one_argument(argument, arg1);

	if (!*arg1)
	{ 
		send_to_char("Usage: saveguild <guild name/id>\r\n", ch);
		return;
	}

	g = guilds_data;
	num = atoi(arg1);

	while (g) {
		if (g->id == num) break;
		if (!strncasecmp(g->name, arg1, strlen(arg1))) break;
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild!\r\n", ch);
		return;
	}

	element = GET_CHAR_GUILDS(ch);

	while (element) {
		if (element->guild == g) break;
		element = element->next;
	}

	if (!element && !IS_ADMIN(ch)) {
		send_to_char("You can't do that!\r\n", ch);
		return;
	}

	if (!IS_ADMIN(ch))
	if (!PERM_FLAGGED(element->guildie, PERM_GOSSIP_WRITE)) {
		send_to_char("You can't do that!\r\n", ch);
		return;
	}

	save_guild(g);

	send_to_char("Done.\r\n", ch);

}


ACMD(do_gdeposit)
{
	char arg1[100], arg2[100];
	int amount, num;
	struct guild_info *g;
	struct char_guild_element *element;

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
	{ 
		send_to_char("Usage: gdeposit <amount> <guild name/id>\r\n", ch);
		return;
	}

	g = guilds_data;
	num = atoi(arg2);
	amount = atoi(arg1);

	while (g) {
		if (g->id == num) break;
		if (!strncasecmp(g->name, arg2, strlen(arg2))) break;
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	element = GET_CHAR_GUILDS(ch);

	while (element) {
		if (element->guild == g) break;
		element = element->next;
	}

	if (!element && !IS_ADMIN(ch) && GLD_FLAGGED(g, GLD_SECRETIVE)) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	if (!element && !IS_ADMIN(ch)) {
		send_to_char("You are not a member of this guild!\r\n", ch);
		return;
	}

	if (GLD_FLAGGED(g, GLD_NOBANK)) {
		send_to_char("This guild has no bank account.\r\n", ch);
		return;
	}

	if (amount <= 0) {
		send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
		return;
	}
	if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || (!IS_ADMIN(ch)))) {
		send_to_char("You don't have that many coins!\r\n", ch);
		return;
	}
	send_to_char("Done.\r\n", ch);
	if (IS_NPC(ch) || (!IS_ADMIN(ch)))
	GET_GOLD(ch) -= amount;
	g->gold += amount;

	if (element) 
	element->guildie->deposited += amount;

	save_guild(g);

}


ACMD(do_gwithdraw)
{
	char arg1[100], arg2[100];
	int amount, num;
	struct guild_info *g;
	struct char_guild_element *element;

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
	{ 
		send_to_char("Usage: gwithdraw <amount> <guild name/id>\r\n", ch);
		return;
	}

	g = guilds_data;
	num = atoi(arg2);
	amount = atoi(arg1);

	while (g) {
		if (g->id == num) break;
		if (!strncasecmp(g->name, arg2, strlen(arg2))) break;
		g = g->next;
	}

	if (!g) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	element = GET_CHAR_GUILDS(ch);

	while (element) {
		if (element->guild == g) break;
		element = element->next;
	}

	if (!element && !IS_ADMIN(ch) && GLD_FLAGGED(g, GLD_SECRETIVE)) {
		send_to_char("No such guild.\r\n", ch);
		return;
	}

	if (!element && !IS_ADMIN(ch)) {
		send_to_char("You are not a member of this guild!\r\n", ch);
		return;
	}

	if (GLD_FLAGGED(g, GLD_NOBANK)) {
		send_to_char("This guild has no bank account.\r\n", ch);
		return;
	}

	if (!IS_ADMIN(ch)) 
	if(!PERM_FLAGGED(element->guildie, PERM_WITHDRAW)) {
		send_to_char("You can't withdraw from that account.\r\n", ch);
		return;
	}

	if (amount <= 0) {
		send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
		return;
	}
	if (g->gold < amount) {
		send_to_char("Not enough gold in that account.\r\n", ch);
		return;
	}
	send_to_char("Done.\r\n", ch);
	g->gold -= amount;
	GET_GOLD(ch) += amount;

	if (element) 
	element->guildie->withdrew += amount;

	save_guild(g);

}


ACMD(do_gskillset)
{
	struct char_data *vict;
	char name[100], buf2[100], buf[100], help[MAX_STRING_LENGTH];
	int skill, value, qend;
	struct gskill_info *gskill = NULL;
	struct char_guild_element *element;

	element = GET_CHAR_GUILDS(ch);

	argument = one_argument(argument, name);

	if (!*name) { /* no arguments. print an informative text */
		send_to_char("Syntax: gskillset <name> '<skill>' <value>\r\n", ch);
		strcpy(help, "Skill being one of the following:\r\n");
		while (element) {
			if (PERM_FLAGGED(element->guildie, PERM_GSKILLSET)) {
				gskill = element->guild->gskills;
				while (gskill) {
					sprintf(help, "%s%s [maximum set: %d]\r\n", help, 
					spell_info[gskill->skill].name,
					gskill->maximum_set);
					gskill = gskill->next;
				}
			}
			element = element->next;
		}
		page_string(ch->desc,help,1);
		send_to_char("\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD, 1))) {
		send_to_char("No such person around.\r\n", ch);
		return;
	}
	skip_spaces(&argument);

	/* If there is no chars in argument */
	if (!*argument) {
		send_to_char("Gskill name expected.\r\n", ch);
		return;
	}
	if (*argument != '\'') {
		send_to_char("Gskill must be enclosed in: ''\r\n", ch);
		return;
	}
	/* Locate the last quote && lowercase the magic words (if any) */

	for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
		*(argument + qend) = LOWER(*(argument + qend));

	if (*(argument + qend) != '\'') {
		send_to_char("Gskill must be enclosed in: ''\r\n", ch);
		return;
	}
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';
	if ((skill = find_skill_num(help)) <= 0) {
		send_to_char("Unrecognized skill.\r\n", ch);
		return;
	}

	element = GET_CHAR_GUILDS(ch);
	while (element) {
		if (PERM_FLAGGED(element->guildie, PERM_GSKILLSET)) {
			gskill = element->guild->gskills;
			while (gskill) {
				if (gskill->skill == skill) break;
				gskill = gskill->next;
			}
		}
		if (gskill) {
			if (!STATUS_FLAGGED(element->guildie, STATUS_GL) && !GET_SKILL(ch, skill)) {
				send_to_char("You don't know this gskill yourself.\r\n", ch);
				return;
			}
			break;
		}
		element = element->next;
	}

	if (!gskill) {
		send_to_char("You can't gskillset this skill.\r\n", ch);
		return;
	}

	argument += qend + 1; /* skip to next parameter */
	argument = one_argument(argument, buf);

	if (!*buf) {
		send_to_char("Learned value expected.\r\n", ch);
		return;
	}
	value = atoi(buf);
	if (value < 0) {
		send_to_char("Minimum value for learned is 0.\r\n", ch);
		return;
	}
	if (value > gskill->maximum_set) {
		sprintf(buf, "Max value for learned is %d.\r\n", gskill->maximum_set);
		send_to_char(buf, ch);
		return;
	}
	if (IS_NPC(vict)) {
		send_to_char("You can't set NPC skills.\r\n", ch);
		return;
	}
	extended_mudlog(BRF, SYSL_GUILDS, TRUE, "%s gskillset %s's %s to %d.", GET_NAME(ch), 
	GET_NAME(vict),
	spell_info[skill].name, value);

	SET_SKILL(vict, skill, value);

	sprintf(buf2, "You gskillset %s's %s to %d.\r\n", GET_NAME(vict),
	spell_info[skill].name, value);
	send_to_char(buf2, ch);
}


ACMD(do_gload)
{
	struct obj_data *obj;
	int number, r_num;
	struct char_guild_element *element = NULL;
	struct gequip_info *gequip = NULL;

	one_argument(argument, buf);

	if (!*buf) {
		send_to_char("Usage: gload <gequip vnum>\r\n", ch);
		return;
	}

	if ((number = atoi(buf)) < 0) {
		send_to_char("A NEGATIVE number?\r\n", ch);
		return;
	}

	element = GET_CHAR_GUILDS(ch);

	while (element) {
		gequip = NULL;
		if (STATUS_FLAGGED(element->guildie, STATUS_GL)) {
			gequip = element->guild->gequip;
			while (gequip) {
				if (gequip->vnum == number) break;
				gequip = gequip->next;
			}
		}
		if (gequip) break;
		element = element->next;
	}

	if (!gequip) {
		send_to_char("You can't gload this object.\r\n", ch);
		return;
	}

	if ((r_num = real_object(number)) < 0) {
		send_to_char("There is no object with that number.\r\n", ch);
		return;
	}

	obj = read_object(r_num, REAL);
	obj_to_char(obj, ch);
	act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
	act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
	act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);

	extended_mudlog(BRF, SYSL_GUILDS, TRUE, "%s has gloaded obj %d (%s).", GET_NAME(ch), number, obj->short_description);

}


ACMD(do_greceive)
{
	int r_num;
	struct obj_data *obj;
	struct char_guild_element *element;
	struct gequip_info *gequip;
	int bool = 1;

	element = GET_CHAR_GUILDS(ch);

	while (element)
	{
		if (element->guildie->gequipsent)
		{
			gequip = element->guildie->gequipsent;
			while (gequip) {
				if ((r_num = real_object(gequip->vnum)) >= 0) {
					obj = read_object(r_num, REAL);
					obj_to_char(obj, ch);
					sprintf(buf, "&RYour GL has sent you %s.&n\r\n", obj->short_description);
					send_to_char(buf, ch);
				}
				element->guildie->gequipsent = gequip->next;
				free(gequip);
				gequip = element->guildie->gequipsent;
			}
			bool = 0;
		}
		element = element->next;
	}
	if (bool)
	send_to_char("&RNothing was sent to you by your GLs.&n\r\n", ch);
}


ACMD(do_guildwalk)
{
	int guild_num;
	struct char_guild_element *element;

	one_argument(argument, arg);

	if (*arg) {
		element = GET_CHAR_GUILDS(ch);
		while (element) {
			guild_num = atoi(arg);
			if ((element->guild->id == guild_num) && 
				STATUS_FLAGGED(element->guildie, STATUS_MEMBER)) {
				do_guild_walk(ch, element);
				return;
			}
			if (!strncasecmp(element->guild->name, arg, strlen(arg)) &&
				STATUS_FLAGGED(element->guildie, STATUS_MEMBER)) {
				do_guild_walk(ch, element);
				GET_POS(ch) = POS_STANDING;
				return;
			}
			element = element->next;
		}
	}

	send_to_char("Use 'guildwalk <guild num/name>'.\r\n", ch);
}


ACMD(do_gtoggle)
{
	struct char_guild_element *element;

	if (IS_NPC(ch))
	return;

	argument = any_one_arg(argument, arg);

	if (!*arg || !arg) {
		send_to_charf(ch, "Usage: %s { <gchan> | tag-<gchan> }.\r\n", CMD_NAME);
		return;
	}

	element = GET_CHAR_GUILDS(ch);
	while (element) {
		if (!strncmp(element->guild->gchan_name, arg,
		strlen(element->guild->gchan_name)) &&
			!GLD_FLAGGED(element->guild, GLD_NOGCHAN)) {
			if (STATUS_FLAGGED(element->guildie, STATUS_CHAN_ON)) {
				sprintf(buf, "You will no longer hear %s.\r\n",
				element->guild->gchan_name);
				send_to_char(buf, ch);
				REMOVE_BIT(element->guildie->status, STATUS_CHAN_ON);
				return;
			}
			else {
				sprintf(buf, "You will now hear %s.\r\n", element->guild->gchan_name);
				send_to_char(buf, ch);
				SET_BIT(element->guildie->status, STATUS_CHAN_ON);
				return;
			}
		}
		sprintf(buf, "tag-%s", element->guild->gchan_name);
		if (!strncmp(buf, arg, strlen(buf)) &&
			!GLD_FLAGGED(element->guild, GLD_NOGCHAN)) {
			if (STATUS_FLAGGED(element->guildie, STATUS_TAG)) {
				sprintf(buf, "You will no longer see the gchan tag in %s.\r\n",
				element->guild->gchan_name);
				send_to_char(buf, ch);
				REMOVE_BIT(element->guildie->status, STATUS_TAG);
				return;
			}
			else {
				sprintf(buf, "You will now see the gchan tag in %s.\r\n",
				element->guild->gchan_name);
				send_to_char(buf, ch);
				SET_BIT(element->guildie->status, STATUS_TAG);
				return;
			}
		}
		element = element->next;
	}

	send_to_char("You have no such guild channel.\r\n", ch);
}
