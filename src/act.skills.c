/* ************************************************************************
*		File: act.skills.c                                  Part of CircleMUD *
*	 Usage: Player-level skills                                             *
*																																					*
*	 This file put together by Torgny Bjers for Arcane Realms MUD           *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: act.skills.c,v 1.47 2004/03/16 03:02:57 cheron Exp $ */

#define	__ACT_SKILLS_C__

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
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "loadrooms.h"
#include "color.h"

/* extern variables */
extern sh_int mortal_start_room[NUM_STARTROOMS +1];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct spell_info_type *spell_info;
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct zone_data *zone_table;
extern int pt_allowed;
extern int max_filesize;
extern int track_through_doors;

/* extern procedures */
void list_skills(struct char_data *ch);
void appear(struct char_data *ch);
void perform_immort_vis(struct char_data *ch);
void die(struct char_data *ch, struct char_data *killer);
int	find_first_step(room_rnum src, room_rnum target);
SPECIAL(shop_keeper);
int	ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);
int	can_take_obj(struct char_data *ch, struct obj_data *obj);

/* local skill procedures */
SKILL(skl_bandage);
SKILL(skl_hide);
SKILL(skl_sneak);
SKILL(skl_streetwise);
SKILL(skl_find_hidden);
SKILL(skl_scan);
SKILL(skl_tutor);
SKILL(skl_forage);
SKILL(skl_dyeing);
SKILL(skl_steal);

/* local functions */
struct char_data *bandage_vict(struct char_data *ch, char *arg);
ACMD(do_forage);
ACMD(do_watch);
ACMD(do_dodge);
ACMD(do_defend);
ACMD(do_meditate);


#define	NUM_FORAGE_OBJS				4			/* The number of forage objects to random		*/
#define	NUM_RESOURCE_OBJS			6			/* The number of resource objects to random	*/

#define	START_FISHING_OBJ			60		/* starting VNUM of fishing objects					*/
#define	START_HUNTING_OBJ			70		/* starting VNUM of hunting objects					*/
#define	START_GATHER_OBJ			80		/* starting VNUM of gather objects					*/
#define	START_LUMBERJACK_OBJ	10213	/* starting VNUM of lumberjacking objects		*/
#define	START_MINING_OBJ			10201	/* starting VNUM of mining objects					*/

SKILL(skl_forage)
{
	struct obj_data *item_found = '\0';
	int item = 0, item_no = 0, num_objs = 1, resource = NOTHING, resource_type = NOTHING;
	int i;

	if (!IS_IMMORTAL(ch))
		GET_MOVE(ch) -= GET_MAX_MOVE(ch) / 20;

	// If we do not succeed, abort early.
	if (!success) {
		sprintf(buf, "Your attempts to %s are fruitless!\r\n", forage_skill[subcmd]);
		send_to_char(buf, ch);
		return;
	}

	switch (subcmd) {
	case SCMD_FORAGE_FISHING:
		item = number(0, NUM_FORAGE_OBJS - 1);
		item_no = START_FISHING_OBJ + item;
		break;
	case SCMD_FORAGE_HUNTING:
		item = number(0, NUM_FORAGE_OBJS - 1);
		item_no = START_HUNTING_OBJ + item;
		break;
	case SCMD_FORAGE_GATHER:
		item = number(0, NUM_FORAGE_OBJS - 1);
		item_no = START_GATHER_OBJ + item;
		break;
	case SCMD_FORAGE_LUMBERJACK:
		item = number(0, NUM_RESOURCE_OBJS - 1);
		resource = RESOURCE_WOOD;
		item_no = START_LUMBERJACK_OBJ + item;
		num_objs = number(3, 6);
		break;
	case SCMD_FORAGE_MINING:
		item = number(0, NUM_RESOURCE_OBJS - 1);
		resource = RESOURCE_ORE;
		item_no = START_MINING_OBJ + item;
		num_objs = number(1, 3);
		break;
	default:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "%s:%s(%d) reached default case.", __FILE__, __FUNCTION__, __LINE__);
		return;
	}

	// Check the area for available resources.
	if (resource != NOTHING) {
		resource_type = (resource * NUM_RESOURCE_TYPES) + item;
		if (ROOM_HAS_RESOURCE(IN_ROOM(ch), 1ULL << resource_type)) {
			if (GET_ROOM_RESOURCE(IN_ROOM(ch), resource) < num_objs) {
				sprintf(buf, "It seems that you cannot %s further here!\r\n", forage_skill[subcmd]);
				send_to_char(buf, ch);
				return;
			} else {
				GET_ROOM_RESOURCE(IN_ROOM(ch), resource) -= num_objs;
			}
		} else {
			sprintf(buf, "You %s for a while but end up with nothing!\r\n", forage_skill[subcmd]);
			send_to_char(buf, ch);
			return;
		}
	}

	// Sanity check for existing object, else we fail automatically.
	if ((item_found = read_object(item_no, VIRTUAL)) == NULL) {
		sprintf(buf, "You %s for a while but end up with nothing!\r\n", forage_skill[subcmd]);
		send_to_char(buf, ch);
		return;
	}

	// Give the object to the character.
	for (i = 0; i < num_objs; i++) {
		if (i > 0)
			item_found = read_object(item_no, VIRTUAL);
		// Check if they can take the object.
		if (can_take_obj(ch, item_found))
			obj_to_char(item_found, ch);
		else
			obj_to_room(item_found, IN_ROOM(ch));
	}

	sprintf(buf, "You %s %s in your attempt to %s!\r\n", forage_method[subcmd], item_found->short_description, forage_skill[subcmd]);
	send_to_char(buf, ch);
	
	sprintf(buf, "$n %s something in $s attempt to %s.\r\n", forage_method[subcmd], forage_skill[subcmd]);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
}


