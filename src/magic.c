/* ************************************************************************
*		File: magic.c                                       Part of CircleMUD *
*	 Usage: low-level functions for magic; spell template code              *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: magic.c,v 1.14 2002/12/10 13:08:30 arcanere Exp $ */


#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include "specset.h"

/* external variables */
extern int mini_mud;
extern int pk_allowed;

extern int sleep_allowed;
extern int summon_allowed;
extern int charm_allowed;
extern int roomaffect_allowed;

extern int top_of_spellt;

void clearMemory(struct char_data *ch);
void weight_change_object(struct obj_data *obj, int weight);
void add_follower(struct char_data *ch, struct char_data *leader);
extern struct spell_info_type *spell_info;
extern int calc_saving_throw(struct char_data *ch, struct char_data *victim, int save_type);
extern struct save_modifier_type save_modifier[];

/* local functions */
int	mag_materials(struct char_data *ch, int item0, int item1, int item2, int extract, int verbose);
void perform_mag_groups(int level, struct char_data *ch, struct char_data *tch, struct spell_info_type *sptr, int savetype);
int	mag_savingthrow(struct char_data *ch, struct char_data *victim, int type);
void affect_update(void);

/*
 * Saving throws are now in class.c as of bpl13.
 */


/*
 * Negative apply_saving_throw[] values make saving throws better!
 * Then, so do negative modifiers.  Though people may be used to
 * the reverse of that. It's due to the code modifying the target
 * saving throw instead of the random number of the character as
 * in some other systems.
 */
int	mag_savingthrow(struct char_data *ch, struct char_data *victim, int type)
{
	int save;
	
	save = calc_saving_throw(ch,victim,type);
	save += GET_SAVE(ch, type);
	
	/* throwing a 0 is always a failure */
	if (MAX(1, save) < number(0, 99))
		return TRUE;
	else
		return FALSE;
}


/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
	struct spell_info_type *sptr;
	struct affected_type *af, *next;
	struct char_data *i;
	
	for (i = character_list; i; i = i->next)
		for (af = i->affected; af; af = next) {
			next = af->next;
			if (af->duration >= 1)
				af->duration--;
			else if (af->duration == -1)	/* No action */
				af->duration = -1;	/* GODs only! unlimited */
			else {
				if ((af->type > 0) && (af->type <= top_of_spellt)) {
					if (!af->next || (af->next->type != af->type) || (af->next->duration > 0)) {
						for (sptr = spell_info; sptr; sptr = sptr->next)
							if (sptr->number == af->type)
								break;
						if (sptr->wear_off_msg) {
							char *wearoffmsg = get_buffer(256);
							sprintf(wearoffmsg, "AFFECTS: &W%s&n\r\n", sptr->wear_off_msg);
							send_to_char(wearoffmsg, i);
							release_buffer(wearoffmsg);
						}
					}
				}
				affect_remove(i, af);
			}
		}
}


/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int	mag_materials(struct char_data *ch, int item0, int item1, int item2,
					int extract, int verbose)
{
	struct obj_data *tobj;
	struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;

	for (tobj = ch->carrying; tobj; tobj = tobj->next_content) {
		if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0)) {
			obj0 = tobj;
			item0 = -1;
		} else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1)) {
			obj1 = tobj;
			item1 = -1;
		} else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2)) {
			obj2 = tobj;
			item2 = -1;
		}
	}
	if ((item0 > 0) || (item1 > 0) || (item2 > 0)) {
		if (verbose) {
			switch (number(0, 2)) {
			case 0:
				send_to_char("A wart sprouts on your nose.\r\n", ch);
				break;
			case 1:
				send_to_char("Your hair falls out in clumps.\r\n", ch);
				break;
			case 2:
				send_to_char("A huge corn develops on your big toe.\r\n", ch);
				break;
			}
		}
		return (FALSE);
	}
	if (extract) {
		if (item0 < 0)
			extract_obj(obj0);
		if (item1 < 0)
			extract_obj(obj1);
		if (item2 < 0)
			extract_obj(obj2);
	}
	if (verbose) {
		send_to_char("A puff of smoke rises from your pack.\r\n", ch);
		act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
	}
	return (TRUE);
}




