/* ************************************************************************
*		File: db.h                                          Part of CircleMUD *
*	 Usage: header file for database handling                               *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001-2002, Torgny Bjers.                                  *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
************************************************************************ */
/* $Id: db.h,v 1.61 2004/04/22 16:42:10 cheron Exp $ */

/* Link to MySQL library headers */
#if defined(_mysql_h)

#define	TABLE_CONFIGURATION					"configuration"

#define	TABLE_BOOT_INDEX						"boot_index"

/* Zone tables */
#define	TABLE_ZON_INDEX							"zon_index"
#define	TABLE_ZON_COMMANDS					"zon_commands"

/* World tables */
#define	TABLE_WLD_INDEX							"wld_index"
#define	TABLE_WLD_EXTRADESCS				"wld_extradescs"
#define	TABLE_WLD_EXITS							"wld_exits"

/* Mobile tables */
#define	TABLE_MOB_INDEX							"mob_index"
#define	TABLE_MOB_EXTRADESCS				"mob_extradescs"

/* Object tables */
#define	TABLE_OBJ_INDEX							"obj_index"
#define	TABLE_OBJ_EXTRADESCS				"obj_extradescs"
#define	TABLE_OBJ_AFFECTS					  "obj_affects"

/* Trigger tables */
#define	TABLE_TRG_INDEX							"trg_index"
#define	TABLE_TRG_ASSIGNS						"trg_assigns"

/* Shop tables */
#define	TABLE_SHP_INDEX							"shp_index"
#define	TABLE_SHP_PRODUCTS					"shp_products"
#define	TABLE_SHP_KEYWORDS					"shp_keywords"
#define	TABLE_SHP_ROOMS							"shp_rooms"

/* Quest tables */
#define	TABLE_QST_INDEX							"qst_index"

/* Help tables */
#define	TABLE_HLP_CATEGORIES				"help_categories"
#define	TABLE_HLP_INDEX							"help_index"

/* Text, socials, misc, etc related tables */
#define	TABLE_TEXT									"lib_text"
#define	TABLE_SOCIALS								"socials"
#define	TABLE_TIP_INDEX							"tip_messages"
#define	TABLE_PUFF_MESSAGES					"puff_messages"
#define TABLE_COMMANDS							"MCL"

/* Player related tables */
#define	TABLE_PLAYER_INDEX					"player_index"
#define	TABLE_PLAYER_SKILLS					"player_skills"
#define	TABLE_PLAYER_AFFECTS				"player_affects"
#define	TABLE_PLAYER_QUESTS					"player_quests"
#define	TABLE_PLAYER_RPLOGS					"player_rplogs"
#define	TABLE_PLAYER_RPDESCRIPTIONS	"player_rpdescriptions"
#define	TABLE_PLAYER_RECOGNIZED			"player_recognized"
#define TABLE_PLAYER_NOTIFYLIST			"player_notifylist"

/* Resource related tables */
#define TABLE_RESOURCE_INDEX				"resource_index"

/* Guild related tables */
#define TABLE_GUILD_INDEX						"guild_index"
#define TABLE_GUILD_RANKS						"guild_ranks"
#define TABLE_GUILD_GUILDIES				"guild_guildies"
#define TABLE_GUILD_SPONSORER				"guild_sponsorer"
#define TABLE_GUILD_EQUIPMENT				"guild_equipment"
#define TABLE_GUILD_SKILLS					"guild_skills"
#define TABLE_GUILD_ZONES						"guild_zones"
#define TABLE_GUILD_HELP						"guild_help"

/* House related tables */
#define	TABLE_HOUSE_INDEX						"house_index"
#define	TABLE_HOUSE_ROOMS						"house_rooms"
#define	TABLE_HOUSE_COWNERS					"house_owners"
#define	TABLE_HOUSE_GUESTS					"house_guests"

/* Rent related tables */
#define TABLE_RENT_PLAYERS					"rent_players"
#define TABLE_RENT_OBJECTS					"rent_objects"
#define TABLE_RENT_AFFECTS					"rent_obj_affects"
#define TABLE_RENT_EXTRADESCS				"rent_obj_extradescs"

#define TABLE_HOUSE_RENT_OBJECTS		"rent_house_objects"
#define TABLE_HOUSE_RENT_AFFECTS		"rent_house_obj_affects"
#define TABLE_HOUSE_RENT_EXTRADESCS	"rent_house_obj_extradescs"

/* Tutor related tables */
#define TABLE_TUTOR_INDEX						"mob_tutor_data"
#define TABLE_TUTOR_SKILLS					"mob_tutor_skills"

