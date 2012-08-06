/* ************************************************************************
*		File: config.c                                      Part of CircleMUD *
*	 Usage: Configuration of various aspects of CircleMUD operation         *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: config.c,v 1.38 2004/04/14 19:34:35 cheron Exp $ */

#define	__CONFIG_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "interpreter.h" /* alias_data definition for structs.h */
#include "loadrooms.h"
#include "db.h"

#define	TRUE    1
#define	YES     1
#define	FALSE   0
#define	NO      0

#include "utils.h"

/* local functions */
void load_config (void);

/* external functions */
extern int isname(const char *str, const char *namelist);

/* external variables */
extern int zone_reset;

/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

/****************************************************************************/
/****************************************************************************/

/* GAME PLAY OPTIONS */

/*
 * pk_allowed sets the tone of the entire game.  If pk_allowed is set to
 * NO, then players will not be allowed to kill, summon, charm, or sleep
 * other players, as well as a variety of other "asshole player" protections.
 * However, if you decide you want to have an all-out knock-down drag-out
 * PK Mud, just set pk_allowed to YES - and anything goes.
 */
int	magic_enabled = YES;
int	pk_allowed = NO;
int	summon_allowed = YES;
int	charm_allowed = NO;
int	sleep_allowed = NO;
int	roomaffect_allowed = NO;

/* is playerthieving allowed? */
int	pt_allowed = NO;

/* number of movement points it costs to holler */
int	holler_move_cost = 20;

/*  how many people can get into a tunnel?  The default is two, but there
 *  is also an alternate message in the case of one person being allowed.
 */
int tunnel_size = 2;

/* exp change limits */
int	max_exp_gain = 500000;	/* max gainable per kill */
int	max_exp_loss = 3000000;	/* max losable per death */

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int	max_npc_corpse_time = 5;
int	max_pc_corpse_time = 10;

/* How many ticks before a player is sent to the void or idle-rented. */
int	idle_void = 8;
int	idle_rent_time = 24;

/* should players be voided and idle-rented? */
int use_void_disconnect = YES;

/* This right and up is immune to idling, RIGHTS_IMPLEMENTOR+1 will disable it. */
bitvector_t	idle_max_rights = RIGHTS_BUILDING;

/* should items in death traps automatically be junked? */
int	dts_are_dumps = YES;

/*
 * Whether you want items that immortals load to appear on the ground or not.
 * It is most likely best to set this to 'YES' so that something else doesn't
 * grab the item before the immortal does, but that also means people will be
 * able to carry around things like boards.  That's not necessarily a bad
 * thing, but this will be left at a default of 'NO' for historic reasons.
 */
int	load_into_inventory = YES;

/* "okay" etc. */
const char *OK = "Okay.\r\n";
const char *NOPERSON = "No-one by that name here.\r\n";
const char *NOEFFECT = "Nothing seems to happen.\r\n";

/*
 * You can define or not define TRACK_THOUGH_DOORS, depending on whether
 * or not you want track to find paths which lead through closed or
 * hidden doors. A setting of 'NO' means to not go through the doors
 * while 'YES' will pass through doors to find the target.
 */
int	track_through_doors = NO;

/****************************************************************************/
/****************************************************************************/


/* RENT/CRASHSAVE OPTIONS */

/*
 * Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,
 * your objects are saved at no cost, as in Merc-type MUDs.)
 */
int	free_rent = YES;

/* maximum number of items players are allowed to rent */
int	max_obj_save = 300;

/* receptionist's surcharge on top of item costs */
int	min_rent_cost = 100;

/*
 * Should we create player corpses or let them keep everything?
 * Defaults to yes, since that is the standard CircleMUD method.
 */
int make_player_corpse = YES;

/*
 * Should the player be returned to the main menu when they die,
 * or should they simply be transported to the OOC lounge?
 */
int soft_player_death = NO;

/*
 * Should the game automatically save people?  (i.e., save player data
 * every 4 kills (on average), and Crash-save as defined below.  This
 * option has an added meaning past bpl13.  If auto_save is YES, then
 * the 'save' command will be disabled to prevent item duplication via
 * game crashes.
 */
int	auto_save = YES;

/*
 * Should the game automatically save builder data every 15 minutes?
 */
int	bautosave = NO;