SKILL(skl_hide)
{
	if (!success)
		return;

	send_to_char("You melt into the shadows.\r\n", ch);

	SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
}


SKILL(skl_sneak)
{
	struct affected_type af;

	if (!success)
		return;

	send_to_char("You melt into the shadows.\r\n", ch);

	af.type = SKILL_SNEAK;
	af.duration = GET_SKILL(ch, SKILL_SNEAK)/250;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = AFF_SNEAK;
	affect_to_char(ch, &af);
}


/* utility function for bandage */
struct char_data *bandage_vict(struct char_data *ch, char *arg)
{
	struct char_data *vict;
	
	if (!*arg) {
		send_to_char("Apply bandages on who?\r\n", ch);
		return (NULL);
	} else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM, 0))) {
		send_to_char(NOPERSON, ch);
		return (NULL);
	} else
		return (vict);
}


SKILL(skl_bandage)
{
	if (ch == vict) {
		act("You apply $p on your wounds.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n applies $p on $s wounds.", FALSE, ch, obj, 0, TO_ROOM);
	} else {
		act("You apply $p on the wounds of $N.", FALSE, ch, obj, vict, TO_CHAR);
		act("$n applies $p on your wounds.", FALSE, ch, obj, vict, TO_VICT);
		act("$n applies $p on the wounds of $N.", TRUE, ch, obj, vict, TO_NOTVICT);
	}

	extract_obj(obj);

	if (success) {
		GET_HIT(vict) += (GET_SKILL(ch, SKILL_BANDAGE) / 200);

		if (GET_HIT(vict) >= GET_MAX_HIT(vict)) {
			if (ch == vict)
				send_to_char("You are now at full health.\r\n", ch);
			else {
				send_to_char("Your patient is now at full health.\r\n", ch);
				send_to_char("You are now at full health.\r\n", vict);
			}
			GET_HIT(vict) = GET_MAX_HIT(vict);
		}
	}
}