/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */
int	mag_damage(int level, struct char_data *ch, struct char_data *victim,
				 struct spell_info_type *sptr, int savetype)
{
	int dam = 0;

	if (victim == NULL || ch == NULL)
		return (0);

	switch (sptr->number) {
		/* Mostly mages */
	case SPELL_MAGIC_MISSILE:
	case SPELL_CHILL_TOUCH:	/* chill touch also has an affect */
		if (IS_MAGI_TYPE(ch))
			dam = dice(1, 8) + 1;
		else
			dam = dice(1, 6) + 1;
		break;
	case SPELL_BURNING_HANDS:
		if (IS_MAGI_TYPE(ch))
			dam = dice(3, 8) + 3;
		else
			dam = dice(3, 6) + 3;
		break;
	case SPELL_SHOCKING_GRASP:
		if (IS_MAGI_TYPE(ch))
			dam = dice(5, 8) + 5;
		else
			dam = dice(5, 6) + 5;
		break;
	case SPELL_LIGHTNING_BOLT:
		if (IS_MAGI_TYPE(ch))
			dam = dice(7, 8) + 7;
		else
			dam = dice(7, 6) + 7;
		break;
	case SPELL_COLOR_SPRAY:
		if (IS_MAGI_TYPE(ch))
			dam = dice(9, 8) + 9;
		else
			dam = dice(9, 6) + 9;
		break;
	case SPELL_FIREBALL:
		if (IS_MAGI_TYPE(ch))
			dam = dice(11, 8) + 11;
		else
			dam = dice(11, 6) + 11;
		break;

		/* Mostly clerics */
	case SPELL_DISPEL_EVIL:
		dam = dice(6, 8) + 6;
		if (IS_EVIL(ch)) {
			victim = ch;
			dam = GET_HIT(ch) - 1;
		} else if (IS_GOOD(victim)) {
			act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
			return (0);
		}
		break;
	case SPELL_DISPEL_GOOD:
		dam = dice(6, 8) + 6;
		if (IS_GOOD(ch)) {
			victim = ch;
			dam = GET_HIT(ch) - 1;
		} else if (IS_EVIL(victim)) {
			act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
			return (0);
		}
		break;

	case SPELL_DIVINE_ARROW:
		if (IS_PALADIN(ch))
			dam = dice(1, 8) + 1;
		else
			dam = dice(1, 6) + 1;
		break;

	case SPELL_CALL_LIGHTNING:
		dam = dice(7, 8) + 7;
		break;

	case SPELL_HARM:
		dam = dice(8, 8) + 8;
		break;

	case SPELL_ENERGY_DRAIN:
		dam = dice(1, 10);
		break;

		/* Area spells */
	case SPELL_EARTHQUAKE:
		dam = dice(2, 8) + level;
		break;

	case SPELL_GALEFORCE:
		dam = dice(2, 8);
		break;

	} /* switch(sptr->number) */


	/* divide damage by two if victim makes his saving throw */
	if (mag_savingthrow(ch, victim, savetype))
		dam /= 2;

	/* and finally, inflict the damage */
	return (damage(ch, victim, dam, sptr->number));
}


/*
 * Every spell that does an affect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
 */

#define	MAX_SPELL_AFFECTS 5	/* change if more needed */

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
					struct spell_info_type *sptr, int savetype)
{
	struct affected_type af[MAX_SPELL_AFFECTS];
	bool accum_affect = FALSE, accum_duration = FALSE;
	const char *to_vict = NULL, *to_room = NULL;
	int i;


	if (victim == NULL || ch == NULL)
		return;

