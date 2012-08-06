/************************************************************************
 * OasisOLC - Objects / oedit.c                                    v2.0 *
 * Original author: Levork                                              *
 * Copyright 1996 by Harvey Gilpin                                      *
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)          *
 ************************************************************************/
/* $Id: oedit.c,v 1.33 2004/04/20 16:30:13 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "events.h"
#include "skills.h"
#include "spells.h"
#include "utils.h"
#include "db.h"
#include "boards.h"
#include "constants.h"
#include "shop.h"
#include "genolc.h"
#include "genobj.h"
#include "oasis.h"
#include "dg_olc.h"
#include "specset.h"
#include "color.h"

/*------------------------------------------------------------------------*/

char *combat_skills[] = {
	"melee",
	"axe",
	"sword",
	"dagger",
	"mace",
	"bow",
	"polearm",
	"thrown",
	"\n"
};

/*
 * External variable declarations.
 */

extern struct obj_data *obj_proto;
extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern obj_rnum top_of_objt;
extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;
extern struct shop_data *shop_index;
extern struct attack_hit_type attack_hit_text[];
extern struct spell_info_type *spell_info;
extern struct board_info_type board_info[];
extern struct descriptor_data *descriptor_list;

/*------------------------------------------------------------------------*/

/*
 * Handy macros.
 */
#define	S_PRODUCT(s, i) ((s)->producing[(i)])

/*------------------------------------------------------------------------*\
	Utility and exported functions
\*------------------------------------------------------------------------*/

void oedit_setup_new(struct descriptor_data *d)
{
	CREATE(OLC_OBJ(d), struct obj_data, 1);

	clear_object(OLC_OBJ(d));
	OLC_OBJ(d)->name = str_dup("unfinished object");
	OLC_OBJ(d)->description = str_dup("An unfinished object is lying here.");
	OLC_OBJ(d)->short_description = str_dup("an unfinished object");
	GET_OBJ_WEAR(OLC_OBJ(d)) = ITEM_WEAR_TAKE;
	OLC_VAL(d) = 0;
	OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
	oedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void oedit_setup_existing(struct descriptor_data *d, int real_num)
{
	struct obj_data *obj;

	/*
	 * Allocate object in memory.
	 */
	CREATE(obj, struct obj_data, 1);
	copy_object(obj, &obj_proto[real_num]);

	if (SCRIPT(obj))
		script_copy(obj, &obj_proto[real_num], OBJ_TRIGGER);

	/*
	 * Attach new object to player's descriptor.
	 */
	OLC_OBJ(d) = obj;
	OLC_VAL(d) = 0;
	OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
	dg_olc_script_copy(d);
	oedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void oedit_save_internally(struct descriptor_data *d)
{
	int i;
	obj_rnum robj_num;
	struct descriptor_data *dsc;
	struct obj_data *obj;

	i = (real_object(OLC_NUM(d)) == NOTHING);

	if ((robj_num = add_object(OLC_OBJ(d), OLC_NUM(d))) == NOWHERE) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "oedit_save_internally: add_object failed.");
		return;
	}

	/* Make sure scripts are updated too. - Welcor */
	/* Free old proto list  */
	if (obj_proto[robj_num].proto_script &&
			obj_proto[robj_num].proto_script != OLC_SCRIPT(d)) {
		struct trig_proto_list *proto, *fproto;
		proto = obj_proto[robj_num].proto_script;
		while (proto) {
			fproto = proto;
			proto = proto->next;
			free(fproto);
		}
	}    

	/* this will handle new instances of the object: */
	 obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
 
	/* this takes care of the objects currently in-game */
	for (obj = object_list; obj; obj = obj->next) {
		if (obj->item_number != robj_num)
			continue;
		/* remove any old scripts */
		if (SCRIPT(obj)) {
			extract_script(SCRIPT(obj));
			SCRIPT(obj) = NULL;
		}
		obj->proto_script = OLC_SCRIPT(d);
		assign_triggers(obj, OBJ_TRIGGER);
	}

	if (!i)        /* If it's not a new object, don't renumber. */
		return;

	/*
	 * Renumber produce in shops being edited.
	 */
	for (dsc = descriptor_list; dsc; dsc = dsc->next)
		if (STATE(dsc) == CON_SEDIT)
			for (i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != -1; i++)
				if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
					S_PRODUCT(OLC_SHOP(dsc), i)++;


	/* Update other people in zedit too. From: C.Raehl 4/27/99 */
	for (dsc = descriptor_list; dsc; dsc = dsc->next)
		if (STATE(dsc) == CON_ZEDIT)
			for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
				switch (OLC_ZONE(dsc)->cmd[i].command) {
					case 'P':
						OLC_ZONE(dsc)->cmd[i].arg3 += (OLC_ZONE(dsc)->cmd[i].arg3 >= robj_num);
						/* Fall through. */
					case 'E':
					case 'G':
					case 'O':
						OLC_ZONE(dsc)->cmd[i].arg1 += (OLC_ZONE(dsc)->cmd[i].arg1 >= robj_num);
						break;
					case 'R':
						OLC_ZONE(dsc)->cmd[i].arg2 += (OLC_ZONE(dsc)->cmd[i].arg2 >= robj_num);
						break;
					default:
					break;
				}
}

