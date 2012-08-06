/* ************************************************************************
*	 File: act.movement.c                                Part of CircleMUD	*
*	 Usage: movement commands, door handling, & sleep/rest/etc state        *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: act.movement.c,v 1.35 2004/04/16 16:15:03 cheron Exp $ */

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
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "events.h"
#include "loadrooms.h"

/* external variables  */
extern int tunnel_size;
extern sh_int mortal_start_room[NUM_STARTROOMS +1];

/* external functions */
void add_follower(struct char_data *ch, struct char_data *leader);
int	special(struct char_data *ch, int cmd, char *arg);
void death_cry(struct char_data *ch);
int	find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);

/* local functions */
int	has_boat(struct char_data *ch);
int	find_door(struct char_data *ch, char *type, char *dir, const char *cmdname);
int	has_key(struct char_data *ch, obj_vnum key);
void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd, char *arg);
int	ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd);
void saythrough_door(struct char_data *ch, struct obj_data *obj, int door, struct room_direction_data *back, char *arg);
void knock_door(struct char_data *ch, struct obj_data *obj, int door, struct room_direction_data *back, char *arg);
ACMD(do_gen_door);
ACMD(do_enter);
ACMD(do_leave);
ACMD(do_stand);
ACMD(do_sit);
ACMD(do_rest);
ACMD(do_sleep);
ACMD(do_wake);
ACMD(do_follow);


