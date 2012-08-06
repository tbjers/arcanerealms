/* ************************************************************************
*		File: act.create.c                                Part of CircleMUD   *
*	 Usage: Player-rights object creation stuff                              *
*																																					*
*	 All rights reserved.	 See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: act.create.c,v 1.15 2002/12/10 13:08:29 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

/* #include <sys/stat.h> */

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "events.h"
#include "spells.h"
#include "skills.h"

/* struct for syls */
struct syllable
{
	char *org;
	char *news;
};

/* extern variables */
extern struct spell_info_type *spell_info;
extern struct syllable syls[];

/* extern procedures */
int	mag_manacost(struct char_data *ch, struct spell_info_type *sptr);

/* local functions */
char *get_spell_name(char *argument);
void make_potion(struct char_data *ch, int potion, struct obj_data *container);
void make_scroll(struct char_data *ch, int scroll, struct obj_data *paper);
ACMD(do_brew);
ACMD(do_forge);
ACMD(do_scribe);


char *get_spell_name(char *argument)
{
	char *s;

	s = strtok(argument, "'");
	s = strtok(NULL, "'");

	return s;
}

const char *potion_names[] =
{
	"milky white", /* 0 */
	"bubbling white", /* 1 */
	"crimson red",
	"flickering red",
	"glowing green",
	"glowing ivory", /* 5 */
	"glowing yellow",
	"glowing brown",
	"flickering lapis lazuli",
	"glowing red",
	"sparkling white", /* 10 */
	"swirling grain",
	"light orange",
	"bubbling purple",
	"incandescent blue",
	"cloudy blue", /* 15 */
	"blood red",
	"sparkling magenta",
	"glowing red",
	"swirling charcoal",
	"pulsating red", /* 20 */
	"cloudy green",
	"gritty charcoal",
	"swirling silver",
	"gritty yellow",
	"teal blue"  /* 25 */
};


