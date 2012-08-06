/* ************************************************************************
*		File: spell_parser.c                                Part of CircleMUD *
*	 Usage: top-level magic routines; outside points of entry to magic sys. *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*  Additions for Dynamic Spell Editor compatibility by                    *
*  Torgny Bjers <artovil@arcanerealms.org>                                *
*  Copyright (C) 2002 by Arcane Realms MUD, www.arcanerealms.org.         *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: spell_parser.c,v 1.28 2003/01/01 13:44:21 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "interpreter.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "genolc.h"
#include "oasis.h"
#include "spedit.h"
#include "constants.h"
int spnum			=	0;
int newspell	=	0;
int top_of_spellt = 0;

extern int magic_enabled;
struct spell_info_type *spell_info;
extern struct room_data *world;
extern struct index_data *obj_index;

/* local functions */
void say_spell(struct char_data *ch, struct spell_info_type *sptr, struct char_data *tch, struct obj_data *tobj);
int	mag_manacost(struct char_data *ch, struct spell_info_type *sptr);
ACMD(do_cast);
void unused_spell(int spl);
void sort_spells(void);

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, 200 should provide
 * ample slots for skills.
 */

struct syllable {
	const char *org;
	const char *news;
};


struct syllable syls[] = {
	{" ", " "},
	{"ar", "abra"},
	{"ate", "i"},
	{"cau", "kada"},
	{"blind", "nose"},
	{"bur", "mosa"},
	{"cu", "judi"},
	{"de", "oculo"},
	{"dis", "mar"},
	{"ect", "kamina"},
	{"en", "uns"},
	{"gro", "cra"},
	{"light", "dies"},
	{"lo", "hi"},
	{"magi", "kari"},
	{"mon", "bar"},
	{"mor", "zak"},
	{"move", "sido"},
	{"ness", "lacri"},
	{"ning", "illa"},
	{"per", "duda"},
	{"ra", "gru"},
	{"re", "candus"},
	{"son", "sabru"},
	{"tect", "infra"},
	{"tri", "cula"},
	{"ven", "nofo"},
	{"word of", "inset"},
	{"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"}, {"g", "t"},
	{"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"},
	{"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"}, {"u", "e"},
	{"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"}, {"z", "k"}, {"", ""}
};

const	char *unused_spellname = "!UNUSED!"; /* So we can get &unused_spellname */


struct spell_info_type *get_spell(int spellnum, const char *file, const char *function)
{
	struct spell_info_type *sptr;
	
	if (spellnum < 1 || spellnum > top_of_spellt) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "%s,%s(%d): Called by %s:%s with invalid spellnum (%d)", __FILE__, __FUNCTION__, __LINE__, file, function, spellnum);
		return (NULL);
	} 
	
	for (sptr = spell_info; sptr; sptr = sptr->next)
		if (sptr->number == spellnum)
			return (sptr);
	
	return (NULL);
}


int	mag_manacost(struct char_data *ch, struct spell_info_type *sptr)
{
	return MAX(sptr->mana_max - (GET_SKILL(ch, sptr->skill)/100),
						 sptr->mana_min);
}


/* say_spell erodes buf, buf1, buf2 */
void say_spell(struct char_data *ch, struct spell_info_type *sptr, struct char_data *tch,
										struct obj_data *tobj)
{
	char lbuf[256];
	const char *format;

	struct char_data *i;
	int j, ofs = 0;
	
	*buf = '\0';
	sprintf(lbuf, "%s", LOWERALL(sptr->name));
	
	while (lbuf[ofs]) {
		for (j = 0; *(syls[j].org); j++) {
			if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
				strcat(buf, syls[j].news);
				ofs += strlen(syls[j].org);
				break;
			}
		}
		/* i.e., we didn't find a match in syls[] */
		if (!*syls[j].org) {
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "No entry in syllable table for substring of '%s'", lbuf);
			ofs++;
		}
	}
	
	if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch)) {
		if (tch == ch)
			format = "$n closes $s eyes and utters the words, '&W%s&n'.";
		else
			format = "$n stares at $N and utters the words, '&W%s&n'.";
	} else if (tobj != NULL &&
		((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch)))
		format = "$n stares at $p and utters the words, '&W%s&n'.";
	else
		format = "$n utters the words, '&W%s&n'.";
	
	sprintf(buf1, format, sptr->name);
	sprintf(buf2, format, buf);
	
	for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
		if (i == ch || i == tch || !i->desc || !AWAKE(i) || IN_OLC(i->desc) || PLR_FLAGGED(i, PLR_WRITING))
			continue;
		if (GET_CLASS(ch) == GET_CLASS(i))
			perform_act(buf1, ch, tobj, tch, i, 0);
		else
			perform_act(buf2, ch, tobj, tch, i, 0);
	}
	
	if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch)) {
		sprintf(buf1, "$n stares at you and utters the words, '&W%s&n'.",
			GET_CLASS(ch) == GET_CLASS(tch) ? sptr->name : buf);
		act(buf1, FALSE, ch, NULL, tch, TO_VICT);
	}
}

/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE.
 */
char *spell_name(int num)
{
  struct spell_info_type *sptr;

	sptr = get_spell(num, __FILE__, __FUNCTION__);

	if (num > 0 && num <= top_of_spellt)
		return (sptr->name);
	else if (num == -1)
		return ("UNUSED");
	else
		return ("UNDEFINED");
}


