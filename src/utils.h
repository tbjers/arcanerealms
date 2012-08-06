/* ************************************************************************
*		File: utils.h                                       Part of CircleMUD *
*	 Usage: header file: utility macros and prototypes of utility funcs     *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: utils.h,v 1.103 2004/04/23 15:18:10 cheron Exp $ */

/* external declarations and prototypes **********************************/

extern struct weather_data weather_info;
extern FILE *logfile;

#define	mlog			basic_mud_log

/* public functions in utils.c */
int replace(char *str, char *search, const char *repl, int mode);
char *str_str(const char *str, const char *txt);
char *escape_quotes(char *source);
char *str_str(const char *str, const char *txt);
int			get_wprof(struct char_data *ch);
int	replace(char *str, char *search, const char *repl, int mode);
int	dice(int number, int size);
int	get_filename(char *orig_name, char *filename, int mode);
time_t	mud_time_to_secs(struct time_info_data *now);
int	get_line(FILE *fl, char *buf);
int	num_pc_in_room(struct room_data *room);
int	number(int from, int to);
int	touch(const char *path);
struct time_info_data *age(struct char_data *ch);
void	add_wprof(struct char_data *ch);
void	string_to_store(char *target, char *source);
void	basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void	core_dump_real(const char *, int);
void	log_death_trap(struct char_data *ch);
void	mudlog(const char *str, int type, bitvector_t rights, int file);
void	extended_mudlog(int type, bitvector_t logtype, int file, const char *format, ...) __attribute__ ((format (printf, 4, 5)));
size_t onebit(bitvector_t bitvector, const char *names[], char *result, size_t reslen);
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen);
size_t sprinttype(int type, const char *names[], char *result, size_t reslen);
int	check_rights(struct char_data *ch, struct char_data *vict);
int	compare_rights(struct char_data *ch, struct char_data *vict);
bitvector_t get_max_rights(struct char_data *ch);
bitvector_t wiz_bit_rights(char *arg);
char *wiz_char_rights(bitvector_t rights);
int wiz_int_rights(bitvector_t rights);
char *true_name (int id);
char *translate_code (const char *str, struct char_data *ch, struct char_data *vict, char *mybuf);
char *color_name (char color);
int	room_is_dark(room_rnum room);
int	room_always_lit(room_rnum room);
const char *readable_proficiency(int percent);
char *create_keywords(char *inbuf);
char menu_letter(int number);
int parse_menu(char arg);

#define	core_dump()		core_dump_real(__FILE__, __LINE__)

/*
 * Only provide our versions if one isn't in the C library. These macro names
 * will be defined by sysdep.h if a strcasecmp or stricmp exists.
 */
#ifndef str_cmp
int	str_cmp(const char *arg1, const char *arg2);
#endif
#ifndef strn_cmp
int	strn_cmp(const char *arg1, const char *arg2, int n);
#endif

/* random functions in random.c */
void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int	MAX(int a, int b);
int	MIN(int a, int b);
char *CAP(char *txt);
char *ALLCAP(char *txt);
char *CAPCOLOR(char *txt);
char *LOWERALL(char *txt);
char *soundex(char *str);

#define LIMIT(var, low, high)	MIN(high, MAX(var, low))

/* Followers */
int	num_followers_charmed(struct char_data *ch);
void	die_follower(struct char_data *ch);
void	add_follower(struct char_data *ch, struct char_data *leader);
void	stop_follower(struct char_data *ch);
bool	circle_follow(struct char_data *ch, struct char_data *victim);

/* in act.informative.c */
void	look_at_room(struct char_data *ch, int mode);
bool find_notify(struct char_data *ch, struct char_data *vict);

/* in act.movmement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following);

/* in limits.c */
int	hit_gain(struct char_data *ch);
int	mana_gain(struct char_data *ch);
int	move_gain(struct char_data *ch);
void	check_idling(struct char_data *ch);
void	gain_condition(struct char_data *ch, int condition, int value);
void	gain_exp(struct char_data *ch, int gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	point_update(void);
void	set_title(struct char_data *ch, char *title);
void	update_pos(struct char_data *victim);
void condition_update(void);

/* in roleplay.c */
bool find_recognized(struct char_data *ch, struct char_data *vict);


/* various constants *****************************************************/

/* defines for mudlog() */
#define	OFF	0
#define	BRF	1
#define	NRM	2
#define	CMP	3

/* get_filename() */
#define	CRASH_FILE					0
#define	ETEXT_FILE					1
#define	ALIAS_FILE					2
#define	TRAV_FILE						3
#define	SCRIPT_VARS_FILE		4

/* breadth-first searching */
#define BFS_ERROR						(-1)
#define BFS_ALREADY_THERE		(-2)
#define BFS_NO_PATH					(-3)

/*
 * XXX: These constants should be configurable. See act.informative.c
 *	and utils.c for other places to change.
 */
/* real-life time (remember Real Life?) */
#define	SECS_PER_REAL_MIN   60
#define	SECS_PER_REAL_HOUR  (60*SECS_PER_REAL_MIN)
#define	SECS_PER_REAL_DAY   (24*SECS_PER_REAL_HOUR)
#define	SECS_PER_REAL_YEAR  (365*SECS_PER_REAL_DAY)

/* mud-life time */
/* 60 secs per minute, 80 minutes per hour, 18 hours per day, 28 days a month, 12 months a year */
#define SECS_PER_MUD_MIN    (SECS_PER_REAL_MIN)
#define	SECS_PER_MUD_HOUR   (80*SECS_PER_MUD_MIN)
#define	SECS_PER_MUD_DAY    (18*SECS_PER_MUD_HOUR)
#define	SECS_PER_MUD_MONTH  (28*SECS_PER_MUD_DAY)
#define	SECS_PER_MUD_YEAR   (13*SECS_PER_MUD_MONTH)


/* string utils **********************************************************/


#define	YESNO(a) ((a) ? "YES" : "NO")
#define	ONOFF(a) ((a) ? "ON" : "OFF")
#define	ICOOC(a) ((a) ? "IC" : "OOC")

#define	LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define	UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define IS_LOWER(c) (((c)>='a') && ((c) <= 'z'))
#define IS_UPPER(c) (((c)>='A') && ((c) <= 'Z'))

#define	ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 

#define	AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")

#define	NULL_STR(length, string)   (length > 0 ? str_dup(string) : '\0')

/* memory utils **********************************************************/

#if !defined(__STRING)
#define __STRING(x)	#x
#endif

#if BUFFER_MEMORY

#define CREATE(result, type, number)  do {\
	if (!((result) = (type *) debug_calloc ((number), sizeof(type), __STRING(result), __FUNCTION__, __LINE__)))\
		{ perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
	if (!((result) = (type *) debug_realloc ((result), sizeof(type) * (number), __STRING(result), __FUNCTION__, __LINE__)))\
		{ perror("realloc failure"); abort(); } } while(0)

#define free(variable)	debug_free((variable), __FUNCTION__, __LINE__)

#define str_dup(variable)	debug_str_dup((variable), __STRING(variable), __FUNCTION__, __LINE__)

#else

#define	CREATE(result, type, number)  do {\
	if ((number) * sizeof(type) <= 0)	\
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	\
	if (!((result) = (type *) calloc ((number), sizeof(type))))	\
		{ perror("SYSERR: malloc failure"); abort(); } } while(0)

