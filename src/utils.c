/* ************************************************************************
*		File: utils.c                                       Part of CircleMUD *
*	 Usage: various internal functions of a utility nature                  *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: utils.c,v 1.58 2004/03/19 21:58:09 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"
#include "roleplay.h"
#include "color.h"

extern struct descriptor_data *descriptor_list;
extern struct time_info_data time_info;
extern struct room_data *world;
extern struct obj_data *obj_proto;
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct index_data *obj_index;
extern struct class_data classes[NUM_CLASSES];
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;

/* local functions */
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void die_follower(struct char_data *ch);
void add_follower(struct char_data *ch, struct char_data *leader);
void prune_crlf(char *txt);
void user_cntr(struct descriptor_data *d);
time_t mud_time_to_secs(struct time_info_data *now);
int	get_weapon_dam (struct obj_data *o);
int	get_armor_ac (struct obj_data *o);
obj_vnum object_name_to_num (char *namestr);
mob_vnum mobile_name_to_num (char *namestr);
void make_who2html(void);
bool valid_email(const char *em);
char *LOWERALL(char *txt);
EVENTFUNC(sysintensive_delay);
int room_is_dark(room_rnum room);
int room_always_lit(room_rnum room);
int num_followers_charmed(struct char_data *ch);


/* creates a random number in interval [from;to] */
int	number(int from, int to)
{
	/* error checking in case people call number() incorrectly */
	if (from > to) {
		int tmp = from;
		from = to;
		to = tmp;
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "number() should be called with lowest, then highest. number(%d, %d), not number(%d, %d).", from, to, to, from);
	}

	/*
	 * This should always be of the form:
	 *
	 *	((float)(from - to + 1) * rand() / (float)(RAND_MAX + from) + from);
	 *
	 * if you are using rand() due to historical non-randomness of the
	 * lower bits in older implementations.  We always use circle_random()
	 * though, which shouldn't have that problem. Mean and standard
	 * deviation of both are identical (within the realm of statistical
	 * identity) if the rand() implementation is non-broken.
	 */
	return ((circle_random() % (to - from + 1)) + from);
}


/* simulates dice roll */
int	dice(int num, int size)
{
	int sum = 0;

	if (size <= 0 || num <= 0)
		return (0);

	while (num-- > 0)
		sum += number(1, size);

	return (sum);
}


int	MIN(int a, int b)
{
	return (a < b ? a : b);
}


int	MAX(int a, int b)
{
	return (a > b ? a : b);
}


char *CAP(char *txt)
{
	*txt = UPPER(*txt);
	return (txt);
}


char *ALLCAP(char *txt)
{
	char *ptr = NULL;

	for (ptr = txt; *ptr; ptr++)
		*ptr = UPPER(*ptr);

	return (txt);
}


char *LOWERALL(char *txt)
{
	char *ptr = NULL;

	for (ptr = txt; *ptr; ptr++)
		*ptr = LOWER(*ptr);

	return (txt);
}


char *CAPCOLOR(char *txt)
{
	char *ptr = NULL;
	int i = 0;

	while (ptr[i]) {
		if (ptr[i] == '&') {
			i+= 2;
			continue;
		} else if ((UPPER(ptr[i]) >= 'A') && (UPPER(ptr[i]) <= 'Z')) {
			ptr[i] = UPPER(ptr[i]);
			break;
		}
		i++;
	}

	return (txt);
}

#if !defined(HAVE_STRLCPY)
/*
 * A 'strlcpy' function in the same fashion as 'strdup' below.
 *
 * This copies up to totalsize - 1 bytes from the source string, placing
 * them and a trailing NUL into the destination string.
 *
 * Returns the total length of the string it tried to copy, not including
 * the trailing NUL.  So a '>= totalsize' test says it was truncated.
 * (Note that you may have _expected_ truncation because you only wanted
 * a few characters from the source string.)
 */
size_t strlcpy(char *dest, const char *source, size_t totalsize)
{
	strncpy(dest, source, totalsize - 1);	/* strncpy: OK (we must assume 'totalsize' is correct) */
	dest[totalsize - 1] = '\0';
	return strlen(source);
}
#endif


#if !defined(HAVE_STRDUP)
/* Create a duplicate of a string */
char *strdup(const char *source)
{
	char *new_z;

	CREATE(new_z, char, strlen(source) + 1);
	return (strcpy(new_z, source)); /* strcpy: OK */
}
#endif

/*
 * Strips \r\n from end of string.
 */
void prune_crlf(char *txt)
{
	int i = strlen(txt) - 1;

	while (txt[i] == '\n' || txt[i] == '\r')
		txt[i--] = '\0';
}


#ifndef str_cmp
/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */
int	str_cmp(const char *arg1, const char *arg2)
{
	int chk, i;

	if (arg1 == NULL || arg2 == NULL) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
		return (0);
	}

	for (i = 0; arg1[i] || arg2[i]; i++)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);        /* not equal */

	return (0);
}
#endif


#ifndef strn_cmp
/*
 * strn_cmp: a case-insensitive version of strncmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different, the end of both, or n is reached.
 */
int	strn_cmp(const char *arg1, const char *arg2, int n)
{
	int chk, i;

	if (arg1 == NULL || arg2 == NULL) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
		return (0);
	}

	for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);        /* not equal */

	return (0);
}
#endif


/* log a death trap hit */
void log_death_trap(struct char_data *ch)
{
	extended_mudlog(NRM, SYSL_DEATHS, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch),
					GET_ROOM_VNUM(IN_ROOM(ch)), world[IN_ROOM(ch)].name);
}


