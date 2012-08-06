/* ************************************************************************
*		File: spells.c                                      Part of CircleMUD *
*	 Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: spells.c,v 1.19 2002/12/31 01:00:19 arcanere Exp $ */

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
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "loadrooms.h"

/* external variables */
extern sh_int mortal_start_room[NUM_STARTROOMS +1];

extern int mini_mud;
extern int pk_allowed;

extern int summon_allowed;
extern int sleep_allowed;
extern int charm_allowed;
extern int roomaffect_allowed;

void clearMemory(struct char_data * ch);
void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
int	mag_savingthrow(struct char_data * ch, struct char_data *victim, int type);
void name_to_drinkcon(struct obj_data * obj, int type);
void name_from_drinkcon(struct obj_data * obj);
int	compute_armor_class(struct char_data *ch);

/*
 * Special spells appear below.
 */

ASPELL(spell_create_water)
{
	int water;
	
	if (ch == NULL || obj == NULL)
		return;
	
	if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
		if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
			name_from_drinkcon(obj);
			GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
			name_to_drinkcon(obj, LIQ_SLIME);
		} else {
			water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
			if (water > 0) {
				if (GET_OBJ_VAL(obj, 1) >= 0)
					name_from_drinkcon(obj);
				GET_OBJ_VAL(obj, 2) = LIQ_WATER;
				GET_OBJ_VAL(obj, 1) += water;
				name_to_drinkcon(obj, LIQ_WATER);
				weight_change_object(obj, water);
				act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
			}
		}
	}
}


ASPELL(spell_recall)
{
	if (victim == NULL || IS_NPC(victim))
		return;

	if (ZONE_FLAGGED(IN_ZONE(ch), ZONE_NORECALL)) {
		send_to_char("\r\n&REldritch wizardry obstructs thee from using the holy Recall...&n\r\n"
			"  Perhaps it is better if you just remain where you are.\r\n",ch);
		return;
	}

	act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, real_room(mortal_start_room[GET_HOME(ch)]));
	act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, 0);
	entry_memory_mtrigger(ch);
	greet_mtrigger(ch, -1);
	greet_memory_mtrigger(ch);
}


ASPELL(spell_teleport)
{
	room_rnum to_room;

	if (victim == NULL || IS_NPC(victim))
		return;

	do {
		to_room = number(0, top_of_world);
	} while (ROOM_FLAGGED(to_room, ROOM_PRIVATE | ROOM_DEATH | ROOM_GODROOM));

	act("$n slowly fades out of existence and is gone.",
			FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, to_room);
	act("$n slowly fades into existence.", FALSE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, 0);
	entry_memory_mtrigger(ch);
	greet_mtrigger(ch, -1);
	greet_memory_mtrigger(ch);
}

#define	SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_summon)
{
	if (ch == NULL || victim == NULL)
		return;

	if (IS_IMMORTAL(victim)) {
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	if (ZONE_FLAGGED(IN_ZONE(victim), ZONE_NOSUMMON)) {
		sprintf(buf, "%s just tried to summon you to: %s.\r\n"
								 "%s failed because you are in a no summon zone.\r\n",
			GET_NAME(ch), world[IN_ROOM(ch)].name,
			(ch->player.sex == SEX_MALE) ? "He" : "She");
		send_to_char(buf, victim);
		
		sprintf(buf, "You failed because %s is in a no summon zone.\r\n",
			GET_NAME(victim));
		send_to_char(buf, ch);
		
		extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s failed summoning %s to %s.",
			GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
		return;
	}

	if (!summon_allowed) {
		if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
			act("As the words escape your lips and $N travels\r\n"
					"through time and space towards you, you realize that $E is\r\n"
					"aggressive and might harm you, so you wisely send $M back.",
				FALSE, ch, 0, victim, TO_CHAR);
			return;
		}
		if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
			!PLR_FLAGGED(victim, PLR_KILLER)) {
			sprintf(buf, "%s just tried to summon you to: %s.\r\n"
									 "%s failed because you have summon protection on.\r\n"
									 "Type NOSUMMON to allow other players to summon you.\r\n",
				GET_NAME(ch), world[IN_ROOM(ch)].name,
				(ch->player.sex == SEX_MALE) ? "He" : "She");
			send_to_char(buf, victim);
			
			sprintf(buf, "You failed because %s has summon protection on.\r\n",
				GET_NAME(victim));
			send_to_char(buf, ch);
			
			extended_mudlog(NRM, SYSL_PLAYERS, TRUE, "%s failed summoning %s to %s.",
				GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
			return;
		}
	}

	if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
			(IS_NPC(victim) && mag_savingthrow(ch, victim, SAVING_SPELL))) {
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);

	char_from_room(victim);
	char_to_room(victim, IN_ROOM(ch));

	act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
	act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(victim, 0);
	entry_memory_mtrigger(ch);
	greet_mtrigger(ch, -1);
	greet_memory_mtrigger(ch);
}



