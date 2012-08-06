/* ************************************************************************
*   File: tutoredit.c                           An extension to CircleMUD *
*  Usage: Editing spell information                                       *
*                                                                         *
*  This file by Torgny Bjers <artovil@arcanerealms.org>                   *
*  Copyright (C) 2002 by Arcane Realms MUD, www.arcanerealms.org.         *
************************************************************************ */
/* $Id: tutoredit.c,v 1.5 2002/11/06 18:55:47 arcanere Exp $ */

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
#include "tutoredit.h"

/* external globals */
extern struct index_data *mob_index;
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct room_data *world;
extern struct index_data *obj_index;
extern const char *unused_spellname;
extern struct tutor_info_type *tutor_info;
extern int tnum;
extern int newtutor;
extern int top_of_tutort;


void copy_tutor(struct tutor_info_type *ttutor, struct tutor_info_type *ftutor)
{
	/*
	 * Copy basic information over.
	 */
	T_NUM(ttutor) = T_NUM(ftutor);
	T_MOB(ttutor) = T_MOB(ftutor);
	T_FLAGS(ttutor) = T_FLAGS(ftutor);

	/*
	 * Copy lists over.
	 */
	copy_tutor_skills(&(T_SKILLS(ttutor)), T_SKILLS(ftutor));

	/*
	 * Copy notification strings over.
	 */
	/* free_tutor_strings(ttutor); */
	if (T_NOSKILL(ftutor))
		T_NOSKILL(ttutor) = str_dup(T_NOSKILL(ftutor));
	if (T_NOREQ(ftutor))
		T_NOREQ(ttutor) = str_dup(T_NOREQ(ftutor));
	if (T_SKILLED(ftutor))
		T_SKILLED(ttutor) = str_dup(T_SKILLED(ftutor));
	if (T_NOCASH(ftutor))
		T_NOCASH(ttutor) = str_dup(T_NOCASH(ftutor));
	if (T_BUYSUCCESS(ftutor))
		T_BUYSUCCESS(ttutor) = str_dup(T_BUYSUCCESS(ftutor));

	if (T_NEXT(ftutor) != NULL)
		T_NEXT(ttutor) = T_NEXT(ftutor);
}


int tutoredit_save_internally(struct descriptor_data *d)
{
	struct tutor_info_type *tptr;

	if (OLC_TUTOR(d)->number == NOTHING) {
		if ((tptr = enqueue_tutor()) != NULL) {
			OLC_TUTOR(d)->number = newtutor;
			copy_tutor(tptr, OLC_TUTOR(d));
		} else
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "%s:%s(%d) Unable to create new tutor!", __FILE__, __FUNCTION__, __LINE__);
		tnum++;
		if (newtutor == tnum)
			newtutor++;
		else 
			newtutor = tnum;
		return (OLC_TUTOR(d)->number);
	}

	for (tptr = tutor_info; tptr && tptr->number != T_NUM(OLC_TUTOR(d)); tptr=tptr->next);
	if (tptr && (tptr->number == T_NUM(OLC_TUTOR(d)))) {
		copy_tutor(tptr, OLC_TUTOR(d));
		free_tutor(OLC_TUTOR(d));
	}
	return (OLC_TUTOR(d)->number);
}


/*
 * Display skill menu.
 */
void tutoredit_disp_skill_menu(struct descriptor_data *d)
{
	char *help = get_buffer(16384);
	int i, qend;

	get_char_colors(d->character);
	clear_screen(d);

	strcpy(help, "Skill being one of the following:\r\n");
	for (qend = 0, i = 0; i <= TOP_SKILL_DEFINE; i++) {
		if (skill_info[i].name == unused_spellname)	/* This is valid. */
			continue;
		sprintf(help + strlen(help), "%s%3d%s) %-20.20s", grn, i, nrm, skill_info[i].name);
		if (qend++ % 3 == 2) {
			strcat(help, "\r\n");
			write_to_output(d, TRUE, "%s", help);
			*help = '\0';
		}
	}
	if (*help)
		write_to_output(d, TRUE, "%s", help);
	write_to_output(d, TRUE, "\r\n");
	
	release_buffer(help);
}