/*
 * New variable argument mlog() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void vbasic_mud_log(bitvector_t logtype, const char *format, va_list args)
{
	static char txt[MAX_STRING_LENGTH];
	time_t ct = time(0);
	char *time_s = asctime(localtime(&ct));

	if (logfile == NULL) {
		puts("SYSERR: Using mlog() before stream was initialized!");
		return;
	}

	if (format == NULL)
		format = "SYSERR: mlog() received a NULL format.";

	time_s[strlen(time_s) - 1] = '\0';

	onebit(logtype, extended_syslogs, buf2, sizeof(buf2));
	sprintf(buf1, "%s: ", buf2);

	fprintf(logfile, "%-15.15s :: %s", time_s + 4, (logtype > 0) ? buf1 : "");
	/* Strip all color codes from the text sent to the logfile. */
	vsnprintf(txt, sizeof(txt), format, args);
	proc_color(txt, 0, TRUE, 0, 0);
	fprintf(logfile, "%s", txt);
	/* Terminate the string with \n. */
	fputc('\n', logfile);
	fflush(logfile);
}


/* Wrapper so mudlog() can use vbasic_mud_log(). */
void basic_mud_log(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vbasic_mud_log(0, format, args);
	va_end(args);
}


/* the "touch" command, essentially. */
int	touch(const char *path)
{
	FILE *fl;

	if (!(fl = fopen(path, "a"))) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "%s: %s", path, strerror(errno));
		return (-1);
	} else {
		fclose(fl);
		return (0);
	}
}


/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(const char *str, int type, bitvector_t rights, int file)
{
	char buf[MAX_STRING_LENGTH], tp;
	struct descriptor_data *i;

	if (str == NULL)
		return;
	if (file)
		mlog("%s", str);
	if (rights < 0)
		return;

	sprintf(buf, "[ %s ]\r\n", str);

	for (i = descriptor_list; i; i = i->next) {
		if (!(STATE(i) == CON_PLAYING || STATE(i) == CON_COPYOVER) || IS_NPC(i->character)) /* switch */
			continue;
		if (!GOT_RIGHTS(i->character, rights))
			continue;
		if (EDITING(i) || PLR_FLAGGED(i->character, PLR_OLC))
			continue;
		tp = ((PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) +
				(PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0));
		if (tp < type)
			continue;

		send_to_char(CCGRN(i->character, C_NRM), i->character);
		send_to_char(buf, i->character);
		send_to_char(CCNRM(i->character, C_NRM), i->character);
	}
}


/*
 * extended_mudlog (xsyslog) -- logs special log messages to file as well as
 * to online immortals that have set their xsyslog to view logtype.
 * based on syslog by Fen Jul 3, 1992 and George Greer's new syslog.
 */
void extended_mudlog(int type, bitvector_t logtype, int file, const char *format, ...)
{
	char vbuf[MAX_STRING_LENGTH], printbuf1[MAX_STRING_LENGTH + 64], printbuf2[32];
	struct descriptor_data *i;
	va_list args;
	time_t ct = time(0);
	char *time_s = asctime(localtime(&ct));

	if (format == NULL)
		return;
	if (logtype < 0)
		return;

	va_start(args, format);
	if (file)
		vbasic_mud_log(logtype, format, args);
	vsnprintf(vbuf, sizeof(vbuf), format, args);
	proc_color(vbuf, 0, TRUE, 0, 0);
	va_end(args);

	for (i = descriptor_list; i; i = i->next) {
		if (!(STATE(i) == CON_PLAYING || STATE(i) == CON_COPYOVER) || IS_NPC(i->character)) /* switch */
			continue;
		if (!GOT_SYSLOG(i->character, logtype))
			continue;
		if (EDITING(i) || PLR_FLAGGED(i->character, PLR_OLC))
			continue;
		if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0)
			continue;

		time_s[strlen(time_s) - 1] = '\0';
		onebit(logtype, extended_syslogs, printbuf2, sizeof(printbuf2));

		sprintf(printbuf1, "%s--%-8.8s--%s--> %s%s\r\n", CCCYN(i->character, C_NRM), time_s + 11, printbuf2, vbuf, CCNRM(i->character, C_NRM));

		send_to_char(printbuf1, i->character);
	}

}


/*
 * If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.
 */
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
	size_t len = 0, nlen;
	long nr;

	*result = '\0';

	for (nr = 0; bitvector && len < reslen; bitvector >>= 1) {
		if (IS_SET(bitvector, 1)) {
			nlen = snprintf(result + len, reslen - len, "%s ", *names[nr] != '\n' ? names[nr] : "UNDEFINED");
			if (len + nlen >= reslen || nlen < 0)
				break;
			len += nlen;
		}

		if (*names[nr] != '\n')
			nr++;
	}

	if (!*result)
		len = strlcpy(result, "NOBITS ", reslen);
	
	return (len);
}


size_t onebit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
	size_t len = 0, nlen;
	long nr;

	*result = '\0';

	for (nr = 0; bitvector; bitvector >>= 1) {
		if (IS_SET(bitvector, 1)) {
			nlen = snprintf(result + len, reslen - len, "%s", *names[nr] != '\n' ? names[nr] : "UNDEFINED");
			if (len + nlen >= reslen || nlen < 0)
				break;
			len += nlen;
		}
		if (*names[nr] != '\n')
			nr++;
	}

	if (!*result)
		len = strlcpy(result, "NOBITS", reslen);
	
	return (len);
}


size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
	int nr = 0;

	while (type && *names[nr] != '\n') {
		type--;
		nr++;
	}

	return strlcpy(result, *names[nr] != '\n' ? names[nr] : "UNDEFINED", reslen);
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
	long secs;
	static struct time_info_data now;

	secs = t2 - t1;

	now.hours = (secs / SECS_PER_REAL_HOUR) % 24;        /* 0..23 hours */
	secs -= SECS_PER_REAL_HOUR * now.hours;

	now.day = (secs / SECS_PER_REAL_DAY);        /* 0..34 days  */
	/* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

	now.month = -1;
	now.year = -1;

	return (&now);
}


/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
	long secs;
	static struct time_info_data now;

	secs = t2 - t1;

	now.hours = (secs / SECS_PER_MUD_HOUR) % 24;        /* 0..23 hours */
	secs -= SECS_PER_MUD_HOUR * now.hours;

	now.day = (secs / SECS_PER_MUD_DAY) % 29;        /* 0..28 days  */
	secs -= SECS_PER_MUD_DAY * now.day;

	now.month = (secs / SECS_PER_MUD_MONTH) % 13;        /* 0..12 months */
	secs -= SECS_PER_MUD_MONTH * now.month;

	now.year = (secs / SECS_PER_MUD_YEAR);        /* 0..XX? years */

	return (&now);
}