ASPELL(spell_locate_object)
{
	struct obj_data *i;
	char name[MAX_INPUT_LENGTH];
	int j;

	/*
	 * FIXME: This is broken.  The spell parser routines took the argument
	 * the player gave to the spell and located an object with that keyword.
	 * Since we're passed the object and not the keyword we can only guess
	 * at what the player originally meant to search for. -gg
	 */
	strcpy(name, fname(obj->name));
	j = level / 2;

	for (i = object_list; i && (j > 0); i = i->next) {
		if (!isname(name, i->name))
			continue;

		if (i->carried_by)
			sprintf(buf, "%s is being carried by %s.\r\n",
				i->short_description, PERS(i->carried_by, ch, 0));
		else if (IN_ROOM(i) != NOWHERE)
			sprintf(buf, "%s is in %s.\r\n", i->short_description,
				world[IN_ROOM(i)].name);
		else if (i->in_obj)
			sprintf(buf, "%s is in %s.\r\n", i->short_description,
				i->in_obj->short_description);
		else if (i->worn_by)
			sprintf(buf, "%s is being worn by %s.\r\n",
				i->short_description, PERS(i->worn_by, ch, 0));
		else
			sprintf(buf, "%s's location is uncertain.\r\n",
				i->short_description);

		CAP(buf);
		send_to_char(buf, ch);
		j--;
	}

	if (j == level / 2)
		send_to_char("You sense nothing.\r\n", ch);
}



ASPELL(spell_charm)
{
	struct affected_type af;

	if (victim == NULL || ch == NULL)
		return;

	if (victim == ch)
		send_to_char("You like yourself even better!\r\n", ch);
	/* else if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE))
		send_to_char("You fail because SUMMON protection is on!\r\n", ch); */
	else if (!IS_NPC(victim))
	{
		if (charm_allowed == 0)
			if (!PRF_FLAGGED(victim, PRF_SUMMONABLE))
				send_to_char("You fail because SUMMON protection is on!\r\n", ch);
	}
	else if (AFF_FLAGGED(victim, AFF_SANCTUARY))
		send_to_char("Your victim is protected by sanctuary!\r\n", ch);
	else if (AFF_FLAGGED(victim, AFF_ORB))
		send_to_char("Your victim is protected by orb!\r\n", ch);
	else if (MOB_FLAGGED(victim, MOB_NOCHARM))
		send_to_char("Your victim resists!\r\n", ch);
	else if (AFF_FLAGGED(ch, AFF_CHARM))
		send_to_char("You can't have any followers of your own!\r\n", ch);
	else if (AFF_FLAGGED(victim, AFF_CHARM) || GET_CHARISMA(ch) < GET_CHARISMA(victim))
		send_to_char("You fail.\r\n", ch);
	/* player charming another player - no legal reason for this */
	else if (!charm_allowed && !IS_NPC(victim))
		send_to_char("You fail - shouldn't be doing it anyway.\r\n", ch);
	else if (circle_follow(victim, ch))
		send_to_char("Sorry, following in circles can not be allowed.\r\n", ch);
	else if (mag_savingthrow(ch, victim, SAVING_PARA))
		send_to_char("Your victim resists!\r\n", ch);
	else {
		if (victim->master)
			stop_follower(victim);
		
		add_follower(victim, ch);
		
		af.type = SPELL_CHARM;
		af.duration = 24 * 2;
		if (GET_CHARISMA(ch))
			af.duration *= GET_CHARISMA(ch);
		if (GET_INTELLIGENCE(victim) / 100)
			af.duration /= (GET_INTELLIGENCE(victim) / 100);
		af.modifier = 0;
		af.location = 0;
		af.bitvector = AFF_CHARM;
		affect_to_char(victim, &af);

		act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
		if (IS_NPC(victim))
			REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
	}
}