void make_potion(struct char_data *ch, int potion, struct obj_data *container)
{
	char *printbuf, *printbuf2;
	struct obj_data *final_potion;
	struct extra_descr_data *new_descr;
	int can_make = TRUE, mana, dam, num = 0;
  struct spell_info_type *sptr;

	if (!(sptr = get_spell(potion, __FILE__, __FUNCTION__))) {
		send_to_char("No such spell found in the spell list.\r\n", ch);
		return;
	}

	/* Modify this list to suit which spells you
	 want to be able to mix. */
	switch (sptr->number) {
	case SPELL_BLINDNESS: /* 0 */
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 0;
		break;
	case SPELL_CURE_BLIND: /* 1 */
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 1;
		break;
	case SPELL_CURE_CRITIC:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 2;
		break;
	case SPELL_CURE_LIGHT:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 3;
		break;
	case SPELL_DETECT_ALIGN:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 4;
		break;
	case SPELL_DETECT_INVIS: /* 5 */
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 5;
		break;
	case SPELL_DETECT_MAGIC:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 6;
	 break;
	case SPELL_DETECT_POISON:
		num = 7;
		break;
	case SPELL_DISPEL_EVIL:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 8;
	 break;
	case SPELL_HEAL:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 9;
		break;
	case SPELL_INVISIBLE: /* 10 */
		num = 10;
		break;
	case SPELL_POISON:
		num = 11;
		break;
	case SPELL_PROT_FROM_EVIL:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 12;
	 break;
	case SPELL_REMOVE_CURSE:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 13;
	 break;
	case SPELL_SANCTUARY:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 14;
		break;
	case SPELL_SLEEP: /* 15 */
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 15;
		break;
	case SPELL_STRENGTH:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 16;
		break;
	case SPELL_REMOVE_POISON:
		num = 17;
		break;
	case SPELL_SENSE_LIFE:
		num = 18;
		break;
	case SPELL_DISPEL_GOOD:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 19;
	 break;
	case SPELL_INFRAVISION: /* 20 */
		num = 20;
		break;
	case SPELL_BARKSKIN:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 21;
		break;
	case SPELL_STONESKIN:
		num = 22;
		break;
	case SPELL_STEELSKIN:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 23;
		break;
	case SPELL_SHROUD:
		if (GET_CLASS(ch) == CLASS_ASSASSIN)
			can_make = FALSE;
		else
			num = 24;
		break;
	case SPELL_ORB: /* 25 */
		num = 25;
		break;
	default:
		can_make = FALSE;
		break;
	}

	if (can_make == FALSE) {
		 send_to_char("That spell cannot be mixed into a potion.\r\n", ch);
		 return;
	} else if ((number(1, 3) == 3) && (IS_MORTAL(ch))) {
		 send_to_char("As you begin mixing the potion, it violently explodes!\r\n",ch);
		 act("$n begins to mix a potion, but it suddenly explodes!", FALSE, ch, 0,0, TO_ROOM);
		 extract_obj(container);
		 dam = number(15, mag_manacost(ch, sptr) * 2);
		 GET_HIT(ch) -= dam;
		 update_pos(ch);
		 return;
	 }

	printbuf = get_buffer(MAX_STRING_LENGTH);

	/* requires x2 mana to mix a potion than the spell */
	mana = mag_manacost(ch, sptr) * 2;
	if (GET_MANA(ch) - mana > 0) {
		if (IS_MORTAL(ch))
			GET_MANA(ch) -= mana;
		sprintf(printbuf, "You create a %s potion.\r\n",
			sptr->name);
		send_to_char(printbuf, ch);
		act("$n creates a potion!", FALSE, ch, 0, 0, TO_ROOM);
		extract_obj(container);
	} else {
		send_to_char("You don't have enough mana to mix that potion!\r\n", ch);
		release_buffer(printbuf);
		return;
	}

	printbuf2 = get_buffer(MAX_STRING_LENGTH);

	final_potion = create_obj();

	final_potion->item_number = NOTHING;
	IN_ROOM(final_potion) = NOWHERE;
	sprintf(printbuf2, "%s %s potion", potion_names[num], sptr->name);
	final_potion->name = str_dup(printbuf2);

	sprintf(printbuf2, "A %s potion lies here.", potion_names[num]);
	final_potion->description = str_dup(printbuf2);

	sprintf(printbuf2, "a %s potion", potion_names[num]);
	final_potion->short_description = str_dup(printbuf2);

	/* extra description coolness! */
	CREATE(new_descr, struct extra_descr_data, 1);
	new_descr->keyword = str_dup(final_potion->name);
	sprintf(printbuf2, "It appears to be a %s potion.", sptr->name);
	new_descr->description = str_dup(printbuf2);
	new_descr->next = NULL;
	final_potion->ex_description = new_descr;

	GET_OBJ_TYPE(final_potion) = ITEM_POTION;
	GET_OBJ_WEAR(final_potion) = ITEM_WEAR_TAKE;
	SET_BIT(GET_OBJ_EXTRA(final_potion), ITEM_MAGIC);
	SET_BIT(GET_OBJ_EXTRA(final_potion), ITEM_UNIQUE_SAVE);
	GET_OBJ_VAL(final_potion, 0) = GET_SKILL(ch, SKILL_BREW);
	GET_OBJ_VAL(final_potion, 1) = potion;
	GET_OBJ_VAL(final_potion, 2) = -1;
	GET_OBJ_VAL(final_potion, 3) = -1;
	GET_OBJ_COST(final_potion) = GET_SKILL(ch, SKILL_BREW) * 250;
	GET_OBJ_WEIGHT(final_potion) = 1;
	GET_OBJ_RENT(final_potion) = 0;

	obj_to_char(final_potion, ch);

	release_buffer(printbuf);
	release_buffer(printbuf2);
}