/*------------------------------------------------------------------------*/

void oedit_save_to_disk(int zone_num)
{
	save_objects(zone_num);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * Display a list of Special Procedures
 */
void oedit_disp_specproc(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i < NUM_OBJ_SPECS; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s  %s", grn, i, nrm, obj_specproc_info[i].name,
												!(++columns % 2) ? "\r\n" : "");

	write_to_output(d, TRUE, "\r\nEnter Spec Proc number : ");
}

/*
 * For container flags.
 */
void oedit_disp_container_flags_menu(struct descriptor_data *d)
{
	get_char_colors(d->character);
	clear_screen(d);

	sprintbit(GET_OBJ_VAL(OLC_OBJ(d), 1), container_bits, buf1, sizeof(buf1));
	write_to_output(d, TRUE,
					"%s1%s) CLOSEABLE\r\n"
					"%s2%s) PICKPROOF\r\n"
					"%s3%s) CLOSED\r\n"
					"%s4%s) LOCKED\r\n"
					"Container flags: %s%s%s\r\n"
					"Enter flag, 0 to quit : ",
					grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn, buf1, nrm);
}

/*
 * For extra descriptions.
 */
void oedit_disp_extradesc_menu(struct descriptor_data *d)
{
	struct extra_descr_data *extra_desc = OLC_DESC(d);

	strcpy(buf1, !extra_desc->next ? "<Not set>\r\n" : "Set.");

	get_char_colors(d->character);
	clear_screen(d);
	write_to_output(d, TRUE,
					"Extra desc menu\r\n"
					"%s1%s) Keyword: %s%s\r\n"
					"%s2%s) Description:\r\n%s\\c98%s\\c99\r\n"
					"%s3%s) Goto next description: %s\r\n"
					"%s0%s) Quit\r\n"
					"Enter choice : ",

							 grn, nrm, yel, (extra_desc->keyword && *extra_desc->keyword) ? extra_desc->keyword : "<NONE>",
					grn, nrm, yel, (extra_desc->description && *extra_desc->description) ? extra_desc->description : "<NONE>",
					grn, nrm, buf1, grn, nrm);
	OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

/*
 * Ask for *which* apply to edit.
 */
void oedit_disp_prompt_apply_menu(struct descriptor_data *d)
{
	int counter;

	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
		if (OLC_OBJ(d)->affected[counter].modifier) {
			sprinttype(OLC_OBJ(d)->affected[counter].location, apply_types, buf2, sizeof(buf2));
			write_to_output(d, TRUE, " %s%d%s) %+d to %s\r\n", grn, counter + 1, nrm,
							OLC_OBJ(d)->affected[counter].modifier, buf2);
		} else {
			write_to_output(d, TRUE, " %s%d%s) None.\r\n", grn, counter + 1, nrm);
		}
	}
	write_to_output(d, TRUE, "\r\nEnter affection to modify (0 to quit) : ");
	OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

/*
 * Ask for liquid type.
 */
void oedit_liquid_type(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; counter < NUM_LIQ_TYPES; counter++)
		write_to_output(d, TRUE, " %s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
						drinks[counter], !(++columns % 2) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\n%sEnter drink type : ", nrm);
	OLC_MODE(d) = OEDIT_VALUE_3;
}

/*
 * The actual apply to set.
 */
void oedit_disp_apply_menu(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; counter < NUM_APPLIES; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
								apply_types[counter], !(++columns % 2) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\nEnter apply type (0 is no apply) : ");
	OLC_MODE(d) = OEDIT_APPLY;
}

/*
 * Weapon type.
 */
void oedit_disp_weapon_menu(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; counter < NUM_ATTACK_TYPES; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
								attack_hit_text[counter].singular,
								!(++columns % 2) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\nEnter weapon type : ");
}

/*
 * Weapon skill.
 */
void oedit_disp_weapon_skills(struct descriptor_data *d)
{
	int counter, columns = 0;
	
	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; *combat_skills[counter] != '\n'; counter++)
		if (find_skill_num(combat_skills[counter]) > 0)
			write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
									combat_skills[counter],
									!(++columns % 2) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\nEnter weapon skill : ");
}

/*
 * Spell type.
 */
void oedit_disp_spells_menu(struct descriptor_data *d)
{
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	struct spell_info_type *sptr;
	int counter = 1, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (sptr = spell_info; sptr; sptr = sptr->next) {
		if (sptr->number == 0)
			continue;
		write_to_output(d, TRUE, "%s%s%3d%s) %s%-20.20s %s", printbuf, grn, counter++, nrm, yel,
								sptr->name, !(++columns % 3) ? "\r\n" : "");
	}
	write_to_output(d, TRUE, "\r\n%sEnter spell choice (-1 for none) : ", nrm);
	release_buffer(printbuf);
}

