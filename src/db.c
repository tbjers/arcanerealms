/* ************************************************************************
*		File: db.c                                          Part of CircleMUD *
*	 Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001, Torgny Bjers.                                       *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
************************************************************************ */
/* $Id: db.c,v 1.207 2004/04/30 17:14:01 cheron Exp $ */

#define	__DB_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "diskio.h"
#include "pfdefaults.h"
#include "assemblies.h"
#include "quest.h"
#include "specset.h"
#include "loadrooms.h"
#include "shop.h"
#include "characters.h"
#include "roleplay.h"
#include "oasis.h"
#include "guild.h"
#include "guild_parser.h"
#include "tutor.h"
#include "color.h"

/**************************************************************************
*	 declarations of most of the 'global' variables                         *
**************************************************************************/

struct room_data *world = NULL;						/* array of rooms									*/
room_rnum	top_of_world = 0;								/* ref to top element of world		*/

struct char_data *character_list = NULL;	/* global linked list of chars		*/

struct index_data **trig_index;						/* index table for triggers				*/
struct trig_data *trigger_list = NULL;		/* all attached triggers					*/
int	top_of_trigt = 0;											/* top of trigger index table			*/
long max_mob_id = MOB_ID_BASE;						/* for unique mob id's						*/
long max_obj_id = OBJ_ID_BASE;						/* for unique obj id's						*/
int dg_owner_purged;											/* For control of scripts					*/
				
struct index_data *mob_index;							/* index table for mobile file		*/
struct char_data *mob_proto;							/* prototypes for mobs						*/
mob_rnum top_of_mobt = 0;									/* top of mobile index table			*/

struct obj_data *object_list = NULL;			/* global linked list of objs			*/
struct index_data *obj_index;							/* index table for object file		*/
struct obj_data *obj_proto;								/* prototypes for objs						*/
obj_rnum top_of_objt = 0;									/* top of object index table			*/

struct zone_data *zone_table;							/* zone table											*/
zone_rnum	top_of_zone_table = 0;					/* top element of zone table			*/
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages			*/

struct aq_data *aquest_table;
int	top_of_aquestt = 0;

struct shop_data *shop_index;
int	top_shop = 0;

struct house_data *house_index;
int top_of_houset = 0;

extern int top_of_spellt;

struct player_index_element *player_table = NULL;	/* index to plr file			*/
int	top_of_p_table = 0;										/* ref to top of table						*/
int	top_of_p_file = 0;										/* ref of size of p file					*/
long top_idnum = 0;												/* highest idnum in use						*/

struct tip_index_element *tip_table = NULL;	/* index to tip table						*/
int	top_of_tip_table = 0;										/* ref to top of table					*/

struct help_categories_element *help_categories = NULL;	/* index to help category table	*/
int	top_of_help_categories = 0;						/* ref to top of table						*/

struct puff_messages_element *puff_messages = NULL;	/* index to puff table	*/
int	top_of_puff_messages = 0;							/* ref to top of table						*/

struct race_list_element *race_list = NULL;	/* index to race list table	*/
int	top_of_race_list = 0;									/* ref to top of table						*/

struct culture_list_element *culture_list = NULL;	/* index to culture list table	*/
int	top_of_culture_list = 0;							/* ref to top of table						*/

int zone_reset = 1;												/* zone resets?										*/
int	no_mail = 0;													/* mail disabled?									*/
int	mini_mud = 0;													/* mini-mud mode?									*/
int	no_rent_check = 0;										/* skip rent check on boot?				*/
time_t boot_time = 0;											/* time of mud boot								*/
time_t copyover_time = 0;									/* time of mud copyover						*/
int total_hotboots = 0;										/* total amounts of hotboots			*/
int	circle_restrict = 0;									/* level of game restriction			*/
sh_int r_mortal_start_room[NUM_STARTROOMS +1];	/* hometowns								*/
room_rnum	r_immort_start_room;						/* rnum of immort start room			*/
room_rnum	r_frozen_start_room;						/* rnum of frozen start room			*/
room_rnum	r_new_start_room;								/* rnum of new player start room	*/
int start_zone_index = 7;									/* index of 'hometowns' of new player start room */
int ooc_lounge_index = 8;									/* index of 'hometowns' of OOC Lounge */
int default_ic_index = 1;									/* index of default TOG IC room		*/

char *credits = NULL;											/* game credits										*/
char *news = NULL;												/* mud news												*/
char *motd = NULL;												/* message of the day - mortals		*/
char *imotd = NULL;												/* message of the day - immorts		*/
char *GREETINGS = NULL;										/* opening credits screen					*/
char *help = NULL;												/* help screen										*/
char *info = NULL;												/* info page											*/
char *wizlist = NULL;											/* list of higher gods						*/
char *immlist = NULL;											/* list of peon gods							*/
char *background = NULL;									/* background story								*/
char *handbook = NULL;										/* handbook for new immortals			*/
char *policies = NULL;										/* policies page									*/
char *meeting = NULL;											/* mud meetings news							*/
char *changes = NULL;											/* mud changes file								*/
char *buildercreds = NULL;								/* mud buildercreds file					*/
char *guidelines = NULL;									/* mud builder guidelines file		*/
char *roleplay = NULL;										/* roleplay file									*/
char *snooplist = NULL;										/* snooplist file									*/
char *namepolicy = NULL;									/* name policy file								*/
char *ideas = NULL;
char *bugs = NULL;
char *typos = NULL;

char *gossip[COMM_SIZE];
char *newbie[COMM_SIZE];
char *sing[COMM_SIZE];
char *obscene[COMM_SIZE];

struct help_index_element *help_table = 0;/* the help table									*/
int	top_of_helpt = 0;											/* top of help index table				*/

struct time_info_data time_info;					/* infomation about the time			*/
struct weather_data weather_info;					/* infomation about the weather		*/
struct player_special_data dummy_mob;			/* dummy spec area for mobs				*/
struct reset_q_type reset_q;							/* queue of zones to be reset			*/

int in_copyover = 0;											/* copyover initiation						*/

extern struct class_data classes[NUM_CLASSES];

extern char *mysql_host;
extern char *mysql_db;
extern char *mysql_usr;
extern char *mysql_pwd;
extern unsigned int mysqlport;
extern char *mysqlsock;

extern bool fCopyOver;

extern char *SELECT_NAME;

/* local functions */
int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits);
bitvector_t	asciiflag_conv(char *flag);
char fread_letter(FILE *fp);
extern void read_mud_date_from_file(void);
int	check_object(struct obj_data *obj);
int	check_object_level(struct obj_data *obj, int val);
int	check_object_spell_number(struct obj_data *obj, int val);
int	check_mobile_values(struct char_data *mob);
int	is_empty(zone_rnum zone_nr);
int	query_to_string(const char *name, char *buf);
int	query_to_string_alloc(const char *name, char **buf);
long get_ptable_by_name(char *name);
void assign_dts(void);
void assign_spec_procs(char type, sh_int vnum, int which, char startup);
void assign_the_shopkeepers(void);
void boot_help_categories(void);
void boot_tip_messages(void);
void boot_puff_messages(void);
void boot_races(void);
void boot_cultures(void);
void boot_world(void);
void build_player_index(void);
void check_start_rooms(void);
void common_index_boot(MYSQL_RES *index, MYSQL_RES *content, int mode);
void discrete_query(MYSQL_RES * content, int mode, int znum);
void get_one_line(FILE *fl, char *buf);
void index_boot(int mode);
void interpret_espec(const char *keyword, const char *value, int i, int nr);
void kill_ems(char *str);
void load_sql_settings(void);
void load_zones(MYSQL_RES * content, int znum);
void log_zone_error(zone_rnum zone, int cmd_no, int id, const char *message);
void mobile_boot_query(MYSQL_RES *content, MYSQL_RES *triggers, int znum);
void mobile_index_boot(MYSQL_RES *index, MYSQL_RES *content);
void mysql_keepalive(void);
void object_boot_query(MYSQL_RES *content, MYSQL_RES *extradescs, MYSQL_RES *affects, MYSQL_RES *triggers, int znum);
void object_index_boot(MYSQL_RES *index, MYSQL_RES *content);
void parse_mobile(MYSQL_ROW mobile_row, MYSQL_RES *triggers, int vnum);
void parse_object(MYSQL_ROW object_row, MYSQL_RES *extradescs, MYSQL_RES *affects, MYSQL_RES *triggers, int vnum);
void parse_quest(MYSQL_ROW quest_row, unsigned long *fieldlength, int vnum);
void parse_room(MYSQL_ROW world_row, MYSQL_RES *extradescs, MYSQL_RES *exits, MYSQL_RES *triggers, int vnum);
void parse_shop(MYSQL_ROW shop_row, unsigned long *fieldlength, int vnum);
void reboot_wizlists(void);
void remove_player(int pfilepos);
void renum_world(void);
void renum_zone_table(void);
void reset_room_flux(struct room_data *room);
void reset_room_resources(struct room_data *room);
void reset_time(void);
void reset_zone(zone_rnum zone);
void setup_dir(MYSQL_ROW exits_row, unsigned long *fieldlength, int room);
void shop_index_boot(MYSQL_RES *index, MYSQL_RES *content);
void world_boot_query(MYSQL_RES *content, MYSQL_RES *extradescs, MYSQL_RES *exits, MYSQL_RES *triggers, int znum);
void world_index_boot(MYSQL_RES *index, MYSQL_RES *content);
void shop_boot_query(MYSQL_RES *content, MYSQL_RES *products, MYSQL_RES *keywords, MYSQL_RES *rooms, int znum);
void set_default_rpdesc(struct char_data *ch, int descnum);
void boot_houses(MYSQL_RES *content);
ACMD(do_reboot);
ACMD(do_reload);

/* external functions */
void paginate_string(char *str, struct descriptor_data *d);
extern void make_who2html(void);
int find_name(const char *name);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void assign_skills(void);
void boot_social_messages(void);
void clean_players(int preview);
void create_command_list(void);
void free_alias(struct alias_data *a);
void free_object_strings(struct obj_data *obj);
void free_object_strings_proto(struct obj_data *obj);
void free_travels(struct char_data *ch);
void load_banned(void);
void load_config(void);
void load_messages(void);
void prune_crlf(char *txt);
void Read_Invalid_List(void);
void save_char_vars(struct char_data *ch);
void sort_commands(void);
void sort_skills(void);
void sort_spells(void);
int sprintascii(char *out, bitvector_t bits);
void update_obj_file(void);        /* In objsave.c */
void weather_and_time(int mode);
void initiate_copyover(bool panic);
void load_spells(void);
void destroy_shops(void);
void	tag_argument(char *argument, char *tag);
void boot_command_list(void);
void boot_guilds(MYSQL_RES *content);
void parse_house(MYSQL_ROW house_row, unsigned long *fieldlength, int vnum);
void set_default_ldesc(struct char_data *ch);

/* external vars */
extern int bautosave;
extern int no_specials;
extern int scheck;
extern const char *unused_spellname;
extern room_vnum frozen_start_room;
extern room_vnum immort_start_room;
extern room_vnum new_start_room;
extern sh_int mortal_start_room[NUM_STARTROOMS +1];
extern struct descriptor_data *descriptor_list;
extern struct spell_info_type *spell_info;
extern int db_command_line;

/* external ascii pfile vars */
extern int auto_pwipe;
extern int selfdelete_fastwipe;
extern struct pclean_criteria_data pclean_criteria[];

/* ascii pfiles - set this TRUE if you want poofin/poofout
 * strings saved in the pfiles
 */
#define	ASCII_SAVE_POOFS        FALSE

#define	READ_SIZE 256


/*************************************************************************
*	 routines for booting the system                                       *
*************************************************************************/

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
	query_to_string_alloc(WIZLIST_FILE, &wizlist);
	query_to_string_alloc(IMMLIST_FILE, &immlist);
}


/* Wipe out all the loaded text files, for shutting down. */
void free_text_files(void)
{
	char **textfiles[] = {
	&credits, &news, &motd, &imotd, &GREETINGS, &help, &info, &wizlist,
	&immlist, &background, &handbook, &policies, &meeting, &changes,
	&buildercreds, &guidelines, &roleplay, &snooplist, 
	&ideas, &typos, &bugs, &namepolicy, NULL
	};
	int rf;

	for (rf = 0; textfiles[rf]; rf++)
		if (*textfiles[rf]) {
			free(*textfiles[rf]);
			*textfiles[rf] = NULL;
		}
}


/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reload)
{
	one_argument(argument, arg);

	if (!str_cmp(arg, "all") || *arg == '*') {
		if (query_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
			prune_crlf(GREETINGS);
		query_to_string_alloc(WIZLIST_FILE, &wizlist);
		query_to_string_alloc(IMMLIST_FILE, &immlist);
		query_to_string_alloc(NEWS_FILE, &news);
		query_to_string_alloc(MEETING_FILE, &meeting);
		query_to_string_alloc(CHANGES_FILE, &changes);
		query_to_string_alloc(BUILDERCREDS_FILE, &buildercreds);
		query_to_string_alloc(GUIDELINES_FILE, &guidelines);
		query_to_string_alloc(CREDITS_FILE, &credits);
		query_to_string_alloc(MOTD_FILE, &motd);
		query_to_string_alloc(IMOTD_FILE, &imotd);
		query_to_string_alloc(HELP_PAGE_FILE, &help);
		query_to_string_alloc(INFO_FILE, &info);
		query_to_string_alloc(POLICIES_FILE, &policies);
		query_to_string_alloc(HANDBOOK_FILE, &handbook);
		query_to_string_alloc(BACKGROUND_FILE, &background);
		query_to_string_alloc(ROLEPLAY_FILE, &roleplay);
		query_to_string_alloc(SNOOPLIST_FILE, &snooplist);
		query_to_string_alloc(NAMEPOLICY_FILE, &namepolicy);
		query_to_string_alloc(IDEA_FILE, &ideas);
		query_to_string_alloc(BUG_FILE, &bugs);
		query_to_string_alloc(TYPO_FILE, &typos);
	} else if (!str_cmp(arg, "wizlist"))
		query_to_string_alloc(WIZLIST_FILE, &wizlist);
	else if (!str_cmp(arg, "immlist"))
		query_to_string_alloc(IMMLIST_FILE, &immlist);
	else if (!str_cmp(arg, "news"))
		query_to_string_alloc(NEWS_FILE, &news);
	else if (!str_cmp(arg, "meeting"))
		query_to_string_alloc(MEETING_FILE, &meeting);
	else if (!str_cmp(arg, "changes"))
		query_to_string_alloc(CHANGES_FILE, &changes);
	else if (!str_cmp(arg, "bcreds"))
		query_to_string_alloc(BUILDERCREDS_FILE, &buildercreds);
	else if (!str_cmp(arg, "guidelines"))
		query_to_string_alloc(GUIDELINES_FILE, &guidelines);
	else if (!str_cmp(arg, "credits"))
		query_to_string_alloc(CREDITS_FILE, &credits);
	else if (!str_cmp(arg, "motd"))
		query_to_string_alloc(MOTD_FILE, &motd);
	else if (!str_cmp(arg, "imotd"))
		query_to_string_alloc(IMOTD_FILE, &imotd);
	else if (!str_cmp(arg, "help"))
		boot_help_categories();
	else if (!str_cmp(arg, "tip"))
		boot_tip_messages();
	else if (!str_cmp(arg, "puff"))
		boot_puff_messages();
	else if (!str_cmp(arg, "player"))
		build_player_index();
	else if (!str_cmp(arg, "info"))
		query_to_string_alloc(INFO_FILE, &info);
	else if (!str_cmp(arg, "policy"))
		query_to_string_alloc(POLICIES_FILE, &policies);
	else if (!str_cmp(arg, "handbook"))
		query_to_string_alloc(HANDBOOK_FILE, &handbook);
	else if (!str_cmp(arg, "background"))
		query_to_string_alloc(BACKGROUND_FILE, &background);
	else if (!str_cmp(arg, "roleplay"))
		query_to_string_alloc(ROLEPLAY_FILE, &roleplay);
	else if (!str_cmp(arg, "snooplist"))
		query_to_string_alloc(SNOOPLIST_FILE, &snooplist);
	else if (!str_cmp(arg, "namepolicy"))
		query_to_string_alloc(NAMEPOLICY_FILE, &namepolicy);
	else if (!str_cmp(arg, "ideas"))
		query_to_string_alloc(IDEA_FILE, &ideas);
	else if (!str_cmp(arg, "bugs"))
		query_to_string_alloc(BUG_FILE, &bugs);
	else if (!str_cmp(arg, "typos"))
		query_to_string_alloc(TYPO_FILE, &typos);
	else if (!str_cmp(arg, "greetings")) {
		if (query_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
			prune_crlf(GREETINGS);
	} else if (!str_cmp(arg, "config")) {
		if (!IS_ADMIN(ch)) {
			send_to_char("You have to be at least an administrator to be able to reload the configuration.\r\n", ch);
			return;
		}
		load_config();
		extended_mudlog(NRM, SYSL_GENERAL, TRUE, "%s reloaded the configuration.", GET_NAME(ch));
	} else {
		send_to_char("Unknown reload option.\r\n", ch);
		return;
	}
	
	if (ch){
		send_to_char(OK, ch);
	}
}


void boot_world(void)
{
	mlog("Loading zone table.");
	index_boot(DB_BOOT_ZON);

	mlog("Loading triggers and generating index.");
	index_boot(DB_BOOT_TRG);

	mlog("Loading rooms.");
	index_boot(DB_BOOT_WLD);

	mlog("Renumbering rooms.");
	renum_world();

	mlog("Checking start rooms.");
	check_start_rooms();

	mlog("Loading objs and generating index.");
	index_boot(DB_BOOT_OBJ);

	mlog("Loading mobs and generating index.");
	index_boot(DB_BOOT_MOB);

	mlog("Renumbering zone table.");
	renum_zone_table();

	if (!no_specials) {
		mlog("Loading shops.");
		index_boot(DB_BOOT_SHP);
	}

	mlog("Loading quests and generating index.");
	index_boot(DB_BOOT_QST);

	mlog("Cleaning who2html.");
	make_who2html();

}


/*
 * load_sql_settings() reads MySQL host, db, username, and password.
 * Thanks to Mike Stilson and Daniel A. Koepke for help with this one.
 * Function by Torgny Bjers for Arcane Realms, 2002-03-05, 14:34:44
 */
void load_sql_settings(void) {
	FBFILE *fl;
	char line[MAX_INPUT_LENGTH + 1], tag[6];

	if(!(fl = fbopen(LIB_ETC"mysql", FB_READ))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "SYSERR: Couldn't open MySQL settings.");
		exit(1);
	}

	while(fbgetline(fl, line)) {
		tag_argument(line, tag);

		if(!strcmp(tag, "host"))
			mysql_host = str_dup(line);
		else if(!strcmp(tag, "db  ")) {
			if (!db_command_line) /* Command line overrides */
				mysql_db = str_dup(line);
		}
		else if(!strcmp(tag, "user"))
			mysql_usr = str_dup(line);
		else if(!strcmp(tag, "pass"))
			mysql_pwd = str_dup(line);
		else if(!strcmp(tag, "port"))
			mysqlport = atoi(line);
		else if(!strcmp(tag, "sock"))
			mysqlsock = str_dup(line);
		else {
			extended_mudlog(BRF, SYSL_SQL, TRUE, "SYSERR: Unknown option %s in MySQL settings.", line);
			exit(1);
		}
	}

	fbclose(fl);

}


/*
 * init_mysql for MySQL by Torgny Bjers, 2001-09-08.
 * Updated 2003-11-18: mysql_connect() is deprecated. Now using
 * mysql_real_connect() instead. For more information, see:
 * http://www.mysql.com/doc/en/mysql_real_connect.html
 */
void init_mysql(const char *mysqldb)
{
	mysql_init(&mysql);
	/*
   * By using mysql_options() the MySQL library will read the [client] and
	 * [your_prog_name] sections in the `my.cnf' file which will ensure that
	 * your program will work, even if someone has set up MySQL in some
	 * non-standard way.
	 */
	mysql_options(&mysql, MYSQL_READ_DEFAULT_GROUP, "circle");
	/*
	 * Make the connection to the MySQL server.
	 */
	if (!mysql_real_connect(&mysql, mysql_host, mysql_usr, mysql_pwd, mysqldb, mysqlport, mysqlsock, CLIENT_COMPRESS | CLIENT_INTERACTIVE)) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Failed to connect to MySQL. (%s)%s:%d: %s", __FILE__, __FUNCTION__, __LINE__, mysql_error(&mysql));
		kill_mysql();
		exit(1);
	} else {
		mlog("MySQL connection established with %s,", mysql_get_host_info(&mysql));
		mlog("using MySQL C API library %s.", mysql_get_client_info());
		mlog("MySQL server %s using database: %s", mysql_get_server_info(&mysql), mysqldb);
	}
}
	

/* kill_mysql for MySQL by Torgny Bjers, 2001-09-08. */
void kill_mysql(void)
{
	mlog("Closing MySQL connection.");
	mysql_close(&mysql);
}


/* mysql_keepalive for MySQL by Torgny Bjers, 2002-01-02. */
void mysql_keepalive(void)
{
	if ((mysql_ping(&mysql)) != 0) {
		extended_mudlog(NRM, SYSL_SQL, TRUE, "MySQL server returned error on ping: %s", mysql_error(&mysql));
		initiate_copyover(TRUE);
	}
}


/*
 * Wrapper for MySQL database query read methods:
 * SELECT
 * Returns a valid MYSQL_RES on success, and NULL on failure.
 * Function by Torgny Bjers for Arcane Realms, 2002-03-05, 14:34:44
 */
MYSQL_RES *mysqlGetResource(const char *table, const char *format, ...)
{
	MYSQL_RES *resource;
	char query[MAX_STRING_LENGTH];
	va_list args;

	// Check to make sure MySQL is alive.
	mysql_keepalive();

	va_start(args, format);
	vsprintf(query, format, args);
	va_end(args);

	if(mysql_real_query(&mysql, query, (unsigned int)strlen(query))) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error loading %s.", table);
		extended_mudlog(BRF, SYSL_SQL, TRUE, "SQL: Query error [%s] (%s)%s:%d: %s", query, __FILE__, __FUNCTION__, __LINE__, mysql_error(&mysql));
		return (NULL);
	}

	if (!(resource = mysql_store_result(&mysql))) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "SYSERR: Error loading %s.", table);
		extended_mudlog(BRF, SYSL_SQL, TRUE, "SQL: Query error (%s)%s:%d: %s", __FILE__, __FUNCTION__, __LINE__, mysql_error(&mysql));
		return (NULL);
	}

	return (resource);
}