/*
 * if bautosave (above) is yes, how often (in minutes) should the MUD
 * auto save builder data?
 */
int	bautosave_time = 5;

/*
 * if auto_save (above) is yes, how often (in minutes) should the MUD
 * Crash-save people's objects?   Also, this number indicates how often
 * the MUD will Crash-save players' houses.
 */
int	autosave_time = 3;

/* Lifetime of crashfiles and forced-rent (idlesave) files in days */
int	crash_file_timeout = 7;

/* Lifetime of normal rent files in days */
int	rent_file_timeout = 30;

/* Do you want to automatically wipe players who've been gone too long? */
int	auto_pwipe = NO;

/* Autowipe deletion criteria
	 This struct holds information used to determine which players to wipe
	 then the mud boots.  The rightss must be in ascending order, with a
	 descending right marking the end of the array.  A RIGHTS_NONE entry in the
	 beginning is the case for players with the PLR_DELETED flag.  The
	 values below match the stock purgeplay.c criteria.

	 Detailed explanation by array element:
	 * Element 0, RIGHTS_NONE, days 0: Players with PLR_DELETED flag are always
	wiped.
	 * Element 1, RIGHTS_MEMBER, days 60: Mortal players are wiped if they
	haven't logged on in the past 60 days.
	 * Element 2, RIGHTS_IMMORTAL, days 90: Regular Immortals are wiped if
	they haven't logged on in the past 90 days.
	 * Element 3, RIGHTS_BUILDING, days 120: Builders get 120 days.
	 * Element 4, RIGHTS_IMPLEMENTOR, days 180: Other Immortals get 180 days.
	 * Element 5: Because RIGHTS_NONE is less than RIGHTS_IMPLEMENTOR, this is assumed to
	be the end of the criteria.  The days entry is not used in this
	case.
*/
struct pclean_criteria_data pclean_criteria[] = {
/*	RIGHTS							DAYS																			*/
  {	RIGHTS_NONE					,1		}, /* players with PLR_DELETE flag	*/
  {	RIGHTS_MEMBER				,60		}, /* mortals												*/
  {	RIGHTS_IMMORTAL			,90		}, /* immortal											*/
  {	RIGHTS_BUILDING			,120	}, /* builders and immortals				*/
  {	RIGHTS_IMPLEMENTOR	,180	}, /* all gods											*/
  {	RIGHTS_NONE					,0		}  /* no more rights checks					*/
};

/* Do you want players who self-delete to be wiped immediately with no
	 backup?
*/
int	selfdelete_fastwipe = YES;

/****************************************************************************/
/****************************************************************************/


/* ROOM NUMBERS */

/* hometowns: virtual number of room that mortals should enter at */
const	sh_int mortal_start_room[NUM_STARTROOMS + 1] =  {
    -1,	/* 0. Nowhere								*/
	7501,	/* 1. Caerdydd							*/
	4201,	/* 2. Arianoch Woods				*/
	7825,	/* 3. Abertawe							*/
	4800,	/* 4. Woodward							*/
	5103,	/* 5. Cambrian							*/
	1204,	/* 6. Immortal Board Room		*/
	 300,	/* 7. Start Zone						*/
	 302	/* 8. The OOC Lounge				*/
};

/* virtual number of room that immorts should enter at by default */
room_vnum	immort_start_room = 1204;

/* virtual number of room that frozen players should enter at */
room_vnum	frozen_start_room = 1202;

/* virtual number of room that new players should enter at */
room_vnum	new_start_room = 300;

/*
 * virtual numbers of donation rooms.  note: you must change code in
 * do_drop of act.item.c if you change the number of non-NOWHERE
 * donation rooms.
 */
room_vnum	donation_room_1 = 3003;
room_vnum	donation_room_2 = NOWHERE;	/* unused - room for expansion */
room_vnum	donation_room_3 = NOWHERE;	/* unused - room for expansion */

/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

/*
 * This is the default port on which the game should run if no port is
 * given on the command-line.  NOTE WELL: If you're using the
 * 'autorun' script, the port number there will override this setting.
 * Change the PORT= line in autorun instead of (or in addition to)
 * changing this.
 */
ush_int	DFLT_PORT = 3011;

