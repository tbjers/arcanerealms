/* ************************************************************************
*	File: commands.c																		  Part of CircleMUD *
*	Usage: functions related to the loading and saving of commands in				*
*	conjunction with MySQL and interpreter.c																*
*																																					*
*	All rights reserved.  See license.doc for complete information.					*
*																																					*
*	Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University	*
*	Copyright (C) 2002 by Arcane Realms MUD, created by Catherine Gore			*
*	CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.								*
************************************************************************ */

#define	__COMMANDS_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "events.h"
#include "handler.h"
#include "spells.h"
#include "skills.h"
#include "commands.h"


/* external variables */
extern int top_of_socialt;
extern struct social_messg *soc_mess_list;

/* local variables */
struct master_command_info *complete_cmd_info = NULL;
struct command_info *cmd_info = NULL;
int top_of_commandt = -1;
int num_reserved_cmds = 0;

/*
 * This is the list of all the ACMD's available in the MUD.  When you want to make a new command,
 * if it requires a new ACMD, add the ACMD to the end of this list, then in the game, use comedit
 * to add the command to the command list.  Please be sure to update the last number of the list 
 * for easy reference.  Catherine Gore (cheron@arcanerealms.org)
 *
 * No longer using NUM_ACMDS in commands.h, instead, using '\n' as with many other arrays.
 * Torgny Bjers (artovil@arcanerealms.org), 2002-11-19.
 */