ACMD(do_brew)
{
	struct obj_data *container = NULL;
	struct obj_data *obj, *next_obj;
	char *bottle_name = get_buffer(MAX_STRING_LENGTH);
	char *spell_name = get_buffer(MAX_STRING_LENGTH);
	char *temp1, *temp2;
	int potion, found = FALSE;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BREW)) {
	send_to_char("You have no idea how to brew potions.\r\n", ch);
	return;
	}

	temp1 = one_argument(argument, bottle_name);

	/* sanity check */
	if (temp1) {
		temp2 = get_spell_name(temp1);
		if (temp2)
			strcpy(spell_name, temp2);
	} else {
		bottle_name[0] = '\0';
		spell_name[0] = '\0';
	}

	if (!(IS_MAGI_TYPE(ch) || GET_CLASS(ch) == CLASS_BARD || GET_CLASS(ch) == CLASS_ASSASSIN || IS_IMMORTAL(ch))) {
		send_to_char("You have no idea how to mix potions!\r\n", ch);
		release_buffer(bottle_name);
		release_buffer(spell_name);
		return;
	}
	if (!*bottle_name || !*spell_name) {
		send_to_char("What do you wish to mix in what?\r\n", ch);
		release_buffer(bottle_name);
		release_buffer(spell_name);
		return;
	}

	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;
		if (obj == NULL) {
			release_buffer(bottle_name);
			release_buffer(spell_name);
			return;
		} else if (!(container = get_obj_in_list_vis(ch, bottle_name, NULL, ch->carrying)))
			continue;
		else
			found = TRUE;
	}
	if (found != FALSE && (GET_OBJ_TYPE(container) != ITEM_DRINKCON)) {
		send_to_char("That item is not a drink container!\r\n", ch);
		release_buffer(bottle_name);
		release_buffer(spell_name);
		return;
	}
	if (found == FALSE) {
		char *printbuf = get_buffer(MAX_STRING_LENGTH);
		sprintf(printbuf, "You don't have %s in your inventory!\r\n", bottle_name);
		send_to_char(printbuf, ch);
		release_buffer(printbuf);
		release_buffer(bottle_name);
		release_buffer(spell_name);
		return;
	}

	if (!spell_name || !*spell_name) {
		send_to_char("Spell names must be enclosed in single quotes!\r\n", ch);
		release_buffer(bottle_name);
		release_buffer(spell_name);
		return;
	}

	potion = find_spell_num(spell_name);

	if ((potion < 1) || (potion > MAX_SPELLS)) {
		send_to_char("That spell exists only in your imagination.\r\n", ch);
		release_buffer(bottle_name);
		release_buffer(spell_name);
		return;
	}

	if (GET_SKILL(ch, SKILL_BREW) == 0) {
		send_to_char("You are unfamiliar with potion brewing.\r\n", ch);
		release_buffer(bottle_name);
		release_buffer(spell_name);
		return;
	}
	make_potion(ch, potion, container);
}


void make_scroll(struct char_data *ch, int scroll, struct obj_data *paper)
{
	char *lbuf;
	int j, ofs = 0;
	char *name, *printbuf2;
	struct obj_data *final_scroll;
	struct extra_descr_data *new_descr;
	int can_make = TRUE, mana, dam = 0;
  struct spell_info_type *sptr = NULL;

	if (!(sptr = get_spell(scroll, __FILE__, __FUNCTION__))) {
		send_to_char("No such spell found in the spell list.\r\n", ch);
		return;
	}

	/* add a case statement here for prohibited spells */
	switch (scroll) {
	case SPELL_WORD_OF_RECALL:
	case SPELL_TELEPORT:
	case SPELL_VENTRILOQUATE:
		can_make = FALSE;
		break;
	default:
		can_make = TRUE;
		break;
	}

	if (can_make == FALSE) {
		send_to_char("That spell cannot be scribed into a scroll.\r\n", ch);
		return;
	} else if ((number(1, 3) == 3) && IS_MORTAL(ch)) {
		send_to_char("As you begin inscribing the final rune, the scroll violently explodes!\r\n",ch);
		act("$n tries to scribe a spell, but it explodes!",
		FALSE, ch, 0,0, TO_ROOM);
		extract_obj(paper);
		dam = number(15, mag_manacost(ch, sptr) * 2);
		GET_HIT(ch) -= dam;
		update_pos(ch);
		return;
	}
	/* requires x2 mana to scribe a scroll than the spell */
	mana = mag_manacost(ch, sptr) * 2;

	if (GET_MANA(ch) - mana > 0) {
		char *printbuf = get_buffer(MAX_STRING_LENGTH);
		if (IS_MORTAL(ch)) GET_MANA(ch) -= mana;
		sprintf(printbuf, "You create a scroll of %s.\r\n",
			sptr->name);
		send_to_char(printbuf, ch);
		act("$n creates a scroll!", FALSE, ch, 0, 0, TO_ROOM);
		extract_obj(paper);
		release_buffer(printbuf);
	} else {
		send_to_char("You don't have enough mana to scribe such a powerful spell!\r\n", ch);
		return;
	}

	final_scroll = create_obj();

	final_scroll->item_number = NOTHING;
	IN_ROOM(final_scroll) = NOWHERE;
	
	lbuf = get_buffer(256);
	name = get_buffer(256);
	
	sprintf(lbuf, "%s", LOWERALL(sptr->name));

	while (lbuf[ofs]) {
		for (j = 0; *(syls[j].org); j++) {
			if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
				strcat(name, syls[j].news);
				ofs += strlen(syls[j].org);
				break;
			}
		}
		/* i.e., we didn't find a match in syls[] */
		if (!*syls[j].org) {
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "No entry in syllable table for substring of '%s' in act.create.c.", lbuf);
			ofs++;
		}
	}

	printbuf2 = get_buffer(MAX_INPUT_LENGTH);
	
	sprintf(printbuf2, "%s %s scroll", sptr->name, name);
	final_scroll->name = str_dup(printbuf2);

	sprintf(printbuf2, "Some parchment inscribed with the runes '%s' lies here.", name);
	final_scroll->description = str_dup(printbuf2);

	sprintf(printbuf2, "a %s scroll", name);
	final_scroll->short_description = str_dup(printbuf2);

	/* extra description coolness! */
	CREATE(new_descr, struct extra_descr_data, 1);
	new_descr->keyword = str_dup(final_scroll->name);
	sprintf(printbuf2, "It appears to be a %s scroll.", sptr->name);
	new_descr->description = str_dup(printbuf2);
	new_descr->next = NULL;
	final_scroll->ex_description = new_descr;

	GET_OBJ_TYPE(final_scroll) = ITEM_SCROLL;
	GET_OBJ_WEAR(final_scroll) = ITEM_WEAR_TAKE;
	SET_BIT(GET_OBJ_EXTRA(final_scroll), ITEM_MAGIC);
	SET_BIT(GET_OBJ_EXTRA(final_scroll), ITEM_UNIQUE_SAVE);
	GET_OBJ_VAL(final_scroll, 0) = GET_SKILL(ch, SKILL_SCRIBE);
	GET_OBJ_VAL(final_scroll, 1) = scroll;
	GET_OBJ_VAL(final_scroll, 2) = -1;
	GET_OBJ_VAL(final_scroll, 3) = -1;
	GET_OBJ_COST(final_scroll) = GET_SKILL(ch, SKILL_SCRIBE) * 500;
	GET_OBJ_WEIGHT(final_scroll) = 1;
	GET_OBJ_RENT(final_scroll) = 0;

	obj_to_char(final_scroll, ch);
	
	release_buffer(name);
	release_buffer(printbuf2);
}


