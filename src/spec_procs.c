/* ************************************************************************
*		File: spec_procs.c                                  Part of CircleMUD *
*	 Usage: implementation of special procedures for mobiles/objects/rooms  *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: spec_procs.c,v 1.38 2003/02/10 16:33:33 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

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
#include "constants.h"
#include "specset.h"
#include "loadrooms.h"
#include "color.h"

/*	 external vars  */
extern struct time_info_data time_info;
extern struct skill_info_type skill_info[];
extern struct spell_info_type *spell_info;
extern sh_int mortal_start_room[NUM_STARTROOMS +1];
extern int top_of_puff_messages;
extern const char *languages[];
extern struct puff_messages_element *puff_messages;

/* extern functions */
void add_follower(struct char_data *ch, struct char_data *leader);
void extract_char_final(struct char_data *ch);
ACMD(do_drop);
ACMD(do_gen_door);
ACMD(do_say);
ACMD(do_action);

/* local functions */
const	char *how_good(int percent);
void list_skills(struct char_data *ch);
void npc_steal(struct char_data *ch, struct char_data *victim);
void perform_remove(struct char_data *ch, int pos);
SPECIAL(invis_item);


/* ********************************************************************
*	 Special procedures for mobiles                                     *
******************************************************************** */

SPECIAL(dump)
{
	struct obj_data *k;
	int value = 0;

	for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
		act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
		extract_obj(k);
	}

	if (!CMD_IS("drop"))
		return (FALSE);

	do_drop(ch, argument, cmd, 0);

	for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
		act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
		value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
		extract_obj(k);
	}

	if (value) {
		send_to_char("You are awarded for outstanding performance.\r\n", ch);
		act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

		GET_GOLD(ch) += value;
	}
	return (TRUE);
}


SPECIAL(mayor)
{
	const char open_path[] =
				"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
	const char close_path[] =
				"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

	static const char *path = NULL;
	static int index;
	static bool move = FALSE;

	if (!move) {
		if (time_info.hours == 6) {
			move = TRUE;
			path = open_path;
			index = 0;
		} else if (time_info.hours == 20) {
			move = TRUE;
			path = close_path;
			index = 0;
		}
	}
	if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
			(GET_POS(ch) == POS_FIGHTING))
		return (FALSE);

	switch (path[index]) {
	case '0':
	case '1':
	case '2':
	case '3':
		perform_move(ch, path[index] - '0', 1);
		break;

	case 'W':
		GET_POS(ch) = POS_STANDING;
		act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'S':
		GET_POS(ch) = POS_SLEEPING;
		act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'a':
		act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
		act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'b':
		act("$n says 'What a view!  I must get something done about that dump!'",
				FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'c':
		act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
				FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'd':
		act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'e':
		act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'E':
		act("$n says 'I hereby declare Estoile closed!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'O':
		do_gen_door(ch, "gate", 0, SCMD_UNLOCK);
		do_gen_door(ch, "gate", 0, SCMD_OPEN);
		break;

	case 'C':
		do_gen_door(ch, "gate", 0, SCMD_CLOSE);
		do_gen_door(ch, "gate", 0, SCMD_LOCK);
		break;

	case '.':
		move = FALSE;
		break;

	}

	index++;
	return (FALSE);
}


/* ********************************************************************
*	 General special procedures for mobiles                             *
******************************************************************** */

void npc_steal(struct char_data *ch, struct char_data *victim)
{
	int gold;

	if (IS_NPC(victim))
		return;
	if (IS_IMMORTAL(victim))
		return;
	if (!CAN_SEE(ch, victim))
		return;

	if (AWAKE(victim) && (number(0, 50) == 0)) {
		act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
		act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
	} else {
		/* Steal some gold coins */
		gold = (int) ((GET_GOLD(victim) * number(1, 10)) / 100);
		if (gold > 0) {
			GET_GOLD(ch) += gold;
			GET_GOLD(victim) -= gold;
		}
	}
}


SPECIAL(snake)
{
	if (cmd)
		return (FALSE);

	if (GET_POS(ch) != POS_FIGHTING)
		return (FALSE);

	if (FIGHTING(ch) && (IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch)) &&
			(number(0, 42 - (number(0, 100))) == 0)) {
		act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
		act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
		call_magic(ch, FIGHTING(ch), 0, get_spell(SPELL_POISON, __FILE__, __FUNCTION__), number(1, 100), CAST_SPELL, 0);
		return (TRUE);
	}
	return (FALSE);
}