/*
 * Wrapper for MySQL database query write methods:
 * REPLACE, UPDATE, DELETE, INSERT
 * Returns true on success, false on failure.
 * Function by Torgny Bjers for Arcane Realms, 2002-03-05, 14:34:44
 */
bool mysqlWrite(const char *format, ...)
{
	char query[MAX_STRING_LENGTH];
	va_list args;

	// Check to make sure MySQL is alive.
	mysql_keepalive();

	va_start(args, format);
	vsnprintf(query, MAX_STRING_LENGTH, format, args);
	va_end(args);

	if(mysql_real_query(&mysql, query, (unsigned int)strlen(query))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Query error (%s)%s:%d: %s", __FILE__, __FUNCTION__, __LINE__, mysql_error(&mysql));
		mlog("QUERYERR: %s", query);
		return (FALSE);
	}

	return (TRUE);
}


void free_extra_descriptions(struct extra_descr_data *edesc)
{
	struct extra_descr_data *enext;

	for (; edesc; edesc = enext) {
		enext = edesc->next;

		free(edesc->keyword);
		free(edesc->description);
		free(edesc);
	}
}


/* Free the world, in a memory allocation sense. */
void destroy_db(void)
{
	ssize_t cnt, itr;
	struct char_data *chtmp;
	struct obj_data *objtmp;

	/* Active Mobiles & Players */
	while (character_list) {
		chtmp = character_list;
		character_list = character_list->next;
		free_char(chtmp);
	}

	/* Active Objects */
	while (object_list) {
		objtmp = object_list;
		object_list = object_list->next;
		free_obj(objtmp);
	}

	/* Rooms */
	for (cnt = 0; cnt <= top_of_world; cnt++) {
		if (world[cnt].name)
			free(world[cnt].name);
		if (world[cnt].description)
			free(world[cnt].description);
		free_extra_descriptions(world[cnt].ex_description);

		for (itr = 0; itr < NUM_OF_DIRS; itr++) {
			if (!world[cnt].dir_option[itr])
				continue;

			if (world[cnt].dir_option[itr]->general_description)
				free(world[cnt].dir_option[itr]->general_description);
			if (world[cnt].dir_option[itr]->keyword)
				free(world[cnt].dir_option[itr]->keyword);
			free(world[cnt].dir_option[itr]);
		}
	}
	free(world);

	/* Objects */
	for (cnt = 0; cnt <= top_of_objt; cnt++) {
		if (obj_proto[cnt].name)
			free(obj_proto[cnt].name);
		if (obj_proto[cnt].description)
			free(obj_proto[cnt].description);
		if (obj_proto[cnt].short_description)
			free(obj_proto[cnt].short_description);
		if (obj_proto[cnt].action_description)
			free(obj_proto[cnt].action_description);
		free_extra_descriptions(obj_proto[cnt].ex_description);
	}
	free(obj_proto);
	free(obj_index);

	/* Mobiles */
	for (cnt = 0; cnt <= top_of_mobt; cnt++) {
		if (mob_proto[cnt].player.name)
			free(mob_proto[cnt].player.name);
		if (mob_proto[cnt].player.title)
			free(mob_proto[cnt].player.title);
		if (mob_proto[cnt].player.short_descr)
			free(mob_proto[cnt].player.short_descr);
		if (mob_proto[cnt].player.long_descr)
			free(mob_proto[cnt].player.long_descr);
		if (mob_proto[cnt].player.description)
			free(mob_proto[cnt].player.description);

		while (mob_proto[cnt].affected)
			affect_remove(&mob_proto[cnt], mob_proto[cnt].affected);
	}
	free(mob_proto);
	free(mob_index);

	/* Shops */
	destroy_shops();

	/* Zones */
	for (cnt = 0; cnt <= top_of_zone_table; cnt++) {
		if (zone_table[cnt].name)
			free(zone_table[cnt].name);
		if (zone_table[cnt].cmd)
			free(zone_table[cnt].cmd);
	}
	free(zone_table);
}


/* initialize communication buffers */
void init_comm_buff(void)
{
	int i = 0;

	for (i = 0; i < COMM_SIZE; i++) {
		gossip[i] = NULL;
		newbie[i] = NULL;
		sing[i] = NULL;
		obscene[i] = NULL;
	}
}


/* body of the booting system */
void boot_db(void)
{
	zone_rnum i;

	mlog("Boot db -- BEGIN.");

	mlog("Resetting the game time:");
	reset_time();

	mlog("Reading news, meeting, credits, help, bground, info & motds.");
	query_to_string_alloc(NEWS_FILE, &news);
	query_to_string_alloc(MEETING_FILE, &meeting);
	query_to_string_alloc(CREDITS_FILE, &credits);
	query_to_string_alloc(CHANGES_FILE, &changes);
	query_to_string_alloc(BUILDERCREDS_FILE, &buildercreds);
	query_to_string_alloc(GUIDELINES_FILE, &guidelines);
	query_to_string_alloc(MOTD_FILE, &motd);
	query_to_string_alloc(IMOTD_FILE, &imotd);
	query_to_string_alloc(INFO_FILE, &info);
	query_to_string_alloc(WIZLIST_FILE, &wizlist);
	query_to_string_alloc(IMMLIST_FILE, &immlist);
	query_to_string_alloc(POLICIES_FILE, &policies);
	query_to_string_alloc(HANDBOOK_FILE, &handbook);
	query_to_string_alloc(BACKGROUND_FILE, &background);
	query_to_string_alloc(ROLEPLAY_FILE, &roleplay);
	query_to_string_alloc(IDEA_FILE, &ideas);
	query_to_string_alloc(BUG_FILE, &bugs);
	query_to_string_alloc(TYPO_FILE, &typos);
	query_to_string_alloc(SNOOPLIST_FILE, &snooplist);
	if (query_to_string_alloc(NAMEPOLICY_FILE, &namepolicy))
		strcpy(namepolicy, SELECT_NAME);
	if (query_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
		prune_crlf(GREETINGS);

	mlog("Loading skill definitions, %d skills loaded.", NUM_GROUPS + NUM_SKILLS);
	assign_skills();

	mlog("Loading spell definitions.");
	load_spells();

	mlog("Loading race definitions.");
	boot_races();

	mlog("Loading culture definitions.");
	boot_cultures();

	boot_world();

	mlog("Loading help categories.");
	boot_help_categories();

	mlog("Loading tip messages.");
	boot_tip_messages();

	mlog("Loading puff messages.");
	boot_puff_messages();

	mlog("Generating player index.");
	build_player_index();

	if(auto_pwipe) {
		mlog("Auto-cleaning the player database.");
		clean_players(0);
		mlog("Auto-pruning houses.");
		house_auto_prune();
	}

	mlog("Loading guilds.");
	index_boot(DB_BOOT_GLD);

	if (!mini_mud) {
		mlog("Loading houses and generating index.");
		index_boot(DB_BOOT_HOU);
	}

	mlog("Loading fight messages.");
	load_messages();

	mlog("Loading commands.");
	boot_command_list();

	mlog("Loading social messages.");
	boot_social_messages();

	mlog("Creating Master Command List.");
	create_command_list();

	mlog("Assigning Special Procedures.");
	assign_spec_procs(0,0,0,1);

	mlog("Loading tutors.");
	load_tutors();

	mlog("Assigning function pointers:");

	if (!no_specials) {
		mlog("   Shopkeepers.");
		assign_the_shopkeepers();
		mlog("   Death trap dumps.");
		assign_dts();
		mlog("   Tutors.");
		assign_the_tutors();
	}

	if (!mini_mud) {
		mlog("Booting assembled objects.");
		assemblyBootAssemblies();
	}
	
	mlog("Initializing communication buffers.");
	init_comm_buff();

	mlog("Sorting command list.");
	sort_commands();

	mlog("Sorting spell list.");
	sort_spells();

	mlog("Sorting skill list.");
	sort_skills();

	mlog("Sorting tutor list.");
	sort_tutors();

	mlog("Booting mail system.");
	if (!scan_file()) {
		mlog("   Mail boot failed -- Mail system disabled");
		no_mail = 1;
	}
	mlog("Reading banned site and invalid-name list.");
	load_banned();
	Read_Invalid_List();

	if (!no_rent_check) {
		mlog("Deleting timed-out crash and rent files:");
		update_obj_file();
		mlog("   Done.");
	}

	for (i = 0; i <= top_of_zone_table; i++) {
		mlog("Resetting #%d: %s (rooms %d-%d).", zone_table[i].number,
				zone_table[i].name, zone_table[i].bot, zone_table[i].top);
		reset_zone(i);
	}

	reset_q.head = reset_q.tail = NULL;

	if (!fCopyOver)
		boot_time = time(0);
	else
		copyover_time = time(0);

	mlog("Boot db -- DONE.");
}


/* reset the time in the game from file */
void reset_time(void)
{
#if	defined(CIRCLE_MACINTOSH)
	long beginning_of_time = -1561789232;
#else
	long beginning_of_time = 650336715;
#endif

	time_info = *mud_time_passed(time(0), beginning_of_time);

	read_mud_date_from_file();

	if (time_info.hours <= 4)
		weather_info.sunlight = SUN_DARK;
	else if (time_info.hours == 5)
		weather_info.sunlight = SUN_RISE;
	else if (time_info.hours <= 20)
		weather_info.sunlight = SUN_LIGHT;
	else if (time_info.hours == 21)
		weather_info.sunlight = SUN_SET;
	else
		weather_info.sunlight = SUN_DARK;

	mlog("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
					time_info.day, time_info.month, time_info.year);

	weather_info.pressure = 960;
	if ((time_info.month >= 7) && (time_info.month <= 12))
		weather_info.pressure += dice(1, 50);
	else
		weather_info.pressure += dice(1, 80);

	weather_info.change = 0;

	if (weather_info.pressure <= 980)
		weather_info.sky = SKY_LIGHTNING;
	else if (weather_info.pressure <= 1000)
		weather_info.sky = SKY_RAINING;
	else if (weather_info.pressure <= 1020)
		weather_info.sky = SKY_CLOUDY;
	else
		weather_info.sky = SKY_CLOUDLESS;
}


void free_player_index(void)
{
	int tp;

	if (!player_table)
		return;

	for (tp = 0; tp <= top_of_p_table; tp++)
		if (player_table[tp].name)
			free(player_table[tp].name);

	free(player_table);
	player_table = NULL;
	top_of_p_table = 0;
}


/* generate index table for players */
/* New version of build_player_index for MySQL by Torgny Bjers, 2001-09-08. */
void build_player_index(void)
{
	MYSQL_RES *playerindex;
	MYSQL_ROW row;
	int rec_count = 0, i;
	unsigned long *fieldlength;

	if (!(playerindex = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT ID, LCASE(Name), Perms, Preferences, Last, Active FROM %s ORDER BY ID ASC;", TABLE_PLAYER_INDEX))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not load player index from table [%s].", TABLE_PLAYER_INDEX);
		kill_mysql();
		exit(1);
	}

	rec_count = mysql_num_rows(playerindex);
	mlog("   %d players in database.", rec_count);

	if(rec_count == 0) {
		player_table = NULL;
		top_of_p_file = top_of_p_table = -1;
		return;
	}

	CREATE(player_table, struct player_index_element, rec_count);
	for(i = 0; i < rec_count; i++)
	{
		row = mysql_fetch_row(playerindex);
		fieldlength = mysql_fetch_lengths(playerindex);
		player_table[i].id = atoi(row[0]);
		player_table[i].rights = asciiflag_conv(row[2]);
		player_table[i].last = atoi(row[4]);
		player_table[i].active = atoi(row[5]);
		CREATE(player_table[i].name, char, fieldlength[1] + 1);
		strcpy(player_table[i].name, row[1]);
		player_table[i].flags = asciiflag_conv(row[3]);
		top_idnum = MAX(top_idnum, player_table[i].id);
	}

	top_of_p_file = top_of_p_table = i - 1;

	/* Free up result sets to conserve memory. */
	mysql_free_result(playerindex);
}


/* New version of index_boot for MySQL by Torgny Bjers, 2001-09-14. */
void index_boot(int mode)
{
	MYSQL_RES *index, *content;
	char *table = NULL;	/* NULL or egcs 1.1 complains */
	int rec_count = 0, size[2];
	const char *modes[] = {"WLD", "MOB", "OBJ", "ZON", "SHP", "HLP", "TRG", "QST"};
	bool error = FALSE;

	switch (mode) {
	case DB_BOOT_TRG:
		table = TABLE_TRG_INDEX;
		break;
	case DB_BOOT_WLD:
		table = TABLE_WLD_INDEX;
		break;
	case DB_BOOT_MOB:
		table = TABLE_MOB_INDEX;
		break;
	case DB_BOOT_OBJ:
		table = TABLE_OBJ_INDEX;
		break;
	case DB_BOOT_ZON:
		table = TABLE_ZON_INDEX;
		break;
	case DB_BOOT_SHP:
		table = TABLE_SHP_INDEX;
		break;
	case DB_BOOT_HLP:
		table = TABLE_HLP_INDEX;
		break;
	case DB_BOOT_QST:
		table = TABLE_QST_INDEX;
		break;
	case DB_BOOT_GLD:
		table = TABLE_GUILD_INDEX;
		break;
	case DB_BOOT_HOU:
		table = TABLE_HOUSE_INDEX;
		break;
	default:
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Unknown subcommand %d to index_boot!", mode);
		kill_mysql();
		exit(1);
	}

	/*
	 * Load different zone indexes depending on what mode we are in.
	 * The zones that should always load no matter what should be
	 * set to mini = 1 in the zone index table. Torgny Bjers, 2001-09-19
	 */
	if (mini_mud) {
		if (!(index = mysqlGetResource(TABLE_BOOT_INDEX, "SELECT * FROM %s WHERE mini = 1 ORDER BY znum ASC;", TABLE_BOOT_INDEX)))
			error = TRUE;
	} else {
		if (!(index = mysqlGetResource(TABLE_BOOT_INDEX, "SELECT * FROM %s ORDER BY znum ASC;", TABLE_BOOT_INDEX)))
			error = TRUE;
	}

	if (error) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error opening %s index (%s).", modes[mode], table);
		kill_mysql();
		exit(1);
	}

	/*
	 * For table compability we need to use different ORDER BY clauses
	 * since the zone and help table does not have vnum. Torgny Bjers, 2001-09-19
	 */
	switch (mode) {
		case DB_BOOT_GLD:
			/* A linked list is last in first out, need to reverse order here */
			if (!(content = mysqlGetResource(table, "SELECT * FROM %s ORDER BY name DESC;", table)))
				error = TRUE;
			break;
		case DB_BOOT_HLP:
			if (!(content = mysqlGetResource(table, "SELECT * FROM %s ORDER BY keyword ASC;", table)))
				error = TRUE;
			break;
		case DB_BOOT_ZON:
			if (!(content = mysqlGetResource(table, "SELECT OI.* FROM %s OI, %s BI WHERE BI.znum = OI.znum%s ORDER BY znum ASC;", table, TABLE_BOOT_INDEX, mini_mud ? " AND mini = 1" : "")))
				error = TRUE;
			break;
		case DB_BOOT_HOU:
			if (!(content = mysqlGetResource(table, "SELECT * FROM %s ORDER BY vnum ASC;", table)))
				error = TRUE;
			break;
		default:
			if (!(content = mysqlGetResource(table, "SELECT OI.* FROM %s OI, %s BI WHERE BI.znum = OI.znum%s ORDER BY znum, vnum ASC;", table, TABLE_BOOT_INDEX, mini_mud ? " AND mini = 1" : "")))
				error = TRUE;
			break;
	}

	if (error) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error opening %s content (%s).", modes[mode], table);
		kill_mysql();
		exit(1);
	}

	rec_count = mysql_num_rows(content);

	/* Exit if 0 records, unless this is houses, shops, or guilds */
	if (!rec_count) {
		if (mode == DB_BOOT_HOU || mode == DB_BOOT_SHP || mode == DB_BOOT_GLD || mode == DB_BOOT_TRG || mode == DB_BOOT_QST)
			return;
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "boot error - 0 records counted in %s.", table);
		kill_mysql();
		exit(1);
	}

  /*
   * Any idea why you put this here Jeremy?
   * This CREATE's the lists one item larger than the actual number of rows
   * in the queries.  Perhaps it has to do with how we search for items?
   * Perhaps if it was left untouched we would not be able to find the last
   * item in the lists?
   */
  rec_count++;

	/* NOTE: "bytes" does NOT include strings or other later malloc'd things. */
	switch (mode) {
	case DB_BOOT_TRG:
		CREATE(trig_index, struct index_data *, rec_count);
		size[0] = sizeof(struct index_data) * rec_count;
		mlog("   %d trigger%s, %d bytes.", rec_count - 1, rec_count - 1 != 1 ? "s" : "", size[0]);
		break;
	case DB_BOOT_WLD:
		CREATE(world, struct room_data, rec_count);
		size[0] = sizeof(struct room_data) * rec_count;
		mlog("   %d room%s, %d bytes.", rec_count - 1, rec_count - 1 != 1 ? "s" : "", size[0]);
		break;
	case DB_BOOT_MOB:
		CREATE(mob_proto, struct char_data, rec_count);
		CREATE(mob_index, struct index_data, rec_count);
		size[0] = sizeof(struct index_data) * rec_count;
		size[1] = sizeof(struct char_data) * rec_count;
		mlog("   %d mob%s, %d bytes in index, %d bytes in prototypes.", rec_count - 1, rec_count - 1 != 1 ? "s" : "", size[0], size[1]);
		break;
	case DB_BOOT_OBJ:
		CREATE(obj_proto, struct obj_data, rec_count);
		CREATE(obj_index, struct index_data, rec_count);
		size[0] = sizeof(struct index_data) * rec_count;
		size[1] = sizeof(struct obj_data) * rec_count;
		mlog("   %d obj%s, %d bytes in index, %d bytes in prototypes.", rec_count - 1, rec_count - 1 != 1 ? "s" : "", size[0], size[1]);
		break;
	case DB_BOOT_ZON:
		CREATE(zone_table, struct zone_data, rec_count);
		size[0] = sizeof(struct zone_data) * rec_count;
		mlog("   %d zone%s, %d bytes.", rec_count - 1, rec_count - 1 != 1 ? "s" : "", size[0]);
		break;
	case DB_BOOT_HLP:
		CREATE(help_table, struct help_index_element, rec_count);
		size[0] = sizeof(struct help_index_element) * rec_count;
		mlog("   %d entrie%s, %d bytes.", rec_count - 1, rec_count - 1 != 1 ? "s" : "", size[0]);
		break;
	case DB_BOOT_QST:
		CREATE(aquest_table, struct aq_data, rec_count);
		size[0] = sizeof(struct aq_data) * rec_count;
		mlog("   %d quest%s, %d bytes.", rec_count - 1, rec_count - 1 != 1 ? "s" : "", size[0]);
		break;
	case DB_BOOT_SHP:
		CREATE(shop_index, struct shop_data, rec_count);
		size[0] = sizeof(struct shop_data) * rec_count;
		mlog("   %d shop%s, %d bytes.", rec_count - 1, rec_count - 1 != 1 ? "s" : "", size[0]);
		break;
	case DB_BOOT_GLD:
		mlog("   %d guild%s in database.", rec_count - 1, rec_count - 1 != 1 ? "s" : "");
		break;
	case DB_BOOT_HOU:
		CREATE(house_index, struct house_data, rec_count);
		size[0] = sizeof(struct house_data) * rec_count;
		mlog("   %d house%s, %d bytes.", rec_count - 1, rec_count - 1 != 1 ? "s" : "", size[0]);
		break;
	}

	/*
	 * Added extra switch in order to simplify queries for extra descriptions,
	 * exits, affects, and triggers.  The server executed approximately 3 queries
	 * per vnum with the previous versions of index_boot and discrete_query,
	 * which meant about 300 queries per zone.  Ridiculous.  Sending several
	 * thousand small queries to a MySQL server may work for me, but will likely
	 * make any slower server fall asleep.  Torgny Bjers (2001-10-31, 10:13:21)
	 */

	switch (mode) {
		case DB_BOOT_ZON:
		case DB_BOOT_TRG:
		case DB_BOOT_QST:
		case DB_BOOT_HLP:
		case DB_BOOT_SHP:
			common_index_boot(index, content, mode);	/* Calls discrete_query() */
			break;
		case DB_BOOT_WLD:
			world_index_boot(index, content);					/* Calls world_boot_query() which in turn calls parse_room */
			break;
		case DB_BOOT_OBJ:
			object_index_boot(index, content);				/* Calls object_boot_query() which in turn calls parse_object */
			break;
		case DB_BOOT_MOB:
			mobile_index_boot(index, content);				/* Calls mobile_boot_query() which in turn calls parse_mobile */
			break;
		case DB_BOOT_GLD:
			boot_guilds(content);											/* Calls boot_guilds() */
			break;
		case DB_BOOT_HOU:
			boot_houses(content);											/* Calls boot_houses() */
			break;
	}

	/* Free up result sets to conserve memory. */
	mysql_free_result(index);
	mysql_free_result(content);

}


void common_index_boot(MYSQL_RES *index, MYSQL_RES *content, int mode)
{
	MYSQL_ROW index_row;
	int znum = 0;

	/* Read through the index result set from query */
	while ((index_row = mysql_fetch_row(index)))
	{
		znum = atoi(index_row[0]);
		switch (mode) {
			case DB_BOOT_ZON:
				load_zones(content, znum);
				break;
			case DB_BOOT_TRG:
			case DB_BOOT_QST:
			case DB_BOOT_SHP:
				discrete_query(content, mode, znum);
				break;
		}
	}
}


void boot_houses(MYSQL_RES *content)
{
	MYSQL_ROW content_row;
	unsigned long *fieldlength;

	/* Read through the index result set from query */
	while ((content_row = mysql_fetch_row(content)))
	{
		fieldlength = mysql_fetch_lengths(content);
		parse_house(content_row, fieldlength, atoi(content_row[0]));
	}
}


