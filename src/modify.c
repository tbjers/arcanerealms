/* ************************************************************************
*		File: modify.c                                      Part of CircleMUD *
*	 Usage: Run-time modification of game variables                         *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: modify.c,v 1.37 2003/01/15 20:14:10 arcanere Exp $ */

/**************************************************************************
* BetterEdit by D. Tyler Barnes                                           *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "mail.h"
#include "boards.h"
#include "oasis.h"
#include "tedit.h"
#include "quest.h"
#include "characters.h"
#include "constants.h"
#include "buffer.h"

void show_string(struct descriptor_data *d, char *input);

extern struct spell_info_type *spell_info;
extern const char *MENU;
extern const char *unused_spellname;	/* spell_parser.c */

extern struct class_data classes[NUM_CLASSES];
extern int top_of_race_list;
extern struct race_list_element *race_list;

/* local functions */
void string_replace(char *cmd, struct descriptor_data *d);
void string_del_line(char *cmd, struct descriptor_data *d);
void string_edit(char *cmd, struct descriptor_data *d);
void string_insert(char *cmd, struct descriptor_data *d);
void string_format(bool indent, struct descriptor_data *d);
void string_save(struct descriptor_data *d);
void string_numlist(struct descriptor_data *d);
void string_finish(struct descriptor_data *d);
void string_abort(struct descriptor_data *d);
void free_editor(struct descriptor_data *d);
void smash_tilde(char *str);
ACMD(do_skillset);
char *next_page(char *str, struct descriptor_data *d);
int	count_pages(char *str, struct descriptor_data *d);
void paginate_string(char *str, struct descriptor_data *d);
void playing_string_cleanup(struct descriptor_data *d, int action);
void exdesc_string_cleanup(struct descriptor_data *d, int action);

const	char *string_fields[] =
{
	"name",
	"short",
	"long",
	"description",
	"title",
	"delete-description",
	"\n"
};


const char *stredit_help =
"Editor command formats: /<letter>\r\n"
"\r\n"
"/a         -  Aborts editor\r\n"
"/c         -  Clears buffer\r\n"
"/d#        -  Deletes a line #\r\n"
"/e# <text> -  Changes the line at # with <text>\r\n"
"/f         -  Formats text\r\n"
"/fi        -  Indented formatting of text\r\n"
"/h or /?   -  List text editor commands\r\n"
"/i# <text> -  Inserts <text> before line #\r\n"
"/l         -  Lists buffer\r\n"
"/n         -  Lists buffer with line numbers\r\n"
"/r 'a' 'b' -  Replace 1st occurrence of text <a> in buffer with text <b>\r\n"
"/ra 'a' 'b'-  Replace all occurrences of text <a> within buffer with text <b>\r\n"
"              Usage: /r[a] 'pattern' 'replacement'\r\n"
"/s or @    -  Saves text\r\n"
;

const char *stredit_header =
"&c===============================================================================&n\r\n"
"Instructions: &W/s&n or &W@&n to save, &W/h&n for more options.\r\n"
"Room descriptions should be formatted with indent, &W/fi&n.\r\n"
"Do not send more than &W2048&n characters per line or you might get disconnected.\r\n"
"&c=========1=========2=========3=========4=========5=========6=========7=========&n\r\n"
;


/* maximum length for text field x+1 */
int	length[] =
{
	15,
	60,
	256,
	240,
	60
};


/* ************************************************************************
*	 modification of malloc'ed strings                                      *
************************************************************************ */

/*
 * Put '#if 1' here to erase ~, or roll your own method.  A common idea
 * is smash/show tilde to convert the tilde to another innocuous character
 * to save and then back to display it. Whatever you do, at least keep the
 * function around because other MUD packages use it, like mudFTP.
 *   -gg 9/9/98
 */
void smash_tilde(char *str)
{
#if	0
	/*
	 * Erase any ~'s inserted by people in the editor.  This prevents anyone
	 * using online creation from causing parse errors in the world files.
	 * Derived from an idea by Sammy <samedi@dhc.net> (who happens to like
	 * his tildes thank you very much.), -gg 2/20/98
	 */
		while ((str = strchr(str, '~')) != NULL)
			*str = ' ';
#endif
}

/*
 * Basic API function to start writing somewhere.
 *
 * 'data' isn't used in stock CircleMUD but you can use it to pass whatever
 * else you may want through it.  The improved editor patch when updated
 * could use it to pass the old text buffer, for instance.
 */