time_t mud_time_to_secs(struct time_info_data *now)
{
  time_t when = 0;

  when += now->year  * SECS_PER_MUD_YEAR;
  when += now->month * SECS_PER_MUD_MONTH;
  when += now->day   * SECS_PER_MUD_DAY;
  when += now->hours * SECS_PER_MUD_HOUR;

  return (time(0) - when);
}


struct time_info_data *age(struct char_data *ch)
{
	static struct time_info_data player_age;

	player_age.year = time_info.year - ch->player_specials->saved.bday_year;        /* All players start at 17 */

	player_age.month = ch->player_specials->saved.bday_month;
	player_age.day = ch->player_specials->saved.bday_day;
	player_age.hours = ch->player_specials->saved.bday_hours;
		
	return (&player_age);
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data *ch, struct char_data *victim)
{
	struct char_data *k;

	for (k = victim; k; k = k->master) {
		if (k == ch)
			return (TRUE);
	}

	return (FALSE);
}



/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data *ch)
{
	struct follow_type *j, *k;

	if (ch->master == NULL) {
		core_dump();
		return;
	}

	if (AFF_FLAGGED(ch, AFF_CHARM)) {
		act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, TO_CHAR);
		act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, TO_NOTVICT);
		act("$n hates your guts!", FALSE, ch, 0, ch->master, TO_VICT);
		if (affected_by_spell(ch, SPELL_CHARM))
			affect_from_char(ch, SPELL_CHARM);
	} else {
		act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
		act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
		act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
	}

	if (ch->master->followers->follower == ch) {        /* Head of follower-list? */
		k = ch->master->followers;
		ch->master->followers = k->next;
		free(k);
	} else {                        /* locate follower who is not head of list */
		for (k = ch->master->followers; k->next->follower != ch; k = k->next);

		j = k->next;
		k->next = j->next;
		free(j);
	}

	ch->master = NULL;
	REMOVE_BIT(AFF_FLAGS(ch), AFF_CHARM | AFF_GROUP);
}


int num_followers_charmed(struct char_data *ch)
{
	struct follow_type *lackey;
	int total = 0;

	for (lackey = ch->followers; lackey; lackey = lackey->next)
		if (AFF_FLAGGED(lackey->follower, AFF_CHARM) && lackey->follower->master == ch)
			total++;

	return (total);
}


/* Called when a character that follows/is followed dies */
void die_follower(struct char_data *ch)
{
	struct follow_type *j, *k;

	if (ch->master)
		stop_follower(ch);

	for (k = ch->followers; k; k = j) {
		j = k->next;
		stop_follower(k->follower);
	}
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data *ch, struct char_data *leader)
{
	struct follow_type *k;

	if (ch->master) {
		core_dump();
		return;
	}

	ch->master = leader;

	CREATE(k, struct follow_type, 1);

	k->follower = ch;
	k->next = leader->followers;
	leader->followers = k;

	act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
	if (CAN_SEE(leader, ch))
		act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
	act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}


/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int	get_line(FILE * fl, char *buf)
{
	char temp[256];
	int lines = 0;
	int sl;

	do {
		if (!fgets(temp, 256, fl))
			return (0);
		lines++;
	} while (*temp == '*' || *temp == '\n' || *temp == '\r');

	/* Last line of file doesn't always have a \n, but it should. */
	sl = strlen(temp);
	while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
		temp[--sl] = '\0';

	strcpy(buf, temp);
	return (lines);
}


int	get_filename(char *orig_name, char *filename, int mode)
{
	const char *prefix, *middle, *suffix;
	char name[64], *ptr;

	if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "NULL pointer or empty string passed to get_filename(), %p or %p.",
								orig_name, filename);
		return (0);
	}

	switch (mode) {
	case ALIAS_FILE:
		prefix = LIB_PLRALIAS;
		suffix = SUF_ALIAS;
		break;
	case ETEXT_FILE:
		prefix = LIB_PLRTEXT;
		suffix = SUF_TEXT;
		break;
	case TRAV_FILE:
		prefix = LIB_PLRTRAV;
		suffix = SUF_TRAV;
		break;
	case SCRIPT_VARS_FILE:
		prefix = LIB_PLRVARS;
		suffix = SUF_MEM;
		break;
	default:
		return (0);
	}

	strcpy(name, orig_name);
	for (ptr = name; *ptr; ptr++)
		*ptr = LOWER(*ptr);

	switch (LOWER(*name)) {
	case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
		middle = "A-E";
		break;
	case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
		middle = "F-J";
		break;
	case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
		middle = "K-O";
		break;
	case 'p':  case 'q':  case 'r':  case 's':  case 't':
		middle = "P-T";
		break;
	case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
		middle = "U-Z";
		break;
	default:
		middle = "ZZZ";
		break;
	}

	sprintf(filename, "%s%s"SLASH"%s.%s", prefix, middle, name, suffix);
	return (1);
}


int	num_pc_in_room(struct room_data *room)
{
	int i = 0;
	struct char_data *ch;

	for (ch = room->people; ch != NULL; ch = ch->next_in_room)
		if (!IS_NPC(ch))
			i++;

	return (i);
}