#define	RECREATE(result,type,number) do {\
	if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("SYSERR: realloc failure"); abort(); } } while(0)

#endif

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define	REMOVE_FROM_LIST(item, head, next)	\
	 if ((item) == (head))		\
			head = (item)->next;		\
	 else {				\
			temp = head;			\
			while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
			if (temp)				\
				 temp->next = (item)->next;	\
	 }					\


/* basic bitvector utils *************************************************/


#define	IS_SET(flag,bit)        ((flag) & (bit))
#define	SET_BIT(var,bit)        ((var) |= (bit))
#define	REMOVE_BIT(var,bit)     ((var) &= ~(bit))
#define	TOGGLE_BIT(var,bit)     ((var) = (var) ^ (bit))

/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if	0
/* Subtle bug in the '#var', but works well for now. */
#define	CHECK_PLAYER_SPECIAL(ch, var) \
	(*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define	CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

#define	MOB_FLAGS(ch)                 ((ch)->char_specials.saved.act)
#define	PLR_FLAGS(ch)                 ((ch)->char_specials.saved.act)
#define	PRF_FLAGS(ch)                 CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.pref))
#define	AFF_FLAGS(ch)                 ((ch)->char_specials.saved.affected_by)
#define	ROOM_FLAGS(loc)               (world[(loc)].room_flags)
#define	SPELL_ROUTINES(spl)           ((spl)->routines)
#define	ZONE_FLAGS(loc)               (zone_table[(loc)].zone_flags)

#define HAS_BODY(ch, flag)						(IS_SET((ch)->player.body_bits, (1 << flag)))

/*
 * See http://www.circlemud.org/~greerga/todo/todo.009 to eliminate MOB_ISNPC.
 * IS_MOB() acts as a VALID_MOB_RNUM()-like function.
 */
#define	IS_NPC(ch)                    (IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)	(IS_NPC(ch) && GET_MOB_RNUM(ch) <= top_of_mobt && \
				GET_MOB_RNUM(ch) != NOBODY)

#define	USER_RIGHTS(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.user_rights))
#define	EXTENDED_SYSLOG(ch)   CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.syslog))
#define	SESSION_FLAGS(ch)		  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.session_flags))

#define	MOB_FLAGGED(ch, flag)         (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define	PLR_FLAGGED(ch, flag)         (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define	AFF_FLAGGED(ch, flag)         (IS_SET(AFF_FLAGS(ch), (flag)))
#define	PRF_FLAGGED(ch, flag)         (IS_SET(PRF_FLAGS(ch), (flag)))
#define	GOT_RIGHTS(ch, flag)          (!IS_NPC(ch) && IS_SET(USER_RIGHTS(ch), (flag)))
#define	GOT_SYSLOG(ch, flag)          (!IS_NPC(ch) && IS_SET(EXTENDED_SYSLOG(ch), (flag)))
#define	SESS_FLAGGED(ch, flag)        (!IS_NPC(ch) && IS_SET(SESSION_FLAGS(ch), (flag)))
#define	ROOM_FLAGGED(loc, flag)       (IS_SET(ROOM_FLAGS(loc), (flag)))
#define	GET_MOB_VAL(mob, val)					((mob)->mob_specials.value[(val)])

#define	ZONE_FLAGGED(loc, flag)       (IS_SET(ZONE_FLAGS(loc), (flag)))

#define	EXIT_FLAGGED(exit, flag)      (IS_SET((exit)->exit_info, (flag)))
#define	OBJVAL_FLAGGED(obj, flag)     (IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define	MOBVAL_FLAGGED(mob, flag)     (IS_SET(GET_MOB_VAL((mob), 1), (flag)))
#define	OBJWEAR_FLAGGED(obj, flag)    (IS_SET(GET_OBJ_WEAR(obj), (flag)))
#define	OBJ_FLAGGED(obj, flag)        (IS_SET(GET_OBJ_EXTRA(obj), (flag)))
#define	HAS_SPELL_ROUTINE(spl, flag)  (IS_SET(SPELL_ROUTINES(spl), (flag)))

/* IS_AFFECTED for backwards compatibility */
#define	IS_AFFECTED(ch, skill)        (AFF_FLAGGED((ch), (skill)))

