/* ************************************************************************
*		File: interpreter.h                                 Part of CircleMUD *
*	 Usage: header file: public procs, macro defs, subcommand defines       *
*                                                                         *
*	 All rights reserved.	See license.doc for complete information.         *
*                                                                         *
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: interpreter.h,v 1.48 2004/04/20 16:30:13 cheron Exp $ */

#define	ACMD(name)	\
	 void name(struct char_data *ch, char *argument, int cmd, int subcmd)

ACMD(do_move);

#define	CMD_NAME (complete_cmd_info[cmd].command)
#define	CMD_IS(cmd_name) (!strcmp(cmd_name, complete_cmd_info[cmd].command))
#define	IS_MOVE(cmdnum) (complete_cmd_info[cmdnum].command_pointer == do_move)

void	command_interpreter(struct char_data *ch, char *argument);
int	search_block(char *arg, const char **list, int exact);
char	lower( char c );
char	*one_argument(char *argument, char *first_arg);
char	*one_word(char *argument, char *first_arg);
char	*any_one_arg(char *argument, char *first_arg);
char	*two_arguments(char *argument, char *first_arg, char *second_arg);
char *one_arg_case_sen(char *argument, char *first_arg);
int	fill_word(char *argument);
void	half_chop(char *string, char *arg1, char *arg2);
void	nanny(struct descriptor_data *d, char *arg);
int	is_abbrev(const char *arg1, const char *arg2);
int	is_number(const char *str);
int	find_command(const char *command);
int find_social(const char *social);
void	skip_spaces(char **string);
char	*delete_doubledollar(char *string);

/* for compatibility with 2.20: */
#define	argument_interpreter(a, b, c) two_arguments(a, b, c)

/*
 * Necessary for CMD_IS macro.	Borland needs the structure defined first
 * so it has been moved down here.
 */
#ifndef	__INTERPRETER_C__
extern struct master_command_info *complete_cmd_info;
#endif

/*
 * Alert! Changed from 'struct alias' to 'struct alias_data' in bpl15
 * because a Windows 95 compiler gives a warning about it having similiar
 * named member.
 */
struct alias_data {
	char *alias;
	char *replacement;
	int type;
	struct alias_data *next;
};

#define	ALIAS_SIMPLE					0
#define	ALIAS_COMPLEX					1

#define	ALIAS_SEP_CHAR				';'
#define	ALIAS_VAR_CHAR				'$'
#define	ALIAS_GLOB_CHAR				'*'

/*
 * SUBCOMMANDS
 *	 You can define these however you want to, and the definitions of the
 *	 subcommands are independent from function to function.
 */

/* directions */
#define	SCMD_NORTH						1
#define	SCMD_EAST							2
#define	SCMD_SOUTH						3
#define	SCMD_WEST							4
#define	SCMD_UP								5
#define	SCMD_DOWN							6
#define	SCMD_NORTHEAST				7
#define	SCMD_SOUTHWEST				8
#define	SCMD_NORTHWEST				9
#define	SCMD_SOUTHEAST				10
#define	SCMD_IN								11
#define	SCMD_OUT							12

/* do_gen_ps */
#define	SCMD_INFO							0
#define	SCMD_HANDBOOK					1 
#define	SCMD_CREDITS					2
#define	SCMD_NEWS							3
#define	SCMD_WIZLIST					4
#define	SCMD_POLICIES					5
#define	SCMD_VERSION					6
#define	SCMD_IMMLIST					7
#define	SCMD_MOTD							8
#define	SCMD_IMOTD						9
#define	SCMD_CLEAR						10
#define	SCMD_WHOAMI						11
#define	SCMD_MEETING					12
#define	SCMD_CHANGES					13
#define	SCMD_BUILDERCREDS			14
#define	SCMD_GUIDELINES				15
#define SCMD_ROLEPLAY					16
#define SCMD_SNOOPLIST				17
#define SCMD_NAMEPOLICY				18
#define SCMD_IDEAS				19
#define SCMD_BUGS				20
#define SCMD_TYPOS				21