/* simple function to determine if char can walk on water */
int	has_boat(struct char_data *ch)
{
	struct obj_data *obj;
	int i;

	if (IS_IMMORTAL(ch))
		return (1);

	if (AFF_FLAGGED(ch, AFF_WATERWALK))
		return (1);

	/* non-wearable boats in inventory will do it */
	for (obj = ch->carrying; obj; obj = obj->next_content)
		if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
			return (1);

	/* and any boat you're wearing will do it too */
	for (i = 0; i < NUM_WEARS; i++)
		if (HAS_BODY(ch, i) && GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
			return (1);

	return (0);
}

	

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If success.
 *   0 : If fail.
 */
int	do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{
	room_rnum was_in;
	int need_movement;
	struct char_data *room;

	/*
	 * If they are in a zone that's not OOC, and they are mortals,
	 * we move them to the lounge if they're OOC.
	 */
	if (IS_OOC(ch) && !ZONE_FLAGGED(IN_ZONE(ch), ZONE_OOC) && IS_MORTAL(ch)) {
		room_rnum lounge = real_room(mortal_start_room[ooc_lounge_index]);
		send_to_char("\r\n&RYou are not allowed here when OOC. Transporting you to OOC Lounge&n\r\n\r\n", ch);
		char_from_room(ch);
		char_to_room(ch, lounge);
		if (ch->desc != NULL)
			look_at_room(ch, 0);
		return (0);
	}
	
	/*
	 * Check for special routines (North is 1 in command list, but 0 here) Note
	 * -- only check if following; this avoids 'double spec-proc' bug
	 */
	if (need_specials_check && special(ch, dir + 1, NULL)) /* XXX: Evaluate NULL */
		return (0);

	/* blocked by a leave trigger? */
	if (!leave_mtrigger(ch, dir))
		return (0);
	if (!leave_wtrigger(&world[IN_ROOM(ch)], ch, dir))
		return (0);

	if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_NOMOB) && IS_NPC(ch))
		return (0);

	/* charmed? */
	if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && IN_ROOM(ch) == IN_ROOM(ch->master)) {
		send_to_char("The thought of leaving your master makes you weep.\r\n", ch);
		act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
		return (0);
	}
	
	/* if this room or the one we're going to needs a boat, check for one */
	if ((SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) ||
		(SECT(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM)) {
		if (!has_boat(ch)) {
			send_to_char("You need a boat to go there.\r\n", ch);
			return (0);
		}
	}
	
	/* move points needed is avg. move loss for src and destination sect type */
	need_movement = (movement_loss[SECT(IN_ROOM(ch))] +
		movement_loss[SECT(EXIT(ch, dir)->to_room)]) / 2;

	if (GET_MOVE(ch) < need_movement && !IS_NPC(ch)) {
		if (need_specials_check && ch->master)
			send_to_char("You are too exhausted to follow.\r\n", ch);
		else
			send_to_char("You are too exhausted.\r\n", ch);
		
		return (0);
	}
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ATRIUM)) {
		if (!house_can_enter(ch, GET_ROOM_VNUM(EXIT(ch, dir)->to_room))) {
			send_to_char("That's private property -- no trespassing!\r\n", ch);
			return (0);
		}
	}
	if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_TUNNEL) &&
			num_pc_in_room(&(world[EXIT(ch, dir)->to_room])) >= tunnel_size) {
		if (tunnel_size > 1)
			send_to_char("There isn't enough room for you to go there!\r\n", ch);
		else
			send_to_char("There isn't enough room there for more than one person!\r\n", ch);
		return (0);
	}
	/* Mortals and low rights gods cannot enter greater god rooms. */
	if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GODROOM) &&
		!IS_GRGOD(ch)) {
		send_to_char("You aren't godly enough to use that room!\r\n", ch);
		return (0);
	}
	
	/* If we cannot see regio entrances, we can't enter the room. */
	if ((ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_FAERIE) && !CAN_SEE_FAERIE(ch)) ||
			(ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_INFERNAL) && !CAN_SEE_INFERNAL(ch)) ||
			(ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_DIVINE) && !CAN_SEE_DIVINE(ch)) ||
			(ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_ANCIENT) && !CAN_SEE_ANCIENT(ch))) {
		send_to_char("Alas, you cannot go that way...\r\n", ch);
		return (0);
	}
	
	/* see if an entry trigger disallows the move */
	if (!entry_mtrigger(ch))
		return (0);
	if (!enter_wtrigger(&world[EXIT(ch, dir)->to_room], ch, dir))
		return (0);

	/* Now we know we're allow to go into the room. */
	if (IS_MORTAL(ch) && !IS_NPC(ch))
		GET_MOVE(ch) -= need_movement;
	
	/* If they are entering a regio, we can't see where they are going. */
	if ((ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_FAERIE) && CAN_SEE_FAERIE(ch)) ||
			(ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_INFERNAL) && CAN_SEE_INFERNAL(ch)) ||
			(ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_DIVINE) && CAN_SEE_DIVINE(ch)) ||
			(ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_ANCIENT) && CAN_SEE_ANCIENT(ch))) {
		sprintf(buf2, "$n has disappeared...");
		act(buf2, TRUE, ch, 0, 0, TO_ROOM);
	} else {
		/* if the exit is not regio, print out direction. */
		if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
			if (!IS_NPC(ch) && GET_TRAVELS(ch) && GET_TRAVELS(ch)->mout) {
				 strcpy(buf2, GET_TRAVELS(ch)->mout);
				 replace(buf2, "*dir*", dirs[dir], REPL_NORMAL);
			} else
				 sprintf(buf2, "%s%s.", travel_defaults[TRAV_MOUT], dirs[dir]);
			act(buf2, TRUE, ch, 0, 0, TO_ROOM);
		}

	}
	
	was_in = IN_ROOM(ch);
	char_from_room(ch);
	char_to_room(ch, world[was_in].dir_option[dir]->to_room);
	
	if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
		char *printbuf = get_buffer(2048);
		if (GET_TRAVELS(ch) && GET_TRAVELS(ch)->min)
			strcpy(printbuf, GET_TRAVELS(ch)->min);
		else
			strcpy(printbuf, travel_defaults[TRAV_MIN]);
		act(printbuf, TRUE, ch, 0, 0, TO_ROOM);
		release_buffer(printbuf);
	} else {
		for (room = world[was_in].people; room; room = room->next_in_room) {
			if (AFF_FLAGGED(room, AFF_SENSE_LIFE)) {
				act("You sense someone leaving.", FALSE, ch, 0, room, TO_VICT);
			}
		}		
		for (room = world[IN_ROOM(ch)].people; room; room = room->next_in_room) {
			if (AFF_FLAGGED(room, AFF_SENSE_LIFE) && ch != room) {
				act("You sense someone approaching.", FALSE, ch, 0, room, TO_VICT);
			}
		}		
	}
	
	if (ch->desc != NULL)
		look_at_room(ch, 0);
	
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) && IS_MORTAL(ch)) {
		log_death_trap(ch);
		death_cry(ch);
		extract_char(ch);
		return (0);
	}

	entry_memory_mtrigger(ch);
	if (!greet_mtrigger(ch, dir)) {
		char_from_room(ch);
		char_to_room(ch, was_in);
		look_at_room(ch, 0);
	} else greet_memory_mtrigger(ch);

	return (1);
}


