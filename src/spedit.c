/* ************************************************************************
*   File: spedit.c                              An extension to CircleMUD *
*  Usage: Editing spell information                                       *
*                                                                         *
*  This file by Torgny Bjers <artovil@arcanerealms.org>                   *
*  Copyright (C) 2002 by Arcane Realms MUD, www.arcanerealms.org.         *
************************************************************************ */
/* $Id: spedit.c,v 1.12 2002/11/06 18:55:47 arcanere Exp $ */

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
#include "spedit.h"
#include "loadrooms.h"

/* external globals */
extern const sh_int mortal_start_room[NUM_STARTROOMS +1];
extern struct index_data *mob_index;
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct room_data *world;
extern struct index_data *obj_index;
extern int spnum;
extern int newspell;
extern const char *unused_spellname;

/*-------------------------------------------------------------------*/

/*
 * Display positions. (sitting, standing, etc)
 */
void spedit_disp_positions(struct descriptor_data *d)
{
	int i;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; *position_types[i] != '\n'; i++)
		write_to_output(d, TRUE, "%s%2d%s) %s\r\n", grn, i, nrm, position_types[i]);
	OLC_MODE(d) = SPEDIT_MIN_POSITION;
	write_to_output(d, TRUE, "Enter position number : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display routine flags menu.
 */
void spedit_disp_routine_flags(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (i = 0; i < NUM_ROUTINES; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, magic_routines[i],
								!(++columns % 2) ? "\r\n" : "");
	sprintbit(OLC_SPELL(d)->routines, magic_routines, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nCurrent flags : %s%s%s\r\nEnter routine flags (0 to quit) : ",
									cyn, buf1, nrm);
}

/*-------------------------------------------------------------------*/

/*
 * Display routine flags menu.
 */
void spedit_disp_target_flags(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (i = 0; i < NUM_TARGETS; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, targets[i],
								!(++columns % 2) ? "\r\n" : "");
	sprintbit(OLC_SPELL(d)->targets, targets, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nCurrent flags : %s%s%s\r\nEnter target flags (0 to quit) : ",
									cyn, buf1, nrm);
}

/*-------------------------------------------------------------------*/

/*
 * Display skill menu.
 */
void spedit_disp_skill_menu(struct descriptor_data *d)
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

void spedit_reagents_menu(struct descriptor_data *d)
{
	int i;

	get_char_colors(d->character);

	clear_screen(d);
	write_to_output(d, TRUE, "##     VNUM    Reagent                               In room?  Extract?\r\n");
	for (i = 0; S_REG_TYPE(OLC_SPELL(d), i) != -1; i++)
		write_to_output(d, TRUE, "%2d - [%s%5d%s] - %s%-35.35s%s   %s%-3s       %-3s%s\r\n", i,
			cyn, obj_index[real_object(S_REG_TYPE(OLC_SPELL(d), i))].vnum, nrm,
			yel, obj_proto[real_object(S_REG_TYPE(OLC_SPELL(d), i))].short_description, nrm,
			cyn, YESNO(S_REG_LOCATION(OLC_SPELL(d), i)), YESNO(S_REG_EXTRACT(OLC_SPELL(d), i)), nrm);
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new reagent.\r\n"
		"%sD%s) Delete a reagent.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = SPEDIT_REAGENTS_MENU;
}

/*-------------------------------------------------------------------*/

void spedit_focuses_menu(struct descriptor_data *d)
{
	int i;

	get_char_colors(d->character);

	clear_screen(d);
	write_to_output(d, TRUE, "##     VNUM    Focus                                 In room?  Extract?\r\n");
	for (i = 0; S_FOC_TYPE(OLC_SPELL(d), i) != -1; i++)
		write_to_output(d, TRUE, "%2d - [%s%5d%s] - %s%-35.35s%s   %s%-3s       %-3s%s\r\n", i,
			cyn, obj_index[real_object(S_FOC_TYPE(OLC_SPELL(d), i))].vnum, nrm,
			yel, obj_proto[real_object(S_FOC_TYPE(OLC_SPELL(d), i))].short_description, nrm,
			cyn, YESNO(S_FOC_LOCATION(OLC_SPELL(d), i)), YESNO(S_FOC_EXTRACT(OLC_SPELL(d), i)), nrm);
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new focus.\r\n"
		"%sD%s) Delete a focus.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = SPEDIT_FOCUSES_MENU;
}

/*-------------------------------------------------------------------*/

void spedit_transmuters_menu(struct descriptor_data *d)
{
	int i;

	get_char_colors(d->character);

	clear_screen(d);
	write_to_output(d, TRUE, "##     VNUM    Transmuter                            In room?  Extract?\r\n");
	for (i = 0; S_TRA_TYPE(OLC_SPELL(d), i) != -1; i++)
		write_to_output(d, TRUE, "%2d - [%s%5d%s] - %s%-35.35s%s   %s%-3s       %-3s%s\r\n", i,
			cyn, obj_index[real_object(S_TRA_TYPE(OLC_SPELL(d), i))].vnum, nrm,
			yel, obj_proto[real_object(S_TRA_TYPE(OLC_SPELL(d), i))].short_description, nrm,
			cyn, YESNO(S_TRA_LOCATION(OLC_SPELL(d), i)), YESNO(S_TRA_EXTRACT(OLC_SPELL(d), i)), nrm);
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new transmuter.\r\n"
		"%sD%s) Delete a transmuter.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = SPEDIT_TRANSMUTERS_MENU;
}

/*-------------------------------------------------------------------*/

/*
 * Display technique.
 */
void spedit_disp_spell_data(struct descriptor_data *d, const char *array[][3], int mode, const char *prompt)
{
	int i;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; *array[i][0] != '\n'; i++)
		write_to_output(d, TRUE, "%s%2d%s) %s (%s) \"%s\"\r\n", grn, i+1, nrm, array[i][0], array[i][1], array[i][2]);
	OLC_MODE(d) = mode;
	write_to_output(d, TRUE, "%s", prompt);
}

