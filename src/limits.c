/* ************************************************************************
*		File: limits.c                                      Part of CircleMUD *
*	 Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: limits.c,v 1.27 2004/04/14 19:34:35 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "spells.h"
#include "events.h"
#include "skills.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "dg_scripts.h"


/* external variables */
extern int max_exp_gain;
extern int max_exp_loss;
extern int idle_rent_time;
extern bitvector_t idle_max_rights;
extern int idle_void;
extern int free_rent;
extern int use_void_disconnect;

ACMD(do_return);
ACMD(do_away);

/* local functions */
int	hitmove_graf(int percent, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
int	mana_graf(int percent, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
int copyover_check(void);
void update_char_objects(struct char_data * ch);        /* handler.c */

/* 
 * graf changed from age based to percent of max stat based.
 * When percent < 10 return the value p0 
 * When percent in 11..25 calculate the line between p1 & p2 
 * When percent in 26..50 calculate the line between p2 & p3 
 * When percent in 51..75 calculate the line between p3 & p4 
 * When percent in 76..90 calculate the line between p4 & p5 
 * When percent >= 91 return the value p6 
 */
int	hitmove_graf(int percent, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

	if (percent < 10)
		return (p0 + ((percent * (p1 - p0)) / 10));               /* < 10   */
	else if (percent <= 25)
		return (p1 + (((percent - 10) * (p2 - p1)) / 10));        /* 11..25 */
	else if (percent <= 50)
		return (p2 + (((percent - 25) * (p3 - p2)) / 10));        /* 26..50 */
	else if (percent <= 75)
		return (p3 + (((percent - 50) * (p4 - p3)) / 10));        /* 51..75 */
	else if (percent <= 90)
		return (p4 + (((percent - 75) * (p5 - p4)) / 10));        /* 76..90 */
	else
		return (p5 + (((percent - 90) * (p6 - p5)) / 10));        /* >= 91 */
}


int	mana_graf(int percent, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

	if (percent < 10)
		return (p0 + ((p1 - p0) / percent));               /* < 10   */
	else if (percent <= 25)
		return (p1 + ((p1 - p2) / (percent - 10)));        /* 11..25 */
	else if (percent <= 50)
		return (p2 + ((p2 - p3) / (percent - 25)));        /* 26..50 */
	else if (percent <= 75)
		return (p3 + ((p3 - p4) / (percent - 50)));        /* 51..75 */
	else if (percent <= 90)
		return (p4 + ((p4 - p5) / (percent - 75)));        /* 76..90 */
	else
		return (p5 + ((p5 - p6) / (percent - 90)));        /* >= 91 */
}


/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int	mana_gain(struct char_data *ch)
{
	int gain, percent = 0;

	if (IS_NPC(ch)) {
		/* Neat and fast */
		gain = GET_DIFFICULTY(ch) * 10;
	} else {
		percent = ((float)GET_MANA(ch) / (float)GET_MAX_MANA(ch)) * 100;
		if (percent == 0)
			percent = 1;
		gain = mana_graf(percent, 1, 1, 2, 3, 5, 8, 13);

		/* Attribute calculations */
		gain += (GET_INTELLIGENCE(ch) / 100 / 5);  /* Intelligence */
		gain += (GET_WILLPOWER(ch) / 100 / 5);     /* Willpower */
		gain += (GET_ESSENCE(ch) / 100 / 5);       /* Essence */

		/* Class calculations */

		/* Skill/Spell calculations */

		/* Position calculations    */
		switch (GET_POS(ch)) {
		case POS_SLEEPING:
			gain *= 2;
			break;
		case POS_RESTING:
			gain += (gain / 2);        /* Divide by 2 */
			break;
		case POS_SITTING:
			gain += (gain / 4);        /* Divide by 4 */
			break;
		case POS_MEDITATING:
			if (!IS_OOC(ch))
				gain *= 1.5;
			break;
		}

		if (IS_MAGI_TYPE(ch) || IS_CLERIC_TYPE(ch))
			gain *= 2;

		if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
			gain /= 4;
	}

	if (AFF_FLAGGED(ch, AFF_POISON))
		gain /= 4;

	return (gain);
}


/* Hitpoint gain pr. game hour */
int	hit_gain(struct char_data *ch)
{
	int gain, percent = 0;

	if (IS_NPC(ch)) {
		/* Neat and fast */
		gain = GET_DIFFICULTY(ch) * 5;
	} else {
		percent = ((float)GET_HIT(ch)/(float)GET_MAX_HIT(ch)) * 100;
		if (percent == 0) percent = 1;
		gain = hitmove_graf(percent, 1, 1, 2, 3, 5, 8, 13);

		/* Attribute calculations */
		gain += (GET_HEALTH(ch) / 100 / 5);     /* Health */
		gain += (GET_STRENGTH(ch) / 100 / 5);   /* Strength */
		gain += (GET_WILLPOWER(ch) / 100 / 5);  /* Willpower */

		/* Class/Level calculations */

		/* Skill/Spell calculations */

		/* Position calculations    */

		switch (GET_POS(ch)) {
		case POS_SLEEPING:
			gain += (gain / 2);        /* Divide by 2 */
			break;
		case POS_RESTING:
			gain += (gain / 4);        /* Divide by 4 */
			break;
		case POS_SITTING:
			gain += (gain / 8);        /* Divide by 8 */
			break;
		}

		if (IS_MAGI_TYPE(ch) || IS_CLERIC_TYPE(ch))
			gain /= 2;        /* Ouch. */

		if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
			gain /= 4;
	}

	if (AFF_FLAGGED(ch, AFF_POISON))
		gain /= 4;

	return (gain);
}



/* move gain pr. game hour */
int	move_gain(struct char_data *ch)
{
	int gain, percent = 0;

	if (IS_NPC(ch)) {
		/* Neat and fast */
		gain = GET_DIFFICULTY(ch) * 15;
	} else {
		percent = ((float)GET_MOVE(ch)/(float)GET_MAX_MOVE(ch)) * 100;
		if (percent == 0) percent = 1;
		gain = hitmove_graf(percent, 0, 1, 1, 2, 3, 5, 8);

		/* Attribute calculations */
		gain += (GET_AGILITY(ch) / 100 / 5);   /* Agility */
		gain += (GET_STRENGTH(ch) / 100 / 5);  /* Strength */
		gain += (GET_WILLPOWER(ch) / 100 / 5); /* Willpower */

		/* Class/Level calculations */

		/* Skill/Spell calculations */


		/* Position calculations    */
		switch (GET_POS(ch)) {
		case POS_SLEEPING:
			gain += (gain / 2);        /* Divide by 2 */
			break;
		case POS_RESTING:
			gain += (gain / 4);        /* Divide by 4 */
			break;
		case POS_SITTING:
			gain += (gain / 8);        /* Divide by 8 */
			break;
		}

		if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
			gain /= 4;
	}

	if (AFF_FLAGGED(ch, AFF_POISON))
		gain /= 4;

	return (gain);
}


void set_title(struct char_data *ch, char *title)
{
	if (title == NULL)
		title = "the Newbie";

	if (strlen(title) > MAX_TITLE_LENGTH)
		title[MAX_TITLE_LENGTH] = '\0';

	if (GET_TITLE(ch) != NULL)
		free(GET_TITLE(ch));

	GET_TITLE(ch) = str_dup(title);
}


void gain_exp(struct char_data *ch, int gain)
{
	if (!IS_PLAYER(ch) || IS_IMMORTAL(ch))
		return;

	if (IS_NPC(ch)) {
		GET_EXP(ch) += gain;
		return;
	}

	GET_EXP(ch) += gain;
	if (GET_EXP(ch) < 0)
		GET_EXP(ch) = 0;
}


void gain_exp_regardless(struct char_data *ch, int gain)
{
	GET_EXP(ch) += gain;
	if (GET_EXP(ch) < 0)
		GET_EXP(ch) = 0;
}


void gain_condition(struct char_data *ch, int condition, int value)
{
	bool intoxicated;

	if (IS_NPC(ch) || GET_COND(ch, condition) == -1)        /* No change */
		return;

	intoxicated = (GET_COND(ch, DRUNK) > 0);

	GET_COND(ch, condition) += value;

	GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
	GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

	if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING) || PLR_FLAGGED(ch, PLR_OLC))
		return;

	switch (condition) {
	case FULL:
		send_to_char("You are hungry.\r\n", ch);
		return;
	case THIRST:
		send_to_char("You are thirsty.\r\n", ch);
		return;
	case DRUNK:
		if (intoxicated)
			send_to_char("You are now sober.\r\n", ch);
		return;
	default:
		break;
	}

}