/* Various other tables */
#define	TABLE_EVENTS								"events"
#define	TABLE_USAGE									"usagestats"
#define	TABLE_USAGE_HISTORY					"usage_history"
#define	TABLE_RACES									"races_list"
#define	TABLE_CULTURES							"cultures_list"
#define	TABLE_VERIFICATION					"pending_users"

#if BUFFER_MEMORY

#define	SQL_MALLOC(source, dest) ((dest) = get_buffer(65536));\
	if(!(source)) strcpy((dest), " ");
#define	SQL_FREE(string) release_buffer((string));

#else

#define	SQL_MALLOC(source, dest) ((dest) = (char *)malloc( (source) ? strlen((source)) * 2 + 1 : 1 ));\
	if(!(source)) strcpy((dest), " ");
#define	SQL_FREE(string) if(string) free(string);

#endif

#define	SQL_ESC(source, dest) if(source) mysql_escape_string(dest, source, strlen(source)); else dest[0]='\0'

MYSQL	mysql;

#else
#error "You have to include the MySQL library headers."
#endif

/* arbitrary constants used by index_boot() (must be unique) */
#define	DB_BOOT_WLD   0
#define	DB_BOOT_MOB   1
#define	DB_BOOT_OBJ   2
#define	DB_BOOT_ZON   3
#define	DB_BOOT_SHP   4
#define	DB_BOOT_HLP   5
#define	DB_BOOT_TRG   6
#define	DB_BOOT_QST   7
#define	DB_BOOT_GLD   8
#define DB_BOOT_HOU		9

#if	defined(CIRCLE_MACINTOSH)
#define	LIB_WORLD			":world:"
#define	LIB_TEXT			":text:"
#define	LIB_TEXT_HELP	":text:help:"
#define	LIB_MISC			":misc:"
#define	LIB_ETC				":etc:"
#define	LIB_PLRTEXT		":plrtext:"
#define	LIB_PLRALIAS	":plralias:"
#define	LIB_PLRTRAV		":plrtrav:"
#define	LIB_PLRVARS		":plrvars:"
#define	LIB_GUILDS		":guilds:"
#define	SLASH					":"
#elif	defined(CIRCLE_AMIGA) || defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS) || defined(CIRCLE_ACORN) || defined(CIRCLE_VMS)
#define	LIB_WORLD			"world/"
#define	LIB_TEXT			"text/"
#define	LIB_TEXT_HELP	"text/help/"
#define	LIB_MISC			"misc/"
#define	LIB_ETC				"etc/"
#define	LIB_PLRTEXT		"plrtext/"
#define	LIB_PLRALIAS	"plralias/"
#define	LIB_PLRTRAV		"plrtrav/"
#define	LIB_PLRVARS		"plrvars/"
#define	LIB_GUILDS		"guilds/"
#define	SLASH					"/"
#else
#error "Unknown path components."
#endif

#define	SUF_TEXT	"text"
#define	SUF_ALIAS	"alias"
#define	SUF_MEM		"mem"
#define	SUF_TRAV  "trav"

#if	defined(CIRCLE_AMIGA)
#define	FASTBOOT_FILE			"/.fastboot"				/* autorun: boot without sleep		*/
#define	KILLSCRIPT_FILE		"/.killscript"			/* autorun: shut mud down					*/
#define	PAUSE_FILE				"/pause"						/* autorun: don't restart mud			*/
#elif	defined(CIRCLE_MACINTOSH)
#define	FASTBOOT_FILE			"::.fastboot"				/* autorun: boot without sleep		*/
#define	KILLSCRIPT_FILE		"::.killscript"			/* autorun: shut mud down	*/
#define	PAUSE_FILE				"::pause"						/* autorun: don't restart mud			*/
#else
#define	FASTBOOT_FILE			"../.fastboot"			/* autorun: boot without sleep		*/
#define	KILLSCRIPT_FILE		"../.killscript"		/* autorun: shut mud down					*/
#define	PAUSE_FILE				"../pause"					/* autorun: don't restart mud			*/
#endif

/* names of various files and directories */
#define	INDEX_FILE				"index"							/* index of world files						*/
#define	MINDEX_FILE				"index.mini"				/* ... and for mini-mud-mode			*/
#define	WLD_PREFIX				LIB_WORLD"wld"SLASH	/* room definitions								*/
#define	MOB_PREFIX				LIB_WORLD"mob"SLASH	/* monster prototypes							*/
#define	OBJ_PREFIX				LIB_WORLD"obj"SLASH	/* object prototypes							*/
#define	ZON_PREFIX				LIB_WORLD"zon"SLASH	/* zon defs & command tables			*/
#define	SHP_PREFIX				LIB_WORLD"shp"SLASH	/* shop definitions								*/
#define	TRG_PREFIX				LIB_WORLD"trg"SLASH	/* shop definitions								*/
#define	QST_PREFIX				LIB_WORLD"qst"SLASH	/* For quest files								*/
#define	HLP_PREFIX				LIB_TEXT"help"SLASH	/* for HELP <keyword>							*/
#define GUILD_PREFIX			"guilds"SLASH				/* for guild files								*/

