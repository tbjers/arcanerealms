/************************************************************************
 * OasisOLC - Mobiles / medit.c                                    v2.0 *
 * Copyright 1996 Harvey Gilpin                                         *
 * Copyright 1997-1999 George Greer (greerga@circlemud.org)             *
 ************************************************************************/
/* $Id: medit.c,v 1.37 2003/03/17 01:46:50 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "interpreter.h"
#include "comm.h"
#include "spells.h"
#include "events.h"
#include "skills.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genmob.h"
#include "genzon.h"
#include "genshp.h"
#include "oasis.h"
#include "handler.h"
#include "constants.h"
#include "dg_olc.h"
#include "specset.h"
#include "characters.h"
#include "screen.h"

/*-------------------------------------------------------------------*/

/*
 * External variable declarations.
 */
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct char_data *character_list;
extern mob_rnum top_of_mobt;
extern struct zone_data *zone_table;
extern struct attack_hit_type attack_hit_text[];
extern struct shop_data *shop_index;
extern struct descriptor_data *descriptor_list;
extern int top_of_race_list;
extern struct race_list_element *race_list;
#if	CONFIG_OASIS_MPROG
extern const char *mobprog_types[];
#endif

/*-------------------------------------------------------------------*/

/*
 * Handy internal macros.
 */
#if	CONFIG_OASIS_MPROG
#define	GET_MPROG(mob)				(mob_index[(mob)->nr].mobprogs)
#define	GET_MPROG_TYPE(mob)		(mob_index[(mob)->nr].progtypes)
#endif

/*-------------------------------------------------------------------*/

/*
 * Function prototypes.
 */
#if	CONFIG_OASIS_MPROG
void medit_disp_mprog(struct descriptor_data *d);
void medit_change_mprog(struct descriptor_data *d);
const	char *medit_get_mprog_type(struct mob_prog_data *mprog);
#endif

/*-------------------------------------------------------------------*\
	utility functions 
\*-------------------------------------------------------------------*/

void medit_save_to_disk(zone_vnum foo)
{
	save_mobiles(foo);
}