/*-------------------------------------------------------------------*/

void spedit_disp_menu(struct descriptor_data * d)
{
	char *skillname;
	if (!OLC_SPELL(d))
		spedit_setup_new(d);

	skillname = str_dup(skill_name((int)OLC_SPELL(d)->skill));

	sprintbit(OLC_SPELL(d)->targets, targets, buf1, sizeof(buf1));
	sprintbit(OLC_SPELL(d)->routines, magic_routines, buf2, sizeof(buf2));
	
	get_char_colors(d->character);

	write_to_output(d, TRUE,
		"-- Spell number : [%s%d%s]\r\n"
		"%s1%s) Name          : %s%s\r\n"
		"%s2%s) Long name     : %s%s\r\n"
		"%s3%s) Victim msg.   : %s%s\r\n"
		"%s4%s) Wear off msg. : %s%s\r\n"
		"%s5%s) Cast msg.     :\r\n%s%s"
		"%s6%s) Mundane msg.  :\r\n%s%s"
		"%s7%s) Target msg.   :\r\n%s%s"
		"%s8%s) Mana Max      : %s%-3d          %s9%s) Mana Min     : %s%-3d\r\n"
		"%sA%s) Min. Position : %s%-10s   %sB%s) Master Skill : %s%s\r\n"
		"%sC%s) Technique     : %s%s (%s) \"%s\"\r\n"
		"%sD%s) Form          : %s%s (%s) \"%s\"\r\n"
		"%sE%s) Range         : %s%s (%s) \"%s\"\r\n"
		"%sF%s) Duration      : %s%s (%s) \"%s\"\r\n"
		"%sG%s) Target Group  : %s%s (%s) \"%s\"\r\n"
		"%sH%s) Learned       : %s%-3s          %sI%s) Violent      : %s%s\r\n"
		"%sJ%s) Targets       : %s%s\r\n"
		"%sK%s) Routines      : %s%s\r\n"
		"%sL%s) Reagents\r\n"
		"%sM%s) Focuses\r\n"
		"%sN%s) Transmuters\r\n"
		"%sP%s) Purge this Spell\r\n"
		"%sO%s) Values Menu\r\n" /* added by Zaquenin for removal of spell constants*/
		"%sQ%s) Quit\r\n"
		"Enter choice : ",
		
		cyn, OLC_SPELL(d)->number, nrm,
		grn, nrm, yel, OLC_SPELL(d)->name,
		grn, nrm, yel, OLC_SPELL(d)->long_name ? OLC_SPELL(d)->long_name : "<None>",
		grn, nrm, yel, OLC_SPELL(d)->victim_msg ? OLC_SPELL(d)->victim_msg : "<None>",
		grn, nrm, yel, OLC_SPELL(d)->wear_off_msg ? OLC_SPELL(d)->wear_off_msg : "<None>",
		grn, nrm, grn, OLC_SPELL(d)->cast_msg,
		grn, nrm, grn, OLC_SPELL(d)->mundane_msg,
		grn, nrm, grn, OLC_SPELL(d)->target_msg,
		grn, nrm, cyn, OLC_SPELL(d)->mana_max,
		grn, nrm, cyn, OLC_SPELL(d)->mana_min,
		grn, nrm, yel, position_types[(int)OLC_SPELL(d)->min_position],
		grn, nrm, yel, CAP(skillname),
		grn, nrm, yel, spell_techniques[(int)OLC_SPELL(d)->technique][0], spell_techniques[(int)OLC_SPELL(d)->technique][1], spell_techniques[(int)OLC_SPELL(d)->technique][2],
		grn, nrm, yel, spell_forms[(int)OLC_SPELL(d)->form][0], spell_forms[(int)OLC_SPELL(d)->form][1], spell_forms[(int)OLC_SPELL(d)->form][2],
		grn, nrm, yel, spell_ranges[(int)OLC_SPELL(d)->range][0], spell_ranges[(int)OLC_SPELL(d)->range][1], spell_ranges[(int)OLC_SPELL(d)->range][2],
		grn, nrm, yel, spell_duration[(int)OLC_SPELL(d)->duration][0], spell_duration[(int)OLC_SPELL(d)->duration][1], spell_duration[(int)OLC_SPELL(d)->duration][2],
		grn, nrm, yel, spell_target_groups[(int)OLC_SPELL(d)->target_group][0], spell_target_groups[(int)OLC_SPELL(d)->target_group][1], spell_target_groups[(int)OLC_SPELL(d)->target_group][2],
		grn, nrm, cyn, YESNO(OLC_SPELL(d)->learned),
		grn, nrm, cyn, YESNO(OLC_SPELL(d)->violent),
		grn, nrm, cyn, buf1,
		grn, nrm, cyn, buf2,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm
	);

	release_buffer(skillname);

	OLC_MODE(d) = SPEDIT_MAIN_MENU;
}


