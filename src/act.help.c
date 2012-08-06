/* ************************************************************************
*		File: act.help.c                                    Part of CircleMUD *
*	 Usage: Help file related commands for display/search of help entries   *
*																																					*
*  This file by Torgny Bjers <artovil@arcanerealms.org> for Arcane Realms *
*  MUD.  Copyright (C) 2002, Arcane Realms.                               *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001, Torgny Bjers.                                       *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: act.help.c,v 1.27 2004/03/19 21:58:08 cheron Exp $ */

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
#include "screen.h"
#include "constants.h"
#include "time.h"
#include "oasis.h"

/* extern variables */
extern char *help;
extern int top_of_helpt;
extern int top_of_help_categories;
extern struct help_index_element *help_table;
extern struct help_categories_element *help_categories;
struct sort_struct {
	int sort_pos;
	byte is_social;
}	*cmd_sort_info;

/* external functions */
int is_name(const char *str, const char *namelist);
int isname(const char *str, const char *namelist);
bitvector_t asciiflag_conv(char *flag);
EVENTFUNC(sysintensive_delay);

/* local functions */
void display_help_categories(struct char_data *ch);
int display_help_content(struct char_data *ch, char *argument);
int find_help_category(const char *argument);
int find_help_entry(const char *argument);
ACMD(do_help);
ACMD(do_action);


/*
 * Help categories are fairly static, and it would be a pain to execute another
 * query just to get them, so we've already read them in in db.c into an array,
 * and here we just loop them over.
 * Torgny Bjers, 2001-11-12
 */
void display_help_categories(struct char_data *ch)
{
	int qend, i;
	char *name, *help = get_buffer(MAX_STRING_LENGTH);
	MYSQL_RES *stats;
	MYSQL_ROW row;
	int numentries = 0;
	time_t modified;

	strcpy(help, "&CARCANE REALMS HELP TOPICS&n\r\n\r\nType 'HELP <topic>' for more information on a specific topic.\r\nYou can use HELP ? <search phrase> to issue a fulltext search.\r\n\r\n");

	for (qend = 0, i = 0; i <= top_of_help_categories; i++) {
		if (!GOT_RIGHTS(ch, help_categories[i].rights) && !GOT_RIGHTS(ch, RIGHTS_HELPFILES))
			continue;
		name = str_dup(help_categories[i].name);
		sprintf(help + strlen(help), "%s%-22.22s", ((qend % 3 == 0)?"    ":""), ALLCAP(name));
		release_buffer(name);
		if (qend++ % 3 == 2) {
			strcat(help, "\r\n");
			send_to_char(help, ch);
			*help = '\0';
		}
	}
	if (*help)
		send_to_char(help, ch);
	if (!(--qend % 3 == 2))
		send_to_char("\r\n", ch);
	*help = '\0';

	release_buffer(help);

	if (!(stats = mysqlGetResource(TABLE_HLP_INDEX, "SELECT COUNT(keyword) AS count, MAX(modified) AS modfied FROM %s ORDER BY modified;", TABLE_HLP_INDEX))) {
		send_to_char("&YA database error has occured.  Please notify an administrator.&n\r\n", ch);
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading help statistics.");
		return;
	}
	mysql_free_result(stats);

	row = mysql_fetch_row(stats);
	modified = atoi(row[1]);
	numentries = atoi(row[0]);

	send_to_charf(ch, "\r\nThere are &W%d&n help entries, latest updated: &W%s&n", numentries, ctime(&modified));
}