void medit_setup_new(struct descriptor_data *d)
{
	struct char_data *mob;

	/*
	 * Allocate a scratch mobile structure.  
	 */
	CREATE(mob, struct char_data, 1);

	init_mobile(mob);

	GET_MOB_RNUM(mob) = NOBODY;
	/*
	 * Set up some default strings.
	 */
	GET_ALIAS(mob) = str_dup("mob unfinished");
	GET_SDESC(mob) = str_dup("the unfinished mob");
	GET_LDESC(mob) = str_dup("An unfinished mob stands here.\r\n");
	GET_DDESC(mob) = str_dup("It looks unfinished.\r\n");
#if	CONFIG_OASIS_MPROG
	OLC_MPROGL(d) = NULL;
	OLC_MPROG(d) = NULL;
#endif

	OLC_MOB(d) = mob;
	/* Has changed flag. (It hasn't so far, we just made it.) */
	OLC_VAL(d) = FALSE;
	OLC_ITEM_TYPE(d) = MOB_TRIGGER;

	medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void medit_setup_existing(struct descriptor_data *d, int rmob_num)
{
	struct char_data *mob;

	/*
	 * Allocate a scratch mobile structure. 
	 */
	CREATE(mob, struct char_data, 1);

	copy_mobile(mob, mob_proto + rmob_num);

#if	CONFIG_OASIS_MPROG
	{
		MPROG_DATA *temp;
		MPROG_DATA *head;

		if (GET_MPROG(mob))
			CREATE(OLC_MPROGL(d), MPROG_DATA, 1);
		head = OLC_MPROGL(d);
		for (temp = GET_MPROG(mob); temp; temp = temp->next) {
			OLC_MPROGL(d)->type = temp->type;
			OLC_MPROGL(d)->arglist = str_dup(temp->arglist);
			OLC_MPROGL(d)->comlist = str_dup(temp->comlist);
			if (temp->next) {
				CREATE(OLC_MPROGL(d)->next, MPROG_DATA, 1);
				OLC_MPROGL(d) = OLC_MPROGL(d)->next;
			}
		}
		OLC_MPROGL(d) = head;
		OLC_MPROG(d) = OLC_MPROGL(d);
	}
#endif

	OLC_MOB(d) = mob;
	OLC_ITEM_TYPE(d) = MOB_TRIGGER;
	dg_olc_script_copy(d);
	medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/*
 * Ideally, this function should be in db.c, but I'll put it here for
 * portability.
 */
void init_mobile(struct char_data *mob)
{
	clear_char(mob);

	GET_HIT(mob) = GET_MANA(mob) = 1;
	GET_MAX_MANA(mob) = GET_MAX_MOVE(mob) = 100;
	GET_NDD(mob) = GET_SDD(mob) = 1;
	GET_WEIGHT(mob) = 200;
	GET_HEIGHT(mob) = 198;

	mob->real_abils.strength = mob->real_abils.agility = mob->real_abils.precision = 1100;
	mob->real_abils.perception = mob->real_abils.health = mob->real_abils.willpower = 1100;
	mob->real_abils.intelligence = mob->real_abils.charisma = mob->real_abils.luck = 1100;
	mob->real_abils.essence = 1100;
	mob->aff_abils = mob->real_abils;

	SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);
	mob->player_specials = &dummy_mob;
}

/*-------------------------------------------------------------------*/

/*
 * Save new/edited mob to memory.
 */
void medit_save_internally(struct descriptor_data *d)
{
	int i;
	mob_rnum new_rnum;
	struct descriptor_data *dsc;
	struct char_data *mob;

	/* put the script into proper position */
	OLC_MOB(d)->proto_script = OLC_SCRIPT(d);

	i = (real_mobile(OLC_NUM(d)) == NOBODY);

	if ((new_rnum = add_mobile(OLC_MOB(d), OLC_NUM(d))) == NOBODY) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "medit_save_internally: add_mobile failed.");
		return;
	}

	/* Make sure scripts are updated too. - Welcor */
	/* Free old proto list  */
	if (mob_proto[new_rnum].proto_script &&
			mob_proto[new_rnum].proto_script != OLC_SCRIPT(d)) {
		struct trig_proto_list *proto, *fproto;
		proto = mob_proto[new_rnum].proto_script;
		while (proto) {
			fproto = proto;
			proto = proto->next;
			free(fproto);
		}
	}    

	/* this will handle new instances of the mob: */
	mob_proto[new_rnum].proto_script = OLC_SCRIPT(d);

	/* this takes care of the mobs currently in-game */
	for (mob = character_list; mob; mob = mob->next) {
		if (GET_MOB_RNUM(mob) != new_rnum) {
			continue;
		}
		/* remove any old scripts */
		if (SCRIPT(mob)) {
			extract_script(SCRIPT(mob));
			SCRIPT(mob) = NULL;
		}
		mob->proto_script = OLC_SCRIPT(d);

		assign_triggers(mob, MOB_TRIGGER);
	}
	
	if (!i)        /* Only renumber on new mobiles. */
		return;

	/*
	 * Update keepers in shops being edited and other mobs being edited.
	 */
	for (dsc = descriptor_list; dsc; dsc = dsc->next) {
		if (STATE(dsc) == CON_SEDIT)
			S_KEEPER(OLC_SHOP(dsc)) += (S_KEEPER(OLC_SHOP(dsc)) >= new_rnum);
		else if (STATE(dsc) == CON_MEDIT)
			GET_MOB_RNUM(OLC_MOB(dsc)) += (GET_MOB_RNUM(OLC_MOB(dsc)) >= new_rnum);
	}

	/*
	 * Update other people in zedit too. From: C.Raehl 4/27/99
	 */
	for (dsc = descriptor_list; dsc; dsc = dsc->next)
		if (STATE(dsc) == CON_ZEDIT)
			for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
				if (OLC_ZONE(dsc)->cmd[i].command == 'M')
					if (OLC_ZONE(dsc)->cmd[i].arg1 >= new_rnum)
						OLC_ZONE(dsc)->cmd[i].arg1++;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * Display positions. (sitting, standing, etc)
 */
void medit_disp_positions(struct descriptor_data *d)
{
	int i;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; *position_types[i] != '\n'; i++)
		write_to_output(d, TRUE, "%s%2d%s) %s\r\n", grn, i, nrm, position_types[i]);
	write_to_output(d, TRUE, "Enter position number : ");
}

/*-------------------------------------------------------------------*/

#if	CONFIG_OASIS_MPROG
/*
 * Get the type of MobProg.
 */
const	char *medit_get_mprog_type(struct mob_prog_data *mprog)
{
	switch (mprog->type) {
	case IN_FILE_PROG:        return ">in_file_prog";
	case ACT_PROG:        return ">act_prog";
	case SPEECH_PROG:        return ">speech_prog";
	case RAND_PROG:        return ">rand_prog";
	case FIGHT_PROG:        return ">fight_prog";
	case HITPRCNT_PROG:        return ">hitprcnt_prog";
	case DEATH_PROG:        return ">death_prog";
	case ENTRY_PROG:        return ">entry_prog";
	case GREET_PROG:        return ">greet_prog";
	case ALL_GREET_PROG:        return ">all_greet_prog";
	case GIVE_PROG:        return ">give_prog";
	case BRIBE_PROG:        return ">bribe_prog";
	}
	return ">ERROR_PROG";
}