struct command_list_info command_list[] = {
	{NULL, "None"},				/* 0 */
	{do_move, "do_move"}, {do_action, "do_action"}, {do_alias, "do_alias"}, {do_approve, "do_approve"}, {do_assedit, "do_assedit"}, 	/* 5 */
	{do_assemble, "do_assemble"}, {do_assist, "do_assist"}, {do_astat, "do_astat"}, {do_at, "do_at"}, {do_attributes, "do_attributes"}, 	/* 10 */
	{do_award, "do_award"}, {do_away, "do_away"}, {do_backstab, "do_backstab"}, {do_ban, "do_ban"}, {do_bash, "do_bash"}, 	/* 15 */
	{do_become, "do_become"}, {do_brew, "do_brew"}, {do_buffer, "do_buffer"}, {do_calendar, "do_calendar"}, {do_cast, "do_cast"}, 	/* 20 */
	{do_chown, "do_chown"}, {do_color, "do_color"}, {do_colorpref, "do_colorpref"}, {do_commands, "do_commands"}, {do_compare, "do_compare"},	/* 25 */
	{do_consider, "do_consider"}, {do_contact, "do_contact"}, {do_contactinfo, "do_contactinfo"}, {do_copyto, "do_copyto"}, {do_cycletip, "do_cycletip"}, 	/* 30 */
	{do_date, "do_date"}, {do_dc, "do_dc"}, {do_defend, "do_defend"}, {do_description, "do_description"}, {do_diagnose, "do_diagnose"}, 	/* 35 */
	{do_dig, "do_dig"}, {do_disarm, "do_disarm"}, {do_display, "do_display"}, {do_dodge, "do_dodge"}, {do_doing, "do_doing"}, 	/* 40 */
	{do_drink, "do_drink"}, {do_drop, "do_drop"}, {do_dwield, "do_dwield"}, {do_eat, "do_eat"}, {do_echo, "do_echo"}, 	/* 45 */
	{do_edit, "do_edit"}, {do_enter, "do_enter"}, {do_equipment, "do_equipment"}, {do_examine, "do_examine"}, {do_exits, "do_exits"}, 	/* 50 */
	{do_file, "do_file"}, {do_find, "do_find"}, {do_flee, "do_flee"}, {do_follow, "do_follow"}, {do_checkdescs, "do_checkdescs"}, 	/* 55 */
	{do_force, "do_force"}, {do_forge, "do_forge"}, {do_freevnums, "do_freevnums"}, {do_gecho, "do_gecho"}, {do_gen_comm, "do_gen_comm"}, 	/* 60 */
	{do_gen_door, "do_gen_door"}, {do_gen_ps, "do_gen_ps"}, {do_gen_write, "do_gen_write"}, {do_get, "do_get"}, {do_give, "do_give"}, 	/* 65 */
	{do_gold, "do_gold"}, {do_goto, "do_goto"}, {do_grab, "do_grab"}, {do_group, "do_group"}, {do_gsay, "do_gsay"}, 	/* 70 */
	{do_hcontrol, "do_hcontrol"}, {do_help, "do_help"}, {do_helpcheck, "do_helpcheck"}, {do_hit, "do_hit"}, {do_house, "do_house"}, 	/* 75 */
	{do_insult, "do_insult"}, {do_inventory, "do_inventory"}, {do_invis, "do_invis"}, {do_ispell, "do_ispell"}, {do_kick, "do_kick"}, 	/* 80 */
	{do_kill, "do_kill"}, {do_languages, "do_languages"}, {do_last, "do_last"}, {do_leave, "do_leave"}, {do_lines, "do_lines"}, 	/* 85 */
	{do_listskills, "do_listskills"}, {do_load, "do_load"}, {do_look, "do_look"}, {do_meditate, "do_meditate"}, {do_newbie, "do_newbie"}, 	 /* 90 */
	{do_not_here, "do_not_here"}, {do_oasis, "do_oasis"}, {do_order, "do_order"}, {do_overflow, "do_overflow"}, {do_page, "do_page"}, 	/* 95 */
	{do_pemote, "do_pemote"}, {do_pending, "do_pending"}, {do_pour, "do_pour"}, {do_practice, "do_practice"}, {do_purge, "do_purge"}, 	/* 100 */
	{do_put, "do_put"}, {do_qcomm, "do_qcomm"}, {do_qlist, "do_qlist"}, {do_qstat, "do_qstat"}, {do_quit, "do_quit"}, 	/* 105 */
	{do_recall, "do_recall"}, {do_reload, "do_reload"}, {do_remove, "do_remove"}, {do_rename, "do_rename"}, {do_reply, "do_reply"}, 	/* 110 */
	{do_replylock, "do_replylock"}, {do_report, "do_report"}, {do_rescue, "do_rescue"}, {do_rest, "do_rest"}, {do_restore, "do_restore"}, 	/* 115 */
	{do_retreat, "do_retreat"}, {do_return, "do_return"}, {do_rolldie, "do_rolldie"}, {do_rplog, "do_rplog"}, {do_sac, "do_sac"}, 	/* 120 */
	{do_save, "do_save"}, {do_say, "do_say"}, {do_score, "do_score"}, {do_scribe, "do_scribe"}, {do_send, "do_send"}, 	/* 125 */
	{do_set, "do_set"}, {do_show, "do_show"}, {do_shutdown, "do_shutdown"}, {do_sit, "do_sit"}, {do_skill_state, "do_skill_state"}, 	/* 130 */
	{do_skills, "do_skills"}, {do_skillset, "do_skillset"}, {do_sleep, "do_sleep"}, {do_snoop, "do_snoop"}, {do_spec_comm, "do_spec_comm"}, 	/* 135 */
	{do_split, "do_split"}, {do_stand, "do_stand"}, {do_stat, "do_stat"}, {do_rescopy, "rescopy"}, {do_switch, "do_switch"}, 	/* 140 */
	{do_syslog, "do_syslog"}, {do_teleport, "do_teleport"}, {do_tell, "do_tell"}, {do_tells, "do_tells"}, {do_time, "do_time"}, 	/* 145 */
	{do_title, "do_title"}, {do_toggle, "do_toggle"}, {do_track, "do_track"}, {do_trans, "do_trans"}, {do_travel, "do_travel"}, 	/* 150 */
	{do_unban, "do_unban"}, {do_undig, "do_undig"}, {do_ungroup, "do_ungroup"}, {do_use, "do_use"}, {do_users, "do_users"},	/* 155 */
	{do_wake, "do_wake"}, {do_watch, "do_watch"}, {do_wear, "do_wear"}, {do_weather, "do_weather"}, {do_where, "do_where"},	/* 160 */
	{do_who, "do_who"}, {do_whois, "do_whois"}, {do_wield, "do_wield"}, {do_wimpy, "do_wimpy"}, {do_wipeplayer, "do_wipeplayer"},	/* 165 */
	{do_visible, "do_visible"}, {do_wizlock, "do_wizlock"}, {do_wiznet, "do_wiznet"}, {do_wizutil, "do_wizutil"}, {do_vnum, "do_vnum"},	/* 170 */
	{do_world, "do_world"}, {do_tester, "!UNUSED!"}, {do_write, "do_write"}, {do_vstat, "do_vstat"}, {do_xname, "do_xname"},	/* 175 */
	{do_xsyslog, "do_xsyslog"}, {do_zdelete, "do_zdelete"}, {do_zreset, "do_zreset"}, {do_attach, "do_attach"}, {do_detach, "do_detach"},	/* 180 */
	{do_tlist, "do_tlist"}, {do_tstat, "do_tstat"}, {do_masound, "do_masound"}, {do_mkill, "do_mkill"}, {do_mjunk, "do_mjunk"},	/* 185 */
	{do_mdoor, "do_mdoor"}, {do_msend, "do_msend"}, {do_mecho, "do_mecho"}, {do_mload, "do_mload"}, {do_mechoaround, "do_mechoaround"},	/* 190 */
	{do_mpurge, "do_mpurge"}, {do_mgoto, "do_mgoto"}, {do_mat, "do_mat"}, {do_mteleport, "do_mteleport"}, {do_mforce, "do_mforce"},	/* 195 */
	{do_mdamage, "do_mdamage"}, {do_mzoneecho, "do_mzoneecho"}, {do_mhunt, "do_mhunt"}, {do_mremember, "do_mremember"}, {do_mforget, "do_mforget"},	/* 200 */
	{do_mtransform, "do_mtransform"}, {do_mreward, "do_mreward"}, {do_mpet, "do_mpet"}, {do_vdelete, "do_vdelete"}, {do_saveall, "do_saveall"},	/* 205 */
	{do_zcheck, "do_zcheck"}, {do_copyover, "do_copyover"}, {do_purgemob, "do_purgemob"}, {do_purgeobj, "do_purgeobj"}, {do_checkloadstatus, "do_checkloadstatus"},	/* 210 */
	{do_oload, "do_oload"}, {do_slay, "do_slay"}, {do_wlist, "do_wlist"}, {do_zone_report, "do_zone_report"}, {do_mdump, "do_mdump"},	/* 215 */
	{do_odump, "do_odump"}, {do_specset, "do_specset"}, {do_speclist, "do_speclist"}, {do_liblist, "do_liblist"}, {do_addguild, "do_addguild"}, 	/* 220 */
	{do_remguild, "do_remguild"}, {do_allguilds, "do_allguilds"}, {do_clubs, "do_clubs"}, {do_myguilds, "do_myguilds"}, {do_granks, "do_granks"}, 	/* 225 */
	{do_guildinfo, "do_guildinfo"}, {do_guildset, "do_guildset"}, {do_guildieset, "do_guildieset"}, {do_gadmininfo, "do_gadmininfo"}, {do_guildieinfo, "do_guildieinfo"}, 	/* 230 */
	{do_guild, "do_guild"}, {do_deguild, "do_deguild"}, {do_seekguild, "do_seekguild"}, {do_seeking, "do_seeking"}, {do_sponsor, "do_sponsor"}, 	/* 235 */
	{do_unsponsor, "do_unsponsor"}, {do_glist, "do_glist"}, {do_gdesc, "do_gdesc"}, {do_greq, "do_greq"}, {do_ghelp, "do_ghelp"}, 	/* 240 */
	{do_saveguild, "do_saveguild"}, {do_gdeposit, "do_gdeposit"}, {do_gwithdraw, "do_gwithdraw"}, {do_gskillset, "do_gskillset"}, {do_gload, "do_gload"}, 	/* 245 */
	{do_guildwalk, "do_guildwalk"}, {do_gtoggle, "do_gtoggle"}, {do_tester, "do_tester"}, {do_test_parse, "do_test_parse"}, {do_tedit, "do_tedit"},	/* 250 */
	{do_enable, "do_enable"}, {do_assminfo, "do_assminfo"}, {do_rpconvert, "do_rpconvert"}, {do_chase, "do_chase"},	{do_recognize, "do_recognize"}, /* 255 */
	{do_qptransfer, "do_qptransfer"}, {do_map, "do_map"}, {do_notifylist, "do_notifylist"}, /* 258 */
	{NULL, "\n"} /* This should always come last! */
};