/*-------------------------------------------------------------------*/

void tutoredit_skills_menu(struct descriptor_data *d)
{
	int i;

	get_char_colors(d->character);

	clear_screen(d);
	write_to_output(d, TRUE, "##     SKNUM   Skill                             Proficiency   Cost/Lesson\r\n");
	for (i = 0; T_SKILL(OLC_TUTOR(d), i) != -1; i++) {
		write_to_output(d, TRUE, "%2d - [%s%5d%s] - %s%-35.35s%s      %s%3d%%   %-4d%s\r\n", i,
			cyn, T_SKILL(OLC_TUTOR(d), i), nrm,
			yel, skill_name(T_SKILL(OLC_TUTOR(d), i)), nrm,
			cyn, T_PROFICIENCY(OLC_TUTOR(d), i) / 100, T_COST(OLC_TUTOR(d), i), nrm);
	}
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new skill.\r\n"
		"%sD%s) Delete a skill.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = TUTOREDIT_SKILLS_MENU;
}

/*-------------------------------------------------------------------*/

void tutoredit_disp_menu(struct descriptor_data * d)
{
	get_char_colors(d->character);
	clear_screen(d);

	sprintbit(OLC_TUTOR(d)->flags, targets, buf1, sizeof(buf1));

	write_to_output(d, TRUE,
		"-- Tutor [%s%d%s]\r\n"
		"%s0%s) Tutor  [%s%5d%s] : %s%s\r\n"
		"%s1%s) Tutor no skill : %s%s\r\n"
		"%s2%s) Player no req  : %s%s\r\n"
		"%s3%s) Player skilled : %s%s\r\n"
		"%s4%s) Player no cash : %s%s\r\n"
		"%s5%s) Buy sucess     : %s%s\r\n"
		"%sS%s) Skills Menu\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ",
			cyn, OLC_TUTOR(d)->number, nrm,
			grn, nrm, cyn, OLC_TUTOR(d)->vnum, nrm,
			yel, (real_mobile(OLC_TUTOR(d)->vnum) != NOTHING) ? mob_proto[real_mobile(OLC_TUTOR(d)->vnum)].player.short_descr : "Nobody",
			grn, nrm, yel, OLC_TUTOR(d)->no_skill ? OLC_TUTOR(d)->no_skill : "<Not set>",
			grn, nrm, yel, OLC_TUTOR(d)->no_req ? OLC_TUTOR(d)->no_req : "<Not set>",
			grn, nrm, yel, OLC_TUTOR(d)->skilled ? OLC_TUTOR(d)->skilled : "<Not set>",
			grn, nrm, yel, OLC_TUTOR(d)->no_cash ? OLC_TUTOR(d)->no_cash : "<Not set>",
			grn, nrm, yel, OLC_TUTOR(d)->buy_success ? OLC_TUTOR(d)->buy_success : "<Not set>",
			grn, nrm,
			grn, nrm
	);

	OLC_MODE(d) = TUTOREDIT_MAIN_MENU;
}


