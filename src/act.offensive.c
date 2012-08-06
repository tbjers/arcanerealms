/* ************************************************************************
*		File: act.offensive.c                               Part of CircleMUD *
*	 Usage: player-level commands of an offensive nature                    *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: act.offensive.c,v 1.20 2003/01/15 22:07:20 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "constants.h"

/* extern variables */
extern int pk_allowed;
extern int summon_allowed;
extern int charm_allowed;
extern int sleep_allowed;

/* extern functions */
void raw_kill(struct char_data *ch, struct char_data *killer);
int	compute_armor_class(struct char_data *ch);
int	ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);

/* local functions */
ACMD(do_assist);
ACMD(do_hit);
ACMD(do_kill);
ACMD(do_backstab);
ACMD(do_order);
ACMD(do_flee);
ACMD(do_bash);
ACMD(do_rescue);
ACMD(do_kick);
ACMD(do_disarm);
ACMD(do_retreat);
ACMD(do_defense);

ACMD(do_assist)
{
	struct char_data *helpee, *opponent;

	if (FIGHTING(ch)) {
		send_to_char("You're already fighting!  How can you assist someone else?\r\n", ch);
		return;
	}
	one_argument(argument, arg);

	if (!*arg)
		send_to_char("Whom do you wish to assist?\r\n", ch);
	else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM, 0)))
		send_to_char(NOPERSON, ch);
	else if (helpee == ch)
		send_to_char("You can't help yourself any more than this!\r\n", ch);
	else {
		/*
		 * Hit the same enemy the person you're helping is.
		 */
		if (FIGHTING(helpee))
			opponent = FIGHTING(helpee);
		else
			for (opponent = world[IN_ROOM(ch)].people;
					 opponent && (FIGHTING(opponent) != helpee);
					 opponent = opponent->next_in_room);

		if (!opponent)
			act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
		else if (!CAN_SEE(ch, opponent))
			act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
		else if (!pk_allowed && !IS_NPC(opponent))	/* prevent accidental pkill */
			act("You are not allowed to attack $N since PK has been turned off.", FALSE, ch, 0, opponent, TO_CHAR);
		else {
			send_to_char("You join the fight!\r\n", ch);
			act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
			act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
			hit(ch, opponent, TYPE_UNDEFINED);
		}
	}
}


ACMD(do_hit)
{
	struct char_data *vict;
	struct follow_type *k;
	
	if (IS_NPC(ch))	{
		if (MOB_FLAGGED(ch, MOB_TARGET)) {
			send_to_char("You're just a target, you can't fight!", ch);
			return;
		}
	}

	one_argument(argument, arg);
	
	if (!*arg)
		send_to_char("Hit who?\r\n", ch);
	else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM, 0)))
		send_to_char("They don't seem to be here.\r\n", ch);
	else if (vict == ch) {
		send_to_char("You hit yourself...OUCH!.\r\n", ch);
		act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
	} else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict)) {
		act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
	} else {
		if (!pk_allowed && !IS_NPC(vict)) {
			send_to_char("PK has been turned off, therefore you cannot attack other players.\r\n", ch);
			return;
		}
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
			send_to_char("This room has a nice peaceful feeling.\r\n", ch);
			return;
		}
		if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
			hit(ch, vict, TYPE_UNDEFINED);
			for (k = ch->followers; k; k=k->next) {
				if (PRF_FLAGGED(k->follower, PRF_AUTOASSIST) &&
					(IN_ROOM(k->follower) == IN_ROOM(ch)))
					do_assist(k->follower, (IS_IC(ch) && GET_KEYWORDS(ch) ? GET_KEYWORDS(ch) : GET_NAME(ch)), 0, 0);
			}
			
			WAIT_STATE(ch, PULSE_VIOLENCE + 2);
		} else
			send_to_char("You do the best you can!\r\n", ch);
	}
}