int	find_spell_num(char *name)
{
	int ok;
	char *temp, *temp2;
	char first[256], first2[256];
  struct spell_info_type *sptr;

	for (sptr = spell_info; sptr; sptr = sptr->next) {
		if (sptr->number == 0) continue;
		if (is_abbrev(name, sptr->name))
			return (sptr->number);

		ok = TRUE;
		/* It won't be changed, but other uses of this function elsewhere may. */
		temp = any_one_arg(sptr->name, first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok) {
			if (!is_abbrev(first2, first))
				ok = FALSE;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}

		if (ok && !*first2)
			return (sptr->number);
	}

	return (-1);
}


/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int	call_magic(struct char_data *caster, struct char_data *cvict,
							 struct obj_data *ovict, struct spell_info_type *sptr,
							 int level, int casttype, char *tar_str)
{
	int savetype;

	if (!magic_enabled)
		return (0);

	if (caster->nr != real_mobile(DG_CASTER_PROXY)) {
		if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC)) {
			send_to_char("Your magic fizzles out and dies.\r\n", caster);
			act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
			return (0);
		}
		if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) &&
				(sptr->violent || IS_SET(sptr->routines, MAG_DAMAGE))) {
			send_to_char("A flash of white light fills the room, dispelling your "
									 "violent magic!\r\n", caster);
			act("White light from no particular source suddenly fills the room, "
					"then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
			return (0);
		}
	}
	/* determine the type of saving throw */
	switch (casttype) {
		case CAST_STAFF:
		case CAST_SCROLL:
		case CAST_POTION:
		case CAST_WAND:
			savetype = SAVING_ROD;
			break;
		case CAST_SPELL:
			savetype = SAVING_SPELL;
			break;
		default:
			savetype = SAVING_BREATH;
			break;
	}

	if (IS_SET(sptr->routines, MAG_DAMAGE))
		if (mag_damage(level, caster, cvict, sptr, savetype) == -1)
			return (-1);        /* Successful and target died, don't cast again. */

	if (IS_SET(sptr->routines, MAG_AFFECTS))
		mag_affects(level, caster, cvict, sptr, savetype);

	if (IS_SET(sptr->routines, MAG_UNAFFECTS))
		mag_unaffects(level, caster, cvict, sptr, savetype);

	if (IS_SET(sptr->routines, MAG_POINTS))
		mag_points(level, caster, cvict, sptr, savetype);

	if (IS_SET(sptr->routines, MAG_ALTER_OBJS))
		mag_alter_objs(level, caster, ovict, sptr, savetype);

	if (IS_SET(sptr->routines, MAG_GROUPS))
		mag_groups(level, caster, sptr, savetype);

	if (IS_SET(sptr->routines, MAG_MASSES))
		mag_masses(level, caster, sptr, savetype);

	if (IS_SET(sptr->routines, MAG_AREAS))
		mag_areas(level, caster, sptr, savetype);

	if (IS_SET(sptr->routines, MAG_SUMMONS))
		mag_summons(level, caster, ovict, sptr, savetype);

	if (IS_SET(sptr->routines, MAG_CREATIONS))
		mag_creations(level, caster, sptr);
	
	if (IS_SET(sptr->routines, MAG_MANUAL))
		switch (sptr->number) {
		 case SPELL_CHARM:           MANUAL_SPELL(spell_charm); break;
		 case SPELL_CREATE_WATER:    MANUAL_SPELL(spell_create_water); break;
		 case SPELL_DETECT_POISON:   MANUAL_SPELL(spell_detect_poison); break;
		 case SPELL_ENCHANT_WEAPON:  MANUAL_SPELL(spell_enchant_weapon); break;
		 case SPELL_IDENTIFY:        MANUAL_SPELL(spell_identify); break;
		 case SPELL_LOCATE_OBJECT:   MANUAL_SPELL(spell_locate_object); break;
		 case SPELL_SUMMON:          MANUAL_SPELL(spell_summon); break;
		 case SPELL_WORD_OF_RECALL:  MANUAL_SPELL(spell_recall); break;
		 case SPELL_TELEPORT:        MANUAL_SPELL(spell_teleport); break;
		 case SPELL_MINOR_IDENTIFY:  MANUAL_SPELL(spell_minor_identify); break;
		 case SPELL_PORTAL:          MANUAL_SPELL(spell_portal); break;
		 case SPELL_ARCANE_WORD:     MANUAL_SPELL(spell_arcane_word); break;
		 case SPELL_ARCANE_PORTAL:   MANUAL_SPELL(spell_arcane_portal); break;
	}

	return (1);
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * For reference, object values 0-3:
 * staff  - [0]        level        [1] max charges        [2] num charges        [3] spell num
 * wand   - [0]        level        [1] max charges        [2] num charges        [3] spell num
 * scroll - [0]        level        [1] spell num          [2] spell num          [3] spell num
 * potion - [0]        level        [1] spell num          [2] spell num          [3] spell num
 * spellbook[0]        % learn      [1] spell num          [2] spell num          [3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */

void mag_objectmagic(struct char_data *ch, struct obj_data *obj,
													char *argument)
{
	int i, k;
	struct char_data *tch = NULL, *next_tch;
	struct obj_data *tobj = NULL;

	if (!magic_enabled)
		return;

	one_argument(argument, arg);

	k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
									 FIND_OBJ_EQUIP, ch, &tch, &tobj);

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_STAFF:
		act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
		if (obj->action_description)
			act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
		else
			act("$n taps $p three times on the ground.", FALSE, ch, obj, 0, TO_ROOM);

		if (GET_OBJ_VAL(obj, 2) <= 0) {
			send_to_char("It seems powerless.\r\n", ch);
			act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
		} else {
			GET_OBJ_VAL(obj, 2)--;
			WAIT_STATE(ch, PULSE_VIOLENCE);
			/* Level to cast spell at. */
			k = GET_OBJ_VAL(obj, 0) ? GET_OBJ_VAL(obj, 0) : DEFAULT_STAFF_LVL;

			/*
			 * Problem : Area/mass spells on staves can cause crashes.
			 * Solution: Remove the special nature of area/mass spells on staves.
			 * Problem : People like that behavior.
			 * Solution: We special case the area/mass spells here.
			 */
			if (HAS_SPELL_ROUTINE(get_spell(GET_OBJ_VAL(obj, 3), __FILE__, __FUNCTION__), MAG_MASSES | MAG_AREAS)) {
				for (i = 0, tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
					i++;
				while (i-- > 0)
					call_magic(ch, NULL, NULL, get_spell(GET_OBJ_VAL(obj, 3), __FILE__, __FUNCTION__), k, CAST_STAFF, 0);
			} else {
				for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
					next_tch = tch->next_in_room;
					if (ch != tch)
						call_magic(ch, tch, NULL, get_spell(GET_OBJ_VAL(obj, 3), __FILE__, __FUNCTION__), k, CAST_STAFF, 0);
				}
			}
		}
		break;
	case ITEM_WAND:
		if (k == FIND_CHAR_ROOM) {
			if (tch == ch) {
				act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
			} else {
				act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
				if (obj->action_description)
					act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
				else
					act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
			}
		} else if (tobj != NULL) {
			act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
			if (obj->action_description)
				act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
			else
				act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
		} else if (IS_SET(get_spell(GET_OBJ_VAL(obj, 3), __FILE__, __FUNCTION__)->routines, MAG_AREAS | MAG_MASSES)) {
			/* Wands with area spells don't need to be pointed. */
			act("You point $p outward.", FALSE, ch, obj, NULL, TO_CHAR);
			act("$n points $p outward.", TRUE, ch, obj, NULL, TO_ROOM);
		} else {
			act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
			return;
		}

		if (GET_OBJ_VAL(obj, 2) <= 0) {
			send_to_char("It seems powerless.\r\n", ch);
			act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
			return;
		}
		GET_OBJ_VAL(obj, 2)--;
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (GET_OBJ_VAL(obj, 0))
			call_magic(ch, tch, tobj, get_spell(GET_OBJ_VAL(obj, 3), __FILE__, __FUNCTION__),
								 GET_OBJ_VAL(obj, 0), CAST_WAND, 0);
		else
			call_magic(ch, tch, tobj, get_spell(GET_OBJ_VAL(obj, 3), __FILE__, __FUNCTION__),
								 DEFAULT_WAND_LVL, CAST_WAND, 0);
		break;
	case ITEM_SCROLL:
		if (*arg) {
			if (!k) {
				act("There is nothing to here to affect with $p.", FALSE,
						ch, obj, NULL, TO_CHAR);
				return;
			}
		} else
			tch = ch;

		act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
		if (obj->action_description)
			act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
		else
			act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);

		WAIT_STATE(ch, PULSE_VIOLENCE);
		for (i = 1; i <= 3; i++)
			if (call_magic(ch, tch, tobj, get_spell(GET_OBJ_VAL(obj, i), __FILE__, __FUNCTION__),
											 GET_OBJ_VAL(obj, 0), CAST_SCROLL, 0) <= 0)
				break;

		if (obj != NULL)
			extract_obj(obj);
		break;
	case ITEM_POTION:
		tch = ch;
		act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
		if (obj->action_description)
			act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
		else
			act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);

		WAIT_STATE(ch, PULSE_VIOLENCE);
		for (i = 1; i <= 3; i++)
			if (call_magic(ch, ch, NULL, get_spell(GET_OBJ_VAL(obj, i), __FILE__, __FUNCTION__),
											 GET_OBJ_VAL(obj, 0), CAST_POTION, 0) <= 0)
				break;

		if (obj != NULL)
			extract_obj(obj);
		break;
 case ITEM_SPELLBOOK:
		tch = ch;
		act("You study $p.", FALSE, ch, obj, NULL, TO_CHAR);
		if (obj->action_description)
			act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
		else
			act("$n studies $p.", TRUE, ch, obj, NULL, TO_ROOM);

		WAIT_STATE(ch, PULSE_VIOLENCE);
		for (i = 1; i <= 3; i++) {
			if (GET_OBJ_VAL(obj, i) < 0) break;
			if (GET_SKILL(ch, GET_OBJ_VAL(obj, i)) < 1)
				SET_SKILL(ch, GET_OBJ_VAL(obj, i), GET_OBJ_VAL(obj, 0));
		}

		if (obj != NULL)
			extract_obj(obj);
		break;
	default:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unknown object_type %d in mag_objectmagic.",
				GET_OBJ_TYPE(obj));
		break;
	}
}


