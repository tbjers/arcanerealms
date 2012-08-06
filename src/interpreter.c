/* ************************************************************************
*		File: interpreter.c                                 Part of CircleMUD *
*	 Usage: parse user commands, search for specials, call ACMD functions   *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: interpreter.c,v 1.178 2004/04/22 16:42:10 cheron Exp $ */

#define	__INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "pfdefaults.h"
#include "loadrooms.h"
#include "characters.h"
#include "constants.h"
#include "guild.h"
#include "guild_parser.h"
#include "commands.h"

/* external variables */
extern sh_int mortal_start_room[NUM_STARTROOMS +1];
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern room_rnum r_new_start_room;
extern int top_of_p_table;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern int selfdelete_fastwipe;
extern int multilogging_allowed;
extern const char *EMAIL_MENU;
extern const char *MAILINGLIST_MENU;
extern const char *VERIFICATION_MENU;
extern const char *VERIFICATION_SENT;
extern const char *travel_defaults[];
extern char *motd;
extern char *imotd;
extern char *background;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern char *ANSI;
extern char *SELECT_NAME;
extern char *namepolicy;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern struct class_data classes[NUM_CLASSES];
extern int top_of_race_list;
extern struct race_list_element *race_list;
extern int top_of_culture_list;
extern struct culture_list_element *culture_list;
extern sh_int r_mortal_start_room[NUM_STARTROOMS +1];
extern int in_copyover;
extern int magic_enabled;
extern int use_verification;

extern const char *circlemud_version;


/* external functions */
char menu_letter(int number);
extern void assedit_parse(struct descriptor_data *d, char *arg);
extern void user_cntr(struct descriptor_data *d);
int	isbanned(char *hostname);
int	newbie_equip(struct char_data *ch);
int parse_menu (char arg);
int	special(struct char_data *ch, int cmd, char *arg);
int	Valid_Name(char *newname);
void assemblies_parse(struct descriptor_data *d, char *arg);
void boot_travels(struct char_data *ch);
void do_start(struct char_data *ch);
void echo_off(struct descriptor_data *d);
void echo_on(struct descriptor_data *d);
void help_class(struct char_data *ch, int chclass);
void help_race(struct char_data *ch, int race);
void help_culture(struct char_data *ch, int culture);
void read_aliases(struct char_data *ch);
void write_aliases(struct char_data *ch);
void read_saved_vars(struct char_data *ch);
void remove_player(int pfilepos);
void assign_base_stats(struct char_data *ch);
void roll_real_abils(struct char_data *ch);
void check_pending_tasks(struct char_data *ch, int subcmd);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
bool valid_email(const char *em);
void race_menu(struct char_data *ch, bool full, bool error);
void culture_menu(struct char_data *ch, bool full, bool error);
void class_menu(struct char_data *ch, bool full, bool error);
bool attributes_menu(struct char_data *ch, char *argument);
int update_attribs(struct char_data *ch);
int	parse_class(struct char_data *ch, char arg);
long get_ptable_by_name(char *name);
void fetch_char_guild_data(struct char_data *ch);
bool email_verification(struct char_data *ch, int subcmd);
void generate_skill_points(struct char_data *ch);
bool skill_selection_menu(struct char_data *ch, char *argument);
void set_default_ldesc(struct char_data *ch);
void add_tell_to_buffer(struct char_data *ch, const char *tell);

/* local functions */
int	perform_dupe_check(struct descriptor_data *d);
void perform_multi_check(struct descriptor_data *d);
int	perform_alias(struct descriptor_data *d, char *orig);
int	reserved_word(char *argument);
int	find_name(const char *name);
int	_parse_name(char *arg, char *name);
void free_alias(struct alias_data *a);
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);
void update_xsyslogs(struct char_data *ch);
int	enter_player_game (struct descriptor_data *d);

const	char *fill[] =
{
	"in",
	"from",
	"with",
	"the",
	"on",
	"at",
	"to",
	"\n"
};

const	char *reserved[] =
{
	"a",
	"an",
	"self",
	"me",
	"all",
	"room",
	"someone",
	"something",
	"\n"
};

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you have the proper rights and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument)
{
	struct char_guild_element *element;
	int cmd, length;
	char *line;

	REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

	/* just drop to next line for hitting CR */
	skip_spaces(&argument);
	if (!*argument)
		return;

	/*
	 * special case to handle one-character, non-alphanumeric commands;
	 * requested by many people so "'hi" or ";godnet test" is possible.
	 * Patch sent by Eric Green and Stefan Wasilewski.
	 */
	if (!isalpha(*argument)) {
		arg[0] = argument[0];
		arg[1] = '\0';
		line = argument + 1;
	} else
		line = any_one_arg(argument, arg);

	/* Since all command triggers check for valid_dg_target before acting, the levelcheck
	 * here has been removed. 
	 */
	/* otherwise, find the command */
	{
		int cont; /* continue the command checks */
		cont = command_wtrigger(ch, arg, line);
		if (!cont) cont += command_mtrigger(ch, arg, line);
		if (!cont) cont = command_otrigger(ch, arg, line);
		if (cont) return; /* command trigger took over */
	}
	/*
	 * This section is for interpreting guild channels and
	 * guild gossips. Notice guild channels and guild gossips 
	 * must have names with more than one letter to work 
	 * correctly, as one-letter words are reserved for dirs
	 */

	if (strlen(arg) > 1) { 
		element = GET_CHAR_GUILDS(ch);
		while (element) {
			if (element->guild->gchan_name != NULL) {
				if (!strncmp(element->guild->gchan_name, arg, strlen(arg))) {
					if (GLD_FLAGGED(element->guild, GLD_NOGCHAN)) {
						send_to_char("Huh?!\r\n", ch);
						return;
					}
					do_guild_channel(ch, element, line);
					return;
				}
			}
			if (GLD_FLAGGED(element->guild, GLD_GOSSIP)) {
				if (element->guild->gossip_name != NULL) {
					if (!strncmp(element->guild->gossip_name, arg, strlen(arg))) {
						do_guild_gossip(ch, element, line);
						return;
					}
				}
			}
			element = element->next;
		}
	}

	for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
		if (!strncmp(complete_cmd_info[cmd].command, arg, length))
			if (IS_NPC(ch) || GOT_RIGHTS(ch, complete_cmd_info[cmd].rights))
				break;

	if (!complete_cmd_info[cmd].enabled) {
		send_to_char("This command has been disabled.\r\n", ch);
		return;
	}

	if (in_copyover && !complete_cmd_info[cmd].copyover) {
		send_to_char("The mud is in copyover mode, you can't do this.\r\n", ch);
		return;
	}

	if (GET_POS(ch) <= complete_cmd_info[cmd].minimum_position) {
		if (GET_POS(ch) == POS_MEDITATING && ((number(1, 101) > (GET_SKILL(ch, SKILL_MEDITATE)/100)))) {
			send_to_char("You break your meditation!\r\n", ch);
			GET_POS(ch) = POS_RESTING;
		}
	}

	if (*complete_cmd_info[cmd].command == '\n') {
		*buf2 = '\0';
		for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
			if (IS_NPC(ch) || GOT_RIGHTS(ch, complete_cmd_info[cmd].rights))
				if (SOUNDEX_MATCH((char*)complete_cmd_info[cmd].command, arg))
					sprintf(buf2 + strlen(buf2), "%s ", complete_cmd_info[cmd].command);
		if (*buf2) {
			sprintf(buf, "Unknown command, \"%s\", perhaps you meant: &W%s&n\r\n", arg, buf2);
			send_to_char(buf, ch);
		} else {
			send_to_char("Huh?!?\r\n", ch);
		}
	}

	else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && !IS_IMPLEMENTOR(ch))
		send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
	else if (!complete_cmd_info[cmd].command_pointer)
		send_to_char("Sorry, that command has not been implemented yet.\r\n", ch);
	else if (IS_NPC(ch) && complete_cmd_info[cmd].rights > RIGHTS_MEMBER)
		send_to_char("You cannot use immortal commands while switched.\r\n", ch);
	else if (GET_POS(ch) < complete_cmd_info[cmd].minimum_position) {
		switch (GET_POS(ch)) {
		case POS_DEAD:
			send_to_char("Lie still; you are DEAD!!!\r\n", ch);
			break;
		case POS_INCAP:
		case POS_MORTALLYW:
			send_to_char("You are in a pretty bad shape, unable to do anything!\r\n", ch);
			break;
		case POS_STUNNED:
			send_to_char("All you can do right now is think about the stars!\r\n", ch);
			break;
		case POS_SLEEPING:
			send_to_char("In your dreams, or what?\r\n", ch);
			break;
		case POS_RESTING:
			send_to_char("Nah... You feel too relaxed to do that..\r\n", ch);
			break;
		case POS_SITTING:
			send_to_char("Maybe you should get on your feet first?\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("No way!  You're fighting for your life!\r\n", ch);
			break;
		case POS_MEDITATING:
			send_to_char("All you can think about is your meditation.\r\n", ch);
			break;
		}
	} else if (no_specials || !special(ch, cmd, line))
		((*complete_cmd_info[cmd].command_pointer) (ch, line, cmd, complete_cmd_info[cmd].subcmd));
}