#define	PLR_TOG_CHK(ch,flag)          ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define	PRF_TOG_CHK(ch,flag)          ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))
#define	SYSL_TOG_CHK(ch,flag)         ((TOGGLE_BIT(EXTENDED_SYSLOG(ch), (flag))) & (flag))
#define	SESS_TOG_CHK(ch,flag)         ((TOGGLE_BIT(SESSION_FLAGS(ch), (flag))) & (flag))

/* room utils ************************************************************/


#define SECT(room)	(VALID_ROOM_RNUM(room) ? \
				world[(room)].sector_type : SECT_INSIDE)

#define IS_DARK(room)			room_is_dark((room))

#define ALWAYS_LIT(room)	room_always_lit((room))

#define	IS_REGIO(room)   (SECT(room) == SECT_FAERIE || \
													SECT(room) == SECT_DIVINE || \
													SECT(room) == SECT_INFERNAL || \
													SECT(room) == SECT_ANCIENT)

#define	IS_LIGHT(room)  (ALWAYS_LIT(room) || !IS_DARK(room))

#define VALID_ROOM_RNUM(rnum)	((rnum) != NOWHERE && (rnum) <= top_of_world)
#define	GET_ROOM_VNUM(rnum) \
	((room_vnum)(VALID_ROOM_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
#define GET_ROOM_SPEC(room) \
	(VALID_ROOM_RNUM(room) ? world[(room)].func : NULL)
#define	GET_ROOM_SPECPROC(room)	(world[real_room(room->number)].specproc)

#define GET_ROOM_RESOURCES(rnum)					(world[(rnum)].resources.resources)
#define GET_ROOM_RESOURCE(rnum, type)			(world[(rnum)].resources.current[(type)])
#define GET_ROOM_RESOURCE_MAX(rnum, type)	(world[(rnum)].resources.max[(type)])
#define	ROOM_HAS_RESOURCE(rnum, flag)			(IS_SET(world[(rnum)].resources.resources, (flag)))

/* char utils ************************************************************/

#define	IN_ROOM(ch)								((ch)->in_room)
#define	IN_ZONE(ch)								(world[(ch)->in_room].zone)
#define	GET_WAS_IN(ch)						((ch)->was_in_room)
#define	GET_AGE(ch)								(age(ch)->year)

#define	GET_PC_NAME(ch)						((ch)->player.name)
#define	GET_NAME(ch)							(IS_NPC(ch) ? (ch)->player.short_descr : GET_PC_NAME(ch))
#define	GET_TITLE(ch)							((ch)->player.title)
#define GET_DIFFICULTY(ch)				((ch)->mob_specials.difficulty)
#define	GET_PASSWD(ch)						((ch)->player.passwd)
#define	GET_PFILEPOS(ch)					((ch)->pfilepos)
#define	GET_EMAIL(ch)							((ch)->player.email)
#define	GET_HOST(ch)							((ch)->player_specials->host)

#define	GET_SDESC(ch)							((ch)->player.short_descr)
#define	GET_LDESC(ch)							((ch)->player.long_descr)
#define	GET_KEYWORDS(ch)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.keywords))

#define	GET_QUEST(ch)							((ch)->current_quest)
#define	GET_QUEST_TYPE(ch)				(aquest_table[real_quest((int)GET_QUEST((ch)))].type)
#define	GET_NUM_QUESTS(ch)				((ch)->num_completed_quests)

#define	IN_ROOM_VNUM(ch)					(GET_ROOM_VNUM(IN_ROOM(ch)))

#define	GET_CLASS(ch)							((ch)->player.chclass)
#define	GET_RACE(ch)							((ch)->player.race)
#define	GET_CULTURE(ch)						((ch)->player.culture)
#define	GET_HOME(ch)							((ch)->player.hometown)
#define	GET_HEIGHT(ch)						((ch)->player.height)
#define	GET_WEIGHT(ch)						((ch)->player.weight)
#define	GET_SEX(ch)								((ch)->player.sex)
#define GET_ACTIVE(ch)						((ch)->player.active)

#define	GET_STRENGTH(ch)					((ch)->aff_abils.strength)
#define	GET_AGILITY(ch)						((ch)->aff_abils.agility)
#define	GET_PRECISION(ch)					((ch)->aff_abils.precision)
#define	GET_PERCEPTION(ch)				((ch)->aff_abils.perception)
#define	GET_HEALTH(ch)						((ch)->aff_abils.health)
#define	GET_WILLPOWER(ch)					((ch)->aff_abils.willpower)
#define	GET_INTELLIGENCE(ch)			((ch)->aff_abils.intelligence)
#define	GET_CHARISMA(ch)					((ch)->aff_abils.charisma)
#define	GET_LUCK(ch)							((ch)->aff_abils.luck)
#define	GET_ESSENCE(ch)						((ch)->aff_abils.essence)

#define	GET_BODY(ch)							((ch)->player.body_bits)
#define	GET_SIZE(ch)							((ch)->player.size)

#define	GET_PIETY(ch)							((ch)->points.piety)
#define	GET_REPUTATION(ch)				((ch)->points.reputation)
#define	GET_SOCIAL_RANK(ch)				((ch)->points.social_rank)
#define	GET_MILITARY_RANK(ch)			((ch)->points.military_rank)
#define	GET_SANITY(ch)						((ch)->points.sanity)

#define	GET_FLUX(ch)							((ch)->points.flux)
#define	GET_MAX_FLUX(ch)					((ch)->points.max_flux)

#define	GET_FATIGUE(ch)						((ch)->points.fatigue)

#define	GET_EXP(ch)								((ch)->points.exp)
#define	GET_PD(ch)								((ch)->points.armor)
#define	GET_REDUCTION(ch)					((ch)->points.damage_reduction)
#define	GET_HIT(ch)								((ch)->points.hit)
#define	GET_MAX_HIT(ch)						((ch)->points.max_hit)
#define	GET_MOVE(ch)							((ch)->points.move)
#define	GET_MAX_MOVE(ch)					((ch)->points.max_move)
#define	GET_MANA(ch)							((ch)->points.mana)
#define	GET_MAX_MANA(ch)					((ch)->points.max_mana)
#define	GET_GOLD(ch)							((ch)->points.gold)
#define	GET_BANK_GOLD(ch)					((ch)->points.bank_gold)
#define	GET_HITROLL(ch)						((ch)->points.hitroll)
#define	GET_DAMROLL(ch)						((ch)->points.damroll)
#define GET_SKILLCAP(ch)					((ch)->points.skillcap)

#define	GET_CREATION_POINTS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.creation_points))

#define	GET_TRAVELS(ch)						CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.travels))

#define	IS_VERIFIED(ch)						CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.verified))

#define	GET_POS(ch)								((ch)->char_specials.position)
#define	GET_IDNUM(ch)							((ch)->char_specials.saved.idnum)
#define	GET_ID(x)									((x)->id)
#define	IS_CARRYING_W(ch)					((ch)->char_specials.carry_weight)
#define	IS_CARRYING_N(ch)					((ch)->char_specials.carry_items)
#define	FIGHTING(ch)							((ch)->char_specials.fighting)
#define	HUNTING(ch)								((ch)->char_specials.hunting)
#define	GET_SAVE(ch, i)						((ch)->char_specials.saved.apply_saving_throw[i])
#define	GET_ALIGNMENT(ch)					((ch)->char_specials.saved.alignment)

#define	GET_COND(ch, i)						CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.conditions[(i)]))
#define	GET_LOADROOM(ch)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.load_room))
#define	GET_PRACTICES(ch)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.spells_to_learn))
#define	GET_INVIS_LEV(ch)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.invis_level))
#define	GET_WIMP_LEV(ch)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.wimp_level))
#define	GET_BAD_PWS(ch)						CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.bad_pws))
#define GET_CONTACT(ch)						CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.contact))
#define	GET_TALK(ch, i)						CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.talks[i]))
#define	GET_LAST_OLC_TARG(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define	GET_LAST_OLC_MODE(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define	GET_ALIASES(ch)						CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define	GET_LAST_TELL(ch)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
#define	GET_MAILINGLIST(ch)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.mailinglist))
#define	GET_TRUENAME(ch)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.true_name))
#define	GET_RPXP(ch)							CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.rpxp))

#define GET_APPROVEDBY(ch)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.approved_by))

#define GET_EYECOLOR(ch)					((ch)->player.eyecolor)
#define GET_HAIRCOLOR(ch)					((ch)->player.haircolor)
#define GET_HAIRSTYLE(ch)					((ch)->player.hairstyle)
#define GET_SKINTONE(ch)					((ch)->player.skintone)

#define GET_COLORPREF_ECHO(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.colorpref_echo))
#define GET_COLORPREF_EMOTE(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.colorpref_emote))
#define GET_COLORPREF_OSAY(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.colorpref_osay))
#define GET_COLORPREF_SAY(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.colorpref_say))
#define GET_COLORPREF_POSE(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.colorpref_pose))

#define GET_RPLOG(ch, i)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.rplog[(i)]))
#define GET_TELLS(ch, i)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.tells[(i)]))

#define GET_RECOGNIZED(ch, i)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.recognized[(i)]))
#define GET_NOTIFY(ch, i)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.notifylist[(i)]))

#define GET_RPDESCRIPTION(ch, i)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.rpdescription[(i)]))
#define GET_ACTIVEDESC(ch)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.activedesc))
#define GET_DOING(ch)							CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.doing))
#define GET_PAGE_LENGTH(ch)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.page_length))
#define GET_CONTACTINFO(ch)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.contactinfo))

#define GET_QP(ch)								CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.questpoints))

#define GET_GUILD(ch)							CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.guild))

#define IS_OOC(ch)								(!IS_NPC((ch)) && !SESS_FLAGGED((ch), SESS_IC))
#define IS_IC(ch)									(IS_NPC((ch)) || SESS_FLAGGED((ch), SESS_IC))

#define GET_REPLYTO(ch)						CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.replyto))

#define	GET_LASTCRAFTING(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.last_crafting))

/* Rights macros */
#define IS_MEMBER(ch)							(GOT_RIGHTS((ch), RIGHTS_MEMBER))
#define IS_PLAYER(ch)							(!IS_NPC((ch)) && IS_MEMBER((ch)))

/* Simple rights */
#define IS_MORTAL(ch)							(IS_PLAYER((ch)) && !GOT_RIGHTS((ch), RIGHTS_IMMORTAL))
#define IS_QUESTING(ch)						(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_QUESTS))
#define IS_GUILDIE(ch)						(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_GUILDS))
#define IS_NEWBIEGUIDE(ch)				(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_NEWBIES))
#define IS_IMMORTAL(ch)						(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_IMMORTAL))
#define IS_QUESTOR(ch)						(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_QUESTOR))
#define IS_RPSTAFF(ch)						(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_PLAYERS))
#define IS_BUILDER(ch)						(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_BUILDING))
#define IS_ADMIN(ch)							(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_ADMIN))
#define IS_DEVELOPER(ch)					(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_DEVELOPER))
#define IS_HEADBUILDER(ch)				(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_HEADBUILDER))
#define IS_IMPLEMENTOR(ch)				(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_IMPLEMENTOR))
#define IS_OWNER(ch)							(IS_PLAYER((ch)) && GOT_RIGHTS((ch), RIGHTS_OWNER))

/* Complex rights */
#define IS_GOD(ch)								(IS_IMMORTAL((ch)) && (IS_BUILDER((ch)) || IS_QUESTOR((ch)) || IS_RPSTAFF((ch))))
#define IS_GRGOD(ch)							(IS_GOD((ch)) && (IS_HEADBUILDER((ch)) || IS_DEVELOPER((ch))))
#define IS_IMPL(ch)								(IS_GRGOD((ch)) && IS_IMPLEMENTOR((ch)))

/* Building rights */
#define CAN_EDIT_ZONE(ch, zone)		((IS_BUILDER((ch)) && is_name(GET_NAME((ch)), zone_table[(zone)].builders)) || IS_HEADBUILDER((ch)) || IS_IMPL((ch)))
#define CAN_EDIT_ACTIONS(ch)			(IS_PLAYER((ch)) && (GOT_RIGHTS((ch), RIGHTS_ACTIONS) || IS_HEADBUILDER((ch)) || IS_IMPL((ch))))
#define CAN_EDIT_HELP(ch)					(IS_PLAYER((ch)) && (GOT_RIGHTS((ch), RIGHTS_HELPFILES) || IS_HEADBUILDER((ch)) || IS_IMPL((ch))))
#define CAN_EDIT_QUESTS(ch)				(IS_RPSTAFF((ch)) || IS_QUESTOR((ch)) || IS_HEADBUILDER((ch)) || IS_IMPL((ch)))
#define CAN_EDIT_ASSEMBLIES(ch)		(IS_PLAYER((ch)) && (IS_HEADBUILDER((ch)) || IS_IMPL((ch))))
#define CAN_EDIT_TRIGGERS(ch)			(IS_PLAYER((ch)) && (GOT_RIGHTS((ch), RIGHTS_TRIGGERS) || IS_HEADBUILDER((ch)) || IS_IMPL((ch))))

#define IS_ACTIVE(ch)							(GET_ACTIVE((ch)))

#define	GET_SKILL(ch, i)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skills[i]))
#define	SET_SKILL(ch, i, pct)			do { CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.skills[i]) = pct; } while(0)

#define	GET_SKILL_STATE(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skill_states[i]))
#define	SET_SKILL_STATE(ch, i, pct)	do { CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.skill_states[i]) = pct; } while(0)

#define	GET_PLAYER_EVENT(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.player_events[i]))

#define	SPEAKING(ch)							((ch)->player_specials->saved.speaking)

#define	GET_EQ(ch, i)							((ch)->equipment[i])

#define	GET_MOB_SPEC(ch)					(IS_MOB(ch) ? mob_index[(ch)->nr].func : NULL)
#define	GET_MOB_SPECPROC(ch)			(mob_index[(ch)->nr].specproc)
#define	GET_MOB_RNUM(mob)					((mob)->nr)
#define	GET_MOB_VNUM(mob)					(IS_MOB(mob) ? mob_index[GET_MOB_RNUM(mob)].vnum : NOBODY)

#define	GET_DEFAULT_POS(ch)				((ch)->mob_specials.default_pos)
#define	MEMORY(ch)								((ch)->mob_specials.memory)

#define	STRENGTH_APPLY_INDEX(ch)	(GET_STRENGTH(ch) / 100)

#define	CAN_CARRY_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w)
#define	CAN_CARRY_N(ch) (5 + ((GET_AGILITY(ch) / 100) >> 1) + ((GET_STRENGTH(ch) / 100) >> 1))
#define	AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define	CAN_SEE_IN_DARK(ch) \
	 (AFF_FLAGGED(ch, AFF_INFRAVISION) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))

/* REGIO ADD-ON */
#define	CAN_SEE_FAERIE(ch) \
	 (AFF_FLAGGED(ch, AFF_SEE_FAERIE) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
#define	CAN_SEE_INFERNAL(ch) \
	 (AFF_FLAGGED(ch, AFF_SEE_INFERNAL) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
#define	CAN_SEE_DIVINE(ch) \
	 (AFF_FLAGGED(ch, AFF_SEE_DIVINE) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
#define	CAN_SEE_ANCIENT(ch) \
	 (AFF_FLAGGED(ch, AFF_SEE_ANCIENT) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))

/* #define REGIO_EXIT(dir)  (EXIT_FLAGGED(EXIT(ch, dir), EX_FAERIE || EX_INFERNAL || EX_DIVINE || EX_ANCIENT)) */

#define	IS_GOOD(ch)								(GET_ALIGNMENT(ch) >= 350)
#define	IS_EVIL(ch)								(GET_ALIGNMENT(ch) <= -350)
#define	IS_NEUTRAL(ch)						(!IS_GOOD(ch) && !IS_EVIL(ch))

/* These three deprecated. */
#define	WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (cycle); } while(0)
#define	CHECK_WAIT(ch)                ((ch)->wait > 0)
#define	GET_MOB_WAIT(ch)      GET_WAIT_STATE(ch)
/* New, preferred macro. */
#define	GET_WAIT_STATE(ch)    ((ch)->wait)


/* descriptor-based utils ************************************************/

/* Hrm, not many.  We should make more. -gg 3/4/99 */
#define	STATE(d)		((d)->connected)
#define EDITING(d)	((d)->textedit)

#define	IS_PLAYING(d)  (STATE(d) == CON_REDIT || STATE(d) == CON_MEDIT || \
												STATE(d) == CON_OEDIT || STATE(d) == CON_ZEDIT || \
												STATE(d) == CON_SEDIT || STATE(d) == CON_AEDIT || \
												STATE(d) == CON_EMAIL || STATE(d) == CON_TRIGEDIT || \
												STATE(d) == CON_QEDIT || STATE(d) == CON_PLAYING || \
												STATE(d) == CON_ASSEDIT || STATE(d) == CON_DESCEDIT || \
												STATE(d) == CON_COPYOVER || STATE(d) == CON_CINFOEDIT || \
												STATE(d) == CON_GEDIT || STATE(d) == CON_COMEDIT || \
												STATE(d) == CON_TUTOREDIT)

#define	IN_OLC(d)      (STATE(d) == CON_REDIT || STATE(d) == CON_MEDIT || \
												STATE(d) == CON_OEDIT || STATE(d) == CON_ZEDIT || \
												STATE(d) == CON_SEDIT || STATE(d) == CON_AEDIT || \
												STATE(d) == CON_EMAIL || STATE(d) == CON_TRIGEDIT || \
												STATE(d) == CON_QEDIT || STATE(d) == CON_ASSEDIT || \
												STATE(d) == CON_DESCEDIT || STATE(d) == CON_CINFOEDIT || \
												STATE(d) == CON_GEDIT || STATE(d) == CON_COMEDIT || \
												STATE(d) == CON_TUTOREDIT)

/* object utils **********************************************************/

/*
 * Check for NOWHERE or the top array index?
 * If using unsigned types, the top array index will catch everything.
 * If using signed types, NOTHING will catch the majority of bad accesses.
 */
#define VALID_OBJ_RNUM(obj)		(GET_OBJ_RNUM(obj) <= top_of_objt && \
																GET_OBJ_RNUM(obj) != NOTHING)