/*-------------------------------------------------------------------*/

/*
 * Display the MobProgs.
 */
void medit_disp_mprog(struct descriptor_data *d)
{
	struct mob_prog_data *mprog = OLC_MPROGL(d);

	OLC_MTOTAL(d) = 1;

	clear_screen(d);
	while (mprog) {
		write_to_output(d, TRUE, "%d) %s %s\r\n", OLC_MTOTAL(d), medit_get_mprog_type(mprog),
								(mprog->arglist ? mprog->arglist : "NONE"));
		OLC_MTOTAL(d)++;
		mprog = mprog->next;
	}
	write_to_output(d, TRUE,  "%d) Create New Mob Prog\r\n"
								"%d) Purge Mob Prog\r\n"
								"Enter number to edit [0 to exit]:  ",
								OLC_MTOTAL(d), OLC_MTOTAL(d) + 1);
	OLC_MODE(d) = MEDIT_MPROG;
}

/*-------------------------------------------------------------------*/

/*
 * Change the MobProgs.
 */
void medit_change_mprog(struct descriptor_data *d)
{
	clear_screen(d);
	write_to_output(d, TRUE,  "1) Type: %s\r\n"
								"2) Args: %s\r\n"
								"3) Commands:\r\n%s\r\n\r\n"
								"Enter number to edit [0 to exit]: ",
				medit_get_mprog_type(OLC_MPROG(d)),
				(OLC_MPROG(d)->arglist ? OLC_MPROG(d)->arglist: "NONE"),
				(OLC_MPROG(d)->comlist ? OLC_MPROG(d)->comlist : "NONE"));

	OLC_MODE(d) = MEDIT_CHANGE_MPROG;
}

/*-------------------------------------------------------------------*/

/*
 * Change the MobProg type.
 */
void medit_disp_mprog_types(struct descriptor_data *d)
{
	int i;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i < NUM_PROGS-1; i++)
		write_to_output(d, TRUE, "%s%2d%s) %s\r\n", grn, i, nrm, mobprog_types[i]);
	write_to_output(d, TRUE, "Enter mob prog type : ");
	OLC_MODE(d) = MEDIT_MPROG_TYPE;
}
#endif

/*-------------------------------------------------------------------*/

/*
 * Display a list of Special Procedures
 */