/*
 * IP address to which the MUD should bind.  This is only useful if
 * you're running Circle on a host that host more than one IP interface,
 * and you only want to bind to *one* of them instead of all of them.
 * Setting this to NULL (the default) causes Circle to bind to all
 * interfaces on the host.  Otherwise, specify a numeric IP address in
 * dotted quad format, and Circle will only bind to that IP address.  (Of
 * course, that IP address must be one of your host's interfaces, or it
 * won't work.)
 */
const	char *DFLT_IP = NULL; /* bind to all interfaces */
/* const char *DFLT_IP = "192.168.1.1";  -- bind only to one interface */

/* default directory to use as data directory */
const	char *DFLT_DIR = "lib";

/*
 * What file to log messages to (ex: "log/syslog").  Setting this to NULL
 * means you want to log to stderr, which was the default in earlier
 * versions of Circle.  If you specify a file, you don't get messages to
 * the screen. (Hint: Try 'tail -f' if you have a UNIX machine.)
 */
const	char *LOGNAME = NULL;
/* const char *LOGNAME = "log/syslog";  -- useful for Windows users */

/* maximum number of players allowed before game starts to turn people away */
int	max_playing = 300;

/* maximum size of bug, typo and idea files in bytes (to prevent bombing) */
int	max_filesize = 500000;

/* maximum number of password attempts before disconnection */
int	max_bad_pws = 3;

/*
 * Rationale for enabling this, as explained by naved@bird.taponline.com.
 *
 * Usually, when you select ban a site, it is because one or two people are
 * causing troubles while there are still many people from that site who you
 * want to still log on.  Right now if I want to add a new select ban, I need
 * to first add the ban, then SITEOK all the players from that site except for
 * the one or two who I don't want logging on.  Wouldn't it be more convenient
 * to just have to remove the SITEOK flags from those people I want to ban
 * rather than what is currently done?
 */
int	siteok_everyone = TRUE;

/*
 * Some nameservers are very slow and cause the game to lag terribly every 
 * time someone logs in.  The lag is caused by the gethostbyaddr() function
 * which is responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */
int	nameserver_is_slow = YES;

/*
 * Multilogging detects if players from the same IP number logs in several
 * times.  This can be turned on or off.
 */
int	multilogging_allowed = FALSE;

/*
 * Use email verification to activate accounts?
 * See db.c:email_verification()
 * Torgny Bjers (artovil@arcanerealms.org) 2002-10-07, 16:34:05.
 */
int use_verification = FALSE;

/*
 * Intervals for events and task queries
 * act.informative.c:check_pending_tasks()
 * Torgny Bjers (artovil@arcanerealms.org) 2002-10-07, 16:34:05.
 */
int tasks_interval = 7;
int events_interval = 30;

/****************************************************************************/

const char *SELECT_NAME =
"\r\n"
"SELECT A NAME:\r\n"
"Arcane Realms is a medieval fantasy environment, located in Wales, 8th\r\n"
"century, and therefore we must ask you to select a name that is in\r\n"
"accordance with this theme.  If you need help selecting a name we have\r\n"
"a name generator available on our webpage.  In short, no asian names,\r\n"
"no movie/anime/book names, and no far eastern names.  If your name is\r\n"
"not acceptable you will not be approved, and you have to select a new\r\n"
"name as soon as possible.  Please note that you will not be able to\r\n"
"participate in IC (In Character) activities until your name has been\r\n"
"approved.\r\n\r\n";


const char *MENU =
"\r\n"
"&c,-------------------------------.\r\n"
"&c| &CARCANE REALMS MAIN MENU       &c|\r\n"
"&c|-------------------------------|\r\n"
"&c| &W0&c) &YQuit the game.             &c|\r\n"
"&c| &W1&c) &YEnter the Realms.          &c|\r\n"
"&c| &W2&c) &YEnter background.          &c|\r\n"
"&c| &W3&c) &YRead the background story. &c|\r\n"
"&c|-------------------------------|\r\n"
"&c| &W4&c) &YTurn on/off mailinglist.   &c|\r\n"
"&c| &W5&c) &YChange email address.      &c|\r\n"
"&c|-------------------------------|\r\n"
"&c| &W6&c) &YChange password.           &c|\r\n"
"&c| &W7&c) &YDelete this character.     &c|\r\n"
"&c'-------------------------------'&n\r\n"
"\r\n"
"  Make your choice: ";


