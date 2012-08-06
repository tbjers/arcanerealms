/* ************************************************************************
*  File: aedit.c                                                          *
*  Comment: OLC for MUDs -- this one edits socials                        *
*  by Michael Scott <scottm@workcomm.net> -- 06/10/96                     *
*  for use with OasisOLC                                                  *
*  ftpable from ftp.circlemud.org:/pub/CircleMUD/contrib/code             *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001, Torgny Bjers.                                       *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
************************************************************************ */
/* $Id: aedit.c,v 1.19 2002/11/06 18:55:46 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "interpreter.h"
#include "handler.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "oasis.h"
#include "screen.h"

extern int      top_of_socialt;
extern struct   social_messg	*soc_mess_list;
extern char     *position_types[];

/* WARNING: if you have added diagonal directions and have them at the
 * beginning of the command list.. change this value to 11 or 15 (depending) */
/* reserve these commands to come straight from the cmd list then start
 * sorting */
#define	RESERVE_CMDS		7

/* external functs */
void sort_commands(void); /* aedit patch -- M. Scott */
void create_command_list(void);
void free_action(struct social_messg *action);


/* function protos */
void aedit_disp_menu(struct descriptor_data * d);
void aedit_parse(struct descriptor_data * d, char *arg);
void aedit_setup_new(struct descriptor_data *d);
void aedit_setup_existing(struct descriptor_data *d, int real_num);
void aedit_save_to_disk(struct descriptor_data *d);
void aedit_mysql_save(struct descriptor_data *d);
void aedit_save_internally(struct descriptor_data *d);



/*
 * Utils and exported functions.
 */