void medit_disp_specproc(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i < NUM_MOB_SPECS; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s  %s", grn, i, nrm, mob_specproc_info[i].name,
												!(++columns % 2) ? "\r\n" : "");

	write_to_output(d, TRUE, "\r\nEnter Spec Proc number : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display a list of races
 */
void medit_disp_race(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i <= top_of_race_list; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s  %s", grn, i, nrm, race_list[i].name,
												!(++columns % 2) ? "\r\n" : "");

	write_to_output(d, TRUE, "\r\nEnter Race : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */
void medit_disp_sex(struct descriptor_data *d)
{
	int i;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i < NUM_GENDERS; i++)
		write_to_output(d, TRUE, "%s%2d%s) %s\r\n", grn, i, nrm, genders[i]);
	write_to_output(d, TRUE, "Enter gender number : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display attack types menu.
 */
void medit_disp_attack_types(struct descriptor_data *d)
{
	int i;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i < NUM_ATTACK_TYPES; i++)
		write_to_output(d, TRUE, "%s%2d%s) %s\r\n", grn, i, nrm, attack_hit_text[i].singular);
	write_to_output(d, TRUE, "Enter attack type : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display mob-flags menu.
 */
void medit_disp_mob_flags(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (i = 0; i < NUM_MOB_FLAGS; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, action_bits[i],
								!(++columns % 2) ? "\r\n" : "");
	sprintbit(MOB_FLAGS(OLC_MOB(d)), action_bits, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nCurrent flags : %s%s%s\r\nEnter mob flags (0 to quit) : ",
									cyn, buf1, nrm);
}

/*-------------------------------------------------------------------*/

/*
 * Display affection flags menu.
 */
void medit_disp_aff_flags(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (i = 0; i < NUM_AFF_FLAGS; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, affected_bits[i],
												!(++columns % 2) ? "\r\n" : "");
	sprintbit(AFF_FLAGS(OLC_MOB(d)), affected_bits, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nCurrent flags   : %s%s%s\r\nEnter aff flags (0 to quit) : ",
													cyn, buf1, nrm);
}

/*-------------------------------------------------------------------*/

/*
 * Display mob difficulty menu.
 */
void medit_disp_difficulties(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (i = 0; i < NUM_MOB_DIFFICULTIES; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, mob_difficulty_type[i],
												!(++columns % 2) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\nEnter mob difficulty : ");
}

/*-------------------------------------------------------------------*/

/*
 * Mob value #1
 */
void medit_disp_val1_menu(struct descriptor_data *d)
{
	OLC_MODE(d) = MEDIT_VALUE_1;
	write_to_output(d, FALSE, "\r\nCorpse vnum (enter 0 for standard corpse) : ");
}

/*
 * Mob value #2
 */
void medit_disp_val2_menu(struct descriptor_data *d)
{
	OLC_VAL(d) = 1; /* If we get here, we changed something */
	OLC_MODE(d) = MEDIT_VALUE_2;
	write_to_output(d, FALSE, "\r\nMaximum number of resources to create on corpse\r\n(enter 0 for no resources at all) : ");
}

/*
 * Mob value #3
 */
void medit_disp_val3_menu(struct descriptor_data *d)
{
	OLC_MODE(d) = MEDIT_VALUE_3;
	write_to_output(d, FALSE, "\r\nResource object #1 vnum\r\n(Enter 0 to not use this slot) : ");
}

/*
 * Mob value #4
 */
void medit_disp_val4_menu(struct descriptor_data *d)
{
	OLC_MODE(d) = MEDIT_VALUE_4;
	write_to_output(d, FALSE, "\r\nResource object #2 vnum\r\n(Enter 0 to not use this slot) : ");
}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void medit_disp_menu(struct descriptor_data *d)
{
	struct char_data *mob;

	mob = OLC_MOB(d);
	get_char_colors(d->character);
	clear_screen(d);

	write_to_output(d, TRUE,
					"-- Mob Number:  [%s%d%s]   Spec Proc : %s%s\r\n"
					"%s1%s) Sex: %s%-7.7s%s              %s2%s) Keywords: %s%s\r\n"
					"%s3%s) S-Desc: %s\\c98%s\\c99\r\n"
					"%s4%s) L-Desc:-\r\n%s\\c98%s\\c99"
					"%s5%s) D-Desc:-\r\n%s\\c98%s\\c99"
					"%s6%s) Difficulty: %s%s%s\r\n"
					"%s7%s) Alignment:       [%s%4d%s]\r\n"
					"%s8%s) NumDamDice:      [%s%4d%s],  %s9%s) SizeDamDice:  [%s%4d%s]\r\n"
					"%sA%s) Num HP Dice:     [%s%4d%s],  %sB%s) Size HP Dice: [%s%4d%s],  %sC%s) HP Bonus: [%s%5d%s]\r\n"
					"%sD%s) Passive Defense: [%s%4d%s],  %sE%s) Gold:     [%s%8d%s]\r\n",

					cyn, OLC_NUM(d), nrm,
					cyn, real_mobile(OLC_NUM(d)) >= 0 ? mob_specproc_info[LIMIT(mob_index[real_mobile(OLC_NUM(d))].specproc, 0, NUM_MOB_SPECS)].name : "None",
					grn, nrm, yel, genders[(int)GET_SEX(mob)], nrm,
					grn, nrm, yel, GET_ALIAS(mob),
					grn, nrm, yel, GET_SDESC(mob),
					grn, nrm, yel, GET_LDESC(mob),
					grn, nrm, yel, GET_DDESC(mob),
					grn, nrm, cyn, mob_difficulty_type[(int)GET_DIFFICULTY(mob)], nrm,
					grn, nrm, cyn, GET_ALIGNMENT(mob), nrm,
					grn, nrm, cyn, GET_NDD(mob), nrm,
					grn, nrm, cyn, GET_SDD(mob), nrm,
					grn, nrm, cyn, GET_HIT(mob), nrm,
					grn, nrm, cyn, GET_MANA(mob), nrm,
					grn, nrm, cyn, GET_MOVE(mob), nrm,
					grn, nrm, cyn, GET_PD(mob), nrm,
					grn, nrm, cyn, GET_GOLD(mob), nrm
					);

	sprintbit(MOB_FLAGS(mob), action_bits, buf1, sizeof(buf1));
	sprintbit(AFF_FLAGS(mob), affected_bits, buf2, sizeof(buf2));
	
	write_to_output(d, TRUE,
					"%sF%s) Position  : %s%s\r\n"
					"%sG%s) Default   : %s%s\r\n"
					"%sH%s) Attack    : %s%s\r\n"
					"%sI%s) NPC Flags : %s%s\r\n"
					"%sJ%s) AFF Flags : %s%s\r\n"
#if	CONFIG_OASIS_MPROG
					"%sK%s) Mob Progs : %s%s\r\n"
#endif
					"%sR%s) Race      : %s%s\r\n"
					"%sS%s) Script    : %s%s\r\n"
					"%sV%s) Values    : %s%d %d %d %d\r\n"
					"%sQ%s) Quit\r\n"
					"Enter choice : ",

					grn, nrm, yel, position_types[LIMIT((int)GET_POS(mob), 0, NUM_POSITIONS)],
					grn, nrm, yel, position_types[LIMIT((int)GET_DEFAULT_POS(mob), 0, NUM_POSITIONS)],
					grn, nrm, yel, attack_hit_text[GET_ATTACK(mob)].singular,
					grn, nrm, cyn, buf1,
					grn, nrm, cyn, buf2,
#if	CONFIG_OASIS_MPROG
					grn, nrm, cyn, (OLC_MPROGL(d) ? "Set." : "Not Set."),
#endif
					grn, nrm, cyn, race_list[(int)GET_RACE(mob)].name,
					grn, nrm, cyn, OLC_SCRIPT(d) ?"Set.":"Not Set.",
					grn, nrm, cyn, GET_MOB_VAL(mob, 0),
					GET_MOB_VAL(mob, 1),
					GET_MOB_VAL(mob, 2),
					GET_MOB_VAL(mob, 3),
					grn, nrm
					);

	OLC_MODE(d) = MEDIT_MAIN_MENU;
}

/************************************************************************
 *                        The GARGANTAUN event handler                        *
 ************************************************************************/

void medit_parse(struct descriptor_data *d, char *arg)
{
	int i = -1;
	int race = -1;

	if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE) {
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
/*-------------------------------------------------------------------*/
	case MEDIT_CONFIRM_SAVESTRING:
		/*
		 * Ensure mob has MOB_ISNPC set or things will go pear shaped.
		 */
		SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
		switch (*arg) {
		case 'y':
		case 'Y':
			/*
			 * Save the mob in memory and to disk.
			 */
			write_to_output(d, TRUE, "Saving mobile to disk and memory.\r\n");
			medit_save_internally(d);
			medit_save_to_disk(OLC_ZNUM(d));
			extended_mudlog(BRF, SYSL_OLC, TRUE, "%s edits mob %d", GET_NAME(d->character), OLC_NUM(d));
			/* FALL THROUGH */
		case 'n':
		case 'N':
			cleanup_olc(d, CLEANUP_ALL);
			return;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\n");
			write_to_output(d, TRUE, "Do you wish to save the mobile? : ");
			return;
		}
		break;

/*-------------------------------------------------------------------*/
	case MEDIT_MAIN_MENU:
		i = 0;
		switch (*arg) {
		case 'q':
		case 'Q':
			if (OLC_VAL(d)) {        /* Anything been changed? */
				write_to_output(d, TRUE, "Do you wish to save the changes to the mobile? (y/n) : ");
				OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
			} else
				cleanup_olc(d, CLEANUP_ALL);
			return;
		case '1':
			OLC_MODE(d) = MEDIT_SEX;
			medit_disp_sex(d);
			return;
		case '2':
			OLC_MODE(d) = MEDIT_ALIAS;
			i--;
			break;
		case '3':
			OLC_MODE(d) = MEDIT_S_DESC;
			i--;
			break;
		case '4':
			OLC_MODE(d) = MEDIT_L_DESC;
			i--;
			break;
		case '5':
			OLC_MODE(d) = MEDIT_D_DESC;
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_MOB(d)->player.description)
				write_to_output(d, FALSE, "%s", OLC_MOB(d)->player.description);
			string_write(d, &OLC_MOB(d)->player.description, MAX_MOB_DESC, 0, STATE(d));
			OLC_VAL(d) = 1;
			return;
		case '6':
			OLC_MODE(d) = MEDIT_DIFFICULTY;
			medit_disp_difficulties(d);
			return;
		case '7':
			OLC_MODE(d) = MEDIT_ALIGNMENT;
			i++;
			break;
		case '8':
			OLC_MODE(d) = MEDIT_NDD;
			i++;
			break;
		case '9':
			OLC_MODE(d) = MEDIT_SDD;
			i++;
			break;
		case 'a':
		case 'A':
			OLC_MODE(d) = MEDIT_NUM_HP_DICE;
			i++;
			break;
		case 'b':
		case 'B':
			OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
			i++;
			break;
		case 'c':
		case 'C':
			OLC_MODE(d) = MEDIT_ADD_HP;
			i++;
			break;
		case 'd':
		case 'D':
			OLC_MODE(d) = MEDIT_AC;
			i++;
			break;
		case 'e':
		case 'E':
			OLC_MODE(d) = MEDIT_GOLD;
			i++;
			break;
		case 'f':
		case 'F':
			OLC_MODE(d) = MEDIT_POS;
			medit_disp_positions(d);
			return;
		case 'g':
		case 'G':
			OLC_MODE(d) = MEDIT_DEFAULT_POS;
			medit_disp_positions(d);
			return;
		case 'h':
		case 'H':
			OLC_MODE(d) = MEDIT_ATTACK;
			medit_disp_attack_types(d);
			return;
		case 'i':
		case 'I':
			OLC_MODE(d) = MEDIT_NPC_FLAGS;
			medit_disp_mob_flags(d);
			return;
		case 'j':
		case 'J':
			OLC_MODE(d) = MEDIT_AFF_FLAGS;
			medit_disp_aff_flags(d);
			return;
#if	CONFIG_OASIS_MPROG
		case 'k':
		case 'K':
			OLC_MODE(d) = MEDIT_MPROG;
			medit_disp_mprog(d);
			return;
#endif
		case 'r':
		case 'R':
			OLC_MODE(d) = MEDIT_RACE;
			medit_disp_race(d);
			return;
		case 's':
		case 'S':
			if (GOT_RIGHTS(d->character, RIGHTS_TRIGGERS)) {
				OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
				dg_script_menu(d);
				return;
			} else {
				write_to_output(d, TRUE, "\r\n&RYou do not have trigger editing rights.&n\r\n\r\n");
				medit_disp_menu(d);
				return;
			}
			break;
		case 'v':
		case 'V':
			/*
			 * Clear any old values  
			 */
			GET_MOB_VAL(OLC_MOB(d), 0) = 0;
			GET_MOB_VAL(OLC_MOB(d), 1) = 0;
			GET_MOB_VAL(OLC_MOB(d), 2) = 0;
			GET_MOB_VAL(OLC_MOB(d), 3) = 0;
			if (MOB_FLAGGED(OLC_MOB(d), MOB_RESOURCE)) {
				medit_disp_val1_menu(d);
			} else {
				OLC_VAL(d) = 1; /* If we get here, we changed something */
				medit_disp_menu(d);
			}
			return;
		default:
			write_to_output(d, TRUE, "\r\n&RInvalid choice!&n\r\n\r\n");
			medit_disp_menu(d);
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
	case OLC_SCRIPT_EDIT:
		if (dg_script_edit_parse(d, arg)) return;
		break;
/*-------------------------------------------------------------------*/
	case MEDIT_ALIAS:
		if (GET_ALIAS(OLC_MOB(d)))
			free(GET_ALIAS(OLC_MOB(d)));
		GET_ALIAS(OLC_MOB(d)) = str_udup(arg);
		break;
/*-------------------------------------------------------------------*/
	case MEDIT_S_DESC:
		if (GET_SDESC(OLC_MOB(d)))
			free(GET_SDESC(OLC_MOB(d)));
		GET_SDESC(OLC_MOB(d)) = str_udup(arg);
		break;
/*-------------------------------------------------------------------*/
	case MEDIT_L_DESC:
		if (GET_LDESC(OLC_MOB(d)))
			free(GET_LDESC(OLC_MOB(d)));
		if (arg && *arg) {
			strcpy(buf, arg);
			strcat(buf, "\r\n");
			GET_LDESC(OLC_MOB(d)) = str_dup(buf);
		} else
			GET_LDESC(OLC_MOB(d)) = str_dup("undefined");

		break;
/*-------------------------------------------------------------------*/
	case MEDIT_D_DESC:
		/*
		 * We should never get here.
		 */
		cleanup_olc(d, CLEANUP_ALL);
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: medit_parse(): Reached D_DESC case!");
		write_to_output(d, TRUE, "Oops...\r\n");
		break;
/*-------------------------------------------------------------------*/
#if	CONFIG_OASIS_MPROG
	case MEDIT_MPROG_COMLIST:
		/*
		 * We should never get here, but if we do, bail out.
		 */
		cleanup_olc(d, CLEANUP_ALL);
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: medit_parse(): Reached MPROG_COMLIST case!");
		break;
#endif
/*-------------------------------------------------------------------*/
	case MEDIT_NPC_FLAGS:
		if ((i = atoi(arg)) <= 0)
			break;
		else if (i <= NUM_MOB_FLAGS)
			TOGGLE_BIT(MOB_FLAGS(OLC_MOB(d)), 1ULL << (i - 1));
		medit_disp_mob_flags(d);
		return;
/*-------------------------------------------------------------------*/
	case MEDIT_AFF_FLAGS:
		if ((i = atoi(arg)) <= 0)
			break;
		else if (i <= NUM_AFF_FLAGS)
			TOGGLE_BIT(AFF_FLAGS(OLC_MOB(d)), 1ULL << (i - 1));
		medit_disp_aff_flags(d);
		return;
/*-------------------------------------------------------------------*/
#if	CONFIG_OASIS_MPROG
	case MEDIT_MPROG:
		if ((i = atoi(arg)) == 0)
			medit_disp_menu(d);
		else if (i == OLC_MTOTAL(d)) {
			struct mob_prog_data *temp;
			CREATE(temp, struct mob_prog_data, 1);
			temp->next = OLC_MPROGL(d);
			temp->type = -1;
			temp->arglist = NULL;
			temp->comlist = NULL;
			OLC_MPROG(d) = temp;
			OLC_MPROGL(d) = temp;
			OLC_MODE(d) = MEDIT_CHANGE_MPROG;
			medit_change_mprog (d);
		} else if (i < OLC_MTOTAL(d)) {
			struct mob_prog_data *temp;
			int x = 1;
			for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
				x++;
			OLC_MPROG(d) = temp;
			OLC_MODE(d) = MEDIT_CHANGE_MPROG;
			medit_change_mprog (d);
		} else if (i == (OLC_MTOTAL(d) + 1)) {
			write_to_output(d, TRUE, "Which mob prog do you want to purge? ");
			OLC_MODE(d) = MEDIT_PURGE_MPROG;
		} else
			medit_disp_menu(d);
		return;
/*-------------------------------------------------------------------*/
	case MEDIT_PURGE_MPROG:
		if ((i = atoi(arg)) > 0 && i < OLC_MTOTAL(d)) {
			struct mob_prog_data *temp;
			int x = 1;

			for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
				x++;
			OLC_MPROG(d) = temp;
			REMOVE_FROM_LIST(OLC_MPROG(d), OLC_MPROGL(d), next);
			free(OLC_MPROG(d)->arglist);
			free(OLC_MPROG(d)->comlist);
			free(OLC_MPROG(d));
			OLC_MPROG(d) = NULL;
			OLC_VAL(d) = 1;
		}
		medit_disp_mprog(d);
		return;
/*-------------------------------------------------------------------*/
	case MEDIT_CHANGE_MPROG:
		if ((i = atoi(arg)) == 1)
			medit_disp_mprog_types(d);
		else if (i == 2) {
			write_to_output(d, TRUE, "Enter new arg list: ");
			OLC_MODE(d) = MEDIT_MPROG_ARGS;
		} else if (i == 3) {
			write_to_output(d, TRUE, "Enter new mob prog commands:\r\n");
			/*
			 * Pass control to modify.c for typing.
			 */
			OLC_MODE(d) = MEDIT_MPROG_COMLIST;
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_MPROG(d)->comlist)
				write_to_output(d, TRUE, "%s", OLC_MPROG(d)->comlist);
			string_write(d, &OLC_MPROG(d)->comlist, MAX_STRING_LENGTH, 0, oldtext);
			OLC_VAL(d) = 1;
		} else
			medit_disp_mprog(d);
		return;
#endif

/*-------------------------------------------------------------------*/

/*
 * Numerical responses.
 */

#if	CONFIG_OASIS_MPROG
	case MEDIT_MPROG_TYPE:
		/*
		 * This calculation may be off by one too many powers of 2?
		 * Someone who actually uses MobProgs will have to check.
		 */
		OLC_MPROG(d)->type = (1 << LIMIT(atoi(arg), 0, NUM_PROGS - 1));
		OLC_VAL(d) = 1;
		medit_change_mprog(d);
		return;

	case MEDIT_MPROG_ARGS:
		OLC_MPROG(d)->arglist = str_dup(arg);
		OLC_VAL(d) = 1;
		medit_change_mprog(d);
		return;
#endif

	case MEDIT_SEX:
		GET_SEX(OLC_MOB(d)) = LIMIT(i, 0, NUM_GENDERS - 1);
		break;

	case MEDIT_RACE:
		race = LIMIT(i, 0, top_of_race_list);
		GET_RACE(OLC_MOB(d)) = race_frames[race].race;
		GET_BODY(OLC_MOB(d)) = race_frames[race].body_bits;
		GET_SIZE(OLC_MOB(d)) = race_list[race].size;
		break;

	case MEDIT_HITROLL:
		GET_HITROLL(OLC_MOB(d)) = LIMIT(i, 0, 50);
		break;

	case MEDIT_DAMROLL:
		GET_DAMROLL(OLC_MOB(d)) = LIMIT(i, 0, 50);
		break;

	case MEDIT_NDD:
		GET_NDD(OLC_MOB(d)) = LIMIT(i, 0, 30);
		break;

	case MEDIT_SDD:
		GET_SDD(OLC_MOB(d)) = LIMIT(i, 0, 127);
		break;

	case MEDIT_NUM_HP_DICE:
		GET_HIT(OLC_MOB(d)) = LIMIT(i, 0, 30);
		break;

	case MEDIT_SIZE_HP_DICE:
		GET_MANA(OLC_MOB(d)) = LIMIT(i, 0, 1000);
		break;

	case MEDIT_ADD_HP:
		GET_MOVE(OLC_MOB(d)) = LIMIT(i, 0, 30000);
		break;

	case MEDIT_AC:
		GET_PD(OLC_MOB(d)) = LIMIT(i, 0, 100);
		break;

	case MEDIT_EXP:
		GET_EXP(OLC_MOB(d)) = MAX(i, 0);
		break;

	case MEDIT_GOLD:
		GET_GOLD(OLC_MOB(d)) = MAX(i, 0);
		break;

	case MEDIT_POS:
		GET_POS(OLC_MOB(d)) = LIMIT(i, 0, NUM_POSITIONS - 1);
		break;

	case MEDIT_DEFAULT_POS:
		GET_DEFAULT_POS(OLC_MOB(d)) = LIMIT(i, 0, NUM_POSITIONS - 1);
		break;

	case MEDIT_ATTACK:
		GET_ATTACK(OLC_MOB(d)) = LIMIT(i, 0, NUM_ATTACK_TYPES - 1);
		break;

	case MEDIT_DIFFICULTY:
		i = LIMIT(i, 0, NUM_MOB_DIFFICULTIES);
		GET_DIFFICULTY(OLC_MOB(d)) = i-1;
		GET_NDD(OLC_MOB(d)) = LIMIT(dice(i,1), 1, 6);
		GET_SDD(OLC_MOB(d)) = LIMIT(dice(i,4), 1, 20);
		GET_HIT(OLC_MOB(d)) = LIMIT(dice(i,8), 1, 30);
		GET_MANA(OLC_MOB(d)) = LIMIT(dice(i,10), 1, 500);
		GET_PD(OLC_MOB(d)) = LIMIT(dice(i*4,5), 0, 100);
		GET_GOLD(OLC_MOB(d)) = MAX(dice(i,50), 0);
		GET_EXP(OLC_MOB(d)) = LIMIT(dice(i*5,40), 1, 1000);
		break;

	case MEDIT_ALIGNMENT:
		GET_ALIGNMENT(OLC_MOB(d)) = LIMIT(i, -1000, 1000);
		break;

/*-------------------------------------------------------------------*/

	case MEDIT_VALUE_1:
		if (real_object(i) < 0) {
			write_to_output(d, TRUE, "\r\n&RNo object exists with that vnum.&n\r\n");
			medit_disp_val1_menu(d);
		} else {
			GET_MOB_VAL(OLC_MOB(d), 0) = i;
			medit_disp_val2_menu(d);
		}
		return;

	case MEDIT_VALUE_2:
		if (i < 0 || i > 10) {
			write_to_output(d, TRUE, "\r\n&RValid ranges are 0-10.&n\r\n");
			medit_disp_val2_menu(d);
		} else {
			GET_MOB_VAL(OLC_MOB(d), 1) = i;
			if (i == 0) {
				write_to_output(d, TRUE, "\r\n");
				medit_disp_menu(d);
			} else {
				medit_disp_val3_menu(d);
			}
		}
		return;

	case MEDIT_VALUE_3:
		if (i < 0 || real_object(i) < 0) {
			write_to_output(d, TRUE, "\r\n&RNo object exists with that vnum.&n\r\n");
			medit_disp_val3_menu(d);
		} else {
			GET_MOB_VAL(OLC_MOB(d), 2) = i;
			if (i == 0) {
				write_to_output(d, TRUE, "\r\n");
				medit_disp_menu(d);
			} else {
				medit_disp_val4_menu(d);
			}
		}
		return;

	case MEDIT_VALUE_4:
		if (i < 0 || real_object(i) < 0) {
			write_to_output(d, TRUE, "\r\n&RNo object exists with that vnum.&n\r\n");
			medit_disp_val4_menu(d);
		} else {
			GET_MOB_VAL(OLC_MOB(d), 3) = i;
			medit_disp_menu(d);
		}
		return;

/*-------------------------------------------------------------------*/
	default:
		/*
		 * We should never get here.
		 */
		cleanup_olc(d, CLEANUP_ALL);
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: medit_parse(): Reached default case!");
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
	medit_disp_menu(d);
}

void medit_string_cleanup(struct descriptor_data *d, int terminator)
{
	switch (OLC_MODE(d)) {

#if	CONFIG_OASIS_MPROG
	case MEDIT_MPROG_COMLIST:
		medit_change_mprog(d);
		break;
#endif

	case MEDIT_D_DESC:
	default:
		 medit_disp_menu(d);
		 break;
	}
}