void string_write(struct descriptor_data *d, char **writeto, size_t len, long mailto, int mode)
{
	/* Mobiles can't use the text editor. */
	if (!IS_NPC(d->character)) {
		if (!EDITING(d)) {
			CREATE(EDITING(d), struct text_edit_data, 1);
			d->textedit->str = writeto;
			d->textedit->max_str = MIN(len, MAX_STRING_LENGTH);
			d->textedit->mail_to = mailto;
			d->textedit->mode = mode;
		} else {
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "(%s)%s:%d: string_write called but character already has an editing struct allocated.", __FILE__, __FUNCTION__, __LINE__);
		}
	}
}


/* Add user input to the 'current' string (as defined by d->str) */
void string_add(struct descriptor_data *d, char *str)
{
	bool terminal = FALSE;
	int l;

	delete_doubledollar(str);
	smash_tilde(str);

	/* initialize */
	if (d->textedit->smod == NULL) {
		CREATE(d->textedit->smod, char, MAX_STRING_LENGTH);
		if (*d->textedit->str)
			strcpy(d->textedit->smod, *d->textedit->str);
	}

	/* determine if this is the terminal string, and truncate if so */
	if (str[strlen(str) - 1] == '@') {
		str[strlen(str) - 1] = '\0';
		terminal = TRUE;
	}

	if (strlen(str) == 1 || *str != '/') {
		/* String add mode */
		if ((l = strlen(d->textedit->smod) + strlen(str) + 3) <= d->textedit->max_str) {
			sprintf(d->textedit->smod, "%s%s%s", d->textedit->smod, str,
			terminal ? "" : "\r\n");

			/* Handle @ saving */
			if (terminal) {
				string_save(d);
				return;
			}

			if (l == d->textedit->max_str)
				write_to_output(d, TRUE, "Text length maximum reached, no more input will be accepted.\r\n");
		} else {
			l = d->textedit->max_str - strlen(d->textedit->smod) - 1;
			write_to_output(d, TRUE, "Text exceeds maximum allowable length, truncated.\r\n");
			write_to_output(d, TRUE, "(%d bytes remaining in buffer, %d received)\r\n", l, strlen(str));
			if (terminal && l >= 0) {
				/* Don't even try to append \r\n if @ used */
				str[l] = '\0';
				strcat(d->textedit->smod, str);
			} else if (l >= 2) {
				str[l - 2] = '\0';
				sprintf(d->textedit->smod, "%s%s\r\n", d->textedit->smod, str);
			}
		}
	} else {
		/* Command Mode */
		switch(LOWER(str[1])) {
		case 'h':
		case '?':
			write_to_output(d, TRUE, "%s", stredit_help);
			return;
		case 'a': string_abort(d);         break;
		case 's': string_save(d);          break;
		case 'n': string_numlist(d);       break;
		case 'r': string_replace(str, d);  break;
		case 'd': string_del_line(str, d); break;
		case 'i': string_insert(str, d);   break;
		case 'e': string_edit(str, d);     break;
		case 'l':
			write_to_output(d, TRUE, "%s", d->textedit->smod);
			break;
		case 'c':
			d->textedit->smod[0] = '\0';
			write_to_output(d, TRUE, "Text buffer cleared.\r\n");
			break;
		case 'f':
			if (strlen(str) == 2)
				string_format(FALSE, d);
			else if (strlen(str) == 3 && LOWER(str[2]) == 'i')
				string_format(TRUE, d);
			else
				write_to_output(d, TRUE, "Unknown formatting option.\r\n");
			break;
		default:
			write_to_output(d, TRUE, "Unknown command, try /H for help.\r\n");
			return;
		}
	}
}