ACMD(do_scribe)
{
	struct obj_data *paper = NULL;
	struct obj_data *obj, *next_obj;
	char paper_name[MAX_STRING_LENGTH];
	char spell_name[MAX_STRING_LENGTH];
	char *temp1, *temp2;
	int scroll = 0, found = FALSE;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SCRIBE)) {
		send_to_char("You have no idea how to scribe scrolls.\r\n", ch);
		return;
	}
	temp1 = one_argument(argument, paper_name);

	/* sanity check */
	if (temp1) {
		temp2 = get_spell_name(temp1);
		if (temp2)
			strcpy(spell_name, temp2);
	} else {
		paper_name[0] = '\0';
		spell_name[0] = '\0';
	}

	if (!(GET_CLASS(ch) == CLASS_MAGUS_ANIMAL || GET_CLASS(ch) == CLASS_MAGUS_AQUAM || GET_CLASS(ch) == CLASS_MAGUS_AURAM || GET_CLASS(ch) == CLASS_MAGUS_CORPOREM || GET_CLASS(ch) == CLASS_MAGUS_HERBAM || GET_CLASS(ch) == CLASS_MAGUS_IGNEM || GET_CLASS(ch) == CLASS_MAGUS_IMAGONEM || GET_CLASS(ch) == CLASS_MAGUS_MENTEM || GET_CLASS(ch) == CLASS_MAGUS_TERRAM || GET_CLASS(ch) == CLASS_MAGUS_VIM || GET_CLASS(ch) == CLASS_MONK || IS_IMMORTAL(ch))) {
		send_to_char("You have no idea how to scribe scrolls!\r\n", ch);
		return;
	}
	if (!*paper_name || !*spell_name) {
		send_to_char("What do you wish to scribe where?\r\n", ch);
		return;
	}

	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;
		if (obj == NULL)
			return;
		else if (!(paper = get_obj_in_list_vis(ch, paper_name, NULL, ch->carrying)))
			continue;
		else
			found = TRUE;
	}
	if (found && (GET_OBJ_TYPE(paper) != ITEM_NOTE)) {
		send_to_char("You can't write on that!\r\n", ch);
		return;
	}
	if (found == FALSE) {
		char *printbuf = get_buffer(MAX_STRING_LENGTH);
		sprintf(printbuf, "You don't have %s in your inventory!\r\n",
		paper_name);
		send_to_char(printbuf, ch);
		release_buffer(printbuf);
		return;
	}

	if (!spell_name || !*spell_name) {
		send_to_char("Spell names must be enclosed in single quotes!\r\n", ch);
		return;
	}

	scroll = find_spell_num(spell_name);

	if ((scroll < 1) || (scroll > MAX_SPELLS)) {
		send_to_char("Scribe what spell?!?\r\n", ch);
		return;
	}
	if (GET_SKILL(ch, scroll) == 0) {
		send_to_char("You don't know any spell like that!\r\n", ch);
		return;
	}
	make_scroll(ch, scroll, paper);
}