/**************************************************************************
 * Routines to handle aliasing                                             *
	**************************************************************************/


struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
	while (alias_list != NULL) {
		if (*str == *alias_list->alias)        /* hey, every little bit counts :-) */
			if (!strcmp(str, alias_list->alias))
				return (alias_list);

		alias_list = alias_list->next;
	}

	return (NULL);
}


void free_alias(struct alias_data *a)
{
	if (a->alias)
		free(a->alias);
	if (a->replacement)
		free(a->replacement);
	free(a);
}


/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
	char *repl;
	struct alias_data *a, *temp;

	if (IS_NPC(ch))
		return;

	repl = any_one_arg(argument, arg);

	if (!*arg) {                        /* no argument specified -- list currently defined aliases */
		send_to_char("Currently defined aliases:\r\n", ch);
		if ((a = GET_ALIASES(ch)) == NULL)
			send_to_char(" None.\r\n", ch);
		else {
			while (a != NULL) {
				sprintf(buf, "%-15s %s\r\n", a->alias, a->replacement);
				send_to_char(buf, ch);
				a = a->next;
			}
		}
	} else {                        /* otherwise, add or remove aliases */
		/* is this an alias we've already defined? */
		if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
			REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
			free_alias(a);
		}
		/* if no replacement string is specified, assume we want to delete */
		if (!*repl) {
			if (a == NULL)
				send_to_char("No such alias.\r\n", ch);
			else
				send_to_char("Alias deleted.\r\n", ch);
		} else {                        /* otherwise, either add or redefine an alias */
			if (!str_cmp(arg, "alias")) {
				send_to_char("You can't alias 'alias'.\r\n", ch);
				return;
			}
			CREATE(a, struct alias_data, 1);
			a->alias = str_dup(arg);
			delete_doubledollar(repl);
			a->replacement = str_dup(repl);
			if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
				a->type = ALIAS_COMPLEX;
			else
				a->type = ALIAS_SIMPLE;
			a->next = GET_ALIASES(ch);
			GET_ALIASES(ch) = a;
			send_to_char("Alias added.\r\n", ch);
		}
	}
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define	NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  int num_of_tokens = 0, num;

  /* First, parse the original string */
	skip_spaces(&orig);
  temp = strtok(strcpy(buf2, orig), " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
				strcpy(write_point, tokens[num]);
				write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
				strcpy(write_point, orig);
				write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
				*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int	perform_alias(struct descriptor_data *d, char *orig)
{
	char first_arg[MAX_INPUT_LENGTH], *ptr;
	struct alias_data *a, *tmp;

	/* Mobs don't have alaises. */
	if (IS_NPC(d->character))
		return (0);

	/* bail out immediately if the guy doesn't have any aliases */
	if ((tmp = GET_ALIASES(d->character)) == NULL)
		return (0);

	/* find the alias we're supposed to match */
	skip_spaces(&orig);
	ptr = any_one_arg(orig, first_arg);

	/* bail out if it's null */
	if (!*first_arg)
		return (0);

	/* if the first arg is not an alias, return without doing anything */
	if ((a = find_alias(tmp, first_arg)) == NULL)
		return (0);

	if (a->type == ALIAS_SIMPLE) {
		skip_spaces(&orig);
		strcpy(orig, a->replacement);
		return (0);
	} else {
		perform_complex_alias(&d->input, ptr, a);
		return (1);
	}
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int	search_block(char *arg, const char **list, int exact)
{
	int i, l;

	/*  We used to have \r as the first character on certain array items to
	 *  prevent the explicit choice of that point.  It seems a bit silly to
	 *  dump control characters into arrays to prevent that, so we'll just
	 *  check in here to see if the first character of the argument is '!',
	 *  and if so, just blindly return a '-1' for not found. - ae.
	 */

	if (*arg == '!')
		return (-1);

	/* Make into lower case, and get length of string */
	for (l = 0; *(arg + l); l++)
		*(arg + l) = LOWER(*(arg + l));

	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++)
			if (!strcmp(arg, *(list + i)))
				return (i);
	} else {
		if (!l)
			l = 1;                        /* Avoid "" to match the first available
																 * string */
		for (i = 0; **(list + i) != '\n'; i++)
			if (!strncmp(arg, *(list + i), l))
				return (i);
	}

	return (-1);
}


int	is_number(const char *str)
{
	while (*str)
		if (!isdigit(*(str++)))
			return (0);

	return (1);
}

/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string)
{
	for (; **string && isspace(**string); (*string)++);
}


/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
	char *read, *write;

	/* If the string has no dollar signs, return immediately */
	if ((write = strchr(string, '$')) == NULL)
		return (string);

	/* Start from the location of the first dollar sign */
	read = write;


	while (*read)   /* Until we reach the end of the string... */
		if ((*(write++) = *(read++)) == '$') /* copy one char */
			if (*read == '$')
				read++; /* skip if we saw 2 $'s in a row */

	*write = '\0';

	return (string);
}


int	fill_word(char *argument)
{
	return (search_block(argument, fill, TRUE) >= 0);
}


int	reserved_word(char *argument)
{
	return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
	char *begin = first_arg;

	if (!argument) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "one_argument received a NULL pointer!");
		*first_arg = '\0';
		return (NULL);
	}

	do {
		skip_spaces(&argument);

		first_arg = begin;
		while (*argument && !isspace(*argument)) {
			*(first_arg++) = LOWER(*argument);
			argument++;
		}

		*first_arg = '\0';
	} while (fill_word(begin));

	return (argument);
}


/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(char *argument, char *first_arg)
{
	char *begin = first_arg;

	do {
		skip_spaces(&argument);

		first_arg = begin;

		if (*argument == '\"') {
			argument++;
			while (*argument && *argument != '\"') {
				*(first_arg++) = LOWER(*argument);
				argument++;
			}
			argument++;
		} else {
			while (*argument && !isspace(*argument)) {
				*(first_arg++) = LOWER(*argument);
				argument++;
			}
		}

		*first_arg = '\0';
	} while (fill_word(begin));

	return (argument);
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
	skip_spaces(&argument);

	while (*argument && !isspace(*argument)) {
		*(first_arg++) = LOWER(*argument);
		argument++;
	}

	*first_arg = '\0';

	return (argument);
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
	return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}


char *one_arg_case_sen(char *argument, char *first_arg) 
{
	char *begin = first_arg; 
	
	do { 
		skip_spaces(&argument); 
		first_arg = begin; 
	 
		while (*argument && !isspace(*argument)) {
			*(first_arg++) = (*argument);
			argument++; 
		}   
		
		*first_arg = '\0'; 
	} while (fill_word(begin)); 
	
	return argument; 
} 


/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returns 1 if arg1 is an abbreviation of arg2
 */
int	is_abbrev(const char *arg1, const char *arg2)
{
	if (!*arg1)
		return (0);

	for (; *arg1 && *arg2; arg1++, arg2++)
		if (LOWER(*arg1) != LOWER(*arg2))
			return (0);

	if (!*arg1)
		return (1);
	else
		return (0);
}