/*
 * Resource.
 */
void oedit_disp_resources(struct descriptor_data *d)
{
	int i, columns = 0;
	
	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i <= NUM_RESOURCES; i++)
		if (i == 0)
			write_to_output(d, TRUE, "&g%2d&n) %-15.15s %s", i, "no resource", !(++columns % 3) ? "\r\n" : "");
		else
			write_to_output(d, TRUE, "&g%2d&n) %-15.15s %s", i, resource_names[i], !(++columns % 3) ? "\r\n" : "");
	write_to_output(d, TRUE, "%sEnter resource number : ", (++columns % 3) ? "\r\n" : "");
}

/*
 * Resource category.
 */
void oedit_disp_resource_categories(struct descriptor_data *d)
{
	int counter, columns = 0;
	
	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; *resource_groups[counter] != '\n'; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
								resource_groups[counter],
								!(++columns % 2) && *resource_groups[counter + 1] != '\n' ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\nResource group : ");
}

/*
 * Display intended size for race using the object.
 */
void oedit_disp_sizes(struct descriptor_data *d)
{
	int i;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i < NUM_SIZES; i++)
		write_to_output(d, TRUE, "%s%2d%s) %s\r\n", grn, i, nrm, race_size[i]);
	write_to_output(d, TRUE, "Enter size number : ");
}

/*
 * Display default color for use with this object, if any.
 */
void oedit_disp_colors(struct descriptor_data *d)
{
	int i, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i <= NUM_EXT_COLORS; i++)
		if (i == 0)
			write_to_output(d, TRUE, "&g%2d&n) &n%-15.15s %s", i, "no color", !(++columns % 3) ? "\r\n" : "");
		else if (strlen(EXTENDED_COLORS[i].name) > 1)
			write_to_output(d, TRUE, "&g%2d&n) %s%-15.15s&n %s", i, number_to_color(EXTENDED_COLORS[i].color), EXTENDED_COLORS[i].name, !(++columns % 3) ? "\r\n" : "");
	write_to_output(d, TRUE, "Enter color number : ");
}

/*
 * Object value #1
 */
void oedit_disp_val1_menu(struct descriptor_data *d)
{
	OLC_MODE(d) = OEDIT_VALUE_1;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
	case ITEM_LIGHT:
		/*
		 * values 0 and 1 are unused.. jump to 2 
		 */
		oedit_disp_val3_menu(d);
		break;
	case ITEM_SCROLL:
	case ITEM_WAND:
	case ITEM_STAFF:
	case ITEM_POTION:
		write_to_output(d, TRUE, "Spell level : ");
		break;
	case ITEM_SPELLBOOK:
		write_to_output(d, TRUE, "% learned : ");
		break;
	case ITEM_WEAPON:
	case ITEM_SHEATH:
		oedit_disp_weapon_skills(d);
		break;
	case ITEM_ARMOR:
		write_to_output(d, TRUE, "Passive Defense : ");
		break;
	case ITEM_CONTAINER:
		write_to_output(d, TRUE, "Max weight to contain : ");
		break;
	case ITEM_DRINKCON:
	case ITEM_FOUNTAIN:
		write_to_output(d, TRUE, "Max drink units : ");
		break;
	case ITEM_FOOD:
		write_to_output(d, TRUE, "Hours to fill stomach : ");
		break;
	case ITEM_MONEY:
		write_to_output(d, TRUE, "Number of gold coins : ");
		break;
	case ITEM_NOTE:
		/*
		 * This is supposed to be language, but it's unused.
		 */
		break;
	case ITEM_RESOURCE:
	case ITEM_TOOL:
		oedit_disp_resource_categories(d);
		break;
	case ITEM_PORTAL:
		oedit_disp_val2_menu(d);
		break;
	default:
		oedit_disp_menu(d);
	}
}

/*
 * Object value #2
 */
void oedit_disp_val2_menu(struct descriptor_data *d)
{
	OLC_VAL(d) = 1; /* If we get here, we changed something */
	OLC_MODE(d) = OEDIT_VALUE_2;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
	case ITEM_SCROLL:
	case ITEM_POTION:
	case ITEM_SPELLBOOK:
		oedit_disp_spells_menu(d);
		break;
	case ITEM_WAND:
	case ITEM_STAFF:
		write_to_output(d, TRUE, "Max number of charges : ");
		break;
	case ITEM_WEAPON:
		write_to_output(d, TRUE, "Number of damage dice : ");
		break;
	case ITEM_ARMOR:
		write_to_output(d, TRUE, "Damage reduction : ");
		break;
	case ITEM_FOOD:
		/*
		 * Values 2 and 3 are unused, jump to 4...Odd.
		 */
		oedit_disp_val4_menu(d);
		break;
	case ITEM_CONTAINER:
		/*
		 * These are flags, needs a bit of special handling.
		 */
		oedit_disp_container_flags_menu(d);
		break;
	case ITEM_DRINKCON:
	case ITEM_FOUNTAIN:
		write_to_output(d, TRUE, "Initial drink units : ");
		break;
	case ITEM_PORTAL:
		write_to_output(d, TRUE, "Room vnum : ");
		break;
	default:
		oedit_disp_menu(d);
	}
}