	for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
		af[i].type = sptr->number;
		af[i].bitvector = 0;
		af[i].modifier = 0;
		af[i].location = APPLY_NONE;
	}

	switch (sptr->number) {

	case SPELL_CHILL_TOUCH:
		af[0].location = APPLY_STRENGTH;
		if (mag_savingthrow(ch, victim, savetype))
			af[0].duration = 1;
		else
			af[0].duration = 4;
		af[0].modifier = -1;
		accum_duration = TRUE;
		to_vict = "You feel your strength wither!";
		break;

	case SPELL_ARMOR:
		af[0].location = APPLY_AC;
		af[0].modifier = -20;
		af[0].duration = 24;
		accum_duration = TRUE;
		to_vict = "You feel someone protecting you.";
		break;

	case SPELL_BLESS:
		af[0].location = APPLY_HITROLL;
		af[0].modifier = 2;
		af[0].duration = 6;

		af[1].location = APPLY_SAVING_SPELL;
		af[1].modifier = -1;
		af[1].duration = 6;

		accum_duration = TRUE;
		to_vict = "You feel righteous.";
		break;

	case SPELL_BLINDNESS:
		if (MOB_FLAGGED(victim,MOB_NOBLIND) || mag_savingthrow(ch, victim, savetype)) {
			send_to_char("You fail.\r\n", ch);
			return;
		}

		af[0].location = APPLY_HITROLL;
		af[0].modifier = -4;
		af[0].duration = 2;
		af[0].bitvector = AFF_BLIND;

		af[1].location = APPLY_AC;
		af[1].modifier = 40;
		af[1].duration = 2;
		af[1].bitvector = AFF_BLIND;

		to_room = "$n seems to be blinded!";
		to_vict = "You have been blinded!";
		break;

	case SPELL_CURSE:
		if (mag_savingthrow(ch, victim, savetype)) {
			send_to_char(NOEFFECT, ch);
			return;
		}

		af[0].location = APPLY_HITROLL;
		af[0].duration = 1 + ((GET_SKILL(ch, SPELL_CURSE)/100) / 6);
		af[0].modifier = -1;
		af[0].bitvector = AFF_CURSE;

		af[1].location = APPLY_DAMROLL;
		af[1].duration = 1 + ((GET_SKILL(ch, SPELL_CURSE)/100) / 6);
		af[1].modifier = -1;
		af[1].bitvector = AFF_CURSE;

		accum_duration = TRUE;
		accum_affect = TRUE;
		to_room = "$n briefly glows red!";
		to_vict = "You feel very uncomfortable.";
		break;

	case SPELL_DETECT_ALIGN:
		af[0].duration = 12 + (level / 3);
		af[0].bitvector = AFF_DETECT_ALIGN;
		accum_duration = TRUE;
		to_vict = "Your eyes tingle.";
		break;

	case SPELL_DETECT_INVIS:
		af[0].duration = 12 + (level / 3);
		af[0].bitvector = AFF_DETECT_INVIS;
		accum_duration = TRUE;
		to_vict = "Your eyes tingle.";
		break;

	case SPELL_DETECT_MAGIC:
		af[0].duration = 12 + (level / 3);
		af[0].bitvector = AFF_DETECT_MAGIC;
		accum_duration = TRUE;
		to_vict = "Your eyes tingle.";
		break;

	case SPELL_INFRAVISION:
		af[0].duration = 12 + (level / 3);
		af[0].bitvector = AFF_INFRAVISION;
		accum_duration = TRUE;
		to_vict = "Your eyes glow red.";
		to_room = "$n's eyes glow red.";
		break;

	case SPELL_INVISIBLE:
		if (!victim)
			victim = ch;

		af[0].duration = 12 + ((GET_SKILL(ch, SPELL_INVISIBLE)/100) / 13);
		af[0].modifier = -40;
		af[0].location = APPLY_AC;
		af[0].bitvector = AFF_INVISIBLE;
		accum_duration = TRUE;
		to_vict = "You vanish.";
		to_room = "$n slowly fades out of existence.";
		break;

	case SPELL_POISON:
		if (mag_savingthrow(ch, victim, savetype)) {
			send_to_char(NOEFFECT, ch);
			return;
		}

		af[0].location = APPLY_STRENGTH;
		af[0].duration = (GET_SKILL(ch, SPELL_POISON)/100);
		af[0].modifier = -2;
		af[0].bitvector = AFF_POISON;
		to_vict = "You feel very sick.";
		to_room = "$n gets violently ill!";
		break;

	case SPELL_PROT_FROM_EVIL:
		af[0].duration = 24;
		af[0].bitvector = AFF_PROTECT_EVIL;
		accum_duration = TRUE;
		to_vict = "You feel invulnerable!";
		break;

	case SPELL_SANCTUARY:
		if (IS_AFFECTED(ch, AFF_ORB)) {
			send_to_char("You cannot use orb and sanctuary at the same time.\r\n", ch);
			return;
		}
		af[0].duration = 4;
		af[0].bitvector = AFF_SANCTUARY;

		accum_duration = TRUE;
		to_vict = "A white aura momentarily surrounds you.";
		to_room = "$n is surrounded by a white aura.";
		break;

	case SPELL_SLEEP:
		if (!sleep_allowed && !IS_NPC(ch) && !IS_NPC(victim))
			return;
		if (MOB_FLAGGED(victim, MOB_NOSLEEP))
			return;
		if (mag_savingthrow(ch, victim, savetype))
			return;

		af[0].duration = 4 + ((GET_SKILL(ch, SPELL_SLEEP)/100) / 13);
		af[0].bitvector = AFF_SLEEP;

		if (GET_POS(victim) > POS_SLEEPING) {
			send_to_char("You feel very sleepy...  Zzzz......\r\n", victim);
			act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
			GET_POS(victim) = POS_SLEEPING;
		}
		break;

	case SPELL_STRENGTH:
		af[0].location = APPLY_STRENGTH;
		af[0].duration = ((GET_SKILL(ch, SPELL_STRENGTH)/100) / 6) + 4;
		af[0].modifier = 1 + (level > 18);
		accum_duration = TRUE;
		accum_affect = TRUE;
		to_vict = "You feel stronger!";
		break;

	case SPELL_SENSE_LIFE:
		to_vict = "Your feel your awareness improve.";
		af[0].duration = (GET_SKILL(ch, SPELL_SENSE_LIFE)/100) / 3;
		af[0].bitvector = AFF_SENSE_LIFE;
		accum_duration = TRUE;
		break;

	case SPELL_WATERWALK:
		af[0].duration = 24;
		af[0].bitvector = AFF_WATERWALK;
		accum_duration = TRUE;
		to_vict = "You feel webbing between your toes.";
		break;

	/* additions go below here */

	case SPELL_BARKSKIN:
		if (IS_AFFECTED(ch, AFF_STONESKIN | AFF_STEELSKIN)) {
			send_to_char("You can only use one skin spell at a time.\r\n", ch);
			return;
		}
		af[0].location = APPLY_AC;
		af[0].modifier = -20 - (IS_CLERIC_TYPE(ch) * 10);
		af[0].duration = level / 5;
		af[0].bitvector = AFF_BARKSKIN;
		to_vict = "You feel your skin hardening.";
		break;

	case SPELL_STONESKIN:
		if (IS_AFFECTED(ch, AFF_BARKSKIN | AFF_STEELSKIN)) {
			send_to_char("You can only use one skin spell at a time.\r\n", ch);
			return;
		}
		af[0].location = APPLY_AC;
		af[0].modifier = -25;
		af[0].duration = level / 3;
		af[0].bitvector = AFF_STONESKIN;
		accum_duration = TRUE;
		to_vict = "You feel your skin turn into granite.";
		break;
		
	case SPELL_STEELSKIN:
		if (IS_AFFECTED(ch, AFF_BARKSKIN | AFF_STONESKIN)) {
			send_to_char("You can only use one skin spell at a time.\r\n", ch);
			return;
		}
		af[0].location = APPLY_AC;
		af[0].modifier = -20 - (GET_CHARISMA(ch) / 100);
		af[0].duration = level / 3;
		af[0].bitvector = AFF_STEELSKIN;
		
		af[1].location = APPLY_SAVING_SPELL;
		af[1].modifier = -8;
		af[1].duration = level / 3;
		
		accum_duration = TRUE;
		to_vict = "You feel your skin become steel!";
		break;

	case SPELL_SHROUD:
		if (MOB_FLAGGED(victim,MOB_NOSHROUD) || mag_savingthrow(ch, victim, savetype)) {
			send_to_char("You fail.\r\n", ch);
			return;
		}
		af[0].location = APPLY_AC;
		af[0].modifier = level / 3;
		af[0].duration = 3;
		af[0].bitvector = AFF_SHROUD;
		accum_duration = TRUE;
		to_room = "$n has been shrouded, and cannot move!";
		to_vict = "You have been shrouded, and cannot move!";
		break;

	case SPELL_ORB:
		if (IS_AFFECTED(ch, AFF_SANCTUARY)) {
			send_to_char("You cannot use orb and sanctuary at the same time.\r\n", ch);
			return;
		}
		af[0].duration = 4;
		af[0].bitvector = AFF_ORB;

		accum_duration = TRUE;
		to_vict = "A pale white aura momentarily surrounds you.";
		to_room = "$n is surrounded by a pale white aura.";
		break;

	/* CLASS ADDITIONS - NEW AFFECT SPELLS */

	case SPELL_HASTE:
		af[0].location = APPLY_MOVE;
		af[0].modifier = level / 3 + (GET_AGILITY(ch) / 100);
		af[0].duration = level / 2;
		af[0].bitvector = AFF_HASTE;

		to_vict = "A surge of energy flow through you, and you feel like you could run forever!";
		break;

	}

	/*
	 * If this is a mob that has this affect set in its mob file, do not
	 * perform the affect.  This prevents people from un-sancting mobs
	 * by sancting them and waiting for it to fade, for example.
	 */
	if (IS_NPC(victim) && !affected_by_spell(victim, sptr->number))
		for (i = 0; i < MAX_SPELL_AFFECTS; i++)
			if (AFF_FLAGGED(victim, af[i].bitvector)) {
				send_to_char(NOEFFECT, ch);
				return;
			}

	/*
	 * If the victim is already affected by this spell, and the spell does
	 * not have an accumulative effect, then fail the spell.
	 */
	if (affected_by_spell(victim, sptr->number) && !(accum_duration||accum_affect)) {
		send_to_char(NOEFFECT, ch);
		return;
	}

	for (i = 0; i < MAX_SPELL_AFFECTS; i++)
		if (af[i].bitvector || (af[i].location != APPLY_NONE))
			affect_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE);

	if (to_vict != NULL) {
		char *spellmsg = get_buffer(1024);
		sprintf(spellmsg, "AFFECTS: &W%s&n", to_vict);
		act(spellmsg, FALSE, victim, 0, ch, TO_CHAR);
		release_buffer(spellmsg);
	}

	if (to_room != NULL)
		act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}