void check_idling(struct char_data *ch)
{
	if (use_void_disconnect) {
		if (++(ch->char_specials.timer) > idle_void) {
			if (GET_WAS_IN(ch) == NOWHERE && IN_ROOM(ch) != NOWHERE) {
				GET_WAS_IN(ch) = IN_ROOM(ch);
				if (FIGHTING(ch)) {
					stop_fighting(FIGHTING(ch));
					stop_fighting(ch);
				}
				act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
				send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
				save_char(ch, NOWHERE, FALSE);
				crash_datasave(ch, 0, RENT_CRASH);
				char_from_room(ch);
				char_to_room(ch, 1);
			} else if (ch->char_specials.timer > idle_rent_time) {
				if (IN_ROOM(ch) != NOWHERE)
					char_from_room(ch);
				char_to_room(ch, 3);
				if (ch->desc) {
					STATE(ch->desc) = CON_DISCONNECT;
					/*
					 * For the 'if (d->character)' test in close_socket().
					 * -gg 3/1/98 (Happy anniversary.)
					 */
					ch->desc->character = NULL;
					ch->desc = NULL;
				}
				if (free_rent)
					crash_datasave(ch, 0, RENT_RENTED);
				else
					crash_datasave(ch, 0, RENT_TIMEDOUT);
				extended_mudlog(BRF, SYSL_RENT, TRUE, "%s force-rented and extracted (idle).", GET_NAME(ch));
				extract_char(ch);
			}
		}
	} else {
		if (++(ch->char_specials.timer) > idle_void && IN_ROOM(ch) != NOWHERE && 
			!SESS_FLAGGED(ch, SESS_AFK) && !SESS_FLAGGED(ch, SESS_AFW)) {
			if (FIGHTING(ch)) {
				stop_fighting(FIGHTING(ch));
				stop_fighting(ch);
			}
			send_to_char("You have been idle, and have been flagged AFK.\r\n", ch);
			do_away(ch, "Idling...", 0, 0);
			save_char(ch, NOWHERE, FALSE);
			crash_datasave(ch, 0, RENT_CRASH);
		}
	}
}