ACMD(do_kill)
{
	struct char_data *vict;

	if (!IS_IMPL(ch) || IS_NPC(ch)) {
		do_hit(ch, argument, cmd, subcmd);
		return;
	}
	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Kill who?\r\n", ch);
	} else {
		if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM, 0)))
			send_to_char("They aren't here.\r\n", ch);
		else if (ch == vict)
			send_to_char("Your mother would be so sad.. :(\r\n", ch);
		else {
			act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
			act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
			act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
			raw_kill(vict, ch);
			if (IS_NPC(vict))
				extended_mudlog(BRF, SYSL_MOBDEATHS, TRUE, "%s has been killed by %s.", GET_NAME(vict), GET_NAME(ch));
			else
				extended_mudlog(BRF, SYSL_DEATHS, TRUE, "%s has been god-slain by %s.", GET_NAME(vict), GET_NAME(ch));
		}
	}
}



ACMD(do_backstab)
{
	struct char_data *vict;
	struct follow_type *k;

	if (IS_NPC(ch))	{
		if (MOB_FLAGGED(ch, MOB_TARGET)) {
			send_to_char("You're just a target, you can't fight!", ch);
			return;
		}
	}

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char("This room has a nice peaceful feeling.\r\n", ch);
		return;
	}

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BACKSTAB)) {
		send_to_char("You have no idea how to do that.\r\n", ch);
		return;
	}

	one_argument(argument, buf);

	if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM, 0))) {
		send_to_char("Backstab who?\r\n", ch);
		return;
	}
	if (!ok_damage_shopkeeper(ch, vict))
		return;

	if (vict == ch) {
		send_to_char("How can you sneak up on yourself?\r\n", ch);
		return;
	}
	if (!GET_EQ(ch, WEAR_WIELD)) {
		send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT) {
		send_to_char("Only piercing weapons can be used for backstabbing.\r\n", ch);
		return;
	}
	if (FIGHTING(vict)) {
		send_to_char("You can't backstab a fighting person -- they're too alert!\r\n", ch);
		return;
	}

	if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) {
		act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
		act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
		act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
		hit(vict, ch, TYPE_UNDEFINED);
		return;
	}

	if (AWAKE(vict) && (!skill_check(ch, SKILL_BACKSTAB, 0)))
		damage(ch, vict, 0, SKILL_BACKSTAB);
	else
		hit(ch, vict, SKILL_BACKSTAB);

	for (k = ch->followers; k; k=k->next) {
		if (PRF_FLAGGED(k->follower, PRF_AUTOASSIST) &&
			(IN_ROOM(k->follower) == IN_ROOM(ch)))
			do_assist(k->follower, GET_NAME(ch), 0, 0);
	}

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
}


ACMD(do_order)
{
	char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
	bool found = FALSE;
	struct char_data *vict;
	struct follow_type *k;

	half_chop(argument, name, message);

	if (!*name || !*message)
		send_to_char("Order who to do what?\r\n", ch);
	else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM, 0)) && !is_abbrev(name, "followers"))
		send_to_char("That person isn't here.\r\n", ch);
	else if (ch == vict)
		send_to_char("You obviously suffer from schizophrenia.\r\n", ch);
	else {
		if (AFF_FLAGGED(ch, AFF_CHARM)) {
			send_to_char("Your superior would not aprove of you giving orders.\r\n", ch);
			return;
		}
		if (vict) {
			sprintf(buf, "$N orders you to '%s'", message);
			act(buf, FALSE, vict, 0, ch, TO_CHAR);
			act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

			if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
				act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
			else {
				send_to_char(OK, ch);
				command_interpreter(vict, message);
			}
		} else {			/* This is order "followers" */
			sprintf(buf, "$n issues the order '%s'.", message);
			act(buf, FALSE, ch, 0, 0, TO_ROOM);

			for (k = ch->followers; k; k = k->next) {
				if (IN_ROOM(ch) == IN_ROOM(k->follower))
					if (AFF_FLAGGED(k->follower, AFF_CHARM)) {
						found = TRUE;
						command_interpreter(k->follower, message);
		}
			}
			if (found)
				send_to_char(OK, ch);
			else
				send_to_char("Nobody here is a loyal subject of yours!\r\n", ch);
		}
	}
}