void tutoredit_parse(struct descriptor_data *d, char *arg)
{
	int i = -1, newtnum = NOTHING;
	struct tutor_skill_data new_entry;

	if (OLC_MODE(d) > TUTOREDIT_NUMERICAL_RESPONSE) {
		i = atoi(arg);
		if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && !isdigit(arg[1])))) {
			write_to_output(d, TRUE, "Field must be numerical, try again : ");
			return;
		}
	} else {	/* String response. */
		if (!genolc_checkstring(d, arg))
			return;
	}

	switch (OLC_MODE(d)) {
	case TUTOREDIT_CONFIRM_SAVE:
		switch (*arg) {
		case 'y':
		case 'Y':
			newtnum = tutoredit_save_internally(d);
			save_tutors();
			extended_mudlog(BRF, SYSL_OLC, TRUE, "%s edits tutor %d", GET_NAME(d->character), newtnum);
			write_to_output(d, TRUE, "Tutor saved to database.\r\n");
			cleanup_olc(d, CLEANUP_STRUCTS);
			break;
		case 'n':
		case 'N':
			/* free everything up, including strings etc */
			cleanup_olc(d, CLEANUP_ALL);
			break;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\n");
			write_to_output(d, TRUE, "Do you wish to save this tutor? : ");
			break;
		}
		return;
		
	case TUTOREDIT_MAIN_MENU:
		i = 0;
		switch (*arg) {
		case 'q':
		case 'Q':
			if (OLC_VAL(d)) {
				/*. Something has been modified .*/
				write_to_output(d, TRUE, "Do you wish to save this tutor? : ");
				OLC_MODE(d) = TUTOREDIT_CONFIRM_SAVE;
			} else
				cleanup_olc(d, CLEANUP_STRUCTS);
			return;
		case '0':
			OLC_MODE(d) = TUTOREDIT_TUTOR;
			i++;
			break;
		case '1':
			OLC_MODE(d) = TUTOREDIT_NOSKILL;
			i--;
			break;
		case '2':
			OLC_MODE(d) = TUTOREDIT_NOREQ;
			i--;
			break;
		case '3':
			OLC_MODE(d) = TUTOREDIT_SKILLED;
			i--;
			break;
		case '4':
			OLC_MODE(d) = TUTOREDIT_NOCASH;
			i--;
			break;
		case '5':
			OLC_MODE(d) = TUTOREDIT_BUYSUCCESS;
			i--;
			break;
		case 's':
		case 'S':
			tutoredit_skills_menu(d);
			return;
		default:
			tutoredit_disp_menu(d);
			return;
		}

		if (i == 0)
			break;
		else if (i == 1)
			write_to_output(d, TRUE, "\r\nEnter new value : ");
		else if (i == -1)
			write_to_output(d, TRUE, "\r\nEnter new text :\r\n] ");
		else
			write_to_output(d, TRUE, "Oops...\r\n");
		return;
/*-------------------------------------------------------------------*/
	case TUTOREDIT_TUTOR:
		if (real_mobile(i) == NOTHING) {
			write_to_output(d, TRUE, "\r\nThat mobile does not exist.\r\nEnter new value : ");
			return;
		} else
			OLC_TUTOR(d)->vnum = i;
		break;
/*-------------------------------------------------------------------*/
	case TUTOREDIT_NOSKILL:
		if (genolc_checkstring(d, arg))
			modify_string(&T_NOSKILL(OLC_TUTOR(d)), arg);
		break;
/*-------------------------------------------------------------------*/
	case TUTOREDIT_NOREQ:
		if (genolc_checkstring(d, arg))
			modify_string(&T_NOREQ(OLC_TUTOR(d)), arg);
		break;
/*-------------------------------------------------------------------*/
	case TUTOREDIT_SKILLED:
		if (genolc_checkstring(d, arg))
			modify_string(&T_SKILLED(OLC_TUTOR(d)), arg);
		break;
/*-------------------------------------------------------------------*/
	case TUTOREDIT_NOCASH:
		if (genolc_checkstring(d, arg))
			modify_string(&T_NOCASH(OLC_TUTOR(d)), arg);
		break;
/*-------------------------------------------------------------------*/
	case TUTOREDIT_BUYSUCCESS:
		if (genolc_checkstring(d, arg))
			modify_string(&T_BUYSUCCESS(OLC_TUTOR(d)), arg);
		break;
/*-------------------------------------------------------------------*/
	case TUTOREDIT_SKILLS_MENU:
		switch (*arg) {
		case 'a':
		case 'A':
			tutoredit_disp_skill_menu(d);
			write_to_output(d, TRUE, "\r\nEnter new skill number : ");
			OLC_MODE(d) = TUTOREDIT_NEW_SKILL;
			return;
		case 'd':
		case 'D':
			write_to_output(d, TRUE, "\r\nDelete which skill? : ");
			OLC_MODE(d) = TUTOREDIT_DELETE_SKILL;
			return;
		case 'q':
		case 'Q':
			break;
		default:
			write_to_output(d, TRUE, "\r\n");
			OLC_MODE(d) = TUTOREDIT_SKILLS_MENU;
			tutoredit_skills_menu(d);
			return;
		}
		break;

	case TUTOREDIT_NEW_SKILL:
		if ((find_skill(i)) == -1) {
			write_to_output(d, TRUE, "That skill does not exist, try again : ");
			return;
		}
		if (i > 0)
			OLC_VAL(d) = i;
		else
			OLC_VAL(d) = -1;
		write_to_output(d, TRUE, "\r\nTo what proficiency should the tutor know this skill (0-100%)? : ");
		OLC_MODE(d) = TUTOREDIT_SKILL_PROFICIENCY;
		return;

	case TUTOREDIT_SKILL_PROFICIENCY:
		OLC_VAL2(d) = 100 * LIMIT(i, 0, 100);
		write_to_output(d, TRUE, "\r\nHow many coins should the skill cost to learn (0-250)? : ");
		OLC_MODE(d) = TUTOREDIT_SKILL_COST;
		return;

	case TUTOREDIT_SKILL_COST:
		TUTOR_SKL_SKILL(new_entry) = OLC_VAL(d);
		TUTOR_SKL_PROFICIENCY(new_entry) = OLC_VAL2(d);
		TUTOR_SKL_COST(new_entry) = LIMIT(i, 0, 250);
		add_to_tutor_skills(&(T_SKILLS(OLC_TUTOR(d))), &new_entry);
		write_to_output(d, TRUE, "\r\n");
		tutoredit_skills_menu(d);
		return;

	case TUTOREDIT_DELETE_SKILL:
		remove_from_tutor_skills(&(T_SKILLS(OLC_TUTOR(d))), i);
		tutoredit_skills_menu(d);
		return;
/*-------------------------------------------------------------------*/
	default:
		cleanup_olc(d, CLEANUP_ALL);
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: tutoredit_parse(): Reached default case!");
		write_to_output(d, TRUE, "Oops...\r\n");
		break;
	}