/* this function adds in the loaded socials and assigns them a command # */
void create_command_list(void)
{
	int i, j, k;
	struct social_messg temp;

	/* free up old command list */
	if (complete_cmd_info) {
		free(complete_cmd_info);
		complete_cmd_info = NULL;
	}

	/* recheck the sort on the socials */
	for (j = 0; j < top_of_socialt; j++) {
		k = j;
		for (i = j + 1; i <= top_of_socialt; i++)
			if (str_cmp(soc_mess_list[i].sort_as, soc_mess_list[k].sort_as) < 0)
				k = i;
		if (j != k) {
			temp = soc_mess_list[j];
			soc_mess_list[j] = soc_mess_list[k];
			soc_mess_list[k] = temp;
		}
	}

	CREATE(complete_cmd_info, struct master_command_info, top_of_socialt + top_of_commandt + 3);

	/* this loop sorts the socials and commands together into one big list */
	i = 0;
	j = 0;
	k = 0;
	while ((i <= top_of_commandt) && (j <= top_of_socialt))  {
		if ((i < num_reserved_cmds) || (j > top_of_socialt) || 
	(str_cmp(cmd_info[i].sort_as, soc_mess_list[j].sort_as) < 1)) {
			complete_cmd_info[k].command					= cmd_info[i].command;
			complete_cmd_info[k].sort_as					= cmd_info[i].sort_as;
			complete_cmd_info[k].minimum_position = cmd_info[i].minimum_position;
			complete_cmd_info[k].command_pointer	= command_list[cmd_info[i].command_num].command;
			complete_cmd_info[k].rights						= cmd_info[i].rights;
			complete_cmd_info[k].subcmd						= cmd_info[i].subcmd;
			complete_cmd_info[k].copyover					= cmd_info[i].copyover;
			complete_cmd_info[k].enabled					= cmd_info[i].enabled;
			complete_cmd_info[k++].reserved				= cmd_info[i++].reserved;
		} else {
			soc_mess_list[j].act_nr		= k;
			complete_cmd_info[k].command					= soc_mess_list[j].command;
			complete_cmd_info[k].sort_as					= soc_mess_list[j].sort_as;
			complete_cmd_info[k].minimum_position	= soc_mess_list[j++].min_char_position;
			complete_cmd_info[k].command_pointer	= do_action;
			complete_cmd_info[k].rights						= RIGHTS_MEMBER;
			complete_cmd_info[k].subcmd						= 0;
			complete_cmd_info[k].copyover					= 1;
			complete_cmd_info[k].enabled					= 1;
			complete_cmd_info[k++].reserved				= 0;
		}
	}
	complete_cmd_info[k].command							= str_dup("\n");
	complete_cmd_info[k].sort_as							= str_dup("zzzzzzz");
	complete_cmd_info[k].minimum_position			= 0;
	complete_cmd_info[k].command_pointer			= 0;
	complete_cmd_info[k].rights								= RIGHTS_MEMBER;
	complete_cmd_info[k].subcmd								= 0;
	complete_cmd_info[k].copyover							= 1;
	complete_cmd_info[k].enabled							= 1;
	complete_cmd_info[k].reserved							= 0;
	mlog("Command info rebuilt, %d total commands.", k);
}