void spedit_free_spell(struct spell_info_type *sptr)
{
	dequeue_spell(sptr->number);
}


void spedit_parse(struct descriptor_data *d, char *arg)
{
	int i = -1;
	struct spell_obj_data new_entry;

	if (OLC_MODE(d) > SPEDIT_NUMERICAL_RESPONSE) {
		i = atoi(arg);
		if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && !isdigit(arg[1])))) {
			write_to_output(d, TRUE, "Field must be numerical, try again : ");
			return;
		}
	} else {        /* String response. */
		if (!genolc_checkstring(d, arg))
			return;
	}
	switch (OLC_MODE(d)) {
	case SPEDIT_CONFIRM_SAVE:
		switch (*arg) {
		case 'y':
		case 'Y':
			save_spells();
			extended_mudlog(BRF, SYSL_OLC, TRUE, "%s edits spell %d", GET_NAME(d->character), OLC_SPELL(d)->number);
			write_to_output(d, TRUE, "Spell saved to disk and memory.\r\n");
			cleanup_olc(d, CLEANUP_STRUCTS);
			break;
		case 'n':
		case 'N':
			/* free everything up, including strings etc */
			cleanup_olc(d, CLEANUP_STRUCTS);
			break;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\n");
			write_to_output(d, TRUE, "Do you wish to save this spell? : ");
			break;
		}
		return;
		
	case SPEDIT_MAIN_MENU:
		i = 0;
		switch (*arg) {
		case 'q':
		case 'Q':
			if (OLC_VAL(d)) {
				/*. Something has been modified .*/
				write_to_output(d, TRUE, "Do you wish to save this spell? : ");
				OLC_MODE(d) = SPEDIT_CONFIRM_SAVE;
			} else
				cleanup_olc(d, CLEANUP_STRUCTS);
			return;
		case '1':
			i--;
			write_to_output(d, TRUE, "Enter spell name:-\r\n| ");
			OLC_MODE(d) = SPEDIT_NAME;
			break;
		case '2':
			i--;
			write_to_output(d, TRUE, "Enter Verbose Spell Name:-\r\n| ");
			OLC_MODE(d) = SPEDIT_LONG_NAME;
			break;
		case '3':
			i--;
			write_to_output(d, TRUE, "Enter spell victim message:-\r\n| ");
			OLC_MODE(d) = SPEDIT_VICTIM_MSG;
			break;
		case '4':
			i--;
			write_to_output(d, TRUE, "Enter spell wear off message:-\r\n| ");
			OLC_MODE(d) = SPEDIT_WEAR_OFF_MSG;
			break;
		case '5':
			i--;
			OLC_MODE(d) = SPEDIT_CAST_MSG;
			clear_screen(d);
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_SPELL(d)->cast_msg)
				write_to_output(d, FALSE, "%s", OLC_SPELL(d)->cast_msg);
			string_write(d, &OLC_SPELL(d)->cast_msg, MAX_ROOM_DESC, 0, STATE(d));
			OLC_VAL(d) = TRUE;
			break;
		case '6':
			i--;
			OLC_MODE(d) = SPEDIT_MUNDANE_MSG;
			clear_screen(d);
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_SPELL(d)->mundane_msg)
				write_to_output(d, FALSE, "%s", OLC_SPELL(d)->mundane_msg);
			string_write(d, &OLC_SPELL(d)->mundane_msg, MAX_ROOM_DESC, 0, STATE(d));
			OLC_VAL(d) = TRUE;
			break;
		case '7':
			i--;
			OLC_MODE(d) = SPEDIT_TARGET_MSG;
			clear_screen(d);
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_SPELL(d)->target_msg)
				write_to_output(d, FALSE, "%s", OLC_SPELL(d)->target_msg);
			string_write(d, &OLC_SPELL(d)->target_msg, MAX_ROOM_DESC, 0, STATE(d));
			OLC_VAL(d) = TRUE;
			break;
		case '8':
			i++;
			write_to_output(d, TRUE, "Enter Mana Max: ");
			OLC_MODE(d) = SPEDIT_MANA_MAX;
			break;
		case '9':
			i++;
			write_to_output(d, TRUE, "Enter Mana Min: ");
			OLC_MODE(d) = SPEDIT_MANA_MIN;
			break;
		case 'A':
		case 'a':
			i++;
			spedit_disp_positions(d);
			return;
		case 'B':
		case 'b':
			i++;
			OLC_MODE(d) = SPEDIT_MASTER_SKILL;
			spedit_disp_skill_menu(d);				
			write_to_output(d, TRUE, "Enter the skill number (-1 for none) : ");
			break;
		case 'C':
		case 'c':
			i++;
			spedit_disp_spell_data(d, spell_techniques, SPEDIT_TECHNIQUE, "Enter technique : ");
			return;
		case 'D':
		case 'd':
			i++;
			spedit_disp_spell_data(d, spell_forms, SPEDIT_FORM, "Enter form : ");
			return;
		case 'E':
		case 'e':
			i++;
			spedit_disp_spell_data(d, spell_ranges, SPEDIT_RANGE, "Enter range : ");
			return;
		case 'F':
		case 'f':
			i++;
			spedit_disp_spell_data(d, spell_duration, SPEDIT_DURATION, "Enter duration : ");
			return;
		case 'G':
		case 'g':
			i++;
			spedit_disp_spell_data(d, spell_target_groups, SPEDIT_TARGET_GROUP, "Enter target group : ");
			return;
		case 'H':
		case 'h':
			i--;
			write_to_output(d, TRUE, "Can the spell be learned? (Yes/No): ");
			OLC_MODE(d) = SPEDIT_LEARNED;
			break;
		case 'I':
		case 'i':
			write_to_output(d, TRUE, "Is the spell violent? (Yes/No): ");
			OLC_MODE(d) = SPEDIT_VIOLENT;
			break;
		case 'J':
		case 'j':
			OLC_MODE(d) = SPEDIT_TARGETS;
			spedit_disp_target_flags(d);
			return;
		case 'K':
		case 'k':
			OLC_MODE(d) = SPEDIT_ROUTINES;
			spedit_disp_routine_flags(d);
			return;
		case 'L':
		case 'l':
			spedit_reagents_menu(d);
			return;
		case 'M':
		case 'm':
			spedit_focuses_menu(d);
			return;
		case 'N':
		case 'n':
			spedit_transmuters_menu(d);
			return;
		case 'P':
		case 'p':
			if (IS_IMPLEMENTOR(d->character)) {
				newspell = OLC_SPELL(d)->number;	/* next new spell will get this one's number */
				/* free everything up, including strings etc */
				cleanup_olc(d, CLEANUP_ALL);
				spnum--;
				write_to_output(d, TRUE, "Spell purged.\r\n");
			} else {
				write_to_output(d, TRUE, "Sorry you are not allowed to do that at this time.\r\n");
				spedit_disp_menu(d);
			}
			return;
		case 'O': /* bring up the values menu */
		case 'o':
			write_to_output(d, TRUE, "Not complete yet.\r\n");
			break;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\n");
			spedit_disp_menu(d);
			break;
		}
		return;