/* do_toggle */
#define	SCMD_NOSUMMON					0
#define	SCMD_NOHASSLE					1
#define	SCMD_BRIEF						2
#define	SCMD_COMPACT					3
#define	SCMD_NOTELL						4
#define	SCMD_DEAF							5
#define	SCMD_NOSOC						6
#define	SCMD_NOWIZ						7
#define	SCMD_QUEST						8
#define	SCMD_ROOMFLAGS				9
#define	SCMD_NOREPEAT					10
#define	SCMD_HOLYLIGHT				11
#define	SCMD_SLOWNS						12
#define	SCMD_AUTOEXIT					13
#define	SCMD_TRACK						14
#define	SCMD_AUTOASSIST				15
#define	SCMD_AUTOSAC					16
#define	SCMD_AUTOSPLIT				17
#define	SCMD_AUTOLOOT					18
#define	SCMD_AUTOGOLD					19
#define	SCMD_METERBAR					20
#define	SCMD_NONEWBIE					21
#define SCMD_IC								22
#define SCMD_NOTIPCHAN				23
#define SCMD_SHOWVNUMS				24
#define SCMD_EMAIL						25
#define SCMD_NOSING						26
#define SCMD_NOOBSCENE				27
#define SCMD_SKILLGAINS				28
#define SCMD_TIMESTAMPS				29
#define SCMD_REALSKILLS				30
#define SCMD_POSEID						31

/* do_wizutil */
#define	SCMD_REROLL						0
#define	SCMD_PARDON						1
#define	SCMD_NOTITLE					2
#define	SCMD_SQUELCH					3
#define	SCMD_FREEZE						4
#define	SCMD_THAW							5
#define	SCMD_UNAFFECT					6
#define SCMD_STRIPRIGHTS			7

/* do_spec_comm */
#define	SCMD_COMM_WHISPER			0
#define	SCMD_COMM_ASK					1

/* do_gen_comm */
#define	SCMD_COMM_HOLLER			0
#define	SCMD_COMM_SHOUT				1
#define	SCMD_COMM_SOC					2
#define	SCMD_COMM_NEWBIE			3
#define SCMD_COMM_SING				4
#define SCMD_COMM_OBSCENE			5

/* do_shutdown */
#define	SCMD_SHUTDOW					0
#define	SCMD_SHUTDOWN					1

/* do_quit */
#define	SCMD_QUI							0
#define	SCMD_QUIT							1

/* do_date */
#define	SCMD_DATE						 0
#define	SCMD_UPTIME					 1

/* do_commands */
#define	SCMD_COMMANDS					0
#define	SCMD_SOCIALS					1
#define	SCMD_WIZHELP					2

/* do_drop */
#define	SCMD_DROP							0
#define	SCMD_JUNK							1
#define	SCMD_DONATE						2

/* do_gen_write */
#define	SCMD_BUG							0
#define	SCMD_TYPO							1
#define	SCMD_IDEA							2

/* do_look */
#define	SCMD_LOOK							0
#define	SCMD_READ							1

/* do_qcomm */
#define	SCMD_QSAY							0
#define	SCMD_QECHO						1

/* do_pour */
#define	SCMD_POUR							0
#define	SCMD_FILL							1

/* do_eat */
#define	SCMD_EAT							0
#define	SCMD_TASTE						1
#define	SCMD_DRINK						2
#define	SCMD_SIP							3

/* do_use */
#define	SCMD_USE							0
#define	SCMD_QUAFF						1
#define	SCMD_RECITE						2
#define	SCMD_STUDY						3

/* do_echo */
#define	SCMD_ECHO							0
#define	SCMD_EMOTE						1
#define SCMD_POSE							2
#define SCMD_OEMOTE						3
#define SCMD_OPOSE						4