SPECIAL(thief)
{
	struct char_data *cons;

	if (cmd)
		return (FALSE);

	if (cmd || GET_POS(ch) != POS_STANDING)
		return (FALSE);

	for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
		if (!IS_NPC(cons) && (IS_MORTAL(cons)) && (!number(0, 4))) {
			npc_steal(ch, cons);
			return (TRUE);
		}
	return (FALSE);
}


SPECIAL(magic_user)
{
	struct char_data *vict;

	if (cmd || GET_POS(ch) != POS_FIGHTING)
		return (FALSE);

	/* pseudo-randomly choose someone in the room who is fighting me */
	for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
		if (FIGHTING(vict) == ch && !number(0, 4))
			break;

	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
		vict = FIGHTING(ch);

	/* Hm...didn't pick anyone...I'll wait a round. */
	if (vict == NULL)
		return (TRUE);

	if ((number(0, 20) > 13) && (number(0, 10) == 0))
		cast_spell(ch, vict, NULL, get_spell(SPELL_SLEEP, __FILE__, __FUNCTION__), 0);

	if ((number(0, 20) > 7) && (number(0, 8) == 0))
		cast_spell(ch, vict, NULL, get_spell(SPELL_BLINDNESS, __FILE__, __FUNCTION__), 0);

	if ((number(0, 20) > 12) && (number(0, 12) == 0)) {
		if (IS_EVIL(ch))
			cast_spell(ch, vict, NULL, get_spell(SPELL_ENERGY_DRAIN, __FILE__, __FUNCTION__), 0);
		else if (IS_GOOD(ch))
			cast_spell(ch, vict, NULL, get_spell(SPELL_DISPEL_EVIL, __FILE__, __FUNCTION__), 0);
	}
	if (number(0, 4))
		return (TRUE);

	switch (number(0, 20)) {
	case 4:
	case 5:
		cast_spell(ch, vict, NULL, get_spell(SPELL_MAGIC_MISSILE, __FILE__, __FUNCTION__), 0);
		break;
	case 6:
	case 7:
		cast_spell(ch, vict, NULL, get_spell(SPELL_CHILL_TOUCH, __FILE__, __FUNCTION__), 0);
		break;
	case 8:
	case 9:
		cast_spell(ch, vict, NULL, get_spell(SPELL_BURNING_HANDS, __FILE__, __FUNCTION__), 0);
		break;
	case 10:
	case 11:
		cast_spell(ch, vict, NULL, get_spell(SPELL_SHOCKING_GRASP, __FILE__, __FUNCTION__), 0);
		break;
	case 12:
	case 13:
		cast_spell(ch, vict, NULL, get_spell(SPELL_LIGHTNING_BOLT, __FILE__, __FUNCTION__), 0);
		break;
	case 14:
	case 15:
	case 16:
	case 17:
		cast_spell(ch, vict, NULL, get_spell(SPELL_COLOR_SPRAY, __FILE__, __FUNCTION__), 0);
		break;
	default:
		cast_spell(ch, vict, NULL, get_spell(SPELL_FIREBALL, __FILE__, __FUNCTION__), 0);
		break;
	}
	return (TRUE);

}


/* ********************************************************************
*	 Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(puff)
{
	int i = 0;

	if (cmd)
		return (FALSE);

	/* How can we send messages when we don't have any? */
	if (top_of_puff_messages < 0)
		return (FALSE);

	/* Randomize between 0 and the top puff message */
	i = number(0, top_of_puff_messages);

	/* We've got 30% chance of sending a message */
	if ((number(0, 10)) < 3) {
		do_say(ch, puff_messages[i].message, 0, 0);
		return (TRUE);
	} else {
		return (FALSE);
	}
}


