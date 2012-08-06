/* ************************************************************************
*  Originally written by: Michael Scott -- Manx.                          *
*  Last known e-mail address: scottm@workcomm.net                         *
*                                                                         *
*  XXX: This needs Oasis-ifying.                                          *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001, Torgny Bjers.                                       *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
************************************************************************ */
/* $Id: tedit.c,v 1.21 2004/03/12 05:40:39 annuminas Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "genolc.h"
#include "oasis.h"
#include "tedit.h"
#include "constants.h"

extern const char *credits;
extern const char *news;
extern const char *motd;
extern const char *imotd;
extern const char *help;
extern const char *info;
extern const char *background;
extern const char *handbook;
extern const char *policies;
extern const char *meeting;
extern const char *changes;
extern const char *wizlist;
extern const char *immlist;
extern const char *buildercreds;
extern const char *guidelines;
extern const char *roleplay;
extern const char *snooplist;
extern const char *namepolicy;
extern const char *ideas;
extern const char *bugs;
extern const char *typos;

void tedit_string_save(struct descriptor_data *d)
{
	char *name=NULL;
	char *description=NULL;
	char *storage = (char *)d->olc;
	char *replace = "REPLACE INTO %s ("
		"name, "
		"text) "
		"VALUES ('%s', '%s');";
	SQL_MALLOC(storage, name);
	SQL_ESC(storage, name);
	SQL_MALLOC(*d->textedit->str, description);
	SQL_ESC(*d->textedit->str, description);
	if (!(mysqlWrite(replace, TABLE_TEXT, name, description))) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Can't write text '%s' to database table %s.", storage, TABLE_TEXT);
		extended_mudlog(NRM, SYSL_SQL, TRUE, "Query error (%s)%s:%d: %s", __FILE__, __FUNCTION__, __LINE__, mysql_error(&mysql));
		return;
	}
	if (*name)
		SQL_FREE(name);
	if (*description)
		SQL_FREE(description);

	extended_mudlog(NRM, SYSL_OLC, TRUE, "%s saves the text '%s'.", GET_NAME(d->character), storage);

	if (d->olc) {
		free(d->olc);
		d->olc = NULL;
	}

}


ACMD(do_tedit)
{
	int l, i;
	char field[MAX_INPUT_LENGTH];
	 
	struct {
		char *cmd;
		bitvector_t rights;
		const char **buffer;
		int  size;
		char *filename;
	} fields[] = {
		/* edit the rights to your own needs */
		{ "credits",		RIGHTS_IMPLEMENTOR,	&credits,	8192, CREDITS_FILE},
		{ "news",		RIGHTS_ADMIN,		&news,		8192, NEWS_FILE},
		{ "meeting",		RIGHTS_HEADBUILDER,	&meeting,	8192, MEETING_FILE},
		{ "changes",		RIGHTS_HEADBUILDER,	&changes,	32768, CHANGES_FILE},
		{ "motd",		RIGHTS_ADMIN,		&motd,		2400, MOTD_FILE},
		{ "imotd",		RIGHTS_ADMIN,		&imotd,		2400, IMOTD_FILE},
		{ "help",		RIGHTS_ADMIN,		&help,		2400, HELP_PAGE_FILE},
		{ "info",		RIGHTS_ADMIN,		&info,		8192, INFO_FILE},
		{ "background",		RIGHTS_IMPLEMENTOR,	&background,	8192, BACKGROUND_FILE},
		{ "handbook",		RIGHTS_HEADBUILDER,	&handbook,	8192, HANDBOOK_FILE},
		{ "policies",		RIGHTS_ADMIN,		&policies,	8192, POLICIES_FILE},
		{ "wizlist",		RIGHTS_ADMIN,		&wizlist,	2400, WIZLIST_FILE},
		{ "immlist",		RIGHTS_ADMIN,		&immlist,	2400, IMMLIST_FILE},
		{ "bcreds",		RIGHTS_HEADBUILDER,	&buildercreds,	8192, BUILDERCREDS_FILE},
		{ "guidelines",		RIGHTS_ADMIN,		&guidelines,	8192, GUIDELINES_FILE},
		{ "roleplay",		RIGHTS_ADMIN,		&roleplay,	8192, ROLEPLAY_FILE},
		{ "snooplist",		RIGHTS_ADMIN,		&snooplist,	8192, SNOOPLIST_FILE},
		{ "namepolicy",		RIGHTS_ADMIN,		&namepolicy,	2400, NAMEPOLICY_FILE},
		{ "ideas",		RIGHTS_ADMIN,		&ideas,		8192,	IDEA_FILE },
		{ "bugs",		RIGHTS_ADMIN,		&bugs,		8192,	BUG_FILE },
		{ "typos",		RIGHTS_ADMIN,		&typos,		8192,	TYPO_FILE },
		{ "\n",			RIGHTS_NONE,		NULL,		0,	NULL }
	};

	if (ch->desc == NULL)
		//return;
	 
	half_chop(argument, field, buf);

	if (!*field) {
		strcpy(buf, "Files available to be edited:\r\n");
		i = 1;
		for (l = 0; *fields[l].cmd != '\n'; l++) {
			if (GOT_RIGHTS(ch, fields[l].rights)) {
				sprintf(buf, "%s%-11.11s", buf, fields[l].cmd);
				if (!(i % 7))
					strcat(buf, "\r\n");
				i++;
			}
		}
		if (--i % 7)
			strcat(buf, "\r\n");
		if (i == 0)
			strcat(buf, "None.\r\n");
		send_to_char(buf, ch);
		return;
	}
	for (l = 0; *(fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, fields[l].cmd, strlen(field)))
			break;
	 
	if (*fields[l].cmd == '\n') {
		send_to_char("Invalid text editor option.\r\n", ch);
		return;
	}
	 
	if (!GOT_RIGHTS(ch, fields[l].rights)) {
		send_to_char("You are not godly enough for that!\r\n", ch);
		return;
	}

	/* set up editor stats */
	clear_screen(ch->desc);

	write_to_output(ch->desc, TRUE, "%s", stredit_header);

	if (*fields[l].buffer)
		write_to_output(ch->desc, FALSE, *fields[l].buffer);

	ch->desc->olc = str_dup(fields[l].filename);
	string_write(ch->desc, (char **)fields[l].buffer, fields[l].size, 0, EDIT_TEXTEDIT);

	act("$n begins to edit a text.", TRUE, ch, 0, 0, TO_ROOM);
}