void string_edit(char *cmd, struct descriptor_data *d)
{
	char oldstr[MAX_STRING_LENGTH], newstr[MAX_STRING_LENGTH];
	char *ln = NULL, *str = NULL;
	bool overflow = FALSE, edited = FALSE;
	int bl, i;

	if (!*d->textedit->smod) {
		write_to_output(d, TRUE, "There is no text to edit.\r\n");
		return;
	}

	/* strip /e */
	cmd += 2;

	if (*cmd)
		if ((ln = strtok(cmd, " ")))
			str = strtok(NULL, "\0");

	if (!*cmd || !(bl = atoi(ln)) || bl < 0 || bl > d->textedit->max_str)
		write_to_output(d, TRUE, "Invalid syntax for edit line command.\r\n");
	else {
		strcpy(oldstr, d->textedit->smod);
		*newstr = '\0';
	for (i = 1 ;; i++) {
		ln = strtok(i == 1 ? oldstr : NULL, "\n");
		if (ln) {
			if (i == bl) {
				if (((str ? strlen(str) : 0) + 2) - (strlen(ln) + 1) + strlen(d->textedit->smod) + 1 <= d->textedit->max_str) {
					sprintf(newstr, "%s%s\r\n", newstr, str ? str : "");
					edited = TRUE;
				} else
					overflow = TRUE;
			} else
				sprintf(newstr, "%s%s\n", newstr, ln);
			}
			if (!ln || overflow)
			break;
		}
		if (edited) {
			strcpy(d->textedit->smod, newstr);
			write_to_output(d, TRUE, "Line edited.\r\n");
		} else if (overflow)
			write_to_output(d, TRUE, "That modification would make the string too long.\r\n");
		else
			write_to_output(d, TRUE, "No such line.\r\n");
	}
}


void string_insert(char *cmd, struct descriptor_data *d)
{
	char oldstr[MAX_STRING_LENGTH];
	char *ln = NULL, *str = NULL;
	int bl, i;

	if (!*d->textedit->smod) {
		write_to_output(d, TRUE, "There is no text. Just start typing.\r\n");
		return;
	}

	/* strip /i */
	cmd += 2;

	if (*cmd)
		if ((ln = strtok(cmd, " ")))
			str = strtok(NULL, "\0");

	if (!*cmd || !(bl = atoi(ln)) || bl < 0 || bl > d->textedit->max_str)
		write_to_output(d, TRUE, "Invalid syntax for insert line command.\r\n");
	else if (strlen(d->textedit->smod) + (str ? strlen(str) : 0) + 3 > d->textedit->max_str)
		write_to_output(d, TRUE, "Inserting that line would make the text too long.\r\n");
	else {

		strcpy(oldstr, d->textedit->smod);
		*d->textedit->smod = '\0';

		for (i = 1 ;; i++) {
			if (i == 1)
				ln = strtok(oldstr, "\n");
			else
				ln = strtok(NULL, "\n");

			if (i == bl)
				sprintf(d->textedit->smod, "%s%s\r\n", d->textedit->smod, str ? str : "");

			if (ln)
				sprintf(d->textedit->smod, "%s%s\n", d->textedit->smod, ln);
			else if (i >= bl)
				break;
		}
		write_to_output(d, TRUE, "Line inserted.\r\n");
	}
}


void string_format(bool indent, struct descriptor_data *d)
{
	char *cur, oldstr[MAX_STRING_LENGTH], newstr[MAX_STRING_LENGTH];
	char line[MAX_INPUT_LENGTH];
	bool overflow = FALSE, punct = FALSE, new_row = FALSE;
	int i;

	if (!*d->textedit->smod) {
		write_to_output(d, TRUE, "There is no text to format.\r\n");
		return;
	}

	strcpy(oldstr, d->textedit->smod);

	/* Change all carriage returns and line feeds to spaces */
	for (cur = oldstr; *cur; cur++)
		if (*cur == '\r' || *cur == '\n')
			*cur = ' ';

	/* Indent if necessary */
	if (indent)
		strcpy(line, "  ");
	else
		*line = '\0';

	*newstr = '\0';

	for (i = 1, cur = oldstr;;i++) {
		skip_spaces(&cur);

		if (i == 1)
			cur = strtok(cur, " ");
		else
			cur = strtok(NULL, " ");

		if (cur) {
			if (strlen(line) + strlen(cur) + 2 <= 79) {
				if (strchr(".!?", line[strlen(line) - 1]))
					punct = TRUE; /* Regular punctuation. */
				else if (strchr("\"'", line[strlen(line) - 1]) && strchr(".!?", line[strlen(line) - 2]))
					punct = TRUE; /* Punctuation within quotes. */
				else
					punct = FALSE;
				sprintf(line, "%s%s%s%s", new_row && strchr(".!?", newstr[strlen(newstr) - 3]) ? CAP(line) : line, *line ? " " : "", punct ? " " : "", i == 1 || punct ? CAP(cur) : cur);
				new_row = FALSE;
			} else if (strlen(newstr) + strlen(line) + 3 <= d->textedit->max_str) {
				sprintf(newstr, "%s%s\r\n", newstr, line);
				strcpy(line, cur);
				new_row = TRUE;
			} else {
				overflow = TRUE;
				break;
			}
		} else if (strlen(newstr) + strlen(line) + 3 <= d->textedit->max_str) {
			sprintf(newstr, "%s%s\r\n", newstr, line ? line : "");
			new_row = FALSE;
			break;
		} else {
			overflow = TRUE;
			break;
		}
	}

	if (overflow)
		write_to_output(d, TRUE, "Formatting this text would make it too large.\r\n");
	else {
		write_to_output(d, TRUE, "The text has been formatted.\r\n");
		strcpy(d->textedit->smod, newstr);
	}
}