/* New version of discrete_load for MySQL by Torgny Bjers, 2001-09-14. */
void discrete_query(MYSQL_RES *content, int mode, int znum)
{
	MYSQL_ROW content_row;
	int zone_found = 0;
	unsigned long *fieldlength;
	const char *modes[] = {"WLD", "MOB", "OBJ", "ZON", "SHP", "HLP", "TRG", "QST"};

	/* Seek to first row, since we are looping this set more than once. */
	mysql_data_seek(content, 0);

	while ((content_row = mysql_fetch_row(content)))
	{
		if (atoi(content_row[1]) >= 99999) {
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Error in zone number for %s with znum #%d.", modes[mode], znum);
			return;
		} else {
			fieldlength = mysql_fetch_lengths(content);
			if (atoi(content_row[0]) == znum) {
				zone_found = 1;
				switch (mode) {
					case DB_BOOT_TRG:
						parse_trigger(content_row, fieldlength, atoi(content_row[1]));
						break;
					case DB_BOOT_QST:
						parse_quest(content_row, fieldlength, atoi(content_row[1]));
						break;
					case DB_BOOT_SHP:
						parse_shop(content_row, fieldlength, atoi(content_row[1]));
						break;
				}
			}
		}
	}

	/*if (zone_found == 0) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "(%s)%s:%d: Requested zone (#%d) not found in %s index.", __FILE__, __FUNCTION__, __LINE__, znum, modes[mode]);
		return;
	}*/

}


void world_index_boot(MYSQL_RES *index, MYSQL_RES *content)
{
	MYSQL_RES *extradescs, *exits, *triggers;
	MYSQL_ROW index_row;
	int znum = 0;
	bool error = FALSE;

	/* open MySQL table for extra descriptions */
	if (!(extradescs = mysqlGetResource(TABLE_WLD_EXTRADESCS, "SELECT * FROM %s ORDER BY znum, vnum ASC;", TABLE_WLD_EXTRADESCS)))
		error = TRUE;
	/* open MySQL table for room exits */
	if (!(exits = mysqlGetResource(TABLE_WLD_EXITS, "SELECT * FROM %s ORDER BY znum, vnum, dir ASC;", TABLE_WLD_EXITS)))
		error = TRUE;
	/* open MySQL table for room triggers */
	if (!(triggers = mysqlGetResource(TABLE_TRG_ASSIGNS, "SELECT * FROM %s WHERE type = %d ORDER BY znum, vnum, id ASC;", TABLE_TRG_ASSIGNS, WLD_TRIGGER)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	/* Read through the index result set from query */
	while ((index_row = mysql_fetch_row(index)))
	{
		znum = atoi(index_row[0]);
		world_boot_query(content, extradescs, exits, triggers, znum);
	}

	mysql_free_result(extradescs);
	mysql_free_result(exits);
	mysql_free_result(triggers);

}


void world_boot_query(MYSQL_RES *content, MYSQL_RES *extradescs, MYSQL_RES *exits, MYSQL_RES *triggers, int znum)
{
	MYSQL_ROW content_row;
	int zone_found = 0;

	/* Seek to first row, since we are looping this set more than once. */
	mysql_data_seek(content, 0);

	while ((content_row = mysql_fetch_row(content)))
	{
		if (atoi(content_row[1]) >= 99999) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error in zone number for WLD with znum #%d.", znum);
			return;
		} else {
			if (atoi(content_row[0]) == znum) {
				zone_found = 1;
				parse_room(content_row, extradescs, exits, triggers, atoi(content_row[1]));
			}
		}
	}

	/*if (zone_found == 0) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "(%s)%s:%d: Requested zone (#%d) not found in %s index.", __FILE__, __FUNCTION__, __LINE__, znum, modes[mode]);
		return;
	}*/

}


void object_index_boot(MYSQL_RES *index, MYSQL_RES *content)
{
	MYSQL_RES *extradescs, *affects, *triggers;
	MYSQL_ROW index_row;
	int znum = 0;
	bool error = FALSE;

	/* open MySQL table for extra descriptions */
	if (!(extradescs = mysqlGetResource(TABLE_OBJ_EXTRADESCS, "SELECT * FROM %s ORDER BY znum, vnum ASC;", TABLE_OBJ_EXTRADESCS)))
		error = TRUE;
	/* open MySQL table for object affects */
	if (!(affects = mysqlGetResource(TABLE_OBJ_AFFECTS, "SELECT * FROM %s ORDER BY id;", TABLE_OBJ_AFFECTS)))
		error = TRUE;
	/* open MySQL table for object triggers */
	if (!(triggers = mysqlGetResource(TABLE_TRG_ASSIGNS, "SELECT * FROM %s WHERE type = %d ORDER BY znum, vnum, id ASC;", TABLE_TRG_ASSIGNS, OBJ_TRIGGER)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	/* Read through the index result set from query */
	while ((index_row = mysql_fetch_row(index)))
	{
		znum = atoi(index_row[0]);
		object_boot_query(content, extradescs, affects, triggers, znum);
	}

	mysql_free_result(extradescs);
	mysql_free_result(affects);
	mysql_free_result(triggers);

}


void object_boot_query(MYSQL_RES *content, MYSQL_RES *extradescs, MYSQL_RES *affects, MYSQL_RES *triggers, int znum)
{
	MYSQL_ROW content_row;
	int zone_found = 0;

	/* Seek to first row, since we are looping this set more than once. */
	mysql_data_seek(content, 0);

	while ((content_row = mysql_fetch_row(content)))
	{
		if (atoi(content_row[1]) >= 99999) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error in zone number for OBJ with znum #%d.", znum);
			return;
		} else {
			if (atoi(content_row[0]) == znum) {
				zone_found = 1;
				parse_object(content_row, extradescs, affects, triggers, atoi(content_row[1]));
			}
		}
	}

	/*if (zone_found == 0) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "(%s)%s:%d: Requested zone (#%d) not found in %s index.", __FILE__, __FUNCTION__, __LINE__, znum, modes[mode]);
		return;
	}*/

}


void mobile_index_boot(MYSQL_RES *index, MYSQL_RES *content)
{
	MYSQL_RES *triggers;
	MYSQL_ROW index_row;
	int znum = 0;
	bool error = FALSE;

	/* open MySQL table for mobile triggers */
	if (!(triggers = mysqlGetResource(TABLE_TRG_ASSIGNS, "SELECT * FROM %s WHERE type = %d ORDER BY znum, vnum, id ASC;", TABLE_TRG_ASSIGNS, MOB_TRIGGER)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	/* Read through the index result set from query */
	while ((index_row = mysql_fetch_row(index)))
	{
		znum = atoi(index_row[0]);
		mobile_boot_query(content, triggers, znum);
	}

	mysql_free_result(triggers);
}


void mobile_boot_query(MYSQL_RES *content, MYSQL_RES *triggers, int znum)
{
	MYSQL_ROW content_row;
	int zone_found = 0;

	/* Seek to first row, since we are looping this set more than once. */
	mysql_data_seek(content, 0);

	while ((content_row = mysql_fetch_row(content)))
	{
		if (atoi(content_row[1]) >= 99999) {
			mlog("Error in zone number for MOB with znum #%d.", znum);
			return;
		} else {
			if (atoi(content_row[0]) == znum) {
				zone_found = 1;
				parse_mobile(content_row, triggers, atoi(content_row[1]));
			}
		}
	}

	/*if (zone_found == 0) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "(%s)%s:%d: Requested zone (#%d) not found in %s index.", __FILE__, __FUNCTION__, __LINE__, znum, modes[mode]);
		return;
	}*/

}


bitvector_t	asciiflag_conv(char *flag)
{
	bitvector_t flags = 0;
	int is_number = 1;
	char *p;

	for (p = flag; *p; p++) {
		if (islower(*p))
			flags |= 1ULL << (*p - 'a');
		else if (isupper(*p))
			flags |= 1ULL << (26 + (*p - 'A'));

		if (!isdigit(*p))
			is_number = 0;
	}

	if (is_number)
		flags = atoll(flag);

	return (flags);
}


char fread_letter(FILE *fp)
{
	char c;
	do {
		c = getc(fp);  
	} while (isspace(c));
	return c;
}


/* New version of parse_room for MySQL by Torgny Bjers, 2001-09-14. */
void parse_room(MYSQL_ROW world_row, MYSQL_RES *extradescs, MYSQL_RES *exits, MYSQL_RES *triggers, int vnum)
{
	MYSQL_ROW extradescs_row, exits_row, triggers_row;
	struct extra_descr_data *new_descr;
	static int room_nr = 0, zone = 0;
	int i, spec;
	unsigned long *fieldlength;
	char *flags = get_buffer(32);

	if (vnum < zone_table[zone].bot) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Room #%d is below zone %d.", vnum, zone);
		kill_mysql();
		exit(1);
	}
	while (vnum > zone_table[zone].top) {
		if (++zone > top_of_zone_table) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Room %d is outside of any zone.", vnum);
			kill_mysql();
			exit(1);
		}
	}

	world[room_nr].zone = zone;
	world[room_nr].number = vnum;
	world[room_nr].name = str_dup(world_row[2]);
	world[room_nr].description = str_dup(world_row[3]); /* StringAllocFix */

	world[room_nr].room_flags = asciiflag_conv(world_row[4]);
	sprintf(flags, "object #%d", vnum);
	check_bitvector_names(world[room_nr].room_flags, room_bits_count, flags, "room");

	release_buffer(flags);

	world[room_nr].sector_type = atoi(world_row[5]);
	world[room_nr].magic_flux = atoi(world_row[7]);

	world[room_nr].func = NULL;
	world[room_nr].contents = NULL;
	world[room_nr].people = NULL;
	world[room_nr].light = 0; /* Zero light sources */

	for (i = 0; i < NUM_OF_DIRS; i++)
		world[room_nr].dir_option[i] = NULL;

	world[room_nr].ex_description = NULL;

	spec = atoi(world_row[6]);

	if (spec >= 0 && spec <= NUM_ROOM_SPECS)
		world[room_nr].specproc = spec;

	/* Assign the resources */
	GET_ROOM_RESOURCES(room_nr) = atoi(world_row[8]);
	GET_ROOM_RESOURCE_MAX(room_nr, RESOURCE_ORE) = atoi(world_row[9]);
	GET_ROOM_RESOURCE_MAX(room_nr, RESOURCE_GEMS) = atoi(world_row[10]);
	GET_ROOM_RESOURCE_MAX(room_nr, RESOURCE_WOOD) = atoi(world_row[11]);
	GET_ROOM_RESOURCE_MAX(room_nr, RESOURCE_STONE) = atoi(world_row[12]);
	GET_ROOM_RESOURCE_MAX(room_nr, RESOURCE_FABRIC) = atoi(world_row[13]);
	/* We only save max values, so both values need to be the same at load. */
	GET_ROOM_RESOURCE(room_nr, RESOURCE_ORE) = atoi(world_row[9]);
	GET_ROOM_RESOURCE(room_nr, RESOURCE_GEMS) = atoi(world_row[10]);
	GET_ROOM_RESOURCE(room_nr, RESOURCE_WOOD) = atoi(world_row[11]);
	GET_ROOM_RESOURCE(room_nr, RESOURCE_STONE) = atoi(world_row[12]);
	GET_ROOM_RESOURCE(room_nr, RESOURCE_FABRIC) = atoi(world_row[13]);

	/* Seek to first row, since we are looping this set more than once. */
	mysql_data_seek(extradescs, 0);

	/* While there are rows in the extradescs result set, create descs. */
	while ((extradescs_row = mysql_fetch_row(extradescs)))
	{
		fieldlength = mysql_fetch_lengths(extradescs);
		if (atoi(extradescs_row[2]) == vnum) {
			CREATE(new_descr, struct extra_descr_data, 1);
			new_descr->keyword = str_dup(extradescs_row[3]);
			new_descr->description = str_dup(extradescs_row[4]); /* StringAllocFix */
			new_descr->next = world[room_nr].ex_description;
			world[room_nr].ex_description = new_descr;
		}
	}

	/* Seek to first row, since we are looping this set more than once. */
	mysql_data_seek(exits, 0);

	/* While there are rows in the exits result set, create exits. */
	while ((exits_row = mysql_fetch_row(exits)))
	{
		fieldlength = mysql_fetch_lengths(exits);
		if (atoi(exits_row[1]) == vnum)
			setup_dir(exits_row, fieldlength, room_nr);
	}

	/* Seek to first row, since we are looping this set more than once. */
	mysql_data_seek(triggers, 0);

	while ((triggers_row = mysql_fetch_row(triggers))) {
		if (atoi(triggers_row[2]) == vnum)
			dg_read_trigger(triggers_row, &world[room_nr], WLD_TRIGGER);
	}

	top_of_world = room_nr++;

}


/* New version of setup_dir for MySQL by Torgny Bjers, 2001-09-14. */
/* read direction data */
void setup_dir(MYSQL_ROW exits_row, unsigned long *fieldlength, int room)
{
	int dir = -1, flag = 0;

	dir = atoi(exits_row[3]);
	flag = atoi(exits_row[6]);

	if (dir < 0)
		return;

	CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
	world[room].dir_option[dir]->general_description = NULL_STR(fieldlength[5], exits_row[4]);
	world[room].dir_option[dir]->keyword = NULL_STR(fieldlength[5], exits_row[5]);

	switch (flag) {
		case 1: world[room].dir_option[dir]->exit_info = EX_ISDOOR; break;
		case 2: world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF; break;
		case 3: world[room].dir_option[dir]->exit_info = EX_FAERIE; break;
		case 4: world[room].dir_option[dir]->exit_info = EX_INFERNAL; break;
		case 5: world[room].dir_option[dir]->exit_info = EX_DIVINE; break;
		case 6: world[room].dir_option[dir]->exit_info = EX_ANCIENT; break;
		default: world[room].dir_option[dir]->exit_info = 0; break;
	}

	world[room].dir_option[dir]->key = atoi(exits_row[7]);
	world[room].dir_option[dir]->to_room = atoi(exits_row[8]);
}


/* hometowns: make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
	int count;

	for (count = 1; count <= (NUM_STARTROOMS + 1); count++)
	if ((r_mortal_start_room[count] = real_room(mortal_start_room[count])) < 0) {
		 if (count > 1)
			 r_mortal_start_room[count] = real_room(mortal_start_room[start_zone_index]);
		 else {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, " Mortal start room does not exist.  Change in config.c.");
				kill_mysql();
				exit(1);
		 }
	}
	if ((r_immort_start_room = real_room(immort_start_room)) < 0) {
		if (!mini_mud)
			extended_mudlog(BRF, SYSL_BUGS, TRUE, " Warning: Immort start room does not exist.  Change in config.c.");
		r_immort_start_room = r_mortal_start_room[start_zone_index];
	}
	if ((r_frozen_start_room = real_room(frozen_start_room)) < 0) {
		if (!mini_mud)
			extended_mudlog(BRF, SYSL_BUGS, TRUE, " Warning: Frozen start room does not exist.  Change in config.c.");
		r_frozen_start_room = r_mortal_start_room[start_zone_index];
	}
	if ((r_new_start_room = real_room(new_start_room)) < 0) {
		if (!mini_mud)
			extended_mudlog(BRF, SYSL_BUGS, TRUE, " Warning: New player start room does not exist.  Change in config.c.");
		r_new_start_room = r_mortal_start_room[start_zone_index];
	}
}


/* resolve all vnums into rnums in the world */
void renum_world(void)
{
	int room, door;

	for (room = 0; room <= top_of_world; room++)
		for (door = 0; door < NUM_OF_DIRS; door++)
			if (world[room].dir_option[door])
				if (world[room].dir_option[door]->to_room != NOWHERE)
					world[room].dir_option[door]->to_room =
						real_room(world[room].dir_option[door]->to_room);
}


#define	ZCMD zone_table[zone].cmd[cmd_no]

/*
 * "resolve vnums into rnums in the zone reset tables"
 *
 * Or in English: Once all of the zone reset tables have been loaded, we
 * resolve the virtual numbers into real numbers all at once so we don't have
 * to do it repeatedly while the game is running.  This does make adding any
 * room, mobile, or object a little more difficult while the game is running.
 *
 * NOTE 1: Assumes NOWHERE == NOBODY == NOTHING.
 * NOTE 2: Assumes sizeof(room_rnum) >= (sizeof(mob_rnum) and sizeof(obj_rnum))
 */
void renum_zone_table(void)
{
	int cmd_no;
	room_rnum a, b, c, olda, oldb, oldc;
	zone_rnum zone;
	char buf[128];

	for (zone = 0; zone <= top_of_zone_table; zone++)
		for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
			a = b = c = 0;
			olda = ZCMD.arg1;
			oldb = ZCMD.arg2;
			oldc = ZCMD.arg3;
			switch (ZCMD.command) {
			case 'M':
				a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
				c = ZCMD.arg3 = real_room(ZCMD.arg3);
				break;
			case 'O':
				a = ZCMD.arg1 = real_object(ZCMD.arg1);
				if (ZCMD.arg3 != NOWHERE)
					c = ZCMD.arg3 = real_room(ZCMD.arg3);
				break;
			case 'G':
				a = ZCMD.arg1 = real_object(ZCMD.arg1);
				break;
			case 'E':
				a = ZCMD.arg1 = real_object(ZCMD.arg1);
				break;
			case 'P':
				a = ZCMD.arg1 = real_object(ZCMD.arg1);
				c = ZCMD.arg3 = real_object(ZCMD.arg3);
				break;
			case 'D':
				a = ZCMD.arg1 = real_room(ZCMD.arg1);
				break;
			case 'R': /* rem obj from room */
				a = ZCMD.arg1 = real_room(ZCMD.arg1);
				b = ZCMD.arg2 = real_object(ZCMD.arg2);
				break;
			case 'T': /* a trigger */
				b = ZCMD.arg2 = real_trigger(ZCMD.arg2);
				c = ZCMD.arg3 = real_room(ZCMD.arg3);
				break;
			case 'V': /* trigger variable assignment */
				c = ZCMD.arg3 = real_room(ZCMD.arg3);
				break;
			}
			if (a == NOWHERE || b == NOWHERE || c == NOWHERE) {
				if (!mini_mud) {
					sprintf(buf,  "Invalid vnum %d, cmd disabled",
												 a == NOWHERE ? olda : b == NOWHERE ? oldb : oldc);
					log_zone_error(zone, cmd_no, ZCMD.id, buf);
				}
				ZCMD.command = '*';
			}
		}
}

#define CASE(test)	\
	if (value && !matched && !str_cmp(keyword, test) && (matched = TRUE))

#define BOOL_CASE(test)	\
	if (!value && !matched && !str_cmp(keyword, test) && (matched = TRUE))
 
#define RANGE(low, high)	\
	(num_arg = MAX((low), MIN((high), (num_arg))))

/*
 * New version of parse_mobile for MySQL by Torgny Bjers, 2001-09-15.
 * This new version incorporates both simple and enhanced mobiles, since
 * the database always looks the same.
 */
void parse_mobile(MYSQL_ROW mobile_row, MYSQL_RES *triggers, int vnum)
{
	MYSQL_ROW triggers_row;
	static int i = 0;
	int j;
	int spec;
	int race;
	ubyte difficulty;
	char *tmpptr, tmpdesc[1024];

	mob_index[i].vnum = vnum;
	mob_index[i].number = 0;
	mob_index[i].func = NULL;

	clear_char(mob_proto + i);

	/*
	 * Mobiles should NEVER use anything in the 'player_specials' structure.
	 * The only reason we have every mob in the game share this copy of the
	 * structure is to save newbie coders from themselves. -gg 2/25/98
	 */
	mob_proto[i].player_specials = &dummy_mob;
	sprintf(buf2, "mob vnum %d", vnum);

	/***** String data *****/
	mob_proto[i].player.name = str_dup(mobile_row[2]);
	tmpptr = mob_proto[i].player.short_descr = str_dup(mobile_row[3]);
	if (tmpptr && *tmpptr)
		if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
				!str_cmp(fname(tmpptr), "the"))
			*tmpptr = LOWER(*tmpptr);
	mob_proto[i].player.description = str_dup(mobile_row[5]); /* StringAllocFix */
	mob_proto[i].player.long_descr = str_dup(mobile_row[4]);
	// Check the L-Desc for finishing \r\n
	if (mob_proto[i].player.long_descr) {
		int len = strlen(mob_proto[i].player.long_descr);
		bool desc_ok = FALSE;
		for (j = len; j > (len - 3); j--)
			if (mob_proto[i].player.long_descr[j] == '\n' || mob_proto[i].player.long_descr[j] == '\r')
				desc_ok = TRUE;
		if (!desc_ok) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Mob #%d has an L-Desc without linebreaks.", vnum);
			sprintf(tmpdesc, "%s\r\n", mob_proto[i].player.long_descr);
			free(mob_proto[i].player.long_descr);
			mob_proto[i].player.long_descr = str_dup(tmpdesc);
		}
	}

	GET_TITLE(mob_proto + i) = NULL;

	for (j = 0; j < NUM_WEARS; j++)
		mob_proto[i].equipment[j] = NULL;

	MOB_FLAGS(mob_proto + i) = asciiflag_conv(mobile_row[6]);
	SET_BIT(MOB_FLAGS(mob_proto + i), MOB_ISNPC);

	if (MOB_FLAGGED(mob_proto + i, MOB_NOTDEADYET)) {
		/* Rather bad to load mobiles with this bit already set. */
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Mob #%d has reserved bit MOB_NOTDEADYET set.", vnum);
		REMOVE_BIT(MOB_FLAGS(mob_proto + i), MOB_NOTDEADYET);
	}

	check_bitvector_names(MOB_FLAGS(mob_proto + i), action_bits_count, buf2, "mobile");

	AFF_FLAGS(mob_proto + i) = asciiflag_conv(mobile_row[7]);
	check_bitvector_names(AFF_FLAGS(mob_proto + i), affected_bits_count, buf2, "mobile affect");
 
	GET_ALIGNMENT(mob_proto + i) = atoi(mobile_row[8]);

	/* AGGR_TO_ALIGN is ignored if the mob is AGGRESSIVE. */
	if (MOB_FLAGGED(mob_proto + i, MOB_AGGRESSIVE) && MOB_FLAGGED(mob_proto + i, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_FAE | MOB_AGGR_DIVINE | MOB_AGGR_INFERNAL | MOB_AGGR_HUMAN))
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Mob #%d both Aggressive and Aggressive_to_Alignment.", vnum);

	difficulty = atoi(mobile_row[9]);
	if (difficulty > NUM_MOB_DIFFICULTIES-1)
		GET_DIFFICULTY(mob_proto + i) = LIMIT((difficulty / 20), 0, NUM_MOB_DIFFICULTIES-1);
	else
		GET_DIFFICULTY(mob_proto + i) = LIMIT(difficulty, 0, NUM_MOB_DIFFICULTIES-1);

	GET_HITROLL(mob_proto + i) = 20 - atoi(mobile_row[10]);
	GET_PD(mob_proto + i) = atoi(mobile_row[11]);

	/* max hit = 0 is a flag that H, M, V is xdy+z */
	GET_MAX_HIT(mob_proto + i) = 0;
	GET_HIT(mob_proto + i) = atoi(mobile_row[12]);
	GET_MANA(mob_proto + i) = atoi(mobile_row[13]);
	GET_MOVE(mob_proto + i) = atoi(mobile_row[14]);

	GET_MAX_MANA(mob_proto + i) = 150;
	GET_MAX_MOVE(mob_proto + i) = 50;

	GET_SKILLCAP(mob_proto + i) = 80000;

	mob_proto[i].mob_specials.damnodice = atoi(mobile_row[15]);
	mob_proto[i].mob_specials.damsizedice = atoi(mobile_row[16]);
	GET_DAMROLL(mob_proto + i) = atoi(mobile_row[17]);

	GET_GOLD(mob_proto + i) = atoi(mobile_row[18]);

	GET_POS(mob_proto + i) = atoi(mobile_row[19]);
	GET_DEFAULT_POS(mob_proto + i) = atoi(mobile_row[20]);
	GET_SEX(mob_proto + i) = atoi(mobile_row[21]);

	mob_proto[i].mob_specials.attack_type = LIMIT(atoi(mobile_row[22]), 0, 99);

	GET_STRENGTH(mob_proto + 1) = LIMIT(atoi(mobile_row[23]), 300, 10000);
	GET_AGILITY(mob_proto + 1) = LIMIT(atoi(mobile_row[25]), 300, 10000);
	GET_PRECISION(mob_proto + 1) = LIMIT(atoi(mobile_row[26]), 300, 10000);
	GET_PERCEPTION(mob_proto + 1) = LIMIT(atoi(mobile_row[27]), 300, 10000);
	GET_HEALTH(mob_proto + 1) = LIMIT(atoi(mobile_row[28]), 300, 32000);
	GET_WILLPOWER(mob_proto + 1) = LIMIT(atoi(mobile_row[29]), 300, 10000);
	GET_INTELLIGENCE(mob_proto + 1) = LIMIT(atoi(mobile_row[30]), 300, 10000);
	GET_CHARISMA(mob_proto + 1) = LIMIT(atoi(mobile_row[31]), 300, 10000);
	GET_LUCK(mob_proto + 1) = LIMIT(atoi(mobile_row[32]), 300, 10000);
	GET_ESSENCE(mob_proto + 1) = LIMIT(atoi(mobile_row[33]), 300, 10000);

	race = LIMIT(atoi(mobile_row[35]), 0, top_of_race_list);

	GET_RACE(mob_proto + 1) = race_frames[race].race;
	GET_BODY(mob_proto + 1) = race_frames[race].body_bits;
	GET_SIZE(mob_proto + 1) = race_list[race].size;

	GET_CULTURE(mob_proto + 1) = CULTURE_UNDEFINED;

	GET_CLASS(mob_proto + i) = 0;
	GET_WEIGHT(mob_proto + i) = 200;
	GET_HEIGHT(mob_proto + i) = 198;

	GET_MOB_VAL(mob_proto + i, 0) = atoi(mobile_row[36]);
	GET_MOB_VAL(mob_proto + i, 1) = atoi(mobile_row[37]);
	GET_MOB_VAL(mob_proto + i, 2) = atoi(mobile_row[38]);
	GET_MOB_VAL(mob_proto + i, 3) = atoi(mobile_row[39]);

	for (j = 0; j < 5; j++)
		GET_SAVE(mob_proto + i, j) = 0;

	spec = atoi(mobile_row[34]);

	if (spec < 0 && spec > NUM_MOB_SPECS) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Call of non-existant specproc in mob #%d.", vnum);
		kill_mysql();
		exit(1);
	} else {
		mob_index[i].specproc = spec;
	}

	/* Seek to first row, since we are looping this set more than once. */
	mysql_data_seek(triggers, 0);

	while ((triggers_row = mysql_fetch_row(triggers))) {
		if (atoi(triggers_row[2]) == vnum)
			dg_read_trigger(triggers_row, &mob_proto[i], MOB_TRIGGER);
	}

	mob_proto[i].aff_abils = mob_proto[i].real_abils;
	
  for (j = 0; j < NUM_WEARS; j++)
    mob_proto[i].equipment[j] = NULL;

  mob_proto[i].nr = i;
  mob_proto[i].desc = NULL;

	check_mobile_values(&mob_proto[i]);
	top_of_mobt = i++;
}