/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
extern FILE *player_fl;
void core_dump_real(const char *who, int line)
{
	extended_mudlog(NRM, SYSL_BUGS, TRUE, "Assertion failed at %s:%d!", who, line);

#if	0        /* By default, let's not litter. */
#if	defined(CIRCLE_UNIX)
	/* These would be duplicated otherwise...make very sure. */
	fflush(stdout);
	fflush(stderr);
	fflush(logfile);
	fflush(player_fl);
	/* Everything, just in case, for the systems that support it. */
	fflush(NULL);

	/*
	 * Kill the child so the debugger or script doesn't think the MUD
	 * crashed.  The 'autorun' script would otherwise run it again.
	 */
	if (fork() == 0)
		abort();
#endif
#endif
}


/* General purpose search/replace function :
 * Pass "str" in a buffer of length MAX_STRING_LENGTH
 *
 * Return Values: 0 = Success (something was replaced)
 *                1 = Success (search string wasn't found)
 *                2 = Success (null string passed, nothing to replace)
 *                3 = Failure (Replace would overflow buffer)
 *                4 = Failure (Replacement string contains search pattern and
 *                            this is a recursive search)
 */
int	replace(char *str, char *search, const char *repl, int mode) {

	char *(*search_string) (const char *ss, const char *sp);
	char *eos = NULL, buf[MAX_STRING_LENGTH];
	long opos, slen = strlen(search), rlen = strlen(repl);
	bool found = FALSE;

	if (!*str || !*search) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "replace was passed a null pointer.");
		return(2);
	}

	/* Determine which function to use */
	if (IS_SET(mode, REPL_MATCH_CASE))
		search_string = strstr;
	else
		search_string = str_str;

	/* Replacement string contains search pattern on a recursive search. At
	 * best this would cause our function to return 3, at worst it'll lock
	 * up. Let's kill it here instead.
	 */
	if (IS_SET(mode, REPL_RECURSIVE) && search_string(repl, search))
		return(4);

	do {

		if (eos && !IS_SET(mode, REPL_RECURSIVE))
			eos = search_string(eos + rlen, search);
		else
			eos = search_string(str, search);

		if (eos) {
			found = TRUE;
			for (opos = 0; (str + opos) < eos; opos++)
					buf[opos] = *(str + opos);
			buf[opos] = '\0';

			if (strlen(buf) + strlen(repl) + strlen(eos + slen) < MAX_STRING_LENGTH) {
					sprintf(buf, "%s%s%s", buf, repl, eos + slen);
					strcpy(str, buf);
			} else
					return(3); /* replacement would overflow */
		} else
			break;

	} while (!IS_SET(mode, REPL_FIRST_ONLY));

	if (found)
			return(0);

	return(1);

}

/*
 * str_str: a case-insensitive version of strstr().
 */
char *str_str(const char *str, const char *txt) {

	unsigned int chk, i;
	
	if (str == NULL || txt == NULL)
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "str_str() passed a NULL pointer, %p or %p.", str, txt);
	else if (strlen(str) >= strlen(txt)) {
		
		for (i = 0, chk = 0; str[i]; i++) {
			if (LOWER(txt[chk]) == LOWER(str[i]))
				chk++;
			else {
				i -= chk;
				chk = 0;
			}
			
			if (chk == strlen(txt))
				return((char *)(str)+i-chk+1);
		}
	}
	
	return (NULL);
}


int	get_weapon_dam (struct obj_data *o)
{
	int i;
	
	for (i=0;i<MAX_OBJ_AFFECT;i++)
		if (o->affected[i].location == APPLY_DAMROLL)
			return (o->affected[i].modifier);
	return (0);
}


int	get_armor_ac (struct obj_data *o)
{
	
	int i;
	
	for (i=0;i<MAX_OBJ_AFFECT;i++)
		if (o->affected[i].location == APPLY_AC)
			return (o->affected[i].modifier);
	return (0);
}


obj_vnum object_name_to_num (char *namestr)
{
	int number;
	struct obj_data *obj;             
	int i=0;
	
	obj = obj_proto;
	for (i=0;i<top_of_objt;i++) {
		if (!isname(namestr, obj[i].name))
			continue;  
		else {
			number = GET_OBJ_VNUM(&obj[i]);               
			return(number);  
		}
	}        
	return(-1);
}


mob_vnum mobile_name_to_num (char *namestr)
{
	int number;
	struct char_data *mob;             
	int i=0;
	
	mob = mob_proto;
	for (i=0;i<top_of_mobt;i++) {
		if (!isname(namestr, mob[i].player.name))
			continue;  
		else {
			number = GET_MOB_VNUM(&mob[i]);               
			return(number);  
		}
	}     
	return(-1);   
}


void make_who2html(void)
{
	FILE *opf;
	struct descriptor_data *d;
	struct char_data *ch;
	
	if ((opf = fopen("who.html", "w")) == NULL) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "%s could not open who.html.", __FUNCTION__);
		return;
	}
	
	for(d = descriptor_list; d; d = d->next)
		if(!d->connected) {
			if(d->original)
				ch = d->original;
			else if (!(ch = d->character))
				continue;
			if(GET_INVIS_LEV(ch) <= RIGHTS_MEMBER) {
				sprintf(buf, "[%4s] <B>%s</B> %s\n<BR>", wiz_char_rights(get_max_rights(ch)), GET_NAME(ch), GET_TITLE(ch));
				fprintf(opf, buf);
			}
		} else
			fprintf(opf,"Apparently, nobody is logged on at the moment...");
	
	fclose(opf);
}


#define	ATEXT(c)      ((c) && (isalnum(c) || strchr(punct_okay, (c))))