/*
 * Object value #3
 */
void oedit_disp_val3_menu(struct descriptor_data *d)
{
	OLC_MODE(d) = OEDIT_VALUE_3;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
	case ITEM_LIGHT:
		write_to_output(d, TRUE, "Number of hours (0 = burnt, -1 is infinite) : ");
		break;
	case ITEM_SCROLL:
	case ITEM_POTION:
	case ITEM_SPELLBOOK:
		oedit_disp_spells_menu(d);
		break;
	case ITEM_WAND:
	case ITEM_STAFF:
		write_to_output(d, TRUE, "Number of charges remaining : ");
		break;
	case ITEM_WEAPON:
		write_to_output(d, TRUE, "Size of damage dice : ");
		break;
	case ITEM_CONTAINER:
		write_to_output(d, TRUE, "Vnum of key to open container (-1 for no key) : ");
		break;
	case ITEM_DRINKCON:
	case ITEM_FOUNTAIN:
		oedit_liquid_type(d);
		break;
	default:
		oedit_disp_menu(d);
	}
}

/*
 * Object value #4
 */
void oedit_disp_val4_menu(struct descriptor_data *d)
{
	OLC_MODE(d) = OEDIT_VALUE_4;
	switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
	case ITEM_SCROLL:
	case ITEM_POTION:
	case ITEM_WAND:
	case ITEM_STAFF:
	case ITEM_SPELLBOOK:
		oedit_disp_spells_menu(d);
		break;
	case ITEM_WEAPON:
		oedit_disp_weapon_menu(d);
		break;
	case ITEM_DRINKCON:
	case ITEM_FOUNTAIN:
	case ITEM_FOOD:
		write_to_output(d, TRUE, "Poisoned (0 = not poison) : ");
		break;
	default:
		oedit_disp_menu(d);
	}
}

/*
 * Object type.
 */
void oedit_disp_type_menu(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; counter < NUM_ITEM_TYPES; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
								item_types[counter], !(++columns % 2) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\nEnter object type : ");
}

/*
 * Object extra flags.
 */
void oedit_disp_extra_menu(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; counter < NUM_ITEM_FLAGS; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
								extra_bits[counter], !(++columns % 2) ? "\r\n" : "");
	sprintbit(GET_OBJ_EXTRA(OLC_OBJ(d)), extra_bits, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nObject flags: %s%s%s\r\n"
					"Enter object extra flag (0 to quit) : ",
					cyn, buf1, nrm);
}

/*
 * Object perm flags.
 */
void oedit_disp_perm_menu(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; counter < NUM_AFF_FLAGS; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm, affected_bits[counter], !(++columns % 2) ? "\r\n" : "");
	sprintbit(GET_OBJ_PERM(OLC_OBJ(d)), affected_bits, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nObject permanent flags: %s%s%s\r\n"
					"Enter object perm flag (0 to quit) : ", cyn, buf1, nrm);
}

/*
 * Object wear flags.
 */
void oedit_disp_wear_menu(struct descriptor_data *d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; counter < NUM_ITEM_WEARS; counter++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
								wear_bits[counter], !(++columns % 2) ? "\r\n" : "");
	sprintbit(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nWear flags: %s%s%s\r\n"
					"Enter wear flag, 0 to quit : ", cyn, buf1, nrm);
}

/*
 * Display main menu.
 */
