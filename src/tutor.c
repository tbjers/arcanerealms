/* ************************************************************************
*   File: tutor.c                               An extension to CircleMUD *
*  Usage: Tutor specific functions and special procedures                 *
*                                                                         *
*  This file by Torgny Bjers <artovil@arcanerealms.org>                   *
*  Copyright (C) 2002 by Arcane Realms MUD, www.arcanerealms.org.         *
************************************************************************ */
/* $Id: tutor.c,v 1.9 2004/04/23 15:18:10 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "genolc.h"
#include "oasis.h"
#include "dg_olc.h"
#include "constants.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "tutor.h"

/* external globals */
extern struct index_data *mob_index;
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct room_data *world;
extern struct index_data *obj_index;
extern const char *unused_spellname;


/* external functions */
ACMD(do_tell);
SKILL(skl_tutor);

/* local globals */
int tnum					=	0;
int newtutor			=	0;
int top_of_tutort	=	0;
struct tutor_info_type *tutor_info;
int	cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_puke;

/* local defines */
#define	SKINFO skill_info[skillnum]
#define	SKTREE skill_tree[skillno]


void assign_the_tutors(void)
{
	struct tutor_info_type *tptr;
	mob_rnum mrnum;

	cmd_say = find_command("say");
	cmd_tell = find_command("tell");
	cmd_emote = find_command("emote");
	cmd_slap = find_command("slap");
	cmd_puke = find_command("puke");

	for (tptr = tutor_info; tptr; tptr = tptr->next) {
		if ((mrnum = real_mobile(tptr->vnum)) == NOBODY)
			continue;
		mob_index[mrnum].func = tutor;
	}
}


void remove_from_tutor_skills(struct tutor_skill_data **list, int num)
{
	int i, num_items;
	struct tutor_skill_data *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i].skill != -1; i++);

	if (num < 0 || num >= i)
		return;
	num_items = i;

	CREATE(nlist, struct tutor_skill_data, num_items);

	for (i = 0; i < num_items; i++)
		nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

	free(*list);
	*list = nlist;
}


void add_to_tutor_skills(struct tutor_skill_data **list, struct tutor_skill_data *newl)
{
	int i, num_items;
	struct tutor_skill_data *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i].skill != -1; i++);
	num_items = i;

	/*
	 * Make a new list and slot in the new entry.
	 */
	CREATE(nlist, struct tutor_skill_data, num_items + 2);

	for (i = 0; i < num_items; i++)
		nlist[i] = (*list)[i];
	nlist[num_items] = *newl;
	nlist[num_items + 1].skill = -1;

	/*
	 * Out with the old, in with the new.
	 */
	free(*list);
	*list = nlist;
}


/*
 * Copy a -1 terminated (in the type field) shop_buy_data 
 * array list.
 */
void copy_tutor_skills(struct tutor_skill_data **tlist, struct tutor_skill_data *flist)
{
	int num_items, i;

	if (*tlist)
		free(*tlist);

	/*
	 * Count number of entries.
	 */
	for (i = 0; TUTOR_SKL_SKILL(flist[i]) != -1; i++);
	num_items = i + 1;

	/*
	 * Make space for entries.
	 */
	CREATE(*tlist, struct tutor_skill_data, num_items);

	/*
	 * Copy entries over.
	 */
	for (i = 0; i < num_items; i++) {
		(*tlist)[i].skill = flist[i].skill;
		(*tlist)[i].proficiency = flist[i].proficiency;
		(*tlist)[i].cost = flist[i].cost;
	}
}


struct tutor_info_type *get_tutor(int tutornum, const char *file, const char *function)
{
	struct tutor_info_type *tptr;
	
	if (tutornum < 1 || tutornum > top_of_tutort) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "%s,%s(%d): Called by %s:%s with invalid tutornum (%d)", __FILE__, __FUNCTION__, __LINE__, file, function, tutornum);
		return (NULL);
	} 
	
	for (tptr = tutor_info; tptr; tptr = tptr->next)
		if (tptr->number == tutornum)
			return (tptr);
	
	return (NULL);
}


int	find_tutor_num(mob_vnum mvnum)
{
  struct tutor_info_type *tptr;

	for (tptr = tutor_info; tptr; tptr = tptr->next) {
		if (tptr->number == 0) continue;
		if (mvnum == tptr->vnum)
			return (tptr->number);
	}

	return (-1);
}