#define	GET_OBJ_NAME(obj)				((obj)->name)
#define	GET_OBJ_SPECPROC(obj)		(obj_index[GET_OBJ_RNUM(obj)].specproc)
#define	GET_OBJ_PERM(obj)				((obj)->obj_flags.bitvector)
#define	GET_OBJ_TYPE(obj)				((obj)->obj_flags.type_flag)
#define	GET_OBJ_COST(obj)				((obj)->obj_flags.cost)
#define	GET_OBJ_RENT(obj)				((obj)->obj_flags.cost_per_day)
#define GET_OBJ_AFFECT(obj)			((obj)->obj_flags.bitvector)
#define	GET_OBJ_EXTRA(obj)			((obj)->obj_flags.extra_flags)
#define	GET_OBJ_WEAR(obj)				((obj)->obj_flags.wear_flags)
#define	GET_OBJ_VAL(obj, val)		((obj)->obj_flags.value[(val)])
#define	GET_OBJ_WEIGHT(obj)			((obj)->obj_flags.weight)
#define	GET_OBJ_TIMER(obj)			((obj)->obj_flags.timer)
#define GET_OBJ_SIZE(obj)				((obj)->obj_flags.size)
#define	GET_OBJ_COLOR(obj)			((obj)->obj_flags.color)
#define	GET_OBJ_RESOURCE(obj)		((obj)->obj_flags.resource)
#define	GET_OBJ_RNUM(obj)				((obj)->item_number)
#define GET_OBJ_VNUM(obj)				(VALID_OBJ_RNUM(obj) ? \
																	obj_index[GET_OBJ_RNUM(obj)].vnum : NOTHING)