/* Loads commands from MySQL */
void boot_command_list(void)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int curr_com = -1;
	unsigned long *fieldlength;

	/* Here we are subtracting 1 from reserved since ENUM ranges from 1..x.. */
	if (!(result = mysqlGetResource(TABLE_COMMANDS, "SELECT command, sort_as, minimum_position, command_num, rights, subcmd, copyover, enabled, (reserved - 1) FROM %s ORDER BY reserved DESC, sort_as ASC;", TABLE_COMMANDS))) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Could not load command table.");
		extended_mudlog(NRM, SYSL_SQL, TRUE, "Query error (%s)%s:%d: %s", __FILE__, __FUNCTION__, __LINE__, mysql_error(&mysql));
		kill_mysql();
		exit(1);
	}

	/* count socials and allocate space */
	top_of_commandt = mysql_num_rows(result);
	mlog("   %d commands loaded.", top_of_commandt);

	CREATE(cmd_info, struct command_info, top_of_commandt + 2);

	/* The first command in the list (0) should be reserved. */
	cmd_info[0].command = str_dup("RESERVED");
	cmd_info[0].sort_as = str_dup("RESERVED");
	cmd_info[0].minimum_position = 0;
	cmd_info[0].command_num = 0;
	cmd_info[0].rights = RIGHTS_NONE;
	cmd_info[0].subcmd = 0;
	cmd_info[0].copyover = 0;
	cmd_info[0].enabled = 0;
	cmd_info[0].reserved = 1;
	num_reserved_cmds++;

	for(curr_com = 1; curr_com <= top_of_commandt; curr_com++)
	{
		row = mysql_fetch_row(result);
		mlog("   Command: %s, Enabled: %d, Reserved: %d", row[0], atoi(row[7]), atoi(row[8]));
		fieldlength = mysql_fetch_lengths(result);
		cmd_info[curr_com].command = str_dup(row[0]);
		cmd_info[curr_com].sort_as = str_dup(row[1]);
		cmd_info[curr_com].minimum_position = atoi(row[2]);
		cmd_info[curr_com].command_num = atoi(row[3]);
		cmd_info[curr_com].rights = atoi(row[4]);
		cmd_info[curr_com].subcmd = atoi(row[5]);
		cmd_info[curr_com].copyover = atoi(row[6]);
		cmd_info[curr_com].enabled = atoi(row[7]);
		cmd_info[curr_com].reserved = atoi(row[8]);
		if (cmd_info[curr_com].reserved)
			num_reserved_cmds++;
	}

	top_of_commandt = curr_com - 1;

	mysql_free_result(result);
}