void string_del_line(char *cmd, struct descriptor_data *d)
{
	char *line, buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int i, del;

	/* strip /d */
	cmd += 2;

	*buf = '\0';

	if (!*cmd || !(del = atoi(cmd)))
		write_to_output(d, TRUE, "Invalid syntax for delete line command.\r\n");
	else if (!*d->textedit->smod)
		write_to_output(d, TRUE, "There are no lines to delete.\r\n");
	else {

		strcpy(buf2, d->textedit->smod);
		for (i = 1;;i++) {

			if (i == 1)
				line = strtok(buf2, "\n");
			else
				line = strtok(NULL, "\n");

			if (line) {
				if (i != del)
					sprintf(buf, "%s%s\n", buf, line);
				else
					del = -1;            /* flag, means we deleted something */
				} else
					break;
			}

		if (del == -1) {
			write_to_output(d, TRUE, "Line deleted.\r\n");
			strcpy(d->textedit->smod, buf);
		} else
			write_to_output(d, TRUE, "No such line.\r\n");
	}
}


void string_replace(char *cmd, struct descriptor_data *d)
{
	char buf[MAX_STRING_LENGTH];
	char *sstr, *rstr;
	int error, mode;

	if (!*d->textedit->smod)
		write_to_output(d, TRUE, "Try typing something first.\r\n");
	else {
		strcpy(buf, d->textedit->smod);

		/* borrowing 'error' for unrelated purpose */
		if ((error = LOWER(*(cmd + 2))) == 'a')
			mode = REPL_NORMAL;
		else if (error == ' ')
			mode = REPL_FIRST_ONLY;
		else {
			write_to_output(d, TRUE, "Unknown replacement mode. Try /R or /RA.\r\n");
			return;
		}

		mode |= REPL_MATCH_CASE;

		sstr = strtok(cmd, "'");
		if ((sstr = strtok(NULL, "'")) == NULL)
			write_to_output(d, TRUE, "Invalid syntax. Type /H for help.\r\n");
		else {
			rstr = strtok(NULL, "'");
			if ((rstr = strtok(NULL, "'")) == NULL) {
				write_to_output(d, TRUE, "Replace '%s' with what?\r\n", sstr);
			} else {
				if ((error = replace(buf, sstr, rstr, mode)) == 3 || strlen(buf) >= d->textedit->max_str)
					write_to_output(d, TRUE, "That replacement would make the text too long.\r\n");
				else if (error != 0)
					write_to_output(d, TRUE, "Search string was not found.\r\n");
				/**** Only if recursive searching is used ****
				else if (error == 4)
				write_to_output(d, TRUE, "The replacement string may not contain the search pattern.\r\n");
				*********************************************/
				else {
					strcpy(d->textedit->smod, buf);
					write_to_output(d, TRUE, "%s of '%s' replaced with '%s'.\r\n",
						IS_SET(mode, REPL_FIRST_ONLY) ?
						"First occurrence" : "All occurrences", sstr, rstr);
				}
			}
		}
	}
}


void string_numlist(struct descriptor_data *d)
{
	char buf[MAX_STRING_LENGTH], *curline;
	int i;

	if (!*d->textedit->smod)
		return;

	strcpy(buf, d->textedit->smod);
	for (i = 1;; i++) {
		if (i == 1)
			curline = strtok(buf, "\n");
		else
			curline = strtok(NULL, "\n");
		if (curline) {
			write_to_output(d, TRUE, "%3d: %s\n", i, curline);
		} else
			break;
	}
}