SPECIAL(fido)
{

	struct obj_data *i, *temp, *next_obj;

	if (cmd || !AWAKE(ch))
		return (FALSE);

	for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
		if (!IS_CORPSE(i))
			continue;
	
		act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
		for (temp = i->contains; temp; temp = next_obj) {
			next_obj = temp->next_content;
			obj_from_obj(temp);
			obj_to_room(temp, IN_ROOM(ch));
		}
		extract_obj(i);
		return (TRUE);
	}

	return (FALSE);
}


SPECIAL(janitor)
{
	struct obj_data *i;

	if (cmd || !AWAKE(ch))
		return (FALSE);

	for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
		if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
			continue;
		if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
			continue;
		act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
		obj_from_room(i);
		obj_to_char(i, ch);
		return (TRUE);
	}

	return (FALSE);
}


SPECIAL(cityguard)
{
	struct char_data *tch, *evil;
	int max_evil;
	
	if (cmd || !AWAKE(ch) || FIGHTING(ch))
		return (FALSE);
	
	max_evil = 1000;
	evil = 0;
	
	for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
		if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
			act("$n screams &c'&CHEY!!!  You're one of those PLAYER KILLERS!!!!!!&c'&n", FALSE, ch, 0, 0, TO_ROOM);
			hit(ch, tch, TYPE_UNDEFINED);
			return (TRUE);
		}
	}

	for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
		if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_THIEF)){
			act("$n screams %c'&CHEY!!!  You're one of those PLAYER THIEVES!!!!!!&c'&n", FALSE, ch, 0, 0, TO_ROOM);
			hit(ch, tch, TYPE_UNDEFINED);
			return (TRUE);
		}
	}

	for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
		if (CAN_SEE(ch, tch) && FIGHTING(tch)) {
			if ((GET_ALIGNMENT(tch) < max_evil) &&
				(IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
				max_evil = GET_ALIGNMENT(tch);
				evil = tch;
			}
		}
	}

	if (evil && (GET_ALIGNMENT(FIGHTING(evil)) >= 0)) {
		act("$n screams &c'&CPROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!&c'&n", FALSE, ch, 0, 0, TO_ROOM);
		hit(ch, evil, TYPE_UNDEFINED);
		return (TRUE);
	}
	return (FALSE);
}


#define	PET_PRICE(pet) (GET_DIFFICULTY(pet) * 3000)

SPECIAL(pet_shops)
{
	char buf[MAX_STRING_LENGTH], pet_name[256];
	room_rnum pet_room;
	struct char_data *pet;

	pet_room = IN_ROOM(ch) + 1;

	if (CMD_IS("list")) {
		send_to_char("Available pets are:\r\n", ch);
		for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
			if (!IS_NPC(pet))
				continue;
			sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
			send_to_char(buf, ch);
		}
		return (TRUE);
	} else if (CMD_IS("buy")) {

		two_arguments(argument, buf, pet_name);

		if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet)) {
			send_to_char("There is no such pet!\r\n", ch);
			return (TRUE);
		}
		if (GET_GOLD(ch) < PET_PRICE(pet)) {
			send_to_char("You don't have enough gold!\r\n", ch);
			return (TRUE);
		}
		GET_GOLD(ch) -= PET_PRICE(pet);

		pet = read_mobile(GET_MOB_RNUM(pet), REAL);
		GET_EXP(pet) = 0;
		SET_BIT(AFF_FLAGS(pet), AFF_CHARM);

		if (*pet_name) {
			sprintf(buf, "%s %s", pet->player.name, pet_name);
			/* free(pet->player.name); don't free the prototype! */
			pet->player.name = str_dup(buf);

			sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
							pet->player.description, pet_name);
			/* free(pet->player.description); don't free the prototype! */
			pet->player.description = str_dup(buf);
		}
		char_to_room(pet, IN_ROOM(ch));
		add_follower(pet, ch);

		/* Be certain that pets can't get/carry/use/wield/wear items */
		IS_CARRYING_W(pet) = 1000;
		IS_CARRYING_N(pet) = 100;

		send_to_char("May you enjoy your pet.\r\n", ch);
		act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

		return (TRUE);
	}
	/* All commands except list and buy */
	return (FALSE);
}