const char *WELC_MESSG =
"\r\n"
"&WWelcome to the Arcane Realms!&n\r\n"
"May your stay be pleasant and interesting.\r\n"
"\r\n\r\n";

const char *START_MESSG =
"Welcome.  This is your new MUD character!  You can now earn gold,\r\n"
"gain experience, find weapons and equipment, and much more -- while\r\n"
"meeting people from around the world!\r\n\r\n"
"Be sure to type &WCOLOR COMPLETE&n and &WPROMPT ALL&n in order\r\n"
"to see color and your health/mana/move points.\r\n\r\n"
"If you are entirely new to MUD you might want to type &WHELP&n to get\r\n"
"some commands that you can use here.\r\n\r\n"
"&mIf you are new to the Arcane Realms, type &MHELP NEWBIE&m.&n\r\n\r\n";

/****************************************************************************/

#define CONFIG_INTEGER	1
#define CONFIG_ROOM			2
#define CONFIG_BOOLEAN	3

struct config_nums_struct {
	const char *name;
	int *field;
	sh_int type;
} config_num_fields[] = {

	// Int fields
	{ "autosave_time"				,	&autosave_time				,	CONFIG_INTEGER	},
	{ "crash_file_timeout"	,	&crash_file_timeout		,	CONFIG_INTEGER	},
	{	"rent_file_timeout"		,	&rent_file_timeout		,	CONFIG_INTEGER	},
	{	"bautosave_time"			,	&bautosave_time				,	CONFIG_INTEGER	},
	{	"min_rent_cost"				,	&min_rent_cost				,	CONFIG_INTEGER	},
	{	"max_obj_save"				,	&max_obj_save					,	CONFIG_INTEGER	},
	{	"holler_move_cost"		,	&holler_move_cost			,	CONFIG_INTEGER	},
	{	"max_exp_gain"				,	&max_exp_gain					,	CONFIG_INTEGER	},
	{	"max_exp_loss"				,	&max_exp_loss					,	CONFIG_INTEGER	},
	{	"max_npc_corpse_time"	,	&max_npc_corpse_time	,	CONFIG_INTEGER	},
	{	"max_pc_corpse_time"	,	&max_pc_corpse_time		,	CONFIG_INTEGER	},
	{	"idle_void"						,	&idle_void						,	CONFIG_INTEGER	},
	{	"idle_rent_time"			,	&idle_rent_time				,	CONFIG_INTEGER	},
	{	"max_playing"					,	&max_playing					,	CONFIG_INTEGER	},
	{	"max_filesize"				,	&max_filesize					,	CONFIG_INTEGER	},
	{	"max_bad_pws"					,	&max_bad_pws					,	CONFIG_INTEGER	},
	{	"tasks_interval"			,	&tasks_interval				,	CONFIG_INTEGER	},
	{	"events_interval"			,	&events_interval			,	CONFIG_INTEGER	},

	// Boolean fields
	{	"siteok_everyone"			,	&siteok_everyone			,	CONFIG_BOOLEAN	},
	{	"pk_allowed"					,	&pk_allowed						,	CONFIG_BOOLEAN	},
	{	"summon_allowed"			,	&summon_allowed				,	CONFIG_BOOLEAN	},
	{	"charm_allowed"				,	&charm_allowed				,	CONFIG_BOOLEAN	},
	{	"sleep_allowed"				,	&sleep_allowed				,	CONFIG_BOOLEAN	},
	{	"roomaffect_allowed"	,	&roomaffect_allowed		,	CONFIG_BOOLEAN	},
	{	"pt_allowed"					,	&pt_allowed						,	CONFIG_BOOLEAN	},
	{	"dts_are_dumps"				,	&dts_are_dumps				,	CONFIG_BOOLEAN	},
	{	"load_into_inventory"	,	&load_into_inventory	,	CONFIG_BOOLEAN	},
	{	"track_through_doors"	,	&track_through_doors	,	CONFIG_BOOLEAN	},
	{	"free_rent"						,	&free_rent						,	CONFIG_BOOLEAN	},
	{	"auto_save"						,	&auto_save						,	CONFIG_BOOLEAN	},
	{	"bautosave"						,	&bautosave						,	CONFIG_BOOLEAN	},
	{	"auto_pwipe"					,	&auto_pwipe						,	CONFIG_BOOLEAN	},
	{	"selfdelete_fastwipe"	,	&selfdelete_fastwipe	,	CONFIG_BOOLEAN	},
	{	"nameserver_is_slow"	,	&nameserver_is_slow		,	CONFIG_BOOLEAN	},
	{	"zone_reset"					,	&zone_reset						,	CONFIG_BOOLEAN	},
	{	"magic_enabled"				,	&magic_enabled				,	CONFIG_BOOLEAN	},
	{	"multilogging_allowed",	&multilogging_allowed	,	CONFIG_BOOLEAN	},
	{	"use_verification"		,	&use_verification			,	CONFIG_BOOLEAN	},
	{	"make_player_corpse"	,	&make_player_corpse		,	CONFIG_BOOLEAN	},
	{	"soft_player_death"		,	&soft_player_death		,	CONFIG_BOOLEAN	},
	{ "use_void_disconnect"	, &use_void_disconnect	, CONFIG_BOOLEAN	},

	{ "\n"									, NULL									,	NOTHING					}

};