/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */

void perform_mag_groups(int level, struct char_data *ch,
			struct char_data *tch, struct spell_info_type *sptr, int savetype)
{
	struct spell_info_type *spell;
	
	switch (sptr->number) {
		case SPELL_GROUP_HEAL:
		spell = get_spell(SPELL_HEAL, __FILE__, __FUNCTION__);
		mag_points(level, ch, tch, spell, savetype);
		break;
	case SPELL_GROUP_ARMOR:
		spell = get_spell(SPELL_ARMOR, __FILE__, __FUNCTION__);
		mag_affects(level, ch, tch, spell, savetype);
		break;
	case SPELL_GROUP_RECALL:
		spell_recall(level, ch, tch, NULL, 0);
		break;
	}
}


/*
 * Every spell that affects the group should run through here
 * perform_mag_groups contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_groups.
 */

void mag_groups(int level, struct char_data *ch, struct spell_info_type *sptr, int savetype)
{
	struct char_data *tch, *k;
	struct follow_type *f, *f_next;

	if (ch == NULL)
		return;

	if (!AFF_FLAGGED(ch, AFF_GROUP))
		return;
	if (ch->master != NULL)
		k = ch->master;
	else
		k = ch;
	for (f = k->followers; f; f = f_next) {
		f_next = f->next;
		tch = f->follower;
		if (IN_ROOM(tch) != IN_ROOM(ch))
			continue;
		if (!AFF_FLAGGED(tch, AFF_GROUP))
			continue;
		if (ch == tch)
			continue;
		perform_mag_groups(level, ch, tch, sptr, savetype);
	}

	if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
		perform_mag_groups(level, ch, k, sptr, savetype);
	perform_mag_groups(level, ch, ch, sptr, savetype);
}