SPECIAL(temple_cleric)
{
	struct char_data *vict;
	struct char_data *hitme = NULL;
	static int this_hour;
	float temp1 = 1;
	float temp2 = 0.75;
	
	if (cmd) return FALSE;
	
	if (time_info.hours != 0) {
				
		this_hour = time_info.hours;

		for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
			if (IS_AFFECTED(vict, AFF_POISON) && !IS_NPC(vict) && IS_SET(PLR_FLAGS(vict), PLR_NEWBIE)) hitme = vict;
		}
		if (hitme != NULL) {
			cast_spell(ch, hitme, NULL, get_spell(SPELL_REMOVE_POISON, __FILE__, __FUNCTION__), 0);
			return TRUE;
		}
		
		for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
			if (IS_AFFECTED(vict, AFF_BLIND) && !IS_NPC(vict) && IS_SET(PLR_FLAGS(vict), PLR_NEWBIE)) hitme = vict;
		}
		if (hitme != NULL) {
			cast_spell(ch, hitme, NULL, get_spell(SPELL_CURE_BLIND, __FILE__, __FUNCTION__), 0);
			return TRUE;
		}
		
		for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
			temp1 = GET_HIT(vict) / GET_MAX_HIT(vict);
			if (temp1 < temp2 && (number(0, 100) > 75) && !IS_NPC(vict) && IS_SET(PLR_FLAGS(vict), PLR_NEWBIE)) {
				temp2 = temp1;
				hitme = vict;
			}
		}
		if (hitme != NULL) {
			cast_spell(ch, hitme, NULL, get_spell(SPELL_HEAL, __FILE__, __FUNCTION__), 0);
			return TRUE;
		}

		for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
			if (!IS_AFFECTED(vict, AFF_SANCTUARY) && (number(0, 100) > 90) && !IS_NPC(vict) && IS_SET(PLR_FLAGS(vict), PLR_NEWBIE)) hitme = vict;
		}
		if (hitme != NULL) {
			cast_spell(ch, hitme, NULL, get_spell(SPELL_SANCTUARY, __FILE__, __FUNCTION__), 0);
			return TRUE;
		}
		
	}
	return 0;
}


SPECIAL	(summoned)
{
	if (cmd) return FALSE;
	if (!(IS_AFFECTED(ch, AFF_CHARM))) {
		act("$n disappears in a cloud of mist.", FALSE, ch, 0, 0, TO_ROOM);
		extract_char(ch);
		return TRUE;
	}
	return 0;
}