ACMD(do_flee)
{
	int i, attempt, loss;
	struct char_data *was_fighting;
  struct follow_type *group;

	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("You are in pretty bad shape, unable to flee!\r\n", ch);
		return;
	}

	if (IS_NPC(ch))	{
		if (MOB_FLAGGED(ch, MOB_TARGET)) {
			send_to_char("You're just a target, you can't run!", ch);
			return;
		}
	}

	for (i = 0; i < 6; i++) {
		attempt = number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
		if (CAN_GO(ch, attempt) &&
				!ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH) &&
				!ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_NOMOB)) {
			act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
			was_fighting = FIGHTING(ch);
			if (do_simple_move(ch, attempt, TRUE)) {
				send_to_char("You flee head over heels.\r\n", ch);
				if (was_fighting && !IS_NPC(ch)) {
					loss = GET_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
					loss *= MAX((GET_DAMROLL(was_fighting) - GET_DAMROLL(ch)), (GET_DAMROLL(ch) - GET_DAMROLL(was_fighting)));
					gain_exp(ch, -loss);
					if (IS_NPC(was_fighting) && MOB_FLAGGED(was_fighting, MOB_CHASE)) {
						do_simple_move(was_fighting, attempt, TRUE);
						send_to_char("You have been chased!\r\n", ch);
						hit(was_fighting, ch, TYPE_UNDEFINED);
						return;
					}
					if (!IS_NPC(was_fighting) && AFF_FLAGGED(was_fighting, AFF_CHASE)) {
						do_simple_move(was_fighting, attempt, TRUE);
						send_to_char("You chase your opponent!\r\n", was_fighting);
						send_to_char("You have been chased!\r\n", ch);
						hit(was_fighting, ch, TYPE_UNDEFINED);
						return;
					}
				}
        if (IS_NPC(ch)) {
					if (was_fighting) {
						if (AFF_FLAGGED(was_fighting, AFF_GROUP)) {
							if (was_fighting->master) {
								stop_fighting(was_fighting->master);
								for (group = was_fighting->master->followers; group; group = group->next) {
									if (AFF_FLAGGED(group->follower, AFF_GROUP) && IN_ROOM(group->follower) == IN_ROOM(was_fighting->master)) {
										stop_fighting(group->follower);
									}
								} 
							} else {
								stop_fighting(was_fighting);
								for (group = was_fighting->followers; group; group = group->next) {
									if (AFF_FLAGGED(group->follower, AFF_GROUP) && IN_ROOM(group->follower) == IN_ROOM(was_fighting))
										stop_fighting(group->follower);
								}
							} 
						}
						if (AFF_FLAGGED(was_fighting, AFF_CHASE)) {
							send_to_char("You chase your opponent!\r\n", was_fighting);
              act("$n chases after $N!", FALSE, was_fighting, 0, ch, TO_ROOM);
							perform_move(was_fighting, attempt, TRUE);
						} else
								stop_fighting(was_fighting);
					}
				}
			} else {
				act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
			}
			return;
		}
	}
	send_to_char("PANIC!  You couldn't escape!\r\n", ch);
}


ACMD(do_bash)
{
	struct char_data *vict;
	int percent = 0;

	if (IS_NPC(ch))	{
		if (MOB_FLAGGED(ch, MOB_TARGET)) {
			send_to_char("You're just a target, you can't fight!", ch);
			return;
		}
	}

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char("This room has a nice peaceful feeling.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BASH)) {
		send_to_char("You have no idea how.\r\n", ch);
		return;
	}
	if (!GET_EQ(ch, WEAR_WIELD)) {
		send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
		return;
	}
	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM, 0))) {
		if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
			vict = FIGHTING(ch);
		} else {
			send_to_char("Bash who?\r\n", ch);
			return;
		}
	}
	if (vict == ch) {
		send_to_char("Aren't we funny today...\r\n", ch);
		return;
	}

	if (MOB_FLAGGED(vict, MOB_NOBASH))
		percent = 101;

	if (!skill_check(ch, SKILL_BASH, percent)) {
		damage(ch, vict, 0, SKILL_BASH);
		GET_POS(ch) = POS_SITTING;
	} else {
		/*
		 * If we bash a player and they wimp out, they will move to the previous
		 * room before we set them sitting.  If we try to set the victim sitting
		 * first to make sure they don't flee, then we can't bash them!  So now
		 * we only set them sitting if they didn't flee. -gg 9/21/98
		 */
		if (damage(ch, vict, 1, SKILL_BASH) > 0) {	/* -1 = dead, 0 = miss */
			WAIT_STATE(vict, PULSE_VIOLENCE);
			if (IN_ROOM(ch) == IN_ROOM(vict))
				GET_POS(vict) = POS_SITTING;
		}
	}
	WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}