void oedit_disp_menu(struct descriptor_data *d)
{
	struct obj_data *obj;

	obj = OLC_OBJ(d);
	get_char_colors(d->character);
	clear_screen(d);

	/*
	 * Build buffers for first part of menu.
	 */
	sprinttype(GET_OBJ_TYPE(obj), item_types, buf1, sizeof(buf1));
	sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf2, sizeof(buf2));

	write_to_output(d, TRUE,
					"-- Item number : [%s%d%s]\r\n"
					"%s1%s) Keywords : %s%s\r\n"
					"%s2%s) S-Desc   : %s\\c98%s\\c99\r\n"
					"%s3%s) L-Desc   :-\r\n%s\\c98%s\\c99\r\n"
					"%s4%s) A-Desc   :-\r\n%s%s"
					"%s5%s) Type          : %s%s\r\n"
					"%s6%s) Extra flags   : %s%s\r\n",

					cyn, OLC_NUM(d), nrm,
					grn, nrm, yel, (obj->name && *obj->name) ? obj->name : "undefined",
					grn, nrm, yel, (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
					grn, nrm, yel, (obj->description && *obj->description) ? obj->description : "undefined",
					grn, nrm, yel, (obj->action_description && *obj->action_description) ? obj->action_description : "<not set>\r\n",
					grn, nrm, cyn, buf1,
					grn, nrm, cyn, buf2
					);

	sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf1, sizeof(buf1));
	sprintbit(GET_OBJ_PERM(obj), affected_bits, buf2, sizeof(buf2));

	write_to_output(d, TRUE,
					"%s7%s) Wear flags    : %s%s\r\n"
					"%s8%s) Weight        : %s%d\r\n"
					"%s9%s) Cost          : %s%d\r\n"
					"%sA%s) Cost/Day      : %s%d\r\n"
					"%sB%s) Timer         : %s%d\r\n"
					"%sC%s) Values        : %s%d %d %d %d\r\n"
					"%sD%s) Applies menu\r\n"
					"%sE%s) Extra descriptions menu\r\n"
					"%sP%s) Perm Affects  : %s%s\r\n"
					"%sS%s) Script        : %s%s\r\n"
					"%sZ%s) Intended size : %s%s\r\n"
					"%sR%s) Resource used : %s\r\n"
					"%sX%s) Default color : %s%s\r\n"
					"%sQ%s) Quit\r\n"
					"Enter choice : ",

					grn, nrm, cyn, buf1,
					grn, nrm, cyn, GET_OBJ_WEIGHT(obj),
					grn, nrm, cyn, GET_OBJ_COST(obj),
					grn, nrm, cyn, GET_OBJ_RENT(obj),
					grn, nrm, cyn, GET_OBJ_TIMER(obj),
					grn, nrm, cyn, GET_OBJ_VAL(obj, 0),
					GET_OBJ_VAL(obj, 1),
					GET_OBJ_VAL(obj, 2),
					GET_OBJ_VAL(obj, 3),
					grn, nrm, grn, nrm,
					grn, nrm, cyn, buf2,
					grn, nrm, cyn, obj->proto_script?"Set.":"Not Set.",
					grn, nrm, cyn, race_size[(int)GET_OBJ_SIZE(obj) + 1],
					grn, nrm, GET_OBJ_RESOURCE(obj) == 0 ? "<no resource set>" : resource_names[(int)GET_OBJ_RESOURCE(obj)],
					grn, nrm, GET_OBJ_COLOR(obj) == 0 ? "&n" : number_to_color(EXTENDED_COLORS[(int)GET_OBJ_COLOR(obj)].color), GET_OBJ_COLOR(obj) == 0 ? "<no color set>" : EXTENDED_COLORS[(int)GET_OBJ_COLOR(obj)].name,
					grn, nrm
	);
	OLC_MODE(d) = OEDIT_MAIN_MENU;
}

/***************************************************************************
 main loop (of sorts).. basically interpreter throws all input to here
 ***************************************************************************/