ACMD(do_defend)
{
	int success = FALSE;

	if (!GET_SKILL(ch, SKILL_DEFEND)) {
		send_to_char("You have no idea how to do that...\r\n", ch);
		return;
	}
	if (FIGHTING(ch) && GET_POS(ch) != POS_DEFEND)
		success = skill_check(ch, SKILL_DEFEND, 0);
	
	switch (GET_POS(ch)) {
	case POS_FIGHTING:
		if (success) {
			send_to_char("You try to defend yourself.\r\n", ch);
			act("$n tries to defend $mself.", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_DEFEND;
		} else
			send_to_char("You find it isn't so easy to defend yourself!\r\n", ch);
		break;
	case POS_DODGE:
		if (success) {
			send_to_char("You stop dodging, and try to defend yourself.\r\n", ch);
			act("$n stops dodging, and tries to defend $mself.", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_DEFEND;
		} else
			send_to_char("You find it isn't so easy to defend yourself!\r\n", ch);
		break;
	case POS_DEFEND:
		send_to_char("You are already defending yourself.\r\n", ch);
		break;
	default:
		send_to_char("You must be fighting someone to defend yourself!\r\n", ch);
		break;
	}
}


ACMD(do_dodge)
{
	int success = FALSE;

	if (!GET_SKILL(ch, SKILL_DODGE)) {
		send_to_char("You have no idea how to do that...\r\n", ch);
		return;
	}
	if (FIGHTING(ch) && GET_POS(ch) != POS_DODGE)
		success = skill_check(ch, SKILL_DODGE, 0);

	switch (GET_POS(ch)) {
	case POS_FIGHTING:
		if (success) {
			send_to_char("You try to dodge your attacker.\r\n", ch);
			act("$n tries to dodge $s attacker.", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_DODGE;
		} else
			send_to_char("You find it isn't so easy to dodge!\r\n", ch);
		break;
	case POS_DODGE:
		send_to_char("You are already dodging!\r\n", ch);
		break;
	case POS_DEFEND:
		if (success) {
			send_to_char("You stop defending yourself, and try to dodge instead.\r\n", ch);
			act("$n stops defending $mself, and tries to dodge instead.", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_DODGE;
		} else
			send_to_char("You find it isn't so easy to dodge!\r\n", ch);
		break;
	default:
		send_to_char("You must be fighting someone to dodge them!\r\n", ch);
		break;
	}
}


ACMD(do_watch)
{
	int success = FALSE;

	if (!GET_SKILL(ch, SKILL_WATCH)) {
		send_to_char("You have no idea how to do that...\r\n", ch);
		return;
	}
	if (GET_POS(ch) != POS_WATCHING)
		success = skill_check(ch, SKILL_WATCH, 0);

	switch (GET_POS(ch)) {
	case POS_FIGHTING:
	case POS_DODGE:
	case POS_DEFEND:
		send_to_char("You can't pay attention to anything but the fight!\r\n", ch);
		break;
	case POS_STANDING:
		if (success) {
			send_to_char("You put yourself on higher alert and start to keep watch.\r\n", ch);
			act("$n heightens $s awareness and keeps watch.", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_WATCHING;
		} else 
			send_to_char("You find you are easily distracted...\r\n", ch);
		break;
	case POS_SLEEPING:
		send_to_char("You must wake up first...\r\n", ch);
		break;
	case POS_WATCHING:
		send_to_char("You are already on watch!\r\n", ch);
		break;
	default:
		send_to_char("You feel too relaxed to keep watch.\r\n", ch);
		break;
	}
}


ACMD(do_meditate)
{
	int success = FALSE;

	if (!GET_SKILL(ch, SKILL_MEDITATE)) {
		send_to_char("You have no idea how to do that...\r\n", ch);
		return;
	}
	if (GET_POS(ch) != POS_MEDITATING)
		success = skill_check(ch, SKILL_MEDITATE, 0);

	switch (GET_POS(ch)) {
	case POS_FIGHTING:
	case POS_DODGE:
	case POS_DEFEND:
		send_to_char("You can't relax yourself enough when you're in a fight!\r\n", ch);
		break;
	case POS_STANDING:
		if (success) {
			send_to_char("You relax yourself slowly, and begin to meditate.\r\n", ch);
			act("$n relaxes $mself slowly, and begins to meditate.", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_MEDITATING;
		} else 
			send_to_char("You cannot seem to keep your concentration...\r\n", ch);
		break;
	case POS_SITTING:
	case POS_RESTING:
		if (success) {
			send_to_char("You begin to meditate.\r\n", ch);
			act("$n begins to meditate.\r\n", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_MEDITATING;
		} else 
			send_to_char("You cannot seem to keep your concentration...\r\n", ch);
		break;
	case POS_SLEEPING:
		send_to_char("You must wake up first...\r\n", ch);
		break;
	case POS_MEDITATING:
		send_to_char("You are already as concentrated as you can be!\r\n", ch);
		break;
	case POS_WATCHING:
		if (success) {
			send_to_char("You stand down from watch, relaxing yourself, and begin to meditate.\r\n", ch);
			act("$n stands down from watch, relaxing $mself, and begins to meditate.\r\n", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_MEDITATING;
		} else 
			send_to_char("You cannot seem to keep your concentration...\r\n", ch);
		break;
	default:
		send_to_char("You can't do that now...\r\n", ch);
		break;
	}
}


SKILL(skl_streetwise)
{
	int dir, home;
	char method[MAX_INPUT_LENGTH], hometown[MAX_INPUT_LENGTH];

	if (!*argument) {
		send_to_char("Usage: lore <method>\r\n", ch);
		return;
	}

	half_chop(argument, method, buf);
	half_chop(buf, hometown, buf);

	if (!skill_check(ch, SKILL_STREETWISE, -20)) {
		int tries = 10;
		/* Find a random direction. :) */
		do {
			dir = number(0, NUM_OF_DIRS - 1);
		} while (!CAN_GO(ch, dir) && --tries);
		sprintf(buf, "You do not really know where to go, but %s seems as good as any direction!\r\n", dirs[dir]);
		send_to_char(buf, ch);
		return;
	}

	for (home = 3; home <= NUM_MORTAL_STARTROOMS; home++)
		if (!strn_cmp(hometown, hometowns[home], strlen(hometown)))
			break;

	if (home < 3 || home > NUM_MORTAL_STARTROOMS) {
		send_to_char("I do not think that place exists.\r\n", ch);
		return;
	}

	dir = find_first_step(IN_ROOM(ch), real_room(mortal_start_room[home]));
	
	switch (dir) {
		case BFS_ERROR:
			send_to_char("Something seems to be wrong in the ether of the world.\r\n", ch);
			break;
		case BFS_ALREADY_THERE:
			send_to_char("You're already there.\r\n", ch);
			break;
		case BFS_NO_PATH:
			sprintf(buf, "You cannot find a way to &W%s&n from here.\r\n", hometowns[home]);
			send_to_char(buf, ch);
			break;
		default:	/* Success! */
			sprintf(buf, "In order to find &W%s&n you should go &W%s&n!\r\n", hometowns[home], dirs[dir]);
			send_to_char(buf, ch);
			break;
	}		
}


SKILL(skl_find_hidden)
{
	int num_chars = 0, num_objs = 0;
	struct char_data *i;
	struct obj_data *j;

	/*
	 * First detect any number of characters and mobiles in the room,
	 * print them out in a list.
	 */
	strcpy(buf, "You find the following beings");
	for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
		if (!CAN_SEE(ch, i) && (check_rights(ch, i) != -1) && skill_check(ch, SKILL_FIND_HIDDEN, 0)) {
			sprintf(buf + strlen(buf), "%s%s", (num_chars == 0)?": ":", ", GET_NAME(i));
			num_chars++;
		}
	}
	strcat(buf, ".\r\n");

	/*
	 * Then we list the objects that are hidden.
	 */
	strcpy(buf2, "You find the following objects");
	for (j = world[IN_ROOM(ch)].contents; j; j = j->next_content) {
		if (!CAN_SEE_OBJ(ch, j) && skill_check(ch, SKILL_FIND_HIDDEN, 15)) {
			sprintf(buf2 + strlen(buf2), "%s%s", (num_objs == 0)?": ":", ", j->short_description);
			num_objs++;				
		}			
	}
	strcat(buf2, ".\r\n");

	if (num_chars > 0) {
		send_to_char(buf, ch);
	}

	if (num_objs > 0) {
		send_to_char(buf2, ch);
	}
		
	if (!num_chars && !num_objs) {
		send_to_char("You do not find anything out of the ordinary.\r\n", ch);
	}

}


SKILL(skl_scan)
{
	struct char_data *i;
	int is_in, dir, dis, maxdis, found = 0;

	const char *distance[] = {
		"right here",
		"immediately ",
		"nearby ",
		"a ways ",
		"far ",
		"very far ",
		"extremely far ",
		"impossibly far ",
	};

	maxdis = (1 + ((GET_SKILL(ch, SKILL_SCAN) * 5) / 10000));
	if (IS_IMMORTAL(ch))
		maxdis = 7;

	if (IS_MORTAL(ch))
		GET_MOVE(ch) -= 1;

	if (!success)
		maxdis = 0;

	is_in = IN_ROOM(ch);
	send_to_char("You scan the area and see the following:\r\n", ch);
	for (dir = 0; dir < NUM_OF_DIRS; dir++) {
		IN_ROOM(ch) = is_in;
		for (dis = 0; dis <= maxdis; dis++) {
			if (((dis == 0) && (dir == 0)) || (dis > 0)) {
				for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
					if ((!((ch == i) && (dis == 0))) && CAN_SEE(ch, i)) {
						sprintf(buf, "%33s: %s%s%s%s", GET_NAME(i), distance[LIMIT(dis, 0, 7)],
										((dis > 0) && (dir < (NUM_OF_DIRS - 2))) ? "to the " : "",
										(dis > 0) ? dirs[dir] : "",
										((dis > 0) && (dir > (NUM_OF_DIRS - 3))) ? "wards" : "");
						act(buf, TRUE, ch, 0, 0, TO_CHAR);
						found++;
					}
				}
			}
			if (!CAN_GO(ch, dir) || (world[IN_ROOM(ch)].dir_option[dir]->to_room == is_in))
				break;
			else
				IN_ROOM(ch) = world[IN_ROOM(ch)].dir_option[dir]->to_room;
		}
	}
	if (found == 0)
		act("Nobody is anywhere near you.", TRUE, ch, 0, 0, TO_CHAR);
	IN_ROOM(ch) = is_in;
}


SKILL(skl_tutor)
{
	int sklnum = 0;

	act("You finish your lesson with $N.", FALSE, ch, 0, vict, TO_CHAR);
	act("$n finishes $s lesson with you.", FALSE, ch, obj, vict, TO_VICT);
	act("$n finished $s lesson with $N.", TRUE, ch, 0, vict, TO_NOTVICT);

	if (success) {
		if (!*argument) {
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "%s:%s(%d) called without an argument?", __FILE__, __FUNCTION__, __LINE__);
			return;
		} else if ((sklnum = find_skill_num(argument)) <= 0) {
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "%s:%s(%d) Skill [%s] not found!", __FILE__, __FUNCTION__, __LINE__, argument);
			return;
		} else {
			char *printbuf = get_buffer(1024);
			if (GET_SKILL(vict, sklnum) == 0)
				sprintf(printbuf, "You have gained '&W%s&n'.\r\n", skill_info[sklnum].name);
			else
				sprintf(printbuf, "Your lesson improves '&W%s&n'.\r\n", skill_info[sklnum].name);
			GET_SKILL(vict, sklnum) += GET_SKILL(ch, SKILL_TEACHING) / 8;
			send_to_char(printbuf, vict);
			release_buffer(printbuf);
		}
	}
}


ACMD(do_chase)
{
	int success = FALSE;

	skip_spaces(&argument);

	if (!*argument) {
		send_to_char("Usage: chase on|off\r\n", ch);
		return;
	}

	if (!str_cmp(argument, "on")) {
		if (AFF_FLAGGED(ch, AFF_CHASE)) 
			send_to_char("But you're already set on chasing your opponents.\r\n", ch);
		else if (!GET_SKILL(ch, SKILL_CHASE))
			send_to_char("You have no idea how to do that...\r\n", ch);
		else {		
			success = skill_check(ch, SKILL_CHASE, 0);
			
			if (success) {
				SET_BIT(AFF_FLAGS(ch), AFF_CHASE);
				send_to_char("You will now chase your opponents.\r\n", ch);
			}	else
				send_to_char("You think about chasing your opponents, but decide against it.\r\n", ch);
		}
		return;
	}

	if (!str_cmp(argument, "off")) {
		if (!AFF_FLAGGED(ch, AFF_CHASE))
			send_to_char("But you aren't trying to chase anyone.\r\n", ch);
		else {
			REMOVE_BIT(AFF_FLAGS(ch), AFF_CHASE);
			send_to_char("You stop chasing your opponents.\r\n", ch);
		}
		return;
	}	
	
	send_to_char("Usage: chase on|off\r\n", ch);
	return;
}


SKILL(skl_dyeing)
{
	struct obj_data *target;
	int col_requirement = 1;
	char vat_name[MAX_INPUT_LENGTH];
	char garb_name[MAX_INPUT_LENGTH];
	char color_name[64];

	half_chop(argument, vat_name, buf);
	half_chop(buf, garb_name, buf);

	if (!(target = get_obj_in_list_vis(ch, garb_name, NULL, ch->carrying))) {
		send_to_char("You have no such object in your inventory.\r\n", ch);
		return;
	}

	GET_OBJ_VAL(obj, 1) -= col_requirement;
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

	strlcpy(color_name, EXTENDED_COLORS[(int)GET_OBJ_COLOR(obj)].name, sizeof(color_name));

	if (success) {
		GET_OBJ_COLOR(target) = GET_OBJ_COLOR(obj);
		sprintf(garb_name, "You dye $p in %s %s color.", STRSANA(color_name), color_name);
		act(garb_name, FALSE, ch, target, 0, TO_CHAR);
		sprintf(garb_name, "$n dyes $p in %s %s color.", STRSANA(color_name), color_name);
		act(garb_name, FALSE, ch, target, 0, TO_ROOM);
	} else {
		act("You fail to dye $p, thus ruining the material permanently.", FALSE, ch, target, 0, TO_CHAR);
		act("$n fails to dye $p, thus ruining the material permanently.", FALSE, ch, target, 0, TO_ROOM);
		extract_obj(target);
	}
}


SKILL(skl_steal)
{
	struct obj_data *item;
	char obj_name[MAX_INPUT_LENGTH];
	int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

	one_argument(argument, obj_name);

	/* 101% is a complete failure */
	percent = -dex_app_skill[GET_AGILITY(ch) / 100].p_pocket;

	if (GET_POS(vict) < POS_SLEEPING)
		percent = -100;	/* ALWAYS SUCCESS, unless heavy object. */

	if (!pt_allowed && !IS_NPC(vict))
		pcsteal = 1;

	if (!AWAKE(vict))	/* Easier to steal from sleeping people. */
		percent -= 50;

	/* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
	if (IS_IMMORTAL(vict) || pcsteal ||	GET_MOB_SPEC(vict) == shop_keeper)
		percent = 101;	/* Failure */

	if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {
		if (!(item = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying))) {
			for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++) {
				if (HAS_BODY(vict, eq_pos) && GET_EQ(vict, eq_pos) &&
							(isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
							CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
					item = GET_EQ(vict, eq_pos);
					break;
				}
			}
			if (!item) {
				act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
				return;
			} else {	/* It is equipment */
				if ((GET_POS(vict) > POS_STUNNED)) {
					send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
					return;
				} else if (GET_OBJ_EXTRA(item) == ITEM_NOSTEAL) {
					send_to_char("The item resists!\r\n", ch);
					return;
				} else {
					act("You unequip $p and steal it.", FALSE, ch, item, 0, TO_CHAR);
					act("$n steals $p from $N.", FALSE, ch, item, vict, TO_NOTVICT);
					obj_to_char(unequip_char(vict, eq_pos), ch);
				}
			}
		} else {	/* item found in inventory */
			percent += GET_OBJ_WEIGHT(item);	/* Make heavy harder */
			if (!skill_check(ch, SKILL_STEAL, percent)) {
				ohoh = TRUE;
				send_to_char("Oops..\r\n", ch);
				act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
				act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
			} else {	/* Steal the item */
				if (GET_OBJ_EXTRA(item) == ITEM_NOSTEAL) {
					send_to_char("The item resists!\r\n", ch);
					return;
				}
				if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
					if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(item) < CAN_CARRY_W(ch)) {
						obj_from_char(item);
						obj_to_char(item, ch);
						send_to_char("Got it!\r\n", ch);
					}
				} else
					send_to_char("You cannot carry that much.\r\n", ch);
			}
		}
	} else {	/* Steal some coins */
		if (AWAKE(vict) && !skill_check(ch, SKILL_STEAL, percent)) {
			ohoh = TRUE;
			send_to_char("Oops..\r\n", ch);
			act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
			act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
		} else {
			/* Steal some gold coins */
			gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
			gold = MIN(1782, gold);
			if (gold > 0) {
				GET_GOLD(ch) += gold;
				GET_GOLD(vict) -= gold;
				if (gold > 1) {
					sprintf(buf, "Bingo!  You got %d gold coins.\r\n", gold);
					send_to_char(buf, ch);
				} else {
					send_to_char("You manage to swipe a solitary gold coin.\r\n", ch);
				}
			} else {
				send_to_char("You couldn't get any gold...\r\n", ch);
			}
		}
	}

	if (ohoh && IS_NPC(vict) && !MOB_FLAGGED(vict, MOB_TARGET) && AWAKE(vict)) {
		set_fighting(vict, ch);
		hit(vict, ch, TYPE_UNDEFINED);
	}
}

#undef __ACT_SKILLS_C__