/*
 * Yes, this is going to take a little bit of time for the commands to save.
 * this is only done when the game is terminated, though, so it won't affect
 * gameplay.  Catherine Gore (cheron@arcanerealms.org)
 *
 * Doesn't really take any time at all, nevertheless, it should only be called
 * upon shutdown.  Torgny Bjers (artovil@arcanerealms.org), 2002-11-19
 */
void save_commands(void)
{
	char *command=NULL;
	char *sort_as=NULL;
	int i, j;
	int num_cmds = 0;

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

	for (i = 0; *complete_cmd_info[i].command != '\n'; i++) {

		if (complete_cmd_info[i].command_pointer == do_action)
			continue;
		
		SQL_MALLOC(complete_cmd_info[i].command, command);
		SQL_MALLOC(complete_cmd_info[i].sort_as, sort_as);
		
		SQL_ESC(complete_cmd_info[i].command, command);
		SQL_ESC(complete_cmd_info[i].sort_as, sort_as);
		
		for (j = 0; *command_list[j].name != '\n'; j++) {
			if (complete_cmd_info[i].command_pointer == command_list[j].command)
				break;
		}

		if (!(mysqlWrite(
			replace,
			TABLE_COMMANDS,
			command,
			sort_as,
			complete_cmd_info[i].minimum_position,
			j,
			complete_cmd_info[i].rights,
			complete_cmd_info[i].subcmd,
			complete_cmd_info[i].copyover,
			complete_cmd_info[i].enabled,
			complete_cmd_info[i].reserved + 1 // ENUM ranges from 1..x..
		))) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing command to database.");
			return;
		}

		num_cmds++;

		SQL_FREE(command);
		SQL_FREE(sort_as);
		command = NULL;
		sort_as = NULL;
	}

	mlog("   %d commands saved.", num_cmds);

}