/*-------------------------------------------------------------------*/
	case SPEDIT_NAME:
		if (OLC_SPELL(d)->name)
			free(OLC_SPELL(d)->name);
		OLC_SPELL(d)->name = str_dup(arg);
		break;				
/*-------------------------------------------------------------------*/
	case SPEDIT_LONG_NAME:
		if (OLC_SPELL(d)->long_name)
			free(OLC_SPELL(d)->long_name);
		if (*arg) {
			delete_doubledollar(arg);
			OLC_SPELL(d)->long_name = str_dup(arg);
		} else
			OLC_SPELL(d)->long_name = '\0';
		break;				
/*-------------------------------------------------------------------*/
	case SPEDIT_VICTIM_MSG:
		if (OLC_SPELL(d)->victim_msg)
			free(OLC_SPELL(d)->victim_msg);
		if (*arg) {
			delete_doubledollar(arg);
			OLC_SPELL(d)->victim_msg = str_dup(arg);
		} else
			OLC_SPELL(d)->victim_msg = '\0';
		break;				
/*-------------------------------------------------------------------*/
	case SPEDIT_WEAR_OFF_MSG:
		if (OLC_SPELL(d)->wear_off_msg)
			free(OLC_SPELL(d)->wear_off_msg);
		if (*arg) {
			delete_doubledollar(arg);
			OLC_SPELL(d)->wear_off_msg = str_dup(arg);
		} else
			OLC_SPELL(d)->wear_off_msg = '\0';
		break;				