bool valid_email(const char *em)
{
	static char punct_okay[] = "_-.";
	const char *mbox = em;

	/* First character can't be a dot. */
	if (*mbox == '.')
		return (FALSE);

	/* Don't allow someone to use root/postmaster/abuse/etc. mailbox. */
	if (!strn_cmp(mbox, "root@", 5) ||
			!strn_cmp(mbox, "postmaster@", 11) ||
			!strn_cmp(mbox, "abuse@", 6) ||
			!strn_cmp(mbox, "webmaster@", 10))
		return (FALSE);

	while (*mbox) {
		if (*mbox == '.')
			mbox++;
		if (!ATEXT(*mbox))
			break;

		mbox++;
	}

	if (mbox == em || *mbox != '@' || !*(++mbox) || *mbox == '.')
		return (FALSE);

	/* Don't allow someone to be "@localhost", etc. */
	if (!str_cmp(mbox, "localhost") ||
			!str_cmp(mbox, "127.0.0.1") ||
			!strn_cmp(mbox, "192.168.", 8))
		return (FALSE);

	while (*mbox) {
		if (*mbox == '.')
			mbox++;
		if (!ATEXT(*mbox))
			break;

		mbox++;
	}

	/* If we made it to the end, we have a valid e-mail address. */
	return (!*mbox);
}


void user_cntr(struct descriptor_data *d) {
	FILE *uc_fp;
	long u_cnt = 0;

	uc_fp = fopen("USRCNT", "r+b");
	if (uc_fp != NULL) {
		fread(&u_cnt, sizeof (long), 1, uc_fp);
		u_cnt ++;
		rewind(uc_fp);
		fwrite(&u_cnt, sizeof (long), 1, uc_fp);
		fclose(uc_fp);
		write_to_output(d, TRUE, "\r\nYou are player &W%ld&n to logon since 06/17/2001.\r\n\r\n", u_cnt);
	}
}

/* Functions to compare the rights of two players.
 * Return values are as such:
 * if ch > vict, return is 1
 * if ch < vict, return is -1
 * if ch = vict, reutrn is 0
 */
int check_rights(struct char_data *ch, struct char_data *vict)
{
	if (get_max_rights(ch) > get_max_rights(vict))
		return (1);
	else if (get_max_rights(ch) == get_max_rights(vict))
		return (0);
	else // (get_max_rights(ch) < get_max_rights(vict))
		return (-1);
}

int compare_rights(struct char_data *ch, struct char_data *vict)
{
	int ccount = 0, vcount = 0, compare = 0, i;

	for (i=0; i <= NUM_USER_RIGHTS; i++) {
		if (GOT_RIGHTS(ch, (1ULL<<i)))
			ccount++;
		if (GOT_RIGHTS(vict, (1ULL<<i)))
			vcount++;
	}

	if ((compare = check_rights(ch, vict)) != 0)  // someone's got a higher right.
		return compare;
	else {
		if (ccount > vcount)  //ch greater
			return 1;
		else if (ccount < vcount) //vict greater
			return -1;
		else //equals in all respects
			return 0;
	}
}

/* *cringe* */
bitvector_t get_max_rights(struct char_data *ch)
{
	if (GOT_RIGHTS(ch, RIGHTS_OWNER))
		return (RIGHTS_OWNER);
	if (GOT_RIGHTS(ch, RIGHTS_IMPLEMENTOR))
		return (RIGHTS_IMPLEMENTOR);
	if (GOT_RIGHTS(ch, RIGHTS_HEADBUILDER))
		return (RIGHTS_HEADBUILDER);
	if (GOT_RIGHTS(ch, RIGHTS_DEVELOPER))
		return (RIGHTS_DEVELOPER);
	if (GOT_RIGHTS(ch, RIGHTS_ADMIN))
		return (RIGHTS_ADMIN);
	if (GOT_RIGHTS(ch, RIGHTS_TRIGGERS))
		return (RIGHTS_TRIGGERS);
	if (GOT_RIGHTS(ch, RIGHTS_BUILDING))
		return (RIGHTS_BUILDING);
	if (GOT_RIGHTS(ch, RIGHTS_PLAYERS))
		return (RIGHTS_PLAYERS);
	if (GOT_RIGHTS(ch, RIGHTS_QUESTOR))
		return (RIGHTS_QUESTOR);
	if (GOT_RIGHTS(ch, RIGHTS_IMMORTAL))
		return (RIGHTS_IMMORTAL);
	if (GOT_RIGHTS(ch, RIGHTS_HELPFILES))
		return (RIGHTS_HELPFILES);
	if (GOT_RIGHTS(ch, RIGHTS_ACTIONS))
		return (RIGHTS_ACTIONS);
	if (GOT_RIGHTS(ch, RIGHTS_NEWBIES))
		return (RIGHTS_NEWBIES);
	if (GOT_RIGHTS(ch, RIGHTS_GUILDS))
		return (RIGHTS_GUILDS);
	if (GOT_RIGHTS(ch, RIGHTS_QUESTS))
		return (RIGHTS_QUESTS);
	if (GOT_RIGHTS(ch, RIGHTS_MEMBER))
		return (RIGHTS_MEMBER);

	return (RIGHTS_NONE);
}


char *wiz_char_rights(bitvector_t rights)
{
	if (rights == RIGHTS_OWNER) return ("own");
	else if (rights == RIGHTS_IMPLEMENTOR) return ("imp");
	else if (rights == RIGHTS_HEADBUILDER) return ("hbld");
	else if (rights == RIGHTS_DEVELOPER) return ("dev");
	else if (rights == RIGHTS_ADMIN) return ("adm");
	else if (rights == RIGHTS_TRIGGERS) return ("trg");
	else if (rights == RIGHTS_BUILDING) return ("bld");
	else if (rights == RIGHTS_PLAYERS) return ("pla");
	else if (rights == RIGHTS_QUESTOR) return ("qst");
	else if (rights == RIGHTS_IMMORTAL) return ("imm");
	else if (rights == RIGHTS_HELPFILES) return ("hlp");
	else if (rights == RIGHTS_ACTIONS) return ("act");
	else if (rights == RIGHTS_NEWBIES) return ("newb");
	else if (rights == RIGHTS_GUILDS) return ("gld");
	else if (rights == RIGHTS_QUESTS) return ("pqst");
	else if (rights == RIGHTS_MEMBER) return ("mem");
	else if (rights == RIGHTS_NONE) return ("none");
	else {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unknown rights passed to wiz_char_rights.");
		return ("error!");
	}
}