#undef CASE
#undef BOOL_CASE
#undef RANGE


/* New version of parse_object for MySQL by Torgny Bjers, 2001-09-15. */
/* read all objects from obj file; generate index and prototypes */
void parse_object(MYSQL_ROW object_row, MYSQL_RES *extradescs, MYSQL_RES *affects, MYSQL_RES *triggers, int vnum)
{
	static int i = 0;
	MYSQL_ROW extradescs_row;
	MYSQL_ROW affects_row;
	MYSQL_ROW triggers_row;
	struct extra_descr_data *new_descr;
	int j = 0, spec = 0;
	char *tmpptr;

	obj_index[i].vnum = vnum;
	obj_index[i].number = 0;
	obj_index[i].func = NULL;

	clear_object(obj_proto + i);
	obj_proto[i].item_number = i;
	obj_proto[i].unique_id = 0;

	sprintf(buf2, "object #%d", vnum);

	/* *** string data *** */
	if ((obj_proto[i].name = str_dup(object_row[2])) == NULL) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Null obj name or format error at or near %s", buf2);
		kill_mysql();
		exit(1);
	}
	tmpptr = obj_proto[i].short_description = str_dup(object_row[3]);
	if (tmpptr && *tmpptr)
		if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
				!str_cmp(fname(tmpptr), "the"))
			*tmpptr = LOWER(*tmpptr);

	tmpptr = obj_proto[i].description = str_dup(object_row[4]); /* StringAllocFix */
	if (tmpptr && *tmpptr)
		CAP(tmpptr);
	obj_proto[i].action_description = str_dup(object_row[5]); /* StringAllocFix */

	/* Object flags checked in check_object(). */
	GET_OBJ_TYPE(obj_proto + i) = atoi(object_row[6]);
	GET_OBJ_EXTRA(obj_proto + i) = asciiflag_conv(object_row[7]);
	GET_OBJ_WEAR(obj_proto + i) = asciiflag_conv(object_row[8]);
	obj_proto[i].obj_flags.bitvector = atoi(object_row[9]);

	GET_OBJ_VAL(obj_proto + i, 0) = atoi(object_row[10]);
	GET_OBJ_VAL(obj_proto + i, 1) = atoi(object_row[11]);
	GET_OBJ_VAL(obj_proto + i, 2) = atoi(object_row[12]);
	GET_OBJ_VAL(obj_proto + i, 3) = atoi(object_row[13]);

	GET_OBJ_WEIGHT(obj_proto + i) = atoi(object_row[14]);
	GET_OBJ_COST(obj_proto + i) = atoi(object_row[15]);
	GET_OBJ_RENT(obj_proto + i) = atoi(object_row[16]);

	GET_OBJ_SIZE(obj_proto + i) = atoi(object_row[19]);
	GET_OBJ_COLOR(obj_proto + i) = LIMIT(atoi(object_row[20]), 0, NUM_EXT_COLORS);
	GET_OBJ_RESOURCE(obj_proto + i) = LIMIT(atoi(object_row[21]), 0, NUM_RESOURCES);

	/* check to make sure that weight of containers exceeds curr. quantity */
	if (GET_OBJ_TYPE(obj_proto + i) == ITEM_DRINKCON || GET_OBJ_TYPE(obj_proto + i) == ITEM_FOUNTAIN) {
		if (GET_OBJ_WEIGHT(obj_proto + i) < GET_OBJ_VAL(obj_proto + i, 1))
			GET_OBJ_WEIGHT(obj_proto + i) = GET_OBJ_VAL(obj_proto + i, 1) + 5;
	}

	/* *** extra descriptions and affect fields *** */

	for (j = 0; j < MAX_OBJ_AFFECT; j++) {
		obj_proto[i].affected[j].location = APPLY_NONE;
		obj_proto[i].affected[j].modifier = 0;
	}

	/* Seek to first row, since we are looping this set more than once. */
	mysql_data_seek(extradescs, 0);

	/* Create extra descriptions */
	while ((extradescs_row = mysql_fetch_row(extradescs)))
	{
		if (atoi(extradescs_row[2]) == vnum) {
			CREATE(new_descr, struct extra_descr_data, 1);
			new_descr->keyword = str_dup(extradescs_row[3]);
			new_descr->description = str_dup(extradescs_row[4]); /* StringAllocFix */
			new_descr->next = obj_proto[i].ex_description;
			obj_proto[i].ex_description = new_descr;
		}
	}

	j = 0;

	/* Seek to first row, since we are looping this set more than once. */
	mysql_data_seek(affects, 0);

	/* Create affects */
	while ((affects_row = mysql_fetch_row(affects)))
	{
		if (atoi(affects_row[0]) == vnum) {
			if (j >= MAX_OBJ_AFFECT) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "Too many Affect fields (%d max), object %d", MAX_OBJ_AFFECT, vnum);
				kill_mysql();
				exit(1);
			}
			obj_proto[i].affected[j].location = atoi(affects_row[2]);
			obj_proto[i].affected[j].modifier = atoi(affects_row[3]);
			j++;
		}
	}

	spec = atoi(object_row[18]);

	if (spec < 0 && spec > NUM_OBJ_SPECS) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Call of non-existant specproc in obj #%d.", vnum);
		kill_mysql();
		exit(1);
	} else {
		obj_index[i].specproc = spec;
	}

	/* Seek to first row, since we are looping this set more than once. */
	mysql_data_seek(triggers, 0);

	while ((triggers_row = mysql_fetch_row(triggers))) {
		if (atoi(triggers_row[2]) == vnum)
			dg_obj_trigger(triggers_row, &obj_proto[i]);
	}

	check_object(&obj_proto[i]);
	top_of_objt = i++;

}


#define	Z        zone_table[zone]

/* New version of load_zones for MySQL by Torgny Bjers, 2001-09-15. */
void load_zones(MYSQL_RES * content, int znum)
{
	MYSQL_RES * commands;
	MYSQL_ROW index_row, commands_row;
	static zone_rnum zone = 0;
	int cmd_no = 0, num_of_cmds = 0, zone_found = 0;

	if (!(commands = mysqlGetResource(TABLE_ZON_COMMANDS, "SELECT * FROM %s WHERE znum = %d ORDER BY cmdnum ASC;", TABLE_ZON_COMMANDS, znum))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not load zone commands for zone [%d] from table [%s].", znum, TABLE_ZON_COMMANDS);
		kill_mysql();
		exit(1);
	}

	/* 
	 * Have to have +1 because the zone command terminator, S, counts as a
	 * command in the list.  We add that at the end. Torgny Bjers, 2001-09-17
	 */
	num_of_cmds = mysql_num_rows(commands) + 1;

	/* Zero commands is impossible, but we leave it for some unknown reason. */
	if (num_of_cmds == 0) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "zone %d is empty!", znum);
		exit(1);
	} else
		CREATE(Z.cmd, struct reset_com, num_of_cmds);

	/*
	 * Have to seek to first row in content since we are looping over the same
	 * result set with every zone.
	 */
	mysql_data_seek(content, 0);

	while ((index_row = mysql_fetch_row(content))) {
		if (atoi(index_row[0]) == znum) {

			zone_found = 1;

			Z.number = atoi(index_row[0]);
			Z.name = str_dup(index_row[1]);
			Z.lifespan = atoi(index_row[2]);
			Z.bot = atoi(index_row[3]);
			Z.top = atoi(index_row[4]);
			Z.reset_mode = atoi(index_row[5]);
			Z.zone_flags = atoi(index_row[6]);
			Z.builders = str_dup(index_row[8]);

			if (Z.bot > Z.top) {
				mlog("SYSERR: Zone %d bottom (%d) > top (%d).", Z.number, Z.bot, Z.top);
				mysql_free_result(content);
				mysql_free_result(commands);
				exit(1);
			}

			while ((commands_row = mysql_fetch_row(commands))) {
				ZCMD.id = atoi(commands_row[0]);
				ZCMD.command = *commands_row[2];
				ZCMD.if_flag = atoi(commands_row[3]);
				ZCMD.arg1 = atoi(commands_row[4]);
				ZCMD.arg2 = atoi(commands_row[5]);
				ZCMD.arg3 = atoi(commands_row[6]);
				ZCMD.sarg1 = str_dup(commands_row[7]);
				ZCMD.sarg2 = str_dup(commands_row[8]);
				ZCMD.line = cmd_no;
				cmd_no++;
			}

			/*
			 * Add the zone command terminator. This terminated the zone commands 
			 * in the zone files before MySQL, now we don't need to read it, but
			 * we still need it in the command list since we don't want to break
			 * the other zone related code.
			 */
			ZCMD.command = 'S';

			top_of_zone_table = zone++;

		}
	}

	if (zone_found == 0) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "(%s)%s:%d: Requested zone (#%d) not found in index.", __FILE__, __FUNCTION__, __LINE__, znum);
		return;
	}

	/* Free up result sets to conserve memory. */
	mysql_free_result(commands);

}

#undef Z


void get_one_line(FILE *fl, char *buf)
{
	if (fgets(buf, READ_SIZE, fl) == NULL) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "error reading help file: not terminated with $?");
		kill_mysql();
		exit(1);
	}

	buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}


/*************************************************************************
*	 procedures for resetting, both play-time and boot-time                  *
*************************************************************************/

/* create a character, and add it to the char list */
struct char_data *create_char(void)
{
	struct char_data *ch;

	CREATE(ch, struct char_data, 1);
	clear_char(ch);
	ch->next = character_list;
	character_list = ch;
	GET_ID(ch) = max_mob_id++;

	return (ch);
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
	mob_rnum i;
	struct char_data *mob;

	if (type == VIRTUAL) {
		if ((i = real_mobile(nr)) < 0) {
			mlog("WARNING: Mobile vnum %d does not exist in database.", nr);
			return (NULL);
		}
	} else
		i = nr;

	CREATE(mob, struct char_data, 1);
	clear_char(mob);
	*mob = mob_proto[i];
	mob->next = character_list;
	character_list = mob;

	if (!mob->points.max_hit) {
		mob->points.max_hit = dice(mob->points.hit, mob->points.mana) +
			mob->points.move;
	} else
		mob->points.max_hit = number(mob->points.hit, mob->points.mana);

	mob->points.hit = mob->points.max_hit;
	mob->points.mana = mob->points.max_mana;
	mob->points.move = mob->points.max_move;

	mob->player.time.birth = time(0);
	mob->player.time.played = 0;
	mob->player.time.logon = time(0);

	mob_index[i].number++;
	GET_ID(mob) = max_mob_id++;
	assign_triggers(mob, MOB_TRIGGER);

	return (mob);
}


/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
	struct obj_data *obj;

	CREATE(obj, struct obj_data, 1);
	clear_object(obj);
	obj->next = object_list;
	object_list = obj;
	GET_ID(obj) = max_obj_id++;
	assign_triggers(obj, OBJ_TRIGGER);

	return (obj);
}


/* create a new object from a prototype */
struct obj_data *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
	struct obj_data *obj;
	obj_rnum i = type == VIRTUAL ? real_object(nr) : nr;

	if (i == NOTHING || i > top_of_objt) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Object (%c) %d does not exist in database.", type == VIRTUAL ? 'V' : 'R', nr);
		return (NULL);
	}

	CREATE(obj, struct obj_data, 1);
	clear_object(obj);
	*obj = obj_proto[i];
	obj->next = object_list;
	object_list = obj;

	obj_index[i].number++;
	GET_ID(obj) = max_obj_id++;
	assign_triggers(obj, OBJ_TRIGGER);

	return (obj);
}


#define	ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
	int i;
	struct reset_q_element *update_u, *temp;
	static int timer = 0;

	/* jelson 10/22/92 */
	if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
		/* one minute has passed */
		/*
		 * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
		 * factor of 60
		 */

		timer = 0;

		/* since one minute has passed, increment zone ages */
		for (i = 0; i <= top_of_zone_table; i++) {
			if ((zone_table[i].age < zone_table[i].lifespan &&
					zone_table[i].reset_mode) && zone_reset)
				(zone_table[i].age)++;

			if (zone_table[i].age >= zone_table[i].lifespan &&
					zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
				/* enqueue zone */

				CREATE(update_u, struct reset_q_element, 1);

				update_u->zone_to_reset = i;
				update_u->next = 0;

				if (!reset_q.head)
					reset_q.head = reset_q.tail = update_u;
				else {
					reset_q.tail->next = update_u;
					reset_q.tail = update_u;
				}

				zone_table[i].age = ZO_DEAD;
			}
		}
	}        /* end - one minute has passed */


	/* dequeue zones (if possible) and reset */
	/* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
	for (update_u = reset_q.head; update_u; update_u = update_u->next)
		if ((zone_table[update_u->zone_to_reset].reset_mode == 2 ||
				is_empty(update_u->zone_to_reset)) && zone_reset) {
			reset_zone(update_u->zone_to_reset);
			extended_mudlog(CMP, SYSL_RESETS, FALSE, "Auto zone reset: %s",
							zone_table[update_u->zone_to_reset].name);
			/* dequeue */
			if (update_u == reset_q.head)
				reset_q.head = reset_q.head->next;
			else {
				for (temp = reset_q.head; temp->next != update_u;
						 temp = temp->next);

				if (!update_u->next)
					reset_q.tail = temp;

				temp->next = update_u->next;
			}

			free(update_u);
			break;
		}
}


void log_zone_error(zone_rnum zone, int cmd_no, int id, const char *message)
{
	extended_mudlog(BRF, SYSL_BUGS, TRUE, "zone file: %s", message);

	extended_mudlog(BRF, SYSL_BUGS, TRUE, "offending cmd: '%c' cmd in zone #%d, line %d, database id %d",
					ZCMD.command, zone_table[zone].number, cmd_no, id);
}