/*-------------------------------------------------------------------*/
	case SPEDIT_CAST_MSG:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached SPEDIT_CAST_MSG case in parse_spedit().");
		write_to_output(d, TRUE, "Oops, in SPEDIT_CAST_MSG.\r\n");
		break;				
/*-------------------------------------------------------------------*/
	case SPEDIT_MUNDANE_MSG:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached SPEDIT_MUNDANE_MSG case in parse_spedit().");
		write_to_output(d, TRUE, "Oops, in SPEDIT_MUNDANE_MSG.\r\n");
		break;				
/*-------------------------------------------------------------------*/
	case SPEDIT_TARGET_MSG:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached SPEDIT_TARGET_MSG case in parse_spedit().");
		write_to_output(d, TRUE, "Oops, in SPEDIT_TARGET_MSG.\r\n");
		break;				
/*-------------------------------------------------------------------*/
	case SPEDIT_MANA_MIN:
		OLC_SPELL(d)->mana_min = LIMIT(i, 0, 500);
		break;
/*-------------------------------------------------------------------*/
	case SPEDIT_MANA_MAX:
		OLC_SPELL(d)->mana_max = LIMIT(i, 0, 1000);
		break;
/*-------------------------------------------------------------------*/
	case SPEDIT_MIN_POSITION:
		OLC_SPELL(d)->min_position = LIMIT(i, 0, NUM_POSITIONS - 1);
		break;
/*-------------------------------------------------------------------*/
	case SPEDIT_MASTER_SKILL:
		OLC_SPELL(d)->skill = LIMIT(i, -1, TOP_SKILL_DEFINE);
		break;
/*-------------------------------------------------------------------*/
	case SPEDIT_TECHNIQUE:
		i--;
		OLC_SPELL(d)->technique = LIMIT(i, 0, NUM_SPELL_TECHNIQUES - 1);
		break;
/*-------------------------------------------------------------------*/
	case SPEDIT_FORM:
		i--;
		OLC_SPELL(d)->form = LIMIT(i, 0, NUM_SPELL_FORMS - 1);
		break;
/*-------------------------------------------------------------------*/
	case SPEDIT_RANGE:
		i--;
		OLC_SPELL(d)->range = LIMIT(i, 0, NUM_SPELL_RANGES- 1);
		break;
/*-------------------------------------------------------------------*/
	case SPEDIT_DURATION:
		i--;
		OLC_SPELL(d)->duration = LIMIT(i, 0, NUM_SPELL_DURATIONS - 1);
		break;
/*-------------------------------------------------------------------*/
	case SPEDIT_TARGET_GROUP:
		i--;
		OLC_SPELL(d)->target_group = LIMIT(i, 0, NUM_SPELL_TARGET_GROUPS - 1);
		break;
/*-------------------------------------------------------------------*/
	case SPEDIT_TARGETS:
		if ((i = atoi(arg)) <= 0)
			break;
		else if (i <= NUM_TARGETS)
			TOGGLE_BIT(OLC_SPELL(d)->targets, 1 << (i - 1));
			spedit_disp_target_flags(d);
		return;
/*-------------------------------------------------------------------*/
	case SPEDIT_VIOLENT:
		switch (*arg) {
		case 'Y':
		case 'y':
			OLC_SPELL(d)->violent = 1;
			break;
		default:
			OLC_SPELL(d)->violent = 0;
			break;
		}
		break;