int wiz_int_rights(bitvector_t rights)
{
	if (rights == RIGHTS_OWNER) return (16);
	else if (rights == RIGHTS_IMPLEMENTOR) return (15);
	else if (rights == RIGHTS_HEADBUILDER) return (14);
	else if (rights == RIGHTS_DEVELOPER) return (13);
	else if (rights == RIGHTS_ADMIN) return (12);
	else if (rights == RIGHTS_TRIGGERS) return (11);
	else if (rights == RIGHTS_BUILDING) return (10);
	else if (rights == RIGHTS_PLAYERS) return (9);
	else if (rights == RIGHTS_QUESTOR) return (8);
	else if (rights == RIGHTS_IMMORTAL) return (7);
	else if (rights == RIGHTS_HELPFILES) return (6);
	else if (rights == RIGHTS_ACTIONS) return (5);
	else if (rights == RIGHTS_NEWBIES) return (4);
	else if (rights == RIGHTS_GUILDS) return (3);
	else if (rights == RIGHTS_QUESTS) return (2);
	else if (rights == RIGHTS_MEMBER) return (1);
	else if (rights == RIGHTS_NONE) return (0);
	else {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unknown rights passed to wiz_int_rights.");
		return (NOTHING);
	}
}


bitvector_t wiz_bit_rights(char *arg)
{
	if (!strcmp(arg, "own") || is_abbrev(arg, "owner")) return (RIGHTS_OWNER);
	else if (!strcmp(arg, "imp") || is_abbrev(arg, "implementor")) return (RIGHTS_IMPLEMENTOR);
	else if (!strcmp(arg, "hbld") || is_abbrev(arg, "headbuilder")) return (RIGHTS_HEADBUILDER);
	else if (!strcmp(arg, "dev") || is_abbrev(arg, "developer")) return (RIGHTS_DEVELOPER);
	else if (!strcmp(arg, "adm") || is_abbrev(arg, "admin")) return (RIGHTS_ADMIN);
	else if (!strcmp(arg, "trg") || is_abbrev(arg, "triggers")) return (RIGHTS_TRIGGERS);
	else if (!strcmp(arg, "bld") || is_abbrev(arg, "building")) return (RIGHTS_BUILDING);
	else if (!strcmp(arg, "pla") || is_abbrev(arg, "players")) return (RIGHTS_PLAYERS);
	else if (!strcmp(arg, "qst") || is_abbrev(arg, "questor")) return (RIGHTS_QUESTOR);
	else if (!strcmp(arg, "imm") || is_abbrev(arg, "immortal")) return (RIGHTS_IMMORTAL);
	else if (!strcmp(arg, "hlp") || is_abbrev(arg, "helpfiles")) return (RIGHTS_HELPFILES);
	else if (!strcmp(arg, "act") || is_abbrev(arg, "actions")) return (RIGHTS_ACTIONS);
	else if (!strcmp(arg, "newb") || is_abbrev(arg, "newbies")) return (RIGHTS_NEWBIES);
	else if (!strcmp(arg, "gld") || is_abbrev(arg, "guilds")) return (RIGHTS_GUILDS);
	else if (!strcmp(arg, "pqst") || is_abbrev(arg, "questing")) return (RIGHTS_QUESTS);
	else if (!strcmp(arg, "mem") || is_abbrev(arg, "members")) return (RIGHTS_MEMBER);
	else return (RIGHTS_NONE);
}


int   sprintascii(char *out, bitvector_t bits);
bool attributes_menu(struct char_data *ch, char *argument);

ACMD(do_tester)
{
	struct char_data *vict;

	one_argument(argument, arg);

	extended_mudlog(BRF, SYSL_GENERAL, FALSE, "%s true name is: %s.", GET_NAME(ch), true_name(GET_ID(ch)));
	extended_mudlog(BRF, SYSL_GENERAL, FALSE, "%s called the do_tester command.", GET_NAME(ch));

	if (!*arg) {
		send_to_char("Testing get_max_rights...\r\n", ch);
		sprintascii(buf1, get_max_rights(ch));
		sprintf(buf, "   get_max_rights: %s, %s\r\n", buf1, wiz_char_rights(get_max_rights(ch)));
		send_to_char(buf, ch);
		send_to_char("Testing wiz_bit_rights with bld...\r\n", ch);
		sprintascii(buf1, wiz_bit_rights("bld"));
		sprintf(buf, "   wiz_bit_rights: %s, %s\r\n", buf1, wiz_char_rights(wiz_bit_rights("bld")));
		send_to_char(buf, ch);
		sprintascii(buf1, wiz_bit_rights("build"));
		send_to_char("Testing wiz_bit_rights with build...\r\n", ch);
		sprintf(buf, "   wiz_bit_rights: %s, %s\r\n", buf1, wiz_char_rights(wiz_bit_rights("build")));
		send_to_char(buf, ch);
	} else {
		if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD, 1)) != NULL) {
			send_to_char("Testing check_rights...\r\n", ch);
			sprintf(buf, "   check_rights: %d, %s\r\n", check_rights(ch, vict), 
				(check_rights(ch, vict) == 0 ? "equal" : 
				(check_rights(ch, vict) == 1 ? GET_NAME(ch) : GET_NAME(vict)) ));
			send_to_char(buf, ch);
			send_to_char("Testing compare_rights...\r\n", ch);
			sprintf(buf, "   compare_rights: %d, %s\r\n", compare_rights(ch, vict), 
				(compare_rights(ch, vict) == 0 ? "equal" : 
				(compare_rights(ch, vict) == 1 ? GET_NAME(ch) : GET_NAME(vict)) ));
			send_to_char(buf, ch);
		} else 
			send_to_char(NOPERSON, ch);
	}

}