void tutor_list_skills(struct tutor_info_type *t, struct char_data *tutor, struct char_data *ch)
{
	if (t == NULL || T_SKILL(t, 0) == NOTHING) {
		sprintf(buf, "%s Currently I cannot teach you any skills.", GET_NAME(ch));
		do_tell(tutor, buf, cmd_tell, 0);
		return;
	} else {
		char *list = get_buffer(MAX_STRING_LENGTH);
		int i;
		get_char_colors(ch);
		sprintf(list,	" Skill                                             Proficiency    Cost/Lesson\r\n"
									"------------------------------------------------------------------------------\r\n");
		for (i = 0; T_SKILL(t, i) != -1; i++) {
			sprintf(list, "%s %s%-45.45s%s     %s%-12.12s   %-4d%s\r\n", list,
				yel, skill_name(T_SKILL(t, i)), nrm,
				cyn, readable_proficiency(T_PROFICIENCY(t, i) / 100), T_COST(t, i), nrm);
		}
		page_string(ch->desc, list, TRUE);
		release_buffer(list);
	}

}


/* Puts tutors in numerical order */
void sort_tutors(void)
{
	struct tutor_info_type *tptr = NULL, *temp = NULL;

	if (tnum > 2) {
		for (tptr = tutor_info; (tptr->next != NULL) && (tptr->next->next != NULL); tptr = tptr->next) {
			if (tptr->next->number > tptr->next->next->number) {
				temp = tptr->next;
				tptr->next = temp->next;
				temp->next = tptr->next->next;
				tptr->next->next = temp;
			}
		}
	}
	
	return;
}


void free_tutor(struct tutor_info_type *t)
{
	free_tutor_strings(t);
	free(T_SKILLS(t));
	free(t);
}


void free_tutor_strings(struct tutor_info_type *t)
{
	if (T_NOSKILL(t)) {
		free(T_NOSKILL(t));
		T_NOSKILL(t) = NULL;
	}
	if (T_NOREQ(t)) {
		free(T_NOREQ(t));
		T_NOREQ(t) = NULL;
	}
	if (T_SKILLED(t)) {
		free(T_SKILLED(t));
		T_SKILLED(t) = NULL;
	}
	if (T_NOCASH(t)) {
		free(T_NOCASH(t));
		T_NOCASH(t) = NULL;
	}
	if (T_BUYSUCCESS(t)) {
		free(T_BUYSUCCESS(t));
		T_BUYSUCCESS(t) = NULL;
	}
}


struct tutor_info_type *enqueue_tutor(void)
{
	struct tutor_info_type *tptr;
	
	/* This is the first tutor loaded if true */
	if (tutor_info == NULL) {
		if ((tutor_info = (struct tutor_info_type *) malloc(sizeof(struct tutor_info_type))) == NULL) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Out of memory for tutors! Aborting...");
			kill_mysql();
			exit(1);
		} else {
			tutor_info->next = NULL;
			return (tutor_info);
		}
	} else { /* tutor_info is not NULL */
		for (tptr = tutor_info; tptr->next != NULL; tptr = tptr->next); /* Loop does the work */
		if ((tptr->next = (struct tutor_info_type *) malloc(sizeof(struct tutor_info_type))) == NULL) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Out of memory for tutors! Aborting...");
			kill_mysql();
			exit(1);
		} else {
			tptr->next->next = NULL;
			return (tptr->next);
		}
	}
	return NULL;
}


void dequeue_tutor(int tutornum)
{
	struct tutor_info_type *tptr = NULL, *temp;
	
	if (tutornum < 0 || tutornum > tnum) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Attempting to dequeue invalid tutor!");
		kill_mysql();
		exit(1);
	} else {
		if (tutor_info->number != tutornum) {
			for (tptr = tutor_info; tptr->next && tptr->next->number != tutornum; tptr = tptr->next)
				;
			if (tptr->next != NULL && tptr->next->number == tutornum) {
				temp = tptr->next;
				tptr->next = temp->next;
				free_tutor(temp);
			}
		} else {
		/* The first one is the one being removed */
			tptr = tutor_info;
			tutor_info = tutor_info->next;
			free_tutor(tptr);
		}
	}
}