/* Update PCs, NPCs, and objects */
void point_update(void)
{
	struct char_data *i, *next_char;
	struct obj_data *j, *next_thing, *jj, *next_thing2;

	/* characters */
	for (i = character_list; i; i = next_char) {
		next_char = i->next;
								
		if (GET_POS(i) >= POS_STUNNED) {
			GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
			GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
			GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));
			if (AFF_FLAGGED(i, AFF_POISON))
				if (damage(i, i, 2, SPELL_POISON) == -1)
					continue;        /* Oops, they died. -gg 6/24/98 */
			if (GET_POS(i) <= POS_STUNNED)
				update_pos(i);
		} else if (GET_POS(i) == POS_INCAP) {
			if (damage(i, i, 1, TYPE_SUFFERING) == -1)
				continue;
		} else if (GET_POS(i) == POS_MORTALLYW) {
			if (damage(i, i, 2, TYPE_SUFFERING) == -1)
				continue;
		}
		if (!IS_NPC(i)) {
			update_char_objects(i);
			if (get_max_rights(i) < idle_max_rights)
				check_idling(i);
		}
	}

	/* objects */
	for (j = object_list; j && j->next; j = next_thing) {
		next_thing = j->next;        /* Next in object list */

		/* If this is a corpse */
		if (IS_CORPSE(j)) {
			/* timer count down */
			if (GET_OBJ_TIMER(j) > 0)
				GET_OBJ_TIMER(j)--;

			if (!GET_OBJ_TIMER(j)) {

				if (j->carried_by)
					act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
				else if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people)) {
					act("A quivering horde of maggots consumes $p.",
							TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
					act("A quivering horde of maggots consumes $p.",
							TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
				}
				for (jj = j->contains; jj; jj = next_thing2) {
					next_thing2 = jj->next_content;        /* Next in inventory */
					obj_from_obj(jj);

					if (j->in_obj)
						obj_to_obj(jj, j->in_obj);
					else if (j->carried_by)
						obj_to_room(jj, IN_ROOM(j->carried_by));
					else if (IN_ROOM(j) != NOWHERE)
						obj_to_room(jj, IN_ROOM(j));
					else
						core_dump();
				}
				extract_obj(j);
			}
		}
		/* If the timer is set, count it down and at 0, try the trigger */
		/* note to .rej hand-patchers: make this last in your point-update() */
		else if (GET_OBJ_TIMER(j)>0) {
			GET_OBJ_TIMER(j)--; 
			if (!GET_OBJ_TIMER(j))
				timer_otrigger(j);
		}
		/* if it's a portal */
		else if (j && GET_OBJ_VNUM(j) == 31)
		{
			if (GET_OBJ_VAL(j,0) > 0)
				GET_OBJ_VAL(j,0)--;
			if (GET_OBJ_VAL(j,0) == 1) {
				if ((IN_ROOM(j) != NOWHERE) &&(world[IN_ROOM(j)].people)) {
					act("$p starts to fade!", 
						FALSE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
					act("$p starts to fade!", 
						FALSE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
				}
			}
			if (GET_OBJ_VAL(j,0) == 0) {
				if ((IN_ROOM(j) != NOWHERE) &&(world[IN_ROOM(j)].people)) {
					act("$p fades out of existence!", 
						FALSE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
					act("$p fades out of existence!", 
						FALSE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
				}
				extract_obj(j);
			}
		}
	
	}
}


int copyover_check(void)
{
	struct descriptor_data *d;
	int ready = 1;

	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) == CON_COPYOVER)
			continue;
		else if (STATE(d) == CON_PLAYING && !FIGHTING(d->character)) {
			if (d->original)
				do_return(d->character, 0, 0, 0);
			STATE(d) = CON_COPYOVER;
		}	else
			ready = 0;
	}

	return (ready);
}


void condition_update(void)
{
	struct char_data *i;

	for (i = character_list; i; i = i->next) {
		gain_condition(i, FULL, -1);
		gain_condition(i, THIRST, -1);
		gain_condition(i, DRUNK, -1);
	}
}