#define	ZONE_ERROR(message) \
				{ log_zone_error(zone, cmd_no, ZCMD.id, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
{
	int cmd_no, last_cmd = 0;
	struct char_data *mob = NULL;
	struct obj_data *obj, *obj_to;
	int room_vnum, room_rnum;
	struct char_data *tmob=NULL; /* for trigger assignment */
	struct obj_data *tobj=NULL;  /* for trigger assignment */

	for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {

		if (ZCMD.if_flag && !last_cmd)
			continue;

		/*  This is the list of actual zone commands.  If any new
		 *  zone commands are added to the game, be certain to update
		 *  the list of commands in load_zone() so that the counting
		 *  will still be correct. - ae.
		 */
		switch (ZCMD.command) {
		case '*':                        /* ignore command */
			last_cmd = 0;
			break;

		case 'M':                        /* read a mobile */
			if (mob_index[ZCMD.arg1].number < ZCMD.arg2) {
				mob = read_mobile(ZCMD.arg1, REAL);
				char_to_room(mob, ZCMD.arg3);
				load_mtrigger(mob);
				tmob = mob;
				last_cmd = 1;
			} else
				last_cmd = 0;
				tobj = NULL;
			break;

		case 'O':                        /* read an object */
			if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
				if (ZCMD.arg3 != NOWHERE) {
					obj = read_object(ZCMD.arg1, REAL);
					obj_to_room(obj, ZCMD.arg3);
					load_otrigger(obj);
					tobj = obj;
					last_cmd = 1;
					/* obj_load = TRUE; */
				} else {
					obj = read_object(ZCMD.arg1, REAL);
					IN_ROOM(obj) = NOWHERE;
					tobj = obj;
					last_cmd = 1;
					/* obj_load = TRUE; */
				}
			} else
				last_cmd = 0;
			tmob = NULL;
			break;

		case 'P':                        /* object to object */
			if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
				obj = read_object(ZCMD.arg1, REAL);
				if (!(obj_to = get_obj_num(ZCMD.arg3))) {
					ZONE_ERROR("target obj not found, command disabled");
					ZCMD.command = '*';
					break;
				}
				obj_to_obj(obj, obj_to);
				load_otrigger(obj);
				tobj = obj;
				last_cmd = 1;
			} else
				last_cmd = 0;
			tmob = NULL;
			break;

		case 'G':                        /* obj_to_char */
			if (!mob) {
				ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
				ZCMD.command = '*';
				break;
			}
			if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
				obj = read_object(ZCMD.arg1, REAL);
				obj_to_char(obj, mob);
				tobj = obj;
				load_otrigger(obj);
				last_cmd = 1;
			} else
				last_cmd = 0;
			tmob = NULL;
			break;
			
		case 'E':                        /* object to equipment list */
			if (!mob) {
				ZONE_ERROR("trying to equip non-existant mob, command disabled");
				ZCMD.command = '*';
				break;
			}
			if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
				if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
					ZONE_ERROR("invalid equipment pos number");
				} else {
					obj = read_object(ZCMD.arg1, REAL);
					
					IN_ROOM(obj) = NOWHERE;
					load_otrigger(obj);
					if (wear_otrigger(obj, mob, ZCMD.arg3))
						equip_char(mob, obj, ZCMD.arg3);
					else
						obj_to_char(obj, mob);
					tobj = obj;
					last_cmd = 1;
				}
			} else
				last_cmd = 0;
			tmob = NULL;
			break;

		case 'R': /* rem obj from room */
			if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL)
				extract_obj(obj);
			last_cmd = 1;
			tmob = NULL;
			tobj = NULL;
			break;


		case 'D':                        /* set state of door */
			if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
					(world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL)) {
				ZONE_ERROR("door does not exist, command disabled");
				ZCMD.command = '*';
			} else
				switch (ZCMD.arg3) {
				case 0:
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_LOCKED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_CLOSED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_FAERIE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_INFERNAL);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_DIVINE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_ANCIENT);
					break;
				case 1:
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
									EX_CLOSED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_LOCKED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_FAERIE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_INFERNAL);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_DIVINE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_ANCIENT);
					break;
				case 2:
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
									EX_LOCKED);
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
									EX_CLOSED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_FAERIE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_INFERNAL);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_DIVINE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_ANCIENT);
					break;
				case 3:
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
									EX_LOCKED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
									EX_CLOSED);
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_FAERIE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_INFERNAL);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_DIVINE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_ANCIENT);
					break;
				case 4:
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
									EX_LOCKED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
									EX_CLOSED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_FAERIE);
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_INFERNAL);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_DIVINE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_ANCIENT);
					break;
				case 5:
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
									EX_LOCKED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
									EX_CLOSED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_FAERIE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_INFERNAL);
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_DIVINE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_ANCIENT);
					break;
				case 6:
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
									EX_LOCKED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
									EX_CLOSED);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_FAERIE);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_INFERNAL);
					REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_DIVINE);
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
										 EX_ANCIENT);
					break;
				}
			last_cmd = 1;
			tmob = NULL;
			tobj = NULL;
			break;

		case 'T': /* trigger command */
			if (ZCMD.arg1 == MOB_TRIGGER && tmob) {
				if (!SCRIPT(tmob))
					CREATE(SCRIPT(tmob), struct script_data, 1);
				add_trigger(SCRIPT(tmob), read_trigger(ZCMD.arg2), -1);
				last_cmd = 1;
			} else if (ZCMD.arg1==OBJ_TRIGGER && tobj) {
				if (!SCRIPT(tobj))
					CREATE(SCRIPT(tobj), struct script_data, 1);
				add_trigger(SCRIPT(tobj), read_trigger(ZCMD.arg2), -1);
				last_cmd = 1;
			} else if (ZCMD.arg1==WLD_TRIGGER) {
				if (ZCMD.arg3 == NOWHERE || ZCMD.arg3 > top_of_world) {
					ZONE_ERROR("Invalid room number in trigger assignment");
				} else {
					if (!world[ZCMD.arg3].script)
						CREATE(world[ZCMD.arg3].script, struct script_data, 1);
					add_trigger(world[ZCMD.arg3].script, read_trigger(ZCMD.arg2), -1);
					last_cmd = 1;
				}
      }

			break;

		case 'V':
			if (ZCMD.arg1 == MOB_TRIGGER && tmob) {
				if (!SCRIPT(tmob)) {
					ZONE_ERROR("Attempt to give variable to scriptless mobile");
				} else
					add_var(&(SCRIPT(tmob)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
									ZCMD.arg3);
				last_cmd = 1;
			} else if (ZCMD.arg1 == OBJ_TRIGGER && tobj) {
				if (!SCRIPT(tobj)) {
					ZONE_ERROR("Attempt to give variable to scriptless object");
				} else
					add_var(&(SCRIPT(tobj)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
									ZCMD.arg3);
				last_cmd = 1;
			} else if (ZCMD.arg1 == WLD_TRIGGER) {
				if (ZCMD.arg3 == NOWHERE || ZCMD.arg3 > top_of_world) {
					ZONE_ERROR("Invalid room number in variable assignment");
				} else {
					if (!(world[ZCMD.arg3].script)) {
						ZONE_ERROR("Attempt to give variable to scriptless room");
					} else
						add_var(&(world[ZCMD.arg3].script->global_vars),
										ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg3);
					last_cmd = 1;
				}
			}
			break;

		default:
			ZONE_ERROR("unknown cmd in reset table; cmd disabled");
			ZCMD.command = '*';
			break;
		}
	}

	zone_table[zone].age = 0;
	
	/* handle reset_wtrigger's and reset_room_flux */
	room_vnum = zone_table[zone].number * 100;
	while (room_vnum <= zone_table[zone].top) {
		room_rnum = real_room(room_vnum);
		if (room_rnum != NOWHERE) {
			reset_wtrigger(&world[room_rnum]);
			reset_room_flux(&world[room_rnum]);
			reset_room_resources(&world[room_rnum]);
		}
		room_vnum++;
	}

}


/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int	is_empty(zone_rnum zone_nr)
{
	struct descriptor_data *i;

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING && STATE(i) != CON_COPYOVER) 
			continue;
		if (IN_ROOM(i->character) == NOWHERE)
			continue;
		if (world[IN_ROOM(i->character)].zone != zone_nr)
			continue;
		/*
		 * if an immortal has nohassle off, he counts as present 
		 * added for testing zone reset triggers - Welcor 
		 */
		if (IS_IMMORTAL(i->character) && (PRF_FLAGGED(i->character, PRF_NOHASSLE)))
			continue;

		return (0);
	}

	return (1);
}


/*************************************************************************
*	 stuff related to the save/load player system                                 *
*************************************************************************/

long get_ptable_by_name(char *name)
{
	int i;

	one_argument(name, arg);
	for (i = 0; i <= top_of_p_table; i++)
		if (!strcmp(player_table[i].name, arg))
			return (i);

	return (NOBODY);
}


long get_id_by_name(char *name)
{
	int i;

	one_argument(name, arg);
	for (i = 0; i <= top_of_p_table; i++)
		if (!strcmp(player_table[i].name, arg))
			return (player_table[i].id);

	return (NOBODY);
}


char *get_name_by_id(long id)
{
	int i;

	for (i = 0; i <= top_of_p_table; i++)
		if (player_table[i].id == id)
			return (player_table[i].name);

	return (NULL);
}


#define	NUM_OF_SAVE_THROWS        5

/* remove ^M's from file output */
/* There may be a similar function in Oasis (and I'm sure
	 it's part of obuild).  Remove this if you get a
	 multiple definition error or if it you want to use a
	 substitute
*/
void kill_ems(char *str)
{
	char *ptr1, *ptr2, *tmp;

	tmp = str;
	ptr1 = str;
	ptr2 = str;

	while(*ptr1) {
		if((*(ptr2++) = *(ptr1++)) == '\r')
			if(*ptr1 == '\r')
				ptr1++;
	}
	*ptr2 = '\0';
}