/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented as of Circle 3.0.
 */

void mag_masses(int level, struct char_data *ch, struct spell_info_type *sptr, int savetype)
{
	struct char_data *tch, *tch_next;

	for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next) {
		tch_next = tch->next_in_room;
		if (tch == ch)
			continue;

		switch (sptr->number) {
		}
	}
}


/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
 */

void mag_areas(int level, struct char_data *ch, struct spell_info_type *sptr, int savetype)
{
	struct char_data *tch, *next_tch;
	const char *to_char = NULL, *to_room = NULL;

	if (ch == NULL)
		return;

	/*
	 * to add spells to this fn, just add the message here plus an entry
	 * in mag_damage for the damaging part of the spell.
	 */
	switch (sptr->number) {
	case SPELL_EARTHQUAKE:
		to_char = "You gesture and the earth begins to shake all around you!";
		to_room ="$n gracefully gestures and the earth begins to shake violently!";
		break;
	case SPELL_GALEFORCE:
		to_char = "You unleash the gale force winds from your palms!";
		to_room ="$n unleash the gale force winds from $s empty hands!";
		break;
	}

	if (to_char != NULL)
		act(to_char, FALSE, ch, 0, 0, TO_CHAR);
	if (to_room != NULL)
		act(to_room, FALSE, ch, 0, 0, TO_ROOM);
	

	for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
		next_tch = tch->next_in_room;

		/*
		 * The skips: 1: the caster
		 *            2: immortals
		 *            3: if no pk on this mud, skips over all players
		 *            4: pets (charmed NPCs)
		 */

		if (tch == ch)
			continue;
		if (!IS_NPC(tch) && IS_IMMORTAL(tch))
			continue;
		if (!roomaffect_allowed && !IS_NPC(ch) && !IS_NPC(tch))
			continue;
		if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
			continue;

		/* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
		mag_damage(level, ch, tch, sptr, 1);
	}
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 *
 *  None of these spells are currently implemented in Circle 3.0; these
 *  were taken as examples from the JediMUD code.  Summons can be used
 *  for spells like clone, ariel servant, etc.
 *
 * 10/15/97 (gg) - Implemented Animate Dead and Clone.
 */

/*
 * These use act(), don't put the \r\n.
 */
const	char *mag_summon_msgs[] = {
	"\r\n",
	"$n makes a strange magical gesture; you feel a strong breeze!",
	"$n animates a corpse!",
	"$N appears from a cloud of thick blue smoke!",
	"$N appears from a cloud of thick green smoke!",
	"$N appears from a cloud of thick red smoke!",
	"$N disappears in a thick black cloud!"
	"As $n makes a strange magical gesture, you feel a strong breeze.",
	"As $n makes a strange magical gesture, you feel a searing heat.",
	"As $n makes a strange magical gesture, you feel a sudden chill.",
	"As $n makes a strange magical gesture, you feel the dust swirl.",
	"$n magically divides!",
	"$n animates a corpse!",
	"$n summons a fire imp!",
	"$n summons a giant fire daemon!"
};

/*
 * Keep the \r\n because these use send_to_char.
 */
const	char *mag_summon_fail_msgs[] = {
	"\r\n",
	"There are no such creatures.\r\n",
	"Uh oh...\r\n",
	"Oh dear.\r\n",
	"Gosh durnit!\r\n",
	"The elements resist!\r\n",
	"You failed.\r\n",
	"There is no corpse!\r\n"
};

/* These mobiles do not exist. */
#define	MOB_MONSUM_I      130
#define	MOB_MONSUM_II     140
#define	MOB_MONSUM_III    150
#define	MOB_GATE_I        160
#define	MOB_GATE_II       170
#define	MOB_GATE_III      180

/* Defined mobiles. */
#define	MOB_ELEMENTAL_BASE  20	/* Only one for now. */
#define	MOB_CLONE           10
#define	MOB_ZOMBIE          11
#define	MOB_AERIALSERVANT   19
/* ADDITIONS */
#define	MOB_FIRE_IMP        12
#define	MOB_FIRE_DAEMON     13


void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
								 struct spell_info_type *sptr, int savetype)
{
	struct char_data *mob = NULL;
	struct obj_data *tobj, *next_obj;
	struct affected_type af;
	int pfail = 0, msg = 0, fmsg = 0, num = 1, handle_corpse = FALSE, i;
	mob_vnum mob_num;
	