SPECIAL(banker)
{
	struct char_data *mob = (struct char_data *)me;
	int amount, player_i;

	if (FIGHTING(mob) || !AWAKE(mob))
		return (FALSE);

	if (CMD_IS("balance")) {
		if (GET_BANK_GOLD(ch) > 0)
			sprintf(buf, "$n tells you, '&WYour current balance is %d coins.&n'",
							GET_BANK_GOLD(ch));
		else
			sprintf(buf, "$n tells you, '&WYou currently have no money deposited.&n'");
		act(buf, FALSE, mob, 0, ch, TO_VICT);
		return (TRUE);
	} else if (CMD_IS("deposit")) {
		skip_spaces(&argument);
		if (!(strcmp(argument, "all"))) 
			amount = GET_GOLD(ch);
		else if ((amount = atoi(argument)) <= 0) {
			send_to_char("How much do you want to deposit?\r\n", ch);
			return (TRUE);
		}
		if (GET_GOLD(ch) < amount) {
			act("$n tells you, '&WYou don't have that many coins!&n'", FALSE, mob, 0, ch, TO_VICT);
			return (TRUE);
		}
		GET_GOLD(ch) -= amount;
		GET_BANK_GOLD(ch) += amount;
		sprintf(buf, "You deposit %d coins.\r\n", amount);
		send_to_char(buf, ch);
		act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
		return (TRUE);
	} else if (CMD_IS("withdraw")) {
		skip_spaces(&argument);
		if (!(strcmp(argument, "all")))
			amount = GET_BANK_GOLD(ch);
		else if ((amount = atoi(argument)) <= 0) {
			send_to_char("How much do you want to withdraw?\r\n", ch);
			return (TRUE);
		}
		if (GET_BANK_GOLD(ch) < amount) {
			act("$n tells you, '&WYou don't have that many coins deposited!&n'", FALSE, mob, 0, ch, TO_VICT);
			return (TRUE);
		}
		GET_GOLD(ch) += amount;
		GET_BANK_GOLD(ch) -= amount;
		sprintf(buf, "You withdraw %d coins.\r\n", amount);
		send_to_char(buf, ch);
		act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
		return (TRUE);
	} else if (CMD_IS("fundtransfer")) {
		struct char_data *victim;
		skip_spaces(&argument);
		two_arguments(argument, buf, buf2);
		if ((amount = atoi(buf)) <= 0) {
			send_to_char("How much do you want to transfer?\r\n", ch);
			return (TRUE);
		}
		if (GET_BANK_GOLD(ch) < amount) {
			act("$n tells you, '&WYou don't have that many coins deposited!&n'", FALSE, mob, 0, ch, TO_VICT);
			return (TRUE);
		}
		if (!(victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD))) {
			CREATE(victim, struct char_data, 1);
			clear_char(victim);
			CREATE(victim->player_specials, struct player_special_data, 1);
			if ((player_i = load_char(buf2, victim)) > -1) {
				char_to_room(victim, 0);
				GET_BANK_GOLD(victim) += amount;
				GET_BANK_GOLD(ch) -= amount;
				GET_PFILEPOS(victim) = player_i;
				save_char(victim, NOWHERE, FALSE);
				sprintf(buf, "You transfer %d coins to %s.\r\n", amount, GET_NAME(victim));
				send_to_char(buf, ch);
				act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
				extract_char_final(victim);
			} else {
				act("$n tells you, '&WThat person does not have an account here.&n'", FALSE, mob, 0, ch, TO_VICT);
				free_char(victim);
			}
		} else {
			GET_BANK_GOLD(victim) += amount;
			GET_BANK_GOLD(ch) -= amount;			
			sprintf(buf, "You transfer %d coins to %s.\r\n", amount, GET_NAME(victim));
			send_to_char(buf, ch);
			sprintf(buf, "%s just deposited %d coins into your account.\r\n", GET_NAME(ch), amount);
			send_to_char(buf, victim);
			act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
		}
		return (TRUE);
	} else
		return (FALSE);
}


/* ********************************************************************
*	 Special procedures for objects                                     *
******************************************************************** */

SPECIAL(bank)
{
	int amount;

	if (CMD_IS("balance")) {
		if (GET_BANK_GOLD(ch) > 0)
			sprintf(buf, "Your current balance is %d coins.\r\n",
							GET_BANK_GOLD(ch));
		else
			sprintf(buf, "You currently have no money deposited.\r\n");
		send_to_char(buf, ch);
		return (TRUE);
	} else if (CMD_IS("deposit")) {
		if ((amount = atoi(argument)) <= 0) {
			send_to_char("How much do you want to deposit?\r\n", ch);
			return (TRUE);
		}
		if (GET_GOLD(ch) < amount) {
			send_to_char("You don't have that many coins!\r\n", ch);
			return (TRUE);
		}
		GET_GOLD(ch) -= amount;
		GET_BANK_GOLD(ch) += amount;
		sprintf(buf, "You deposit %d coins.\r\n", amount);
		send_to_char(buf, ch);
		act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
		return (TRUE);
	} else if (CMD_IS("withdraw")) {
		if ((amount = atoi(argument)) <= 0) {
			send_to_char("How much do you want to withdraw?\r\n", ch);
			return (TRUE);
		}
		if (GET_BANK_GOLD(ch) < amount) {
			send_to_char("You don't have that many coins deposited!\r\n", ch);
			return (TRUE);
		}
		GET_GOLD(ch) += amount;
		GET_BANK_GOLD(ch) -= amount;
		sprintf(buf, "You withdraw %d coins.\r\n", amount);
		send_to_char(buf, ch);
		act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
		return (TRUE);
	} else
		return (FALSE);
}

/* Below I adjusted the spec_proc to remove the check for specific vnum */
/* This version has been tested and works */
/* You can now assign any object this spec_proc and it will work */