#define	HELP_FILE					"help.hlp"					/* default file for help					*/
#define	CREDITS_FILE			"credits"						/* for the 'credits' command			*/
#define	NEWS_FILE					"news"							/* for the 'news' command					*/
#define	MEETING_FILE			"meeting"						/* for the 'meeting' command			*/
#define	CHANGES_FILE			"changes"						/* for the 'changes' command			*/
#define	BUILDERCREDS_FILE	"buildercreds"			/* for the 'changes' command			*/
#define	MOTD_FILE					"motd"							/* messages of the day / mortal		*/
#define	IMOTD_FILE				"imotd"							/* messages of the day / immort		*/
#define	GREETINGS_FILE		"greetings"					/* The opening screen.						*/
#define	HELP_PAGE_FILE		"helpscreen"				/* for HELP <CR>									*/
#define	INFO_FILE					"info"							/* for INFO												*/
#define	WIZLIST_FILE			"wizlist"						/* for WIZLIST										*/
#define	IMMLIST_FILE			"immlist"						/* for IMMLIST										*/
#define	BACKGROUND_FILE		"background"				/* for the background story				*/
#define	POLICIES_FILE			"policies"					/* player policies/rules					*/
#define	HANDBOOK_FILE			"handbook"					/* handbook for new immorts				*/
#define	GUIDELINES_FILE		"guidelines"				/* handbook for new builders			*/
#define ROLEPLAY_FILE			"roleplay"					/* handbook for roleplaying				*/
#define SNOOPLIST_FILE		"snooplist"					/* players that are being snooped	*/
#define NAMEPOLICY_FILE		"namepolicy"				/* policies for name selection		*/

#define	IDEA_FILE		"ideas"				/* for the 'idea'-command					 */
#define	TYPO_FILE		"typos"				/*         'typo'									*/
#define	BUG_FILE		"bugs"				/*         'bug'									*/
#define	MESS_FILE					LIB_MISC"messages"	/* damage messages								*/
#define	SOCMESS_FILE			LIB_MISC"socials"		/* messgs for social acts					*/
#define	XNAME_FILE				LIB_MISC"xnames"		/* invalid name substrings				*/

#define	PLAYER_FILE				LIB_ETC"players"		/* the player database						*/
#define	ASSEMBLIES_FILE		LIB_ETC"assemblies"	/* Assemblies engine							*/
#define	MAIL_FILE					LIB_ETC"plrmail"		/* for the mudmail system					*/
#define	BAN_FILE					LIB_ETC"badsites"		/* for the siteban system					*/
#define	HCONTROL_FILE			LIB_ETC"hcontrol"		/* for the house system						*/
#define	SPELL_FILE				LIB_ETC"spells"			/* the spells file								*/
#define TIME_FILE					LIB_ETC"time"				/* for calendar system						*/

/* change these if you want to put all files in the same directory (or if
	 you just like big file names
*/
#define	PLR_SUFFIX				""

/* new bitvector data for use in player_index_element */
#define	PINDEX_DELETED		(1 << 0)	/* deleted player	*/
#define	PINDEX_NODELETE		(1 << 1)	/* protected player	*/
#define	PINDEX_SELFDELETE	(1 << 2)	/* player is selfdeleting*/

/* public procedures in db.c */
void boot_db(void);
void	destroy_db(void);
void init_mysql(const char *mysqldb);
void kill_mysql(void);
void mysql_keepalive(void);
int	create_entry(char *name);
void	zone_update(void);
char	*fread_string(FILE *fl, const char *error);
long	get_id_by_name(char *name);
char	*get_name_by_id(long id);
void	save_mud_time(struct time_info_data *when);
void	free_extra_descriptions(struct extra_descr_data *edesc);
void	free_text_files(void);
void	free_player_index(void);

zone_rnum real_zone(zone_vnum vnum);
room_rnum real_room(room_vnum vnum);
mob_rnum real_mobile(mob_vnum vnum);
obj_rnum real_object(obj_vnum vnum);

int	load_char(char *name, struct char_data *ch);

void	save_char(struct char_data *ch, room_vnum load_room, int system);
void	init_char(struct char_data *ch);
struct char_data* create_char(void);
struct char_data *read_mobile(mob_vnum nr, int type);
void	clear_char(struct char_data *ch);
void	reset_char(struct char_data *ch);
void	free_char(struct char_data *ch);
void	save_player_index(void);