#define GET_OBJ_PROTOVNUM(obj)	(VALID_OBJ_RNUM(obj) ? \
																	obj_index[GET_OBJ_RNUM(obj)].vnum : (obj)->proto_number)
#define GET_OBJ_SPEC(obj)				(VALID_OBJ_RNUM(obj) ? \
																	obj_index[GET_OBJ_RNUM(obj)].func : NULL)

#define	IS_CORPSE(obj)					(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
																	GET_OBJ_VAL((obj), 3) == 1)

#define CAN_WEAR(obj, part)			OBJWEAR_FLAGGED((obj), (part))

#define	IS_OBJ_PORTAL(obj)			(GET_OBJ_TYPE(obj) == ITEM_PORTAL)


/* compound utilities and other macros **********************************/

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define	CIRCLEMUD_VERSION(major, minor, patchlevel) \
	(((major) << 16) + ((minor) << 8) + (patchlevel))

#define	HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define	HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define	HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define	ANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "An" : "A")
#define	SANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "an" : "a")

#define	STRANA(str) (strchr("aeiouAEIOU", *str) ? "An" : "A")
#define	STRSANA(str) (strchr("aeiouAEIOU", *str) ? "an" : "a")

/* Various macros building up to CAN_SEE */

#define	LIGHT_OK(sub)	(!AFF_FLAGGED(sub, AFF_BLIND) && \
	 (IS_LIGHT(IN_ROOM(sub)) || AFF_FLAGGED((sub), AFF_INFRAVISION)))