/* Loads the tutors from the text file */
void load_tutors(void)
{
	MYSQL_RES *tutors, *skills;
	MYSQL_ROW row, skill_row;
	unsigned long *fieldlength;
	int tutornum = 0, temp = 0, count = 0;
	struct tutor_info_type *tptr = NULL;
	bool news = FALSE;

	if (!(tutors = mysqlGetResource(TABLE_TUTOR_INDEX, "SELECT * FROM %s ORDER BY ID ASC;", TABLE_TUTOR_INDEX))) {
		extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not load tutor index from table [%s].", TABLE_TUTOR_INDEX);
		return;
	}

	tnum = mysql_num_rows(tutors);
	top_of_tutort = tnum;

	if(tnum == 0) {
		tutor_info = NULL;
		top_of_tutort = -1;
		return;
	}

	for (tutornum = 0; tutornum < tnum; tutornum++) {
		row = mysql_fetch_row(tutors);
		fieldlength = mysql_fetch_lengths(tutors);

		/* create some spell shaped memory space */
		if ((tptr = enqueue_tutor()) != NULL) {
			tptr->number = atoi(row[0]);

			/* setup the global number of next new spell number */
			if (!news) {
				if (tptr->number != tutornum) {
					newtutor = tutornum;
					news = TRUE;
				}
			}

			if (news) {
				if (newtutor == tptr->number) {
					newtutor = top_of_tutort;
					news = FALSE;
				}
			} else
				newtutor = tptr->number + 1;

			/*
			 * Tutor mobile vnum
			 */
			tptr->vnum = atoi(row[1]);

			/*
			 * Read the strings in first.
			 */
			tptr->no_skill = NULL_STR(fieldlength[2], row[2]);
			tptr->no_req = NULL_STR(fieldlength[3], row[3]);
			tptr->skilled = NULL_STR(fieldlength[4], row[4]);
			tptr->no_cash = NULL_STR(fieldlength[5], row[5]);
			tptr->buy_success = NULL_STR(fieldlength[6], row[6]);

			tptr->flags = atoi(row[7]);

			/*
			 * Read skills list.
			 */
			if (!(skills = mysqlGetResource(TABLE_TUTOR_SKILLS, "SELECT * FROM %s WHERE tutor = %d ORDER BY ID ASC LIMIT %d;", TABLE_TUTOR_SKILLS, tptr->number, MAX_LIST_SKILLS))) {
				extended_mudlog(BRF, SYSL_SQL, TRUE, "Could not load tutor skills from table [%s].", TABLE_TUTOR_SKILLS);
				return;
			}

			/*
			 * Create the skills list on the tutor.
			 */
			if ((temp = mysql_num_rows(skills)) > 0) {
				CREATE(tptr->skills, struct tutor_skill_data, temp);
				for (count = 0; count < temp && count < MAX_LIST_SKILLS; count++) {
					skill_row = mysql_fetch_row(skills);
					T_SKILL(tptr, count) = atoi(skill_row[2]);
					T_PROFICIENCY(tptr, count) = atoi(skill_row[3]);
					T_COST(tptr, count) = atoi(skill_row[4]);
				}
			} else {
				CREATE(tptr->skills, struct tutor_skill_data, 1);
				T_SKILL(tptr, 0) = NOTHING;
				T_PROFICIENCY(tptr, 0) = 0;
				T_COST(tptr, 0) = 0;
			}				

		} else break;
		/* process the next tutor */

	}

	mlog("   %d tutors in database.", top_of_tutort - 1);

}