/*-------------------------------------------------------------------*/
	case SPEDIT_ROUTINES:
		if ((i = atoi(arg)) <= 0)
			break;
		else if (i <= NUM_ROUTINES)
			TOGGLE_BIT(OLC_SPELL(d)->routines, 1 << (i - 1));
			spedit_disp_routine_flags(d);
		return;
/*-------------------------------------------------------------------*/
	case SPEDIT_LEARNED:
		switch (*arg) {
		case 'Y':
		case 'y':
			OLC_SPELL(d)->learned = 1;
			break;
		default:
			OLC_SPELL(d)->learned = 0;
			break;
		}
		break;
/*-------------------------------------------------------------------*/
	case SPEDIT_REAGENTS_MENU:
		switch (*arg) {
		case 'a':
		case 'A':
			write_to_output(d, TRUE, "\r\nEnter new reagent vnum number : ");
			OLC_MODE(d) = SPEDIT_NEW_REAGENT;
			return;
		case 'd':
		case 'D':
			write_to_output(d, TRUE, "\r\nDelete which reagent? : ");
			OLC_MODE(d) = SPEDIT_DELETE_REAGENT;
			return;
		case 'q':
		case 'Q':
			break;
		default:
			write_to_output(d, TRUE, "\r\n");
			OLC_MODE(d) = SPEDIT_DELETE_REAGENT;
			spedit_reagents_menu(d);
			return;
		}
		break;

	case SPEDIT_NEW_REAGENT:
		if ((i = atoi(arg)) != -1)
			if ((i = real_object(i)) == -1) {
				write_to_output(d, TRUE, "That object does not exist, try again : ");
				return;
			}
		if (i > 0)
			OLC_VAL(d) = atoi(arg);
		else
			OLC_VAL(d) = -1;
		write_to_output(d, TRUE, "\r\nShould the item be in the room? : ");
		OLC_MODE(d) = SPEDIT_REAGENT_LOCATION;
		return;

	case SPEDIT_REAGENT_LOCATION:
		switch(*arg) {
		case 'Y':
		case 'y':
			OLC_VAL2(d) = 1;
			break;
		case 'N':
		case 'n':
			OLC_VAL2(d) = 0;
			break;
		default:
			write_to_output(d, TRUE, "Please answer Yes or No : ");
			return;
		}
		write_to_output(d, TRUE, "\r\nShould the item be spent upon usage? : ");
		OLC_MODE(d) = SPEDIT_REAGENT_EXTRACT;
		return;

	case SPEDIT_REAGENT_EXTRACT:
		SPELL_OBJ_TYPE(new_entry) = OLC_VAL(d);
		SPELL_OBJ_LOCATION(new_entry) = OLC_VAL2(d);
		switch(*arg) {
		case 'Y':
		case 'y':
			SPELL_OBJ_EXTRACT(new_entry) = 1;
			break;
		case 'N':
		case 'n':
			SPELL_OBJ_EXTRACT(new_entry) = 0;
			break;
		default:
			write_to_output(d, TRUE, "Please answer Yes or No : ");
			return;
		}
		add_to_obj_list(&(S_REAGENTS(OLC_SPELL(d))), &new_entry);
		write_to_output(d, TRUE, "\r\n");
		spedit_reagents_menu(d);
		return;

	case SPEDIT_DELETE_REAGENT:
		remove_from_obj_list(&(S_REAGENTS(OLC_SPELL(d))), atoi(arg));
		spedit_reagents_menu(d);
		return;