#define	INVIS_OK(sub, obj) \
 (!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED(sub,AFF_DETECT_INVIS))

#define HIDE_OK(sub, obj) \
 (!AFF_FLAGGED((obj), AFF_HIDE) || AFF_FLAGGED(sub, AFF_SENSE_LIFE))

#define	MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj) && HIDE_OK(sub, obj))

#define	IMM_CAN_SEE(sub, obj) \
	 (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define	SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define	CAN_SEE(sub, obj) (SELF(sub, obj) || ((IS_NPC(obj) || (GOT_RIGHTS(sub, GET_INVIS_LEV(obj)) || \
		(GET_INVIS_LEV(obj) == RIGHTS_NONE)) || IS_IMPLEMENTOR(sub)) && IMM_CAN_SEE(sub, obj)) )

#define CAN_SEE_WHO(sub, obj) (SELF(sub, obj) || ((IS_NPC(obj) || (GOT_RIGHTS(sub, GET_INVIS_LEV(obj)) || \
		(GET_INVIS_LEV(obj) == RIGHTS_NONE)) || IS_IMPLEMENTOR(sub)) && \
		(INVIS_OK(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))) )

/* End of CAN_SEE */


#define	INVIS_OK_OBJ(sub, obj) \
	(!OBJ_FLAGGED((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */
#define	CAN_SEE_OBJ_CARRIER(sub, obj) \
	((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) &&	\
	 (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))

#define	MORT_CAN_SEE_OBJ(sub, obj) \
	(LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj))

#define	CAN_SEE_OBJ(sub, obj) \
	 (MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define	CAN_CARRY_OBJ(ch,obj)  \
	 (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
		((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define	CAN_GET_OBJ(ch, obj)   \
	 (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
		CAN_SEE_OBJ((ch),(obj)))

#define	PERS(ch, vict, use_name)   (CAN_SEE(vict, ch) ? (use_name || IS_OOC(ch) ? GET_NAME(ch) : (GET_SDESC(ch) ? GET_SDESC(ch) : "somebody")) : "someone")

#define	OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "something")

#define	OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "something")


#define EXIT(ch, door)  (world[IN_ROOM(ch)].dir_option[door])
#define	W_EXIT(room, num)	(world[(room)].dir_option[(num)])
#define	R_EXIT(room, num)	((room)->dir_option[(num)])

#define	CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))

#define OUTSIDE(ch) (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))

#define	IS_REGIO_EXIT(ch, door) ((EXIT_FLAGGED(EXIT(ch, door), EX_FAERIE) || \
																 (EXIT_FLAGGED(EXIT(ch, door), EX_DIVINE) || \
																 (EXIT_FLAGGED(EXIT(ch, door), EX_INFERNAL) || \
																 (EXIT_FLAGGED(EXIT(ch, door), EX_ANCIENT))


#define	CLASS_ABBR(ch) (IS_NPC(ch) ? "--" : classes[(int)GET_CLASS(ch)].abbrev)
#define	RACE_ABBR(ch) (IS_NPC(ch) ? "---" : races[(int)GET_RACE(ch)].abbrev)

#define	IS_MAGUS_ANIMAL(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGUS_ANIMAL))
#define	IS_MAGUS_AQUAM(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGUS_AQUAM))
#define	IS_MAGUS_AURAM(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGUS_AURAM))
#define	IS_MAGUS_CORPOREM(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGUS_CORPOREM))
#define	IS_MAGUS_HERBAM(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGUS_HERBAM))
#define	IS_MAGUS_IGNEM(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGUS_IGNEM))
#define	IS_MAGUS_IMAGONEM(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGUS_IMAGONEM))
#define	IS_MAGUS_MENTEM(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGUS_MENTEM))
#define	IS_MAGUS_TERRAM(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGUS_TERRAM))
#define	IS_MAGUS_VIM(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGUS_VIM))
#define	IS_MONK(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MONK))
#define	IS_BARD(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_BARD))
#define	IS_THIEF(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_THIEF))
#define	IS_ASSASSIN(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_ASSASSIN))
#define	IS_HUNTER(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_HUNTER))
#define	IS_WARRIOR(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_WARRIOR))
#define	IS_GLADIATOR(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_GLADIATOR))
#define	IS_PALADIN(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_PALADIN))

#define	IS_HUMAN(ch)					(GET_RACE(ch) == RACE_HUMAN)
#define	IS_GRAYELF(ch)				(GET_RACE(ch) == RACE_GRAYELF)
#define	IS_HIGHELF(ch)				(GET_RACE(ch) == RACE_HIGHELF)
#define	IS_SATYR(ch)					(GET_RACE(ch) == RACE_SATYR)
#define	IS_PIXIE(ch)					(GET_RACE(ch) == RACE_PIXIE)
#define	IS_SPRITE(ch)					(GET_RACE(ch) == RACE_SPRITE)
#define	IS_NYMPH(ch)					(GET_RACE(ch) == RACE_NYMPH)
#define	IS_GNOME(ch)					(GET_RACE(ch) == RACE_GNOME)
#define	IS_AASIMARI(ch)				(GET_RACE(ch) == RACE_AASIMARI)
#define	IS_GIANT(ch)					(GET_RACE(ch) == RACE_GIANT)
#define	IS_VAMPIRE(ch)				(GET_RACE(ch) == RACE_VAMPIRE)
#define	IS_TIEFLING(ch)				(GET_RACE(ch) == RACE_TIEFLING)
#define	IS_NIXIE(ch)					(GET_RACE(ch) == RACE_NIXIE)
#define	IS_LEPRECHAUN(ch)			(GET_RACE(ch) == RACE_LEPRECHAUN)
#define	IS_TROLL(ch)					(GET_RACE(ch) == RACE_TROLL)
#define	IS_GOBLIN(ch)					(GET_RACE(ch) == RACE_GOBLIN)
#define	IS_BROWNIE(ch)				(GET_RACE(ch) == RACE_BROWNIE)
#define	IS_DRAGON(ch)					(GET_RACE(ch) == RACE_DRAGON)
#define	IS_UNDEAD(ch)					(GET_RACE(ch) == RACE_UNDEAD)
#define	IS_ETHEREAL(ch)				(GET_RACE(ch) == RACE_ETHEREAL)
#define	IS_ANIMAL(ch)					(GET_RACE(ch) == RACE_ANIMAL)
#define	IS_LEGENDARYRACE(ch)	(GET_RACE(ch) == RACE_LEGENDARY)
#define	IS_ELEMENTALRACE(ch)	(GET_RACE(ch) == RACE_ELEMENTAL)
#define	IS_MAGICALRACE(ch)		(GET_RACE(ch) == RACE_MAGICAL)
#define	IS_SERAPHI(ch)				(GET_RACE(ch) == RACE_SERAPHI)
#define	IS_ADEPHI(ch)					(GET_RACE(ch) == RACE_ADEPHI)
#define	IS_DEMON(ch)					(GET_RACE(ch) == RACE_DEMON)
#define	IS_DEITY(ch)					(GET_RACE(ch) == RACE_DEITY)
#define	IS_ENDLESS(ch)				(GET_RACE(ch) == RACE_ENDLESS)

#define IS_FAE(ch)						( (GET_RACE(ch) == RACE_GRAYELF)		|| \
																(GET_RACE(ch) == RACE_HIGHELF)		|| \
																(GET_RACE(ch) == RACE_SATYR)			|| \
																(GET_RACE(ch) == RACE_PIXIE)			|| \
																(GET_RACE(ch) == RACE_SPRITE)			|| \
																(GET_RACE(ch) == RACE_NYMPH)			|| \
																(GET_RACE(ch) == RACE_NIXIE)			|| \
																(GET_RACE(ch) == RACE_LEPRECHAUN)	|| \
																(GET_RACE(ch) == RACE_TROLL)			|| \
																(GET_RACE(ch) == RACE_GOBLIN)			|| \
																(GET_RACE(ch) == RACE_BROWNIE)		|| \
																(GET_RACE(ch) == RACE_GNOME))

#define	IS_SEELIE(ch)					( (GET_RACE(ch) == RACE_GRAYELF)		|| \
																(GET_RACE(ch) == RACE_HIGHELF)		|| \
																(GET_RACE(ch) == RACE_SATYR)			|| \
																(GET_RACE(ch) == RACE_PIXIE)			|| \
																(GET_RACE(ch) == RACE_SPRITE)			|| \
																(GET_RACE(ch) == RACE_NYMPH))

#define	IS_UNSEELIE(ch)				( (GET_RACE(ch) == RACE_NIXIE)			|| \
																(GET_RACE(ch) == RACE_LEPRECHAUN)	|| \
																(GET_RACE(ch) == RACE_TROLL)			|| \
																(GET_RACE(ch) == RACE_GOBLIN)			|| \
																(GET_RACE(ch) == RACE_BROWNIE)		|| \
																(GET_RACE(ch) == RACE_GNOME))

#define	IS_LITTLE_FAE(ch)			( (GET_RACE(ch) == RACE_PIXIE)			|| \
																(GET_RACE(ch) == RACE_SPRITE)			|| \
																(GET_RACE(ch) == RACE_NYMPH)			|| \
																(GET_RACE(ch) == RACE_LEPRECHAUN)	|| \
																(GET_RACE(ch) == RACE_NIXIE)			|| \
																(GET_RACE(ch) == RACE_GOBLIN)			|| \
																(GET_RACE(ch) == RACE_BROWNIE))

#define	IS_DIVINE(ch)					( (GET_RACE(ch) == RACE_SERAPHI)		|| \
																(GET_RACE(ch) == RACE_ADEPHI)			|| \
																(GET_RACE(ch) == RACE_AASIMARI))

#define	IS_DEMONIC(ch)				( (GET_RACE(ch) == RACE_DEMON)			|| \
																(GET_RACE(ch) == RACE_TIEFLING)		|| \
																(GET_RACE(ch) == RACE_VAMPIRE))

#define	IS_BASE_ANIMAL(ch)		( (GET_RACE(ch) == RACE_ANIMAL)			|| \
																(GET_RACE(ch) == RACE_LEGENDARY))

#define	IS_MUNDANE(ch)				( (IS_HUMAN(ch))			|| \
																(GET_RACE(ch) == RACE_AASIMARI)		|| \
																(GET_RACE(ch) == RACE_TIEFLING))