int	perform_move(struct char_data *ch, int dir, int need_specials_check)
{
	room_rnum was_in;
	struct follow_type *k, *next;
	
	if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
		return (0);
	else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE ||
		(EXIT_FLAGGED(EXIT(ch, dir), EX_FAERIE) && !CAN_SEE_FAERIE(ch)) ||
		(EXIT_FLAGGED(EXIT(ch, dir), EX_INFERNAL) && !CAN_SEE_INFERNAL(ch)) ||
		(EXIT_FLAGGED(EXIT(ch, dir), EX_DIVINE) && !CAN_SEE_DIVINE(ch)) ||
		(EXIT_FLAGGED(EXIT(ch, dir), EX_ANCIENT) && !CAN_SEE_ANCIENT(ch)))
		send_to_char("Alas, you cannot go that way...\r\n", ch);
	else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
		if (EXIT(ch, dir)->keyword) {
			sprintf(buf2, "The %s seems to be closed.\r\n", fname(EXIT(ch, dir)->keyword));
			send_to_char(buf2, ch);
		} else
			send_to_char("It seems to be closed.\r\n", ch);
	} else {
		if (!ch->followers)
			return (do_simple_move(ch, dir, need_specials_check));
		
		was_in = IN_ROOM(ch);
		if (!do_simple_move(ch, dir, need_specials_check))
			return (0);
		
		for (k = ch->followers; k; k = next) {
			next = k->next;
			if ((IN_ROOM(k->follower) == was_in) &&
				(GET_POS(k->follower) >= POS_STANDING)) {
				act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
				perform_move(k->follower, dir, 1);
			}
		}
		return (1);
	}
	return (0);
}


ACMD(do_move)
{
	int nr_times = 1, loop;
	/*
	 * This is basically a mapping of cmd numbers to perform_move indices.
	 * It cannot be done in perform_move because perform_move is called
	 * by other functions which do not require the remapping.
	 */

	/*
	 * You cannot move around and do skills at the same time.
	 * Torgny Bjers, 2002-06-13.
	 */
	if (GET_PLAYER_EVENT(ch, EVENT_SKILL)) {
		send_to_char("You are occupied with a task, and cannot move.\r\n", ch);
		return;
	}

	/*
	 * Speedwalking code submitted to Ceramic Mouse
	 * From: Brian Christopher Guilbault <guil9964@nova.gmi.edu>
	 */

	if(!*argument)
		nr_times = 1;
	if(*argument) {
		if(atoi(argument) <= 0)
			nr_times = 1;
		else
			nr_times = atoi(argument);
	}
	if (nr_times <= 15) {
		for(loop = 0; loop < nr_times; loop++) {
			perform_move(ch, subcmd - 1, 0);
			if (loop < nr_times-1)
				send_to_char("\r\n",ch);
		}
	}
	else
		send_to_char("Please limit your speedwalking to 15 moves per direction.\r\n", ch);
}


int	find_door(struct char_data *ch, char *type, char *dir, const char *cmdname)
{
	int door, found = 0;
	
	if (*dir) {                        /* a direction was specified */
		if ((door = search_block(dir, dirs, FALSE)) < 0 &&
							(door = search_block(dir, abbr_dirs, FALSE)) < 0) { /* Partial Match */
			send_to_char("That's not a direction.\r\n", ch);
			return (-1);
		}
		if (EXIT(ch, door)) {        /* Braces added according to indent. -gg */
			if (EXIT(ch, door)->keyword) {
				if (isname(type, EXIT(ch, door)->keyword))
					return (door);
				else {
					sprintf(buf2, "I see no %s there.\r\n", type);
					send_to_char(buf2, ch);
					return (-1);
				}
			} else
				return (door);
		} else {
			sprintf(buf2, "I really don't see how you can %s anything there.\r\n", cmdname);
			send_to_char(buf2, ch);
			return (-1);
		}
	} else {                        /* try to locate the keyword */
		if (!*type) { 
			sprintf(buf2, "What is it you want to %s?\r\n", cmdname);
			send_to_char(buf2, ch);
			return (-1);
		}
		if ((door = search_block(type, dirs, FALSE)) > 0 || 
					(door = search_block(type, abbr_dirs, FALSE)) > 0) {  /* twas a direction! */
			found = 1;
		}
		
		if (!found) {
			for (door = 0; door < NUM_OF_DIRS; door++)
				if (EXIT(ch, door))
					if (EXIT(ch, door)->keyword)
						if (isname(type, EXIT(ch, door)->keyword))
							return (door);

			sprintf(buf2, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
			send_to_char(buf2, ch);
			return (-1);
		} else {
			if (EXIT(ch, door))
				return (door);
			else {
				sprintf(buf2, "I really don't see how you can %s anything there.\r\n", cmdname);
				send_to_char(buf2, ch);
				return (-1);
			}
		}
	}
}


int	has_key(struct char_data *ch, obj_vnum key)
{
	struct obj_data *o;

	for (o = ch->carrying; o; o = o->next_content)
		if (GET_OBJ_VNUM(o) == key)
			return (1);

	if (GET_EQ(ch, WEAR_HOLD))
		if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key)
			return (1);

	return (0);
}