	if (ch == NULL)
		return;
	
	switch (sptr->number) {
	case SPELL_CLONE:
		msg = 10;
		fmsg = number(2, 6);	/* Random fail message. */
		mob_num = MOB_CLONE;
		pfail = 50;	/* 50% failure, should be based on something later. */
		break;
		
	case SPELL_ANIMATE_DEAD:
		if (obj == NULL || !IS_CORPSE(obj)) {
			act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
		handle_corpse = TRUE;
		msg = 11;
		fmsg = number(2, 6);	/* Random fail message. */
		mob_num = MOB_ZOMBIE;
		pfail = 10;	/* 10% failure, should vary in the future. */
		break;
		
	case SPELL_FIRE_IMP:
		msg = 12;
		fmsg = number(2, 6);	/* Random fail message. */
		mob_num = MOB_FIRE_IMP;
		pfail = 10;	/* 10% failure, should vary in the future. */
		break;
		
	case SPELL_FIRE_DAEMON:
		msg = 13;
		fmsg = number(2, 6);	/* Random fail message. */
		mob_num = MOB_FIRE_DAEMON;
		pfail = 10;	/* 10% failure, should vary in the future. */
		break;
		
	default:
		return;
	}
	
	if (AFF_FLAGGED(ch, AFF_CHARM)) {
		send_to_char("You are too giddy to have any followers!\r\n", ch);
		return;
	}
	if (number(0, 101) < pfail) {
		send_to_char(mag_summon_fail_msgs[fmsg], ch);
		return;
	}
	for (i = 0; i < num; i++) {
		if (!(mob = read_mobile(mob_num, VIRTUAL))) {
			send_to_char("You don't quite remember how to make that creature.\r\n", ch);
			return;
		}
		char_to_room(mob, IN_ROOM(ch));
		IS_CARRYING_W(mob) = 0;
		IS_CARRYING_N(mob) = 0;

		af.type = SPELL_CHARM;

		if (GET_INTELLIGENCE(mob))
			af.duration = 3 * 18 / GET_INTELLIGENCE(mob);
		else
			af.duration = 3 * 18;

		af.modifier = 0;
		af.location = 0;
		af.bitvector = AFF_CHARM;
		affect_to_char(mob, &af);
		ASSIGNMOB(GET_MOB_VNUM(mob), summoned);

		if (sptr->number == SPELL_CLONE) {	/* Don't mess up the proto with strcpy. */
			mob->player.name = str_dup(GET_NAME(ch));
			mob->player.short_descr = str_dup(GET_NAME(ch));
		}
		act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
		load_mtrigger(mob);
		add_follower(mob, ch);
	}
	if (handle_corpse) {
		for (tobj = obj->contains; tobj; tobj = next_obj) {
			next_obj = tobj->next_content;
			obj_from_obj(tobj);
			obj_to_char(tobj, mob);
		}
		extract_obj(obj);
	}
}


void mag_points(int level, struct char_data *ch, struct char_data *victim,
				 struct spell_info_type *sptr, int savetype)
{
	int healing = 0, move = 0;