#define	IS_MAGI_TYPE(ch)    (IS_MAGUS_ANIMAL(ch)   || IS_MAGUS_AQUAM(ch)    || \
														 IS_MAGUS_AURAM(ch)    || IS_MAGUS_CORPOREM(ch) || \
														 IS_MAGUS_HERBAM(ch)   || IS_MAGUS_IGNEM(ch)    || \
														 IS_MAGUS_IMAGONEM(ch) || IS_MAGUS_MENTEM(ch)   || \
														 IS_MAGUS_TERRAM(ch)   || IS_MAGUS_VIM(ch))

#define	IS_CLERIC_TYPE(ch)  (IS_BARD(ch) || IS_MONK(ch) || IS_PALADIN(ch))
				 
#define	IS_WARRIOR_TYPE(ch) (IS_WARRIOR(ch) || IS_GLADIATOR(ch))

#define	IS_THIEF_TYPE(ch)   (IS_THIEF(ch) || IS_HUNTER(ch) || \
														 IS_ASSASSIN(ch))

#define	CAN_CAST_SPELLS(ch)  (IS_MAGUS_ANIMAL(ch)   || IS_MAGUS_AQUAM(ch)    || \
														 IS_MAGUS_AURAM(ch)     || IS_MAGUS_CORPOREM(ch) || \
														 IS_MAGUS_HERBAM(ch)    || IS_MAGUS_IGNEM(ch)    || \
														 IS_MAGUS_IMAGONEM(ch)  || IS_MAGUS_MENTEM(ch)   || \
														 IS_MAGUS_TERRAM(ch)    || IS_MAGUS_VIM(ch)      || \
														 IS_MONK(ch)            || IS_BARD(ch)           || \
														 IS_PALADIN(ch)         || IS_ASSASSIN(ch))