/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2)
{
	char *temp;

	temp = any_one_arg(string, arg1);
	skip_spaces(&temp);
	strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int	find_command(const char *command)
{
	int cmd;

	for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
		if (!strcmp(complete_cmd_info[cmd].command, command))
			return (cmd);

	return (-1);
}

/* The opposite of find_command... find_social */
int	find_social(const char *social)
{
	int soc;

	for (soc = 0; *complete_cmd_info[soc].command != '\n'; soc++)
		if (!strcmp(complete_cmd_info[soc].command, social))
			return (soc);

	return (-1);
}


int	special(struct char_data *ch, int cmd, char *arg)
{
	struct obj_data *i;
	struct char_data *k;
	int j;

	/* special in room? */
	if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
		if (GET_ROOM_SPEC(IN_ROOM(ch)) (ch, world + IN_ROOM(ch), cmd, arg))
			return (1);

	/* special in equipment list? */
	for (j = 0; j < NUM_WEARS; j++)
		if (HAS_BODY(ch, j) && GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
			if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
				return (1);

	/* special in inventory? */
	for (i = ch->carrying; i; i = i->next_content)
		if (GET_OBJ_SPEC(i) != NULL)
			if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
				return (1);

	/* special in mobile present? */
	for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
		if (GET_MOB_SPEC(k) != NULL)
			if (GET_MOB_SPEC(k) (ch, k, cmd, arg))
				return (1);

	/* special in object present? */
	for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
		if (GET_OBJ_SPEC(i) != NULL)
			if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
				return (1);

	return (0);
}



/* *************************************************************************
*	 Stuff for controlling the non-playing sockets (get name, pwd etc)       *
*************************************************************************	*/


/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int	find_name(const char *name)
{
	int i;
	
	if (!(*name)) {
		return (-1);
	}

	for (i = 0; i <= top_of_p_table; i++) {
		if (!str_cmp((player_table + i)->name, name))
			return (i);
	}

	return (-1);
}


int	_parse_name(char *arg, char *name)
{
	int i;

	/* skip whitespaces */
	for (; isspace(*arg); arg++);

	for (i = 0; (*name = *arg); arg++, i++, name++)
		if ((!isalpha(*arg) && *arg != '\'') || (i == 0 && *arg == '\'')) /* Allowing apostrophes in names. */
			return (1);

	if (!i)
		return (1);

	return (0);
}


#define	RECON					1
#define	USURP					2
#define	UNSWITCH			3

/*
 * XXX: Make immortals 'return' instead of being disconnected when switched
 *      into person returns.  This function seems a bit over-extended too.
 */
int	perform_dupe_check(struct descriptor_data *d)
{
	struct descriptor_data *k, *next_k;
	struct char_data *target = NULL, *ch, *next_ch;
	int mode = 0;

	int id = GET_IDNUM(d->character);

	/*
	 * Now that this descriptor has successfully logged in, disconnect all
	 * other descriptors controlling a character with the same ID number.
	 */

	for (k = descriptor_list; k; k = next_k) {
		next_k = k->next;

		if (k == d)
			continue;

		if (k->original && (GET_IDNUM(k->original) == id)) {    /* switched char */
			write_to_output(k, TRUE, "\r\nMultiple login detected -- disconnecting.\r\n");
			STATE(k) = CON_CLOSE;
			if (!target) {
				target = k->original;
				mode = UNSWITCH;
			}
			if (k->character)
				k->character->desc = NULL;
			k->character = NULL;
			k->original = NULL;
		} else if (k->character && (GET_IDNUM(k->character) == id)) {
			if (!target && (STATE(k) == CON_PLAYING || STATE(k) == CON_COPYOVER)) {
				write_to_output(k, TRUE, "\r\nThis body has been usurped!\r\n");
				target = k->character;
				mode = USURP;
			}
			k->character->desc = NULL;
			k->character = NULL;
			k->original = NULL;
			write_to_output(k, TRUE, "\r\nMultiple login detected -- disconnecting.\r\n");
			STATE(k) = CON_CLOSE;
		}
	}

 /*
	* now, go through the character list, deleting all characters that
	* are not already marked for deletion from the above step (i.e., in the
	* CON_HANGUP state), and have not already been selected as a target for
	* switching into.  In addition, if we haven't already found a target,
	* choose one if one is available (while still deleting the other
	* duplicates, though theoretically none should be able to exist).
	*/

	for (ch = character_list; ch; ch = next_ch) {
		next_ch = ch->next;

		if (IS_NPC(ch))
			continue;
		if (GET_IDNUM(ch) != id)
			continue;

		/* ignore chars with descriptors (already handled by above step) */
		if (ch->desc)
			continue;

		/* don't extract the target char we've found one already */
		if (ch == target)
			continue;

		/* we don't already have a target and found a candidate for switching */
		if (!target) {
			target = ch;
			mode = RECON;
			continue;
		}

		/* we've found a duplicate - blow him away, dumping his eq in limbo. */
		if (IN_ROOM(ch) != NOWHERE)
			char_from_room(ch);
		char_to_room(ch, 1);
		extract_char(ch);
	}

	/* no target for swicthing into was found - allow login to continue */
	if (!target)
		return (0);

	/* Okay, we've found a target.  Connect d to target. */
	free_char(d->character); /* get rid of the old char */
	d->character = target;
	d->character->desc = d;
	d->original = NULL;
	d->character->char_specials.timer = 0;
	REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING | PLR_OLC);
	REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
	STATE(d) = CON_PLAYING;

	switch (mode) {
	case RECON:
		write_to_output(d, TRUE, "Reconnecting.\r\n");
		act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
		extended_mudlog(NRM, SYSL_LINKS, TRUE, "%s@%s has reconnected.", GET_NAME(d->character), d->host);
		break;
	case USURP:
		write_to_output(d, TRUE, "You take over your own body, already in use!\r\n");
		act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
			"$n's body has been taken over by a new spirit!",
			TRUE, d->character, 0, 0, TO_ROOM);
		extended_mudlog(NRM, SYSL_LINKS, TRUE, "%s@%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character), d->host);
		break;
	case UNSWITCH:
		write_to_output(d, TRUE, "Reconnecting to unswitched char.");
		extended_mudlog(NRM, SYSL_LINKS, TRUE, "%s@%s has reconnected.", GET_NAME(d->character), d->host);
		break;
	}

	if (GET_TELLS(d->character, 0) && SESS_FLAGGED(d->character, SESS_HAVETELLS))
		write_to_output(d, TRUE, "\007&RYou have buffered tells.&n\r\n");

	return (1);
}


/* check if a site is connected with more than one char */
void perform_multi_check(struct descriptor_data *d)
{
	if (!multilogging_allowed) {
		struct char_data *ch, *next_ch;

		int id = GET_IDNUM(d->character);

		for (ch = character_list; ch; ch = next_ch) {
			next_ch = ch->next;

			if (IS_NPC(ch))
				continue;
			if (GET_IDNUM(ch) == id)
				continue;

			if (!strncmp(d->host, GET_HOST(ch), strlen(d->host)) && (!IS_GRGOD(ch) || !IS_IMPL(ch)) && (!IS_GRGOD(d->character) || !IS_IMPL(d->character))) {
				write_to_output(d, TRUE, "\r\n&RYou have been ticketed for multilogging.&n\r\n\r\n");
				extended_mudlog(BRF, SYSL_SITES, TRUE, "***MULTILOG: %s:Possible multilog by %s", d->host, GET_NAME(d->character));
			}

		}

	}

}


/* check the player's extended syslogs and remove those that he has not the right to see */
void update_xsyslogs(struct char_data *ch)
{
	int l;

	for (l = 0; *(log_fields[l].cmd) != '\n'; l++)
		if (!GOT_RIGHTS(ch, log_fields[l].rights))
			REMOVE_BIT(EXTENDED_SYSLOG(ch), 1ULL << l);
}


/* load the player, put them in the right room - used by copyover_recover too */
int	enter_player_game (struct descriptor_data *d)
{
	int load_result;
	sh_int load_room;
	
	reset_char(d->character);
	read_aliases(d->character);
	read_saved_vars(d->character);
	boot_travels(d->character);
	GET_ID(d->character) = GET_IDNUM(d->character);
	update_xsyslogs(d->character);
	fetch_char_guild_data(d->character);
	
	if (PLR_FLAGGED(d->character, PLR_INVSTART))
		GET_INVIS_LEV(d->character) = get_max_rights(d->character);

	if (IS_IC(d->character) && !IS_ACTIVE(d->character))
		REMOVE_BIT(SESSION_FLAGS(d->character), SESS_IC);

	if (SPEAKING(d->character) < MIN_LANGUAGES || SPEAKING(d->character) > MAX_LANGUAGES)
		SPEAKING(d->character) = MIN_LANGUAGES;

	/* True Name sanity check, all players MUST have a true pattern */
	if (!GET_TRUENAME(d->character))
		GET_TRUENAME(d->character) = str_dup(true_name(GET_ID(d->character)));

	if (!IS_IMMORTAL(d->character)) {
		REMOVE_BIT(PRF_FLAGS(d->character), PRF_SHOWVNUMS);
		REMOVE_BIT(PRF_FLAGS(d->character), PRF_HOLYLIGHT);
		EXTENDED_SYSLOG(d->character) = 0;
	}

	/*
	 * We have to place the character in a room before equipping them
	 * or equip_char() will gripe about the person in NOWHERE.
	 */
	if ((IS_IC(d->character) || IS_IMMORTAL(d->character)) && (load_room = GET_LOADROOM(d->character)) != NOWHERE)
		load_room = real_room(load_room);
	else
		load_room = real_room(mortal_start_room[ooc_lounge_index]);
	
	/* if we have NOWHERE currently, let's try the HOMEtown of the char */
	if (load_room == NOWHERE && IS_IC(d->character))
		load_room = real_room(mortal_start_room[GET_HOME(d->character)]);

	/* If char was saved with NOWHERE, or real_room above failed... */
	if (load_room == NOWHERE) {
		if (IS_IMMORTAL(d->character))
			load_room = r_immort_start_room;
		else if (GOT_RIGHTS(d->character, RIGHTS_NONE) && !PLR_FLAGGED(d->character, PLR_NORIGHTS))
			load_room = real_room(mortal_start_room[start_zone_index]);
		else
			load_room = real_room(mortal_start_room[ooc_lounge_index]); /* OOC Lounge */
	}

	if (GOT_RIGHTS(d->character, RIGHTS_NONE) && !PLR_FLAGGED(d->character, PLR_NORIGHTS))
		load_room = real_room(mortal_start_room[start_zone_index]); /* Start Room */

	if (PLR_FLAGGED(d->character, PLR_FROZEN))
		load_room = r_frozen_start_room;
	
	d->character->next = character_list;
	character_list = d->character;
	char_to_room(d->character, load_room);
	load_result = Crash_load(d->character);
	if (d->character->player_specials->host) {
		free(d->character->player_specials->host);
		d->character->player_specials->host = NULL;
	}
	d->character->player_specials->host = str_dup(d->host);
	d->character->player.time.logon = time(0);

	/* Clear their load room if it's not persistant. */
	if (!PLR_FLAGGED(d->character, PLR_LOADROOM))
		GET_LOADROOM(d->character) = NOWHERE;

	save_char(d->character, NOWHERE, FALSE);
	
	return load_result;
}


/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
	MYSQL_RES *playerindex;
	MYSQL_ROW row;
	int rec_count = 0, player_i, load_result, i;
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char *tmp_name = NULL;
	char *esc_name;
	char *esc_password;
	char *get_password = "SELECT Password FROM %s WHERE Password = MD5('%s') AND Name = '%s';";
	char *new_password = "SELECT MD5('%s');";
	struct time_info_data playing_time;
	struct descriptor_data *k;
	char *notify;

	/* OasisOLC states */
	struct {
		int state;
		void (*func)(struct descriptor_data *, char*);
	} olc_functions[] = {
		{ CON_OEDIT, oedit_parse },
		{ CON_ZEDIT, zedit_parse },
		{ CON_SEDIT, sedit_parse },
		{ CON_MEDIT, medit_parse },
		{ CON_REDIT, redit_parse },
		{ CON_AEDIT, aedit_parse },
		{ CON_TRIGEDIT, trigedit_parse },
		{ CON_ASSEDIT, assedit_parse },
		{ CON_EMAIL, email_parse },
		{ CON_QEDIT, qedit_parse },
		{ CON_SPEDIT, spedit_parse },
		{ CON_GEDIT, gedit_parse },
		{ CON_COMEDIT, comedit_parse },
		{ CON_TUTOREDIT, tutoredit_parse },
		{ CON_HEDIT, hedit_parse },
		{ -1, NULL }
	};

	skip_spaces(&arg);

	/*
	 * Quick check for the OLC states.
	 */
	for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
		if (STATE(d) == olc_functions[player_i].state) {
			(*olc_functions[player_i].func)(d, arg);
			release_buffer(printbuf);
			return;
		}

	/* Not in OLC. */
	switch (STATE(d)) {
		case CON_GET_NAME:                /* wait for input of name */
		case CON_GET_BECOME:
			if (d->character != NULL)
				free_char(d->character);  /* clear out the old char */
			CREATE(d->character, struct char_data, 1);
			clear_char(d->character);
			CREATE(d->character->player_specials, struct player_special_data, 1);
			d->character->desc = d;
			
			if (!*arg)
				STATE(d) = CON_CLOSE;
			else {
				tmp_name = get_buffer(MAX_INPUT_LENGTH);
				if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
					strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
					fill_word(strcpy(printbuf, tmp_name)) || reserved_word(printbuf)) {
					write_to_output(d, TRUE, "Invalid name, please try another.\r\n"
						"Name: ");
					release_buffer(tmp_name);
					release_buffer(printbuf);
					return;
				}
				if ((player_i = load_char(tmp_name, d->character)) > -1) {
					GET_PFILEPOS(d->character) = player_i;
						
					if (PLR_FLAGGED(d->character, PLR_DELETED)) {

						/* make sure old entries are removed so the new player doesn't get
							 the deleted player's equipment (this should probably be a
							 stock behavior)
						*/
						if((player_i = find_name(tmp_name)) >= 0)
							remove_player(player_i);

						/* We get a false positive from the original deleted character. */
						free_char(d->character);
						/* Check for multiple creations... */
						if (!Valid_Name(tmp_name)) {
							write_to_output(d, TRUE, "Invalid name, please try another.\r\nName: ");
							release_buffer(tmp_name);
							release_buffer(printbuf);
							return;
						}
						CREATE(d->character, struct char_data, 1);
						clear_char(d->character);
						CREATE(d->character->player_specials, struct player_special_data, 1);
						d->character->desc = d;
						CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
						strcpy(d->character->player.name, CAP(tmp_name));
						GET_PFILEPOS(d->character) = player_i;
						write_to_output(d, TRUE, "%s", namepolicy);
						write_to_output(d, TRUE, "Did I get that right, %s (Y/N)? ", tmp_name);
						STATE(d) = CON_NAME_CNFRM;
					} else {
						/* undo it just in case they are set */
						REMOVE_BIT(PLR_FLAGS(d->character),
											 PLR_WRITING | PLR_MAILING | PLR_CRYO | PLR_OLC);
						REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
						write_to_output(d, TRUE, "Password: ");
						echo_off(d);
						d->idle_tics = 0;
						STATE(d) = CON_PASSWORD;
					}
				} else {
					/* player unknown -- make new character */

					/* Check for multiple creations of a character. */
					if (!Valid_Name(tmp_name)) {
						write_to_output(d, TRUE, "Invalid name, please try another.\r\nName: ");
						release_buffer(tmp_name);
						release_buffer(printbuf);
						return;
					}
					CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
					strcpy(d->character->player.name, CAP(tmp_name));
					/* set pfilepos so that we know this is a new character later on */
					GET_PFILEPOS(d->character) = player_i;

					write_to_output(d, TRUE, "%s", namepolicy);
					write_to_output(d, TRUE, "Did I get that right, %s (Y/N)? ", tmp_name);
					STATE(d) = CON_NAME_CNFRM;
				}
				release_buffer(tmp_name);
			}
			break;
		case CON_NAME_CNFRM:                /* wait for conf. of new name    */
			if (UPPER(*arg) == 'Y') {
				if (isbanned(d->host) >= BAN_NEW) {
					extended_mudlog(NRM, SYSL_SITES, TRUE, "Siteban: Request for new char %s@%s denied.",
									GET_PC_NAME(d->character), d->host);
					write_to_output(d, TRUE, "Sorry, new characters are not allowed from your site!\r\n");
					STATE(d) = CON_CLOSE;
					release_buffer(printbuf);
					return;
				}
				if (circle_restrict) {
					write_to_output(d, TRUE, "Sorry, new players can't be created at the moment.\r\n");
					extended_mudlog(NRM, SYSL_SITES, TRUE, "Wizlock: Request for new char %s@%s denied.",
									GET_PC_NAME(d->character), d->host);
					STATE(d) = CON_CLOSE;
					release_buffer(printbuf);
					return;
				}
				write_to_output(d, TRUE, "\r\nNew character.\r\n");
				write_to_output(d, TRUE, "\r\nGive me a password for %s: ", GET_PC_NAME(d->character));
				echo_off(d);
				STATE(d) = CON_NEWPASSWD;
			} else if (*arg == 'n' || *arg == 'N') {
				write_to_output(d, TRUE, "Okay, what IS it, then? ");
				free(d->character->player.name);
				d->character->player.name = NULL;
				STATE(d) = CON_GET_NAME;
			} else {
				write_to_output(d, TRUE, "Please type Yes or No: ");
			}
			break;
		case CON_PASSWORD:                /* get pwd for known player      */
			/*
			 * To really prevent duping correctly, the player's record should
			 * be reloaded from disk at this point (after the password has been
			 * typed).  However I'm afraid that trying to load a character over
			 * an already loaded character is going to cause some problem down the
			 * road that I can't see at the moment.  So to compensate, I'm going to
			 * (1) add a 15 or 20-second time limit for entering a password, and (2)
			 * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
			 */

			echo_on(d);    /* turn echo back on */
			
			/* New echo_on() eats the return on telnet. Extra space better than none. */
			write_to_output(d, TRUE, "\r\n");
			
			if (!*arg)
				STATE(d) = CON_CLOSE;
			else {

				SQL_MALLOC(arg, esc_password);
				SQL_ESC(arg, esc_password);

				SQL_MALLOC(GET_NAME(d->character), esc_name);
				SQL_ESC(GET_NAME(d->character), esc_name);

				if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, get_password, TABLE_PLAYER_INDEX, esc_password, esc_name))) {
					extended_mudlog(BRF, SYSL_SQL, TRUE, "Could compare player password on table [%s].", TABLE_PLAYER_INDEX);
					release_buffer(printbuf);
					return;
				}

				if (*esc_password)
					SQL_FREE(esc_password);

				if (*esc_name)
					SQL_FREE(esc_name);

				rec_count = mysql_num_rows(playerindex);

				if(rec_count == 0) {
					extended_mudlog(NRM, SYSL_SECURE, TRUE, "Bad password for %s@%s.", GET_NAME(d->character), d->host);
					GET_BAD_PWS(d->character)++;
					save_char(d->character, NOWHERE, FALSE);
					if (++(d->bad_pws) >= max_bad_pws) {        /* 3 strikes and you're out. */
						write_to_output(d, TRUE, "Wrong password... disconnecting.\r\n");
						STATE(d) = CON_CLOSE;
					} else {
						write_to_output(d, TRUE, "Wrong password.\r\nPassword: ");
						echo_off(d);
					}
					release_buffer(printbuf);
					return;
				}

				mysql_free_result(playerindex);

				/* Password was correct. */
				load_result = GET_BAD_PWS(d->character);
				GET_BAD_PWS(d->character) = 0;
				d->bad_pws = 0;
				
				if (isbanned(d->host) == BAN_SELECT &&
					!PLR_FLAGGED(d->character, PLR_SITEOK)) {
					write_to_output(d, TRUE, "Sorry, this char has not been cleared for login from your site!\r\n");
					STATE(d) = CON_CLOSE;
					extended_mudlog(NRM, SYSL_SITES, TRUE, "Connection denied for %s@%s.",
						GET_NAME(d->character), d->host);
					release_buffer(printbuf);
					return;
				}

				if (circle_restrict) {
					if (!GOT_RIGHTS(d->character, (1ULL << circle_restrict))) {
						write_to_output(d, TRUE, "The game is temporarily restricted.. try again later.\r\n");
						STATE(d) = CON_CLOSE;
						extended_mudlog(NRM, SYSL_SITES, TRUE, "Wizlock: Request for login denied for %s@%s.",
							GET_NAME(d->character), d->host);
						release_buffer(printbuf);
						return;
					}
				}

				/* check and make sure no other copies of this player are logged in */
				if (perform_dupe_check(d)) {
					release_buffer(printbuf);
					return;
				}

				if (use_verification) {
					if (!email_verification(d->character, EMAIL_CHECK) && !IS_VERIFIED(d->character)) {
						write_to_output(d, TRUE, "\r\nYour account has not yet been verified.\r\nValid commands are CHECK and QUIT > ");
						STATE(d) = CON_QVERIFICATION;
						release_buffer(printbuf);
						return;
					}
				}

				perform_multi_check(d);

				user_cntr(d);

				if (IS_IMMORTAL(d->character))
					page_string(d, imotd, 0);
				else
					page_string(d, motd, 0);

				extended_mudlog(BRF, SYSL_SITES, TRUE, "%s@%s has connected.", GET_NAME(d->character), d->host);
				
				if (load_result) {
					write_to_output(d, TRUE, "\r\n\r\n\007\007\007"
						"%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
						CCRED(d->character, C_SPR), load_result,
						(load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
					GET_BAD_PWS(d->character) = 0;
				}

				STATE(d) = CON_RMOTD;
			}
			break;
			
		case CON_NEWPASSWD:
		case CON_CHPWD_GETNEW:
			if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
				!str_cmp(arg, GET_PC_NAME(d->character))) {
				write_to_output(d, TRUE, "\r\nIllegal password.\r\n");
				write_to_output(d, TRUE, "Password: ");
				release_buffer(printbuf);
				return;
			}

			SQL_MALLOC(arg, esc_password);
			SQL_ESC(arg, esc_password);

			if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, new_password, esc_password))) {
				extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not create player password on table [%s].", TABLE_PLAYER_INDEX);
				release_buffer(printbuf);
				return;
			}

			if (*esc_password)
				SQL_FREE(esc_password);

			row = mysql_fetch_row(playerindex);

			strncpy(GET_PASSWD(d->character), row[0], MAX_PWD_LENGTH);
			*(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

			mysql_free_result(playerindex);

			write_to_output(d, TRUE, "\r\nPlease retype password: ");
			if (STATE(d) == CON_NEWPASSWD)
				STATE(d) = CON_CNFPASSWD;
			else
				STATE(d) = CON_CHPWD_VRFY;
			
			break;
			
		case CON_CNFPASSWD:
		case CON_CHPWD_VRFY:

			if (strlen(arg) < 3) {
				write_to_output(d, TRUE, "\r\nPasswords don't match... start over.\r\n");
				write_to_output(d, TRUE, "Password: ");
				if (STATE(d) == CON_CNFPASSWD)
					STATE(d) = CON_NEWPASSWD;
				else
					STATE(d) = CON_CHPWD_GETNEW;
				release_buffer(printbuf);
				return;
			}

			SQL_MALLOC(arg, esc_password);
			SQL_ESC(arg, esc_password);

			if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, new_password, esc_password))) {
				extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not create player password on table [%s].", TABLE_PLAYER_INDEX);
				release_buffer(printbuf);
				return;
			}

			if (*esc_password)
				SQL_FREE(esc_password);

			row = mysql_fetch_row(playerindex);

			if (strncmp(row[0], GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
				mysql_free_result(playerindex);
				write_to_output(d, TRUE, "\r\nPasswords don't match... start over.\r\n");
				write_to_output(d, TRUE, "Password: ");
				if (STATE(d) == CON_CNFPASSWD)
					STATE(d) = CON_NEWPASSWD;
				else
					STATE(d) = CON_CHPWD_GETNEW;
				release_buffer(printbuf);
				return;
			}
			echo_on(d);

			mysql_free_result(playerindex);
			
			if (STATE(d) == CON_CNFPASSWD) {
				write_to_output(d, TRUE, "\r\nDoes your terminal support the ANSI color standard? [Y/n] : ");
				STATE(d) = CON_QANSI;
			} else {
				save_char(d->character, NOWHERE, FALSE);
				echo_on(d);
				write_to_output(d, TRUE, "\r\nDone.\r\n");
				write_to_output(d, TRUE, "%s", MENU);
				STATE(d) = CON_MENU;
			}
			break;

		case CON_QANSI:
			if (!*arg || LOWER(*arg) == 'y') {
				SET_BIT(PRF_FLAGS(d->character), PRF_COLOR_1 | PRF_COLOR_2);
				write_to_output(d, TRUE, "\r\n&YColor is &Won&Y.&n\r\n");

				write_to_output(d, TRUE, "%s", EMAIL_MENU);
				if (use_verification) {
					write_to_output(d, TRUE, "%s", VERIFICATION_MENU);
				}
				write_to_output(d, TRUE, "\r\n&WPlease enter a valid email address &n: "); 
				STATE(d) = CON_EGET;

			} else if (LOWER(*arg) == 'n') {
				REMOVE_BIT(PRF_FLAGS(d->character), PRF_COLOR_1 | PRF_COLOR_2);
				write_to_output(d, TRUE, "ANSI color turned off.\r\n");

				write_to_output(d, TRUE, "%s", EMAIL_MENU);
				if (use_verification) {
					write_to_output(d, TRUE, "%s", VERIFICATION_MENU);
				}
				write_to_output(d, TRUE, "\r\nPlease enter a valid email address : "); 
				STATE(d) = CON_EGET;

			} else {
				write_to_output(d, TRUE, "That is not a proper response.\r\n");
				write_to_output(d, TRUE, "\r\nDoes your terminal support the ANSI color standard? [Y/n] : ");
				release_buffer(printbuf);
				return;
			}
			break;
			
		case CON_EGET:
			if (!valid_email(arg)) { /* see the Merury.TODO file -spl */
				write_to_output(d, TRUE, "&YThis is not a valid E-mail address!\r\n");
				write_to_output(d, TRUE, "&WAddress:&n ");
				release_buffer(printbuf);
				return;
			}
			if (!*arg || strlen(arg) > MAX_INPUT_LENGTH || strlen(arg) < 5) {
				write_to_output(d, TRUE, "\r\n&YIllegal address.&n\r\n");
				write_to_output(d, TRUE, "&WAddress:&n ");
				release_buffer(printbuf);
				return;
			}
			strncpy(GET_EMAIL(d->character), arg, MAX_INPUT_LENGTH);
			write_to_output(d, TRUE, "\r\n&WYou have chosen the email address\r\n&Y%s\r\n&Wis this your correct address? [Y/n]&n : ", GET_EMAIL(d->character));
			STATE(d) = CON_QCONFIRMEMAIL;
			break;

		case CON_QCONFIRMEMAIL:
			switch (*arg) {
				case 'n':
				case 'N':
					write_to_output(d, TRUE, "%s", EMAIL_MENU);
				if (use_verification) {
					write_to_output(d, TRUE, "%s", VERIFICATION_MENU);
				}
					write_to_output(d, TRUE, "\r\n&WPlease enter a valid email address:&n "); 
					STATE(d) = CON_EGET;
					break;
				case 'y':
				case 'Y':
				default:
					write_to_output(d, TRUE, "%s", MAILINGLIST_MENU);
					write_to_output(d, TRUE, "\r\n&WDo you wish to be added to our low-frequency news mailing list? [Y/N]&n : ");
					STATE(d) = CON_QCONFIRMELIST;
					break;
			}
			break;

		case CON_QCONFIRMELIST:
			switch (*arg) {
				case 'n':
				case 'N':
					GET_MAILINGLIST(d->character) = NO;
					write_to_output(d, TRUE, "\r\n&YYou will NOT receive email from Arcane Realms.&n\r\n"); 
					break;
				case 'y':
				case 'Y':
					GET_MAILINGLIST(d->character) = YES;
					write_to_output(d, TRUE, "\r\n&YYou have been ADDED to the low-frequency mailinglist.&n\r\n"); 
					break;
				default:
					write_to_output(d, TRUE, "%s", MAILINGLIST_MENU);
					write_to_output(d, TRUE, "\r\n&WDo you wish to be added to our low-frequency news mailing list? [Y/N] &n: ");
					STATE(d) = CON_QCONFIRMELIST;
					release_buffer(printbuf);
					return;
			}
			write_to_output(d, TRUE, "\r\n&WChoose your gender [Male/Female] &n: ");
			STATE(d) = CON_QSEX;
			break;

		case CON_QSEX:                /* query sex of new user         */
			switch (*arg) {
			case 'm':
			case 'M':
				d->character->player.sex = SEX_MALE;
				break;
			case 'f':
			case 'F':
				d->character->player.sex = SEX_FEMALE;
				break;
			default:
				write_to_output(d, TRUE, "&YThat is not a gender...&n\r\n&WChoose your gender [Male/Female] &n: ");
				release_buffer(printbuf);
				return;
			}
			
			/*
			 * Assign the base stats/attributes for the player, including creation points.
			 */
			assign_base_stats(d->character);
			set_race(d->character, RACE_HUMAN);
			culture_menu(d->character, FALSE, FALSE);
			STATE(d) = CON_QCULTURE;
			break;
			
		case CON_QCULTURE:
			load_result = parse_menu(*arg);
			if (load_result == CULTURE_UNDEFINED || load_result > top_of_culture_list) {
				culture_menu(d->character, FALSE, TRUE);
				release_buffer(printbuf);
				return;
			} else {
				if (!culture_list[load_result].selectable) {
					culture_menu(d->character, FALSE, TRUE);
					release_buffer(printbuf);
					return;
				}
				GET_CULTURE(d->character) = load_result;
			}
			write_to_output(d, TRUE, "\r\n");
			GET_PAGE_LENGTH(d->character) = 100;
			help_culture(d->character, GET_CULTURE(d->character));
			write_to_output(d, TRUE, "\r\n&WDo you accept this culture? [Y/N] &n: ");
			STATE(d) = CON_QCONFIRMCULTURE;
			break;
			
		case CON_QCONFIRMCULTURE:
			switch (*arg) {
			case 'y':
			case 'Y':
				GET_CREATION_POINTS(d->character) -= culture_list[(int)GET_CULTURE(d->character)].cost;
				write_to_output(d, TRUE, "\r\n&c.---------------------.\r\n");
				for (i = 0; i < 6; i++)
					write_to_output(d, TRUE, "&c| [&W%c&c] &Y%-15s &c|\r\n", menu_letter(i), short_character_heights[i][(int)GET_SEX(d->character)]);
				write_to_output(d, TRUE, "&c'---------------------'\r\n");
				write_to_output(d, TRUE, "&WHeight &n: ");
				STATE(d) = CON_QHEIGHT;
				break;
			default:
				culture_menu(d->character, FALSE, FALSE);
				STATE(d) = CON_QCULTURE;
				break;
			}
			break;
			
		case CON_QHEIGHT:
			load_result = parse_height(*arg, d->character);
			if (load_result == ATTRIB_UNDEFINED) {
				write_to_output(d, TRUE, "\r\nTry again.\r\n&WHeight &n: ");
				release_buffer(printbuf);
				return;
			} else
				GET_HEIGHT(d->character) = load_result;
			write_to_output(d, TRUE, "\r\n&c.---------------------.\r\n");
			for (i = 0; i < 6; i++)
				write_to_output(d, TRUE, "&c| [&W%c&c] &Y%-15s &c|\r\n", menu_letter(i), short_character_weights[i][(int)GET_SEX(d->character)]);
			write_to_output(d, TRUE, "&c'---------------------'\r\n");
			write_to_output(d, TRUE, "&WWeight &n: ");
			STATE(d) = CON_QWEIGHT;
			break;
			
		case CON_QWEIGHT:
			load_result = parse_weight(*arg, d->character);
			if (load_result == ATTRIB_UNDEFINED) {
				write_to_output(d, TRUE, "\r\nTry again.\r\n&WWeight &n: ");
				release_buffer(printbuf);
				return;
			} else
				GET_WEIGHT(d->character) = load_result;
			write_to_output(d, TRUE, "\r\n&c.---------------------.\r\n");
			for (i = 0; i < num_select_eyes; i++)
				write_to_output(d, TRUE, "&c| [&W%c&c] &Y%-15s &c|\r\n", menu_letter(i), eye_color[i]);
			write_to_output(d, TRUE, "&c'---------------------'\r\n");
			write_to_output(d, TRUE, "&WEye Color &n: ");
			STATE(d) = CON_QEYECOLOR;
			break;
			
		case CON_QEYECOLOR:
			load_result = parse_features(*arg, num_select_eyes);
			if (load_result == ATTRIB_UNDEFINED) {
				write_to_output(d, TRUE, "\r\nTry again.\r\n&WEye Color &n: ");
				release_buffer(printbuf);
				return;
			} else
				GET_EYECOLOR(d->character) = load_result;
			write_to_output(d, TRUE, "\r\n&c.---------------------.\r\n");
			for (i = 0; i < num_select_hair; i++)
				write_to_output(d, TRUE, "&c| [&W%c&c] &Y%-15s &c|\r\n", menu_letter(i), hair_color[i]);
			write_to_output(d, TRUE, "&c'---------------------'\r\n");
			write_to_output(d, TRUE, "&WHair Color &n: ");
			STATE(d) = CON_QHAIRCOLOR;
			break;
			
		case CON_QHAIRCOLOR:
			load_result = parse_features(*arg, num_select_hair);
			if (load_result == ATTRIB_UNDEFINED) {
				write_to_output(d, TRUE, "\r\nTry again.\r\n&WHair Color &n: ");
				release_buffer(printbuf);
				return;
			} else
				GET_HAIRCOLOR(d->character) = load_result;
			write_to_output(d, TRUE, "\r\n&c.---------------------.\r\n");
			for (i = 0; i < NUM_HAIRSTYLES; i++)
				write_to_output(d, TRUE, "&c| [&W%c&c] &Y%-15s &c|\r\n", menu_letter(i), hair_style[i]);
			write_to_output(d, TRUE, "&c'---------------------'\r\n");
			write_to_output(d, TRUE, "&WHair Style &n: ");
			STATE(d) = CON_QHAIRSTYLE;
			break;
			
		case CON_QHAIRSTYLE:
			load_result = parse_features(*arg, NUM_HAIRSTYLES);
			if (load_result == ATTRIB_UNDEFINED) {
				write_to_output(d, TRUE, "\r\nTry again.\r\n&WHair Style &n: ");
				release_buffer(printbuf);
				return;
			} else
				GET_HAIRSTYLE(d->character) = load_result;
			write_to_output(d, TRUE, "\r\n&c.---------------------.\r\n");
			for (i = 0; i < NUM_SKINTONES; i++)
				write_to_output(d, TRUE, "&c| [&W%c&c] &Y%-15s &c|\r\n", menu_letter(i), skin_tone[i]);
			write_to_output(d, TRUE, "&c'---------------------'\r\n");
			write_to_output(d, TRUE, "&WSkin Tone &n: ");
			STATE(d) = CON_QSKINTONE;
			break;
			
		case CON_QSKINTONE:
			load_result = parse_features(*arg, NUM_SKINTONES);
			if (load_result == ATTRIB_UNDEFINED) {
				write_to_output(d, TRUE, "\r\nTry again.\r\n&WSkin Tone &n: ");
				release_buffer(printbuf);
				return;
			} else
				GET_SKINTONE(d->character) = load_result;
			/*
			 * Now go ahead and create a pfile position and save
			 * the char down so we can continue.
			 */
			if (GET_PFILEPOS(d->character) < 0)
				GET_PFILEPOS(d->character) = create_entry(GET_NAME(d->character));
			init_char(d->character);
			SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
			save_char(d->character, NOWHERE, FALSE);
			extended_mudlog(NRM, SYSL_NEWBIES, TRUE, "Newbie alert: a %s %s (%s).", genders[(int)GET_SEX(d->character)], race_list[(int)GET_RACE(d->character)].name, GET_NAME(d->character));
			extended_mudlog(BRF, SYSL_SITES, TRUE, "%s@%s new player.", GET_NAME(d->character), d->host);
			SET_BIT(PLR_FLAGS(d->character), PLR_NEWBIE);
			SET_BIT(PRF_FLAGS(d->character), PRF_HASPROMPT);
			set_default_ldesc(d->character);
			/*
			 * Pass player on to hometown selection.
			 */
			write_to_output(d, TRUE, "\r\n&c.---------------------.\r\n");
			for (i = 1; i <= NUM_MORTAL_STARTROOMS; i++)
				write_to_output(d, TRUE, "&c| [&W%c&c] &Y%-15s &c|\r\n", menu_letter(i), hometowns[i]);
			write_to_output(d, TRUE, "&c'---------------------'\r\n");
			write_to_output(d, TRUE, "&WHome Town &n: ");
			STATE(d) = CON_QHOMETOWN;
			break;

		case CON_QHOMETOWN:
			load_result = parse_features(*arg, NUM_MORTAL_STARTROOMS);
			if (load_result == ATTRIB_UNDEFINED || load_result == 0) {
				write_to_output(d, TRUE, "\r\nTry again.\r\n&WHome Town &n: ");
				release_buffer(printbuf);
				return;
			} else
				GET_HOME(d->character) = load_result;
			/*
			 * Pass the player on to the custom creation menu.
			 */
			attributes_menu(d->character, "list");
			STATE(d) = CON_QROLLSTATS;
			break;
			
		case CON_QROLLSTATS:
			if (!attributes_menu(d->character, arg)) {
				release_buffer(printbuf);
				return;
			} else {
				GET_PAGE_LENGTH(d->character) = PAGE_LENGTH;
				update_attribs(d->character);
				REMOVE_BIT(PLR_FLAGS(d->character), PLR_DELETED);
				/*
				 * Let player select starting skills.
				 */
				generate_skill_points(d->character);
				for (i = 1; i <= MAX_SKILLS; i++) {
					GET_SKILL(d->character, i) = 0;
					GET_SKILL_STATE(d->character, i) = SSTATE_INCREASE;
				}
				assign_skill_bases(d->character);
				skill_selection_menu(d->character, "list");
				STATE(d) = CON_QSELECTSKILLS;
				break;
			}

		case CON_QSELECTSKILLS:
			if (!skill_selection_menu(d->character, arg)) {
				release_buffer(printbuf);
				return;
			} else {
				save_char(d->character, NOWHERE, FALSE);
				if (use_verification) {
					if (email_verification(d->character, EMAIL_VERIFY)) {
						write_to_output(d, TRUE, "%s", VERIFICATION_SENT);
						write_to_output(d, TRUE, "\r\nYour account has not yet been verified.\r\nValid commands are CHECK and QUIT > ");
						STATE(d) = CON_QVERIFICATION;
					} else {
						user_cntr(d);
						page_string(d, motd, 0);
						STATE(d) = CON_RMOTD;
					}
				} else {
					user_cntr(d);
					page_string(d, motd, 0);
					STATE(d) = CON_RMOTD;
					IS_VERIFIED(d->character) = TRUE;
				}
				break;
			}

		case CON_QVERIFICATION:
			if (email_verification(d->character, EMAIL_CHECK)) {
				save_char(d->character, NOWHERE, FALSE);
				user_cntr(d);
				page_string(d, motd, 0);
				STATE(d) = CON_RMOTD;
			} else {
				if (isname(arg, "check")) {
					write_to_output(d, TRUE, "\r\nYour account has not yet been verified.\r\nValid commands are CHECK and QUIT > ");
				} else if (isname(arg, "quit")) {
					REMOVE_BIT(PLR_FLAGS(d->character), PLR_DELETED);
					save_char(d->character, NOWHERE, FALSE);
					write_to_output(d, TRUE, "Fare thee well traveller.\r\n");
					STATE(d) = CON_CLOSE;
				} else {
					write_to_output(d, TRUE, "\r\nValid commands are CHECK and QUIT > ");
				}
			}
			break;

		case CON_RMOTD:                /* read CR after printing motd   */
			/*
			 * Check player to see if they have PLR_FORCE_RENAME set
			 * upon login, if so, force them to rename their character.
			 * Torgny Bjers <artovil@arcanerealms.org>, 2002-06-20
			 */
			if (IS_SET(PLR_FLAGS(d->character), PLR_FORCE_RENAME)) {
				extended_mudlog(NRM, SYSL_SITES, TRUE, "%s@%s was forced to rename.",
					GET_NAME(d->character), d->host);
				write_to_output(d, TRUE, "\r\n&RYour name was not allowed, and you have to select a new name now.&n\r\n\r\n");
				write_to_output(d, TRUE, "%s", namepolicy);
				write_to_output(d, TRUE, "New name: ");
				if (GET_APPROVEDBY(d->character)) {
					free(GET_APPROVEDBY(d->character));
					GET_APPROVEDBY(d->character) = NULL;
				}
				IS_ACTIVE(d->character) = FALSE;
				if (Valid_Name(GET_NAME(d->character))) {
					char tempname[MAX_INPUT_LENGTH];
					FILE *fp;
					/*
					 * Xname the old name to make sure they, or anybody else,
					 * cannot select it again.
					 */
					strcpy(tempname, GET_NAME(d->character));
					if(!(fp = fopen(XNAME_FILE, "a"))) {
						perror("Problems opening xname file for PLR_FORCE_RENAME");
						return;
					}
					for (i = 0; tempname[i]; i++)
						tempname[i] = LOWER(tempname[i]);
					fprintf(fp, "%s\n", tempname);
					fclose(fp);
				}
				STATE(d) = CON_RENAME;
				release_buffer(printbuf);
				return;
			}

			if (!IS_ACTIVE(d->character))
				write_to_output(d, TRUE, "&MYour character is not yet approved.\r\nThis usually takes less than 24 hours.&n\r\n");
			else
				check_pending_tasks(d->character, SCMD_EVENTS);
			write_to_output(d, TRUE, "%s", MENU);
			STATE(d) = CON_MENU;
			break;
			
		case CON_MENU:                /* get selection from main menu  */
			switch (*arg) {
				case '0':
					write_to_output(d, TRUE, "Fare thee well traveller.\r\n");
					STATE(d) = CON_CLOSE;
					break;

				case '1':
					if (!(load_result = enter_player_game(d)))
						send_to_char(WELC_MESSG, d->character);
					if (d->character->player.time.played == 0)
						extended_mudlog(NRM, SYSL_LOGINS, TRUE, "%s@%s has entered the game for the first time.", GET_NAME(d->character), d->host);
					if (GET_TRAVELS(d->character) && GET_TRAVELS(d->character)->gin)
						strcpy(buf2, GET_TRAVELS(d->character)->gin);
					else
						strcpy(buf2, travel_defaults[TRAV_GIN]);
					act(buf2, TRUE, d->character, 0, 0, TO_ROOM);
					/* tell the people who should be notified that this person has logged in */
					notify = get_buffer(1024);
					for (k = descriptor_list; k; k = k->next) {
						if ((STATE(k) == CON_PLAYING || STATE(k) == CON_COPYOVER) && !EDITING(k) && k->character && 
							k->character != d->character && find_notify(k->character, d->character)) {
							sprintf(notify, "&C%s has logged on.&n", GET_NAME(d->character));
							send_to_charf(k->character, "%s\r\n", notify);
							add_tell_to_buffer(k->character, notify);
						}
					}
					release_buffer(notify);

					playing_time = *real_time_passed((time(0) - d->character->player.time.logon) + (d->character->player.time.played), 0);

					if (IS_SET(PLR_FLAGS(d->character), PLR_NEWBIE)) {
						if (playing_time.day > 0) {
							REMOVE_BIT(PLR_FLAGS(d->character), PLR_NEWBIE);
							REMOVE_BIT(PLR_FLAGS(d->character), PLR_NOSHOUT);
							write_to_output(d, TRUE, "\007&RYour character is older than 24 game hours, and therefore your newbie status\r\nhas worn off.  You will be unable to use newbie equipment and gain automatic\r\nhelp from mobiles now.&n\r\n\r\n");
						}
					}

					if (GOT_RIGHTS(d->character, RIGHTS_NONE) && !PLR_FLAGGED(d->character, PLR_NORIGHTS)) {
						USER_RIGHTS(d->character) = RIGHTS_MEMBER;
						player_table[GET_PFILEPOS(d->character)].rights = USER_RIGHTS(d->character);
						if (!(newbie_equip(d->character)))
							extended_mudlog(NRM, SYSL_BUGS, TRUE, "Could not newbie_equip %s.\r\n", GET_NAME(d->character));
						do_start(d->character);
						send_to_char(START_MESSG, d->character);
					}

					greet_mtrigger(d->character, -1);
					greet_memory_mtrigger(d->character);
				
					STATE(d) = CON_PLAYING;

					look_at_room(d->character, 0);
					if (load_result == 2) { /* rented items lost */
						send_to_char("\r\n\007You could not afford your rent!\r\n"
												 "Your possesions have been donated to the Salvation Army!\r\n",
												 d->character);
					}
					check_pending_tasks(d->character, SCMD_TASKS);
					receive_mail(d->character);
					d->has_prompt = 0;
					break;

				case '2':
					STATE(d) = CON_BACKGROUND;
					clear_screen(d);
					write_to_output(d, TRUE, "%s", stredit_header);
					if (d->character->player.description)
						write_to_output(d, FALSE, "%s", d->character->player.description);
					string_write(d, &d->character->player.description, MAX_BACKGROUND_LENGTH, 0, EDIT_BACKGROUND);
					break;

				case '3':
					page_string(d, background, 0);
					STATE(d) = CON_RMOTD;
					break;

				case '4':
					if (GET_MAILINGLIST(d->character))
						GET_MAILINGLIST(d->character) = 0;
					else
						GET_MAILINGLIST(d->character) = 1;
					write_to_output(d, TRUE, "\r\nMailing list has been turned: %s.\r\n", GET_MAILINGLIST(d->character)?"ON":"OFF");
					write_to_output(d, TRUE, "%s", MENU);
					break;

				case '5':
					write_to_output(d, TRUE, "\r\nEnter your new email address: ");
					STATE(d) = CON_CHANGEEMAIL;
					break;

				case '6':
					write_to_output(d, TRUE, "\r\nEnter your old password: ");
					echo_off(d);
					STATE(d) = CON_CHPWD_GETOLD;
					break;

				case '7':
					write_to_output(d, TRUE, "\r\nEnter your password for verification: ");
					echo_off(d);
					STATE(d) = CON_DELCNF1;
					break;

				default:
					write_to_output(d, TRUE, "\r\nThat's not a menu choice!\r\n");
					write_to_output(d, TRUE, "%s", MENU);
					break;
				}
			break;

		case CON_CHANGEEMAIL:
			if (!valid_email(arg)) { /* see the Merury.TODO file -spl */
				write_to_output(d, TRUE, "This is not a valid E-mail address!\r\n");
				write_to_output(d, TRUE, "\r\nEnter your new email address: ");
				release_buffer(printbuf);
				return;
			}
			if (!*arg || strlen(arg) > MAX_INPUT_LENGTH || strlen(arg) < 5) {
				write_to_output(d, TRUE, "\r\nIllegal address.\r\n");
				write_to_output(d, TRUE, "\r\nEnter your new email address: ");
				release_buffer(printbuf);
				return;
			}
			strncpy(GET_EMAIL(d->character), arg, MAX_INPUT_LENGTH);
			write_to_output(d, TRUE, "%s", MENU);
			STATE(d) = CON_MENU;
			release_buffer(printbuf);
			return;
		
		case CON_CHPWD_GETOLD:

			SQL_MALLOC(arg, esc_password);
			SQL_ESC(arg, esc_password);

			SQL_MALLOC(GET_NAME(d->character), esc_name);
			SQL_ESC(GET_NAME(d->character), esc_name);

			if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, get_password, TABLE_PLAYER_INDEX, esc_password, esc_name))) {
				extended_mudlog(BRF, SYSL_SQL, TRUE, "Could compare player password on table [%s].", TABLE_PLAYER_INDEX);
				release_buffer(printbuf);
				return;
			}

			if (*esc_password)
				SQL_FREE(esc_password);

			if (*esc_name)
				SQL_FREE(esc_name);

			rec_count = mysql_num_rows(playerindex);

			mysql_free_result(playerindex);

			if(rec_count == 0) {
				echo_on(d);
				write_to_output(d, TRUE, "\r\nIncorrect password.\r\n");
				write_to_output(d, TRUE, "%s", MENU);
				STATE(d) = CON_MENU;
			} else {
				write_to_output(d, TRUE, "\r\nEnter a new password: ");
				STATE(d) = CON_CHPWD_GETNEW;
			}
			release_buffer(printbuf);
			return;
			
		case CON_DELCNF1:
			echo_on(d);

			SQL_MALLOC(arg, esc_password);
			SQL_ESC(arg, esc_password);

			SQL_MALLOC(GET_NAME(d->character), esc_name);
			SQL_ESC(GET_NAME(d->character), esc_name);

			if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, get_password, TABLE_PLAYER_INDEX, esc_password, esc_name))) {
				extended_mudlog(BRF, SYSL_SQL, TRUE, "Could compare player password on table [%s].", TABLE_PLAYER_INDEX);
				release_buffer(printbuf);
				return;
			}

			if(*esc_password)
				SQL_FREE(esc_password);

			if(*esc_name)
				SQL_FREE(esc_name);

			rec_count = mysql_num_rows(playerindex);

			mysql_free_result(playerindex);

			if(rec_count == 0) {
				write_to_output(d, TRUE, "\r\nIncorrect password.\r\n");
				write_to_output(d, TRUE, "%s", MENU);
				STATE(d) = CON_MENU;
			} else {
				write_to_output(d, TRUE, "\r\n&RYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
					"ARE YOU ABSOLUTELY SURE?&n\r\n\r\n"
					"Please type \"yes\" to confirm: ");
				STATE(d) = CON_DELCNF2;
			}
			break;
			
		case CON_DELCNF2:
			if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
				if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
					write_to_output(d, TRUE, "You try to kill yourself, but the ice stops you.\r\n");
					write_to_output(d, TRUE, "Character not deleted.\r\n\r\n");
					STATE(d) = CON_CLOSE;
					release_buffer(printbuf);
					return;
				}
				if (!IS_GRGOD(d->character))
					SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
				save_char(d->character, NOWHERE, FALSE);

				/* If the selfdelete_fastwipe flag is set (in config.c), remove all
					 the player's immediately
				*/ 
				if(selfdelete_fastwipe) {
					if((player_i = find_name(GET_NAME(d->character))) >= 0) {
						SET_BIT(player_table[player_i].flags, PINDEX_SELFDELETE);
						remove_player(player_i);
					}
                } else
    				Crash_delete_file(GET_IDNUM(d->character));


				write_to_output(d, TRUE, "Character '%s' deleted!\r\n"
					"Goodbye.\r\n", GET_NAME(d->character));
				extended_mudlog(NRM, SYSL_LOGINS, TRUE, "%s@%s has self-deleted.", GET_NAME(d->character), GET_HOST(d->character));
				STATE(d) = CON_CLOSE;
				release_buffer(printbuf);
				return;
			} else {
				write_to_output(d, TRUE, "\r\nCharacter not deleted.\r\n");
				write_to_output(d, TRUE, MENU);
				STATE(d) = CON_MENU;
			}
			break;
			
		case CON_RENAME:
			tmp_name = get_buffer(MAX_INPUT_LENGTH);
			if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
				strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
				fill_word(strcpy(printbuf, tmp_name)) || reserved_word(printbuf) || !*arg) {
				write_to_output(d, TRUE, "Invalid name, please try another.\r\n"
					"Name: ");
				release_buffer(tmp_name);
				release_buffer(printbuf);
				return;
			}
			if((player_i = find_name(tmp_name)) >= 0) {
				write_to_output(d, TRUE, "Invalid name, please try another.\r\n"
					"Name: ");
				release_buffer(tmp_name);
				release_buffer(printbuf);
				return;
			}
			remove_player(GET_PFILEPOS(d->character));
			player_table[GET_PFILEPOS(d->character)].name = str_dup(tmp_name);
			player_table[GET_PFILEPOS(d->character)].active = FALSE;
			extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s force-renamed to \"%s\".", d->character->player.name, CAP(tmp_name));
			if (d->character->player.name) {
				free(d->character->player.name);
				d->character->player.name = NULL;
			}
			CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
			strcpy(d->character->player.name, CAP(tmp_name));
			REMOVE_BIT(PLR_FLAGS(d->character), PLR_FORCE_RENAME);
			write_aliases(d->character);
			crash_datasave(d->character, 0, RENT_CRASH);
			save_char(d->character, NOWHERE, FALSE);
			write_to_output(d, TRUE, "\r\n&yYour new name, \"&Y%s&y\", still has to be approved.&n\r\n", CAP(tmp_name));
			write_to_output(d, TRUE, "%s", MENU);
			STATE(d) = CON_MENU;
			release_buffer(tmp_name);
			release_buffer(printbuf);
			return;
		
		 /*
			* It's possible, if enough pulses are missed, to kick someone off
			* while they are at the password prompt. We'll just defer to let  
			* the game_loop() axe them.
			*/
		case CON_CLOSE:
			break;
			
		default:
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Nanny: illegal state of con'ness (%d) for %s@%s; closing connection.",
				STATE(d), d->character ? GET_NAME(d->character) : "unknown", d->host ? d->host : "nowhere");
			STATE(d) = CON_DISCONNECT;        /* Safest to do. */
			break;
	}

	release_buffer(printbuf);

}