/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */

int	cast_spell(struct char_data *ch, struct char_data *tch,
									 struct obj_data *tobj, struct spell_info_type *sptr,
									 char *tar_str)
{

	if (!magic_enabled)
		return (0);

	if (!sptr) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "cast_spell() called without a valid sptr: ch: %s, tch: %s", 
			GET_NAME(ch), GET_NAME(tch));
		return (0);
	}

	if (GET_POS(ch) < sptr->min_position) {
		switch (GET_POS(ch)) {
			case POS_SLEEPING:
			send_to_char("You dream about great magical powers.\r\n", ch);
			break;
		case POS_RESTING:
			send_to_char("You cannot concentrate while resting.\r\n", ch);
			break;
		case POS_SITTING:
			send_to_char("You can't do this sitting!\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Impossible!  You can't concentrate enough!\r\n", ch);
			break;
		default:
			send_to_char("You can't do much of anything like this!\r\n", ch);
			break;
		}
		return (0);
	}
	if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch)) {
		send_to_char("You are afraid you might hurt your master!\r\n", ch);
		return (0);
	}
	if ((tch != ch) && IS_SET(sptr->targets, TAR_SELF_ONLY)) {
		send_to_char("You can only cast this spell upon yourself!\r\n", ch);
		return (0);
	}
	if ((tch == ch) && IS_SET(sptr->targets, TAR_NOT_SELF)) {
		send_to_char("You cannot cast this spell upon yourself!\r\n", ch);
		return (0);
	}
	if (IS_SET(sptr->routines, MAG_GROUPS) && !AFF_FLAGGED(ch, AFF_GROUP)) {
		send_to_char("You can't cast this spell if you're not in a group!\r\n",ch);
		return (0);
	}

	if (cast_mtrigger(tch, ch, tar_str, sptr) == 0)
		return (0);
	if (cast_otrigger(tobj, ch, tar_str, sptr) == 0)
		return (0);
	if (cast_wtrigger(ch, tch, tobj, tar_str, sptr) == 0)
		return (0);

	send_to_char(OK, ch);
	say_spell(ch, sptr, tch, tobj);

	return (call_magic(ch, tch, tobj, sptr, (GET_SKILL(ch, sptr->skill)/100), CAST_SPELL,
										 tar_str));
}