int display_help_content(struct char_data *ch, char *argument)
{
	MYSQL_RES *commandlist;
	MYSQL_ROW row;
	int qend, i, rec_count;
	unsigned long *fieldlength;
	char *entry = get_buffer(MAX_STRING_LENGTH);
	char *name;

	for (i = 0; i <= top_of_help_categories; i++) { // If it is a category:
		if (!GOT_RIGHTS(ch, help_categories[i].rights) && !GOT_RIGHTS(ch, RIGHTS_HELPFILES))
			continue;
		if (is_name(argument, help_categories[i].name)) {
			/*
			 * A full category name was matched in the string, so we print out that
			 * category with its belonging information instead of trying to search
			 * the help entries for something.
			 */

			/*
			 * Get all the commands that belong to this category.  Make sure that they do not see
			 * commands that are not of their rights.
			 */
			if (!(commandlist = mysqlGetResource(TABLE_HLP_INDEX, "SELECT menutitle, keyword, rights FROM %s WHERE category = %d ORDER BY keyword ASC;", TABLE_HLP_INDEX, help_categories[i].num))) {
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading help index.");
				extended_mudlog(NRM, SYSL_SQL, TRUE, "Query error (%s)%s:%d: %s", __FILE__, __FUNCTION__, __LINE__, mysql_error(&mysql));
				release_buffer(entry);
				return (0);
			}

			name = str_dup(help_categories[i].name);

			sprintf(entry, "\r\n&C%s&n\r\n%s%s", ALLCAP(name), (strlen(help_categories[i].text) > 1?"\r\n":""), (strlen(help_categories[i].text) > 1?help_categories[i].text:""));
			send_to_char(entry, ch);

			release_buffer(name);

			rec_count = mysql_num_rows(commandlist);

			if (rec_count == 0) {
				send_to_char("\r\n    This help category is empty.\r\n", ch);
				mysql_free_result(commandlist);
				release_buffer(entry);
				return (1);
			}

			strcpy(entry, "\r\n");

			for (qend = 0, i = 0; i < rec_count; i++) {
				row = mysql_fetch_row(commandlist);
				fieldlength = mysql_fetch_lengths(commandlist);
				if (!GOT_RIGHTS(ch, asciiflag_conv(row[2])))
					continue;
				sprintf(entry + strlen(entry), "%s%-18s", ((qend % 4 == 0)?"    ":""), ((fieldlength[0] > 0)?ALLCAP(row[0]):ALLCAP(row[1])));
				if (qend++ % 4 == 3) {
					strcat(entry, "\r\n");
					send_to_char(entry, ch);
					*entry = '\0';
				}
			}

			if (*entry)
				send_to_char(entry, ch);
			if (!(--qend % 4 == 3))
				send_to_char("\r\n", ch);

			mysql_free_result(commandlist);

			release_buffer(entry);

			return (1);

		}
	}

	release_buffer(entry);

	return (0);

}


/*
 * New searchable do_help for MySQL by Torgny Bjers.  Uses the new function
 * display_help_categories() in order to display categories.
 * Torgny Bjers, 2001-11-12
 */