ASPELL(spell_identify)
{
	int i;
	int found;

	if (obj) {
		send_to_char("You feel informed:\r\n", ch);
		sprintf(buf, "Object '%s', Item type: ", obj->short_description);
		sprinttype(GET_OBJ_TYPE(obj), item_types, buf2, sizeof(buf2));
		strcat(buf, buf2);
		strcat(buf, "\r\n");
		send_to_char(buf, ch);

		if (GET_OBJ_AFFECT(obj)) {
			send_to_char("Item will give you following abilities:  ", ch);
			sprintbit(GET_OBJ_AFFECT(obj), affected_bits, buf, sizeof(buf));
			strcat(buf, "\r\n");
			send_to_char(buf, ch);
		}
		send_to_char("Item is: ", ch);
		sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf, sizeof(buf));
		strcat(buf, "\r\n");
		send_to_char(buf, ch);

		sprintf(buf, "Weight: %d, Value: %d, Rent: %d\r\n",
			GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));
		send_to_char(buf, ch);

		switch (GET_OBJ_TYPE(obj)) {
		case ITEM_SCROLL:
		case ITEM_POTION:
			sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);

			if (GET_OBJ_VAL(obj, 1) >= 1)
				sprintf(buf + strlen(buf), " %s", skill_name(GET_OBJ_VAL(obj, 1)));
			if (GET_OBJ_VAL(obj, 2) >= 1)
				sprintf(buf + strlen(buf), " %s", skill_name(GET_OBJ_VAL(obj, 2)));
			if (GET_OBJ_VAL(obj, 3) >= 1)
				sprintf(buf + strlen(buf), " %s", skill_name(GET_OBJ_VAL(obj, 3)));
			strcat(buf, "\r\n");
			send_to_char(buf, ch);
			break;
		case ITEM_WAND:
		case ITEM_STAFF:
			sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
			sprintf(buf + strlen(buf), " %s\r\n", skill_name(GET_OBJ_VAL(obj, 3)));
			sprintf(buf + strlen(buf), "It has %d maximum charge%s and %d remaining.\r\n",
				GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
				GET_OBJ_VAL(obj, 2));
			send_to_char(buf, ch);
			break;
		case ITEM_WEAPON:
			sprintf(buf, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
				GET_OBJ_VAL(obj, 2));
			sprintf(buf + strlen(buf), " for an average per-round damage of %.1f.\r\n",
				(((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
			send_to_char(buf, ch);
			break;
		case ITEM_ARMOR:
			sprintf(buf, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
			send_to_char(buf, ch);
			break;
		}
		found = FALSE;
		for (i = 0; i < MAX_OBJ_AFFECT; i++) {
			if ((obj->affected[i].location != APPLY_NONE) &&
		(obj->affected[i].modifier != 0)) {
	if (!found) {
		send_to_char("Can affect you as :\r\n", ch);
		found = TRUE;
	}
	sprinttype(obj->affected[i].location, apply_types, buf2, sizeof(buf2));
	sprintf(buf, "   Affects: %s By %d\r\n", buf2, obj->affected[i].modifier);
	send_to_char(buf, ch);
			}
		}
	} else if (victim) {		/* victim */
		sprintf(buf, "Name: %s\r\n", GET_NAME(victim));
		send_to_char(buf, ch);
		if (!IS_NPC(victim)) {
			sprintf(buf, "%s is %d years, %d months, %d days and %d hours old.\r\n",
				GET_NAME(victim), age(victim)->year, age(victim)->month,
				age(victim)->day, age(victim)->hours);
			send_to_char(buf, ch);
		}
		sprintf(buf, "Height %d cm, Weight %d pounds\r\n",
			GET_HEIGHT(victim), GET_WEIGHT(victim));
		sprintf(buf + strlen(buf), "Hits: %d, Mana: %d\r\n",
			GET_HIT(victim), GET_MANA(victim));
		sprintf(buf + strlen(buf), "AC: %d, Hitroll: %d, Damroll: %d\r\n",
			compute_armor_class(victim), GET_HITROLL(victim), GET_DAMROLL(victim));
		sprintf(buf + strlen(buf), "Str: %d, Agl: %d, Pre: %d, Per: %d, Hea: %d\r\n"
				"Wil: %d, Int: %d, Cha: %d, Luc: %d, Ess: %d\r\n",
					GET_STRENGTH(victim) / 100, GET_AGILITY(victim) / 100, GET_PRECISION(victim) / 100, 
					GET_PERCEPTION(victim) / 100, GET_HEALTH(victim) / 100, GET_WILLPOWER(victim) / 100, GET_INTELLIGENCE(victim) / 100, 
					GET_CHARISMA(victim) / 100, GET_LUCK(victim) / 100, GET_ESSENCE(victim) / 100);
		send_to_char(buf, ch);
		
	}
}



/*
 * Cannot use this spell on an equipped object or it will mess up the
 * wielding character's hit/dam totals.
 */
ASPELL(spell_enchant_weapon)
{
	int i;

	if (ch == NULL || obj == NULL)
		return;

	/* Either already enchanted or not a weapon. */
	if (GET_OBJ_TYPE(obj) != ITEM_WEAPON || OBJ_FLAGGED(obj, ITEM_MAGIC))
		return;
	
	/* Make sure no other affections. */
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		if (obj->affected[i].location != APPLY_NONE)
			return;
		
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
		
		obj->affected[0].location = APPLY_HITROLL;
		obj->affected[0].modifier = 1 + (level >= 18);
		
		obj->affected[1].location = APPLY_DAMROLL;
		obj->affected[1].modifier = 1 + (level >= 20);
		
		if (IS_GOOD(ch)) {
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
			act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
		} else if (IS_EVIL(ch)) {
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
			act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
		} else
			act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
}


ASPELL(spell_detect_poison)
{
	if (victim) {
		if (victim == ch) {
			if (AFF_FLAGGED(victim, AFF_POISON))
				send_to_char("You can sense poison in your blood.\r\n", ch);
			else
				send_to_char("You feel healthy.\r\n", ch);
		} else {
			if (AFF_FLAGGED(victim, AFF_POISON))
				act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
			else
				act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
		}
	}

	if (obj) {
		switch (GET_OBJ_TYPE(obj)) {
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:
		case ITEM_FOOD:
			if (GET_OBJ_VAL(obj, 3))
				act("You sense that $p has been contaminated.",FALSE,ch,obj,0,TO_CHAR);
			else
				act("You sense that $p is safe for consumption.", FALSE, ch, obj, 0,
				TO_CHAR);
			break;
		default:
			send_to_char("You sense that it should not be consumed.\r\n", ch);
		}
	}
}


ASPELL(spell_minor_identify)
{
	char *min_id[] = {
		"%sIt would bring %s if you sold it!\r\n",              /* NONE */
			"your physique!\r\n",                                 /* STR */
			"your speed!\r\n",                                    /* DEX */
			"your thoughts!\r\n",                                 /* INT */
			"your knowledge of things!\r\n",                      /* WIS */
			"your endurance!\r\n",                                /* CON */
			"your appearance!\r\n",                               /* CHA */
			"!CLASS!\r\n",                                        /* CLASS */
			"the time!\r\n",                                      /* AGE */
			"your burden of life!\r\n",                           /* WEIGHT */
			"your vertical stance!\r\n",                          /* HEIGHT */
			"your magical aura!\r\n",                             /* MANA */
			"your physical resistance!\r\n",                      /* HIT */
			"your ability to move!\r\n",                          /* MOVE */
			"your wealth!\r\n",                                   /* GOLD */
			"your conscious perception!\r\n",                     /* EXP */
			"your armor!\r\n",                                    /* AC */
			"the way you hit!\r\n",                               /* HITROLL */
			"the damage you give!\r\n",                           /* DAMROLL */
			"your ability to withstand paralysis!\r\n",           /* PARA */
			"your ability to withstand rod attacks!\r\n",         /* ROD */
			"your ability to withstand petrification!\r\n",       /* PETRI */
			"your ability to withstand breath Attacks!\r\n",      /* BREATH */
			"your ability to withstand spells!\r\n",              /* SPELL */
			"!RACES!\r\n"                                         /* RACE */
			"\n"};
		
		int cost = 0, i, x, mes_get = 0;
		bool found = FALSE, sent = FALSE;
		
		if (obj == NULL) {
			send_to_char("Maybe you should try to actually IDENTIFY something?\r\n", ch);
			return;
		}

		if (GET_OBJ_COST(obj) == 0)
			cost = number(1, 1000);
		else
			cost = GET_OBJ_COST(obj);
		
		if (!obj->affected[0].modifier) {
		sprintf(buf, "%s cannot help you in any special way.\r\nBut it might bring %s if you sold it.\r\n", obj->short_description, money_desc(cost));
		send_to_char(buf, ch);
		return;
		}
		sprintf(buf, "%s can help you in the following way:\r\n", obj->short_description);
		for (i = 0; i < MAX_OBJ_AFFECT; i++)
			if (obj->affected[i].modifier) {
				if (number(0, 20) > (GET_INTELLIGENCE(ch) / 100) && IS_MORTAL(ch))
					continue;
				
				switch(obj->affected[i].location) {
				case APPLY_NONE:
				case APPLY_RACE:
				case APPLY_CLASS:
					if (!found) {
						sprintf(buf, min_id[0], buf, money_desc(cost));
						found = TRUE;
						sent = TRUE;
					}
					break;
				default:
					mes_get = obj->affected[i].location;
					x = number(0, 1);
					sprintf(buf, "%s%s%s", buf, (x ? "it might do something about ":
					"It could do something about "), min_id[mes_get]);
					sent = TRUE;
					break;
				}
			}
			
			
			if (!sent) {
				sprintf(buf, "It seems to you that %s cannot help you in any special way.\r\n", obj->short_description);
					send_to_char(buf, ch);
				return;
			}
			
			send_to_char(buf, ch);
			
}



#define	PORTAL 31

ASPELL(spell_portal)
{
	/* create a magic portal */
	struct obj_data *tmp_obj, *tmp_obj2;
	struct extra_descr_data *ed;
	struct room_data *rp, *nrp;
	struct char_data *tmp_ch = (struct char_data *) victim;
	char buf[512];
	
	assert(ch);
	assert((level >= 0) && (level <= 100));
	
	
	/*
	check target room for legality.
	*/
	rp = &world[IN_ROOM(ch)];
	tmp_obj = read_object(PORTAL, VIRTUAL);
	if (!rp || !tmp_obj) {
		send_to_char("The magic fails.\n\r", ch);
		extract_obj(tmp_obj);
		return;
	}
	
	if (IS_SET(rp->room_flags, ROOM_NOMAGIC)) {
		send_to_char("Eldritch wizardry obstructs thee.\n\r", ch);
		extract_obj(tmp_obj);
		return;
	}
	
	if (IS_SET(rp->room_flags, ROOM_TUNNEL)) {
		send_to_char("There is no room in here to summon!\n\r", ch);
		extract_obj(tmp_obj);
		return;
	}
	
	if (!(nrp = &world[IN_ROOM(tmp_ch)])) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "MAGIC: %s not in any room (S1)", GET_NAME(tmp_ch));
		send_to_char("The magic cannot locate the target\n", ch);
		extract_obj(tmp_obj);
		return;
	}
	
	if (ROOM_FLAGGED(IN_ROOM(tmp_ch), ROOM_NOMAGIC)) {
		send_to_char("Your target is protected against your magic.\r\n", ch);
		extract_obj(tmp_obj);
		return;
	}
	
	/* if (!CAN_SUMMON(IN_ROOM(tmp_ch), IN_ROOM(ch))) {
	send_to_char("Your magic is broken by the divine forces.\r\n", ch);
	extract_obj(tmp_obj);
	return;
}	*/
	
	sprintf(buf, "Through the mists of the portal, you can faintly see %s.\r\n", nrp->name);
	
	CREATE(ed , struct extra_descr_data, 1);
	ed->next = tmp_obj->ex_description;
	tmp_obj->ex_description = ed;
	CREATE(ed->keyword, char, strlen(tmp_obj->name) + 1);
	strcpy(ed->keyword, tmp_obj->name);
	ed->description = str_dup(buf);
	
	tmp_obj->obj_flags.value[0] = level/30;
	tmp_obj->obj_flags.value[1] = IN_ROOM(tmp_ch);
	
	obj_to_room(tmp_obj,IN_ROOM(ch));
	
	act("&M$p suddenly appears.&n",TRUE,ch,tmp_obj,0,TO_ROOM);
	act("&M$p suddenly appears.&n",TRUE,ch,tmp_obj,0,TO_CHAR);
	
	/* Portal at other side */
	rp = &world[IN_ROOM(ch)];
	tmp_obj2 = read_object(PORTAL, VIRTUAL);
	if (!rp || !tmp_obj2) {
		send_to_char("The magic fails.\n\r", ch);
		extract_obj(tmp_obj2);
		return;
	}
	sprintf(buf, "Through the mists of the portal, you can faintly see %s\r\n", rp->name);
	
	CREATE(ed , struct extra_descr_data, 1);
	ed->next = tmp_obj2->ex_description;
	tmp_obj2->ex_description = ed;
	CREATE(ed->keyword, char, strlen(tmp_obj2->name) + 1);
	strcpy(ed->keyword, tmp_obj2->name);
	ed->description = str_dup(buf);
	
	tmp_obj2->obj_flags.value[0] = level/30;
	tmp_obj2->obj_flags.value[1] = IN_ROOM(ch);
	
	obj_to_room(tmp_obj2,IN_ROOM(tmp_ch));
	
	act("&M$p suddenly appears.&n",TRUE,tmp_ch,tmp_obj2,0,TO_ROOM);
	act("&M$p suddenly appears.&n",TRUE,tmp_ch,tmp_obj2,0,TO_CHAR);
	
}