/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 */

ACMD(do_cast)
{
	struct spell_info_type *sptr;
	struct char_data *tch = NULL;
	struct obj_data *tobj = NULL;
	char *s, *t;
	int mana, spellnum, i, target = 0;

	if (IS_NPC(ch))
		return;

	if (IS_MORTAL(ch) && !CAN_CAST_SPELLS(ch)) {
		send_to_char("You do not know how to cast spells!\r\n", ch);
		return;
	}

	if (!magic_enabled) {
		send_to_char("The magic system has been disabled!\r\n", ch);
		return;
	}

	/* get: blank, spell name, target name */
	s = strtok(argument, "'");

	if (s == NULL) {
		send_to_char("Cast what where?\r\n", ch);
		return;
	}
	s = strtok(NULL, "'");
	if (s == NULL) {
		send_to_char("Spell names must be enclosed in the Holy Magic Symbols: '\r\n", ch);
		return;
	}
	t = strtok(NULL, "\0");

	/* spellnum = search_block(s, spells, 0); */
	spellnum = find_spell_num(s);

  if (!(sptr = get_spell(spellnum, __FILE__, __FUNCTION__))) {
  	send_to_char("Cast what!?\r\n", ch);
  	return;
  }

	if (sptr->learned == 0 && (sptr->learned && GET_SKILL(ch, sptr->skill) == 0)) {
		send_to_char("You do not know that spell!\r\n", ch);
		return;
	}
	if (GET_SKILL(ch, sptr->skill) == 0) {
		send_to_char("You are unfamiliar with that spell.\r\n", ch);
		return;
	}
	/* Find the target */
	if (t != NULL) {
		one_argument(strcpy(arg, t), t);
		skip_spaces(&t);
	}
	if (IS_SET(sptr->targets, TAR_IGNORE)) {
		target = TRUE;
	} else if (t != NULL && *t) {
		if (!target && (IS_SET(sptr->targets, TAR_CHAR_ROOM))) {
			if ((tch = get_char_vis(ch, t, NULL, FIND_CHAR_ROOM, 0)) != NULL)
				target = TRUE;
		}
		if (!target && IS_SET(sptr->targets, TAR_CHAR_WORLD))
			if ((tch = get_char_vis(ch, t, NULL, FIND_CHAR_WORLD, 0)) != NULL)
				target = TRUE;

		if (!target && IS_SET(sptr->targets, TAR_OBJ_INV))
			if ((tobj = get_obj_in_list_vis(ch, t, NULL, ch->carrying)) != NULL)
				target = TRUE;

		if (!target && IS_SET(sptr->targets, TAR_OBJ_EQUIP)) {
			for (i = 0; !target && i < NUM_WEARS; i++)
				if (HAS_BODY(ch, i) && GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->name)) {
					tobj = GET_EQ(ch, i);
					target = TRUE;
				}
		}
		if (!target && IS_SET(sptr->targets, TAR_OBJ_ROOM))
			if ((tobj = get_obj_in_list_vis(ch, t, NULL, world[IN_ROOM(ch)].contents)) != NULL)
				target = TRUE;

		if (!target && IS_SET(sptr->targets, TAR_OBJ_WORLD))
			if ((tobj = get_obj_vis(ch, t, NULL)) != NULL)
				target = TRUE;

	} else {                        /* if target string is empty */
		if (!target && IS_SET(sptr->targets, TAR_FIGHT_SELF))
			if (FIGHTING(ch) != NULL) {
				tch = ch;
				target = TRUE;
			}
		if (!target && IS_SET(sptr->targets, TAR_FIGHT_VICT))
			if (FIGHTING(ch) != NULL) {
				tch = FIGHTING(ch);
				target = TRUE;
			}
		/* if no target specified, and the spell isn't violent, default to self */
		if (!target && IS_SET(sptr->targets, TAR_CHAR_ROOM) &&
				!sptr->violent) {
			tch = ch;
			target = TRUE;
		}
		if (!target) {
			sprintf(buf, "Upon %s should the spell be cast?\r\n",
				IS_SET(sptr->targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP) ? "what" : "who");
			send_to_char(buf, ch);
			return;
		}
	}

	if (target && (tch == ch) && sptr->violent) {
		send_to_char("You shouldn't cast that on yourself -- could be bad for your health!\r\n", ch);
		return;
	}
	if (!target) {
		send_to_char("Cannot find the target of your spell!\r\n", ch);
		return;
	}

	mana = mag_manacost(ch, sptr);

	if ((mana > 0) && (GET_MANA(ch) < mana) && IS_MORTAL(ch)) {
		send_to_char("You haven't the energy to cast that spell!\r\n", ch);
		return;
	}

	/* Throw the dice, 101% is total failure */
	if (number(0, 101) > GET_SKILL(ch, spellnum)) {
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (!tch || !skill_message(0, ch, tch, sptr->skill))
			send_to_char("You lost your concentration!\r\n", ch);
		if (mana > 0)
			GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana / 2)));
		if (sptr->violent && tch && IS_NPC(tch))
			hit(tch, ch, TYPE_UNDEFINED);
	} else { /* cast spell returns 1 on success; subtract mana & set waitstate */
		if (cast_spell(ch, tch, tobj, sptr, t)) {
			WAIT_STATE(ch, PULSE_VIOLENCE);
			if (mana > 0)
				GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
		}
	}
}