void save_tutors(void)
{
	struct tutor_info_type *tptr = tutor_info;
	int tutornum = 0, j;
	char *no_skill;
	char *no_req;
	char *skilled;
	char *no_cash;
	char *buy_success;

	const char *tutor_data = "REPLACE INTO %s ("
		"ID, "						// 1
		"vnum, "
		"no_skill, "
		"no_req, "
		"skilled, "				// 5
		"no_cash, "
		"buy_success, "
		"flags"						// 8
	") VALUES ("
		"%d, "						// 1
		"%d, "
		"'%s', "
		"'%s', "
		"'%s', "					// 5
		"'%s', "
		"'%s', "
		"%llu"						// 8
	");";

	const char *skill_data = "INSERT INTO %s ("
		"tutor, "					// 1
		"skillnum, "
		"proficiency, "
		"cost"						// 4
	") VALUES ("
		"%d, "						// 1
		"%d, "
		"%d, "
		"%d"							// 4
	");";


	if (tptr == NULL) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "No tutors to save.");
		return;
	}
	
	/* Put the tutors in order */
	sort_tutors();

	mysqlWrite("DELETE FROM %s WHERE ID >= %d;", TABLE_TUTOR_INDEX, tnum - 1);
	mysqlWrite("DELETE FROM %s;", TABLE_TUTOR_SKILLS);

	while (tutornum < tnum && tptr != NULL) {

		SQL_MALLOC(T_NOSKILL(tptr), no_skill);
		SQL_MALLOC(T_NOREQ(tptr), no_req);
		SQL_MALLOC(T_SKILLED(tptr), skilled);
		SQL_MALLOC(T_NOCASH(tptr), no_cash);
		SQL_MALLOC(T_BUYSUCCESS(tptr), buy_success);

		SQL_ESC(T_NOSKILL(tptr), no_skill);
		SQL_ESC(T_NOREQ(tptr), no_req);
		SQL_ESC(T_SKILLED(tptr), skilled);
		SQL_ESC(T_NOCASH(tptr), no_cash);
		SQL_ESC(T_BUYSUCCESS(tptr), buy_success);

		if (!(mysqlWrite(
			tutor_data,
			TABLE_TUTOR_INDEX,
			tptr->number,
			tptr->vnum,
			no_skill,
			no_req,
			skilled,
			no_cash,
			buy_success,
			tptr->flags
		))) {
			extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "Error writing tutor %d (vnum #%d) to database.", tptr->number, tptr->vnum);
			return;
		}

		SQL_FREE(no_skill);
		SQL_FREE(no_req);
		SQL_FREE(skilled);
		SQL_FREE(no_cash);
		SQL_FREE(buy_success);

		/*
		 * Write the skills down to database.
		 */
		j = -1;
		do {
			j++;
			mysqlWrite(skill_data, TABLE_TUTOR_SKILLS, tptr->number, T_SKILL(tptr, j), T_PROFICIENCY(tptr, j), T_COST(tptr, j));
		} while (T_SKILL(tptr, j) != -1);

		/* process the next tutor */
		tptr = tptr->next;
		tutornum++;
	}

}


int tutor_skill_level(struct tutor_info_type *t, int skillno)
{
	int j = -1;
	do {
		j++;
		if (T_SKILL(t, j) == SKTREE.skill)
			return (T_PROFICIENCY(t, j));
	} while (T_SKILL(t, j) != -1);

	return (NOTHING);
}


int tutor_skill_cost(struct tutor_info_type *t, int skillno)
{
	int j = -1;
	do {
		j++;
		if (T_SKILL(t, j) == SKTREE.skill)
			return (T_COST(t, j));
	} while (T_SKILL(t, j) != -1);

	return (-1);
}


void finish_tutor_session(struct char_data *student, struct char_data *tutor, int skillno)
{
	int skill_gain = (GET_DIFFICULTY(tutor) + 1) * 100;
	char *printbuf = get_buffer(1024);

	act("$n finishes $s lesson with you.", FALSE, tutor, 0, student, TO_VICT);
	act("$n finished $s lesson with $N.", TRUE, tutor, 0, student, TO_NOTVICT);

	if (skill_cap(student, skill_gain, FALSE) > GET_SKILLCAP(student)) {
		if (!decrease_skills(student, skill_gain)) {
			sprintf(printbuf, "%s You seem to have capped, I am afraid you did not gain anything.", GET_NAME(student));
			do_tell(tutor, printbuf, cmd_tell, 0);
		}
	} else {
		if (GET_SKILL(student, SKTREE.skill) == 0)
			sprintf(printbuf, "You have gained '&W%s&n'.\r\n", skill_name(SKTREE.skill));
		else
			sprintf(printbuf, "Your lesson improves '&W%s&n'.\r\n", skill_name(SKTREE.skill));
		GET_SKILL(student, SKTREE.skill) += skill_gain;
		send_to_char(printbuf, student);
	}
	release_buffer(printbuf);
}


EVENTFUNC(student_delay)
{
	struct tutor_event_obj *seo = (struct tutor_event_obj *) event_obj;
	struct char_data *student;

	student = seo->student;	/* pointer to ch			*/
	--seo->time;						/* subtract from time	*/

	if (seo->time <= 0) {
		/*
		 * The event time has expired, now we call the proper
		 * function and clean up.
		 */
		finish_tutor_session(student, seo->tutor, seo->skill);
		free(event_obj);
		GET_PLAYER_EVENT(student,	EVENT_SKILL) = NULL;
		return (0);
	}

	return (PULSE_SKILL);
}


