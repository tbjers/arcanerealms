/* ************************************************************************
*  File: comedit.c                              An extension to CircleMUD *
*  Usage: Editing command information                                     *
*                                                                         *
*  This file by Catherine Gore <cheron@arcanerealms.org>                  *
*  Modifications and cleanup by Torgny Bjers (artovil@arcanerealms.org)   *
*                                                                         *
*  Copyright (C) 2002 by Arcane Realms MUD, www.arcanerealms.org.         *
************************************************************************ */

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
#include "commands.h"

extern int    top_of_commandt;
extern struct master_command_info *complete_cmd_info;
extern struct command_info *cmd_info;
extern struct command_list_info command_list[];
extern char   *position_types[];

/* external functs */
void sort_commands(void);
void create_command_list(void);
void free_command(struct command_info *command);
void disp_bitvector_menu(struct descriptor_data *d, const char *names[], int length);
bitvector_t asciiflag_conv(char *flag);
extern const char *user_rights[];

/*
 * Utils and exported functions.
 */

void comedit_setup_new(struct descriptor_data *d) 
{
	 CREATE(OLC_COMMAND(d), struct command_info, 1);
	 OLC_COMMAND(d)->command = str_dup(OLC_STORAGE(d));
	 OLC_COMMAND(d)->sort_as = str_dup(OLC_STORAGE(d));
	 OLC_COMMAND(d)->minimum_position = POS_STANDING;
	 OLC_COMMAND(d)->command_num = 0;
	 OLC_COMMAND(d)->rights = RIGHTS_MEMBER;
	 OLC_COMMAND(d)->subcmd = 0;
	 OLC_COMMAND(d)->copyover = 0;
	 OLC_COMMAND(d)->enabled = 0;
	 OLC_COMMAND(d)->reserved = 0;
	 comedit_disp_menu(d);
	 OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void comedit_setup_existing(struct descriptor_data *d, int real_num) 
{
	 CREATE(OLC_COMMAND(d), struct command_info, 1);
	 OLC_COMMAND(d)->command = str_dup(cmd_info[real_num].command);
	 OLC_COMMAND(d)->sort_as = str_dup(cmd_info[real_num].sort_as);
	 OLC_COMMAND(d)->minimum_position = cmd_info[real_num].minimum_position;
	 OLC_COMMAND(d)->command_num  = cmd_info[real_num].command_num;
	 OLC_COMMAND(d)->rights  = cmd_info[real_num].rights;
	 OLC_COMMAND(d)->subcmd  = cmd_info[real_num].subcmd;
	 OLC_COMMAND(d)->copyover  = cmd_info[real_num].copyover;
	 OLC_COMMAND(d)->enabled = cmd_info[real_num].enabled;
	 OLC_COMMAND(d)->reserved = cmd_info[real_num].reserved;
	 OLC_VAL(d) = 0;
	 comedit_disp_menu(d);
}


void comedit_save_internally(struct descriptor_data *d) 
{
	struct command_info *new_cmd_info = NULL;
	int i;

	/* add a new command into the list */
	if (OLC_ZNUM(d) > top_of_commandt) {
		CREATE(new_cmd_info, struct command_info, top_of_commandt + 2);
		for (i = 0; i <= top_of_commandt; i++)
			new_cmd_info[i] = cmd_info[i];
		new_cmd_info[++top_of_commandt] = *OLC_COMMAND(d);
		free(cmd_info);
		cmd_info = new_cmd_info;
		create_command_list();
		sort_commands();
		comedit_mysql_save(d);
	}
	/* pass the edited command back to the list and rebuild */
	else {
		i = find_command(OLC_COMMAND(d)->command);
		comedit_mysql_save(d);
		free_command(cmd_info + OLC_ZNUM(d));
		cmd_info[OLC_ZNUM(d)] = *OLC_COMMAND(d);
		if (i > NOTHING) {
			complete_cmd_info[i].command = cmd_info[OLC_ZNUM(d)].command;
			complete_cmd_info[i].sort_as = cmd_info[OLC_ZNUM(d)].sort_as;
			complete_cmd_info[i].minimum_position = cmd_info[OLC_ZNUM(d)].minimum_position;
			complete_cmd_info[i].rights = cmd_info[OLC_ZNUM(d)].rights;
			complete_cmd_info[i].command_pointer = command_list[cmd_info[OLC_ZNUM(d)].command_num].command;
			complete_cmd_info[i].subcmd = cmd_info[OLC_ZNUM(d)].subcmd;
			complete_cmd_info[i].copyover = cmd_info[OLC_ZNUM(d)].copyover;
			complete_cmd_info[i].enabled = cmd_info[OLC_ZNUM(d)].enabled;
			complete_cmd_info[i].reserved = cmd_info[OLC_ZNUM(d)].reserved;
		} else {
			create_command_list();
			sort_commands();
		}
	}
}


/*------------------------------------------------------------------------*/

void comedit_mysql_save(struct descriptor_data *d)
{
	char *command=NULL;
	char *sort_as=NULL;

	char *replace = "REPLACE INTO %s ("
		"command, "
		"sort_as, "
		"minimum_position, "
		"command_num, "
		"rights, "
		"subcmd, "
		"copyover, "
		"enabled, "
		"reserved) "
		"VALUES ('%s', '%s', %d, %d, %llu, %d, %d, %d, %d);";

	SQL_MALLOC(OLC_COMMAND(d)->command, command);
	SQL_MALLOC(OLC_COMMAND(d)->sort_as, sort_as);

	SQL_ESC(OLC_COMMAND(d)->command, command);
	SQL_ESC(OLC_COMMAND(d)->sort_as, sort_as);

	if (!(mysqlWrite(
		replace,
		TABLE_COMMANDS,
		command,
		sort_as,
		OLC_COMMAND(d)->minimum_position,
		OLC_COMMAND(d)->command_num,
		OLC_COMMAND(d)->rights,
		OLC_COMMAND(d)->subcmd,
		OLC_COMMAND(d)->copyover,
		OLC_COMMAND(d)->enabled,
		OLC_COMMAND(d)->reserved + 1
	))) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing command to database.");
		return;
	}

	SQL_FREE(command);
	SQL_FREE(sort_as);
}