/* Puts spells in numerical order */
void sort_spells(void)
{
	struct spell_info_type *sptr = NULL, *temp = NULL;

	if (spnum > 2) {
		for (sptr = spell_info; (sptr->next != NULL) && (sptr->next->next != NULL); sptr = sptr->next) {
			if (sptr->next->number > sptr->next->next->number) {
				temp = sptr->next;
				sptr->next = temp->next;
				temp->next = sptr->next->next;
				sptr->next->next = temp;
			}
		}
	}
	
	return;
}


void free_spell(struct spell_info_type *s)
{
#if !BUFFER_MEMORY
	if (s != NULL) {
		free(s->name);
		free(s);
	}
#endif
}


struct spell_info_type *enqueue_spell(void)
{
	struct spell_info_type *sptr;
	
	/* This is the first spell loaded if true */
	if (spell_info == NULL) {
		if ((spell_info = (struct spell_info_type *) malloc(sizeof(struct spell_info_type))) == NULL) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Out of memory for spells! Aborting...");
			kill_mysql();
			exit(1);
		} else {
			spell_info->next = NULL;
			return (spell_info);
		}
	} else { /* spell_info is not NULL */
		for (sptr = spell_info; sptr->next != NULL; sptr = sptr->next)
			; /* Loop does the work */
		if ((sptr->next = (struct spell_info_type *) malloc(sizeof(struct spell_info_type))) == NULL) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Out of memory for spells! Aborting...");
			kill_mysql();
			exit(1);
		} else {
			sptr->next->next = NULL;
			return (sptr->next);
		}
	}
	return NULL;
}


void dequeue_spell(int spellnum)
{
	struct spell_info_type *sptr = NULL, *temp;
	
	if (spellnum < 0 || spellnum > spnum) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Attempting to dequeue invalid spell!");
		kill_mysql();
		exit(1);
	} else {
		if (spell_info->number != spellnum) {
			for (sptr = spell_info; sptr->next && sptr->next->number != spellnum; sptr = sptr->next)
				;
			if (sptr->next != NULL && sptr->next->number == spellnum) {
				temp = sptr->next;
				sptr->next = temp->next;
				free_spell(temp);
			}
		} else {
		/* The first one is the one being removed */
			sptr = spell_info;
			spell_info = spell_info->next;
			free_spell(sptr);
		}
	}
}


void create_spellfile(void)
{
	FILE *sfile;
	
	/* Should be no reason it can't open... */
	if ((sfile = fopen(SPELL_FILE, "wt")) == NULL) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Cannot create new spell file! Aborting...");
		kill_mysql();
		exit(1);
	}

	/* The total number of spells */
	fprintf(sfile, "1\n");
	/* The Void Spell */
	fprintf(sfile, "#0\n"
		"!RESERVED!~\n"
		"~\n"
		"~\n"
		"~\n"
		"~\n"
		"~\n"
		"~\n"
		"-1 0 0\n"
		"-1 0 0\n"
		"-1 0 0\n"
		"0 0 0 0\n"
		"0 0 0 0\n"
		"0 0 0 0 0\n"
		"$\n");

	fclose(sfile);
}