ASPELL(spell_arcane_word)
{
	struct obj_data *final_scroll='\0';
	struct extra_descr_data *new_descr;
	int roomno;
	
	if (ch == NULL)
		return;

	roomno = world[IN_ROOM(ch)].number;

	sprintf(buf2, "%s", true_name(roomno));

	sprintf(buf, "You feel a strange sensation in your body, as the wheel of time stops.\r\n");
	send_to_char(buf, ch);
	sprintf(buf, "Strange sounds can be heard through the mists of time.\r\n");
	send_to_char(buf, ch);
	sprintf(buf, "The Parcae tell you: 'The Arcane name for this place is &W%s&n'.\r\n", 
		buf2); 
	send_to_char(buf, ch);
	
	final_scroll = create_obj();

	final_scroll->item_number = NOTHING;
	IN_ROOM(final_scroll) = NOWHERE;

	sprintf(buf, "arcane parchment %s parch", buf2);
	final_scroll->name = str_dup(buf);

	final_scroll->short_description = str_dup("an arcane parchment");

	sprintf(buf, "A parchment inscribed with the runes '%s' lies here.", buf2);
	final_scroll->description = str_dup(buf);

	/* extra description coolness! */
	CREATE(new_descr, struct extra_descr_data, 1);
	new_descr->keyword = str_dup(final_scroll->name);
	sprintf(buf, "The Arcane word for &W%s&n is &Y%s&n.", world[IN_ROOM(ch)].name, buf2);
	new_descr->description = str_dup(buf);
	new_descr->next = NULL;
	final_scroll->ex_description = new_descr;

	GET_OBJ_TYPE(final_scroll) = ITEM_NOTE;
	GET_OBJ_WEAR(final_scroll) = ITEM_WEAR_TAKE;
	SET_BIT(GET_OBJ_EXTRA(final_scroll), ITEM_MAGIC);
	SET_BIT(GET_OBJ_EXTRA(final_scroll), ITEM_UNIQUE_SAVE);
	GET_OBJ_VAL(final_scroll, 0) = (GET_SKILL(ch, SPELL_ARCANE_WORD)/100);
	GET_OBJ_VAL(final_scroll, 1) = -1;
	GET_OBJ_VAL(final_scroll, 2) = -1;
	GET_OBJ_VAL(final_scroll, 3) = -1;
	GET_OBJ_COST(final_scroll) = (GET_SKILL(ch, SPELL_ARCANE_WORD)/100) * 500;
	GET_OBJ_WEIGHT(final_scroll) = 1;
	GET_OBJ_RENT(final_scroll) = 0;

	obj_to_char(final_scroll, ch);

	send_to_char("You create an arcane parchment.\r\n", ch);
	act("$n creates a scroll!", FALSE, ch, 0, 0, TO_ROOM);

}