void free_command_list(void)
{
	int cmd;
	struct command_info *comm;

	for (cmd = 0; cmd <= top_of_commandt; cmd++) {
		comm = &cmd_info[cmd];

		if (comm->command) free(comm->command);
		if (comm->sort_as) free(comm->sort_as);
	}
	free(cmd_info);
}


void free_command(struct command_info *mess)
{
	if (mess->command) free(mess->command);
	if (mess->sort_as) free(mess->sort_as);
	memset(mess, 0, sizeof(struct command_info));
}


void mysql_save_command(struct command_info *cmd)
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

	SQL_MALLOC(cmd->command, command);
	SQL_MALLOC(cmd->sort_as, sort_as);

	SQL_ESC(cmd->command, command);
	SQL_ESC(cmd->sort_as, sort_as);

	if (!(mysqlWrite(
		replace,
		TABLE_COMMANDS,
		command,
		sort_as,
		cmd->minimum_position,
		cmd->command_num,
		cmd->rights,
		cmd->subcmd,
		cmd->copyover,
		cmd->enabled,
		cmd->reserved + 1 // ENUM ranges from 1..x..
	))) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing command to database.");
		return;
	}

	SQL_FREE(command);
	SQL_FREE(sort_as);
}


ACMD(do_enable)
{
	struct command_info *command;
	int i, j;

	skip_spaces(&argument);

	if (!*argument) {
		send_to_char("Usage: enable/disable command\r\n", ch);
		return;
	}
	
	i = find_command(argument);

	if (i == NOTHING) {
		send_to_char("That command does not exist.\r\n", ch);
		return;
	}

	for (j = 0; (j <= top_of_commandt); j++)  {
		if (is_abbrev(argument, cmd_info[j].command))
			break;
	}

	command = &cmd_info[j];

	if (cmd_info[j].reserved) {
		send_to_char("You cannot alter a reserved command.\r\n", ch);
		return;
	}

	switch (subcmd) {
		case SCMD_ENABLE:
			if (cmd_info[j].enabled) {
				send_to_char("This command is already enabled.\r\n", ch);
				return;
			}
			complete_cmd_info[i].enabled = 1;
			cmd_info[j].enabled = 1;
			send_to_char("Command enabled.\r\n", ch);
			extended_mudlog(NRM, SYSL_GENERAL, TRUE, "%s has enabled command %s.", GET_NAME(ch), cmd_info[j].command);
			break;
		case SCMD_DISABLE:
			if (!(cmd_info[j].enabled)) {
				send_to_char("This command is already disabled.\r\n", ch);
				return;
			}
			complete_cmd_info[i].enabled = 0;
			cmd_info[j].enabled = 0;
			send_to_char("Command disabled.\r\n", ch);
			extended_mudlog(NRM, SYSL_GENERAL, TRUE, "%s has disabled command %s.", GET_NAME(ch), cmd_info[j].command);
			break;
		default:
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "ACMD(do_enable): Reached default case!");
			break;
	}
	
	mysql_save_command(command);

}