void read_reg_obj_line(struct spell_info_type *sptr, FILE *spell_f, const char *string, void *data1, void *data2, void *data3)
{
  if (!get_line(spell_f, buf) || !sscanf(buf, string, data1, data2, data3)) {
    mlog("SYSERR: Error in spell #%d\n", SPELL_NUM(sptr));
    kill_mysql();
    exit(1);
  }
}


int add_to_reg_obj_list(struct spell_obj_data *list, int type, int *len, int *val1, int *val2, int *val3)
{
  if (*val1 >= 0) {
    if (*len < MAX_SPELL_OBJ) {
      if (*val1 >= 0) {
				SPELL_OBJ_TYPE(list[*len]) = *val1;
      } else
				*val1 = 0;
      if (*val2 >= 0) {
				SPELL_OBJ_LOCATION(list[*len]) = *val2;
      } else
				*val2 = 0;
      if (*val3 >= 0) {
			  SPELL_OBJ_EXTRACT(list[(*len)++]) = *val3;
      } else
				*val3 = 0;
      return (FALSE);
    } else
      return (TRUE);
  }
  return (FALSE);
}


int end_read_reg_obj_list(struct spell_obj_data *list, int len, int error)
{
  if (error)
    mlog("SYSERR: Raise MAX_SPELL_OBJ constant in spells.h to %d", len + error);
  SPELL_OBJ_TYPE(list[len]) = NOTHING;
  SPELL_OBJ_LOCATION(list[len]) = 0;
  SPELL_OBJ_EXTRACT(list[len++]) = 0;
  return (len);
}


int read_reg_obj_list(struct spell_info_type *sptr, FILE *spell_f, struct spell_obj_data *list, int new_format,
	          int max, int type)
{
  int count, tmp1, tmp2, tmp3, len = 0, error = 0;

  if (new_format) {
    do {
      read_reg_obj_line(sptr, spell_f, "%d %d %d", &tmp1, &tmp2, &tmp3);
      error += add_to_reg_obj_list(list, type, &len,  &tmp1, &tmp2, &tmp3);
    } while (tmp1 >= 0);
  } else
    for (count = 0; count < max; count++) {
      read_reg_obj_line(sptr, spell_f, "%d %d %d", &tmp1, &tmp2, &tmp3);
      error += add_to_reg_obj_list(list, type, &len,  &tmp1, &tmp2, &tmp3);
    }
  return (end_read_reg_obj_list(list, len, error));
}