#define	NEED_OPEN        (1 << 0)
#define	NEED_CLOSED        (1 << 1)
#define	NEED_UNLOCKED        (1 << 2)
#define	NEED_LOCKED        (1 << 3)

const	char *cmd_door[] =
{
	"open",
	"close",
	"unlock",
	"lock",
	"pick",
	"knock on",
	"say through"
};

const	int flags_door[] =
{
	NEED_CLOSED | NEED_UNLOCKED,
	NEED_OPEN,
	NEED_CLOSED | NEED_LOCKED,
	NEED_CLOSED | NEED_UNLOCKED,
	NEED_CLOSED | NEED_LOCKED
};


#define	EXITN(room, door)                (world[room].dir_option[door])
#define	OPEN_DOOR(room, obj, door)        ((obj) ?\
												(REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
												(REMOVE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define CLOSE_DOOR(room, obj, door)	((obj) ?\
												(SET_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
												(SET_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define	LOCK_DOOR(room, obj, door)        ((obj) ?\
												(SET_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
												(SET_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define UNLOCK_DOOR(room, obj, door)	((obj) ?\
												(REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
												(REMOVE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define	DOOR_IS_OPENABLE(ch, obj, door)        ((obj) ? \
												((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && \
												OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) :\
												(EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define	DOOR_IS_OPEN(ch, obj, door)        ((obj) ? \
												(!OBJVAL_FLAGGED(obj, CONT_CLOSED)) :\
												(!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define	DOOR_IS_UNLOCKED(ch, obj, door)        ((obj) ? \
												(!OBJVAL_FLAGGED(obj, CONT_LOCKED)) :\
												(!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))
#define	DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
												(OBJVAL_FLAGGED(obj, CONT_PICKPROOF)) : \
												(EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))

#define	DOOR_IS_CLOSED(ch, obj, door)		(!(DOOR_IS_OPEN(ch, obj, door)))
#define	DOOR_IS_LOCKED(ch, obj, door)		(!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define	DOOR_KEY(ch, obj, door)					((obj) ? (GET_OBJ_VAL(obj, 2)) : \
																				(EXIT(ch, door)->key))
#define	DOOR_LOCK(ch, obj, door)				((obj) ? (GET_OBJ_VAL(obj, 1)) : \
																				(EXIT(ch, door)->exit_info))

void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd, char *arg)
{
	int other_room = 0;
	struct room_direction_data *back = NULL;

	if (!door_mtrigger(ch, scmd, door))
		return;

	if (!door_wtrigger(ch, scmd, door))
		return;

	sprintf(buf, "$n %ss ", cmd_door[scmd]);
	if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
		if ((back = world[other_room].dir_option[rev_dir[door]]) != NULL)
			if (back->to_room != IN_ROOM(ch))
				back = 0;

	switch (scmd) {
	case SCMD_OPEN:
		OPEN_DOOR(IN_ROOM(ch), obj, door);
		if (back)
			OPEN_DOOR(other_room, obj, rev_dir[door]);
		send_to_char(OK, ch);
		break;

	case SCMD_CLOSE:
		CLOSE_DOOR(IN_ROOM(ch), obj, door);
		if (back)
			CLOSE_DOOR(other_room, obj, rev_dir[door]);
		send_to_char(OK, ch);
		break;

	case SCMD_LOCK:
		LOCK_DOOR(IN_ROOM(ch), obj, door);
		if (back)
			LOCK_DOOR(other_room, obj, rev_dir[door]);
		send_to_char("*Click*\r\n", ch);
		break;

	case SCMD_UNLOCK:
		UNLOCK_DOOR(IN_ROOM(ch), obj, door);
		if (back)
			UNLOCK_DOOR(other_room, obj, rev_dir[door]);
		send_to_char("*Click*\r\n", ch);
		break;

	case SCMD_PICK:
		UNLOCK_DOOR(IN_ROOM(ch), obj, door);
		if (back)
			UNLOCK_DOOR(other_room, obj, rev_dir[door]);
		send_to_char("The lock quickly yields to your skills.\r\n", ch);
		strcpy(buf, "$n skillfully picks the lock on ");
		break;

	case SCMD_KNOCK:
		if (DOOR_IS_OPEN(ch, obj, door))
			send_to_char("But it isn't closed...\r\n", ch);
		else if (back)
			knock_door(ch, obj, door, back, arg);
		else
			send_to_char("This isn't a 2-way door!\r\n", ch);
		return;

	case SCMD_SAYTHROUGH:
		if (back)
			saythrough_door(ch, obj, door, back, arg);
		else
			send_to_char("This isn't a 2-way door!\r\n", ch);
		return;
	}

	/* Notify the room */
	sprintf(buf + strlen(buf), "%s%s.", obj ? "" : "the ", obj ? "$p" :
				EXIT(ch, door)->keyword ? "$F" : "door");
	if (!obj || IN_ROOM(obj) != NOWHERE)
		act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

	/* Notify the other room */
	if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE)) {
		sprintf(buf, "The %s is %s%s from the other side.",
					back->keyword ? fname(back->keyword) : "door", cmd_door[scmd],
					scmd == SCMD_CLOSE ? "d" : "ed");
		if (world[EXIT(ch, door)->to_room].people) {
			act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_ROOM);
			act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_CHAR);
		}
	}
}


int	ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd)
{
	int percent;

	percent = number(1, 101);

	if (scmd == SCMD_PICK) {
		if (keynum == NOTHING)
			send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
		else if (pickproof)
			send_to_char("It resists your attempts to pick it.\r\n", ch);
		else if (percent > (GET_SKILL(ch, SKILL_PICK_LOCK)/100))
			send_to_char("You failed to pick the lock.\r\n", ch);
		else {
			GET_SKILL(ch, SKILL_PICK_LOCK) = skill_gain(ch, SKILL_PICK_LOCK, 101 - percent);
			return (1);
		}
		return (0);
	}
	return (1);
}

void saythrough_door(struct char_data *ch, struct obj_data *obj, int door, struct room_direction_data *back, char *arg)
{
	if (!*arg) {
		sprintf(buf, "Say what through %s%s?\r\n", ((obj) ? "" : "the "), ((obj) ? 
			(EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "door") : "door"));
		send_to_char(buf, ch);
		return;
	}
	/* Let the people in ch's room hear it */
	sprintf(buf, "&cYou say through %s%s,&n\r\n     &c'&C%s&c'&n", ((obj) ? "" : "the "), (obj) ? "$p" :
					(EXIT(ch, door)->keyword ? "$F" : "door"), arg);
	if (!(obj) || (IN_ROOM(obj) != NOWHERE))
		act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_CHAR);
	sprintf(buf, "&c$n says through %s%s,&n\r\n     &c'&C%s&c'&n", ((obj) ? "" : "the "), (obj) ? "$p" :
					(EXIT(ch, door)->keyword ? "$F" : "door"), arg);
	if (!(obj) || (IN_ROOM(obj) != NOWHERE))
		act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

	/* Let the people in the other room hear it */
	sprintf(buf, "&cSomeone says through the %s,&n\r\n     &c'&C%s&c'&n",
		 (back->keyword ? fname(back->keyword) : "door"), arg);
	if (world[EXIT(ch, door)->to_room].people) {
		act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_ROOM);
		act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_CHAR);
	}
}

void knock_door(struct char_data *ch, struct obj_data *obj, int door, struct room_direction_data *back, char *arg)
{
	/* This room */
	sprintf(buf, "You %s%sknock on %s%s.", ((*arg) ? arg : ""), ((*arg) ? " ":""), ((obj) ? "" : "the "), 
		(obj) ? "$p" : (EXIT(ch, door)->keyword ? "$F" : "door"));
	if (!(obj) || (IN_ROOM(obj) != NOWHERE))
		act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_CHAR);

	sprintf(buf, "$n %s%sknocks on %s%s.", ((*arg) ? arg : ""), ((*arg) ? " ":""), ((obj) ? "" : "the "), 
		(obj) ? "$p" : (EXIT(ch, door)->keyword ? "$F" : "door"));
	if (!(obj) || (IN_ROOM(obj) != NOWHERE))
		act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

	/* Other room */
	sprintf(buf, "Someone %s%sknocks on the %s.", ((*arg) ? arg : ""), ((*arg) ? " ":""),
		 (back->keyword ? fname(back->keyword) : "door"));
	if (world[EXIT(ch, door)->to_room].people) {
		act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_ROOM);
		act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_CHAR);
	}
}

ACMD(do_gen_door)
{
	int door = -1;
	obj_vnum keynum;
	char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
	struct obj_data *obj = NULL;
	struct char_data *victim = NULL;

	skip_spaces(&argument);
	if (!*argument) {
		sprintf(buf, "%s what?\r\n", cmd_door[subcmd]);
		send_to_char(CAP(buf), ch);
		return;
	}
	half_chop(argument, type, argument);
	half_chop(argument, dir, argument);

	if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
		door = find_door(ch, type, dir, cmd_door[subcmd]);

	if (((obj) || (door >= 0)) && !(subcmd == SCMD_KNOCK || subcmd == SCMD_SAYTHROUGH)) {
		keynum = DOOR_KEY(ch, obj, door);
		if (!(DOOR_IS_OPENABLE(ch, obj, door)))
			act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
		else if (!DOOR_IS_OPEN(ch, obj, door) &&
						 IS_SET(flags_door[subcmd], NEED_OPEN))
			send_to_char("But it's already closed!\r\n", ch);
		else if (!DOOR_IS_CLOSED(ch, obj, door) &&
						 IS_SET(flags_door[subcmd], NEED_CLOSED))
			send_to_char("But it's currently open!\r\n", ch);
		else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
						 IS_SET(flags_door[subcmd], NEED_LOCKED))
			send_to_char("Oh.. it wasn't locked, after all..\r\n", ch);
		else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
						 IS_SET(flags_door[subcmd], NEED_UNLOCKED))
			send_to_char("It seems to be locked.\r\n", ch);
		else if (!has_key(ch, keynum) && !IS_GOD(ch) &&
						 ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
			send_to_char("You don't seem to have the proper key.\r\n", ch);
		else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), subcmd))
			do_doorcmd(ch, obj, door, subcmd, NULL);
	} else if (((obj) || (door >= 0) )) /* && subcmd == SCMD_KNOCK | SCMD_SAYTHROUGH */
		do_doorcmd(ch, obj, door, subcmd, argument);

	return;
}