/* By the time we reach here, we should be totally ready to just save and go.
* String length checked, \r\n appended, etc
*/
void string_save(struct descriptor_data *d)
{

	if (*d->textedit->str)
		free(*d->textedit->str);

	*d->textedit->str = str_dup(d->textedit->smod);

	/* +1 to count null terminator */
	write_to_output(d, TRUE, "Saving text. (%d bytes)\r\n", strlen(*d->textedit->str) + 1);

	/* Mode dependant cleanup when saving (Stuff not needed when aborting) */
	switch(EDITING(d)->mode) {
	case EDIT_MAIL:
		store_mail(d->textedit->mail_to, GET_IDNUM(d->character), *d->textedit->str);
		write_to_output(d, TRUE, "Message sent!\r\n");
		break;
	case EDIT_BOARD:
		Board_save_board(d->textedit->mail_to);
		break;
	case EDIT_RPDESC:
		write_to_output(d, TRUE, "Description saved.\r\n");
		break;
	case EDIT_CONTACTINFO:
		GET_CONTACTINFO(d->character) = str_dup(*d->textedit->str);
		write_to_output(d, TRUE, "Contact Info saved.\r\n");
		break;
	case EDIT_TEXTEDIT:
		tedit_string_save(d);
		break;
	}

	string_finish(d);
}


void string_abort(struct descriptor_data *d)
{
	/* Mode dependant cleanup when aborting. (So far unused) */
	switch(EDITING(d)->mode) {
	case EDIT_RPDESC:
		write_to_output(d, TRUE, "Description edit aborted.\r\n");
		break;
	case EDIT_CONTACTINFO:
		write_to_output(d, TRUE, "Contactinfo edit aborted.\r\n");
		break;
	case EDIT_TEXTEDIT:
		write_to_output(d, TRUE, "Editing Aborted.\r\n");
		if (d->olc) {
			free(d->olc);
			d->olc = NULL;
		}
		break;
	default:
		write_to_output(d, TRUE, "Editing Aborted.\r\n");
		break;
	}

	string_finish(d);
}


/* Exiting editor, stuff common to both abort and save */
void string_finish(struct descriptor_data *d)
{
	/*
	 * Redisplay menus, etc here.
	 */
	int i;
	struct {
		int mode;
		void (*func)(struct descriptor_data *dsc, int todo);
	} cleanup_modes[] = {
		{ CON_MEDIT			, medit_string_cleanup },
		{ CON_OEDIT			, oedit_string_cleanup },
		{ CON_REDIT			, redit_string_cleanup },
		{ CON_TRIGEDIT	, trigedit_string_cleanup },
		{ CON_EMAIL			, email_string_cleanup },
		{ CON_QEDIT			, qedit_string_cleanup },
		{ CON_SPEDIT		, spedit_string_cleanup },
		{ CON_GEDIT			, gedit_string_cleanup },
		{ -1, NULL }
	};

	for (i = 0; cleanup_modes[i].func; i++)
		if (STATE(d) == cleanup_modes[i].mode)
			(*cleanup_modes[i].func)(d, 0);

	switch (EDITING(d)->mode) {
	case EDIT_BACKGROUND:
		save_char(d->character, NOWHERE, FALSE);
		STATE(d) = CON_MENU;
		write_to_output(d, TRUE, "%s", MENU);
		break;
	case EDIT_RPDESC:
		act("$n stops editing $s description.", TRUE, d->character, 0, 0, TO_ROOM);
		break;
	case EDIT_CONTACTINFO:
		act("$n stops editing $s contact info.", TRUE, d->character, 0, 0, TO_ROOM);
		break;
	case EDIT_TEXTEDIT:
		act("$n stops editing a text.", TRUE, d->character, 0, 0, TO_ROOM);
		break;
	case EDIT_BOARD:
		act("$n finishes writing a message.", TRUE, d->character, 0, 0, TO_ROOM);
		break;
	}
	free_editor(d);
}


/* Free all memory related to text editing (common to abort/save) */
void free_editor(struct descriptor_data *d) {
	/* Mode dependant cleanup (So far only EDIT_MAIL needs it) */
	switch(EDITING(d)->mode) {
	case EDIT_MAIL:
		if (d->textedit->str) {
			if (*d->textedit->str)
				free(*d->textedit->str);
			free(d->textedit->str);
		}
		break;
	}

	if (d->textedit->smod)
		free(d->textedit->smod);
	free(EDITING(d));
	EDITING(d) = NULL;

}