/* New version of load_char for MySQL by Torgny Bjers, 2001-09-15. */
/* Load a char, TRUE if loaded, FALSE if not */
int	load_char(char *name, struct char_data *ch)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int id, i, NumAffects = 0, NumQuests = 0, NumSkills = 0;
	struct affected_type af;
	unsigned long *fieldlength;

	if((id = find_name(name)) < 0)
		return (NOBODY);
	else {

		if (!(result = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT * FROM %s WHERE ID = %ld;", TABLE_PLAYER_INDEX, player_table[id].id)))
			return (NOBODY);

		/* character initializations */
		/* initializations necessary to keep some things straight */
		ch->affected = NULL;
		for(i = 1; i <= MAX_SKILLS; i++) {
			GET_SKILL(ch, i) = 0;
			GET_SKILL_STATE(ch, i) = SSTATE_INCREASE;
		}
		GET_SEX(ch) = PFDEF_SEX;
		GET_CLASS(ch) = PFDEF_CLASS;
		GET_RACE(ch) = PFDEF_RACE;
		GET_CULTURE(ch) = PFDEF_CULTURE;
		GET_HOME(ch) = PFDEF_HOMETOWN;
		GET_HEIGHT(ch) = PFDEF_HEIGHT;
		GET_WEIGHT(ch) = PFDEF_WEIGHT;
		GET_ALIGNMENT(ch) = PFDEF_ALIGNMENT;
		PLR_FLAGS(ch) = PFDEF_PLRFLAGS;
		AFF_FLAGS(ch) = PFDEF_AFFFLAGS;
		for(i = 0; i < NUM_OF_SAVE_THROWS; i++)
			GET_SAVE(ch, i) = PFDEF_SAVETHROW;
		GET_LOADROOM(ch) = PFDEF_LOADROOM;
		GET_INVIS_LEV(ch) = PFDEF_INVISLEV;
		GET_WIMP_LEV(ch) = PFDEF_WIMPLEV;
		GET_COND(ch, FULL) = PFDEF_HUNGER;
		GET_COND(ch, THIRST) = PFDEF_THIRST;
		GET_COND(ch, DRUNK) = PFDEF_DRUNK;
		GET_BAD_PWS(ch) = PFDEF_BADPWS;
		PRF_FLAGS(ch) = PFDEF_PREFFLAGS;
		GET_PRACTICES(ch) = PFDEF_PRACTICES;
		GET_GOLD(ch) = PFDEF_GOLD;
		GET_BANK_GOLD(ch) = PFDEF_BANK;
		GET_EXP(ch) = PFDEF_EXP;
		GET_HITROLL(ch) = PFDEF_HITROLL;
		GET_DAMROLL(ch) = PFDEF_DAMROLL;
		GET_PD(ch) = PFDEF_AC;
		ch->real_abils.strength = PFDEF_STRENGTH;
		ch->real_abils.agility = PFDEF_AGILITY;
		ch->real_abils.precision = PFDEF_PRECISION;
		ch->real_abils.perception = PFDEF_PERCEPTION;
		ch->real_abils.health = PFDEF_HEALTH;
		ch->real_abils.willpower = PFDEF_WILLPOWER;
		ch->real_abils.intelligence = PFDEF_INTELL;
		ch->real_abils.charisma = PFDEF_CHARISMA;
		ch->real_abils.luck = PFDEF_LUCK;
		ch->real_abils.essence = PFDEF_ESSENCE;
		GET_HIT(ch) = PFDEF_HIT;
		GET_MAX_HIT(ch) = PFDEF_MAXHIT;
		GET_MANA(ch) = PFDEF_MANA;
		GET_MAX_MANA(ch) = PFDEF_MAXMANA;
		GET_MOVE(ch) = PFDEF_MOVE;
		GET_MAX_MOVE(ch) = PFDEF_MAXMOVE;
		GET_SKILLCAP(ch) = PFDEF_SKILLCAP;
		GET_QUEST(ch) = -1;
		GET_NUM_QUESTS(ch) = -1;
		ch->completed_quests = NULL;
		ch->num_completed_quests = 0;
		ch->player_specials->saved.bday_year = time_info.year-17;
		ch->player_specials->saved.bday_month = time_info.month;
		ch->player_specials->saved.bday_day = time_info.day;
		ch->player_specials->saved.bday_hours = time_info.hours;
		GET_ID(ch) = max_mob_id++;
		GET_CONTACT(ch) = NULL;
		GET_APPROVEDBY(ch) = NULL;
		IS_ACTIVE(ch) = FALSE;
		SPEAKING(ch) = -1;
		GET_EYECOLOR(ch) = PFDEF_EYECOLOR;
		GET_HAIRCOLOR(ch) = PFDEF_HAIRCOLOR;
		GET_HAIRSTYLE(ch) = PFDEF_HAIRSTYLE;
		GET_SKINTONE(ch) = PFDEF_SKINTONE;
		USER_RIGHTS(ch) = PFDEF_USERRIGHTS;
		EXTENDED_SYSLOG(ch) = PFDEF_XSYSLOG;
		GET_MAILINGLIST(ch) = PFDEF_MAILINGLIST;
		GET_TRUENAME(ch) = NULL;
		GET_COLORPREF_ECHO(ch) = 0;
		GET_COLORPREF_EMOTE(ch) = 0;
		GET_COLORPREF_POSE(ch) = 0;
		GET_COLORPREF_SAY(ch) = 0;
		GET_COLORPREF_OSAY(ch) = 0;
		GET_RPXP(ch) = 0;
		GET_PAGE_LENGTH(ch) = PAGE_LENGTH;
		GET_ACTIVEDESC(ch) = 0;
		GET_OLC_ZONE(ch) = 0;
		GET_AWAY(ch) = NULL;
		GET_QP(ch) = 0;
		GET_CONTACTINFO(ch) = 0;
		GET_PIETY(ch) = PIETY_UNBELIEVER;
		GET_REPUTATION(ch) = REP_UNKNOWN;
		GET_SOCIAL_RANK(ch) = SRANK_SLAVE;
		GET_MILITARY_RANK(ch) = NOTHING;
		GET_SANITY(ch) = SAN_SANE;
		GET_FATIGUE(ch) = 0;
		GET_FLUX(ch) = 0;
		GET_MAX_FLUX(ch) = 0;
		IS_VERIFIED(ch) = FALSE;

		GET_SDESC(ch) = NULL;
		GET_LDESC(ch) = NULL;
		GET_DOING(ch) = NULL;
		GET_KEYWORDS(ch) = NULL;

		for (i = 0; i < RPLOG_SIZE; i++)
			GET_RPLOG(ch, i) = NULL;

		for (i = 0; i < NUM_DESCS; i++)
			GET_RPDESCRIPTION(ch, i) = NULL;

		for (i = 0; i < MAX_RECOGNIZED; i++)
			GET_RECOGNIZED(ch, i) = 0;

		for (i = 0; i < MAX_NOTIFY; i++)
			GET_NOTIFY(ch, i) = 0;

		/* Fill in with values from the database. */
		if ((row = mysql_fetch_row(result)))
		{
			fieldlength = mysql_fetch_lengths(result);
			GET_PC_NAME(ch) = str_dup(row[0]);
			strcpy(GET_PASSWD(ch), row[1]);
			strcpy(GET_EMAIL(ch), row[2]);
			GET_TITLE(ch) = str_dup(row[3]);
			ch->player.description = str_dup(row[4]);
			GET_SEX(ch) = atoi(row[5]);
			GET_CLASS(ch) = atoi(row[6]);
			set_race(ch, atoi(row[7]));
			GET_HOME(ch) = atoi(row[8]);
			ch->player.time.birth = atoi(row[9]);
			ch->player.time.played = atoi(row[10]);
			ch->player.time.logon = atoi(row[11]);
			ch->player_specials->saved.bday_year = atoi(row[12]);
			ch->player_specials->saved.bday_month = atoi(row[13]);
			ch->player_specials->saved.bday_day = atoi(row[14]);
			ch->player_specials->saved.bday_hours = atoi(row[15]);
			ch->player_specials->host = str_dup(row[16]);
			GET_HEIGHT(ch) = atoi(row[17]);
			GET_WEIGHT(ch) = atoi(row[18]);
			GET_ALIGNMENT(ch) = LIMIT(atoi(row[19]), -1000, 1000);
			GET_IDNUM(ch) = atoi(row[20]);
			PLR_FLAGS(ch) = atoi(row[21]);
			AFF_FLAGS(ch) = asciiflag_conv(row[22]);
			GET_SAVE(ch, 0) = atoi(row[23]);
			GET_SAVE(ch, 1) = atoi(row[24]);
			GET_SAVE(ch, 2) = atoi(row[25]);
			GET_SAVE(ch, 3) = atoi(row[26]);
			GET_SAVE(ch, 4) = atoi(row[27]);
			GET_WIMP_LEV(ch) = atoi(row[28]);
			GET_INVIS_LEV(ch) = asciiflag_conv(row[29]);
			GET_LOADROOM(ch) = atoi(row[30]);
			PRF_FLAGS(ch) = asciiflag_conv(row[31]);
			GET_BAD_PWS(ch) = atoi(row[32]);
			GET_COND(ch, FULL) = atoi(row[33]);
			GET_COND(ch, THIRST) = atoi(row[34]);
			GET_COND(ch, DRUNK) = atoi(row[35]);
			GET_PRACTICES(ch) = atoi(row[36]);
			for (i = 0; i < top_of_race_list; i++)
				if (race_list[i].constant == GET_RACE(ch))
					break;
			ch->real_abils.strength = LIMIT(atoi(row[37]), race_list[i].min_attrib, race_list[i].max_attrib);
			ch->real_abils.agility = LIMIT(atoi(row[38]), race_list[i].min_attrib, race_list[i].max_attrib);
			ch->real_abils.precision = LIMIT(atoi(row[39]), race_list[i].min_attrib, race_list[i].max_attrib);
			ch->real_abils.perception = LIMIT(atoi(row[40]), race_list[i].min_attrib, race_list[i].max_attrib);
			ch->real_abils.health = LIMIT(atoi(row[41]), race_list[i].min_attrib, 32000);
			ch->real_abils.willpower = LIMIT(atoi(row[42]), race_list[i].min_attrib, race_list[i].max_attrib);
			ch->real_abils.intelligence = LIMIT(atoi(row[43]), race_list[i].min_attrib, race_list[i].max_attrib);
			ch->real_abils.charisma = LIMIT(atoi(row[44]), race_list[i].min_attrib, race_list[i].max_attrib);
			ch->real_abils.luck = LIMIT(atoi(row[45]), race_list[i].min_attrib, race_list[i].max_attrib);
			ch->real_abils.essence = LIMIT(atoi(row[46]), race_list[i].min_attrib, race_list[i].max_attrib);
			GET_HIT(ch) = atoi(row[47]);
			GET_MANA(ch) = atoi(row[48]);
			GET_MOVE(ch) = atoi(row[49]);
			GET_MAX_HIT(ch) = LIMIT(atoi(row[50]), 1, 4500);
			GET_MAX_MANA(ch) = LIMIT(atoi(row[51]), 1, 4500);
			GET_MAX_MOVE(ch) = LIMIT(atoi(row[52]), 1, 4500);
			GET_GOLD(ch) = atoi(row[54]);
			GET_BANK_GOLD(ch) = atoi(row[55]);
			GET_EXP(ch) = atoi(row[56]);
			GET_HITROLL(ch) = atoi(row[57]);
			GET_DAMROLL(ch) = atoi(row[58]);
			GET_QUEST(ch) = atoi(row[59]);
			NumAffects = atoi(row[60]);
			NumQuests = atoi(row[61]);
			NumSkills = atoi(row[63]);
			GET_CONTACT(ch) = NULL_STR(fieldlength[64], row[64]);
			IS_ACTIVE(ch) = atoi(row[65]);
			SPEAKING(ch) = atoi(row[66]);
			GET_EYECOLOR(ch) = LIMIT(atoi(row[67]), 0, NUM_EYECOLORS - 1);
			GET_HAIRCOLOR(ch) = LIMIT(atoi(row[68]), 0, NUM_HAIRCOLORS - 1);
			GET_HAIRSTYLE(ch) = LIMIT(atoi(row[69]), 0, NUM_HAIRSTYLES - 1);
			GET_SKINTONE(ch) = LIMIT(atoi(row[70]), 0, NUM_SKINTONES - 1);
			GET_MAILINGLIST(ch) = (bool)atoi(row[71]);
			USER_RIGHTS(ch) = asciiflag_conv(row[72]);
			EXTENDED_SYSLOG(ch) = asciiflag_conv(row[73]);
			GET_TRUENAME(ch) = NULL_STR(fieldlength[74], row[74]);
			GET_COLORPREF_ECHO(ch) = *row[75];
			GET_COLORPREF_EMOTE(ch) = *row[76];
			GET_COLORPREF_POSE(ch) = *row[77];
			GET_COLORPREF_SAY(ch) = *row[78];
			GET_COLORPREF_OSAY(ch) = *row[79];
			GET_RPXP(ch) = atoi(row[80]);
			GET_PAGE_LENGTH(ch) = LIMIT(atoi(row[81]), 10, 200);
			GET_ACTIVEDESC(ch) = LIMIT(atoi(row[82]), 0, NUM_DESCS - 1);
			GET_OLC_ZONE(ch) = atoi(row[83]);
			GET_AWAY(ch) = NULL_STR(fieldlength[84], row[84]);
			GET_DOING(ch) = NULL_STR(fieldlength[85], row[85]);
			GET_APPROVEDBY(ch) = NULL_STR(fieldlength[86], row[86]);
			GET_QP(ch) = atoi(row[87]);
			GET_CONTACTINFO(ch) = NULL_STR(fieldlength[88], row[88]);
			GET_FATIGUE(ch) = atoi(row[89]);
			GET_PIETY(ch) = atoi(row[90]);
			GET_REPUTATION(ch) = atoi(row[91]);
			GET_SOCIAL_RANK(ch) = atoi(row[92]);
			GET_MILITARY_RANK(ch) = atoi(row[93]);
			GET_SANITY(ch) = atoi(row[94]);
			GET_FLUX(ch) = atoi(row[95]);
			GET_MAX_FLUX(ch) = atoi(row[96]);
			IS_VERIFIED(ch) = (bool)atoi(row[97]);
			GET_SDESC(ch) = NULL_STR(fieldlength[98], row[98]);
			GET_LDESC(ch) = NULL_STR(fieldlength[99], row[99]);
			GET_KEYWORDS(ch) = NULL_STR(fieldlength[100], row[100]);
			GET_CULTURE(ch) = atoi(row[101]);
			GET_SKILLCAP(ch) = atoi(row[102]);

			/* Free loaded result to conserve memory */
			mysql_free_result(result);
		}

		if (!(result = mysqlGetResource(TABLE_PLAYER_AFFECTS, "SELECT type, duration, modifier, location, bitvector FROM %s WHERE ID = %ld;", TABLE_PLAYER_AFFECTS, GET_IDNUM(ch))))
			return (NOBODY);

		while ((row = mysql_fetch_row(result)))
		{
			if (atoi(row[0]) > top_of_spellt)
				continue;
			af.type = atoi(row[0]);
			af.duration = atoi(row[1]);
			af.modifier = atoi(row[2]);
			af.location = atoi(row[3]);
			af.bitvector = atoi(row[4]);
			affect_to_char(ch, &af);
		}
		mysql_free_result(result);

		/* Skills */
		if (IS_MORTAL(ch)) {
			if (!(result = mysqlGetResource(TABLE_PLAYER_SKILLS, "SELECT skillnum, level, state FROM %s WHERE id = %ld;", TABLE_PLAYER_SKILLS, GET_IDNUM(ch))))
				return (NOBODY);
			while ((row = mysql_fetch_row(result))) {
				GET_SKILL(ch, atoi(row[0])) = atoi(row[1]);
				GET_SKILL_STATE(ch, atoi(row[0])) = atoi(row[2]);
			}
			mysql_free_result(result);
		}

		/* Quests */
		if (!(result = mysqlGetResource(TABLE_PLAYER_QUESTS, "SELECT questnum FROM %s WHERE PlayerID = %ld;", TABLE_PLAYER_QUESTS, GET_IDNUM(ch))))
			return (NOBODY);
		while ((row = mysql_fetch_row(result))) {
			if (real_quest(atoi(row[0])) >= 0)
				add_completed_quest(ch, atoi(row[0]));
		}
		mysql_free_result(result);
		ch->num_completed_quests = NumQuests;

		/* RP Logs */
		if (!(result = mysqlGetResource(TABLE_PLAYER_RPLOGS, "SELECT * FROM %s WHERE PlayerID = %ld ORDER BY Row;", TABLE_PLAYER_RPLOGS, GET_IDNUM(ch))))
			return (NOBODY);
		i = 0;
		while ((row = mysql_fetch_row(result)))
			if (i++ < RPLOG_SIZE)
				add_to_rplog(ch, row[3]);
		mysql_free_result(result);

		/* RP Descriptions */
		if (!(result = mysqlGetResource(TABLE_PLAYER_RPLOGS, "SELECT * FROM %s WHERE PlayerID = %ld ORDER BY Row;", TABLE_PLAYER_RPDESCRIPTIONS, GET_IDNUM(ch))))
			return (NOBODY);
		i = 0;
		while ((row = mysql_fetch_row(result)))
			if (i < NUM_DESCS)
				GET_RPDESCRIPTION(ch, i++) = str_dup(row[3]);
		mysql_free_result(result);

		/* Recognized players */
		if (!(result = mysqlGetResource(TABLE_PLAYER_RECOGNIZED, "SELECT * FROM %s WHERE playerid = %ld ORDER BY id;", TABLE_PLAYER_RECOGNIZED, GET_IDNUM(ch))))
			return (NOBODY);
		i = 0;
		while ((row = mysql_fetch_row(result)))
			if (i < MAX_RECOGNIZED)
				GET_RECOGNIZED(ch, i++) = atoi(row[2]);
		mysql_free_result(result);

		/* Notifylisted players */
		if (!(result = mysqlGetResource(TABLE_PLAYER_NOTIFYLIST, "SELECT * FROM %s WHERE playerid = %ld ORDER BY id;", TABLE_PLAYER_NOTIFYLIST, GET_IDNUM(ch))))
			return (NOBODY);
		i = 0;
		while ((row = mysql_fetch_row(result)))
			if (i < MAX_NOTIFY)
				GET_NOTIFY(ch, i++) = atoi(row[2]);
		mysql_free_result(result);

	}

	affect_total(ch);

	/* initialization for imms */
	if(GOT_RIGHTS(ch, RIGHTS_IMMORTAL)) {
		for(i = 1; i <= MAX_SKILLS; i++)
			SET_SKILL(ch, i, 10000);
		GET_COND(ch, FULL) = -1;
		GET_COND(ch, THIRST) = -1;
		GET_COND(ch, DRUNK) = -1;
	}

	if (IS_IC(ch) && !IS_ACTIVE(ch))
		REMOVE_BIT(SESSION_FLAGS(ch), SESS_IC);

	return (id);
}


/*
 * insert the vital data of a player to database
 * NOTE: load_room should be an *VNUM* now.  It is converted to a vnum here.
 * This is the mysql pfile save routine */

/* New version of save_char for MySQL by Torgny Bjers, 2001-09-15. */
void save_char(struct char_data *ch, room_vnum load_room, int system)
{
	char player_aff_flags[128], player_prf_flags[128], player_user_rights[128], player_xsyslog[128];
	char player_invis_level[128];
	int i, id, NumAffects = 0, NumQuests = 0, NumSkills = 0;
	struct affected_type *aff, tmp_aff[MAX_AFFECT];
	struct obj_data *char_eq[NUM_WEARS];

	char *esc_name;
	char *esc_desc;
	char *esc_title;
	char *esc_contact;
	char *esc_rplog;
	char *esc_rpdescription;
	char *esc_awaymsg;
	char *esc_doing;
	char *esc_approvedby;
	char *esc_contactinfo;
	char *esc_sdesc;
	char *esc_ldesc;
	char *esc_keywords;

	const char *replace = "REPLACE INTO %s SET "
		"Name = '%s', "
		"Password = '%s', " // sprintf argument #5
		"Email = '%s', "
		"Title = '%s', "
		"Background = '%s', "
		"Sex = %d, "
		"Class = %d, " // 10
		"Race = %d, "
		"Home = %d, "
		"Birth = %ld, "
		"Played = %d, "
		"Last = %ld, " // 15
		"BirthYear = %d, "
		"BirthMonth = %d, "
		"BirthDay = %d, "
		"BirthHours = %d, "
		"Host = '%s', " // 20
		"Height = %d, "
		"Weight = %d, "
		"Align = %d, "
		"ID = %ld, "
		"Flags = %llu, " // 25
		"Aff = '%s', "
		"Savethrow1 = %d, "
		"Savethrow2 = %d, "
		"Savethrow3 = %d, "
		"Savethrow4 = %d, " // 30
		"Savethrow5 = %d, "
		"Wimpy = %d, "
		"Invis = '%s', " 
		"Room = %d, "
		"Preferences = '%s', " // 35
		"Badpws = %d, "
		"Hunger = %d, "
		"Thirst = %d, "
		"Drunk = %d, "
		"Practices = %d, " // 40
		"Strength = %d, "
		"Agility = %d, "
		"`Precision` = %d, "
		"Perception = %d, "
		"Health = %d, " // 45
		"Willpower = %d, "
		"Intelligence = %d, "
		"Charisma = %d, "
		"Luck = %d, "
		"Essence = %d, " // 50
		"Hit = %d, "
		"Mana = %d, "
		"Move = %d, "
		"MaxHit = %d, "
		"MaxMana = %d, " // 55
		"MaxMove = %d, "
		"AC = %d, "
		"Gold = %d, "
		"Bank = %d, "
		"Exp = %d, " // 60
		"Hitroll = %d, "
		"Damroll = %d, "
		"CurrentQuest = %d, " 
		"NumAffects = %d, "
		"NumQuests = %d, " // 65
		"NumSkills = %d, "
		"Contact = '%s', "
		"Active = %d, "
		"Language = %d, "
		"EyeColor = %d, " // 70
		"HairColor = %d, "
		"HairStyle = %d, "
		"SkinTone = %d, "
		"Perms = '%s', "
		"Syslog = '%s', "  //75
		"OKtoMail = %d, "
		"TrueName = '%s', "	
		"Colorpref_echo = '%c', "
		"Colorpref_emote = '%c', "
		"Colorpref_pose = '%c', "	// 80
		"Colorpref_say = '%c', "
		"Colorpref_osay = '%c', " 
		"RPxp = %d, "
		"PageLength = %d, "
		"ActiveDesc = %d, " // 85
		"OLC = %d, "
		"AwayMsg = '%s', "
		"Doing = '%s', "
		"ApprovedBy = '%s', "
		"Questpoints = %d, "	// 90
		"ContactInfo = '%s', "
		"Fatigue = %d, "
		"Piety = %d, "
		"Reputation = %d, "
		"SocialRank = %d, "	// 95
		"MilitaryRank = %d, "
		"Sanity = %d, "
		"Flux = %d, "
		"MaxFlux = %d, "
		"Verified = %d, "	// 100
		"SDesc = '%s', "
		"LDesc = '%s', "
		"Keywords = '%s', "
		"Culture = %d, "
		"Skillcap = %d"	// 105
		";";

	char *affects = "INSERT INTO %s (id, type, duration, modifier,"
		"location, bitvector) VALUES (%ld, %d, %d, %d, %d, %llu)";

	char *skills = "INSERT INTO %s (id, skillnum, level, state) VALUES (%ld, %d, %d, %d)";
	char *quests = "INSERT INTO %s (PlayerID, QuestNum) VALUES (%ld, %d)";

	char *rplogs = "INSERT INTO %s (PlayerID, Row, Log) VALUES (%ld, %d, '%s')";
	char *rpdescription = "INSERT INTO %s (PlayerID, Row, Description) VALUES (%ld, %d, '%s')";
	char *recognized = "INSERT INTO %s (playerid, memid) VALUES (%ld, %ld)";
	char *notifylist = "INSERT INTO %s (playerid, notifyid) VALUES (%ld, %ld)";

	if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
		return;
	/*
	 * This version of save_char allows ch->desc to be null (to allow
	 * "set file" to use it).  This causes the player's last host
	 * and probably last login to null out.
	 */

  if (IS_IC(ch) && !IS_ACTIVE(ch)) 
    REMOVE_BIT(SESSION_FLAGS(ch), SESS_IC);

	if (!PLR_FLAGGED(ch, PLR_LOADROOM))
		if (load_room != NOWHERE)
			GET_LOADROOM(ch) = load_room;

	/* Make sure we have an S-Desc */
	if (!GET_SDESC(ch)) {
		char *printbuf = get_buffer(256);
		sprintf(printbuf, "a %s %s",
			genders[(int)GET_SEX(ch)], race_list[(int)GET_RACE(ch)].name);
		GET_SDESC(ch) = str_dup(LOWERALL(printbuf));
		release_buffer(printbuf);
	}
	/* Make sure we have an L-Desc */
	if (!GET_LDESC(ch) || !*GET_LDESC(ch))
		set_default_ldesc(ch);
	/* Check the L-Desc for finishing \r\n */
	if (GET_LDESC(ch)) {
		int j, len = strlen(ch->player.long_descr);
		bool desc_ok = FALSE;
		char *printbuf = get_buffer(256);
		for (j = len; j > (len - 3); j--)
			if (ch->player.long_descr[j] == '\n' || ch->player.long_descr[j] == '\r')
				desc_ok = TRUE;
		if (!desc_ok) {
			sprintf(printbuf, "%s\r\n", ch->player.long_descr);
			free(ch->player.long_descr);
			ch->player.long_descr = str_dup(printbuf);
		}
		release_buffer(printbuf);
	}

	/* Make sure we have keywords */
	if (!GET_KEYWORDS(ch))
		GET_KEYWORDS(ch) = str_dup(create_keywords(GET_LDESC(ch)));

	/* remove affects from eq and spells */
	/* Unaffect everything a character can be affected by */
	
	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, i))
			char_eq[i] = unequip_char(ch, i);
		else
			char_eq[i] = NULL;
	}
	
	for (aff = ch->affected, i = 0; i < MAX_AFFECT; i++) {
		if (aff) {
			tmp_aff[i] = *aff;
			tmp_aff[i].next = 0;
			aff = aff->next;
		} else {
			tmp_aff[i].type = 0;        /* Zero signifies not used */
			tmp_aff[i].duration = 0;
			tmp_aff[i].modifier = 0;
			tmp_aff[i].location = 0;
			tmp_aff[i].bitvector = 0;
			tmp_aff[i].next = 0;
		}
	}
	
	/*
	* remove the affections so that the raw values are stored; otherwise the
	* effects are doubled when the char logs back in.
	*/
	
	while (ch->affected)
		affect_remove(ch, ch->affected);
	
	if ((i >= MAX_AFFECT) && aff && aff->next)
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");
	
	ch->aff_abils = ch->real_abils;
	
	/* end char_to_store code */

	sprintascii(player_aff_flags, AFF_FLAGS(ch));
	sprintascii(player_prf_flags, PRF_FLAGS(ch));
	sprintascii(player_user_rights, USER_RIGHTS(ch));
	sprintascii(player_xsyslog, EXTENDED_SYSLOG(ch));
	sprintascii(player_invis_level, GET_INVIS_LEV(ch));

	SQL_MALLOC(GET_NAME(ch), esc_name);
	SQL_MALLOC(ch->player.description, esc_desc);
	SQL_MALLOC(GET_TITLE(ch), esc_title);
	SQL_MALLOC(GET_CONTACT(ch), esc_contact);
	SQL_MALLOC(GET_AWAY(ch), esc_awaymsg);
	SQL_MALLOC(GET_DOING(ch), esc_doing);
	SQL_MALLOC(GET_APPROVEDBY(ch), esc_approvedby);
	SQL_MALLOC(GET_CONTACTINFO(ch), esc_contactinfo);
	SQL_MALLOC(GET_SDESC(ch), esc_sdesc);
	SQL_MALLOC(GET_LDESC(ch), esc_ldesc);
	SQL_MALLOC(GET_KEYWORDS(ch), esc_keywords);

	SQL_ESC(GET_NAME(ch), esc_name);
	SQL_ESC(ch->player.description, esc_desc);
	SQL_ESC(GET_TITLE(ch), esc_title);
	SQL_ESC(GET_CONTACT(ch), esc_contact);
	SQL_ESC(GET_AWAY(ch), esc_awaymsg);
	SQL_ESC(GET_DOING(ch), esc_doing);
	SQL_ESC(GET_APPROVEDBY(ch), esc_approvedby);
	SQL_ESC(GET_CONTACTINFO(ch), esc_contactinfo);
	SQL_ESC(GET_SDESC(ch), esc_sdesc);
	SQL_ESC(GET_LDESC(ch), esc_ldesc);
	SQL_ESC(GET_KEYWORDS(ch), esc_keywords);

	/* affected_type */
	mysqlWrite("DELETE FROM %s WHERE id = %ld;", TABLE_PLAYER_AFFECTS, GET_IDNUM(ch));
	if(tmp_aff[0].type > 0) {
		for (i = 0; i < MAX_AFFECT; i++) {
			aff = &tmp_aff[i];
			if (aff->type) {
				mysqlWrite(affects, TABLE_PLAYER_AFFECTS, GET_IDNUM(ch), aff->type, aff->duration, aff->modifier, aff->location, aff->bitvector);
				NumAffects++;
			}
		}
	}

	/* Quests */
	mysqlWrite("DELETE FROM %s WHERE PlayerID = %ld;", TABLE_PLAYER_QUESTS, GET_IDNUM(ch));
	if (ch->num_completed_quests > 0) {
		for (i = 0; i < ch->num_completed_quests; i++) {
			mysqlWrite(quests, TABLE_PLAYER_QUESTS, GET_IDNUM(ch), ch->completed_quests[i]);
			NumQuests++;
		}
	}

	/* skills */
	mysqlWrite("DELETE FROM %s WHERE id = %ld;", TABLE_PLAYER_SKILLS, GET_IDNUM(ch));
	if (IS_MORTAL(ch) || (GOT_RIGHTS(ch, RIGHTS_NONE) && !PLR_FLAGGED(ch, PLR_NORIGHTS))) {
		for (i = 1; i <= MAX_SKILLS; i++) {
			if (GET_SKILL(ch, i)) {
				mysqlWrite(skills, TABLE_PLAYER_SKILLS, GET_IDNUM(ch), i, GET_SKILL(ch, i), GET_SKILL_STATE(ch, i));
			}
		}
	}

	/* RP log */
	mysqlWrite("DELETE FROM %s WHERE PlayerID = %ld;", TABLE_PLAYER_RPLOGS, GET_IDNUM(ch));
	for (i = 0; i < RPLOG_SIZE; i++) {
		if (GET_RPLOG(ch, i)) {
			SQL_MALLOC(GET_RPLOG(ch, i), esc_rplog);
			SQL_ESC(GET_RPLOG(ch, i), esc_rplog);
			mysqlWrite(rplogs, TABLE_PLAYER_RPLOGS, GET_IDNUM(ch), i, esc_rplog);
			SQL_FREE(esc_rplog);
		}
	}

	/* RP descriptions */
	mysqlWrite("DELETE FROM %s WHERE PlayerID = %ld;", TABLE_PLAYER_RPDESCRIPTIONS, GET_IDNUM(ch));
	for (i = 0; i < NUM_DESCS; i++) {
		if (GET_RPDESCRIPTION(ch, i)) {
			SQL_MALLOC(GET_RPDESCRIPTION(ch, i), esc_rpdescription);
			SQL_ESC(GET_RPDESCRIPTION(ch, i), esc_rpdescription);
			mysqlWrite(rpdescription, TABLE_PLAYER_RPDESCRIPTIONS, GET_IDNUM(ch), i, esc_rpdescription);
			SQL_FREE(esc_rpdescription);
		}
	}

	/* Recognized players */
	mysqlWrite("DELETE FROM %s WHERE PlayerID = %ld;", TABLE_PLAYER_RECOGNIZED, GET_IDNUM(ch));
	for (i = 0; i < MAX_RECOGNIZED; i++)
		if (GET_RECOGNIZED(ch, i) > 0)
			mysqlWrite(recognized, TABLE_PLAYER_RECOGNIZED, GET_IDNUM(ch), GET_RECOGNIZED(ch, i));

	/* Notifylisted players */
	mysqlWrite("DELETE FROM %s WHERE PlayerID = %ld;", TABLE_PLAYER_NOTIFYLIST, GET_IDNUM(ch));
	for (i = 0; i < MAX_NOTIFY; i++)
		if (GET_NOTIFY(ch, i) > 0)
			mysqlWrite(notifylist, TABLE_PLAYER_NOTIFYLIST, GET_IDNUM(ch), GET_NOTIFY(ch, i));

	/* Sanity check */
	mysqlWrite("DELETE FROM %s WHERE Name = '%s' AND ID != %ld;", TABLE_PLAYER_INDEX, esc_name, GET_IDNUM(ch));

	if (!(mysqlWrite(
		replace,
		TABLE_PLAYER_INDEX,
		esc_name,
		GET_PASSWD(ch),
		GET_EMAIL(ch),
		esc_title,
		esc_desc,
		GET_SEX(ch),
		GET_CLASS(ch),
		GET_RACE(ch),
		GET_HOME(ch),
		ch->player.time.birth,
		system ? ch->player.time.played : ch->player.time.played + (long) (time(0) - ch->player.time.logon),
		system ? ch->player.time.logon : time(0),
		((ch->player_specials->saved.bday_year == 0)?(time_info.year - 17):ch->player_specials->saved.bday_year),
		((ch->player_specials->saved.bday_year == 0)?time_info.month:ch->player_specials->saved.bday_month),
		((ch->player_specials->saved.bday_year == 0)?time_info.day:ch->player_specials->saved.bday_day),
		((ch->player_specials->saved.bday_year == 0)?time_info.hours:ch->player_specials->saved.bday_hours),
		ch->player_specials->host,
		GET_HEIGHT(ch),
		GET_WEIGHT(ch),
		GET_ALIGNMENT(ch),
		GET_IDNUM(ch),
		PLR_FLAGS(ch), /* PLR, Flags */
		player_aff_flags, /* AFF, Affects */
		GET_SAVE(ch, 0),
		GET_SAVE(ch, 1),
		GET_SAVE(ch, 2),
		GET_SAVE(ch, 3),
		GET_SAVE(ch, 4),
		GET_WIMP_LEV(ch),
		player_invis_level,  /* GET_INVIS_LEV(ch) */
		GET_LOADROOM(ch),
		player_prf_flags, /* PRF, Preferences */
		GET_BAD_PWS(ch),
		GET_COND(ch, FULL),
		GET_COND(ch, THIRST),
		GET_COND(ch, DRUNK),
		GET_PRACTICES(ch),
		GET_STRENGTH(ch),
		GET_AGILITY(ch),
		GET_PRECISION(ch),
		GET_PERCEPTION(ch),
		GET_HEALTH(ch),
		GET_WILLPOWER(ch),
		GET_INTELLIGENCE(ch),
		GET_CHARISMA(ch),
		GET_LUCK(ch),
		GET_ESSENCE(ch),
		GET_HIT(ch),
		GET_MANA(ch),
		GET_MOVE(ch),
		GET_MAX_HIT(ch),
		GET_MAX_MANA(ch),
		GET_MAX_MOVE(ch),
		0,
		GET_GOLD(ch),
		GET_BANK_GOLD(ch),
		GET_EXP(ch),
		GET_HITROLL(ch),
		GET_DAMROLL(ch),
		GET_QUEST(ch),
		NumAffects,
		NumQuests,
		NumSkills,
		esc_contact,
		IS_ACTIVE(ch),
		SPEAKING(ch),
		GET_EYECOLOR(ch),
		GET_HAIRCOLOR(ch),
		GET_HAIRSTYLE(ch),
		GET_SKINTONE(ch),
		player_user_rights,
		player_xsyslog,
		GET_MAILINGLIST(ch),
		GET_TRUENAME(ch),
		GET_COLORPREF_ECHO(ch) ? GET_COLORPREF_ECHO(ch) : 'n',
		GET_COLORPREF_EMOTE(ch) ? GET_COLORPREF_EMOTE(ch) : 'n',
		GET_COLORPREF_POSE(ch) ? GET_COLORPREF_POSE(ch) : 'n',
		GET_COLORPREF_SAY(ch) ? GET_COLORPREF_SAY(ch) : 'C',
		GET_COLORPREF_OSAY(ch) ? GET_COLORPREF_OSAY(ch) : 'c',
		GET_RPXP(ch),
		GET_PAGE_LENGTH(ch),
		GET_ACTIVEDESC(ch),
		GET_OLC_ZONE(ch),
		esc_awaymsg,
		esc_doing,
		esc_approvedby,
		GET_QP(ch),
		esc_contactinfo,
		GET_FATIGUE(ch),
		GET_PIETY(ch),
		GET_REPUTATION(ch),
		GET_SOCIAL_RANK(ch),
		GET_MILITARY_RANK(ch),
		GET_SANITY(ch),
		GET_FLUX(ch),
		GET_MAX_FLUX(ch),
		IS_VERIFIED(ch),
		esc_sdesc,
		esc_ldesc,
		esc_keywords,
		GET_CULTURE(ch),
		GET_SKILLCAP(ch)
	))) {
		extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "Error writing player to database.");
		return;
	}

	SQL_FREE(esc_name);
	SQL_FREE(esc_desc);
	SQL_FREE(esc_title);
	SQL_FREE(esc_contact);
	SQL_FREE(esc_awaymsg);
	SQL_FREE(esc_doing);
	SQL_FREE(esc_approvedby);
	SQL_FREE(esc_contactinfo);
	SQL_FREE(esc_sdesc);
	SQL_FREE(esc_ldesc);
	SQL_FREE(esc_keywords);

	/* update player_table for this char */
	if((id = find_name(GET_NAME(ch))) < 0)
		return;
	player_table[id].last = time(0);
	player_table[id].rights = USER_RIGHTS(ch);


	/* more char_to_store code to restore affects */
	
	/* add spell and eq affections back in now */
	for (i = 0; i < MAX_AFFECT; i++) {
		if (tmp_aff[i].type)
			affect_to_char(ch, &tmp_aff[i]);
	}
	
	for (i = 0; i < NUM_WEARS; i++) {
		if (char_eq[i])
			equip_char(ch, char_eq[i], i);
	}
	
	/* end char_to_store code */
	
}