/*------------------------------------------------------------------------*/

void comedit_delete_command(struct descriptor_data *d)
{
	char *command=NULL;
	char *sort_as=NULL;
	int i, rnum;
	/* delete query for the MySQL database */
	char *remove = "DELETE FROM %s WHERE "
		"command = '%s' AND "
		"sort_as = '%s' AND "
		"command_num = %d AND "
		"subcmd = %d;";

	SQL_MALLOC(OLC_COMMAND(d)->command, command);
	SQL_MALLOC(OLC_COMMAND(d)->sort_as, sort_as);

	SQL_ESC(OLC_COMMAND(d)->command, command);
	SQL_ESC(OLC_COMMAND(d)->sort_as, sort_as);

	if (!(mysqlWrite(
		remove,
		TABLE_COMMANDS,
		command,
		sort_as,
		OLC_COMMAND(d)->command_num,
		OLC_COMMAND(d)->subcmd
	))) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error deleting command from database.");
		return;
	}

	SQL_FREE(command);
	SQL_FREE(sort_as);

	rnum = OLC_ZNUM(d);
	
	/* remove command from the cmd_info structure */
	free_command(cmd_info + rnum);
	/* move the commands down in the list */
	for (i = rnum; i < top_of_commandt; i++)
		cmd_info[i] = cmd_info[i + 1];
	
	/* recreate the MCL */
	top_of_commandt--;
	create_command_list();
	sort_commands();
}


/*------------------------------------------------------------------------*/

/* Menu functions */

void comedit_disp_positions(struct descriptor_data *d)
{
	int i, columns = 0;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; *position_types[i] != '\n'; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-14.14s%s", grn, i, nrm, position_types[i],
			!(++columns % 2) ? "\r\n" : "");

	release_buffer(buf);
}


void comedit_disp_functions(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; *command_list[i].name != '\n'; i++)
		write_to_output(d, TRUE, "%s%3d%s) %-20.20s%s", grn, i, nrm, command_list[i].name,
			!(++columns % 3) ? "\r\n" : "");
}


/* the main menu */
void comedit_disp_menu(struct descriptor_data * d)
{
	char *buf, *rbuf;

	if (!OLC_COMMAND(d))
		comedit_setup_new(d);
	
	get_char_colors(d->character);
	
	buf = get_buffer(MAX_STRING_LENGTH);
	rbuf = get_buffer(80);
	
	sprintbit(OLC_COMMAND(d)->rights, user_rights, rbuf, 80);

	write_to_output(d, TRUE,
		"-- Command Editor\r\n"
		"%s1%s) Command Name  : %s%s\r\n"
		"%s2%s) Sort as       : %s%s\r\n"
		"%s3%s) Min. Position : %s%-10s\r\n"
		"%s4%s) Command Func  : %s%s\r\n"
		"%s5%s) Rights        : %s%s\r\n"
		"%s6%s) Subcommand    : %s%d\r\n"
		"%s7%s) Copyover      : %s%s\r\n"
		"%s8%s) Enabled       : %s%s\r\n"
		"%sD%s) Delete this command\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ",
		
		grn, nrm, yel, OLC_COMMAND(d)->command,
		grn, nrm, yel, OLC_COMMAND(d)->sort_as ? OLC_COMMAND(d)->sort_as : OLC_COMMAND(d)->command,
		grn, nrm, yel, position_types[(int)OLC_COMMAND(d)->minimum_position],
		grn, nrm, yel, command_list[OLC_COMMAND(d)->command_num].name,
		grn, nrm, yel, rbuf,
		grn, nrm, yel, OLC_COMMAND(d)->subcmd,
		grn, nrm, yel, (OLC_COMMAND(d)->copyover ? "ALLOWED" : "NOT ALLOWED"),
		grn, nrm, yel, (OLC_COMMAND(d)->enabled ? "ENABLED" : "DISABLED"),
		grn, nrm,
		grn, nrm
	);

	OLC_MODE(d) = COMEDIT_MAIN_MENU;

	release_buffer(buf);
	release_buffer(rbuf);
}