void aedit_setup_new(struct descriptor_data *d) {
	 CREATE(OLC_ACTION(d), struct social_messg, 1);
	 OLC_ACTION(d)->id = 0;
	 OLC_ACTION(d)->command = str_dup(OLC_STORAGE(d));
	 OLC_ACTION(d)->sort_as = str_dup(OLC_STORAGE(d));
	 OLC_ACTION(d)->hide    = 0;
	 OLC_ACTION(d)->min_victim_position = POS_STANDING;
	 OLC_ACTION(d)->min_char_position   = POS_STANDING;
	 OLC_ACTION(d)->char_no_arg = str_dup("This action is unfinished.");
	 OLC_ACTION(d)->others_no_arg = str_dup("This action is unfinished.");
	 OLC_ACTION(d)->char_found = NULL;
	 OLC_ACTION(d)->others_found = NULL;
	 OLC_ACTION(d)->vict_found = NULL;
	 OLC_ACTION(d)->not_found = NULL;
	 OLC_ACTION(d)->char_auto = NULL;
	 OLC_ACTION(d)->others_auto = NULL;
	 OLC_ACTION(d)->char_body_found = NULL;
	 OLC_ACTION(d)->others_body_found = NULL;
	 OLC_ACTION(d)->vict_body_found = NULL;
	 OLC_ACTION(d)->char_obj_found = NULL;
	 OLC_ACTION(d)->others_obj_found = NULL;
	 aedit_disp_menu(d);
	 OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void aedit_setup_existing(struct descriptor_data *d, int real_num) {
	 CREATE(OLC_ACTION(d), struct social_messg, 1);
	 OLC_ACTION(d)->id = soc_mess_list[real_num].id;
	 OLC_ACTION(d)->command = str_dup(soc_mess_list[real_num].command);
	 OLC_ACTION(d)->sort_as = str_dup(soc_mess_list[real_num].sort_as);
	 OLC_ACTION(d)->hide    = soc_mess_list[real_num].hide;
	 OLC_ACTION(d)->min_victim_position = soc_mess_list[real_num].min_victim_position;
	 OLC_ACTION(d)->min_char_position   = soc_mess_list[real_num].min_char_position;
	 if (soc_mess_list[real_num].char_no_arg)
		 OLC_ACTION(d)->char_no_arg = str_dup(soc_mess_list[real_num].char_no_arg);
	 if (soc_mess_list[real_num].others_no_arg)
		 OLC_ACTION(d)->others_no_arg = str_dup(soc_mess_list[real_num].others_no_arg);
	 if (soc_mess_list[real_num].char_found)
		 OLC_ACTION(d)->char_found = str_dup(soc_mess_list[real_num].char_found);
	 if (soc_mess_list[real_num].others_found)
		 OLC_ACTION(d)->others_found = str_dup(soc_mess_list[real_num].others_found);
	 if (soc_mess_list[real_num].vict_found)
		 OLC_ACTION(d)->vict_found = str_dup(soc_mess_list[real_num].vict_found);
	 if (soc_mess_list[real_num].not_found)
		 OLC_ACTION(d)->not_found = str_dup(soc_mess_list[real_num].not_found);
	 if (soc_mess_list[real_num].char_auto)
		 OLC_ACTION(d)->char_auto = str_dup(soc_mess_list[real_num].char_auto);
	 if (soc_mess_list[real_num].others_auto)
		 OLC_ACTION(d)->others_auto = str_dup(soc_mess_list[real_num].others_auto);
	 if (soc_mess_list[real_num].char_body_found)
		 OLC_ACTION(d)->char_body_found = str_dup(soc_mess_list[real_num].char_body_found);
	 if (soc_mess_list[real_num].others_body_found)
		 OLC_ACTION(d)->others_body_found = str_dup(soc_mess_list[real_num].others_body_found);
	 if (soc_mess_list[real_num].vict_body_found)
		 OLC_ACTION(d)->vict_body_found = str_dup(soc_mess_list[real_num].vict_body_found);
	 if (soc_mess_list[real_num].char_obj_found)
		 OLC_ACTION(d)->char_obj_found = str_dup(soc_mess_list[real_num].char_obj_found);
	 if (soc_mess_list[real_num].others_obj_found)
		 OLC_ACTION(d)->others_obj_found = str_dup(soc_mess_list[real_num].others_obj_found);
	 OLC_VAL(d) = 0;
	 aedit_disp_menu(d);
}


void aedit_save_internally(struct descriptor_data *d) {
	struct social_messg *new_soc_mess_list = NULL;
	int i;

	/* add a new social into the list */
	if (OLC_ZNUM(d) > top_of_socialt)  {
		CREATE(new_soc_mess_list, struct social_messg, top_of_socialt + 2);
		for (i = 0; i <= top_of_socialt; i++)
			new_soc_mess_list[i] = soc_mess_list[i];
		new_soc_mess_list[++top_of_socialt] = *OLC_ACTION(d);
		free(soc_mess_list);
		soc_mess_list = new_soc_mess_list;
		create_command_list();
		sort_commands();
		aedit_mysql_save(d);
	}
	/* pass the editted action back to the list - no need to add */
	else {
		i = find_command(OLC_ACTION(d)->command);
		aedit_mysql_save(d);
		OLC_ACTION(d)->act_nr = soc_mess_list[OLC_ZNUM(d)].act_nr;
		/* why did i do this..? hrm */
		free_action(soc_mess_list + OLC_ZNUM(d));
		soc_mess_list[OLC_ZNUM(d)] = *OLC_ACTION(d);
		if (i > NOTHING) {
			complete_cmd_info[i].command = soc_mess_list[OLC_ZNUM(d)].command;
			complete_cmd_info[i].sort_as = soc_mess_list[OLC_ZNUM(d)].sort_as;
			complete_cmd_info[i].minimum_position = soc_mess_list[OLC_ZNUM(d)].min_char_position;
			complete_cmd_info[i].rights = RIGHTS_MEMBER;
		}

	}

}


/*------------------------------------------------------------------------*/

void aedit_mysql_save(struct descriptor_data *d)
{
	char *command=NULL;
	char *sort_as=NULL;
	char *char_no_arg=NULL;
	char *others_no_arg=NULL;
	char *char_found=NULL;
	char *others_found=NULL;
	char *vict_found=NULL;
	char *not_found=NULL;
	char *char_auto=NULL;
	char *others_auto=NULL;
	char *char_body_found=NULL;
	char *others_body_found=NULL;
	char *vict_body_found=NULL;
	char *char_obj_found=NULL;
	char *others_obj_found=NULL;

	char *replace = "REPLACE INTO %s ("
		"id, "
		"command, "
		"sort_as, "
		"hide, "
		"min_char_position, "
		"min_victim_position, "
		"char_no_arg, "
		"others_no_arg, "
		"char_found, "
		"others_found, "
		"vict_found, "
		"not_found, "
		"char_auto, "
		"others_auto, "
		"char_body_found, "
		"others_body_found, "
		"vict_body_found, "
		"char_obj_found, "
		"others_obj_found) "
		"VALUES (%d, '%s', '%s', %d, %d, %d, '%s', '%s', '%s', "
		"'%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s');";

	SQL_MALLOC(OLC_ACTION(d)->command, command);
	SQL_MALLOC(OLC_ACTION(d)->sort_as, sort_as);
	SQL_MALLOC(OLC_ACTION(d)->char_no_arg, char_no_arg);
	SQL_MALLOC(OLC_ACTION(d)->others_no_arg, others_no_arg);
	SQL_MALLOC(OLC_ACTION(d)->char_found, char_found);
	SQL_MALLOC(OLC_ACTION(d)->others_found, others_found);
	SQL_MALLOC(OLC_ACTION(d)->vict_found, vict_found);
	SQL_MALLOC(OLC_ACTION(d)->not_found, not_found);
	SQL_MALLOC(OLC_ACTION(d)->char_auto, char_auto);
	SQL_MALLOC(OLC_ACTION(d)->others_auto, others_auto);
	SQL_MALLOC(OLC_ACTION(d)->char_body_found, char_body_found);
	SQL_MALLOC(OLC_ACTION(d)->others_body_found, others_body_found);
	SQL_MALLOC(OLC_ACTION(d)->vict_body_found, vict_body_found);
	SQL_MALLOC(OLC_ACTION(d)->char_obj_found, char_obj_found);
	SQL_MALLOC(OLC_ACTION(d)->others_obj_found, others_obj_found);

	SQL_ESC(OLC_ACTION(d)->command, command);
	SQL_ESC(OLC_ACTION(d)->sort_as, sort_as);
	SQL_ESC(OLC_ACTION(d)->char_no_arg, char_no_arg);
	SQL_ESC(OLC_ACTION(d)->others_no_arg, others_no_arg);
	SQL_ESC(OLC_ACTION(d)->char_found, char_found);
	SQL_ESC(OLC_ACTION(d)->others_found, others_found);
	SQL_ESC(OLC_ACTION(d)->vict_found, vict_found);
	SQL_ESC(OLC_ACTION(d)->not_found, not_found);
	SQL_ESC(OLC_ACTION(d)->char_auto, char_auto);
	SQL_ESC(OLC_ACTION(d)->others_auto, others_auto);
	SQL_ESC(OLC_ACTION(d)->char_body_found, char_body_found);
	SQL_ESC(OLC_ACTION(d)->others_body_found, others_body_found);
	SQL_ESC(OLC_ACTION(d)->vict_body_found, vict_body_found);
	SQL_ESC(OLC_ACTION(d)->char_obj_found, char_obj_found);
	SQL_ESC(OLC_ACTION(d)->others_obj_found, others_obj_found);

	if (!(mysqlWrite(
		replace,
		TABLE_SOCIALS,
		OLC_ACTION(d)->id,
		command,
		sort_as,
		OLC_ACTION(d)->hide,
		OLC_ACTION(d)->min_char_position,
		OLC_ACTION(d)->min_victim_position,
		char_no_arg,
		others_no_arg,
		char_found,
		others_found,
		vict_found,
		not_found,
		char_auto,
		others_auto,
		char_body_found,
		others_body_found,
		vict_body_found,
		char_obj_found,
		others_obj_found
	))) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing action to database.");
		return;
	}

	SQL_FREE(command);
	SQL_FREE(sort_as);
	SQL_FREE(char_no_arg);
	SQL_FREE(others_no_arg);
	SQL_FREE(char_found);
	SQL_FREE(others_found);
	SQL_FREE(vict_found);
	SQL_FREE(not_found);
	SQL_FREE(char_auto);
	SQL_FREE(others_auto);
	SQL_FREE(char_body_found);
	SQL_FREE(others_body_found);
	SQL_FREE(vict_body_found);
	SQL_FREE(char_obj_found);
	SQL_FREE(others_obj_found);

}