	if (victim == NULL)
		return;

	switch (sptr->number) {
	case SPELL_CURE_LIGHT:
		healing = dice(1, 8) + 1 + (level / 4);
		send_to_char("You feel better.\r\n", victim);
		break;
	case SPELL_CURE_CRITIC:
		healing = dice(3, 8) + 3 + (level / 4);
		send_to_char("You feel a lot better!\r\n", victim);
		break;
	case SPELL_HEAL:
		healing = 100 + dice(3, 8);
		send_to_char("A warm feeling floods your body.\r\n", victim);
		break;
	}
	GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + healing);
	GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move);
	update_pos(victim);
}


void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
						struct spell_info_type *sptr, int type)
{
	int spell = 0;
	const char *to_vict = NULL, *to_room = NULL;

	if (victim == NULL)
		return;

	switch (sptr->number) {
	case SPELL_CURE_BLIND:
	case SPELL_HEAL:
		spell = SPELL_BLINDNESS;
		to_vict = "Your vision returns!";
		to_room = "There's a momentary gleam in $n's eyes.";
		break;
	case SPELL_REMOVE_POISON:
		spell = SPELL_POISON;
		to_vict = "A warm feeling runs through your body!";
		to_room = "$n looks better.";
		break;
	case SPELL_REMOVE_CURSE:
		spell = SPELL_CURSE;
		to_vict = "You don't feel so unlucky.";
		break;
	default:
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "unknown spellnum %d passed to mag_unaffects.", sptr->number);
		return;
	}

	if (!affected_by_spell(victim, spell)) {
		if (sptr->number != SPELL_HEAL)		/* 'cure blindness' message. */
			send_to_char(NOEFFECT, ch);
		return;
	}

	affect_from_char(victim, spell);
	if (to_vict != NULL)
		act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
	if (to_room != NULL)
		act(to_room, TRUE, victim, 0, ch, TO_ROOM);

}