void oedit_parse(struct descriptor_data *d, char *arg)
{
	int number, max_val, min_val, i = 0;

	switch (OLC_MODE(d)) {

	case OEDIT_CONFIRM_SAVESTRING:
		switch (*arg) {
		case 'y':
		case 'Y':
			write_to_output(d, TRUE, "Saving object to disk and memory.\r\n");
			oedit_save_internally(d);
			oedit_save_to_disk(OLC_ZNUM(d));
			extended_mudlog(BRF, SYSL_OLC, TRUE, "%s edits obj %d", GET_NAME(d->character), OLC_NUM(d));
			/* Fall through. */
		case 'n':
		case 'N':
			cleanup_olc(d, CLEANUP_ALL);
			return;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\n");
			write_to_output(d, TRUE, "Do you wish to save this object?\r\n");
			return;
		}

	case OEDIT_MAIN_MENU:
		/*
		 * Throw us out to whichever edit mode based on user input.
		 */
		switch (*arg) {
		case 'q':
		case 'Q':
			if (OLC_VAL(d)) {        /* Something has been modified. */
				write_to_output(d, TRUE, "Do you wish to save this object? : ");
				OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
			} else
				cleanup_olc(d, CLEANUP_ALL);
			return;
		case '1':
			write_to_output(d, TRUE, "Enter keywords : ");
			OLC_MODE(d) = OEDIT_EDIT_NAMELIST;
			break;
		case '2':
			write_to_output(d, TRUE, "Enter short desc : ");
			OLC_MODE(d) = OEDIT_SHORTDESC;
			break;
		case '3':
			write_to_output(d, TRUE, "Enter long desc :-\r\n| ");
			OLC_MODE(d) = OEDIT_LONGDESC;
			break;
		case '4':
			OLC_MODE(d) = OEDIT_ACTDESC;
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_OBJ(d)->action_description)
				write_to_output(d, FALSE, "%s", OLC_OBJ(d)->action_description);
			string_write(d, &OLC_OBJ(d)->action_description, MAX_MESSAGE_LENGTH, 0, STATE(d));
			OLC_VAL(d) = 1;
			break;
		case '5':
			oedit_disp_type_menu(d);
			OLC_MODE(d) = OEDIT_TYPE;
			break;
		case '6':
			oedit_disp_extra_menu(d);
			OLC_MODE(d) = OEDIT_EXTRAS;
			break;
		case '7':
			oedit_disp_wear_menu(d);
			OLC_MODE(d) = OEDIT_WEAR;
			break;
		case '8':
			write_to_output(d, TRUE, "Enter weight : ");
			OLC_MODE(d) = OEDIT_WEIGHT;
			break;
		case '9':
			write_to_output(d, TRUE, "Enter cost : ");
			OLC_MODE(d) = OEDIT_COST;
			break;
		case 'a':
		case 'A':
			write_to_output(d, TRUE, "Enter cost per day : ");
			OLC_MODE(d) = OEDIT_COSTPERDAY;
			break;
		case 'b':
		case 'B':
			write_to_output(d, TRUE, "Enter timer : ");
			OLC_MODE(d) = OEDIT_TIMER;
			break;
		case 'c':
		case 'C':
			/*
			 * Clear any old values  
			 */
			GET_OBJ_VAL(OLC_OBJ(d), 0) = 0;
			GET_OBJ_VAL(OLC_OBJ(d), 1) = 0;
			GET_OBJ_VAL(OLC_OBJ(d), 2) = 0;
			GET_OBJ_VAL(OLC_OBJ(d), 3) = 0;
			oedit_disp_val1_menu(d);
			break;
		case 'd':
		case 'D':
			oedit_disp_prompt_apply_menu(d);
			break;
		case 'e':
		case 'E':
			/*
			 * If extra descriptions don't exist.
			 */
			if (OLC_OBJ(d)->ex_description == NULL) {
				CREATE(OLC_OBJ(d)->ex_description, struct extra_descr_data, 1);
				OLC_OBJ(d)->ex_description->next = NULL;
			}
			OLC_DESC(d) = OLC_OBJ(d)->ex_description;
			oedit_disp_extradesc_menu(d);
			break;
		case 'p':
		case 'P':
			oedit_disp_perm_menu(d);
			OLC_MODE(d) = OEDIT_PERM;
			break;
		case 's':
		case 'S':
			if (GOT_RIGHTS(d->character, RIGHTS_TRIGGERS)) {
				OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
				dg_script_menu(d);
				return;
			} else {
				write_to_output(d, TRUE, "\r\n&RYou do not have trigger editing rights.&n\r\n\r\n");
				oedit_disp_menu(d);
				return;
			}
		case 'r':
		case 'R':
			oedit_disp_resources(d);
			OLC_MODE(d) = OEDIT_RESOURCE;
			break;
		case 'x':
		case 'X':
			oedit_disp_colors(d);
			OLC_MODE(d) = OEDIT_COLOR;
			break;
		case 'z':
		case 'Z':
			oedit_disp_sizes(d);
			OLC_MODE(d) = OEDIT_SIZE;
			break;
		default:
			write_to_output(d, TRUE, "\r\n&RInvalid choice!&n\r\n\r\n");
			oedit_disp_menu(d);
			break;
		}
		return;                        /*
																 * end of OEDIT_MAIN_MENU 
																 */

	case OLC_SCRIPT_EDIT:
		if (dg_script_edit_parse(d, arg)) return;
		break;

	case OEDIT_EDIT_NAMELIST:
		if (!genolc_checkstring(d, arg))
			break;
		if (OLC_OBJ(d)->name)
			free(OLC_OBJ(d)->name);
		OLC_OBJ(d)->name = str_udup(arg);
		break;

	case OEDIT_SHORTDESC:
		if (!genolc_checkstring(d, arg))
			break;
		if (OLC_OBJ(d)->short_description)
			free(OLC_OBJ(d)->short_description);
		OLC_OBJ(d)->short_description = str_udup(arg);
		break;

	case OEDIT_LONGDESC:
		if (!genolc_checkstring(d, arg))
			break;
		if (OLC_OBJ(d)->description)
			free(OLC_OBJ(d)->description);
		OLC_OBJ(d)->description = str_udup(arg);
		break;

	case OEDIT_TYPE:
		number = atoi(arg);
		if ((number < 1) || (number >= NUM_ITEM_TYPES)) {
			write_to_output(d, TRUE, "Invalid choice, try again : ");
			return;
		} else
			GET_OBJ_TYPE(OLC_OBJ(d)) = number;
		break;

	case OEDIT_EXTRAS:
		number = atoi(arg);
		if ((number < 0) || (number > NUM_ITEM_FLAGS)) {
			oedit_disp_extra_menu(d);
			return;
		} else if (number == 0)
			break;
		else {
			TOGGLE_BIT(GET_OBJ_EXTRA(OLC_OBJ(d)), 1ULL << (number - 1));
			oedit_disp_extra_menu(d);
			return;
		}

	case OEDIT_WEAR:
		number = atoi(arg);
		if ((number < 0) || (number > NUM_ITEM_WEARS)) {
			write_to_output(d, TRUE, "That's not a valid choice!\r\n");
			oedit_disp_wear_menu(d);
			return;
		} else if (number == 0)        /* Quit. */
			break;
		else {
			TOGGLE_BIT(GET_OBJ_WEAR(OLC_OBJ(d)), 1ULL << (number - 1));
			oedit_disp_wear_menu(d);
			return;
		}

	case OEDIT_WEIGHT:
		GET_OBJ_WEIGHT(OLC_OBJ(d)) = atoi(arg);
		break;

	case OEDIT_COST:
		GET_OBJ_COST(OLC_OBJ(d)) = atoi(arg);
		break;

	case OEDIT_COSTPERDAY:
		GET_OBJ_RENT(OLC_OBJ(d)) = atoi(arg);
		break;

	case OEDIT_TIMER:
		GET_OBJ_TIMER(OLC_OBJ(d)) = atoi(arg);
		break;

	case OEDIT_PERM:
		if ((number = atoi(arg)) == 0)
			break;
		if (number > 0 && number <= NUM_AFF_FLAGS)
			TOGGLE_BIT(GET_OBJ_PERM(OLC_OBJ(d)), 1ULL << (number - 1));
		oedit_disp_perm_menu(d);
		return;
		
	case OEDIT_VALUE_1:
		/*
		 * Here, I do need to check for out of range values on weapons.
		 */
		number = atoi(arg);
		switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_WEAPON:
		case ITEM_SHEATH:
			for (i = 0; *combat_skills[i] != '\n'; i++)
				/* the loop does it */ ;
			if (number > i || number < 1)
				oedit_disp_val1_menu(d);
			else {
				GET_OBJ_VAL(OLC_OBJ(d), 0) = find_skill_num(combat_skills[number - 1]);
				oedit_disp_val2_menu(d);
			}
			break;
		case ITEM_TOOL:
			if (number < 0 || number > NUM_RESOURCE_GROUPS)
				oedit_disp_val1_menu(d);
			else {
				GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
				oedit_disp_val2_menu(d);
			}
			break;
		default:
			GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
			oedit_disp_val2_menu(d);
		}
		return;
	case OEDIT_VALUE_2:
		/*
		 * Here, I do need to check for out of range values.
		 */
		number = atoi(arg);
		switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_SCROLL:
		case ITEM_POTION:
			if (number < 0 || number >= NUM_SPELLS)
				oedit_disp_val2_menu(d);
			else {
				GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
				oedit_disp_val3_menu(d);
			}
			break;
		case ITEM_CONTAINER:
			/*
			 * Needs some special handling since we are dealing with flag values
			 * here.
			 */
			if (number < 0 || number > 4)
				oedit_disp_container_flags_menu(d);
			else if (number != 0) {
				TOGGLE_BIT(GET_OBJ_VAL(OLC_OBJ(d), 1), 1ULL << (number - 1));
				OLC_VAL(d) = 1;
				oedit_disp_val2_menu(d);
			} else
				oedit_disp_val3_menu(d);
			break;

		case ITEM_PORTAL:
			if (number < 0 || number > 32000) {
				oedit_disp_val2_menu(d);
			} else {
				GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
				oedit_disp_val3_menu(d);
			}
			break;

		default:
			GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
			oedit_disp_val3_menu(d);
		}
		return;

	case OEDIT_VALUE_3:
		number = atoi(arg);
		/*
		 * Quick'n'easy error checking.
		 */
		switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_SCROLL:
			min_val = -1;
			max_val = NUM_SPELLS - 1;
			break;
		case ITEM_POTION:
			min_val = 0;
			max_val = NUM_SPELLS - 1;
			break;
		case ITEM_WEAPON:
			min_val = 1;
			max_val = 50;
			break;
		case ITEM_WAND:
		case ITEM_STAFF:
			min_val = 0;
			max_val = 20;
			break;
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:
			min_val = 0;
			max_val = NUM_LIQ_TYPES - 1;
			break;
		default:
			min_val = -32000;
			max_val = 32000;
		}
		GET_OBJ_VAL(OLC_OBJ(d), 2) = LIMIT(number, min_val, max_val);
		oedit_disp_val4_menu(d);
		return;

	case OEDIT_VALUE_4:
		number = atoi(arg);
		switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
		case ITEM_SCROLL:
			min_val = -1;
			max_val = NUM_SPELLS - 1;
			break;
		case ITEM_POTION:
			min_val = 0;
			max_val = NUM_SPELLS - 1;
			break;
		case ITEM_WAND:
		case ITEM_STAFF:
			min_val = 1;
			max_val = NUM_SPELLS - 1;
			break;
		case ITEM_WEAPON:
			min_val = 0;
			max_val = NUM_ATTACK_TYPES - 1;
			break;
		default:
			min_val = -32000;
			max_val = 32000;
			break;
		}
		GET_OBJ_VAL(OLC_OBJ(d), 3) = LIMIT(number, min_val, max_val);
		break;

	case OEDIT_PROMPT_APPLY:
		if ((number = atoi(arg)) == 0)
			break;
		else if (number < 0 || number > MAX_OBJ_AFFECT) {
			oedit_disp_prompt_apply_menu(d);
			return;
		}
		OLC_VAL(d) = number - 1;
		OLC_MODE(d) = OEDIT_APPLY;
		oedit_disp_apply_menu(d);
		return;

	case OEDIT_APPLY:
		if ((number = atoi(arg)) == 0) {
			OLC_OBJ(d)->affected[OLC_VAL(d)].location = 0;
			OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = 0;
			oedit_disp_prompt_apply_menu(d);
		} else if (number < 0 || number >= NUM_APPLIES)
			oedit_disp_apply_menu(d);
		else {
			OLC_OBJ(d)->affected[OLC_VAL(d)].location = number;
			write_to_output(d, TRUE, "Modifier : ");
			OLC_MODE(d) = OEDIT_APPLYMOD;
		}
		return;

	case OEDIT_SIZE:
		number = LIMIT(atoi(arg), 0, NUM_SIZES);
		GET_OBJ_SIZE(OLC_OBJ(d)) = number - 1;
		break;

	case OEDIT_COLOR:
		number = LIMIT(atoi(arg), 0, NUM_EXT_COLORS);
		GET_OBJ_COLOR(OLC_OBJ(d)) = number;
		break;

	case OEDIT_RESOURCE:
		number = LIMIT(atoi(arg), 0, NUM_RESOURCES);
		GET_OBJ_RESOURCE(OLC_OBJ(d)) = number;
		break;

	case OEDIT_APPLYMOD:
		OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = atoi(arg);
		oedit_disp_prompt_apply_menu(d);
		return;

	case OEDIT_EXTRADESC_KEY:
		if (genolc_checkstring(d, arg)) {
			if (OLC_DESC(d)->keyword)
				free(OLC_DESC(d)->keyword);
			OLC_DESC(d)->keyword = str_udup(arg);
		}
		oedit_disp_extradesc_menu(d);
		return;

	case OEDIT_EXTRADESC_MENU:
		switch ((number = atoi(arg))) {
		case 0:
			if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
				struct extra_descr_data **tmp_desc;

				if (OLC_DESC(d)->keyword)
					free(OLC_DESC(d)->keyword);
				if (OLC_DESC(d)->description)
					free(OLC_DESC(d)->description);

				/*
				 * Clean up pointers  
				 */
				for (tmp_desc = &(OLC_OBJ(d)->ex_description); *tmp_desc; tmp_desc = &((*tmp_desc)->next)) {
					if (*tmp_desc == OLC_DESC(d)) {
						*tmp_desc = NULL;
						break;
					}
				}
				free(OLC_DESC(d));
			}
		break;

		case 1:
			OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
			write_to_output(d, TRUE, "Enter keywords, separated by spaces :-\r\n| ");
			return;

		case 2:
			OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_DESC(d)->description)
				write_to_output(d, FALSE, "%s", OLC_DESC(d)->description);
			string_write(d, &OLC_DESC(d)->description, MAX_MESSAGE_LENGTH, 0, STATE(d));
			OLC_VAL(d) = 1;
			return;

		case 3:
			/*
			 * Only go to the next description if this one is finished.
			 */
			if (OLC_DESC(d)->keyword && OLC_DESC(d)->description) {
				struct extra_descr_data *new_extra;

				if (OLC_DESC(d)->next)
					OLC_DESC(d) = OLC_DESC(d)->next;
				else {        /* Make new extra description and attach at end. */
					CREATE(new_extra, struct extra_descr_data, 1);
					OLC_DESC(d)->next = new_extra;
					OLC_DESC(d) = OLC_DESC(d)->next;
				}
			}
			/*
			 * No break - drop into default case.
			 */
		default:
			oedit_disp_extradesc_menu(d);
			return;
		}
		break;
	default:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: Reached default case in oedit_parse()!");
		write_to_output(d, TRUE, "Oops...\r\n");
		break;
	}

	/*
	 * If we get here, we have changed something.  
	 */
	OLC_VAL(d) = 1;
	oedit_disp_menu(d);
}

void oedit_string_cleanup(struct descriptor_data *d, int terminator)
{
	switch (OLC_MODE(d)) {
	case OEDIT_ACTDESC:
		oedit_disp_menu(d);
		break;
	case OEDIT_EXTRADESC_DESCRIPTION:
		oedit_disp_extradesc_menu(d);
		break;
	}
}