#define AWAY_DEFAULT		"I am currently away from my computer."
#define GET_AWAY(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.away_message))

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef	NULL
#define	NULL (void *)0
#endif

#ifndef	FALSE
#define	FALSE 0
#endif

#ifndef	TRUE
#define	TRUE  (!FALSE)
#endif

#ifndef	NO
#define	NO FALSE
#endif

#ifndef	YES
#define	YES  TRUE
#endif

/* defines for fseek */
#ifndef	SEEK_SET
#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#endif

/*
 * NOCRYPT can be defined by an implementor manually in sysdep.h.
 * CIRCLE_CRYPT is a variable that the 'configure' script
 * automatically sets when it determines whether or not the system is
 * capable of encrypting.
 */
#if	defined(NOCRYPT) || !defined(CIRCLE_CRYPT)
#define	CRYPT(a,b) (a)
#else
#define	CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

/**	Defines for 'replace' function in utils.h ******************************/
#define	REPL_NORMAL           0
#define	REPL_FIRST_ONLY       (1 << 0)
#define	REPL_MATCH_CASE       (1 << 1)
#define	REPL_RECURSIVE        (1 << 2)

#define SOUNDEX_MATCH(search, target) (!strncmp(soundex((search)), soundex((target)), strlen((search))))

/** Defines for 'replace' function in utils.h ******************************/
#define REPL_NORMAL						0
#define REPL_FIRST_ONLY				(1 << 0)
#define REPL_MATCH_CASE				(1 << 1)
#define REPL_RECURSIVE				(1 << 2)