/*------------------------------------------------------------------------*/

/* Menu functions */

void aedit_disp_positions(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; *position_types[i] != '\n'; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-14.14s%s", grn, i, nrm, position_types[i],
			!(++columns % 2) ? "\r\n" : "");
}

/* the main menu */
void aedit_disp_menu(struct descriptor_data * d) {
	struct social_messg *action = OLC_ACTION(d);
	struct char_data *ch        = d->character;

	get_char_colors(ch);
	
	sprintf(buf, /* "\x1B[H\x1B[J" */
		"%s-- Action editor\r\n\r\n"
		"%sN%s) Command           : %s%-15.15s%s %s1%s) Sort as Command  : %s%-15.15s%s\r\n"
		"%s2%s) Min Position [CH] : %s%-8.8s        %s3%s) Min Position [VT]: %s%-8.8s\r\n"
		"%s4%s) Show if Invisible : %s%s\r\n\r\n%s"

		"$n - name of CHAR                $N - name of VICT\r\n"
		"$e - he/she (CHAR)               $E - he/she (VICT)\r\n"
		"$m - him/her (CHAR)              $M - him/her (VICT)\r\n"
		"$s - his/her (CHAR)              $S - his/her (VICT)\r\n\r\n"
		"$p - OBJECT name                 $t - BODY part\r\n"
		"$u - UPPERCASE PREVIOUS word     $U - UPPERCASE NEXT word\r\n\r\n"

		"%sA%s) Char     [NO ARG] : %s%s\r\n"
		"%sB%s) Others   [NO ARG] : %s%s\r\n"
		"%sC%s) Char  [NOT FOUND] : %s%s\r\n"
		"%sD%s) Char   [ARG SELF] : %s%s\r\n"
		"%sE%s) Others [ARG SELF] : %s%s\r\n"
		"%sF%s) Char       [VICT] : %s%s\r\n"
		"%sG%s) Others     [VICT] : %s%s\r\n"
		"%sH%s) Victim     [VICT] : %s%s\r\n"
		"%sI%s) Char   [BODY PRT] : %s%s\r\n"
		"%sJ%s) Others [BODY PRT] : %s%s\r\n"
		"%sK%s) Victim [BODY PRT] : %s%s\r\n"
		"%sL%s) Char        [OBJ] : %s%s\r\n"
		"%sM%s) Others      [OBJ] : %s%s\r\n"
		"%sQ%s) Quit\r\n",
		nrm,
		grn, nrm,
		yel, action->command, nrm,
		grn, nrm,
		yel, action->sort_as, nrm,
		grn, nrm,
		cyn, position_types[action->min_char_position],
		grn, nrm,
		cyn, position_types[action->min_victim_position],
		grn, nrm,
		cyn, (action->hide?"HIDDEN":"NOT HIDDEN"),
		nrm,
		grn, nrm, cyn,
		action->char_no_arg ? action->char_no_arg : "<Null>",
		grn, nrm, cyn,
		action->others_no_arg ? action->others_no_arg : "<Null>",
		grn, nrm, cyn,
		action->not_found ? action->not_found : "<Null>",
		grn, nrm, cyn,
		action->char_auto ? action->char_auto : "<Null>",
		grn, nrm, cyn,
		action->others_auto ? action->others_auto : "<Null>",
		grn, nrm, cyn,
		action->char_found ? action->char_found : "<Null>",
		grn, nrm, cyn,
		action->others_found ? action->others_found : "<Null>",
		grn, nrm, cyn,
		action->vict_found ? action->vict_found : "<Null>",
		grn, nrm, cyn,
		action->char_body_found ? action->char_body_found : "<Null>",
		grn, nrm, cyn,
		action->others_body_found ? action->others_body_found : "<Null>",
		grn, nrm, cyn,
		action->vict_body_found ? action->vict_body_found : "<Null>",
		grn, nrm, cyn,
		action->char_obj_found ? action->char_obj_found : "<Null>",
		grn, nrm, cyn,
		action->others_obj_found ? action->others_obj_found : "<Null>",
		grn, nrm);

	strcat(buf,"\r\n");
	strcat(buf, "Enter choice: ");

	send_to_char(buf, d->character);
	OLC_MODE(d) = AEDIT_MAIN_MENU;
}