ACMD(do_help)
{
	MYSQL_RES *helpindex;
	MYSQL_ROW row;
	int rec_count = 0, i, id = 0, fulltext = 0;
	char *searcharg = NULL;
	unsigned long *fieldlength;
	char *entry, *related;
	char *line, arg[2];
	time_t modified;

	if (!ch->desc)
		return;

	skip_spaces(&argument);

	if (!*argument) {
		display_help_categories(ch);
		return;
	}

	if (*argument == '?') {
		arg[0] = argument[0];
		arg[1] = '\0';
		line = argument + 1;
		fulltext = 1;
		skip_spaces(&line);
	} else
		line = str_dup(argument);

	if (!*line) {
		display_help_categories(ch);
		return;
	}

	if (GET_PLAYER_EVENT(ch, EVENT_SYSTEM) && !IS_IMMORTAL(ch) && IS_MEMBER(ch)) {
		send_to_char("&RYou recently performed a system intensive task. Try again later...&n\r\n", ch);
		return;
	} else {
		if (!IS_IMMORTAL(ch) && IS_MEMBER(ch)) {
			struct delay_event_obj *deo;
			CREATE(deo, struct delay_event_obj, 1);
			deo->vict = ch;														/* pointer to ch									*/
			deo->type = EVENT_SYSTEM;									/* action type										*/
			deo->time = 1;														/* event time * PULSE_SKILL				*/
			GET_PLAYER_EVENT(ch, EVENT_SYSTEM) = event_create(sysintensive_delay, deo, 2 RL_SEC);
		}
	}

	if (display_help_content(ch, line) && !fulltext)
		return;

	id = atoi(line);

	SQL_MALLOC(LOWERALL(line), searcharg);
	SQL_ESC(LOWERALL(line), searcharg);

	if (id > 0 && !fulltext) { // we received a number instead of a string
		if (!(helpindex = mysqlGetResource(TABLE_HLP_INDEX, "SELECT * FROM %s WHERE ID = %d ORDER BY keyword ASC LIMIT 120;", TABLE_HLP_INDEX, id))) {
			send_to_char("&YA database error has occured.  Please notify an administrator.&n\r\n", ch);
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading help index.");
			return;
		}
	} else if (fulltext) {
		if (!(helpindex = mysqlGetResource(TABLE_HLP_INDEX, "SELECT * FROM %s WHERE body LIKE '%%%s%%' OR brief LIKE '%%%s%%' OR keyword LIKE '%%%s%%' ORDER BY keyword ASC LIMIT 101;", TABLE_HLP_INDEX, searcharg, searcharg))) {
			send_to_char("&YA database error has occured.  Please notify an administrator.&n\r\n", ch);
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading help index.");
			return;
		}
		id = -1; // make sure the ID is below zero for the matching below...
	} else {
		if (!(helpindex = mysqlGetResource(TABLE_HLP_INDEX, "SELECT * FROM %s WHERE keyword LIKE '%%%s%%' ORDER BY keyword ASC LIMIT 120;", TABLE_HLP_INDEX, searcharg))) {
			send_to_char("&YA database error has occured.  Please notify an administrator.&n\r\n", ch);
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading help index.");
			return;
		}
		id = -1; // make sure the ID is below zero for the matching below...
	}

	if (*searcharg)
		SQL_FREE(searcharg);

	rec_count = mysql_num_rows(helpindex);

	if (rec_count == 0) { // if the search did not yield any matches:
		char *printbuf = get_buffer(MAX_STRING_LENGTH);
		for (i = 0; i <= top_of_help_categories; i++) {
			if (!GOT_RIGHTS(ch, help_categories[i].rights))
				continue;
			if (!strncmp(line, help_categories[i].name, strlen(line))) {
				display_help_content(ch, help_categories[i].name);
				mysql_free_result(helpindex);
				return;
			}
		}
		if (id < 0 && strlen(line) > 2 && !fulltext) // alpha + longer string
			sprintf(printbuf, "&RNothing could be found in the help database on %s.&n\r\n", line);
		else if (fulltext) // fulltext search
			sprintf(printbuf, "&RYour search for %s returned no matches.&n\r\n", line);
		else // short string or number
			sprintf(printbuf, "&RNo such entry could be found in the help database.&n\r\n");
		send_to_char(printbuf, ch);
		if (id < 0 && strlen(line) > 2 && !fulltext && IS_MEMBER(ch)) { // alpha + longer string
			extended_mudlog(NRM, SYSL_NOHELP, TRUE, "%s tried to get help on %s", GET_NAME(ch), line);
		}
		mysql_free_result(helpindex);
		release_buffer(printbuf);
		return;
	} else if (rec_count > 100 && fulltext) {
		send_to_char("&RThe search returned more than 100 records.\r\nPlease narrow your search criteria.&n\r\n", ch);
		mysql_free_result(helpindex);
		return;
	}

	entry = get_buffer(MAX_STRING_LENGTH);
	related = get_buffer(MAX_STRING_LENGTH);
	
	if (fulltext)
		strcpy(related, "Here are the results for your search query.\r\nType help <topic | number> to see the actual help entry.\r\n\r\n");
	else
		strcpy(related, "There were no direct matches for your search criteria.\r\nType help <topic | number> to see the actual help entry.\r\nHere are some possible matches:\r\n\r\n");

	for (i = 0; i < rec_count; i++)
	{
		row = mysql_fetch_row(helpindex); // fetch a row from the result set
		fieldlength = mysql_fetch_lengths(helpindex); // fetch field lengths from result set
		if ((!strncmp(line, row[1], fieldlength[1]) || (fieldlength[11] > 0 && !strncmp(line, row[11], fieldlength[11])) || id > 0) && !fulltext) { // MATCH, display entry
			if (GOT_RIGHTS(ch, asciiflag_conv(row[10])) || asciiflag_conv(row[10]) == RIGHTS_MEMBER) {
				sprintf(entry, "\r\n[&W%s&n] &C%s&n\r\n\r\n", row[0], ALLCAP(row[1]));
				if (fieldlength[4] > 0)
					sprintf(entry, "%s&WSyntax:&n %s\r\n\r\n", entry, row[4]);
				if (IS_IMMORTAL(ch) || GOT_RIGHTS(ch, RIGHTS_HELPFILES)) { // If they are immortal they will see additional information.
					char *printbuf = get_buffer(1024);
					strcat(entry, "&WImminfo:&n\r\n");
					modified = atoi(row[5]);
					sprintbit(asciiflag_conv(row[10]), user_rights, printbuf, 1024);
					sprintf(entry, "%sRights: %s\r\nModified: %s\r\n", entry, printbuf, ctime(&modified));
					release_buffer(printbuf);
					if (fieldlength[7] > 1)
						sprintf(entry + strlen(entry), "%s\r\n", row[7]);
				}
				sprintf(entry, "%s%s", entry, row[9]);
				if (fieldlength[8] > 0)
					sprintf(entry, "%s\r\n&WSee also:&n %s\r\n", entry, ALLCAP(row[8]));
				if (IS_MEMBER(ch))
					page_string(ch->desc, entry, TRUE);
				else
					page_string(ch->desc, entry, FALSE);
			} else {
				char *printbuf = get_buffer(MAX_STRING_LENGTH);
				if (id < 0 && strlen(line) > 2) // alpha + longer string
					sprintf(printbuf, "&RNothing could be found in the help database on %s.&n\r\n", line);
				else // short string or number
					sprintf(printbuf, "&RNo such entry could be found in the help database.&n\r\n");
				send_to_char(printbuf, ch);
				release_buffer(printbuf);
			}
			mysql_free_result(helpindex);
			release_buffer(entry);
			release_buffer(related);
			return;
		} else { // NO MATCH, display related materials
			if (GOT_RIGHTS(ch, asciiflag_conv(row[10]))) {
				sprintf(related, "%s[&W%5s&n] %-18s%s%-40s\r\n", related, row[0], ((fieldlength[11] > 0)?ALLCAP(row[11]):ALLCAP(row[1])), ((fieldlength[3] > 1)?": ":""), ((fieldlength[3] > 1)?row[3]:""));
			}
		}
	}

	release_buffer(entry);

	mysql_free_result(helpindex);

	if (*related && IS_MEMBER(ch)) {
		page_string(ch->desc, related, TRUE);
		release_buffer(related);
		return;
	}

	release_buffer(related);

	send_to_char("&RThere is no help on that word.&n\r\n", ch);

}