SPECIAL(invis_item)
{
	int i;
	char arg1[MAX_INPUT_LENGTH];
	struct obj_data *invis_obj = (struct obj_data *)me;
	/* cast the "me" pointer and assign it to invis_obj */
	if (invis_obj->worn_by == ch) {
		/* check to see if the person carrying the invis_obj is the character */
		if(CMD_IS("disappear")) {
			send_to_char("You slowly fade out of view.\r\n", ch);
			act("$n slowly fades out of view.\r\n", FALSE, ch, 0, 0,TO_ROOM);
			SET_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
			return (TRUE);
		}

		if (CMD_IS("appear")) {
			REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
			send_to_char("You slowly fade into view.\r\n", ch);

			act("$n slowly fades into view.\r\n", FALSE, ch, 0, 0,TO_ROOM);
			return(TRUE);
		}

		one_argument(argument, arg1);

		if (is_abbrev(arg1, "magical")) {
			for (i=0; i< NUM_WEARS; i++) {
				if (GET_EQ(ch, i)) {
					if (IS_SET(AFF_FLAGS(ch), AFF_INVISIBLE)) {
						REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
						perform_remove(ch, i);
						send_to_char("You slowly fade into view.\r\n", ch);
						act("$n slowly fades into view.\r\n", FALSE, ch, 0, 0, TO_ROOM);
						return (TRUE);
					} else {
						perform_remove(ch, i);
						return (TRUE);
					}
					return (FALSE);
				}
			}
			return (FALSE);
		}
		return (FALSE);
	}
	return (FALSE);
}


SPECIAL	(portal)
{
	struct obj_data *obj = (struct obj_data *) me;
	struct obj_data *port;
	char obj_name[MAX_STRING_LENGTH];

		if (!CMD_IS("enter")) return FALSE;

		argument = one_argument(argument,obj_name);
		if (!(port = get_obj_in_list_vis(ch, obj_name, NULL, world[IN_ROOM(ch)].contents)))        {
			return(FALSE);
		}
		
		if (port != obj)
			return(FALSE);
		
		if (port->obj_flags.value[1] <= 0 ||
			port->obj_flags.value[1] > 32000) {
			send_to_char("The portal leads nowhere.\r\n", ch);
			return TRUE;
		}
		
		act("\r\n&M$n enters $p, and vanishes!&n", FALSE, ch, port, 0, TO_ROOM);
		act("\r\n&MYou enter $p, and you are transported elsewhere...&n\r\n", FALSE, ch, port, 0, TO_CHAR);
		char_from_room(ch);  
		char_to_room(ch, port->obj_flags.value[1]);
		look_at_room(ch,0);
		act("\r\n&M$n enters the room through a portal.&n", FALSE, ch, 0, 0, TO_ROOM);
	return TRUE;
}


#define	RACE_STARTROOM          4
#define	HUNTER_STARTROOM        4
#define	CLERIC_TYPE_STARTROOM   4
#define	MAGI_TYPE_STARTROOM     4
#define	WARRIOR_TYPE_STARTROOM  4
#define	THIEF_TYPE_STARTROOM    4
#define IMMORTAL_STARTROOM			8