ACMD(do_rescue)
{
	struct char_data *vict, *tmp_ch;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_RESCUE)) {
		send_to_char("You have no idea how to do that.\r\n", ch);
		return;
	}

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char("This room has a nice peaceful feeling.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM, 0))) {
		send_to_char("Whom do you want to rescue?\r\n", ch);
		return;
	}
	if (vict == ch) {
		send_to_char("What about fleeing instead?\r\n", ch);
		return;
	}
	if (FIGHTING(ch) == vict) {
		send_to_char("How can you rescue someone you are trying to kill?\r\n", ch);
		return;
	}
	for (tmp_ch = world[IN_ROOM(ch)].people; tmp_ch &&
			 (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

	if (!tmp_ch) {
		act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	if (!skill_check(ch, SKILL_RESCUE, 0)) {
		send_to_char("You fail the rescue!\r\n", ch);
		return;
	}
	send_to_char("Banzai!  To the rescue...\r\n", ch);
	act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
	act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

	if (FIGHTING(vict) == tmp_ch)
		stop_fighting(vict);
	if (FIGHTING(tmp_ch))
		stop_fighting(tmp_ch);
	if (FIGHTING(ch))
		stop_fighting(ch);

	set_fighting(ch, tmp_ch);
	set_fighting(tmp_ch, ch);

	WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
}



ACMD(do_kick)
{
	struct char_data *vict;
	int percent;

	if (IS_NPC(ch))	{
		if (MOB_FLAGGED(ch, MOB_TARGET)) {
			send_to_char("You're just a target, you can't fight!", ch);
			return;
		}
	}

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char("This room has a nice peaceful feeling.\r\n", ch);
		return;
	}

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_KICK)) {
		send_to_char("You have no idea how.\r\n", ch);
		return;
	}
	one_argument(argument, arg);

	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM, 0))) {
		if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
			vict = FIGHTING(ch);
		} else {
			send_to_char("Kick who?\r\n", ch);
			return;
		}
	}
	if (vict == ch) {
		send_to_char("Aren't we funny today...\r\n", ch);
		return;
	}

	if (!ok_damage_shopkeeper(ch, vict))
		return;

	/* 101% is a complete failure */
	percent = ((10 - (compute_armor_class(vict) / 10)) * 2);

	if (!skill_check(ch, SKILL_KICK, percent)) {
		damage(ch, vict, 0, SKILL_KICK);
	} else
		damage(ch, vict, GET_SKILL(ch, SKILL_KICK) / 200, SKILL_KICK);

	WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}