/* Loads the spells from the text file */
void load_spells(void)
{
	FILE *fl = NULL;
	int temp, spellnum = 0, line_num = 0, tmp, tmp1, tmp2, tmp3, tmp4, new_format = 1, count;
	struct spell_info_type *sptr = NULL;
  struct spell_obj_data list[MAX_SPELL_OBJ + 1];
	bool news = FALSE;
	
	if ((fl = fopen(SPELL_FILE, "rt")) == NULL) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "No spell file found, re-creating.");
		create_spellfile();
		fl = fopen(SPELL_FILE, "rt");
	}
	
	line_num += get_line(fl, buf);
	if (sscanf(buf, "%d", &spellnum) != 1) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Format error in spell, line %d (number of spells)", line_num);
		kill_mysql();
		exit(1);
	}
	/* Setup the global total number of spells */
	spnum = spellnum;
	
	top_of_spellt = spnum;
	
	/* process each spell in order */
	for (spellnum = 0; spellnum < spnum; spellnum++) {
		/* Get the info for the individual spells */
		line_num += get_line(fl, buf);
		if (sscanf(buf, "#%d", &tmp) != 1) {
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Format error in spell (No Unique GID), line %d\n", line_num);
			kill_mysql();
			exit(1);
		}
		/* create some spell shaped memory space */
		if ((sptr = enqueue_spell()) != NULL) {
			sptr->number = tmp;
			
			/* setup the global number of next new spell number */
			if (!news) {
				if (sptr->number != spellnum) {
					newspell = spellnum;
					news = TRUE;
				}
			}

			if (news) {
				if (newspell == sptr->number) {
					newspell = spnum;
					news = FALSE;
				}
			} else
				newspell = sptr->number + 1;

			/*
			 * Read the strings in first.
			 */
			sptr->name = fread_string(fl, buf);
			sptr->long_name = fread_string(fl, buf);
			sptr->victim_msg = fread_string(fl, buf);
			sptr->wear_off_msg = fread_string(fl, buf);
			sptr->cast_msg = fread_string(fl, buf);
			sptr->mundane_msg = fread_string(fl, buf);
			sptr->target_msg = fread_string(fl, buf);

			/*
			 * Read reagents list.
			 */
      temp = read_reg_obj_list(sptr, fl, list, new_format, MAX_REAGENTS, LIST_REAGENTS);
      CREATE(sptr->reagents, struct spell_obj_data, temp);
			for (count = 0; count < temp; count++) {
				S_REG_TYPE(sptr, count) = SPELL_OBJ_TYPE(list[count]);
				S_REG_LOCATION(sptr, count) = SPELL_OBJ_LOCATION(list[count]);
				S_REG_EXTRACT(sptr, count) = SPELL_OBJ_EXTRACT(list[count]);
			}

			/*
			 * Read focuses list.
			 */
      temp = read_reg_obj_list(sptr, fl, list, new_format, MAX_FOCUSES, LIST_FOCUSES);
      CREATE(sptr->focuses, struct spell_obj_data, temp);
			for (count = 0; count < temp; count++) {
				S_FOC_TYPE(sptr, count) = SPELL_OBJ_TYPE(list[count]);
				S_FOC_LOCATION(sptr, count) = SPELL_OBJ_LOCATION(list[count]);
				S_FOC_EXTRACT(sptr, count) = SPELL_OBJ_EXTRACT(list[count]);
			}

			/*
			 * Read transmuters list.
			 */
      temp = read_reg_obj_list(sptr, fl, list, new_format, MAX_TRANSMUTERS, LIST_TRANSMUTERS);
      CREATE(sptr->transmuters, struct spell_obj_data, temp);
			for (count = 0; count < temp; count++) {
				S_TRA_TYPE(sptr, count) = SPELL_OBJ_TYPE(list[count]);
				S_TRA_LOCATION(sptr, count) = SPELL_OBJ_LOCATION(list[count]);
				S_TRA_EXTRACT(sptr, count) = SPELL_OBJ_EXTRACT(list[count]);
			}

			/* Now get mana_min, mana_max, min_position, and skill */
			line_num += get_line(fl, buf);
			if (sscanf(buf, "%d %d %d %d", &tmp, &tmp1, &tmp2, &tmp3) != 4) {
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Format error in spell, line %d (mana_min, mana_max, min_position, skill)\n", line_num);
				kill_mysql();
				exit(1);
			}
			sptr->mana_min = tmp;
			sptr->mana_max = tmp1;
			sptr->min_position = tmp2;
			sptr->skill = tmp3;

			/* Now read targets, violent, routines, learned */
			line_num += get_line(fl, buf);
			if (sscanf(buf, "%d %d %d %d", &tmp, &tmp1, &tmp2, &tmp3) != 4) {
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Format error in spell, line %d (targets, violent, routines, learned)\n", line_num);
				kill_mysql();
				exit(1);
			}
			sptr->targets = tmp;
			sptr->violent = tmp1;
			sptr->routines = tmp2;
			sptr->learned = tmp3;

			/* Now read technique, form, range, duration, and target_group */
			line_num += get_line(fl, buf);
			if (sscanf(buf, "%d %d %d %d %d", &tmp, &tmp1, &tmp2, &tmp3, &tmp4) != 5) {
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Format error in spell, line %d (technique, form, range, duration, target_group)\n", line_num);
				kill_mysql();
				exit(1);
			}
			sptr->technique = tmp;
			sptr->form = tmp1;
			sptr->range = tmp2;
			sptr->duration = tmp3;
			sptr->target_group = tmp4;

			/* Skip the divider token */
			line_num += get_line(fl, buf);

		} else break;
		/* process the next spell */
	}
	/* done processing spells -- close the file */
	mlog("   %d spells loaded.", top_of_spellt - 1);
	fclose(fl);
}


void save_spells(void)
{
	FILE *sfile;
	int spellnum = 0, j;
	struct spell_info_type *sptr = spell_info;
	
	if (sptr == NULL) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "No spells to save.");
		return;
	}
	if ((sfile = fopen(SPELL_FILE, "wt")) == NULL) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Cannot save spells.");
		kill_mysql();
		exit(1);
	}
	
	/* Put the spells in order */
	sort_spells();
	
	/* The total number of spells */
	fprintf(sfile, "%d\n", spnum);
	
	/* Save each spell */
	while (spellnum < spnum && sptr != NULL) {
		fprintf(sfile,  "#%d\n"
			"%s%c\n"
			"%s%c\n"
			"%s%c\n"
			"%s%c\n"
			"%s%c\n"
			"%s%c\n"
			"%s%c\n",
			sptr->number,
			sptr->name ? sptr->name : "Untitled", STRING_TERMINATOR,
			sptr->long_name ? sptr->long_name : "", STRING_TERMINATOR,
			sptr->victim_msg ? sptr->victim_msg : "", STRING_TERMINATOR,
			sptr->wear_off_msg ? sptr->wear_off_msg : "", STRING_TERMINATOR,
			sptr->cast_msg ? sptr->cast_msg : "", STRING_TERMINATOR,
			sptr->mundane_msg ? sptr->mundane_msg : "", STRING_TERMINATOR,
			sptr->target_msg ? sptr->target_msg : "", STRING_TERMINATOR);

		/*
		 * Save the reagents.
		 */
		j = -1;
		do {
			j++;
			fprintf(sfile, "%d %d %d\n",
				S_REG_TYPE(sptr, j),
				S_REG_LOCATION(sptr, j),
				S_REG_EXTRACT(sptr, j));
		} while (S_REG_TYPE(sptr, j) != -1);

		/*
		 * Save the focuses.
		 */
		j = -1;
		do {
			j++;
			fprintf(sfile, "%d %d %d\n",
				S_FOC_TYPE(sptr, j),
				S_FOC_LOCATION(sptr, j),
				S_FOC_EXTRACT(sptr, j));
		} while	(S_FOC_TYPE(sptr, j) != -1);


		/*
		 * Save the transmuters.
		 */
		j = -1;
		do {
			j++;
			fprintf(sfile, "%d %d %d\n",
				S_TRA_TYPE(sptr, j),
				S_TRA_LOCATION(sptr, j),
				S_TRA_EXTRACT(sptr, j));
		} while	(S_TRA_TYPE(sptr, j) != -1);

		/*
		 * Save down mana min/max, min_position, master skill,
		 * targets, violent, routines, and learned.
		 */
		fprintf(sfile,
			"%d %d %d %d\n"
			"%d %d %d %d\n"
			"%d %d %d %d %d\n",
			sptr->mana_min, sptr->mana_max, sptr->min_position, sptr->skill,
			sptr->targets, sptr->violent, sptr->routines, sptr->learned,
			sptr->technique, sptr->form, sptr->range, sptr->duration, sptr->target_group);

		fprintf(sfile, "$\n");

		/* process the next spell */
		sptr = sptr->next;
		spellnum++;
	}
	/* done processing spells */
	fclose(sfile);
}