/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int	create_entry(char *name)
{
	int i, pos;

	if (top_of_p_table == -1) {        /* no table */
		CREATE(player_table, struct player_index_element, 1);
		pos = top_of_p_table = 0;
	} else if ((pos = get_ptable_by_name(name)) == -1) {        /* new name */
		i = ++top_of_p_table + 1;

		RECREATE(player_table, struct player_index_element, i);
		pos = top_of_p_table;
	}

	CREATE(player_table[pos].name, char, strlen(name) + 1);

	/* copy lowercase equivalent of name to table field */
	for (i = 0; (player_table[pos].name[i] = LOWER(name[i])); i++)
				/* Nothing */;

	return (pos);
}



/************************************************************************
*	 funcs of a (more or less) general utility nature                        *
************************************************************************/


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, const char *error)
{
	char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
	register char *point;
	int done = 0, length = 0, templength;

	*buf = '\0';

	do {
		if (!fgets(tmp, 512, fl)) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "fread_string: format error at or near %s", error);
			kill_mysql();
			exit(1);
		}
		/* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
		if ((point = strchr(tmp, '~')) != NULL) {
			*point = '\0';
			done = 1;
		} else {
			point = tmp + strlen(tmp) - 1;
			*(point++) = '\r';
			*(point++) = '\n';
			*point = '\0';
		}

		templength = strlen(tmp);

		if (length + templength >= MAX_STRING_LENGTH) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "fread_string: string too large (db.c)");
			mlog(error);
			kill_mysql();
			exit(1);
		} else {
			strcat(buf + length, tmp);
			length += templength;
		}
	} while (!done);

	/* allocate space for the new string and copy it */
	if (strlen(buf) > 0) {
		CREATE(rslt, char, length + 1);
		strcpy(rslt, buf);
	} else
		rslt = NULL;

	return (rslt);
}


/* release memory allocated for a char struct */
void free_char(struct char_data *ch)
{
	int i;
	struct alias_data *a;

	if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
		while ((a = GET_ALIASES(ch)) != NULL) {
			GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
			free_alias(a);
		}

		free_travels(ch);

		for (i = 0; i < NUM_DESCS; i++)
			if (GET_RPDESCRIPTION(ch, i))
				release_memory(GET_RPDESCRIPTION(ch, i));

		for (i = 0; i < RPLOG_SIZE; i++)
			if (GET_RPLOG(ch, i))
				release_memory(GET_RPLOG(ch, i));

		for (i = 0; i < TELLS_SIZE; i++)
			if (GET_TELLS(ch, i))
				release_memory(GET_TELLS(ch, i));

		if (GET_KEYWORDS(ch))
			free(GET_KEYWORDS(ch));

		if (GET_DOING(ch))
			free(GET_DOING(ch));

		if (GET_CONTACTINFO(ch))
			free(GET_CONTACTINFO(ch));

		if (GET_CONTACT(ch))
			free(GET_CONTACT(ch));

		if (ch->player_specials->host)
			free(ch->player_specials->host);
		free(ch->player_specials);
		if (IS_NPC(ch))
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
	}
	if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == NOBODY)) {
		/* if this is a player, or a non-prototyped non-player, free all */
		if (GET_NAME(ch))
			free(GET_NAME(ch));
		if (ch->player.title)
			free(ch->player.title);
		if (ch->player.short_descr)
			free(ch->player.short_descr);
		if (ch->player.long_descr)
			free(ch->player.long_descr);
		if (ch->player.description)
			free(ch->player.description);
		if (ch->completed_quests)
			free(ch->completed_quests);
	} else if ((i = GET_MOB_RNUM(ch)) != NOBODY) {
		/* otherwise, free strings only if the string is not pointing at proto */
#if BUFFER_MEMORY == 0
		if (ch->player.name && ch->player.name != mob_proto[i].player.name)
			free(ch->player.name);
		if (ch->player.title && ch->player.title != mob_proto[i].player.title)
			free(ch->player.title);
		if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
			free(ch->player.short_descr);
		if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
			free(ch->player.long_descr);
		if (ch->player.description && ch->player.description != mob_proto[i].player.description)
			free(ch->player.description);
#endif
	}
	while (ch->affected)
		affect_remove(ch, ch->affected);

	if (ch->desc)
		ch->desc->character = NULL;

	free(ch);
}


/* release memory allocated for an obj struct */
void free_obj(struct obj_data *obj)
{
	if (GET_OBJ_RNUM(obj) == NOWHERE)
		free_object_strings(obj);
	else
		free_object_strings_proto(obj);

	free(obj);
}


/*
 * Steps:
 *   1: Make sure no one is using the pointer in paging.
 *   2: Read contents of a text file.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * str_dup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 */
/* query_to_string_alloc for MySQL by Torgny Bjers, 2001-09-15. */
int	query_to_string_alloc(const char *name, char **buf)
{
	char temp[MAX_STRING_LENGTH];
	struct descriptor_data *in_use;
	int temppage;

	/* Lets not free() what used to be there unless we succeeded. */
	if (query_to_string(name, temp) < 0)
		return (-1);

	for (in_use = descriptor_list; in_use; in_use = in_use->next) {
		if (!in_use->showstr_count || *in_use->showstr_vector != *buf)
			continue;

		/* Let's be nice and leave them at the page they were on. */
		temppage = in_use->showstr_page;
		paginate_string((in_use->showstr_head = str_dup(*in_use->showstr_vector)), in_use);
		in_use->showstr_page = temppage;
	}

	if (*buf)
		free(*buf);

	*buf = str_dup(temp);
	return (0);
}


/* read contents of query, and place in buf */
/* query_to_string for MySQL by Torgny Bjers, 2001-09-15. */
int	query_to_string(const char *name, char *buf)
{
	MYSQL_RES *result;
	MYSQL_ROW row;

	*buf = '\0';

	if (!(result = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT name, text FROM %s WHERE name = '%s';", TABLE_TEXT, name))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not load player text from table [%s].", TABLE_TEXT);
		return (-1);
	}

	if (!(row = mysql_fetch_row(result))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Query error in %s trying to fetch '%s' (%s)%s:%d: %s", TABLE_TEXT, name, __FILE__, __FUNCTION__, __LINE__, mysql_error(&mysql));
		return (-1);
	}

	strcat(buf, row[1]);

	if (strlen(buf) > MAX_STRING_LENGTH) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "%s: string too big (%d max)", name, MAX_STRING_LENGTH);
		*buf = '\0';
		mysql_free_result(result);
		return (-1);
	}

	mysql_free_result(result);

	return (0);
}


/* clear some of the the working variables of a char */
void reset_char(struct char_data *ch)
{
	int i;

	for (i = 0; i < NUM_WEARS; i++)
		GET_EQ(ch, i) = NULL;

	ch->followers = NULL;
	ch->master = NULL;
	IN_ROOM(ch) = NOWHERE;
	ch->carrying = NULL;
	ch->next = NULL;
	ch->next_fighting = NULL;
	ch->next_in_room = NULL;
	FIGHTING(ch) = NULL;
	ch->char_specials.position = POS_STANDING;
	ch->mob_specials.default_pos = POS_STANDING;
	ch->char_specials.carry_weight = 0;
	ch->char_specials.carry_items = 0;

	if (GET_HIT(ch) <= 0)
		GET_HIT(ch) = 1;
	if (GET_MOVE(ch) <= 0)
		GET_MOVE(ch) = 1;
	if (GET_MANA(ch) <= 0)
		GET_MANA(ch) = 1;

	GET_LAST_TELL(ch) = NOBODY;
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data *ch)
{
	memset((char *) ch, 0, sizeof(struct char_data));

	IN_ROOM(ch) = NOWHERE;
	GET_PFILEPOS(ch) = -1;
	GET_MOB_RNUM(ch) = NOBODY;
	GET_WAS_IN(ch) = NOWHERE;
	GET_POS(ch) = POS_STANDING;
	ch->mob_specials.default_pos = POS_STANDING;

	GET_PD(ch) = 0;	/* Basic Armor */
	if (ch->points.max_mana < 100)
		ch->points.max_mana = 100;
}


void clear_object(struct obj_data *obj)
{
	memset((char *) obj, 0, sizeof(struct obj_data));

	obj->item_number = NOTHING;
	obj->proto_number = NOTHING;
	IN_ROOM(obj) = NOWHERE;
	obj->worn_on = NOWHERE;
}




/* initialize a new character only if race and class are set */
void init_char(struct char_data *ch)
{
	int i;

	/* create a player_special structure */
	if (ch->player_specials == NULL)
		CREATE(ch->player_specials, struct player_special_data, 1);

	/* *** if this is our first player --- he be Implementor *** */

	if (top_of_p_table == 0) {
		GET_EXP(ch) = EXP_MAX;
			/* Set player to have full user rights. (TB) */
		IS_VERIFIED(ch) = TRUE;
		USER_RIGHTS(ch) = RIGHTS_MEMBER;
		USER_RIGHTS(ch) |= RIGHTS_QUESTS;
		USER_RIGHTS(ch) |= RIGHTS_GUILDS;
		USER_RIGHTS(ch) |= RIGHTS_NEWBIES;
		USER_RIGHTS(ch) |= RIGHTS_IMMORTAL;
		USER_RIGHTS(ch) |= RIGHTS_ACTIONS;
		USER_RIGHTS(ch) |= RIGHTS_HELPFILES;
		USER_RIGHTS(ch) |= RIGHTS_BUILDING;
		USER_RIGHTS(ch) |= RIGHTS_QUESTOR;
		USER_RIGHTS(ch) |= RIGHTS_TRIGGERS;
		USER_RIGHTS(ch) |= RIGHTS_ADMIN;
		USER_RIGHTS(ch) |= RIGHTS_PLAYERS;
		USER_RIGHTS(ch) |= RIGHTS_HEADBUILDER;
		USER_RIGHTS(ch) |= RIGHTS_DEVELOPER;
		USER_RIGHTS(ch) |= RIGHTS_IMPLEMENTOR;
		USER_RIGHTS(ch) |= RIGHTS_OWNER;
		ch->points.max_hit = 500;
		ch->points.max_mana = 500;
		ch->points.max_move = 500;
		ch->points.max_flux = 500;
		for (i = 1; i < MAX_SKILLS; i++) {
			SET_SKILL(ch, i, 10000);
		}
	} else {
		/* Set player to have no rights until after enter_player_game (CG) */
		USER_RIGHTS(ch) = RIGHTS_NONE;
	}

	SPEAKING(ch) = LANG_COMMON;
	set_title(ch, NULL);
	ch->player.short_descr = NULL;
	ch->player.long_descr = NULL;

	GET_RPDESCRIPTION(ch, 0) = NULL;
	GET_CONTACTINFO(ch) = NULL;
	
	ch->player.description = NULL;

	ch->player.hometown = 0;

	ch->player.time.birth = time(0);
	ch->player.time.played = 0;
	ch->player.time.logon = time(0);

	ch->player_specials->saved.bday_year = time_info.year - 17;
	ch->player_specials->saved.bday_month = time_info.month;
	ch->player_specials->saved.bday_day = time_info.day;
	ch->player_specials->saved.bday_hours = time_info.hours;
	ch->player_specials->saved.doing = NULL;

	for (i = 0; i < MAX_TONGUE; i++)
		GET_TALK(ch, i) = 0;

	ch->points.max_mana = 100;
	ch->points.mana = GET_MAX_MANA(ch);
	ch->points.hit = GET_MAX_HIT(ch);
	ch->points.max_move = 82;
	ch->points.move = GET_MAX_MOVE(ch);
	ch->points.max_flux = 0;
	ch->points.flux = GET_MAX_FLUX(ch);
	ch->points.armor = 0;
	ch->points.skillcap = 80000;

	if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
		player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
	else if (GET_PFILEPOS(ch) > 0)
		GET_IDNUM(ch) = player_table[GET_PFILEPOS(ch)].id;
	else
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "init_char: Character '%s' not found in player table.", GET_NAME(ch));

	/* make sure the player table rights match the character's rights */
	if (player_table[i].rights != USER_RIGHTS(ch))
		player_table[i].rights = USER_RIGHTS(ch);

	for (i = 1; i <= MAX_SKILLS; i++) {
		if (!GOT_RIGHTS(ch, RIGHTS_IMMORTAL))
			SET_SKILL(ch, i, 0);
		else
			SET_SKILL(ch, i, 10000);
	}

	ch->char_specials.saved.affected_by = 0;

	for (i = 0; i < 5; i++)
		GET_SAVE(ch, i) = 0;

	for (i = 0; i < 3; i++)
		GET_COND(ch, i) = (IS_IMPLEMENTOR(ch) ? -1 : 24);

	GET_LOADROOM(ch) = NOWHERE;

	/* Cheron: 9.13.01, setting prompt default to all */
	SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_HASPROMPT);

	/* Artovil: 5.28.02, setting primary toggles */
	SET_BIT(PRF_FLAGS(ch), PRF_SKILLGAINS | PRF_AUTOEXIT);

	GET_QUEST(ch) = -1;
	GET_NUM_QUESTS(ch) = -1;
	ch->completed_quests = NULL;
	ch->num_completed_quests = 0;
	GET_INVIS_LEV(ch) = RIGHTS_NONE;

	GET_COLORPREF_ECHO(ch) = 'n';
	GET_COLORPREF_EMOTE(ch) = 'n';
	GET_COLORPREF_POSE(ch) = 'n';
	GET_COLORPREF_SAY(ch) = 'C';
	GET_COLORPREF_OSAY(ch) = 'c';

	GET_PAGE_LENGTH(ch) = PAGE_LENGTH;

}



/* returns the real number of the room with given virtual number */
room_rnum	real_room(room_vnum vnum)
{
	room_rnum bot, top, mid;

	bot = 0;
	top = top_of_world;

	/* perform binary search on world-table */
	for (;;) {
		mid = (bot + top) / 2;

		if ((world + mid)->number == vnum)
			return (mid);
		if (bot >= top)
			return (NOWHERE);
		if ((world + mid)->number > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}



/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
	mob_rnum bot, top, mid;

	bot = 0;
	top = top_of_mobt;

	/* perform binary search on mob-table */
	for (;;) {
		mid = (bot + top) / 2;

		if ((mob_index + mid)->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (NOBODY);
		if ((mob_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}



/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
	obj_rnum bot, top, mid;

	bot = 0;
	top = top_of_objt;

	/* perform binary search on obj-table */
	for (;;) {
		mid = (bot + top) / 2;

		if ((obj_index + mid)->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (NOTHING);
		if ((obj_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}


/*
 * Extend later to include more checks.
 * TODO: Add checks for unknown bitvectors.
 */
int	check_object(struct obj_data *obj)
{
	char *objname = get_buffer(1024);
	int error = FALSE;

	if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Object #%d (%s) has negative weight (%d).",
				GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_WEIGHT(obj));

	if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Object #%d (%s) has negative cost/day (%d).",
				GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_RENT(obj));

	sprintf(objname, "Object #%d (%s)", GET_OBJ_VNUM(obj), obj->short_description);
	error |= check_bitvector_names(GET_OBJ_WEAR(obj), wear_bits_count, objname, "object wear");
	error |= check_bitvector_names(GET_OBJ_EXTRA(obj), extra_bits_count, objname, "object extra");
	error |= check_bitvector_names(GET_OBJ_AFFECT(obj), affected_bits_count, objname, "object affect");

	release_buffer(objname);

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_DRINKCON:
	{
		char onealias[MAX_INPUT_LENGTH], *space = strrchr(obj->name, ' ');

		strcpy(onealias, space ? space + 1 : obj->name);
		if (search_block(onealias, drinknames, TRUE) < 0 && (error = TRUE))
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Object #%d (%s) doesn't have drink type as last alias. (%s)",
							 GET_OBJ_VNUM(obj), obj->short_description, obj->name);
	}
	/* Fall through. */
	case ITEM_FOUNTAIN:
		if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE))
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Object #%d (%s) contains (%d) more than maximum (%d).",
								GET_OBJ_VNUM(obj), obj->short_description,
								GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
		break;
	case ITEM_SCROLL:
	case ITEM_POTION:
		error |= check_object_level(obj, 0);
		error |= check_object_spell_number(obj, 1);
		error |= check_object_spell_number(obj, 2);
		error |= check_object_spell_number(obj, 3);
		break;
	case ITEM_WAND:
	case ITEM_STAFF:
		error |= check_object_level(obj, 0);
		error |= check_object_spell_number(obj, 3);
		if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Object #%d (%s) has more charges (%d) than maximum (%d).",
								GET_OBJ_VNUM(obj), obj->short_description,
								GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
		break;
 }

	return (error);
}


int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits)
{
	unsigned int flagnum;
	bool error = FALSE;

	/* See if any bits are set above the ones we know about. */
	if (bits <= (~(bitvector_t)0 >> (sizeof(bitvector_t) * 8 - namecount)))
		return (FALSE);

	for (flagnum = namecount; flagnum < sizeof(bitvector_t) * 8; flagnum++)
		if ((1ULL << flagnum) & bits) {
			mlog("SYSERR: %s has unknown %s flag, bit %d (0 through %d known).", whatami, whatbits, flagnum, namecount - 1);
			error = TRUE;
		}

	return (error);
}


int	check_object_spell_number(struct obj_data *obj, int val)
{
	int error = FALSE;

	if (GET_OBJ_VAL(obj, val) == -1)        /* i.e.: no spell */
		return (error);

	/*
	 * Check for negative spells, spells beyond the top define, and any
	 * spell which is actually a skill.
	 */
	if (GET_OBJ_VAL(obj, val) < 0)
		error = TRUE;
	if (GET_OBJ_VAL(obj, val) > top_of_spellt)
		error = TRUE;
	if (error)
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Object #%d (%s) has out of range spell #%d.",
				GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

	/*
	 * This bug has been fixed, but if you don't like the special behavior...
	 */
#if	0
	if (GET_OBJ_TYPE(obj) == ITEM_STAFF &&
				HAS_SPELL_ROUTINE(get_spell(GET_OBJ_VAL(obj, val), __FILE__, __FUNCTION__), MAG_AREAS | MAG_MASSES))
		mlog("... '%s' (#%d) uses %s spell '%s'.",
				obj->short_description,        GET_OBJ_VNUM(obj),
				HAS_SPELL_ROUTINE(get_spell(GET_OBJ_VAL(obj, val), __FILE__, __FUNCTION__), MAG_AREAS) ? "area" : "mass",
				spell_name(GET_OBJ_VAL(obj, val)));
#endif

	if (scheck)                /* Spell names don't exist in syntax check mode. */
		return (error);

#if	0
	/* Now check for unnamed spells. */
	spellname = spell_name(GET_OBJ_VAL(obj, val));

	if ((spellname == unused_spellname || !str_cmp("UNDEFINED", spellname)) && (error = TRUE))
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Object #%d (%s) uses '%s' spell #%d.",
								GET_OBJ_VNUM(obj), obj->short_description, spellname,
								GET_OBJ_VAL(obj, val));
#endif

	return (error);
}


int	check_object_level(struct obj_data *obj, int val)
{
	int error = FALSE;

	if ((GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > 100) && (error = TRUE))
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Object #%d (%s) has out of range level #%d.",
				GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

	return (error);
}


int check_mobile_values(struct char_data *mob)
{
	int error = FALSE;

	if ((GET_MOB_VAL(mob, 0) && real_object(GET_MOB_VAL(mob, 0)) < 0) && (error = TRUE))
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Mobile #%d (%s) has non-existant corpse object #%d.",
				GET_MOB_VNUM(mob), mob->player.short_descr, GET_MOB_VAL(mob, 0));
	if ((GET_MOB_VAL(mob, 1) < 0 || GET_MOB_VAL(mob, 1) > 10) && (error = TRUE))
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Mobile #%d (%s) has out of range resource number #%d.",
				GET_MOB_VNUM(mob), mob->player.short_descr, GET_MOB_VAL(mob, 1));
	if ((GET_MOB_VAL(mob, 2) && real_object(GET_MOB_VAL(mob, 2)) < 0) && (error = TRUE))
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Mobile #%d (%s) has non-existant resource #1 object #%d.",
				GET_MOB_VNUM(mob), mob->player.short_descr, GET_MOB_VAL(mob, 2));
	if ((GET_MOB_VAL(mob, 3) && real_object(GET_MOB_VAL(mob, 3)) < 0) && (error = TRUE))
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Mobile #%d (%s) has non-existant resource #2 object #%d.",
				GET_MOB_VNUM(mob), mob->player.short_descr, GET_MOB_VAL(mob, 3));

	return (error);
}

/* new functions used by ascii pfiles */

/* This is a general purpose function originally from obuild OLC,
	 and there is probably a similar function in Oasis.  Feel free to
	 remove this and use another
*/

/* Change BITSIZE to 1LL for bitvector_t of long long*/
#ifdef BITSIZE
#undef BITSIZE
#endif

#define BITSIZE 1LL

#ifdef BITSET
#undef BITSET
#endif

#define BITSET(bit_pattern, bit) (bit_pattern & (BITSIZE << bit))

int sprintascii(char *out, bitvector_t bits)
{
  int i, j = 0;
  /* 
   * 64 bits, don't use past 'F' unless your
   * bitvector_t is larger than 32 bits
   * Also modify the BITSIZE define above if using 64 bit vectors
   */
  char *flags="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ:;<=>?[\\]^_`";

  for (i = 0; flags[i] != '\0'; i++)
    if (BITSET(bits, i)) 
      out[j++] = flags[i];

  if (j == 0) /* Didn't write anything. */
    out[j++] = '0';

  /* NUL terminate the output string. */
  out[j++] = '\0'; 
  return j;
}


/* remove_player removes all files associated with a player who is
	 self-deleted, deleted by an immortal, or deleted by the auto-wipe
	 system (if enabled).  If you add more character files, you'll want
	 to put a remover here.
*/ 
/* New version of remove_player for MySQL by Torgny Bjers, 2001-09-15. */
/* Let's try something a little more robust... actually cleans out all entries
 * associated with this player - Cathy Gore, 2004-03-05.  
 */