/*
 * Soundex function adapted from PHP Zend.
 * This particular soundex function is one described by
 * Donald Knuth in "The Art Of Computer Programming,
 * vol. 3: Sorting And Searching", Addison-Wesley (1973),
 * pp. 391-392.
 * Adapted for Arcane Realms / CircleMUD by Torgny Bjers.
 * http://www.arcanerealms.org/
 * artovil@arcanerealms.org
 */
char *soundex(char *str)
{
	int i, _small, code, last;
	char soundex[4 + 1];

	static char soundex_table[26] =
	{0,							/* A */
	 '1',						/* B */
	 '2',						/* C */
	 '3',						/* D */
	 0,							/* E */
	 '1',						/* F */
	 '2',						/* G */
	 0,							/* H */
	 0,							/* I */
	 '2',						/* J */
	 '2',						/* K */
	 '4',						/* L */
	 '5',						/* M */
	 '5',						/* N */
	 0,							/* O */
	 '1',						/* P */
	 '2',						/* Q */
	 '6',						/* R */
	 '2',						/* S */
	 '3',						/* T */
	 0,							/* U */
	 '1',						/* V */
	 0,							/* W */
	 '2',						/* X */
	 0,							/* Y */
	 '2'};					/* Z */

	if (!*str)
		return ('\0');

	/* build soundex string */
	last = -1;
	for (i = 0, _small = 0; i < strlen(str) && _small < 4; i++) {
		/* convert chars to upper case and strip non-letter chars */
		code = toupper(str[i]);
		if (code >= 'A' && code <= 'Z') {
			if (_small == 0) {
				/* remember first valid char */
				soundex[_small++] = code;
				last = soundex_table[code - 'A'];
			}
			else {
				/* ignore sequences of consonants with same soundex */
				/* code in trail, and vowels unless they separate */
				/* consonant letters */
				code = soundex_table[code - 'A'];
				if (code != last) {
					if (code != 0) {
						soundex[_small++] = code;
					}
					last = code;
				}
			}
		}
	}
	/* pad with '0' and terminate with 0 ;-) */
	while (_small < 4) {
		soundex[_small++] = '0';
	}
	soundex[_small] = '\0';

	str = str_dup(soundex);

	return (str);

}


char *true_name (int id)
{
	int pattern = 0, tmp, tenthousands, thousands, hundreds, tens, ones;

	if (!id) {
		extended_mudlog(BRF, SYSL_BUGS, FALSE, "%s:%s(%d) called without a valid ID.", __FILE__, __FUNCTION__, __LINE__);
		return ("\0");
	}

	ones = pattern % 10;				/* 12345 = 5    */
	tmp = pattern / 10;					/* 12345 = 1234 */
	tens = tmp % 10;						/* 1234  = 4    */ 
	tmp = tmp / 10;							/* 1234  = 123  */
	hundreds = tmp % 10;				/* 123   = 3    */
	tmp = tmp / 10;							/* 123   = 12   */
	thousands = tmp % 10;				/* 12    = 2    */
	if (tmp > 0) 
		tenthousands = tmp / 10;	/* 12    = 1    */
	else
		tenthousands = 0;

	sprintf(buf2, "%s%s%s%s%s",
		arc_sylls[0][number(0,1)][tenthousands], arc_sylls[1][number(0,1)][thousands], 
		arc_sylls[2][number(0,1)][hundreds], arc_sylls[3][number(0,1)][tens], 
		arc_sylls[4][number(0,1)][ones]);

	return (buf2);

}


/* returns new string, translating $n into a name. */
char *translate_code (const char *str, struct char_data *ch, struct char_data *vict, char *mybuf) 
{
  int i;

	for (i = 0; i < strlen(str); i++) {
		if (str[i] == '$') {
			if (str[i+1] == 'n') { /* the bastardly 'n' */
				/* next char! */
				i++;
				strcat(mybuf, (find_recognized(vict, ch) ? GET_NAME(ch) : GET_SDESC(ch)));
			}	else { /* add the $ and the char, it's not lookin' for a name */
				char me[2];
				
				me[0] = str[i];
				me[1] = '\0';
				
				strcat(mybuf, me); 		
			}

		} /* end if for '$' */
		else { /* not '$' */
			char me[2];
			me[0] = str[i];
			me[1] = '\0';
			strcat(mybuf,me);
		}
	
	} /* end for loop */
	
	return (CAP(mybuf));
}


EVENTFUNC(sysintensive_delay)
{
	struct delay_event_obj *seo = (struct delay_event_obj *) event_obj;
	struct char_data *vict;
	int type;

	vict = seo->vict;	/* pointer to vict		*/
	type = seo->type;	/* skill type					*/
	--seo->time;			/* subtract from time	*/

	switch(type) {
	case EVENT_SYSTEM:
		if (seo->time <= 0) {
			free(event_obj);
			GET_PLAYER_EVENT(vict, type) = NULL;
			return (0);
		}
		break;
	default:
		extended_mudlog(NRM, SYSL_GENERAL, TRUE, "Invalid type #%d passed to %s, %s:%d.", type, __FILE__, __FUNCTION__, __LINE__);
		return (0);
	}

	return (60 RL_SEC);
}