/* NOTE: MOB_NOBASH prevents from disarming */
ACMD(do_disarm)
{
	int percent;
	struct obj_data *obj;
	struct char_data *vict;

	if (IS_NPC(ch))	{
		if (MOB_FLAGGED(ch, MOB_TARGET)) {
			send_to_char("You're just a target, you can't fight!", ch);
			return;
		}
	}

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char("This room has a nice peaceful feeling.\r\n", ch);
		return;
	}


	one_argument(argument, buf);

	percent = (!IS_NPC(ch) ? 0 : number(-100, 0));

	if (!*buf) {
		send_to_char("Whom do you want to disarm?\r\n", ch);
		return;
	}
	else if (!(vict = get_char_room_vis(ch, buf, NULL, 0))) {
		send_to_char(NOPERSON, ch);
		return;
	}
	else if (vict == ch) {
		send_to_char("Try removing your weapon instead.\r\n", ch);
		return;
	}
	else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict)) {
		send_to_char("The thought of disarming your master seems revolting to you.\r\n", ch);
		return;
	}
	else if (!ok_damage_shopkeeper(ch, vict))
		return;
	else if (!(obj = GET_EQ(vict, WEAR_WIELD)))
		act("$N is unarmed!", FALSE, ch, 0, vict, TO_CHAR);
	else if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_NODISARM) ||
			 MOB_FLAGGED(vict, MOB_NOBASH) || !skill_check(ch, SKILL_DEFEND, percent)) {
		act("You failed to disarm $N!", FALSE, ch, 0, vict, TO_CHAR);
		damage(vict, ch, number(0, GET_SKILL(ch, SKILL_DISARM) / 100), TYPE_HIT);
	}
	else if (dice(2, GET_STRENGTH(ch) / 100) + GET_SKILL(ch, SKILL_DISARM) <= dice(2, GET_STRENGTH(vict) / 100) + GET_SKILL(vict, SKILL_DISARM)) {
		act("You almost succeed in disarming $N", FALSE, ch, 0, vict, TO_CHAR);
		act("You were almost disarmed by $N!", FALSE, vict, 0, ch, TO_CHAR);
		damage(vict, ch, number(0, GET_SKILL(ch, SKILL_DISARM) / 200), TYPE_HIT);
	} else {
		obj_to_room(unequip_char(vict, WEAR_WIELD), IN_ROOM(vict));
		act("You succeed in disarming your enemy!", FALSE, ch, 0, 0, TO_CHAR);
		act("Your $p flies from your hands!", FALSE, vict, obj, 0, TO_CHAR);
		act("$n disarms $N, $p drops to the ground.", FALSE, ch, obj, vict, TO_ROOM);
		/* improve_skill(ch, SKILL_DISARM); */
	}
	hit(vict , ch, TYPE_UNDEFINED);
	WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}



ACMD(do_retreat)
{
	int percent;
	int retreat_type;
	
	if (IS_NPC(ch))	{
		if (MOB_FLAGGED(ch, MOB_TARGET)) {
			send_to_char("You're just a target, you can't move!", ch);
			return;
		}
	}

	one_argument(argument, arg);
	
	if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_RETREAT) == 0) {
		send_to_char("You don't know how to retreat!  Use flee instead!\r\n", ch);
		return;
	}
	if (!FIGHTING(ch))
	{
		send_to_char("You are not fighting!\r\n", ch);
		return;
	}
	if (!*arg)
	{
		send_to_char("Retreat where?!\r\n", ch);
		return;
	}
	
	retreat_type = search_block(arg, dirs, 0);
	
	if (retreat_type < 0 || !EXIT(ch, retreat_type) || EXIT(ch, retreat_type)->to_room == NOWHERE)
	{
		send_to_char("Retreat where?!\r\n", ch);
		return;
	}
	
	percent = !IS_NPC(ch) ? 0 : -50;
	
	if (skill_check(ch, SKILL_RETREAT, percent)){
		if (CAN_GO(ch, retreat_type) && !IS_SET(ROOM_FLAGS(EXIT(ch,retreat_type)->to_room), ROOM_DEATH))
		{
			act("$n skillfully retreats from combat.", TRUE, ch, 0, 0, TO_ROOM);
			send_to_char("You skillfully retreat from combat.\r\n", ch);
			WAIT_STATE(ch, PULSE_VIOLENCE);
			/* improve_skill(ch, SKILL_RETREAT); */
			do_simple_move(ch, retreat_type, TRUE);
			if (FIGHTING(ch) && FIGHTING(FIGHTING(ch)) == ch)
				stop_fighting(FIGHTING(ch));
			stop_fighting(ch);
		} else {
			act("$n tries to retreat from combat but has no where to go!", TRUE, ch, 0, 0, TO_ROOM);
			send_to_char("You cannot retreat in that direction!\r\n", ch);
			return;
		}
	} else {
		send_to_char("You fail your attempt to retreat!\r\n", ch);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return;
	}
}