/*-------------------------------------------------------------------*/

/*
 * END OF CASE 
 * If we get here, we have probably changed something, and now want to
 * return to main menu.  Use OLC_VAL as a 'has changed' flag  
 */

	OLC_VAL(d) = TRUE;
	tutoredit_disp_menu(d);
}

/*
* Create a new tutor with some default strings.
*/
void tutoredit_setup_new(struct descriptor_data *d)
{
	CREATE(OLC_TUTOR(d), struct tutor_info_type, 1);
	/* Set up the basic information */
	OLC_TUTOR(d)->number = NOTHING;
	OLC_TUTOR(d)->vnum = NOTHING;
	OLC_TUTOR(d)->no_skill = str_dup("%s Sorry, I do not know that skill.");
	OLC_TUTOR(d)->no_req = str_dup("%s Sorry, you have to know %s first.");
	OLC_TUTOR(d)->skilled = str_dup("%s Sorry, I cannot teach you further in that skill.");
	OLC_TUTOR(d)->no_cash = str_dup("%s You are too poor!");
	OLC_TUTOR(d)->buy_success = str_dup("%s That'll be %d coins, thanks.");
	OLC_TUTOR(d)->flags = 0;
	/* Create a skill structure */	
	CREATE(T_SKILLS(OLC_TUTOR(d)), struct tutor_skill_data, 1);
	T_SKILL(OLC_TUTOR(d), 0) = NOTHING;
	T_PROFICIENCY(OLC_TUTOR(d), 0) = 0;
	T_COST(OLC_TUTOR(d), 0) = 0;
	tutoredit_disp_menu(d);
	OLC_VAL(d) = 0;
}


void tutoredit_setup_existing(struct descriptor_data *d, struct tutor_info_type *t)
{
	if (t == NULL) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "%s:%s(%d) Unable to set up existing tutor!", __FILE__, __FUNCTION__, __LINE__);
	}

	CREATE(OLC_TUTOR(d), struct tutor_info_type, 1);
	copy_tutor(OLC_TUTOR(d), t);
	tutoredit_disp_menu(d);
	OLC_VAL(d) = 0;
}