/*-------------------------------------------------------------------*/
	case SPEDIT_FOCUSES_MENU:
		switch (*arg) {
		case 'a':
		case 'A':
			write_to_output(d, TRUE, "\r\nEnter new focus vnum number : ");
			OLC_MODE(d) = SPEDIT_NEW_FOCUS;
			return;
		case 'd':
		case 'D':
			write_to_output(d, TRUE, "\r\nDelete which focus? : ");
			OLC_MODE(d) = SPEDIT_DELETE_FOCUS;
			return;
		case 'q':
		case 'Q':
			break;
		default:
			write_to_output(d, TRUE, "\r\n");
			OLC_MODE(d) = SPEDIT_DELETE_FOCUS;
			spedit_focuses_menu(d);
			return;
		}
		break;

	case SPEDIT_NEW_FOCUS:
		if ((i = atoi(arg)) != -1)
			if ((i = real_object(i)) == -1) {
				write_to_output(d, TRUE, "That object does not exist, try again : ");
				return;
			}
		if (i > 0)
			OLC_VAL(d) = atoi(arg);
		else
			OLC_VAL(d) = -1;
		write_to_output(d, TRUE, "\r\nShould the item be in the room? : ");
		OLC_MODE(d) = SPEDIT_FOCUS_LOCATION;
		return;

	case SPEDIT_FOCUS_LOCATION:
		switch(*arg) {
		case 'Y':
		case 'y':
			OLC_VAL2(d) = 1;
			break;
		case 'N':
		case 'n':
			OLC_VAL2(d) = 0;
			break;
		default:
			write_to_output(d, TRUE, "Please answer Yes or No : ");
			return;
		}
		write_to_output(d, TRUE, "\r\nShould the item be spent upon usage? : ");
		OLC_MODE(d) = SPEDIT_FOCUS_EXTRACT;
		return;

	case SPEDIT_FOCUS_EXTRACT:
		SPELL_OBJ_TYPE(new_entry) = OLC_VAL(d);
		SPELL_OBJ_LOCATION(new_entry) = OLC_VAL2(d);
		switch(*arg) {
		case 'Y':
		case 'y':
			SPELL_OBJ_EXTRACT(new_entry) = 1;
			break;
		case 'N':
		case 'n':
			SPELL_OBJ_EXTRACT(new_entry) = 0;
			break;
		default:
			write_to_output(d, TRUE, "Please answer Yes or No : ");
			return;
		}
		add_to_obj_list(&(S_FOCUSES(OLC_SPELL(d))), &new_entry);
		write_to_output(d, TRUE, "\r\n");
		spedit_focuses_menu(d);
		return;

	case SPEDIT_DELETE_FOCUS:
		remove_from_obj_list(&(S_FOCUSES(OLC_SPELL(d))), atoi(arg));
		spedit_focuses_menu(d);
		return;
/*-------------------------------------------------------------------*/
	case SPEDIT_TRANSMUTERS_MENU:
		switch (*arg) {
		case 'a':
		case 'A':
			write_to_output(d, TRUE, "\r\nEnter new transmuter vnum number : ");
			OLC_MODE(d) = SPEDIT_NEW_TRANSMUTER;
			return;
		case 'd':
		case 'D':
			write_to_output(d, TRUE, "\r\nDelete which transmuter? : ");
			OLC_MODE(d) = SPEDIT_DELETE_TRANSMUTER;
			return;
		case 'q':
		case 'Q':
			break;
		default:
			write_to_output(d, TRUE, "\r\n");
			OLC_MODE(d) = SPEDIT_DELETE_TRANSMUTER;
			spedit_transmuters_menu(d);
			return;
		}
		break;

	case SPEDIT_NEW_TRANSMUTER:
		if ((i = atoi(arg)) != -1)
			if ((i = real_object(i)) == -1) {
				write_to_output(d, TRUE, "That object does not exist, try again : ");
				return;
			}
		if (i > 0)
			OLC_VAL(d) = atoi(arg);
		else
			OLC_VAL(d) = -1;
		write_to_output(d, TRUE, "\r\nShould the item be in the room? : ");
		OLC_MODE(d) = SPEDIT_TRANSMUTER_LOCATION;
		return;

	case SPEDIT_TRANSMUTER_LOCATION:
		switch(*arg) {
		case 'Y':
		case 'y':
			OLC_VAL2(d) = 1;
			break;
		case 'N':
		case 'n':
			OLC_VAL2(d) = 0;
			break;
		default:
			write_to_output(d, TRUE, "Please answer Yes or No : ");
			return;
		}
		write_to_output(d, TRUE, "\r\nShould the item be spent upon usage? : ");
		OLC_MODE(d) = SPEDIT_TRANSMUTER_EXTRACT;
		return;

	case SPEDIT_TRANSMUTER_EXTRACT:
		SPELL_OBJ_TYPE(new_entry) = OLC_VAL(d);
		SPELL_OBJ_LOCATION(new_entry) = OLC_VAL2(d);
		switch(*arg) {
		case 'Y':
		case 'y':
			SPELL_OBJ_EXTRACT(new_entry) = 1;
			break;
		case 'N':
		case 'n':
			SPELL_OBJ_EXTRACT(new_entry) = 0;
			break;
		default:
			write_to_output(d, TRUE, "Please answer Yes or No : ");
			return;
		}
		add_to_obj_list(&(S_TRANSMUTERS(OLC_SPELL(d))), &new_entry);
		write_to_output(d, TRUE, "\r\n");
		spedit_transmuters_menu(d);
		return;

	case SPEDIT_DELETE_TRANSMUTER:
		remove_from_obj_list(&(S_TRANSMUTERS(OLC_SPELL(d))), atoi(arg));
		spedit_transmuters_menu(d);
		return;