void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
						 struct spell_info_type *sptr, int savetype)
{
	const char *to_char = NULL, *to_room = NULL;

	if (obj == NULL)
		return;

	switch (sptr->number) {
		case SPELL_BLESS:
			if (!OBJ_FLAGGED(obj, ITEM_BLESS) &&
		(GET_OBJ_WEIGHT(obj) <= 5 * (GET_SKILL(ch, SPELL_BLESS)/100))) {
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_BLESS);
	to_char = "$p glows briefly.";
			}
			break;
		case SPELL_CURSE:
			if (!OBJ_FLAGGED(obj, ITEM_NODROP)) {
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP);
	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
		GET_OBJ_VAL(obj, 2)--;
	to_char = "$p briefly glows red.";
			}
			break;
		case SPELL_INVISIBLE:
			if (!OBJ_FLAGGED(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
				SET_BIT(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
				to_char = "$p vanishes.";
			}
			break;
		case SPELL_POISON:
			if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
				 (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
				 (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3)) {
			GET_OBJ_VAL(obj, 3) = 1;
			to_char = "$p steams briefly.";
			}
			break;
		case SPELL_REMOVE_CURSE:
			if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
				REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP);
				if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
					GET_OBJ_VAL(obj, 2)++;
				to_char = "$p briefly glows blue.";
			}
			break;
		case SPELL_REMOVE_POISON:
			if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
				 (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
				 (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, 3)) {
				GET_OBJ_VAL(obj, 3) = 0;
				to_char = "$p steams briefly.";
			}
			break;
	}

	if (to_char == NULL)
		send_to_char(NOEFFECT, ch);
	else
		act(to_char, TRUE, ch, obj, 0, TO_CHAR);

	if (to_room != NULL)
		act(to_room, TRUE, ch, obj, 0, TO_ROOM);
	else if (to_char != NULL)
		act(to_char, TRUE, ch, obj, 0, TO_ROOM);

}



void mag_creations(int level, struct char_data *ch, struct spell_info_type *sptr)
{
	struct obj_data *tobj;
	obj_vnum z;

	if (ch == NULL)
		return;

	switch (sptr->number) {
	case SPELL_CREATE_FOOD:
		z = 10;
		break;
	default:
		send_to_char("Spell unimplemented, it would seem.\r\n", ch);
		return;
	}

	if (!(tobj = read_object(z, VIRTUAL))) {
		send_to_char("I seem to have goofed.\r\n", ch);
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "spell_creations, spell %d, obj %d: obj not found",
			sptr->number, z);
		return;
	}
	obj_to_char(tobj, ch);
	act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
	act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
	load_otrigger(tobj);
}