ACMD(do_enter)
{
	struct obj_data *portal;
	int door, rroom = 0;

	one_argument(argument, buf);

	if (*buf) {  /* an argument was supplied,   *
								* search for door keyword     */

		/* Object Portal Code with REGIO checking  *
		 * by Artovil (tb@sbbs.se), Arcane Realms, *
		 * arcane-realms.dyndns.org:3011           */

		/* first, check to see if there's an object with ITEM_PORTAL flag */
		if ((portal = get_obj_in_list_vis(ch, buf, NULL, world[IN_ROOM(ch)].contents)) && (IS_OBJ_PORTAL(portal)))        {
			/* crude sanity check */
			if (portal->obj_flags.value[1] <= 0 ||
					portal->obj_flags.value[1] >= 32000) {
				send_to_char("The portal leads nowhere.\r\n", ch);
				return;
			}
			/* Then check to see if the room actually exists */
			rroom = real_room(portal->obj_flags.value[1]);
			if (rroom <= 0) {
				sprintf(buf, "&RThere seems to be something wrong with %s,\r\n  maybe you should leave it alone...&n\r\n", portal->short_description);
				send_to_char(buf, ch);
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Portal #%d (%s) leads to non-existant room #%d.", GET_OBJ_VNUM(portal), portal->short_description, portal->obj_flags.value[1]);
				return;
			}
			/* check to see if the player is restricted from entry */
			if ( ( (GET_OBJ_PERM(portal) == AFF_SEE_FAERIE && IS_FAE(ch))				||
						 (GET_OBJ_PERM(portal) == AFF_SEE_INFERNAL && IS_DEMONIC(ch))	||
						 (GET_OBJ_PERM(portal) == AFF_SEE_DIVINE && IS_DIVINE(ch))		||
						 (GET_OBJ_PERM(portal) == 0) ) || IS_GRGOD(ch) ) {
				act("&M$n enters $p, and vanishes!&n", TRUE, ch, portal, 0, TO_ROOM);
				act("&MYou enter $p.&n\r\n", FALSE, ch, portal, 0, TO_CHAR);
				char_from_room(ch);  
				char_to_room(ch, real_room(portal->obj_flags.value[1]));
				look_at_room(ch, 0);
				act("&M$n enters the room through a portal.&n", TRUE, ch, 0, 0, TO_ROOM);
				return;
			} else {
				send_to_char("You do not seem to be able to enter that portal.\r\n", ch);
				return;
			}
		}

		for (door = 0; door < NUM_OF_DIRS; door++)
			if (EXIT(ch, door))
				if (EXIT(ch, door)->keyword)
					if (!str_cmp(EXIT(ch, door)->keyword, buf)) {
						perform_move(ch, door, 1);
						return;
					}
		sprintf(buf2, "There is no %s here.\r\n", buf);
		send_to_char(buf2, ch);
	} else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
		send_to_char("You are already indoors.\r\n", ch);
	else {
		/* try to locate an entrance */
		for (door = 0; door < NUM_OF_DIRS; door++)
			if (EXIT(ch, door))
				if (EXIT(ch, door)->to_room != NOWHERE)
					if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
							ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
						perform_move(ch, door, 1);
						return;
					}
		send_to_char("You can't seem to find anything to enter.\r\n", ch);
	}
}