int find_help_category(const char *argument)
{
	int i;

	if (!*argument)
		return (0);

	for (i = 0; i <= top_of_help_categories; i++)
		if (!strncmp(argument, help_categories[i].name, strlen(argument)))
			return (i);

	return (0);
}


int find_help_entry(const char *argument)
{
	int rec_count;
	MYSQL_RES *helpindex;
	char *searcharg=NULL, *line;

	if (!*argument)
		return (0);

	line = str_dup(argument);

	SQL_MALLOC(LOWERALL(line), searcharg);
	SQL_ESC(LOWERALL(line), searcharg);

	if (!(helpindex = mysqlGetResource(TABLE_HLP_INDEX, "SELECT keyword FROM %s WHERE keyword LIKE '%%%s%%' LIMIT 1;", TABLE_HLP_INDEX, searcharg))) {
		if (*searcharg)
			SQL_FREE(searcharg);
		release_buffer(line);
		return (0);
	}

	release_buffer(line);

	rec_count = mysql_num_rows(helpindex);
	mysql_free_result(helpindex);

	if (*searcharg)
		SQL_FREE(searcharg);

	if (rec_count == 1)
		return (1);

	return (0);
}


ACMD(do_helpcheck)
{
	char *list = get_buffer(MAX_STRING_LENGTH);
	int command, no = 0, numentries = 0;
	time_t modified;
	MYSQL_RES *stats;
	MYSQL_ROW row;

	if (GET_OLC_ZONE(ch) != HCHECK_PERMISSION && !GOT_RIGHTS(ch, RIGHTS_HELPFILES)) {
		send_to_char("You do not have sufficient trust rights to use this command.\r\n", ch);
	} else {
		if (GET_PLAYER_EVENT(ch, EVENT_SYSTEM) && !IS_IMPLEMENTOR(ch)) {
			send_to_char("&RYou recently performed a system intensive task. Try again later...&n\r\n", ch);
			return;
		} else {
			if (!IS_IMPLEMENTOR(ch)) {
				struct delay_event_obj *deo;
				CREATE(deo, struct delay_event_obj, 1);
				deo->vict = ch;														/* pointer to ch									*/
				deo->type = EVENT_SYSTEM;									/* action type										*/
				deo->time = 1;														/* event time * PULSE_SKILL				*/
				GET_PLAYER_EVENT(ch, EVENT_SYSTEM) = event_create(sysintensive_delay, deo, 60 RL_SEC);
			}
		}
		strcpy(list, "\r\nTHE FOLLOWING COMMANDS DO NOT HAVE HELP ENTRIES:\r\n\r\n&n");
		for (command = 0; *complete_cmd_info[command].command != '\n'; command++) {
			if (!find_help_category(complete_cmd_info[command].command) &&
				!find_help_entry(complete_cmd_info[command].command) &&
				!cmd_sort_info[command].is_social &&
				strncmp(complete_cmd_info[command].command, "RESERVED", 8)) {
				sprintf(list, "%s%s%-13.13s%s ", list, (complete_cmd_info[command].rights == RIGHTS_NONE) ? "&K" : ((complete_cmd_info[command].rights >= RIGHTS_IMMORTAL) ? "&W" : ""),
						complete_cmd_info[command].command,
						(complete_cmd_info[command].rights == RIGHTS_NONE || complete_cmd_info[command].rights >= RIGHTS_IMMORTAL) ? "&n" : "");
				if (no++ % 5 == 4)
					strcat(list, "\r\n");
			}
		}
		if (no % 5)
			strcat(list, "\r\n");

		if (!(stats = mysqlGetResource(TABLE_HLP_INDEX, "SELECT COUNT(keyword) AS count, MAX(modified) AS modfied FROM %s ORDER BY modified;", TABLE_HLP_INDEX))) {
			send_to_char("&YA database error has occured.  Please notify an administrator.&n\r\n", ch);
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error loading help statistics.");
			return;
		}
		mysql_free_result(stats);

		row = mysql_fetch_row(stats);
		modified = atoi(row[1]);
		numentries = atoi(row[0]);

		sprintf(list, "%s\r\n&nEntries marked with darker color are commands with RIGHTS_NONE,\r\nand entries marked with bright color are >= RIGHTS_IMMORTAL.\r\n\r\nCommands without help entries: &W%d&n\r\n\r\nThere are &W%d&n help entries, latest updated: &W%s&n", list, no, numentries, ctime(&modified));

		page_string(ch->desc, list, TRUE);
	}
	release_buffer(list);
}