struct config_rooms_struct {
	const char *name;
	room_vnum *field;
	sh_int type;
} config_rooms_fields[] = {

	// Room vnum fields
	{	"immort_start_room"		,	&immort_start_room		,	CONFIG_ROOM			},
	{	"frozen_start_room"		,	&frozen_start_room		,	CONFIG_ROOM			},
	{	"donation_room_1"			,	&donation_room_1			,	CONFIG_ROOM			},
	{	"donation_room_2"			,	&donation_room_2			,	CONFIG_ROOM			},
	{	"donation_room_3"			,	&donation_room_3			,	CONFIG_ROOM			},

	{ "\n"									, NULL									,	NOTHING					}

};


/*
 * Load the configuration from MySQL.
 */
void load_config (void)
{
	MYSQL_RES *configs;
	MYSQL_ROW configs_row;
	int i;

	if (!(configs = mysqlGetResource(TABLE_CONFIGURATION, "SELECT * FROM %s ORDER BY id;", TABLE_CONFIGURATION))) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error loading %s.", TABLE_CONFIGURATION);
		return;
	}

	while ((configs_row = mysql_fetch_row(configs)))
	{
		for (i=0; *(config_num_fields[i].name) != '\n'; i++)
			if (isname(configs_row[1], config_num_fields[i].name))
				*config_num_fields[i].field = atoi(configs_row[2]);
		for (i=0; *(config_rooms_fields[i].name) != '\n'; i++)
			if (isname(configs_row[1], config_rooms_fields[i].name))
				*config_rooms_fields[i].field = atoi(configs_row[2]);
	}	

}


const char *display_config_vars (void)
{
	int i, row = 0;

	sprintf(buf2, "&CCurrent MUD Configuration&n\r\n");

	for (i=0; *(config_num_fields[i].name) != '\n'; i++) {
		switch (config_num_fields[i].type) {
		case CONFIG_INTEGER:
			sprintf(buf2 + strlen(buf2), "&c%21.21s: &C%-10d&n%s", config_num_fields[i].name, *config_num_fields[i].field, ((row % 2 == 1) ? "\r\n" : ""));
			row++;
			break;
		case CONFIG_BOOLEAN:
			sprintf(buf2 + strlen(buf2), "&y%21.21s: &Y%-10.10s&n%s", config_num_fields[i].name, YESNO(*config_num_fields[i].field), ((row % 2 == 1) ? "\r\n" : ""));
			row++;
			break;
		}
	}

	for (i=0; *(config_rooms_fields[i].name) != '\n'; i++)
		if (config_rooms_fields[i].type == CONFIG_ROOM) {
			sprintf(buf2 + strlen(buf2), "&g%21.21s: &G%-10d&n%s", config_rooms_fields[i].name, *config_rooms_fields[i].field, ((row % 2 == 1) ? "\r\n" : ""));
			row++;
		}

	if (row % 2 != 0)
		strcat(buf2, "\r\n");

	return (buf2);
}


#undef CONFIG_INTEGER
#undef CONFIG_ROOM
#undef CONFIG_BOOLEAN