ASPELL(spell_arcane_portal)
{	
	struct obj_data *tmp_obj, *tmp_obj2;
	struct extra_descr_data *ed;
	struct room_data *rp, *nrp;
	char buf[512];
	
	char *arc_name = tar_str;
	char *tmp = 0;
	int arc_found = 0;
	int nr, icount;
	sh_int location;
	
	int dicechance = 0;
	
	location = 0;
	
	assert(ch);
	assert((level >= 0) && (level <= 100));

	if (ch == NULL)
		return;
	
	/* It takes *SOME* kind of Intelligence to portal!! */
	
	if (GET_INTELLIGENCE(ch) / 100 < 7) {
		send_to_char("What is a portal?", ch);
		return;
	}
	
	if (arc_name == NULL) {
		send_to_char("Portal to where?", ch);
		return;
	}

	for (nr = 0; nr < 5;nr++) {
		arc_found = FALSE;
		icount = 0;
		for (icount = 0; icount <= 9;icount++) {
			if (!arc_found) {
				tmp = str_dup(arc_sylls[nr][0][icount]);
				if(!strncmp(arc_name, tmp, strlen(tmp))) {
					arc_found = TRUE;
					arc_name = arc_name + strlen(tmp);
					location = location * 10 + icount;
				}
				if (!arc_found) {
					tmp = str_dup(arc_sylls[nr][1][icount]);
					if(!strncmp(arc_name, tmp, strlen(tmp))) {
						arc_found = TRUE;
						arc_name = arc_name + strlen(tmp);
						location = location * 10 + icount;
					}
				}
			}
		}
		/* here we have tried all 20 chars, and that means.. If still !arc_found = total fail */
		/* They still can be saved by their Intelligence, the higher intelligence, then less */ 
		if (!arc_found) {
			dicechance = (GET_INTELLIGENCE(ch) / 100) - number(((GET_INTELLIGENCE(ch) / 100) - 7), (GET_INTELLIGENCE(ch) / 100));
			switch(dicechance) { 
			case 0 : /* Full Score Nothing happens! */
				send_to_char("You feel strangely relieved, like something terrible just was avoided.\r\n",ch);
				return;
			case 1 : /* Minor Mishap! Give a lame text or push the character somewhere */
	 			send_to_char("Your concentration is lost.\r\n", ch);
				return;
			case 7 : /* TOTAL Fail!! Let's send the bugger off to somewhere <g> */ 
				send_to_char("You feel a disturbance in the magical flux continuum, and your portal fails.\r\n", ch);
				do {
					location = number(0, top_of_world);
				} while (ROOM_FLAGGED(location, ROOM_TUNNEL | ROOM_PRIVATE | ROOM_DEATH | ROOM_GODROOM) || ZONE_FLAGGED(world[location].zone,ZONE_NOTELEPORT));	 
				act("$n fails a portal terribly, and is sent off to somewhere.", FALSE, ch, 0, 0, TO_ROOM);
				char_from_room(ch);
				char_to_room(ch, location);
				act("A portal opens, and $n tumbles out of it, clearly confused.", FALSE, ch, 0, 0, TO_ROOM);
				look_at_room(ch, 0);
				return;
			default : /* Hum, just do something text based */
				send_to_char("Your portal fails.\r\n",ch);
				return;
			} 
		}
	}
	/* At this point all have been successful! now, let's make magic
	 * The portal spell takes a persons int and wisdom, and uses them for a saving throw
	 * Beware it *IS* possible to die from this spell!!! (should happen once out of 1000 or so
	 */
	
	if ((GET_SKILL(ch, SPELL_ARCANE_PORTAL)/100) > 80) {

		/* If the caster is above level 80 we will do a little special portal object for
		 * him/her instead.  Otherwise just teleport them to the location.
		 */
		
		rp = &world[IN_ROOM(ch)];
		tmp_obj = read_object(PORTAL, VIRTUAL);
		if (!rp || !tmp_obj) {
			send_to_char("The magic fails.\r\n", ch);
			extract_obj(tmp_obj);
			return;
		}
		
		if (IS_SET(rp->room_flags, ROOM_NOMAGIC)) {
			send_to_char("Eldritch wizardry obstructs thee.\r\n", ch);
			extract_obj(tmp_obj);
			return;
		}
		
		if (IS_SET(rp->room_flags, ROOM_TUNNEL)) {
			send_to_char("There is no room in here to portal!  Using teleport instead.\r\n", ch);
			extract_obj(tmp_obj);

			act("$n opens a portal, steps into it and vanishes.", FALSE, ch, 0, 0, TO_ROOM);
		
			char_from_room(ch);
			char_to_room(ch, real_room(location));
		
			act("A portal opens, and $n steps out of it.", FALSE, ch, 0, 0, TO_ROOM);
			look_at_room(ch, 0);

			return;
		}
		
		if (!(nrp = &world[real_room(location)])) {
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "MAGIC: %s, no such location (S2)", tar_str);
			send_to_char("The magic cannot locate the target.\r\n", ch);
			extract_obj(tmp_obj);
			return;
		}
		
		if (ROOM_FLAGGED(real_room(location), ROOM_NOMAGIC)) {
			send_to_char("Your target is protected against your magic.\r\n", ch);
			extract_obj(tmp_obj);
			return;
		}
		
		sprintf(buf, "Through the mists of the portal, you can faintly see %s.", nrp->name);
		
		CREATE(ed , struct extra_descr_data, 1);
		ed->next = tmp_obj->ex_description;
		tmp_obj->ex_description = ed;
		CREATE(ed->keyword, char, strlen(tmp_obj->name) + 1);
		strcpy(ed->keyword, tmp_obj->name);
		ed->description = str_dup(buf);
		
		tmp_obj->obj_flags.value[0] = level/30;
		tmp_obj->obj_flags.value[1] = real_room(location);
		
		obj_to_room(tmp_obj,IN_ROOM(ch));
		
		act("&M$p suddenly appears.&n",TRUE,ch,tmp_obj,0,TO_ROOM);
		act("&M$p suddenly appears.&n",TRUE,ch,tmp_obj,0,TO_CHAR);
		
		/* Portal at other side */
		rp = &world[IN_ROOM(ch)];
		tmp_obj2 = read_object(PORTAL, VIRTUAL);
		if (!rp || !tmp_obj2) {
		send_to_char("The magic fails.\n\r", ch);
		extract_obj(tmp_obj2);
		return;
	}
		sprintf(buf, "Through the mists of the portal, you can faintly see %s.", rp->name);
		
		CREATE(ed , struct extra_descr_data, 1);
		ed->next = tmp_obj2->ex_description;
		tmp_obj2->ex_description = ed;
		CREATE(ed->keyword, char, strlen(tmp_obj2->name) + 1);
		strcpy(ed->keyword, tmp_obj2->name);
		ed->description = str_dup(buf);
		
		tmp_obj2->obj_flags.value[0] = level/30;
		tmp_obj2->obj_flags.value[1] = IN_ROOM(ch);
		
		obj_to_room(tmp_obj2, real_room(location));
		
		act("&M$p suddenly appears.&n", FALSE, 0, tmp_obj2, 0, TO_ROOM);

	} else {

		/* They were below level 80, just teleport them to the location. */
		
		if (!(nrp = &world[real_room(location)])) {
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "MAGIC: %s, no such location (S3)", tar_str);
			send_to_char("The magic cannot reach your target.\r\n", ch);
			return;
		}
		
		if (ROOM_FLAGGED(real_room(location), ROOM_NOMAGIC)) {
			send_to_char("Your target is protected against your magic.\r\n", ch);
			return;
		}
		
		act("$n opens a portal, steps into it and vanishes.", FALSE, ch, 0, 0, TO_ROOM);
		
		char_from_room(ch);
		char_to_room(ch, real_room(location));
		
		act("A portal opens, and $n steps out of it.", FALSE, ch, 0, 0, TO_ROOM);
		look_at_room(ch, 0);
		
	}
	
}	