void remove_player(int pfilepos)
{
  char filename[60];
  char *esc_name, *pbuf = get_buffer(256);
  long playerID = -1;
  room_vnum houseID = -1;
  //room_vnum boardID = -1;
  MYSQL_RES *result;
	MYSQL_ROW row;


	if(!*player_table[pfilepos].name)
		return;

	SQL_MALLOC(player_table[pfilepos].name, esc_name);	
	SQL_ESC(player_table[pfilepos].name, esc_name);

	/* Make sure we've got the right player ID */
	if (!(result = mysqlGetResource(TABLE_PLAYER_INDEX, "SELECT ID FROM %s WHERE ID = %ld OR Name = '%s';", TABLE_PLAYER_INDEX, player_table[pfilepos].id, esc_name)))
		return;
	if ((row = mysql_fetch_row(result))) {
		playerID = atoi(row[0]);
	}
	/* Free loaded result to conserve memory */
	mysql_free_result(result);

	sprintf(pbuf, "%s Last: %s", player_table[pfilepos].name, ctime(&player_table[pfilepos].last));
	extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "PCLEAN: %s", pbuf);
	

	/* Child tables first */
	/* player information tables */
	mysqlWrite("DELETE FROM %s WHERE playerid = %ld;", TABLE_VERIFICATION, playerID); // pending
	mysqlWrite("DELETE FROM %s WHERE id = %ld;", TABLE_PLAYER_AFFECTS, playerID); // affects
	mysqlWrite("DELETE FROM %s WHERE playerid = %ld;", TABLE_PLAYER_QUESTS, playerID); // quests
	mysqlWrite("DELETE FROM %s WHERE playerid = %ld OR memid = %ld;", TABLE_PLAYER_RECOGNIZED, playerID, playerID); // recognized
	mysqlWrite("DELETE FROM %s WHERE playerid = %ld;", TABLE_PLAYER_RPDESCRIPTIONS, playerID); // rpdescs
	mysqlWrite("DELETE FROM %s WHERE playerid = %ld;", TABLE_PLAYER_RPLOGS, playerID); // rplogs
	mysqlWrite("DELETE FROM %s WHERE id = %ld;", TABLE_PLAYER_SKILLS, playerID); // skills
	mysqlWrite("DELETE FROM %s WHERE playerid = %ld OR notifyid = %ld;", TABLE_PLAYER_NOTIFYLIST, playerID, playerID); // notifylist

	/* player board data */
/*if ((result = mysqlGetResource(TABLE_DYNABOARD_INDEX, "SELECT vnum FROM %s WHERE owner = %ld;", TABLE_HOUSE_INDEX, playerID))) {
		while ((row = mysql_fetch_row(result))) {
			boardID = atoi(row[0]);
      mysqlWrite("DELETE FROM %s WHERE board_id = %d;", TABLE_DYNABOARD_POSTS, boardID); // posts on player-owned boards
      mysqlWrite("DELETE FROM %s WHERE board_id = %d;", TABLE_DYNABOARD_UPDATES, boardID); // updates on player-owned boards 
		}
	}
	mysqlWrite("DELETE FROM %s WHERE owner = %ld;", TABLE_DYNABOARD_INDEX, playerID); // boards 
	mysqlWrite("DELETE FROM %s WHERE player_id = %ld;", TABLE_DYNABOARD_POSTS, playerID); // posts 
	mysqlWrite("DELETE FROM %s WHERE player_id = %ld;", TABLE_DYNABOARD_UPDATES, playerID); // updates
*/

	/* player guild data */
	mysqlWrite("DELETE FROM %s WHERE idnum = %ld;", TABLE_GUILD_GUILDIES, playerID); // guildie
	mysqlWrite("DELETE FROM %s WHERE idnum = %ld OR name='%s';", TABLE_GUILD_SPONSORER, playerID, esc_name); // sponsors

	/* player house data */
  mysqlWrite("DELETE FROM %s WHERE player = %ld;", TABLE_HOUSE_COWNERS, playerID); // co-owner
  mysqlWrite("DELETE FROM %s WHERE player = %ld;", TABLE_HOUSE_GUESTS, playerID); // guest

  /* Get the house IDs and remove them */
	if ((result = mysqlGetResource(TABLE_HOUSE_INDEX, "SELECT vnum FROM %s WHERE owner = %ld;", TABLE_HOUSE_INDEX, playerID))) {
		while ((row = mysql_fetch_row(result))) {
			houseID = atoi(row[0]);
      house_delete_file(houseID);
		}
	}
	/* Free loaded result to conserve memory */
	mysql_free_result(result);

	/* player rent files */
  mysqlWrite("DELETE FROM %s WHERE playerID = %ld;", TABLE_RENT_PLAYERS, playerID); // rent
  mysqlWrite("DELETE FROM %s WHERE player_id = %ld;", TABLE_RENT_OBJECTS, playerID); // rent objects
  mysqlWrite("DELETE FROM %s WHERE player_id = %ld;", TABLE_RENT_AFFECTS, playerID); // rent affects
  mysqlWrite("DELETE FROM %s WHERE player_id = %ld;", TABLE_RENT_EXTRADESCS, playerID); // rent xdescs

	/* Now the parent table */
	mysqlWrite("DELETE FROM %s WHERE ID = %ld OR Name = '%s';", TABLE_PLAYER_INDEX, playerID, esc_name);

	SQL_FREE(esc_name);

	/* Unlink any other player-owned files here if you have them  */

	/* Delete the player travels file */
	get_filename(player_table[pfilepos].name, filename, TRAV_FILE);
	remove(filename);

	/* Delete the player alias file */
	get_filename(player_table[pfilepos].name, filename, ALIAS_FILE);
	remove(filename);

	player_table[pfilepos].name[0] = '\0';
	player_table[pfilepos].active = FALSE;
	release_buffer(pbuf);
}


void clean_players(int preview)
{
	int i, ci, timeout;
	char *pbuf = get_buffer(256);

	for(i = 0; i <= top_of_p_table; i++) {
		if(IS_SET(player_table[i].flags, PINDEX_NODELETE))
			continue;
		timeout = -1;
		for(ci = 0; ci == 0 || (pclean_criteria[ci].rights > 
				pclean_criteria[ci - 1].rights); ci++) {
			if((pclean_criteria[ci].rights == RIGHTS_NONE && IS_SET(player_table[i].flags, PINDEX_DELETED)) ||
					player_table[i].rights <= pclean_criteria[ci].rights) {
				timeout = pclean_criteria[ci].days;
				break;
			}
		}
		if(timeout >= 0) {
			timeout *= SECS_PER_REAL_DAY;
			if((time(0) - player_table[i].last) > timeout && *player_table[i].name) {
				if (preview) {
					sprintf(pbuf, "%s Last: %s", player_table[i].name, ctime(&player_table[i].last));
					extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "PCLEAN PREVIEW: %s", pbuf);
				} else {
					remove_player(i);
				}
			}
		}
	}
	release_buffer(pbuf);
}


/*
 * Boot tip messages from MySQL. Added 2001-11-07 by Torgny Bjers.
 * This function is an almost direct copy of the new build_player_index.
 */
void boot_tip_messages(void)
{
	MYSQL_RES *tipmessages;
	MYSQL_ROW row;
	int rec_count = 0, i;
	unsigned long *fieldlength;

	if (!(tipmessages = mysqlGetResource(TABLE_TIP_INDEX, "SELECT title, tip, rights FROM %s ORDER BY title ASC;", TABLE_TIP_INDEX)))
		return;

	rec_count = mysql_num_rows(tipmessages);
	mlog("   %d tip messages in database.", rec_count);

	if(rec_count == 0) {
		tip_table = NULL;
		top_of_tip_table = -1;
		return;
	}

	CREATE(tip_table, struct tip_index_element, rec_count);
	for(i = 0; i < rec_count; i++)
	{
		row = mysql_fetch_row(tipmessages);
		fieldlength = mysql_fetch_lengths(tipmessages);
		tip_table[i].rights = asciiflag_conv(row[2]);
		CREATE(tip_table[i].title, char, fieldlength[0] + 1);
		strcpy(tip_table[i].title, row[0]);
		CREATE(tip_table[i].tip, char, fieldlength[1] + 1);
		strcpy(tip_table[i].tip, row[1]);
	}

	top_of_tip_table = i - 1;

	/* Free up result sets to conserve memory. */
	mysql_free_result(tipmessages);

}


/*
 * Load help categories from MySQL. Added 2001-11-11 by Torgny Bjers.
 * This function is an almost direct copy of the new build_player_index.
 */
void boot_help_categories(void)
{
	MYSQL_RES *categories;
	MYSQL_ROW row;
	int rec_count = 0, i;
	unsigned long *fieldlength;

	if (!(categories = mysqlGetResource(TABLE_HLP_CATEGORIES, "SELECT id, name, rights, text FROM %s ORDER BY sortorder ASC;", TABLE_HLP_CATEGORIES))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error opening help categories from table [%s].", TABLE_HLP_CATEGORIES);
		kill_mysql();
		exit(1);
	}

	rec_count = mysql_num_rows(categories);
	mlog("   %d help categories in database.", rec_count);

	if(rec_count == 0) {
		help_categories = NULL;
		top_of_help_categories = -1;
		return;
	}

	CREATE(help_categories, struct help_categories_element, rec_count);
	for(i = 0; i < rec_count; i++)
	{
		row = mysql_fetch_row(categories);
		fieldlength = mysql_fetch_lengths(categories);
		help_categories[i].num = atoi(row[0]);
		help_categories[i].rights = asciiflag_conv(row[2]);
		CREATE(help_categories[i].name, char, fieldlength[1] + 1);
		strcpy(help_categories[i].name, row[1]);
		CREATE(help_categories[i].text, char, fieldlength[3] + 1);
		strcpy(help_categories[i].text, row[3]);
	}

	top_of_help_categories = i - 1;

	/* Free up result sets to conserve memory. */
	mysql_free_result(categories);

}


/*
 * Load puff messages from MySQL. Added 2002-05-20 by Torgny Bjers.
 * This function is an almost direct copy of the new build_player_index.
 */
void boot_puff_messages(void)
{
	MYSQL_RES *messages;
	MYSQL_ROW row;
	int rec_count = 0, i;
	unsigned long *fieldlength;

	if (!(messages = mysqlGetResource(TABLE_PUFF_MESSAGES, "SELECT Message FROM %s;", TABLE_PUFF_MESSAGES))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error opening puff messages from table [%s].", TABLE_PUFF_MESSAGES);
		kill_mysql();
		exit(1);
	}

	rec_count = mysql_num_rows(messages);
	mlog("   %d puff messages in database.", rec_count);

	if(rec_count == 0) {
		puff_messages = NULL;
		top_of_puff_messages = -1;
		return;
	}

	CREATE(puff_messages, struct puff_messages_element, rec_count);
	for(i = 0; i < rec_count; i++)
	{
		row = mysql_fetch_row(messages);
		fieldlength = mysql_fetch_lengths(messages);
		CREATE(puff_messages[i].message, char, fieldlength[0] + 1);
		strcpy(puff_messages[i].message, row[0]);
	}

	top_of_puff_messages = i - 1;

	/* Free up result sets to conserve memory. */
	mysql_free_result(messages);

}


/* Magical Flux related functions */
void reset_room_flux(struct room_data *room)
{
	int flux = 0;
	
	switch (room->magic_flux) {
	case FLUX_POOR: flux = 5; break;
	case FLUX_TYPICAL: flux = 10; break;
	case FLUX_HIGH: flux = 15; break;
	case FLUX_RICH: flux = 20; break;
	case FLUX_DEAD:
	default:
		flux = 0;
		break;
	}
	if (IS_REGIO(real_room(room->number)))
		flux *= room->sector_type / 2; // 10-13 / 2
	room->available_flux = flux;
}


/* Magical Flux related functions */
void reset_room_resources(struct room_data *room)
{
	room_rnum rnum;
	rnum = real_room(room->number);
	if (GET_ROOM_RESOURCE(rnum, RESOURCE_ORE) < GET_ROOM_RESOURCE_MAX(rnum, RESOURCE_ORE))
		++GET_ROOM_RESOURCE(rnum, RESOURCE_ORE);
	if (GET_ROOM_RESOURCE(rnum, RESOURCE_GEMS) < GET_ROOM_RESOURCE_MAX(rnum, RESOURCE_GEMS))
		++GET_ROOM_RESOURCE(rnum, RESOURCE_GEMS);
	if (GET_ROOM_RESOURCE(rnum, RESOURCE_WOOD) < GET_ROOM_RESOURCE_MAX(rnum, RESOURCE_WOOD))
		++GET_ROOM_RESOURCE(rnum, RESOURCE_WOOD);
	if (GET_ROOM_RESOURCE(rnum, RESOURCE_STONE) < GET_ROOM_RESOURCE_MAX(rnum, RESOURCE_STONE))
		++GET_ROOM_RESOURCE(rnum, RESOURCE_STONE);
	if (GET_ROOM_RESOURCE(rnum, RESOURCE_FABRIC) < GET_ROOM_RESOURCE_MAX(rnum, RESOURCE_FABRIC))
		++GET_ROOM_RESOURCE(rnum, RESOURCE_FABRIC);
}


/*
 * Load races from MySQL. Added 2002-06-12 by Torgny Bjers.
 * This function is an almost direct copy of the new build_player_index.
 */
void boot_races(void)
{
	MYSQL_RES *races;
	MYSQL_ROW row;
	int rec_count = 0, i;
	unsigned long *fieldlength;

	/*
	 * ENUM column range begins at 1, but we want it to begin at 0, so we subtract
	 * one from Magic, and Selectable.  The defines for Plane, Realm, and Size all begin
	 * at (-1) which means we have to subtract 2.  Why are we using ENUM?  Because it is
	 * easier to use in mysql queries and in the phpmyadmin interface. (TB, 2002-10-02)
	 */

	/*                                                  0   1             2     3          4               5     6          7          8            9      10        11         12 */
	if (!(races = mysqlGetResource(TABLE_RACES, "SELECT ID, Abbreviation, Name, (Magic)-1, (Selectable)-1, Cost, (Plane)-2, (Realm)-2, Description, Const, (Size)-2, MinAttrib, MaxAttrib FROM %s ORDER BY Const ASC;", TABLE_RACES))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error opening races from table [%s].", TABLE_RACES);
		kill_mysql();
		exit(1);
	}

	rec_count = mysql_num_rows(races);
	mlog("   %d races in database.", rec_count);

	if(rec_count == 0) {
		race_list = NULL;
		top_of_race_list = -1;
		return;
	}

	CREATE(race_list, struct race_list_element, rec_count);
	for(i = 0; i < rec_count; i++)
	{
		row = mysql_fetch_row(races);
		fieldlength = mysql_fetch_lengths(races);
		CREATE(race_list[i].abbr, char, fieldlength[1] + 1);
		strcpy(race_list[i].abbr, row[1]);
		CREATE(race_list[i].name, char, fieldlength[2] + 1);
		strcpy(race_list[i].name, row[2]);
		race_list[i].magic = (bool)atoi(row[3]);
		race_list[i].selectable = (bool)atoi(row[4]);
		race_list[i].cost = atoi(row[5]);
		race_list[i].plane = atoi(row[6]);
		race_list[i].realm = atoi(row[7]);
		CREATE(race_list[i].description, char, fieldlength[8] + 1);
		strcpy(race_list[i].description, row[8]);
		race_list[i].constant = atoi(row[9]);
		race_list[i].size = atoi(row[10]);
		race_list[i].min_attrib = atoi(row[11]);
		race_list[i].max_attrib = atoi(row[12]);
	}

	top_of_race_list = i - 1;

	/* Free up result sets to conserve memory. */
	mysql_free_result(races);

}


/*
 * Load races from MySQL. Added 2002-06-12 by Torgny Bjers.
 * This function is an almost direct copy of the new build_player_index.
 */
void boot_cultures(void)
{
	MYSQL_RES *cultures;
	MYSQL_ROW row;
	int rec_count = 0, i;
	unsigned long *fieldlength;

	/*                                                     0   1             2     3               4     5            6 */
	if (!(cultures = mysqlGetResource(TABLE_RACES, "SELECT ID, Abbreviation, Name, (Selectable)-1, Cost, Description, Const FROM %s ORDER BY Const ASC;", TABLE_CULTURES))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Error opening cultures from table [%s].", TABLE_CULTURES);
		kill_mysql();
		exit(1);
	}

	rec_count = mysql_num_rows(cultures);
	mlog("   %d cultures in database.", rec_count);

	if(rec_count == 0) {
		culture_list = NULL;
		top_of_culture_list = -1;
		return;
	}

	CREATE(culture_list, struct culture_list_element, rec_count);
	for(i = 0; i < rec_count; i++)
	{
		row = mysql_fetch_row(cultures);
		fieldlength = mysql_fetch_lengths(cultures);
		CREATE(culture_list[i].abbr, char, fieldlength[1] + 1);
		strcpy(culture_list[i].abbr, row[1]);
		CREATE(culture_list[i].name, char, fieldlength[2] + 1);
		strcpy(culture_list[i].name, row[2]);
		culture_list[i].selectable = (bool)atoi(row[3]);
		culture_list[i].cost = atoi(row[4]);
		CREATE(culture_list[i].description, char, fieldlength[5] + 1);
		strcpy(culture_list[i].description, row[5]);
		culture_list[i].constant = atoi(row[6]);
	}
	top_of_culture_list = i - 1;


	/* Free up result sets to conserve memory. */
	mysql_free_result(cultures);

}


/* Separate a 4-character id tag from the data it precedes */
void tag_argument(char *argument, char *tag)
{
	char *tmp = argument, *ttag = tag, *wrt = argument;
	int i;

	for(i = 0; i < 4; i++)
		*(ttag++) = *(tmp++);
	*ttag = '\0';
	
	while(*tmp == ':' || *tmp == ' ')
		tmp++;

	while(*tmp)
		*(wrt++) = *(tmp++);
	*wrt = '\0';
}


/*
 * EMAIL VERIFICATION:
 * Sends the player an email with a PASSWORD() hashed verification code.
 *
 * The email contains an URL that should be formatted like this:
 * http://www.arcanerealms.org/verify_player.php?code=2451fa101a5576b7
 * The hash contains the player username.
 *
 * Once the player goes to the link, their account is validated and they
 * may log into the game providing that other requirements are met already,
 * such as their name and description.
 *
 * nanny() uses this function to check whether or not a player has been
 * verified, providing that use_verification has been turned on.
 *
 * ALTER TABLE `arcanere_g`.`player_index` ADD `Verified` TINYINT(1) UNSIGNED DEFAULT '0' NOT NULL;
 *
 * Torgny Bjers (artovil@arcanerealms.org) 2002-10-07, 14:14:51.
 */

#define	SENDMAIL "/usr/bin/sendmail"

bool email_verification(struct char_data *ch, int subcmd)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	char *esc_name;
	bool success;

	if (!ch || GET_IDNUM(ch) <= 0)
		return (FALSE);

	switch (subcmd) {

	case EMAIL_VERIFY:
		/*
		 * Player needs to be verified.  Send out an email with a
		 * link they should follow in order to activate.
		 */
		SQL_MALLOC(GET_NAME(ch), esc_name);
		SQL_ESC(GET_NAME(ch), esc_name);

		if (mysqlWrite("REPLACE INTO %s (PlayerID, Code) VALUES(%d, PASSWORD('%s'));", TABLE_VERIFICATION, GET_IDNUM(ch), esc_name)) {
			char *application = get_buffer(256);
			char *email = get_buffer(2048);
			FILE *fp;

			SQL_FREE(esc_name);

			if (!(result = mysqlGetResource(TABLE_VERIFICATION, "SELECT Code FROM %s WHERE PlayerID = %ld;", TABLE_VERIFICATION, GET_IDNUM(ch)))) {
				extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not compare player ID on table [%s].", TABLE_VERIFICATION);
				release_buffer(application);
				release_buffer(email);
				return (FALSE);
			}

			/*
			 * Send the verification email to the player.
			 */
			sprintf(application, "%s %s", SENDMAIL, GET_EMAIL(ch));
			fp = popen(application, "w");
			release_buffer(application);

			row = mysql_fetch_row(result);

			snprintf(email, 2048,
				"From: \"Arcane Realms Auto Activation\" <activation@arcanerealms.org>\n"
				"To: \"%s\" <%s>\n"
				"Reply-To: activation@arcanerealms.org\n"
				"X-Mailer: Arcane Realms MUD Mailing System <http://www.arcanerealms.org/>\n"
				"Questions: This e-mail was sent by the Arcane Realms Auto Activation System.\n"
				"Subject: Your code: %s\n"
				"Welcome to Arcane Realms!\n"
				"\n"
				"Please click the link below to activate your account:\n"
				"http://www.arcanerealms.org/verify.php?q=%s&code=%s\n"
				"\n"
				"You have received this email because you, or somebody else, signed up for an account\n"
				"at Arcane Realms.  If you have received this email in error, please email\n"
				"abuse@arcanerealms.org and let us know.\n",
				GET_NAME(ch),
				GET_EMAIL(ch),
				row[0],
				mysql_db,
				row[0]
			);
			fprintf(fp, "%s", email);
			pclose(fp);

			extended_mudlog(BRF, SYSL_SECURE, TRUE, "Emailed auto-activation code %s to \"%s\" <%s>", row[0], GET_NAME(ch), GET_EMAIL(ch));

			mysql_free_result(result);
			release_buffer(email);

			return (TRUE);
		}
		SQL_FREE(esc_name);
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not write auto-activation code to table [%s]", TABLE_VERIFICATION);
		break;

	case EMAIL_CHECK:
		/*
		 * Check if the player has verified.
		 */
		if (!(result = mysqlGetResource(TABLE_VERIFICATION, "SELECT Verified FROM %s WHERE ID = %ld;", TABLE_PLAYER_INDEX, GET_IDNUM(ch)))) {
			extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not check verification code in table [%s].", TABLE_PLAYER_INDEX);
			return (FALSE);
		}
		row = mysql_fetch_row(result);
		success = (bool)atoi(row[0]);
		IS_VERIFIED(ch) = success;
		mysql_free_result(result);
		if (success)
			return (TRUE);
		break;

	default:
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Reached default case in %s:%s(%d)", __FILE__, __FUNCTION__, __LINE__);
		break;
	}

	return (FALSE);
}