struct obj_data *create_obj(void);
void	clear_object(struct obj_data *obj);
void	free_obj(struct obj_data *obj);
struct obj_data *read_object(obj_vnum nr, int type);

MYSQL_RES *mysqlGetResource(const char *table, const char *format, ...);
bool mysqlWrite(const char *format, ...);

#define	REAL 0
#define	VIRTUAL 1


/* structure for the reset commands */
struct reset_com {
	int id;
	char	command;	/* current command											*/

	bool if_flag;		/* if TRUE: exe only if preceding exe'd	*/
	int	arg1;				/*																			*/
	int	arg2;				/* Arguments to the command							*/
	int	arg3;				/*																			*/
	int line;				/* line number this command appears on	*/
	char *sarg1;		/* string argument											*/
	char *sarg2;		/* string argument											*/

 /* 
	*  Commands:              *
	*  'M': Read a mobile     *
	*  'O': Read an object    *
	*  'G': Give obj to mob   *
	*  'P': Put obj in obj    *
	*  'G': Obj to char       *
	*  'E': Obj to char equip *
	*  'D': Set state of door *
	*  'T': Trigger command   *
	*/
};


/* zone definition structure. for the 'zone-table'   */
struct zone_data {
	char	*name;											/* name of this zone									*/
	char *builders;										/* builder list for this zone					*/
	int	lifespan;           					/* how long between resets (minutes)	*/
	int	age;													/* current age of this zone (minutes)	*/
	room_vnum bot;										/* starting room number for this zone */
	room_vnum top;										/* upper limit for rooms in this zone	*/

	/*
	 * Reset mode:
	 *   0: Don't reset, and don't update age.
	 *   1: Reset if no PC's are located in zone.
	 *   2: Just reset.
	 */
	int	reset_mode;										/* conditions for reset (see below)		*/
	zone_vnum number;									/* virtual number of this zone				*/
	struct reset_com *cmd;						/* command table for reset						*/
	bitvector_t zone_flags;						/* for zone flags											*/
};


/* for queueing zones for update   */
struct reset_q_element {
	zone_rnum zone_to_reset;            /* ref to zone_data */
	struct reset_q_element *next;
};


/* structure for the update queue     */
struct reset_q_type {
	struct reset_q_element *head;
	struct reset_q_element *tail;
};


/* Added rights, flags, and last, primarily for pfile autocleaning.  You
	 can also use them to keep online statistics, and can add race, class,
	 etc if you like.
*/
struct player_index_element {
	char	*name;
	long id;
	bitvector_t rights;
	int flags;
	time_t last;
	int active;
};


struct tip_index_element {
	bitvector_t	rights;
	char		*title;
	char		*tip;
};


struct help_categories_element {
	int		num;
	char	*name;
	char	*text;
	bitvector_t rights;
};


struct puff_messages_element {
	char	*message;
};


/* don't change these */
#define	BAN_NOT 	0
#define	BAN_NEW 	1
#define	BAN_SELECT	2
#define	BAN_ALL		3

#define	BANNED_SITE_LENGTH    50
struct ban_list_element {
	char	site[BANNED_SITE_LENGTH+1];
	int	type;
	time_t date;
	char	name[MAX_NAME_LENGTH+1];
	struct ban_list_element *next;
};


/* global buffering system */

#ifdef __DB_C__
#if USE_CIRCLE_BUFFERS
char	buf[MAX_STRING_LENGTH];
char	buf1[MAX_STRING_LENGTH];
char	buf2[MAX_STRING_LENGTH];
char	arg[MAX_STRING_LENGTH];
#endif
#else
extern struct room_data *world;
extern room_rnum top_of_world;

extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;

extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct player_special_data dummy_mob;
extern int top_of_p_table;

extern int start_zone_index;
extern int ooc_lounge_index;
extern int default_ic_index;

extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern mob_rnum top_of_mobt;

extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto;
extern obj_rnum top_of_objt;

/* buffers for communication channels */
extern char *gossip[COMM_SIZE];
extern char *newbie[COMM_SIZE];
extern char *sing[COMM_SIZE];
extern char *obscene[COMM_SIZE];


#if USE_CIRCLE_BUFFERS
extern char	buf[MAX_STRING_LENGTH];
extern char	buf1[MAX_STRING_LENGTH];
extern char	buf2[MAX_STRING_LENGTH];
extern char	arg[MAX_STRING_LENGTH];
#endif
#endif

#ifndef	__CONFIG_C__
extern char	*OK;
extern char	*NOPERSON;
extern char	*NOEFFECT;
#endif

#define	CUR_WORLD_VERSION   1
#define	CUR_ZONE_VERSION    2