/* do_gen_door */
#define	SCMD_OPEN							0
#define	SCMD_CLOSE						1
#define	SCMD_UNLOCK						2
#define	SCMD_LOCK							3
#define	SCMD_PICK							4
#define SCMD_KNOCK						5
#define SCMD_SAYTHROUGH				6

/* do_oasis */
#define	SCMD_OASIS_REDIT			0
#define	SCMD_OASIS_OEDIT			1
#define	SCMD_OASIS_ZEDIT			2
#define	SCMD_OASIS_MEDIT			3
#define	SCMD_OASIS_SEDIT			4
#define	SCMD_OASIS_AEDIT			5
#define	SCMD_OASIS_TRIGEDIT		6
#define	SCMD_OASIS_EMAIL			7
#define	SCMD_OASIS_SPEDIT			8
#define	SCMD_OASIS_QEDIT			9
#define	SCMD_OASIS_GEDIT			10
#define SCMD_OASIS_COMEDIT		11
#define	SCMD_OASIS_TUTOREDIT	12
#define SCMD_OASIS_HEDIT			13
#define	SCMD_OLC_SAVEINFO			14

/* * do_assemble * These constants *must* corespond with
		 the ASSM_xxx constants in *assemblies.h. */
#define	SCMD_ASM_ASSEMBLE			0
#define	SCMD_ASM_BAKE					1
#define	SCMD_ASM_BREW					2
#define	SCMD_ASM_CRAFT				3
#define	SCMD_ASM_FLETCH				4
#define	SCMD_ASM_KNIT					5
#define	SCMD_ASM_MAKE					6
#define	SCMD_ASM_MIX					7
#define	SCMD_ASM_THATCH				8
#define	SCMD_ASM_WEAVE				9
#define	SCMD_ASM_COOK					10

	/* do_liblist */
#define	SCMD_OLIST						0
#define	SCMD_MLIST						1
#define	SCMD_RLIST						2
#define	SCMD_ZLIST						3
#define SCMD_SPLIST						4
#define SCMD_TUTORLIST				5

/* do_say */
#define	SCMD_SAY							0
#define	SCMD_COMMON						1
#define	SCMD_ELVEN						2
#define	SCMD_GNOMISH					3
#define	SCMD_DWARVEN					4
#define	SCMD_ANCIENT					5
#define	SCMD_THIEF						6
#define SCMD_SIGNLANG					7
#define SCMD_OSAY							8	
#define SCMD_SAYTO						9 /* always comes last */

/* do_world */
#define	SCMD_PK								0
#define	SCMD_PKSLEEP					1
#define	SCMD_PKSUMMON					2
#define	SCMD_PKCHARM					3
#define	SCMD_PKROOMAFFECTS		4
#define	SCMD_PKMAIN						5

/* do_zdelete */
#define	SCMD_ZDELETE					1

/* do_forage */
#define	SCMD_FORAGE_FISHING			0
#define	SCMD_FORAGE_HUNTING			1
#define	SCMD_FORAGE_GATHER			2
#define	SCMD_FORAGE_LUMBERJACK	3
#define	SCMD_FORAGE_MINING			4

/* do_approve */
#define SCMD_UNAPPROVE				0
#define SCMD_APPROVE					1

/* void check_pending_tasks and do_tasks */
#define SCMD_TASKS				0
#define SCMD_EVENTS				1

/* do_descedit */
#define SCMD_DESCEDIT			0
#define SCMD_DESCSWITCH		1
#define SCMD_SDESCEDIT		2
#define SCMD_LDESCEDIT		3

/* do_away */
#define SCMD_AWAY_AFK			0
#define SCMD_AWAY_AFW			1

/* do_enable */
#define SCMD_ENABLE				0
#define SCMD_DISABLE			1

/* do_put */
#define SCMD_PUT					0
#define SCMD_SHEATHE			1

/* do_get */
#define SCMD_GET					0
#define SCMD_DRAW					1

/* email_verification */
#define EMAIL_VERIFY			0
#define EMAIL_CHECK				1