ACMD(do_forge)
{
	/*
	 * PLEASE NOTE!!!  This command alters the object_values of the target
	 * weapon, and this will save to the rent files.  It should not cause
	 * a problem with stock Circle, but if your weapons use the first
	 * position [ GET_OBJ_VAL(weapon, 0); ], then you WILL have a problem.
	 * This command stores the character's level in the first value to
	 * prevent the weapon from being "forged" more than once by mortals.
	 * Install at your own risk.  You have been warned...
	 */

	struct obj_data *weapon = NULL;
	struct obj_data *obj, *next_obj;
	char weapon_name[MAX_STRING_LENGTH];
	int found = FALSE, prob = 0, dam = 0;

	one_argument(argument, weapon_name);

	if (!(GET_CLASS(ch) == CLASS_WARRIOR || GET_CLASS(ch) == CLASS_GLADIATOR || GET_CLASS(ch) == CLASS_ASSASSIN || GET_CLASS(ch) == CLASS_THIEF || IS_IMMORTAL(ch))) {
		send_to_char("You have no idea how to forge weapons!\r\n", ch);
		return;
	}
	if (!*weapon_name) {
		send_to_char("What do you wish to forge?\r\n", ch);
		return;
	}

	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;
		if (obj == NULL)
			return;
		else if (!(weapon = get_obj_in_list_vis(ch, weapon_name, NULL, ch->carrying)))
			continue;
		else
			found = TRUE;
	}

	if (found == FALSE) {
		char *printbuf = get_buffer(MAX_INPUT_LENGTH);
		sprintf(printbuf, "You don't have %s in your inventory!\r\n",
		weapon_name);
		send_to_char(printbuf, ch);
		release_buffer(printbuf);
		return;
	}

	if (found && (GET_OBJ_TYPE(weapon) != ITEM_WEAPON)) {
		char *printbuf = get_buffer(MAX_INPUT_LENGTH);
		sprintf(printbuf, "It doesn't look like %s would make a good weapon...\r\n", weapon_name);
		send_to_char(printbuf, ch);
		release_buffer(printbuf);
		return;
	}

	if ((GET_OBJ_VAL(weapon, 0) > 0) && IS_MORTAL(ch)) {
		send_to_char("You cannot forge a weapon more than once!\r\n", ch);
		return;
	}

	if (GET_OBJ_EXTRA(weapon) & ITEM_MAGIC) {
		send_to_char("The weapon is imbued with magical powers beyond"
		"your grasp.\r\nYou cannot further affect its form.\r\n", ch);
		return;
	}

	/* determine success probability */
	prob += ((GET_INTELLIGENCE(ch) / 100) << 1) + (((GET_AGILITY(ch) / 100) - 11) << 1);
	prob += (((GET_STRENGTH(ch) / 100) - 11) << 1);

	if ((number(10, 100) > prob) && IS_MORTAL(ch)) {
		send_to_char("As you pound out the dents in the weapon,"
		" you hit a weak spot and it explodes!\r\n", ch);
		act("$n tries to forge a weapon, but it explodes!",
		FALSE, ch, 0,0, TO_ROOM);
		extract_obj(weapon);
		dam = number(20, 60);
		GET_HIT(ch) -= dam;
		update_pos(ch);
		return;
	}

	GET_OBJ_VAL(weapon, 0) = prob;
	GET_OBJ_VAL(weapon, 1) += (prob % 3) + number(-1, 2);
	GET_OBJ_VAL(weapon, 2) += (prob % 2) + number(-1, 2);
	GET_OBJ_RENT(weapon) += (prob << 3);
	SET_BIT(GET_OBJ_EXTRA(weapon), ITEM_MAGIC);
	SET_BIT(GET_OBJ_EXTRA(weapon), ITEM_UNIQUE_SAVE);

	send_to_char("You have forged new life into the weapon!\r\n", ch);
	act("$n vigorously pounds on a weapon!",
	FALSE, ch, 0, 0, TO_ROOM);
}