/*
 * The main loop
 */
void comedit_parse(struct descriptor_data * d, char *arg) {
	int i;

	switch (OLC_MODE(d)) {
		case COMEDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
			case 'y':
			case 'Y':
				comedit_save_internally(d);
				extended_mudlog(BRF, SYSL_OLC, TRUE, "%s edits command %s", GET_NAME(d->character),
					OLC_COMMAND(d)->command);
				/* do not free the strings.. just the structure */
				cleanup_olc(d, CLEANUP_STRUCTS);
				send_to_char("Command saved to memory and database.\r\n", d->character);
				break;
			case 'n':
			case 'N':
				/* free everything up, including strings etc */
				cleanup_olc(d, CLEANUP_ALL);
				break;
			default:
				send_to_char("Invalid choice!\r\nDo you wish to save this command? ", d->character);
				break;
			}
			return; /* end of COMEDIT_CONFIRM_SAVESTRING */

		case COMEDIT_CONFIRM_DELETE:
			switch (*arg) {
			case 'y':
			case 'Y':
				comedit_delete_command(d);
				extended_mudlog(BRF, SYSL_OLC, TRUE, "%s deletes command %s", GET_NAME(d->character),
					OLC_COMMAND(d)->command);
				/* free everything up, including strings etc */
				cleanup_olc(d, CLEANUP_ALL);
				send_to_char("Command deleted.\r\n", d->character);
				break;
			case 'n':
			case 'N':
				comedit_disp_menu(d);
				break;
			default:
				send_to_char("Invalid choice!\r\nDo you wish to delete this command? ", d->character);
				break;
			}
			return;

		case COMEDIT_CONFIRM_EDIT:
			switch (*arg)  {
			case 'y':
			case 'Y':
				comedit_setup_existing(d, OLC_ZNUM(d));
				break;
			case 'q':
			case 'Q':
				cleanup_olc(d, CLEANUP_ALL);
				break;
			case 'n':
			case 'N':
				OLC_ZNUM(d)++;
				for (;(OLC_ZNUM(d) <= top_of_commandt); OLC_ZNUM(d)++)
					if (is_abbrev(OLC_STORAGE(d), cmd_info[OLC_ZNUM(d)].command)) break;
				if (OLC_ZNUM(d) > top_of_commandt) {
					if (find_social(OLC_STORAGE(d)) > NOTHING)  {
						cleanup_olc(d, CLEANUP_ALL);
						break;
					}
					sprintf(buf, "Do you wish to add the '%s' command? ",
						OLC_STORAGE(d));
					send_to_char(buf, d->character);
					OLC_MODE(d) = COMEDIT_CONFIRM_ADD;
				}
				else {
					sprintf(buf, "Do you wish to edit the '%s' command? ", cmd_info[OLC_ZNUM(d)].command);
					send_to_char(buf, d->character);
					OLC_MODE(d) = COMEDIT_CONFIRM_EDIT;
				}
				break;
			default:
				sprintf(buf, "Invalid choice!\r\nDo you wish to edit the '%s' command? ", cmd_info[OLC_ZNUM(d)].command);
				send_to_char(buf, d->character);
				break;
			}
			return;

		case COMEDIT_CONFIRM_ADD:
			switch (*arg)  {
			case 'y':
			case 'Y':
				comedit_setup_new(d);
				break;
			case 'n':
			case 'N':
			case 'q':
			case 'Q':
				cleanup_olc(d, CLEANUP_ALL);
				break;
			default:
				sprintf(buf, "Invalid choice!\r\nDo you wish to add the '%s' command? ", OLC_STORAGE(d));
				send_to_char(buf, d->character);
				break;
			}
			return;

		case COMEDIT_MAIN_MENU:
			switch (*arg) {
			case 'q':
			case 'Q':
				if (OLC_VAL(d))  { /* Something was modified */
						send_to_char("Do you wish to save this command? ", d->character);
						OLC_MODE(d) = COMEDIT_CONFIRM_SAVESTRING;
				}
				else cleanup_olc(d, CLEANUP_ALL);
				break;
			case 'd':
			case 'D':
				send_to_char("Are you sure you wish to delete this command? ", d->character);
				OLC_MODE(d) = COMEDIT_CONFIRM_DELETE;
				return;
			case '1':
				send_to_char("Enter command name: ", d->character);
				OLC_MODE(d) = COMEDIT_COMMAND_NAME;
				return;
			case '2':
				send_to_char("Enter sort-as info for this command: ", d->character);
				OLC_MODE(d) = COMEDIT_SORT_AS;
				return;
			case '3':
				comedit_disp_positions(d);
				send_to_char("\r\nEnter the minimum position for this command: ", d->character);
				OLC_MODE(d) = COMEDIT_MIN_POS;
				return;
			case '4':
				comedit_disp_functions(d);
				send_to_char("\r\nEnter the command function association: ", d->character);
				OLC_MODE(d) = COMEDIT_COMMAND_FUNC;
				return;
			case '5':
				disp_bitvector_menu(d, user_rights, NUM_USER_RIGHTS);
				send_to_char("\r\nEnter the rights for this command: ", d->character);
				OLC_MODE(d) = COMEDIT_RIGHTS;
				return;
			case '6':
				send_to_char("Please refer to interpreter.h for the list of subcmds.\r\n"
					"If you're unsure of what that means, you probably shouldn't be doing this.\r\n"
					"&WEnter the subcommand for this command (&Y0 &Wif none):&n ", d->character);
				OLC_MODE(d) = COMEDIT_SUBCMD;
				return;
			case '7':
				OLC_VAL(d) = 1;
				OLC_COMMAND(d)->copyover = !OLC_COMMAND(d)->copyover;
				sprintf(buf, "Copyover mode now %s.\r\n", OLC_COMMAND(d)->copyover ? "ON" : "OFF");
				comedit_disp_menu(d);
				break;
			case '8':
				OLC_VAL(d) = 1;
				OLC_COMMAND(d)->enabled = !OLC_COMMAND(d)->enabled;
				sprintf(buf, "Enabled mode now %s.\r\n", OLC_COMMAND(d)->copyover ? "ON" : "OFF");
				comedit_disp_menu(d);
				break;
			default:
				comedit_disp_menu(d);
				break;
			}
			return;
	 
		case COMEDIT_COMMAND_NAME:
			if (*arg) {
				if (strchr(arg,' ')) {
					comedit_disp_menu(d);
					return;
				} else {
					if (OLC_COMMAND(d)->command)
						free(OLC_COMMAND(d)->command);
					OLC_COMMAND(d)->command = str_dup(arg);
				}
			} else {
				comedit_disp_menu(d);
				return;
			}
			break;

		case COMEDIT_SORT_AS:
			if (*arg) {
				if (strchr(arg,' ')) {
					comedit_disp_menu(d);
					return;
				} else  {
					if (OLC_COMMAND(d)->sort_as)
						free(OLC_COMMAND(d)->sort_as);
					OLC_COMMAND(d)->sort_as = str_dup(arg);
				}
			} else {
				comedit_disp_menu(d);
				return;
			}
			break;

		case COMEDIT_MIN_POS:
			if (*arg)  {
				i = atoi(arg);
				if ((i < 0) && (i > POS_WATCHING))  {
					comedit_disp_menu(d);
					return;
				} else
					OLC_COMMAND(d)->minimum_position = i;
			} else  {
				comedit_disp_menu(d);
				return;
			}
			break;

		case COMEDIT_COMMAND_FUNC:
			if (*arg)	{
				int j;
				i = atoi(arg);
				for(j=0;*command_list[j].name!='\n'; j++) {
					if(!strcmp(arg,command_list[j].name)) {
						i = j++;
						break;
					}
				}
				if ((i < 0) || (i >= j)) {
					send_to_char("\r\n&RNo such command function!&n\r\nEnter the command function association: ", d->character);
					return;
				} else
					OLC_COMMAND(d)->command_num = i;
			} else {
				comedit_disp_menu(d);
				return;
			}
			break;

		case COMEDIT_RIGHTS:
			if (*arg)	{
				OLC_COMMAND(d)->rights = asciiflag_conv(arg);
			} else {
				OLC_COMMAND(d)->rights = RIGHTS_MEMBER;
				comedit_disp_menu(d);
				return;
			}
			break;

		case COMEDIT_SUBCMD:
			if (*arg)	{
				i = atoi(arg);
				if (i < 0) {
					comedit_disp_menu(d);
					return;
				} else
					OLC_COMMAND(d)->subcmd = i;
			} else {
				comedit_disp_menu(d);
				return;
			}
			break;
		default:
			cleanup_olc(d, CLEANUP_ALL);
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: comedit_parse(): Reached default case!");
			write_to_output(d, TRUE, "Oops...\r\n");
			break;
	}
	OLC_VAL(d) = 1;
	comedit_disp_menu(d);
}