ACMD(do_leave)
{
	int door;

	if (OUTSIDE(ch))
		send_to_char("You are outside.. where do you want to go?\r\n", ch);
	else {
		for (door = 0; door < NUM_OF_DIRS; door++)
			if (EXIT(ch, door))
				if (EXIT(ch, door)->to_room != NOWHERE)
					if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
						!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
						perform_move(ch, door, 1);
						return;
					}
		send_to_char("I see no obvious exits to the outside.\r\n", ch);
	}
}


ACMD(do_stand)
{
	switch (GET_POS(ch)) {
	case POS_STANDING:
		send_to_char("You are already standing.\r\n", ch);
		break;
	case POS_SITTING:
		send_to_char("You stand up.\r\n", ch);
		act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
		/* Will be sitting after a successful bash and may still be fighting. */
		GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
		break;
	case POS_RESTING:
		send_to_char("You stop resting, and stand up.\r\n", ch);
		act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_STANDING;
		break;
	case POS_SLEEPING:
		send_to_char("You have to wake up first!\r\n", ch);
		break;
	case POS_FIGHTING:
		send_to_char("Do you not consider fighting as standing?\r\n", ch);
		break;
	case POS_DODGE:
		if (FIGHTING(ch)) {
			send_to_char("You stop dodging attacks and take them full on.\r\n", ch);
			act("$n stops dodging attacks.\r\n", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_FIGHTING;
		} else {
			send_to_char("You stop dodging things.\r\n", ch);
			act("$n stops dodging things.\r\n", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_STANDING;
		}
		break;
	case POS_DEFEND:
		if (FIGHTING(ch)) {
			send_to_char("You stop defending yourself and fight back.\r\n", ch);
			act("$n stops defending $mself and fights back.\r\n", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_FIGHTING;
		} else {
			send_to_char("You stop defending yourself.\r\n", ch);
			act("$n stops defending $mself.\r\n", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_STANDING;
		}
		break;
	case POS_WATCHING:
		send_to_char("You stand down from watch.\r\n", ch);
		act("$n stands down from watch.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_STANDING;
		break;
	case POS_MEDITATING:
		send_to_char("You break your concentration, and stand.\r\n", ch);
		act("$n breaks $s concentration, and stands up.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_STANDING;
		break;
	default:
		send_to_char("You stop floating around, and put your feet on the ground.\r\n", ch);
		act("$n stops floating around, and puts $s feet on the ground.",
				TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_STANDING;
		break;
	}
}


ACMD(do_sit)
{
	switch (GET_POS(ch)) {
	case POS_STANDING:
		send_to_char("You sit down.\r\n", ch);
		act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
		break;
	case POS_SITTING:
		send_to_char("You're sitting already.\r\n", ch);
		break;
	case POS_RESTING:
		send_to_char("You stop resting, and sit up.\r\n", ch);
		act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
		break;
	case POS_SLEEPING:
		send_to_char("You have to wake up first.\r\n", ch);
		break;
	case POS_FIGHTING:
		send_to_char("Sit down while fighting? Are you MAD?\r\n", ch);
		break;
	case POS_DODGE:
		send_to_char("You can't expect to dodge sitting!\r\n", ch);
		break;
	case POS_DEFEND:
		send_to_char("You can't sit and defend yourself!\r\n", ch);
		break;
	case POS_WATCHING:
		send_to_char("You stand down from watch, and take a seat.\r\n", ch);
		act("$n stands down from watch, and sits down.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
		break;
	case POS_MEDITATING:
		send_to_char("You break your concentration.\r\n", ch);
		act("$n breaks $s concentration.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
		break;
	default:
		send_to_char("You stop floating around, and sit down.\r\n", ch);
		act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
		break;
	}
}


ACMD(do_rest)
{
	switch (GET_POS(ch)) {
	case POS_STANDING:
		send_to_char("You sit down and rest your tired bones.\r\n", ch);
		act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_RESTING;
		break;
	case POS_SITTING:
		send_to_char("You rest your tired bones.\r\n", ch);
		act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_RESTING;
		break;
	case POS_RESTING:
		send_to_char("You are already resting.\r\n", ch);
		break;
	case POS_SLEEPING:
		send_to_char("You have to wake up first.\r\n", ch);
		break;
	case POS_FIGHTING:
		send_to_char("Rest while fighting?  Are you MAD?\r\n", ch);
		break;
	case POS_DODGE:
		send_to_char("You can't expect to dodge resting!\r\n", ch);
		break;
	case POS_DEFEND:
		send_to_char("You can't rest and defend yourself!\r\n", ch);
		break;
	case POS_WATCHING:
		send_to_char("You stand down from watch, and rest yourself.\r\n", ch);
		act("$n stands down from watch, and sits down to rest.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_RESTING;
		break;
	case POS_MEDITATING:
		send_to_char("You break your concentration, and simply rest.\r\n", ch);
		act("$n breaks $s concentration, and simply rests.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
		break;
	default:
		send_to_char("You stop floating around, and stop to rest your tired bones.\r\n", ch);
		act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_RESTING;
		break;
	}
}


ACMD(do_sleep)
{
	switch (GET_POS(ch)) {
	case POS_STANDING:
	case POS_SITTING:
	case POS_RESTING:
		send_to_char("You go to sleep.\r\n", ch);
		act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SLEEPING;
		break;
	case POS_SLEEPING:
		send_to_char("You are already sound asleep.\r\n", ch);
		break;
	case POS_FIGHTING:
		send_to_char("Sleep while fighting?  Are you MAD?\r\n", ch);
		break;
	case POS_DODGE:
		send_to_char("You can't expect to dodge while sleeping!\r\n", ch);
		break;
	case POS_DEFEND:
		send_to_char("You can't sleep and defend yourself!\r\n", ch);
		break;
	case POS_WATCHING:
		send_to_char("You stand down from watch, and fall asleep.\r\n", ch);
		act("$n stands down from watch, and falls asleep.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SLEEPING;
		break;
	case POS_MEDITATING:
		send_to_char("You break your concentration, and fall asleep.\r\n", ch);
		act("$n breaks $s concentration, and falls asleep.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
		break;
	default:
		send_to_char("You stop floating around, and lie down to sleep.\r\n", ch);
		act("$n stops floating around, and lie down to sleep.",
				TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SLEEPING;
		break;
	}
}


ACMD(do_wake)
{
	struct char_data *vict;
	int self = 0;

	one_argument(argument, arg);
	if (*arg) {
		if (GET_POS(ch) == POS_SLEEPING)
			send_to_char("Maybe you should wake yourself up first.\r\n", ch);
		else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM, 0)) == NULL)
			send_to_char(NOPERSON, ch);
		else if (vict == ch)
			self = 1;
		else if (AWAKE(vict))
			act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
		else if (AFF_FLAGGED(vict, AFF_SLEEP))
			act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
		else if (GET_POS(vict) < POS_SLEEPING)
			act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
		else {
			act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
			act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
			GET_POS(vict) = POS_SITTING;
		}
		if (!self)
			return;
	}
	if (AFF_FLAGGED(ch, AFF_SLEEP))
		send_to_char("You can't wake up!\r\n", ch);
	else if (GET_POS(ch) > POS_SLEEPING)
		send_to_char("You are already awake...\r\n", ch);
	else {
		send_to_char("You awaken, and sit up.\r\n", ch);
		act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
	}
}

ACMD(do_follow)
{
	struct char_data *leader;

	one_argument(argument, buf);

	if (*buf) {
		if (!(leader = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM, 0))) {
			send_to_char(NOPERSON, ch);
			return;
		}
	} else {
		send_to_char("Whom do you wish to follow?\r\n", ch);
		return;
	}

	if (ch->master == leader) {
		act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
		return;
	}
	if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) {
		act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
	} else {                        /* Not Charmed follow person */
		if (leader == ch) {
			if (!ch->master) {
				send_to_char("You are already following yourself.\r\n", ch);
				return;
			}
			stop_follower(ch);
		} else {
			if (circle_follow(ch, leader)) {
				send_to_char("Sorry, but following in loops is not allowed.\r\n", ch);
				return;
			}
			if (ch->master)
				stop_follower(ch);
			REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
			add_follower(ch, leader);
		}
	}
}