SPECIAL	(start_portal)
{
	int location;
	struct obj_data *obj = (struct obj_data *) me;
	struct obj_data *port;
	char obj_name[MAX_STRING_LENGTH];

	if (!CMD_IS("enter"))
		return FALSE;

	argument = one_argument(argument,obj_name);
	if (!(port = get_obj_in_list_vis(ch, obj_name, NULL, world[IN_ROOM(ch)].contents)))        {
		return(FALSE);
	}
	
	if (port != obj)
		return(FALSE);

	if (!IS_HUMAN(ch)) {
		location = RACE_STARTROOM;
	} else if (GET_CLASS(ch) == CLASS_HUNTER && IS_HUMAN(ch)) {
		location = HUNTER_STARTROOM;
	} else {
		switch (GET_CLASS(ch)) {
			case CLASS_MAGUS_ANIMAL:
			case CLASS_MAGUS_AQUAM:
			case CLASS_MAGUS_AURAM:
			case CLASS_MAGUS_CORPOREM:
			case CLASS_MAGUS_HERBAM:
			case CLASS_MAGUS_IGNEM:
			case CLASS_MAGUS_IMAGONEM:
			case CLASS_MAGUS_MENTEM:
			case CLASS_MAGUS_TERRAM:
			case CLASS_MAGUS_VIM:
				location = MAGI_TYPE_STARTROOM;
				break;
			case CLASS_MONK:
			case CLASS_PALADIN:
				location = CLERIC_TYPE_STARTROOM;
				break;
			case CLASS_BARD:
			case CLASS_WARRIOR:
				location = WARRIOR_TYPE_STARTROOM;
				break;
			case CLASS_THIEF:
			case CLASS_GLADIATOR:
			case CLASS_ASSASSIN:
				location = THIEF_TYPE_STARTROOM;
				break;
			default:
				location = WARRIOR_TYPE_STARTROOM;
				break;
		}
	}

	if (GET_HOME(ch) <= 1) {
		if (!GOT_RIGHTS(ch, RIGHTS_IMMORTAL))
			GET_HOME(ch) = location;
		else
			GET_HOME(ch) = IMMORTAL_STARTROOM;
	}

	act("\r\n&M$n enters $p, and vanishes!&n", FALSE, ch, port, 0, TO_ROOM);
	act("\r\n&MYou enter $p, and you are transported elsewhere...&n\r\n", FALSE, ch, port, 0, TO_CHAR);
	char_from_room(ch);
	char_to_room(ch, real_room(mortal_start_room[location]));
	look_at_room(ch, 0);
	act("\r\n&MA portal appears in a swirling cloud of matter, and $n steps out.&n", FALSE, ch, 0, 0, TO_ROOM);
	return TRUE;
}


SPECIAL(dye_fill)
{
	struct obj_data *obj = (struct obj_data *) me;
	struct obj_data *target;
	char obj_name[MAX_STRING_LENGTH];
	char color_name[64];

	if (!CMD_IS("dyefill"))
		return (FALSE);

	if (GET_OBJ_TYPE(obj) != ITEM_TOOL) {
		act("$p does not appear to be a dye.", FALSE, ch, obj, 0, TO_CHAR);
		return (TRUE);
	}

	if (GET_OBJ_COLOR(obj) == 0) {
		act("$p does not appear to be a color base.\r\n", FALSE, ch, obj, 0, TO_CHAR);
		return (TRUE);
	}

	argument = one_argument(argument, obj_name);

	if (!(target = get_obj_in_list_vis(ch, obj_name, NULL, ch->carrying))) {
		send_to_char("You have no such object in your inventory.\r\n", ch);
		return (TRUE);
	}

	if (target == obj) {
		send_to_char("You cannot fill that with itself, silly.\r\n", ch);
		return (TRUE);
	}

	if (GET_OBJ_TYPE(target) != ITEM_DRINKCON) {
		send_to_char("That does not appear to be a liquid container.\r\n", ch);
		return (TRUE);
	}

	if (GET_OBJ_VAL(target, 2) != LIQ_COLOR) {
		send_to_char("That does not appear suited for dyes.\r\n", ch);
		return (TRUE);
	}

	if (GET_OBJ_VAL(target, 1) > 0 && GET_OBJ_COLOR(target) != GET_OBJ_COLOR(obj)) {
		send_to_char("That container has dye in it already, empty it first.\r\n", ch);
		return (TRUE);
	}

	// Set the color.
	GET_OBJ_COLOR(target) = GET_OBJ_COLOR(obj);
	// Set the liquid units to max in the vat.
	GET_OBJ_VAL(target, 1) = GET_OBJ_VAL(target, 0);
	// Save this object down uniquely to preserve color.
	SET_BIT(GET_OBJ_EXTRA(target), ITEM_UNIQUE_SAVE);

	strlcpy(color_name, EXTENDED_COLORS[(int)GET_OBJ_COLOR(target)].name, sizeof(color_name));

	sprintf(obj_name, "You fill $p with %s %s color.", STRSANA(color_name), color_name);
	act(obj_name, FALSE, ch, target, 0, TO_CHAR);
	act("$n fills $p with color.", FALSE, ch, target, 0, TO_ROOM);

	extract_obj(obj);

	return (TRUE);

}