char *color_name (char color)
{
	if (!color)
		return ("");
	switch (color) {
		case 'n': return ("normal");
		case 'b': return ("blue");
		case 'c': return ("cyan");
		case 'd': return ("black");
		case 'g': return ("green");
		case 'm': return ("magenta");
		case 'r': return ("red");
		case 'w': return ("light gray");
		case 'y': return ("yellow");
		case 'B': return ("bright blue");
		case 'C': return ("bright cyan");
		case 'D': return ("dark gray");
		case 'G': return ("bright green");
		case 'M': return ("bright magenta");
		case 'R': return ("bright red");
		case 'W': return ("white");
		case 'Y': return ("bright yellow");
		default: return ("");
	}
}


/*
 * Rules (unless overridden by ROOM_DARK):
 *
 * Inside and City rooms are always lit.
 * Outside rooms are dark at sunset and night.
 */
int room_is_dark(room_rnum room)
{
	if (!VALID_ROOM_RNUM(room)) {
		mlog("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
		return (FALSE);
	}

	if (world[room].light)
		return (FALSE);

	if (ROOM_FLAGGED(room, ROOM_DARK))
		return (TRUE);

	if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_CITY)
		return (FALSE);

	if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
		return (TRUE);

	return (FALSE);
}


/*
 * Rules (unless overridden by ROOM_DARK):
 *
 * Inside and City rooms are always lit.
 * Outside rooms are dark at sunset and night.
 */
int room_always_lit(room_rnum room)
{
	if (!VALID_ROOM_RNUM(room)) {
		mlog("room_always_lit: Invalid room rnum %d. (0-%d)", room, top_of_world);
		return (FALSE);
	}

	if (ROOM_FLAGGED(room, ROOM_ALWAYS_LIT))
		return (TRUE);

	return (FALSE);
}


/*
 * Readable proficiencies for players and tutors.
 */
const char *readable_proficiency(int percent)
{
	if (percent >= 0 && percent <= 25)
		return (tutor_proficiency[0]);
	else if (percent > 25 && percent < 51)
		return (tutor_proficiency[1]);
	else if (percent > 50 && percent < 76)
		return (tutor_proficiency[2]);
	else if (percent > 75 && percent < 100)
		return (tutor_proficiency[3]);
	else if (percent == 100)
		return (tutor_proficiency[4]);
	return ("none");
}


/*
 * Takes an L-Desc and creates keywords from them.
 * Torgny Bjers (artovil@arcanerealms.org), 2002-12-31.
 * Happy New Year MUD.
 */
char *create_keywords(char *inbuf)
{
	int i;
	char *outbuf = '\0';
	char *basic_words[] = {
		" an ",
		" a ",
		" the ",
		" with ",
		" on ",
		" of ",
		" and ",
		"stands",
		"here",
		"bearing",
		"eyes",
		"this",
		"hair",
		"face",
		"tall",
		"dark",
		"length",
		",",
		".",
		";",
		":",
		"?",
		"!",
		"    ",
		"   ",
		"  ",
		"\n"
	};

	if (strlen(inbuf) < 3)
		return ("");

	outbuf = strdup(inbuf);

	strcpy(outbuf, LOWERALL(outbuf));

	if (outbuf[0] == 'a' && outbuf[1] == ' ')
		replace(outbuf, "a ", "", REPL_FIRST_ONLY);
	else if (outbuf[0] == 'a' && outbuf[1] == 'n' && outbuf[2] == ' ')
		replace(outbuf, "an ", "", REPL_FIRST_ONLY);

	for (i = 0; *basic_words[i] != '\n'; i++)
		replace(outbuf, basic_words[i], " ", REPL_NORMAL);

	skip_spaces(&outbuf);

	return (outbuf && *outbuf ? strdup(outbuf) : "");
}


char menu_letter(int number)
{
	switch(number) {
		case 0: return 'a';
		case 1: return 'b';
		case 2: return 'c';
		case 3: return 'd';
		case 4: return 'e';
		case 5: return 'f';
		case 6: return 'g';
		case 7: return 'h';
		case 8: return 'i';
		case 9: return 'j';
		case 10: return 'k';
		case 11: return 'l';
		case 12: return 'm';
		case 13: return 'n';
		case 14: return 'o';
		case 15: return 'p';
		case 16: return 'q';
		case 17: return 'r';
		case 18: return 's';
		case 19: return 't';
		case 20: return 'u';
		case 21: return 'v';
		case 22: return 'w';
		case 23: return 'x';
		case 24: return 'y';
		case 25: return 'z';
		case 26: return 'A';
		case 27: return 'B';
		case 28: return 'C';
		case 29: return 'D';
		case 30: return 'E';
		case 31: return 'F';
		case 32: return 'G';
		case 33: return 'H';
		case 34: return 'I';
		case 35: return 'J';
		case 36: return 'K';
		case 37: return 'L';
		case 38: return 'M';
		case 39: return 'N';
		case 40: return 'O';
		default: return '-';
	}
}


int parse_menu(char arg)
{
	switch (arg) {
		case 'a': return 0;
		case 'b': return 1;
		case 'c': return 2;
		case 'd': return 3;
		case 'e': return 4;
		case 'f': return 5;
		case 'g': return 6;
		case 'h': return 7;
		case 'i': return 8;
		case 'j': return 9;
		case 'k': return 10;
		case 'l': return 11;
		case 'm': return 12;
		case 'n': return 13;
		case 'o': return 14;
		case 'p': return 15;
		case 'q': return 16;
		case 'r': return 17;
		case 's': return 18;
		case 't': return 19;
		case 'u': return 20;
		case 'v': return 21;
		case 'w': return 22;
		case 'x': return 23;
		case 'y': return 24;
		case 'z': return 25;
		case 'A': return 26;
		case 'B': return 27;
		case 'C': return 28;
		case 'D': return 29;
		case 'E': return 30;
		case 'F': return 31;
		case 'G': return 32;
		case 'H': return 33;
		case 'I': return 34;
		case 'J': return 35;
		case 'K': return 36;
		case 'L': return 37;
		case 'M': return 38;
		case 'N': return 39;
		case 'O': return 40;
		default: return NOTHING;
	}
}