SPECIAL(tutor)
{
	struct char_data *tutor = (struct char_data *)me;
	struct tutor_info_type *tptr;
	int tutornum, skillno;
	struct tutor_event_obj *deo;

	if (FIGHTING(tutor) || !AWAKE(tutor))
		return (FALSE);

	tutornum = find_tutor_num(GET_MOB_VNUM(tutor));

	if ((tptr = get_tutor(tutornum, __FILE__, __FUNCTION__)) != NULL) {

		if (CMD_IS("list"))
			tutor_list_skills(tptr, tutor, ch);
		else if (CMD_IS("learn")) {
			if (GET_PLAYER_EVENT(ch, EVENT_SKILL)) {
				/*
				 * The player already has an action event, and thus, has to wait
				 * until the event has been completed or cancelled.
				 */
				send_to_char("You are occupied and cannot learn anything now.\r\n", ch);
				return (TRUE);
			} else if (!*argument) {
				sprintf(buf, T_NOSKILL(tptr), GET_NAME(ch));
				do_tell(tutor, buf, cmd_tell, 0);
				return (TRUE);
			} else {
				char *sklname = get_buffer(MAX_INPUT_LENGTH);
				int qend = 0, sklnum = 0, skill_level = 0, skill_cost = 0;

				skip_spaces(&argument);

				/* If there are no chars in argument */
				if (!*argument) {
					sprintf(buf, "%s Skill name expected.", GET_NAME(ch));
					do_tell(tutor, buf, cmd_tell, 0);
					release_buffer(sklname);
					return (TRUE);
				}

				if (*argument != '\'') {
					sprintf(buf, "%s Skill must be enclosed in: ''", GET_NAME(ch));
					do_tell(tutor, buf, cmd_tell, 0);
					release_buffer(sklname);
					return (TRUE);
				}

				/* Locate the last quote and lowercase the magic words (if any) */
				for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
					argument[qend] = LOWER(argument[qend]);
			
				if (argument[qend] != '\'') {
					sprintf(buf, "%s Skill must be enclosed in: ''", GET_NAME(ch));
					do_tell(tutor, buf, cmd_tell, 0);
					release_buffer(sklname);
					return (TRUE);
				}

				strcpy(sklname, (argument + 1));
				sklname[qend - 1] = '\0';
				if ((sklnum = find_skill_num(sklname)) <= 0) {
					sprintf(buf, T_NOSKILL(tptr), GET_NAME(ch));
					do_tell(tutor, buf, cmd_tell, 0);
					release_buffer(sklname);
					return (TRUE);
				}

				skillno = find_skill(sklnum);
				skill_level = tutor_skill_level(tptr, skillno);
				skill_cost = tutor_skill_cost(tptr, skillno);

				if (skill_level == NOTHING || skill_cost == NOTHING) {
					sprintf(buf, T_NOSKILL(tptr), GET_NAME(ch));
					do_tell(tutor, buf, cmd_tell, 0);
					release_buffer(sklname);
					return (TRUE);
				}

				strcpy(argument, sklname);
				release_buffer(sklname);

				if (GET_SKILL(ch, sklnum) > skill_level) {
					sprintf(buf, T_SKILLED(tptr), GET_NAME(ch));
					do_tell(tutor, buf, cmd_tell, 0);
					return (TRUE);
				}

				if (GET_SKILL(ch, SKTREE.pre) <= 0) {
					sprintf(buf, T_NOREQ(tptr), GET_NAME(ch), skill_name(SKTREE.pre));
					do_tell(tutor, buf, cmd_tell, 0);
					return (TRUE);
				}

				if (GET_GOLD(ch) < skill_cost) {
					sprintf(buf, T_NOCASH(tptr), GET_NAME(ch));
					do_tell(tutor, buf, cmd_tell, 0);
					return (TRUE);
				} else {
					sprintf(buf, T_BUYSUCCESS(tptr), GET_NAME(ch), skill_cost);
					do_tell(tutor, buf, cmd_tell, 0);
					GET_GOLD(ch) -= skill_cost;
				}

			}

			act("$n begins to tutor you.", FALSE, tutor, 0, ch, TO_VICT);
			act("$n begins to tutor $N.", TRUE, tutor, 0, ch, TO_NOTVICT);

			CREATE(deo, struct tutor_event_obj, 1);
			deo->student = ch;		/* pointer to student							*/
			deo->tutor = tutor;		/* pointer to tutor								*/
			deo->skill = skillno;	/* pointer to student							*/
			deo->time = 3;				/* event time * PULSE_SKILL				*/
			GET_PLAYER_EVENT(ch, EVENT_SKILL) = event_create(student_delay, deo, PULSE_SKILL);

		} else
			return (FALSE);

	}

	return (TRUE);
}