/* 4/16/2001 - Zaquenin -
 * This function takes a string, 'input_string', which is the cmd line of the 
 * character, and a struct 'info' of type wild_spell_info.  It returns void, 
 * and as a result of the function, the 'info' struct's values are altered based on 
 * 'input_string'.  A byproduct of the function is that the global 'buf' is altered,
 * its contents replaced by those of 'input_string'.
 * 
 *  Uses strcasecmp and the different array constants rather than is_abbrev 
 *  as per Shammy's orders.
 *
 * Supporting files - constants.c, constants.h, spells.h
 * 
 */
void parse_wild_spell(const char *input_string, struct wild_spell_info *info)
{
	int	i;
	char *cur_arg = get_buffer(MAX_INPUT_LENGTH);
	char *printbuf = get_buffer(MAX_INPUT_LENGTH);

	strcpy(printbuf,input_string);
 
	half_chop(printbuf,cur_arg,printbuf); /* slice off the technique */
	info->tech = M_TECH_ERROR;
	for	(i = 0; i < NUM_SPELL_TECHNIQUES; i++)	{
		if (!strcasecmp(cur_arg,spell_techniques[i][0]) ||
				!strcasecmp(cur_arg,spell_techniques[i][1]) ||
				!strcasecmp(cur_arg,spell_techniques[i][2]))	{ 
			info->tech = i; /* will correspond to appropriate constant */
			break;
		}
	}

	half_chop(printbuf,cur_arg,printbuf); /* slice off the form */
	info->form = M_FORM_ERROR;
	for (i = 0; i < NUM_SPELL_FORMS; i++ ) {
		if (!strcasecmp(cur_arg,spell_forms[i][0]) || 
				!strcasecmp(cur_arg,spell_forms[i][1]) ||
				!strcasecmp(cur_arg,spell_forms[i][2])) { 
			info->form = i; /* will correspond to appropriate constant */
			break;
		}
	}


	half_chop(printbuf,cur_arg,printbuf); /* slice off the range */
	info->range = M_RANGE_ERROR;
	for (i = 0;	i < NUM_SPELL_RANGES;	i++) {
		if (!strcasecmp(cur_arg,spell_ranges[i][0]) || 
				!strcasecmp(cur_arg,spell_ranges[i][1])  ||
				!strcasecmp(cur_arg,spell_ranges[i][2])) { 
			info->range = i; /* will correspond to appropriate constant */
			break;
		}
	}


	half_chop(printbuf,cur_arg,printbuf); /* slice off the duration */
	info->duration = M_DUR_ERROR;
	for (i = 0;	i < NUM_SPELL_DURATIONS;	i++) {
		if (!strcasecmp(cur_arg,spell_duration[i][0]) || 
				!strcasecmp(cur_arg,spell_duration[i][1])  ||
				!strcasecmp(cur_arg,spell_duration[i][2]))	{ 
			info->duration = i; /* will correspond to appropriate constant */
			break;
		}
	}

	half_chop(printbuf,cur_arg,printbuf); /* slice off the technique */
	info->target_group = M_TAR_ERROR;
	for (i = 0; i < NUM_SPELL_TARGET_GROUPS; i++) {
		if (!strcasecmp(cur_arg,spell_target_groups[i][0]) || 
				!strcasecmp(cur_arg,spell_target_groups[i][1]) ||
				!strcasecmp(cur_arg,spell_target_groups[i][2])) { 
			info->target_group = i; /* will correspond to appropriate constant */
			break;
		}
	}

	half_chop(printbuf,cur_arg,printbuf); /* slice off the technique */
	info->seed = M_SEED_ERROR;
	for (i = 0; i < NUM_SPELL_SEED_TYPES; i++) {
		if (!strcasecmp(cur_arg,spell_seed_types[i][0]) || 
				!strcasecmp(cur_arg,spell_seed_types[i][1]) ||
				!strcasecmp(cur_arg,spell_seed_types[i][2])) { 
			info->seed = i; /* will correspond to appropriate constant */
			break;
		}
	}

	release_buffer(printbuf);
	release_buffer(cur_arg);

}