/*
 * The main loop
 */

void aedit_parse(struct descriptor_data * d, char *arg) {
	int i;

	switch (OLC_MODE(d)) {
		case AEDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
			case 'y': case 'Y':
				aedit_save_internally(d);
				extended_mudlog(BRF, SYSL_OLC, TRUE, "%s edits action %s", GET_NAME(d->character),
					OLC_ACTION(d)->command);
				/* do not free the strings.. just the structure */
				cleanup_olc(d, CLEANUP_STRUCTS);
				send_to_char("Action saved to memory and database.\r\n", d->character);
				break;
			case 'n': case 'N':
				/* free everything up, including strings etc */
				cleanup_olc(d, CLEANUP_ALL);
				break;
			default:
				send_to_char("Invalid choice!\r\nDo you wish to save this action? ", d->character);
				break;
			}
			return; /* end of AEDIT_CONFIRM_SAVESTRING */

		case AEDIT_CONFIRM_EDIT:
			switch (*arg)  {
			case 'y': case 'Y':
				aedit_setup_existing(d, OLC_ZNUM(d));
				break;
			case 'q': case 'Q':
				cleanup_olc(d, CLEANUP_ALL);
				break;
			case 'n': case 'N':
				OLC_ZNUM(d)++;
				for (;(OLC_ZNUM(d) <= top_of_socialt); OLC_ZNUM(d)++)
					if (is_abbrev(OLC_STORAGE(d), soc_mess_list[OLC_ZNUM(d)].command)) break;
				if (OLC_ZNUM(d) > top_of_socialt) {
					if (find_command(OLC_STORAGE(d)) > NOTHING)  {
						cleanup_olc(d, CLEANUP_ALL);
						break;
					}
					sprintf(buf, "Do you wish to add the '%s' action? ",
						OLC_STORAGE(d));
					send_to_char(buf, d->character);
					OLC_MODE(d) = AEDIT_CONFIRM_ADD;
				}
				else {
					sprintf(buf, "Do you wish to edit the '%s' action? ", soc_mess_list[OLC_ZNUM(d)].command);
					send_to_char(buf, d->character);
					OLC_MODE(d) = AEDIT_CONFIRM_EDIT;
				}
				break;
			default:
				sprintf(buf, "Invalid choice!\r\nDo you wish to edit the '%s' action? ", soc_mess_list[OLC_ZNUM(d)].command);
				send_to_char(buf, d->character);
				break;
			}
			return;

		case AEDIT_CONFIRM_ADD:
			switch (*arg)  {
			case 'y': case 'Y':
				aedit_setup_new(d);
				break;
			case 'n': case 'N': case 'q': case 'Q':
				cleanup_olc(d, CLEANUP_ALL);
				break;
			default:
				sprintf(buf, "Invalid choice!\r\nDo you wish to add the '%s' action? ", OLC_STORAGE(d));
				send_to_char(buf, d->character);
				break;
			}
			return;

		case AEDIT_MAIN_MENU:
			switch (*arg) {
			case 'q': case 'Q':
				if (OLC_VAL(d))  { /* Something was modified */
						send_to_char("Do you wish to save this action? ", d->character);
						OLC_MODE(d) = AEDIT_CONFIRM_SAVESTRING;
				}
				else cleanup_olc(d, CLEANUP_ALL);
				break;
			case 'n':
				send_to_char("Enter action name: ", d->character);
				OLC_MODE(d) = AEDIT_ACTION_NAME;
				return;
			case '1':
				send_to_char("Enter sort info for this action (for the command listing): ", d->character);
				OLC_MODE(d) = AEDIT_SORT_AS;
				return;
			case '2':
				aedit_disp_positions(d);
				send_to_char("Enter the minimum position for CHAR: ", d->character);
				OLC_MODE(d) = AEDIT_MIN_CHAR_POS;
				return;
			case '3':
				aedit_disp_positions(d);
				send_to_char("Enter the minimum position for VICT: ", d->character);
				OLC_MODE(d) = AEDIT_MIN_VICT_POS;
				return;
			case '4':
				OLC_ACTION(d)->hide = !OLC_ACTION(d)->hide;
				aedit_disp_menu(d);
				OLC_VAL(d) = 1;
				break;
			case 'a': case 'A':
				sprintf(buf, "Enter social shown to the Character when there is no argument supplied.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->char_no_arg)?OLC_ACTION(d)->char_no_arg:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_NOVICT_CHAR;
				return;
			case 'b': case 'B':
				sprintf(buf, "Enter social shown to Others when there is no argument supplied.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->others_no_arg)?OLC_ACTION(d)->others_no_arg:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_NOVICT_OTHERS;
				return;
			case 'c': case 'C':
				sprintf(buf, "Enter text shown to the Character when his victim isnt found.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->not_found)?OLC_ACTION(d)->not_found:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_VICT_NOT_FOUND;
				return;
			case 'd': case 'D':
				sprintf(buf, "Enter social shown to the Character when it is its own victim.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->char_auto)?OLC_ACTION(d)->char_auto:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_SELF_CHAR;
				return;
			case 'e': case 'E':
				sprintf(buf, "Enter social shown to Others when the Char is its own victim.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->others_auto)?OLC_ACTION(d)->others_auto:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_SELF_OTHERS;
				return;
			case 'f': case 'F':
				sprintf(buf, "Enter normal social shown to the Character when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->char_found)?OLC_ACTION(d)->char_found:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_VICT_CHAR_FOUND;
				return;
			case 'g': case 'G':
				sprintf(buf, "Enter normal social shown to Others when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->others_found)?OLC_ACTION(d)->others_found:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_VICT_OTHERS_FOUND;
				return;
			case 'h': case 'H':
				sprintf(buf, "Enter normal social shown to the Victim when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->vict_found)?OLC_ACTION(d)->vict_found:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_VICT_VICT_FOUND;
				return;
			case 'i': case 'I':
				sprintf(buf, "Enter 'body part' social shown to the Character when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->char_body_found)?OLC_ACTION(d)->char_body_found:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_VICT_CHAR_BODY_FOUND;
				return;
			case 'j': case 'J':
				sprintf(buf, "Enter 'body part' social shown to Others when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->others_body_found)?OLC_ACTION(d)->others_body_found:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_VICT_OTHERS_BODY_FOUND;
				return;
			case 'k': case 'K':
				sprintf(buf, "Enter 'body part' social shown to the Victim when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->vict_body_found)?OLC_ACTION(d)->vict_body_found:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_VICT_VICT_BODY_FOUND;
				return;
			case 'l': case 'L':
				sprintf(buf, "Enter 'object' social shown to the Character when the object is found.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->char_obj_found)?OLC_ACTION(d)->char_obj_found:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_OBJ_CHAR_FOUND;
				return;
			case 'm': case 'M':
				sprintf(buf, "Enter 'object' social shown to the Room when the object is found.\r\n[OLD]: %s\r\n[NEW]: ",
					((OLC_ACTION(d)->others_obj_found)?OLC_ACTION(d)->others_obj_found:"NULL"));
				send_to_char(buf, d->character);
				OLC_MODE(d) = AEDIT_OBJ_OTHERS_FOUND;
				return;
			default:
				aedit_disp_menu(d);
				break;
			}
			return;
	 
		case AEDIT_ACTION_NAME:
			if (*arg) {
				if (strchr(arg,' ')) {
					aedit_disp_menu(d);
					return;
				} else {
					if (OLC_ACTION(d)->command)
						free(OLC_ACTION(d)->command);
					OLC_ACTION(d)->command = str_dup(arg);
				}
			} else {
				aedit_disp_menu(d);
				return;
			}
			break;

		case AEDIT_SORT_AS:
			if (*arg) {
				if (strchr(arg,' ')) {
					aedit_disp_menu(d);
					return;
				} else  {
					if (OLC_ACTION(d)->sort_as)
						free(OLC_ACTION(d)->sort_as);
					OLC_ACTION(d)->sort_as = str_dup(arg);
				}
			} else {
				aedit_disp_menu(d);
				return;
			}
			break;

		case AEDIT_MIN_CHAR_POS:
		case AEDIT_MIN_VICT_POS:
			if (*arg)  {
				i = atoi(arg);
				if ((i < 0) && (i > POS_STANDING))  {
					aedit_disp_menu(d);
					return;
				} else {
					if (OLC_MODE(d) == AEDIT_MIN_CHAR_POS)
						OLC_ACTION(d)->min_char_position = i;
					else OLC_ACTION(d)->min_victim_position = i;
				}
			} else  {
				aedit_disp_menu(d);
				return;
			}
			break;

		case AEDIT_NOVICT_CHAR:
			if (OLC_ACTION(d)->char_no_arg)
				free(OLC_ACTION(d)->char_no_arg);
			if (*arg)	{
				delete_doubledollar(arg);
				OLC_ACTION(d)->char_no_arg = str_dup(arg);
			} else
				OLC_ACTION(d)->char_no_arg = NULL;
			break;

		case AEDIT_NOVICT_OTHERS:
			if (OLC_ACTION(d)->others_no_arg)
				free(OLC_ACTION(d)->others_no_arg);
			if (*arg)	{
				delete_doubledollar(arg);
				OLC_ACTION(d)->others_no_arg = str_dup(arg);
			} else OLC_ACTION(d)->others_no_arg = NULL;
			break;

		case AEDIT_VICT_CHAR_FOUND:
			if (OLC_ACTION(d)->char_found)
				free(OLC_ACTION(d)->char_found);
			if (*arg)	{
				delete_doubledollar(arg);
				OLC_ACTION(d)->char_found = str_dup(arg);
			} else
				OLC_ACTION(d)->char_found = NULL;
			break;

		case AEDIT_VICT_OTHERS_FOUND:
			if (OLC_ACTION(d)->others_found)
				free(OLC_ACTION(d)->others_found);
			if (*arg)	{
				delete_doubledollar(arg);
				OLC_ACTION(d)->others_found = str_dup(arg);
			}	else
				OLC_ACTION(d)->others_found = NULL;
			break;

		case AEDIT_VICT_VICT_FOUND:
			if (OLC_ACTION(d)->vict_found)
				free(OLC_ACTION(d)->vict_found);
			if (*arg)	{
				delete_doubledollar(arg);
				OLC_ACTION(d)->vict_found = str_dup(arg);
			} else
				OLC_ACTION(d)->vict_found = NULL;
			break;

		case AEDIT_VICT_NOT_FOUND:
			if (OLC_ACTION(d)->not_found)
				free(OLC_ACTION(d)->not_found);
			if (*arg) {
				delete_doubledollar(arg);
				OLC_ACTION(d)->not_found = str_dup(arg);
			} else
				OLC_ACTION(d)->not_found = NULL;
			break;

		case AEDIT_SELF_CHAR:
			if (OLC_ACTION(d)->char_auto)
				free(OLC_ACTION(d)->char_auto);
			if (*arg)	{
				delete_doubledollar(arg);
				OLC_ACTION(d)->char_auto = str_dup(arg);
			} else
				OLC_ACTION(d)->char_auto = NULL;
			break;

		case AEDIT_SELF_OTHERS:
			if (OLC_ACTION(d)->others_auto)
				free(OLC_ACTION(d)->others_auto);
			if (*arg)	{
				delete_doubledollar(arg);
				OLC_ACTION(d)->others_auto = str_dup(arg);
			} else
				OLC_ACTION(d)->others_auto = NULL;
			break;

		case AEDIT_VICT_CHAR_BODY_FOUND:
			if (OLC_ACTION(d)->char_body_found)
				free(OLC_ACTION(d)->char_body_found);
			if (*arg)	{
				delete_doubledollar(arg);
				OLC_ACTION(d)->char_body_found = str_dup(arg);
			} else
				OLC_ACTION(d)->char_body_found = NULL;
			break;

		case AEDIT_VICT_OTHERS_BODY_FOUND:
			if (OLC_ACTION(d)->others_body_found)
				free(OLC_ACTION(d)->others_body_found);
			if (*arg)	{
				delete_doubledollar(arg);
				OLC_ACTION(d)->others_body_found = str_dup(arg);
			} else
				OLC_ACTION(d)->others_body_found = NULL;
			break;

		case AEDIT_VICT_VICT_BODY_FOUND:
			if (OLC_ACTION(d)->vict_body_found)
				free(OLC_ACTION(d)->vict_body_found);
			if (*arg)	{
				delete_doubledollar(arg);
				OLC_ACTION(d)->vict_body_found = str_dup(arg);
			} else
				OLC_ACTION(d)->vict_body_found = NULL;
			break;

		case AEDIT_OBJ_CHAR_FOUND:
			if (OLC_ACTION(d)->char_obj_found)
				free(OLC_ACTION(d)->char_obj_found);
			if (*arg)	{
				delete_doubledollar(arg);
				OLC_ACTION(d)->char_obj_found = str_dup(arg);
			} else
				OLC_ACTION(d)->char_obj_found = NULL;
			break;

		case AEDIT_OBJ_OTHERS_FOUND:
			if (OLC_ACTION(d)->others_obj_found)
			free(OLC_ACTION(d)->others_obj_found);
			if (*arg)	{
			delete_doubledollar(arg);
			OLC_ACTION(d)->others_obj_found = str_dup(arg);
			}
			else OLC_ACTION(d)->others_obj_found = NULL;
			break;

		default:
			/* we should never get here */
			break;
	}
	OLC_VAL(d) = 1;
	aedit_disp_menu(d);
}