/* **********************************************************************
*	 Modification of character skills                                     *
********************************************************************** */

ACMD(do_skillset)
{
	struct char_data *vict;
	char name[MAX_INPUT_LENGTH], buf2[128];
	char buf[MAX_INPUT_LENGTH], help[MAX_STRING_LENGTH];
	int skill, i, qend;
	long value;

	argument = one_argument(argument, name);

	if (!*name) {			/* no arguments. print an informative text */
		send_to_char("Syntax: skillset <name> '<skill>' <value>\r\n", ch);
		strcpy(help, "Skill being one of the following:\r\n");
		for (qend = 0, i = 0; i <= TOP_SKILL_DEFINE; i++) {
			if (skill_info[i].name == unused_spellname)	/* This is valid. */
				continue;
			sprintf(help + strlen(help), "%-23s", skill_info[i].name);
			if (qend++ % 3 == 2) {
				strcat(help, "\r\n");
				send_to_char(help, ch);
				*help = '\0';
			}
		}
		if (*help)
			send_to_char(help, ch);
		send_to_char("\r\n", ch);
		return;
	}

	if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD, 1))) {
		send_to_char(NOPERSON, ch);
		return;
	}
	skip_spaces(&argument);

	/* If there is no chars in argument */
	if (!*argument) {
		send_to_char("Skill name expected.\r\n", ch);
		return;
	}
	if (*argument != '\'') {
		send_to_char("Skill must be enclosed in: ''\r\n", ch);
		return;
	}
	/* Locate the last quote and lowercase the magic words (if any) */

	for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
		argument[qend] = LOWER(argument[qend]);

	if (argument[qend] != '\'') {
		send_to_char("Skill must be enclosed in: ''\r\n", ch);
		return;
	}
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';
	if ((skill = find_skill_num(help)) <= 0) {
		send_to_char("Unrecognized skill.\r\n", ch);
		return;
	}
	argument += qend + 1;		/* skip to next parameter */
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
	if (value > 10000) {
		send_to_char("Max value for learned is 10000 (100%).\r\n", ch);
		return;
	}
	if (IS_NPC(vict)) {
		send_to_char("You can't set NPC skills.\r\n", ch);
		return;
	}

	/*
	 * find_skill_num() guarantees a valid skill_info[] index, or -1, and we
	 * checked for the -1 above so we are safe here.
	 */
	extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s changed %s's %s to %ld.", GET_NAME(ch), GET_NAME(vict),
		skill_info[skill].name, value);

	SET_SKILL(vict, skill, value);

	sprintf(buf2, "You change %s's %s to %ld.\r\n", GET_NAME(vict),
		skill_info[skill].name, value);
	send_to_char(buf2, ch);
}



/*********************************************************************
*	New Pagination Code
*	Michael Buselli submitted the following code for an enhanced pager
*	for CircleMUD.  All functions below are his.  --JE 8 Mar 96
*
*********************************************************************/

/* Traverse down the string until the begining of the next page has been
 * reached.  Return NULL if this is the last page of the string.
 */
char *next_page(char *str, struct descriptor_data *d)
{
	int col = 1, line = 1, length = PAGE_LENGTH, spec_code = FALSE, color_code = FALSE;

	if (!IS_NPC(d->character))
		length = LIMIT(GET_PAGE_LENGTH(d->character), 10, 200);

	for (;; str++) {
		/* If end of string, return NULL. */
		if (*str == '\0')
			return (NULL);

		/* If we're at the start of the next page, return this fact. */
		else if (line > length)
			return (str);
		
		/* check for &? color code */
		else if (*str == '&' && !color_code)
			color_code = TRUE;
		
		else if (color_code && *str != '&')
			color_code = FALSE;
		
		/* Check for the begining of an ANSI color code block. */
		else if (*str == '\x1B' && !spec_code)
			spec_code = TRUE;
		
		/* Check for the end of an ANSI color code block. */
		else if (*str == 'm' && spec_code)
			spec_code = FALSE;
		
		/* Check for everything else. */
		else if (!spec_code && !color_code) {
			/* Carriage return puts us in column one. */
			if (*str == '\r')
				col = 1;
			/* Newline puts us on the next line. */
			else if (*str == '\n')
				line++;
			
				/* We need to check here and see if we are over the page width,
				* and if so, compensate by going to the begining of the next line.
			*/
			else if (col++ > PAGE_WIDTH) {
				col = 1;
				line++;
			}
		}
	}
}


