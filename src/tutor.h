/* ************************************************************************
*   File: tutor.h                               An extension to CircleMUD *
*  Usage: Header file with definitions for tutor system                   *
*                                                                         *
*  This file by Torgny Bjers <artovil@arcanerealms.org>                   *
*  Copyright (C) 2002 by Arcane Realms MUD, www.arcanerealms.org.         *
************************************************************************ */
/* $Id: tutor.h,v 1.3 2002/12/29 11:34:38 arcanere Exp $ */

#ifndef	_TUTOR_H_
#define	_TUTOR_H_

#define MAX_LIST_SKILLS						20

#define	T_SKILLS(i)								((i)->skills)

#define TUTOR_SKL_SKILL(i)				((i).skill)
#define TUTOR_SKL_PROFICIENCY(i)	((i).proficiency)
#define TUTOR_SKL_COST(i)					((i).cost)

#define	T_SKILL(i, num)						(TUTOR_SKL_SKILL((i)->skills[(num)]))
#define	T_PROFICIENCY(i, num)			(TUTOR_SKL_PROFICIENCY((i)->skills[(num)]))
#define	T_COST(i, num)						(TUTOR_SKL_COST((i)->skills[(num)]))

#define	T_NUM(i)									((i)->number)
#define	T_MOB(i)									((i)->vnum)
#define	T_NOSKILL(i)							((i)->no_skill)
#define	T_NOREQ(i)								((i)->no_req)
#define	T_SKILLED(i)							((i)->skilled)
#define	T_NOCASH(i)								((i)->no_cash)
#define	T_BUYSUCCESS(i)						((i)->buy_success)
#define	T_FLAGS(i)								((i)->flags)
#define	T_NEXT(i)									((i)->next)

struct tutor_info_type {
	sh_int number;
	mob_vnum vnum;
	char *no_skill;
	char *no_req;
	char *skilled;
	char *no_cash;
	char *buy_success;
	struct tutor_skill_data *skills;
	bitvector_t flags;
	struct tutor_info_type *next;
};

struct tutor_skill_data {
	int skill;
	ush_int proficiency;
	ush_int cost;
};

/* event object structure tutor events */
struct tutor_event_obj {
	struct char_data *student;	/* character that uses the skill			*/
	struct char_data *tutor;		/* victim character										*/
	int time;										/* number of pulses to queue					*/
	ush_int skill;							/* skill to process										*/
};

void assign_the_tutors(void);
void copy_tutor_skills(struct tutor_skill_data **tlist, struct tutor_skill_data *flist);
struct tutor_info_type *get_tutor(int tutornum, const char *file, const char *function);
int	find_tutor_num(mob_vnum mvnum);
void sort_tutors(void);
void free_tutor(struct tutor_info_type *t);
void free_tutor_strings(struct tutor_info_type *t);
void remove_from_tutor_skills(struct tutor_skill_data **list, int num);
void add_to_tutor_skills(struct tutor_skill_data **list, struct tutor_skill_data *newl);
struct tutor_info_type *enqueue_tutor(void);
void dequeue_tutor(int tutornum);
void load_tutors(void);
void save_tutors(void);
SPECIAL(tutor);

#endif