/*-------------------------------------------------------------------*/
	default:
		cleanup_olc(d, CLEANUP_ALL);
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: spedit_parse(): Reached default case!");
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
	spedit_disp_menu(d);
}

/*
* Create a new spell with some default strings.
*/
void spedit_setup_new(struct descriptor_data *d)
{
	if ((OLC_SPELL(d) = enqueue_spell()) != NULL) {
		OLC_SPELL(d)->number = newspell;
		OLC_SPELL(d)->skill = GROUP_MAGERY;
		OLC_SPELL(d)->name = str_dup("New spell");
		OLC_SPELL(d)->long_name = '\0';
		OLC_SPELL(d)->wear_off_msg = '\0';
		OLC_SPELL(d)->victim_msg = '\0';
		OLC_SPELL(d)->cast_msg = str_dup("Nothing.\r\n");
		OLC_SPELL(d)->mundane_msg = str_dup("Nothing.\r\n");
		OLC_SPELL(d)->target_msg = str_dup("Nothing.\r\n");
		OLC_SPELL(d)->mana_min = 0;
		OLC_SPELL(d)->mana_max = 0;
		OLC_SPELL(d)->min_position = POS_STANDING;
		OLC_SPELL(d)->technique = 0;
		OLC_SPELL(d)->form = 0;
		OLC_SPELL(d)->range = 0;
		OLC_SPELL(d)->duration = 0;
		OLC_SPELL(d)->target_group = 0;
		OLC_SPELL(d)->targets = TAR_IGNORE;
		OLC_SPELL(d)->violent = NO;
		OLC_SPELL(d)->routines = 0;
		OLC_SPELL(d)->learned = YES;
		
		CREATE(S_REAGENTS(OLC_SPELL(d)), struct spell_obj_data, 1);
		S_REG_TYPE(OLC_SPELL(d), 0) = NOTHING;
		S_REG_LOCATION(OLC_SPELL(d), 0) = 0;
		S_REG_EXTRACT(OLC_SPELL(d), 0) = 0;
		
		CREATE(S_FOCUSES(OLC_SPELL(d)), struct spell_obj_data, 1);
		S_FOC_TYPE(OLC_SPELL(d), 0) = NOTHING;
		S_FOC_LOCATION(OLC_SPELL(d), 0) = 0;
		S_FOC_EXTRACT(OLC_SPELL(d), 0) = 0;
		
		CREATE(S_TRANSMUTERS(OLC_SPELL(d)), struct spell_obj_data, 1);
		S_TRA_TYPE(OLC_SPELL(d), 0) = NOTHING;
		S_TRA_LOCATION(OLC_SPELL(d), 0) = 0;
		S_TRA_EXTRACT(OLC_SPELL(d), 0) = 0;
		
	} else
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: spedit_setup_new(): Unable to create new spell!");
	spnum++;
	if (newspell == spnum)
		newspell++;
	else 
		newspell = spnum;
	spedit_disp_menu(d);
	OLC_VAL(d) = 0;
}


void spedit_string_cleanup(struct descriptor_data *d, int terminator)
{
	switch (OLC_MODE(d)) {
	case SPEDIT_CAST_MSG:
		spedit_disp_menu(d);
		break;
	case SPEDIT_MUNDANE_MSG:
		spedit_disp_menu(d);
		break;
	case SPEDIT_TARGET_MSG:
		spedit_disp_menu(d);
		break;
	default:
		spedit_disp_menu(d);
		break;
	}
}

/*-------------------------------------------------------------------*/

void remove_from_obj_list(struct spell_obj_data **list, int num)
{
	int i, num_items;
	struct spell_obj_data *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i].type != -1; i++);

	if (num < 0 || num >= i)
		return;
	num_items = i;

	CREATE(nlist, struct spell_obj_data, num_items);

	for (i = 0; i < num_items; i++)
		nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

	free(*list);
	*list = nlist;
}

/*-------------------------------------------------------------------*/

void add_to_obj_list(struct spell_obj_data **list, struct spell_obj_data *newl)
{
	int i, num_items;
	struct spell_obj_data *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i].type != -1; i++);
	num_items = i;

	/*
	 * Make a new list and slot in the new entry.
	 */
	CREATE(nlist, struct spell_obj_data, num_items + 2);

	for (i = 0; i < num_items; i++)
		nlist[i] = (*list)[i];
	nlist[num_items] = *newl;
	nlist[num_items + 1].type = -1;

	/*
	 * Out with the old, in with the new.
	 */
	free(*list);
	*list = nlist;
}