/* Function that returns the number of pages in the string. */
int	count_pages(char *str, struct descriptor_data *d)
{
	int pages;

	for (pages = 1; (str = next_page(str, d)); pages++);
	return (pages);
}


/* This function assigns all the pointers for showstr_vector for the
 * page_string function, after showstr_vector has been allocated and
 * showstr_count set.
 */
void paginate_string(char *str, struct descriptor_data *d)
{
	int i;

	if (d->showstr_count)
		*(d->showstr_vector) = str;

	for (i = 1; i < d->showstr_count && str; i++)
		str = d->showstr_vector[i] = next_page(str, d);

	d->showstr_page = 0;
}


/* The call that gets the paging ball rolling... */
void page_string(struct descriptor_data *d, char *str, int keep_internal)
{
	if (!d)
		return;

	if (!str || !*str) {
		send_to_char("", d->character);
		return;
	}

	d->showstr_count = count_pages(str, d);
	CREATE(d->showstr_vector, char *, d->showstr_count);

	if (keep_internal) {
		d->showstr_head = str_dup(str);
		paginate_string(d->showstr_head, d);
	} else
		paginate_string(str, d);

	show_string(d, "");
}


/* The call that displays the next page. */
void show_string(struct descriptor_data *d, char *input)
{
	char buffer[MAX_STRING_LENGTH];
	int diff;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	any_one_arg(input, buf);

	/* Q is for quit. :) */
	if (LOWER(*buf) == 'q') {
		free(d->showstr_vector);
		d->showstr_vector = NULL;
		d->showstr_count = 0;
		if (d->showstr_head) {
			free(d->showstr_head);
			d->showstr_head = NULL;
		}
		release_buffer(buf);
		return;
	}
	/* R is for refresh, so back up one page internally so we can display
	 * it again.
	 */
	else if (LOWER(*buf) == 'r')
		d->showstr_page = MAX(0, d->showstr_page - 1);

	/* B is for back, so back up two pages internally so we can display the
	 * correct page here.
	 */
	else if (LOWER(*buf) == 'b')
		d->showstr_page = MAX(0, d->showstr_page - 2);

	/* Feature to 'goto' a page.  Just type the number of the page and you
	 * are there!
	 */
	else if (isdigit(*buf))
		d->showstr_page = MAX(0, MIN(atoi(buf) - 1, d->showstr_count - 1));

	else if (*buf) {
		send_to_char(
			"Valid commands while paging are RETURN, Q, R, B, or a numeric value.\r\n",
			d->character);
		release_buffer(buf);
		return;
	}
	/* If we're displaying the last page, just send it to the character, and
	 * then free up the space we used.
	 */
	if (d->showstr_page + 1 >= d->showstr_count) {
		send_to_char(d->showstr_vector[d->showstr_page], d->character);
		free(d->showstr_vector);
		d->showstr_vector = NULL;
		d->showstr_count = 0;
		if (d->showstr_head) {
			free(d->showstr_head);
			d->showstr_head = NULL;
		}
	}
	/* Or if we have more to show.... */
	else {
		diff = d->showstr_vector[d->showstr_page + 1] - d->showstr_vector[d->showstr_page];
		if (diff > MAX_STRING_LENGTH - 3) /* 3=\r\n\0 */
			diff = MAX_STRING_LENGTH - 3;
		strncpy(buffer, d->showstr_vector[d->showstr_page], diff);
		/*
		 * Fix for prompt overwriting last line in compact mode submitted by
		 * Peter Ajamian <peter@pajamian.dhs.org> on 04/21/2001
		 */
		if (buffer[diff-2] == '\r' && buffer[diff-1]=='\n')
			buffer[diff] = '\0';
		else if (buffer[diff-2] == '\n' && buffer[diff-1] == '\r')
			/* This is backwards.  Fix it. */
			strcpy(buffer+diff-2, "\r\n");
		else if (buffer[diff-1] == '\r' || buffer[diff-1] == '\n')
			/* Just one of \r\n.  Overwrite it. */
			strcpy(buffer+diff-1, "\r\n");
		else
			/* Tack \r\n onto the end to fix bug with prompt overwriting last line. */
      strcpy(buffer+diff, "\r\n");

		send_to_char(buffer, d->character);
		d->showstr_page++;
	}
	release_buffer(buf);
}
