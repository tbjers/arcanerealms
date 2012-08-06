/* ************************************************************************
*		File: act.item.c                                    Part of CircleMUD *
*	 Usage: object handling routines -- get/drop and container handling     *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: act.item.c,v 1.48 2004/05/08 03:36:22 cheron Exp $ */

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
#include "dg_scripts.h"
#include "assemblies.h"
#include "quest.h"
#include "house.h"

/* extern variables */
extern room_rnum donation_room_1;
#if	0
extern room_rnum donation_room_2;  /* uncomment if needed! */
extern room_rnum donation_room_3;  /* uncomment if needed! */
#endif
extern struct obj_data *obj_proto;
extern struct room_data *world;
extern obj_rnum top_of_objt;
extern mob_rnum top_of_mobt;
extern char *combat_skills[];

/* local functions */
int	can_take_obj(struct char_data *ch, struct obj_data *obj);
void get_check_money(struct char_data *ch, struct obj_data *obj);
int	perform_get_from_room(struct char_data *ch, struct obj_data *obj);
void get_from_room(struct char_data *ch, char *arg, int amount);
void perform_give_gold(struct char_data *ch, struct char_data *vict, int amount);
void perform_give(struct char_data *ch, struct char_data *vict, struct obj_data *obj);
int	perform_drop(struct char_data *ch, struct obj_data *obj, byte mode, const char *sname, room_rnum RDR);
void perform_drop_gold(struct char_data *ch, int amount, byte mode, room_rnum RDR);
struct char_data *give_find_vict(struct char_data *ch, char *arg);
void weight_change_object(struct obj_data *obj, int weight);
int perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont);
void name_from_drinkcon(struct obj_data *obj);
void get_from_container(struct char_data *ch, struct obj_data *cont, char *arg, int mode, int amount, int subcmd);
void name_to_drinkcon(struct obj_data *obj, int type);
void wear_message(struct char_data *ch, struct obj_data *obj, int where);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where, int show);
int	find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
int perform_get_from_container(struct char_data *ch, struct obj_data *obj, struct obj_data *cont, int mode);
void perform_get_from_sheath(struct char_data *ch, struct obj_data *obj);
void perform_remove(struct char_data *ch, int pos);
int	perform_sac(struct char_data *ch, struct obj_data *obj);
void perform_sac_gold(struct char_data *ch, int amount);
ACMD(do_compare);
ACMD(do_assemble);
ACMD(do_remove);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);
ACMD(do_sac);
ACMD(do_dwield);


EVENTFUNC(assembly_event)
{
	struct assembly_event_obj *aeo = (struct assembly_event_obj *) event_obj;
	struct char_data *ch;
	int method;
	int i = 0;
	struct obj_data *pObject = NULL;

	ch = aeo->ch;					/* pointer to ch			*/
	method = aeo->method;	/* skill type					*/
	--aeo->time;					/* subtract from time	*/

	if (aeo->time <= 0) {
		/*
		 * The event time has expired, now we create the object
		 * and do a skill check along with sending out completion
		 * messages to char and room.
		 */
		switch (method) {
		case ASSM_ASSEMBLE:
		case ASSM_BAKE:
		case ASSM_BREW:
		case ASSM_CRAFT:
		case ASSM_FLETCH:
		case ASSM_KNIT:
		case ASSM_MAKE:
		case ASSM_MIX:
		case ASSM_THATCH:
		case ASSM_WEAVE:
		case ASSM_COOK:
		case ASSM_SEW:
		case ASSM_BUTCHER:
		case ASSM_TAN:
		case ASSM_SMELT:
			if ((pObject = read_object(aeo->lVnum, VIRTUAL)) == NULL ) {
				sprintf(buf, "You can't %s that.\r\n", AssemblyTypeNames[method][ASSM_TYPE_TO]);
				send_to_char(buf, ch);
			} else {
				if (!skill_check(ch, aeo->skill, (assemblyGetSkillReq(aeo->lVnum, ch) / 100) - 99)) {
					sprintf(buf, "You fail to %s %s.\r\n", AssemblyTypeNames[method][ASSM_TYPE_TO], obj_proto[real_object(aeo->lVnum)].short_description);
					send_to_char(buf, ch);
					sprintf(buf, "$n attempts to %s $p, but fails.", AssemblyTypeNames[method][ASSM_TYPE_TO]);
					act(buf, FALSE, ch, pObject, NULL, TO_ROOM);
					assemblyLoadByproducts(aeo->lVnum, ch, FALSE);
				} else {
					/* Now give the object(s) to the character. */
					for (i = 0; i < aeo->lProduces; i++) {
						/*
						 * we already created the first object above,
						 * need to create one for each further iteration.
						 */
						if (i > 0)
							pObject = read_object(aeo->lVnum, VIRTUAL);
						/*
						 * If we have component colors or a resource type, we
						 * assign the same color to the product(s) created.
						 * Also set the object to ITEM_UNIQUE_SAVE and to have
						 * the ITEM_COLORIZE flag if a color is present.
						 */
						if (aeo->iColor > 0) {
							GET_OBJ_COLOR(pObject) = aeo->iColor;
							SET_BIT(GET_OBJ_EXTRA(pObject), ITEM_COLORIZE);
							SET_BIT(GET_OBJ_EXTRA(pObject), ITEM_UNIQUE_SAVE);
						}
						if (aeo->iResource > 0) {
							GET_OBJ_RESOURCE(pObject) = aeo->iResource;
							SET_BIT(GET_OBJ_EXTRA(pObject), ITEM_UNIQUE_SAVE);
						}
						if (can_take_obj(ch, pObject))
							obj_to_char(pObject, ch);
						else
							obj_to_room(pObject, IN_ROOM(ch));
					}
					/* Tell the character they made something. */
					sprintf(buf, "You %s $p.", AssemblyTypeNames[method][ASSM_TYPE_TO]);
					act(buf, FALSE, ch, pObject, NULL, TO_CHAR);
					/* Tell the room the character made something. */
					sprintf(buf, "$n %s $p.", AssemblyTypeNames[method][ASSM_TYPE_DOES]);
					act(buf, FALSE, ch, pObject, NULL, TO_ROOM);
					assemblyLoadByproducts(aeo->lVnum, ch, TRUE);
				}
			}
			free(event_obj);
			GET_PLAYER_EVENT(ch, EVENT_SKILL) = NULL;
			return (0);

		default:
			extended_mudlog(NRM, SYSL_GENERAL, TRUE, "Invalid assembly method #%d passed to %s, %s:%d.", method, __FILE__, __FUNCTION__, __LINE__);
			free(event_obj);
			GET_PLAYER_EVENT(ch, EVENT_SKILL) = NULL;
			return (0);
		}
	}

	return (6 RL_SEC);

}


ACMD(do_assemble)
{
	struct assembly_event_obj *aeo;
	long lVnum = NOTHING, aVnum = NOTHING;
	long lProduces = 1;
	int skill = -1, iColor = 0, iResource = -1;
	
	skip_spaces(&argument);

	if (GET_PLAYER_EVENT(ch, EVENT_SKILL)) {
		/*
		 * The player already has an action event, and thus, has to wait
		 * until the event has been completed or cancelled.
		 */
		send_to_char("You cannot use another skill right now.\r\n", ch);
		return;
	} else {
		if (!*argument || ((aVnum = atoi(argument)) <= 0 && !is_abbrev(argument, "last"))) {
			assemblyListGroupToChar(ch, subcmd);
			return;
		}
		if (is_abbrev(argument, "last") && !GET_LASTCRAFTING(ch)) {
			send_to_charf(ch, "You have not %s anything yet.\r\n", AssemblyTypeNames[subcmd][ASSM_TYPE_DID]);
			return;
		} else if ((lVnum = assemblyFindAssembly(aVnum)) < 0 && !is_abbrev(argument, "last")) {
			send_to_charf(ch, "That item cannot be %s.\r\n", AssemblyTypeNames[subcmd][ASSM_TYPE_DID]);
			return;
		}
		if (GET_LASTCRAFTING(ch) && is_abbrev(argument, "last"))
			lVnum = GET_LASTCRAFTING(ch);
		else
			GET_LASTCRAFTING(ch) = lVnum;
		if (assemblyGetType(lVnum) != subcmd) {
			send_to_charf(ch, "You can't %s that.\r\n", AssemblyTypeNames[subcmd][ASSM_TYPE_TO]);
			return;
		} else if ((skill = assemblyCheckSkill(lVnum, ch)) < 0) {
			send_to_charf(ch, "You do not know how to %s that.\r\n", AssemblyTypeNames[subcmd][ASSM_TYPE_TO]);
			return;
		} else if (!(lProduces = assemblyGetProduces(lVnum))) {
			lProduces = 1;
		}

		/* Check the components for colors. */
		iColor = assemblyCheckComponents(lVnum, ch, COMPONENT_COLOR);

		/* Check the components for resource type. */
		iResource = assemblyCheckComponents(lVnum, ch, COMPONENT_RESOURCE);

		/* Finally, check and extract components if ok. */
		if (!assemblyCheckComponents(lVnum, ch, COMPONENT_ITEMS)) {
			send_to_charf(ch, "\r\n&RYou have not got all the parts to %s that&n.\r\n\r\n", AssemblyTypeNames[subcmd][ASSM_TYPE_TO]);
			if (!assemblyGetHidden(lVnum))
				assemblyListItemToChar(ch, lVnum);
			return;
		}

		CREATE(aeo, struct assembly_event_obj, 1);
		aeo->ch = ch;																				/* pointer to ch									*/
		aeo->method = subcmd;																/* crafting method								*/
		aeo->time = assemblyGetTime(lVnum);									/* event time * 6 RL_SEC					*/
		aeo->skill = skill;																	/* the skill we are using					*/
		aeo->lVnum = lVnum;																	/* the vnum of the assembly				*/
		aeo->lProduces = lProduces;													/* number of items to produce			*/
		aeo->iColor = iColor;																/* color of item									*/
		aeo->iResource = iResource;													/* resource used in item					*/
		aeo->cmd = CMD_NAME;																/* command name										*/
		GET_PLAYER_EVENT(ch, EVENT_SKILL) = event_create(assembly_event, aeo, 6 RL_SEC);

		/* Tell the character they are making something. */
		sprintf(buf, "You start to %s...", CMD_NAME);
		act(buf, FALSE, ch, NULL, NULL, TO_CHAR);
		/* Tell the room the character is making something. */
		sprintf(buf, "$n starts to %s something...", CMD_NAME);
		act(buf, FALSE, ch, NULL, NULL, TO_ROOM);

	}
}


ACMD(do_assminfo)
{
	long lVnum = NOTHING, aVnum = NOTHING;

	if (!*argument || (aVnum = atoi(argument)) <= 0) {
		send_to_char("Usage: components <item number>\r\n", ch);
		return;
	}

	if ((lVnum = assemblyFindAssembly(aVnum)) < 0) {
		send_to_char("No such item.\r\n", ch);
		return;
	}

	assemblyListItemToChar(ch, lVnum);

}


void perform_put_message(struct char_data *ch, struct obj_data *obj, struct obj_data *cont, int amount)
{
	char *number = get_buffer(64);

	sprintf(number, "(%d) ", amount);
	sprintf(buf, "You put %s$p%s.", (amount > 1) ? number : "", cont ? " in $P" : "");
	act(buf, FALSE, ch, obj, cont, TO_CHAR);
	sprintf(buf, "$n puts %s$p%s.", (amount > 1) ? number : "", cont ? " in $P" : "");
	act(buf, TRUE, ch, obj, cont, TO_ROOM);

	release_buffer(number);
}


int perform_put(struct char_data *ch, struct obj_data *obj,
											struct obj_data *cont)
{
	int eq_pos = 0;

	if (!drop_otrigger(obj, ch))
		return (0);

	if (!obj) /* object might be extracted by drop_otrigger */
		return (0);
	
	switch (GET_OBJ_TYPE(cont)) {
		case ITEM_CONTAINER: 
			if (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0)) {
				act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
				return (0);
			}
			obj_from_char(obj);
			obj_to_obj(obj, cont);
			return (1);
		case ITEM_SHEATH:
			if (cont->contains) {
				act("$P already contains a weapon.", FALSE, ch, obj, cont, TO_CHAR);
				return (0);
			}
			if (GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
				act("$p is not a weapon.", FALSE, ch, obj, cont, TO_CHAR);
				return (0);
			} else if (GET_OBJ_VAL(cont, 0) != GET_OBJ_VAL(obj, 0)) {
				act("$P is not a sheath for $p.", FALSE, ch, obj, cont, TO_CHAR);
				return (0);
			}
			for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++) {
				if (HAS_BODY(ch, eq_pos) && GET_EQ(ch, eq_pos) &&
							(!strcmp(obj->name, GET_EQ(ch, eq_pos)->name)) &&
							CAN_SEE_OBJ(ch, GET_EQ(ch, eq_pos))) {
					unequip_char(ch, eq_pos);
					obj_to_char(obj, ch);
					break;
				}
			}
			obj_from_char(obj);
			obj_to_obj(obj, cont);

			act("$n sheathes $p in $P.", TRUE, ch, obj, cont, TO_ROOM);
			act("You sheathe $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
			return (0);
	}

	return (0);
}


/* The following put modes are supported by the code below:

				1) put <object> <container>
				2) put all.<object> <container>
				3) put all <container>

				<container> must be in inventory, worn or on ground.
				all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	struct obj_data *obj, *next_obj, *cont;
	struct char_data *tmp_char;
	int obj_dotmode, cont_dotmode, found = 0, howmany = 1, total = 0;
	char *theobj, *thecont;

	argument = two_arguments(argument, arg1, arg2);
	one_argument(argument, arg3);

	if (*arg3 && is_number(arg1)) {
		howmany = atoi(arg1);
		theobj = arg2;
		thecont = arg3;
	} else {
		theobj = arg1;
		thecont = arg2;
	}
	obj_dotmode = find_all_dots(theobj);
	cont_dotmode = find_all_dots(thecont);

	if (!*theobj) 
		send_to_charf(ch, "%s what in what?\r\n", (subcmd == SCMD_SHEATHE) ? "Sheathe" : "Put");
	else if (cont_dotmode != FIND_INDIV)
		send_to_char("You can only put things into one container at a time.\r\n", ch);
	else if (subcmd == SCMD_SHEATHE && obj_dotmode != FIND_INDIV)
		send_to_char("You can only sheathe one weapon at a time.\r\n", ch);
	else if (!*thecont)
		send_to_charf(ch, "What do you want to %s %s in?\r\n", (subcmd == SCMD_SHEATHE) ? "sheathe" : "put", ((obj_dotmode == FIND_INDIV) ? "it" : "them"));
	else {
		if (subcmd == SCMD_SHEATHE)
			generic_find(thecont, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tmp_char, &cont);
		else
			generic_find(thecont, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &cont);
		if (!cont)
			send_to_charf(ch, "You don't see %s %s here.\r\n", AN(thecont), thecont);
		else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER && GET_OBJ_TYPE(cont) != ITEM_SHEATH)
			act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
		else if (subcmd == SCMD_SHEATHE && GET_OBJ_TYPE(cont) != ITEM_SHEATH)	{
			act("$p is not a sheath.", FALSE, ch, cont, 0, TO_CHAR);
		}
		else if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
			send_to_char("You'd better open it first!\r\n", ch);
		else {
			if (obj_dotmode == FIND_INDIV) {        /* put <obj> <container> */
				if (subcmd == SCMD_SHEATHE) 
					generic_find(theobj, FIND_OBJ_EQUIP, ch, &tmp_char, &obj);
				else
					obj = get_obj_in_list_vis(ch, theobj, NULL, ch->carrying);
				if (!obj)
					send_to_charf(ch, "You aren't %s %s %s.\r\n", (subcmd == SCMD_SHEATHE) ? "wielding" : "carrying", AN(theobj), theobj);
				else if (obj == cont)
					send_to_char("You attempt to fold it into itself, but fail.\r\n", ch);
				else {
					while(obj && howmany--) {
						next_obj = obj->next_content;
						total += perform_put(ch, obj, cont);
						if ((!next_obj || (strcmp(next_obj->name, obj->name)) || howmany == 0) && total != 0) {
							perform_put_message(ch, obj, cont, total);
							total = 0;
						}
						obj = get_obj_in_list_vis(ch, theobj, NULL, next_obj);
					}
				}
			} else {
				for (obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->next_content;
					if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
							(obj_dotmode == FIND_ALL || isname(theobj, obj->name))) {
						found = 1;
						total += perform_put(ch, obj, cont);
						if ((!next_obj || (strcmp(next_obj->name, obj->name)) || howmany == 0) && total != 0) {
							perform_put_message(ch, obj, cont, total);
							total = 0;
						}
					}
				}
				if (!found) {
					if (obj_dotmode == FIND_ALL)
						send_to_char("You don't seem to have anything to put in it.\r\n", ch);
					else {
						sprintf(buf, "You don't seem to have any %ss.\r\n", theobj);
						send_to_char(buf, ch);
					}
				}
			}
		}
	}
}


void perform_get_message(struct char_data *ch, struct obj_data *obj, struct obj_data *cont, int amount)
{
	char *number = get_buffer(64);

	sprintf(number, " (%d)", amount);
	sprintf(buf, "You get %s%s%s.", (amount > 1) ? number : "", (GET_OBJ_TYPE(obj) == ITEM_MONEY) ? "some gold" : "$p", cont ? " from $P" : "");
	act(buf, FALSE, ch, obj, cont, TO_CHAR);
	sprintf(buf, "$n gets %s%s%s.", (amount > 1) ? number : "", (GET_OBJ_TYPE(obj) == ITEM_MONEY) ? "some gold" : "$p", cont ? " from $P" : "");
	act(buf, TRUE, ch, obj, cont, TO_ROOM);

	release_buffer(number);
}


int	can_take_obj(struct char_data *ch, struct obj_data *obj)
{
	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
		act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	} else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
		act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	} else if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE))) {
		act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	} else if (!house_can_take(ch, GET_ROOM_VNUM(IN_ROOM(ch)))) {
		act("$p: you cannot take objects from houses!", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}
	return (1);
}


void get_check_money(struct char_data *ch, struct obj_data *obj)
{
	int value = GET_OBJ_VAL(obj, 0);

	if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
		return;

	GET_GOLD(ch) += value;

	if (value == 1)
		send_to_char("There was 1 coin.\r\n", ch);
	else {
		sprintf(buf, "There were %d coins.\r\n", value);
		send_to_char(buf, ch);
	}
}


int perform_get_from_container(struct char_data *ch, struct obj_data *obj,
																		 struct obj_data *cont, int mode)
{
	if (mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
		if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
			act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
		else if (get_otrigger(obj, ch) && (obj)) { /* obj may be purged */
			obj_from_obj(obj);
			obj_to_char(obj, ch);
			get_check_money(ch, obj);
			return (1);
		}
	}
	return (0);
}


void get_from_container(struct char_data *ch, struct obj_data *cont,
														 char *arg, int mode, int howmany, int subcmd)
{
	struct obj_data *obj, *next_obj;
	int obj_dotmode, found = 0, total = 0;

	obj_dotmode = find_all_dots(arg);

	if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
		act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
	else if (obj_dotmode == FIND_INDIV) {
		if (!(obj = get_obj_in_list_vis(ch, arg, NULL, cont->contains))) {
			sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg), arg);
			act(buf, FALSE, ch, cont, 0, TO_CHAR);
		} else {
			struct obj_data *obj_next;
			while (obj && howmany--) {
				obj_next = obj->next_content;
				if (subcmd == SCMD_DRAW)
					perform_get_from_sheath(ch, obj);
				else {
					total += perform_get_from_container(ch, obj, cont, mode);
					if ((!obj_next || (strcmp(obj_next->name, obj->name)) || howmany == 0) && total != 0) {
						perform_get_message(ch, obj, cont, total);
						total = 0;
					}
				}
				obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
			}
		}
	} else {
		if (obj_dotmode == FIND_ALLDOT && !*arg) {
			send_to_char("Get all of what?\r\n", ch);
			return;
		}
		for (obj = cont->contains; obj; obj = next_obj) {
			next_obj = obj->next_content;
			if (CAN_SEE_OBJ(ch, obj) &&
					(obj_dotmode == FIND_ALL || isname(arg, obj->name))) {
				found = 1;
				if (subcmd == SCMD_DRAW)
					perform_get_from_sheath(ch, obj);
				else {
					total += perform_get_from_container(ch, obj, cont, mode);
					if ((!next_obj || (strcmp(next_obj->name, obj->name))) && total != 0) {
						perform_get_message(ch, obj, cont, total);
						total = 0;
					}
				}
			}
		}
		if (!found) {
			if (obj_dotmode == FIND_ALL)
				act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
			else {
				sprintf(buf, "You can't seem to find any %ss in $p.", arg);
				act(buf, FALSE, ch, cont, 0, TO_CHAR);
			}
		}
	}
}


int	perform_get_from_room(struct char_data *ch, struct obj_data *obj)
{
	if (can_take_obj(ch, obj) &&
			get_otrigger(obj, ch) &&
			(obj)) { /* obj may be purged by get_otrigger */
		obj_from_room(obj);
		obj_to_char(obj, ch);
		get_check_money(ch, obj);
		return (1);
	}
	return (0);
}


void get_from_room(struct char_data *ch, char *arg, int howmany)
{
	struct obj_data *obj, *next_obj;
	int dotmode, found = 0, total = 0;

	dotmode = find_all_dots(arg);

	if (dotmode == FIND_INDIV) {
		if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
			sprintf(buf, "You don't see %s %s here.\r\n", AN(arg), arg);
			send_to_char(buf, ch);
		} else {
			struct obj_data *obj_next;
			while(obj && howmany--) {
				obj_next = obj->next_content;
				total += perform_get_from_room(ch, obj);
				if ((!obj_next || (strcmp(obj_next->name, obj->name)) || howmany == 0) && total != 0) {
					perform_get_message(ch, obj, 0, total);
					total = 0;
				}
				obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
			}
		}
	} else {
		if (dotmode == FIND_ALLDOT && !*arg) {
			send_to_char("Get all of what?\r\n", ch);
			return;
		}
		for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj) {
			next_obj = obj->next_content;
			if (CAN_SEE_OBJ(ch, obj) &&
					(dotmode == FIND_ALL || isname(arg, obj->name))) {
				found = 1;
				total += perform_get_from_room(ch, obj);
			}
			if ((!next_obj || (strcmp(next_obj->name, obj->name))) && total != 0) {
				perform_get_message(ch, obj, 0, total);
				total = 0;
			}
		}
		if (!found) {
			if (dotmode == FIND_ALL)
				send_to_char("There doesn't seem to be anything here.\r\n", ch);
			else {
				sprintf(buf, "You don't see any %ss here.\r\n", arg);
				send_to_char(buf, ch);
			}
		}
	}
}


ACMD(do_get)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];

	int cont_dotmode, found = 0, mode;
	struct obj_data *cont;
	struct char_data *tmp_char;

	argument = two_arguments(argument, arg1, arg2);
	one_argument(argument, arg3);

	if (!*arg1)
		send_to_char("Get what?\r\n", ch);
	else if (!*arg2)
		get_from_room(ch, arg1, 1);
	else if (is_number(arg1) && !*arg3)
		get_from_room(ch, arg2, atoi(arg1));
	else {
		int amount = 1;
		if (is_number(arg1)) {
			amount = atoi(arg1);
			strcpy(arg1, arg2);
			strcpy(arg2, arg3);
		}
		cont_dotmode = find_all_dots(arg2);
		if (cont_dotmode == FIND_INDIV) {
			mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &cont);
			if (!cont) {
				sprintf(buf, "You don't have %s %s.\r\n", AN(arg2), arg2);
				send_to_char(buf, ch);
			} else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER && GET_OBJ_TYPE(cont) != ITEM_SHEATH)
				act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
			else if (subcmd == SCMD_DRAW && GET_OBJ_TYPE(cont) != ITEM_SHEATH)
				act("$p is not a sheath.", FALSE, ch, cont, 0, TO_CHAR);
			else
				get_from_container(ch, cont, arg1, mode, amount, subcmd);
		} else {
			if (cont_dotmode == FIND_ALLDOT && !*arg2) {
				send_to_char("Get from all of what?\r\n", ch);
				return;
			}
			for (cont = ch->carrying; cont; cont = cont->next_content)
				if (CAN_SEE_OBJ(ch, cont) &&
						(cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
					if (subcmd == SCMD_DRAW && GET_OBJ_TYPE(cont) != ITEM_SHEATH) {
						found = 1;
						act("$p is not a sheath.", FALSE, ch, cont, 0, TO_CHAR);
					} else if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER || GET_OBJ_TYPE(cont) == ITEM_SHEATH) {
						found = 1;
						get_from_container(ch, cont, arg1, FIND_OBJ_INV, amount, subcmd);
					} else if (cont_dotmode == FIND_ALLDOT) {
						found = 1;
						act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
					}
				}
			for (cont = world[IN_ROOM(ch)].contents; cont; cont = cont->next_content)
				if (CAN_SEE_OBJ(ch, cont) &&
						(cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
					if (subcmd == SCMD_DRAW && GET_OBJ_TYPE(cont) != ITEM_SHEATH) {
						found = 1;
						act("$p is not a sheath.", FALSE, ch, cont, 0, TO_CHAR);
					} else if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER || GET_OBJ_TYPE(cont) == ITEM_SHEATH) {
						get_from_container(ch, cont, arg1, FIND_OBJ_ROOM, amount, subcmd);
						found = 1;
					} else if (cont_dotmode == FIND_ALLDOT) {
						act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
						found = 1;
					}
				}
			if (!found) {
				if (cont_dotmode == FIND_ALL)
					send_to_charf(ch, "You can't seem to find any %s.\r\n", (subcmd == SCMD_DRAW) ? "sheaths" : "containers");
				else {
					sprintf(buf, "You can't seem to find any %ss here.\r\n", arg2);
					send_to_char(buf, ch);
				}
			}
		}
	}
}


void perform_drop_gold(struct char_data *ch, int amount,
														byte mode, room_rnum RDR)
{
	struct obj_data *obj;

	if (amount <= 0)
		send_to_char("Heh heh heh.. we are jolly funny today, eh?\r\n", ch);
	else if (GET_GOLD(ch) < amount)
		send_to_char("You don't have that many coins!\r\n", ch);
	else {
		if (mode != SCMD_JUNK) {
			WAIT_STATE(ch, PULSE_VIOLENCE);        /* to prevent coin-bombing */
			obj = create_money(amount);
			if (mode == SCMD_DONATE) {
				send_to_char("You throw some gold into the air where it disappears in a puff of smoke!\r\n", ch);
				act("$n throws some gold into the air where it disappears in a puff of smoke!",
						FALSE, ch, 0, 0, TO_ROOM);
				obj_to_room(obj, RDR);
				act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
			} else {
				if (!drop_wtrigger(obj, ch) && (obj)) { /* obj may be purged */
					extract_obj(obj);
					return;
				}
				send_to_char("You drop some gold.\r\n", ch);
				sprintf(buf, "$n drops %s.", money_desc(amount));
				act(buf, TRUE, ch, 0, 0, TO_ROOM);
				obj_to_room(obj, IN_ROOM(ch));
			}
		} else {
			sprintf(buf, "$n drops %s which disappears in a puff of smoke!",
							money_desc(amount));
			act(buf, FALSE, ch, 0, 0, TO_ROOM);
			send_to_char("You drop some gold which disappears in a puff of smoke!\r\n", ch);
		}
		GET_GOLD(ch) -= amount;
	}
}


#define	VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
											"  It vanishes in a puff of smoke!" : "")

void perform_drop_message(struct char_data *ch, struct obj_data *obj, const char *sname, int amount, byte mode)
{
	char *number = get_buffer(64);

	sprintf(number, "(%d) ", amount);
	sprintf(buf, "You %s %s$p.%s", sname, (amount > 1) ? number : "", VANISH(mode));
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	sprintf(buf, "$n %ss %s$p.%s", sname, (amount > 1) ? number : "", VANISH(mode));
	act(buf, TRUE, ch, obj, 0, TO_ROOM);

	release_buffer(number);
}

int	perform_drop(struct char_data *ch, struct obj_data *obj,
										 byte mode, const char *sname, room_rnum RDR)
{
	int value;

	if (!drop_otrigger(obj, ch))
		return 0;

	if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
		return 0;

	if (!obj) /* obj may be purged */
		return 0;

	if (OBJ_FLAGGED(obj, ITEM_NODROP) && !IS_BUILDER(ch)) {
		sprintf(buf, "You can't %s $p, it must be CURSED!", sname);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}
	obj_from_char(obj);

	if ((mode == SCMD_DONATE) && OBJ_FLAGGED(obj, ITEM_NODONATE))
		mode = SCMD_JUNK;

	switch (mode) {
	case SCMD_DROP:
		obj_to_room(obj, IN_ROOM(ch));
		return (1);
	case SCMD_DONATE:
		obj_to_room(obj, RDR);
		act("$p suddenly appears in a puff a smoke!", FALSE, 0, obj, 0, TO_ROOM);
		return (1);
	case SCMD_JUNK:
		value = MAX(1, MIN(200, GET_OBJ_COST(obj) / 16));
		return (value);
	default:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Incorrect argument %d passed to perform_drop.", mode);
		break;
	}

	return (0);
}


ACMD(do_drop)
{
	struct obj_data *obj, *next_obj;
	room_rnum RDR = 0;
	byte mode = SCMD_DROP;
	int dotmode, amount = 0, multi, total = 0, result = 0;
	const char *sname;

	switch (subcmd) {
	case SCMD_JUNK:
		sname = "junk";
		mode = SCMD_JUNK;
		break;
	case SCMD_DONATE:
		sname = "donate";
		mode = SCMD_DONATE;
		switch (number(0, 2)) {
		case 0:
			mode = SCMD_JUNK;
			break;
		case 1:
		case 2:
			RDR = real_room(donation_room_1);
			break;
/*		case 3: RDR = real_room(donation_room_2); break;
			case 4: RDR = real_room(donation_room_3); break;
*/
		}
		if (RDR == NOWHERE) {
			send_to_char("Sorry, you can't donate anything right now.\r\n", ch);
			return;
		}
		break;
	default:
		sname = "drop";
		break;
	}

	argument = one_argument(argument, arg);

	if (!*arg) {
		sprintf(buf, "What do you want to %s?\r\n", sname);
		send_to_char(buf, ch);
		return;
	} else if (is_number(arg)) {
		multi = atoi(arg);
		one_argument(argument, arg);
		if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
			perform_drop_gold(ch, multi, mode, RDR);
		else if (multi <= 0)
			send_to_char("Yeah, that makes sense.\r\n", ch);
		else if (!*arg) {
			sprintf(buf, "What do you want to %s %d of?\r\n", sname, multi);
			send_to_char(buf, ch);
		} else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
			sprintf(buf, "You don't seem to have any %ss.\r\n", arg);
			send_to_char(buf, ch);
		} else {
			do {
				next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
				result = perform_drop(ch, obj, mode, sname, RDR);
				if (result != 0) {
					amount += result;
					total++;
				}
				if ((!next_obj || (strcmp(next_obj->name, obj->name))) && total != 0) {
					perform_drop_message(ch, obj, sname, total, mode);
					total = 0;
				}
				if (mode == SCMD_JUNK && result != 0) {
					extract_obj(obj);
				}
				obj = next_obj;
			} while (obj && --multi);
		}
	} else {
		dotmode = find_all_dots(arg);

		/* Can't junk or donate all */
		if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
			if (subcmd == SCMD_JUNK)
				send_to_char("Go to the dump if you want to junk EVERYTHING!\r\n", ch);
			else
				send_to_char("Go do the donation room if you want to donate EVERYTHING!\r\n", ch);
			return;
		}
		if (dotmode == FIND_ALL) {
			if (!ch->carrying)
				send_to_char("You don't seem to be carrying anything.\r\n", ch);
			else {
				for (obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->next_content;
					result = perform_drop(ch, obj, mode, sname, RDR);
					if (result != 0) {
						amount += result;
						total++;
					}
					if ((!next_obj || (strcmp(next_obj->name, obj->name))) && total != 0) {
						perform_drop_message(ch, obj, sname, total, mode);
						total = 0;
					}
					if (mode == SCMD_JUNK && result != 0) {
						extract_obj(obj);
					}
				}
			}
		} else if (dotmode == FIND_ALLDOT) {
			if (!*arg) {
				sprintf(buf, "What do you want to %s all of?\r\n", sname);
				send_to_char(buf, ch);
				return;
			}
			if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
				sprintf(buf, "You don't seem to have any %ss.\r\n", arg);
				send_to_char(buf, ch);
			}
			while (obj) {
				next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
				result = perform_drop(ch, obj, mode, sname, RDR);
				if (result != 0) {
					amount += result;
					total++;
				}
				if ((!next_obj || (strcmp(next_obj->name, obj->name))) && total != 0) {
					perform_drop_message(ch, obj, sname, total, mode);
					total = 0;
				}
				if (mode == SCMD_JUNK && result != 0) {
					extract_obj(obj);
				}
				obj = next_obj;
			}
		} else {
			if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
				sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
				send_to_char(buf, ch);
			} else {
				result = perform_drop(ch, obj, mode, sname, RDR);
				if (result != 0) {
					amount += result;
					total++;
				}
				if (total != 0) {
					perform_drop_message(ch, obj, sname, total, mode);
					total = 0;
				}
				if (mode == SCMD_JUNK && result != 0) {
					extract_obj(obj);
				}
			}
		}
	}

	if (amount && (subcmd == SCMD_JUNK)) {
		send_to_char("You have been rewarded by the gods!\r\n", ch);
		act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
		GET_GOLD(ch) += amount;
	}
}


void perform_give(struct char_data *ch, struct char_data *vict,
											 struct obj_data *obj)
{
	if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
		act("You can't let go of $p! Yeech!", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
		act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
		act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (!give_otrigger(obj, ch, vict) || !receive_mtrigger(vict, ch, obj))
		return;
	if (!obj) /* obj might be purged */
		return;

	obj_from_char(obj);
	obj_to_char(obj, vict);
	act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
	act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
	act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);

	autoquest_trigger_check(ch, vict, obj, AQ_RETURN_OBJ);
}


/* utility function for give */
struct char_data *give_find_vict(struct char_data *ch, char *arg)
{
	struct char_data *vict;

	if (!*arg) {
		send_to_char("To who?\r\n", ch);
		return (NULL);
	} else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM, 0))) {
		send_to_char(NOPERSON, ch);
		return (NULL);
	} else if (vict == ch) {
		send_to_char("What's the point of that?\r\n", ch);
		return (NULL);
	} else
		return (vict);
}


void perform_give_gold(struct char_data *ch, struct char_data *vict,
														int amount)
{
	if (amount <= 0) {
		send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
		return;
	}
	if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || (!IS_GOD(ch)))) {
		send_to_char("You don't have that many coins!\r\n", ch);
		return;
	}
	send_to_char(OK, ch);
	sprintf(buf, "$n gives you %d gold coin%s.", amount, amount == 1 ? "" : "s");
	act(buf, FALSE, ch, 0, vict, TO_VICT);
	sprintf(buf, "$n gives %s to $N.", money_desc(amount));
	act(buf, TRUE, ch, 0, vict, TO_NOTVICT);
	if (IS_NPC(ch) || !IS_GOD(ch))
		GET_GOLD(ch) -= amount;
	GET_GOLD(vict) += amount;

	bribe_mtrigger(vict, ch, amount);
}


ACMD(do_give)
{
	int amount, dotmode;
	struct char_data *vict;
	struct obj_data *obj, *next_obj;

	argument = one_argument(argument, arg);

	if (!*arg)
		send_to_char("Give what to who?\r\n", ch);
	else if (is_number(arg)) {
		amount = atoi(arg);
		argument = one_argument(argument, arg);
		if (!str_cmp("coins", arg) || !str_cmp("coin", arg)) {
			one_argument(argument, arg);
			if ((vict = give_find_vict(ch, arg)) != NULL)
				perform_give_gold(ch, vict, amount);
			return;
		} else if (!*arg) {        /* Give multiple code. */
			sprintf(buf, "What do you want to give %d of?\r\n", amount);
			send_to_char(buf, ch);
		} else if (!(vict = give_find_vict(ch, argument))) {
			return;
		} else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
			sprintf(buf, "You don't seem to have any %ss.\r\n", arg);
			send_to_char(buf, ch);
		} else {
			while (obj && amount--) {
				next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
				perform_give(ch, vict, obj);
				obj = next_obj;
			}
		}
	} else {
		one_argument(argument, buf1);
		if (!(vict = give_find_vict(ch, buf1)))
			return;
		dotmode = find_all_dots(arg);
		if (dotmode == FIND_INDIV) {
			if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
				sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
				send_to_char(buf, ch);
			} else
				perform_give(ch, vict, obj);
		} else {
			if (dotmode == FIND_ALLDOT && !*arg) {
				send_to_char("All of what?\r\n", ch);
				return;
			}
			if (!ch->carrying)
				send_to_char("You don't seem to be holding anything.\r\n", ch);
			else
				for (obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->next_content;
					if (CAN_SEE_OBJ(ch, obj) &&
							((dotmode == FIND_ALL || isname(arg, obj->name))))
						perform_give(ch, vict, obj);
				}
		}
	}
}


void weight_change_object(struct obj_data *obj, int weight)
{
	struct obj_data *tmp_obj;
	struct char_data *tmp_ch;

	if (IN_ROOM(obj) != NOWHERE) {
		GET_OBJ_WEIGHT(obj) += weight;
	} else if ((tmp_ch = obj->carried_by)) {
		obj_from_char(obj);
		GET_OBJ_WEIGHT(obj) += weight;
		obj_to_char(obj, tmp_ch);
	} else if ((tmp_obj = obj->in_obj)) {
		obj_from_obj(obj);
		GET_OBJ_WEIGHT(obj) += weight;
		obj_to_obj(obj, tmp_obj);
	} else {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unknown attempt to subtract weight from an object.");
	}
}


void name_from_drinkcon(struct obj_data *obj)
{

	char *new_name, *cur_name, *next;
	const char *liqname;
	int liqlen, cpylen;

	if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
		return;

	liqname = drinknames[GET_OBJ_VAL(obj, 2)];
	if (!isname(liqname, obj->name)) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Can't remove liquid '%s' from '%s' (%d) item.", liqname, obj->name, obj->item_number);
		return;
	}

	liqlen = strlen(liqname);
	CREATE(new_name, char, strlen(obj->name) - strlen(liqname)); /* +1 for NUL, -1 for space */

	for (cur_name = obj->name; cur_name; cur_name = next) {
		if (*cur_name == ' ')
			cur_name++;

		if ((next = strchr(cur_name, ' ')))
			cpylen = next - cur_name;
		else
			cpylen = strlen(cur_name);
 
		if (!strn_cmp(cur_name, liqname, liqlen))
			continue;
 
		if (*new_name)
			strcat(new_name, " ");
			strncat(new_name, cur_name, cpylen);
	}

	if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
		free(obj->name);
		obj->name = new_name;

}


void name_to_drinkcon(struct obj_data *obj, int type)
{
	char *new_name;

	if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
		return;

	CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
	sprintf(new_name, "%s %s", obj->name, drinknames[type]);
	if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
		free(obj->name);
	obj->name = new_name;
}


ACMD(do_drink)
{
	struct obj_data *temp;
	struct affected_type af;
	int amount, weight;
	int on_ground = 0;

	one_argument(argument, arg);

	if (IS_NPC(ch))        /* Cannot use GET_COND() on mobs. */
		return;

	if (!*arg) {
		send_to_char("Drink from what?\r\n", ch);
		return;
	}
	if (!(temp = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
		if (!(temp = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
			send_to_char("You can't find it!\r\n", ch);
			return;
		} else
			on_ground = 1;
	}
	if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
			(GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
		send_to_char("You can't drink from that!\r\n", ch);
		return;
	}
	if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
		send_to_char("You have to be holding that to drink from it.\r\n", ch);
		return;
	}
	if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
		/* The pig is drunk */
		send_to_char("You can't seem to get close enough to your mouth.\r\n", ch);
		act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
		return;
	}
	if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 0)) {
		send_to_char("Your stomach can't contain anymore!\r\n", ch);
		return;
	}
	if (!GET_OBJ_VAL(temp, 1)) {
		send_to_char("It's empty.\r\n", ch);
		return;
	}
	if (subcmd == SCMD_DRINK) {
		sprintf(buf, "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp, 2)]);
		act(buf, TRUE, ch, temp, 0, TO_ROOM);

		sprintf(buf, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
		send_to_char(buf, ch);

		if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
			amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK];
		else
			amount = number(3, 10);

	} else {
		act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
		sprintf(buf, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
		send_to_char(buf, ch);
		amount = 1;
	}

	amount = MIN(amount, GET_OBJ_VAL(temp, 1));

	/* You can't subtract more than the object weighs */
	weight = MIN(amount, GET_OBJ_WEIGHT(temp));

	weight_change_object(temp, -weight);        /* Subtract amount */

	gain_condition(ch, DRUNK,
				 (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount) / 4);

	gain_condition(ch, FULL,
					(int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][FULL] * amount) / 4);

	gain_condition(ch, THIRST,
				(int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount) / 4);

	if (GET_COND(ch, DRUNK) > 10)
		send_to_char("You feel drunk.\r\n", ch);

	if (GET_COND(ch, THIRST) > 20)
		send_to_char("You don't feel thirsty any more.\r\n", ch);

	if (GET_COND(ch, FULL) > 20)
		send_to_char("You are full.\r\n", ch);

	if (GET_OBJ_VAL(temp, 3)) {        /* The crap was poisoned ! */
		send_to_char("Oops, it tasted rather strange!\r\n", ch);
		act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);

		af.type = SPELL_POISON;
		af.duration = amount * 3;
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.bitvector = AFF_POISON;
		affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
	}
	/* empty the container, and no longer poison. */
	GET_OBJ_VAL(temp, 1) -= amount;
	if (!GET_OBJ_VAL(temp, 1)) {        /* The last bit */
		name_from_drinkcon(temp);
		GET_OBJ_VAL(temp, 2) = 0;
		GET_OBJ_VAL(temp, 3) = 0;
	}
	return;
}


ACMD(do_eat)
{
	struct obj_data *food;
	struct affected_type af;
	int amount;

	one_argument(argument, arg);

	if (IS_NPC(ch))        /* Cannot use GET_COND() on mobs. */
		return;

	if (!*arg) {
		send_to_char("Eat what?\r\n", ch);
		return;
	}
	if (!(food = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
		return;
	}
	if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
															 (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
		do_drink(ch, argument, 0, SCMD_SIP);
		return;
	}
	if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && !IS_GOD(ch)) {
		send_to_char("You can't eat THAT!\r\n", ch);
		return;
	}
	if (GET_COND(ch, FULL) > 20) {/* Stomach full */
		send_to_char("You are too full to eat more!\r\n", ch);
		return;
	}
	if (subcmd == SCMD_EAT) {
		act("You eat $p.", FALSE, ch, food, 0, TO_CHAR);
		act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
	} else {
		act("You nibble a little bit of $p.", FALSE, ch, food, 0, TO_CHAR);
		act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
	}

	amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);

	gain_condition(ch, FULL, amount);

	if (GET_COND(ch, FULL) > 20)
		send_to_char("You are full.\r\n", ch);

	if (GET_OBJ_VAL(food, 3) && !IS_IMMORTAL(ch)) {
		/* The crap was poisoned ! */
		send_to_char("Oops, that tasted rather strange!\r\n", ch);
		act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);

		af.type = SPELL_POISON;
		af.duration = amount * 2;
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.bitvector = AFF_POISON;
		affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
	}
	if (subcmd == SCMD_EAT)
		extract_obj(food);
	else {
		if (!(--GET_OBJ_VAL(food, 0))) {
			send_to_char("There's nothing left now.\r\n", ch);
			extract_obj(food);
		}
	}
}


ACMD(do_pour)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	struct obj_data *from_obj = NULL, *to_obj = NULL;
	int amount;

	two_arguments(argument, arg1, arg2);

	if (subcmd == SCMD_POUR) {
		if (!*arg1) {                /* No arguments */
			send_to_char("From what do you want to pour?\r\n", ch);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
			send_to_char("You can't find it!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
			send_to_char("You can't pour from that!\r\n", ch);
			return;
		}
	}
	if (subcmd == SCMD_FILL) {
		if (!*arg1) {                /* no arguments */
			send_to_char("What do you want to fill?  And what are you filling it from?\r\n", ch);
			return;
		}
		if (!(to_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
			send_to_char("You can't find it!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
			act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
			return;
		}
		if (!*arg2) {                /* no 2nd argument */
			act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis(ch, arg2, NULL, world[IN_ROOM(ch)].contents))) {
			sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
			send_to_char(buf, ch);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
			act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
			return;
		}
	}
	if (GET_OBJ_VAL(from_obj, 1) == 0) {
		act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
		return;
	}
	if (subcmd == SCMD_POUR) {        /* pour */
		if (!*arg2) {
			send_to_char("Where do you want it?  Out or in what?\r\n", ch);
			return;
		}
		if (!str_cmp(arg2, "out")) {
			act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
			act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);

			weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1)); /* Empty */

			name_from_drinkcon(from_obj);
			GET_OBJ_VAL(from_obj, 1) = 0;
			if (GET_OBJ_VAL(from_obj, 2) != LIQ_COLOR) {
				GET_OBJ_VAL(from_obj, 2) = 0;
				GET_OBJ_VAL(from_obj, 3) = 0;
			}

			return;
		}
		if (!(to_obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
			send_to_char("You can't find it!\r\n", ch);
			return;
		}
		if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
				(GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
			send_to_char("You can't pour anything into that.\r\n", ch);
			return;
		}
	}
	if (to_obj == from_obj) {
		send_to_char("A most unproductive effort.\r\n", ch);
		return;
	}
	if ((GET_OBJ_VAL(to_obj, 1) != 0) &&
			(GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2))) {
		send_to_char("There is already another liquid in it!\r\n", ch);
		return;
	}
	if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0))) {
		send_to_char("There is no room for more.\r\n", ch);
		return;
	}
	if (subcmd == SCMD_POUR) {
		sprintf(buf, "You pour the %s into the %s.",
						drinks[GET_OBJ_VAL(from_obj, 2)], arg2);
		send_to_char(buf, ch);
	}
	if (subcmd == SCMD_FILL) {
		act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
		act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
	}
	/* New alias */
	if (GET_OBJ_VAL(to_obj, 1) == 0)
		name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));

	/* First same type liq. */
	GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

	/* Then how much to pour */
	GET_OBJ_VAL(from_obj, 1) -= (amount =
												 (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));

	GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

	if (GET_OBJ_VAL(from_obj, 1) < 0) {        /* There was too little */
		GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
		amount += GET_OBJ_VAL(from_obj, 1);
		name_from_drinkcon(from_obj);
		GET_OBJ_VAL(from_obj, 1) = 0;
		GET_OBJ_VAL(from_obj, 2) = 0;
		GET_OBJ_VAL(from_obj, 3) = 0;
	}
	/* Then the poison boogie */
	GET_OBJ_VAL(to_obj, 3) =
		(GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));

	/* And the weight boogie */
	weight_change_object(from_obj, -amount);
	weight_change_object(to_obj, amount);        /* Add weight */
}


void wear_message(struct char_data *ch, struct obj_data *obj, int where)
{
	const char *wear_messages[][2] = {
		{"$n lights $p and holds it.",
		"You light $p and hold it."},

		{"$n slides $p on to $s right ring finger.",
		"You slide $p on to your right ring finger."},

		{"$n slides $p on to $s left ring finger.",
		"You slide $p on to your left ring finger."},

		{"$n wears $p around $s neck.",
		"You wear $p around your neck."},

		{"$n wears $p on $s body.",
		"You wear $p on your body."},

		{"$n wears $p on $s head.",
		"You wear $p on your head."},

		{"$n puts $p on $s legs.",
		"You put $p on your legs."},

		{"$n wears $p on $s feet.",
		"You wear $p on your feet."},

		{"$n puts $p on $s hands.",
		"You put $p on your hands."},

		{"$n wears $p on $s arms.",
		"You wear $p on your arms."},

		{"$n straps $p around $s arm as a shield.",
		"You start to use $p as a shield."},

		{"$n wears $p about $s body.",
		"You wear $p around your body."},

		{"$n wears $p around $s waist.",
		"You wear $p around your waist."},

		{"$n puts $p on around $s right wrist.",
		"You put $p on around your right wrist."},

		{"$n puts $p on around $s left wrist.",
		"You put $p on around your left wrist."},

		{"$n wields $p.",
		"You wield $p."},

		{"$n grabs $p.",
		"You grab $p."},

		{"$n wields $p as a second weapon.",
		"You wield $p as a second weapon."},

		{"$n wears $p on $s face.",
		"You wear $p on your face."},

	  {"$n releases $p and it begins to float.",
	  "You release $p and it begins to float."},

    {"$n wears $p on $s back.",
    "You wear $p on your back."},

		{"$n puts $p on $s belt.",
		"You put $p on your belt."},
		
		{"$n puts $p on $s belt.",
		"You put $p on your belt."},

		{"$n wears $p over the rest of $s clothing.",
		"You wear $p over the rest of your clothing."},
		
		{"$n puts $p around $s throat.",
		"You put $p around your throat."},

		{"$n puts $p around $s throat.",
		"You put $p around your throat."},

		{"$n puts $p on $s wings.",
		"You put $p on your wings."},

		{"$n puts $p on $s horns.",
		"You put $p on your horns."},

		{"$n slides $p onto $s tail.",
		"You slide $p onto your tail."}

	};
	
	/* If light has no burn hours left, you don't "light" it. */
	if (where == WEAR_LIGHT && !(GET_OBJ_VAL(obj, 2))) {
		act("$n grabs $p and tries to light it, but it remains unlit.", TRUE, ch, obj, 0, TO_ROOM);
		act("You grab $p and try to light it, but it remains unlit.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
	act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}


void perform_wear(struct char_data *ch, struct obj_data *obj, int where, int show)
{
	/*
	 * ITEM_WEAR_TAKE is used for objects that do not require special bits
	 * to be put into that position (e.g. you can hold any object, not just
	 * an object with a HOLD bit.)
	 */

	int wear_bitvectors[] = {
		ITEM_WEAR_TAKE, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
		ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS, ITEM_WEAR_FEET, 
		ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD, ITEM_WEAR_ABOUT, 
		ITEM_WEAR_WAIST, ITEM_WEAR_WRIST, ITEM_WEAR_WRIST, ITEM_WEAR_WIELD, 
		ITEM_WEAR_TAKE, ITEM_WEAR_DWIELD, ITEM_WEAR_FACE, ITEM_WEAR_FLOAT, 
		ITEM_WEAR_BACK, ITEM_WEAR_BELT, ITEM_WEAR_BELT, ITEM_WEAR_OUTSIDE, 
		ITEM_WEAR_THROAT, ITEM_WEAR_THROAT, ITEM_WEAR_WINGS, ITEM_WEAR_HORNS,
		ITEM_WEAR_TAIL
	};

	const char *already_wearing[] = {
		"You're already using a light.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"You're already wearing something on both of your ring fingers.\r\n",
		"You're already wearing something on your neck.\r\n",
		"You're already wearing something on your body.\r\n",
		"You're already wearing something on your head.\r\n",
		"You're already wearing something on your legs.\r\n",
		"You're already wearing something on your feet.\r\n",
		"You're already wearing something on your hands.\r\n",
		"You're already wearing something on your arms.\r\n",
		"You're already using a shield.\r\n",
		"You're already wearing something about your body.\r\n",
		"You already have something around your waist.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"You're already wearing something around both of your wrists.\r\n",
		"You're already wielding a weapon.\r\n",
		"You're already holding something.\r\n",
		"You're already wielding a second weapon.\r\n",
		"You're already wearing something on your face.\r\n",
		"There's already something floating near you.\r\n",
    "You're already wearing something on your back.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"You can't put anything else on your belt.\r\n",
		"You're already wearing something over the rest of your clothing.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"You can't wear anything else around your throat.\r\n",
		"You can't wear anything else on your wings.\r\n",
		"You can't wear anything else on your horns.\r\n",
		"You can't wear anything else around your tail.\r\n",
	};

	/* first, make sure that the character has the wear location. */
	if (!HAS_BODY(ch, where)) {
		send_to_char("You do not have that body position.\r\n", ch);
		return;
	}

	/* second, make sure that the object has the right size. */
	if (GET_SIZE(ch) < GET_OBJ_SIZE(obj) - 1 && GET_OBJ_SIZE(obj) != SIZE_NONE) {
		send_to_char("That item appears to be too large for you to use.\r\n", ch);
		return;
	} else if (GET_SIZE(ch) > GET_OBJ_SIZE(obj) && (GET_OBJ_TYPE(obj) == ITEM_WORN || GET_OBJ_TYPE(obj) == ITEM_ARMOR) && GET_OBJ_SIZE(obj) != SIZE_NONE) {
		send_to_char("That item appears to be too small for you to wear.\r\n", ch);
		return;
	}

	/* lastly, make sure that the wear position is valid. */
	if (!CAN_WEAR(obj, wear_bitvectors[where])) {
		if (show)
			act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

  /* Cheron: for back, check to make sure if you try to wear 'body' or 'about' that you 
   * remove the backpack first
   */
  if ((where == WEAR_BODY) || (where == WEAR_ABOUT)) {
    if (GET_EQ(ch, WEAR_BACK)) {
			if (show)
		    send_to_char("You really ought to remove what's on your back first.\r\n", ch);
      return;
    }
  }

	/* Artovil: For belt wear locations make sure that they have a belt first. */
  if ((where == WEAR_BELT_1) || (where == WEAR_BELT_2)) {
    if (!GET_EQ(ch, WEAR_WAIST)) {
			if (show)
		    send_to_char("You have to wear a belt before you can wear items on it.\r\n", ch);
      return;
    }
  }

	/* for finger, belt, throat and wrist, try pos 2 if pos 1 is already full */
	if ((where == WEAR_FINGER_R) || (where == WEAR_BELT_1) || (where == WEAR_WRIST_R) || (where == WEAR_THROAT_1))
		if (GET_EQ(ch, where) && HAS_BODY(ch, (where + 1)))
			where++;

	if (GET_EQ(ch, where)) {
		if (show)
			send_to_char(already_wearing[where], ch);
		return;
	}
	
	if (!wear_otrigger(obj, ch, where))
		return;
	if (show)
		wear_message(ch, obj, where);
	obj_from_char(obj);
	equip_char(ch, obj, where);
}


int	find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg)
{
	int where = -1;

	/* \r to prevent explicit wearing. Don't use \n, it's end-of-array marker. */
	const char *keywords[] = {
		"!RESERVED!",
		"finger",
		"!RESERVED!",
		"neck",
		"body",
		"head",
		"legs",
		"feet",
		"hands",
		"arms",
		"shield",
		"about",
		"waist",
		"wrist",
		"!RESERVED!",
		"!RESERVED!",
		"!RESERVED!",
		"!RESERVED!",
		"face",
		"float",
    "back",
		"belt",
		"!RESERVED!",
		"outside",
		"throat",
		"!RESERVED!",
		"wings",
		"horns",
		"tail",
		"\n"
	};

	if (!arg || !*arg) {

		if (CAN_WEAR(obj, ITEM_WEAR_FINGER))      where = WEAR_FINGER_R;
		if (CAN_WEAR(obj, ITEM_WEAR_NECK))        where = WEAR_NECK;
		if (CAN_WEAR(obj, ITEM_WEAR_BODY))        where = WEAR_BODY;
		if (CAN_WEAR(obj, ITEM_WEAR_HEAD))        where = WEAR_HEAD;
		if (CAN_WEAR(obj, ITEM_WEAR_LEGS))        where = WEAR_LEGS;
		if (CAN_WEAR(obj, ITEM_WEAR_FEET))        where = WEAR_FEET;
		if (CAN_WEAR(obj, ITEM_WEAR_HANDS))       where = WEAR_HANDS;
		if (CAN_WEAR(obj, ITEM_WEAR_ARMS))        where = WEAR_ARMS;
		if (CAN_WEAR(obj, ITEM_WEAR_SHIELD) && (!GET_EQ(ch, WEAR_DWIELD)))      where = WEAR_SHIELD;
		if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))       where = WEAR_ABOUT;
		if (CAN_WEAR(obj, ITEM_WEAR_WAIST))       where = WEAR_WAIST;
		if (CAN_WEAR(obj, ITEM_WEAR_WRIST))       where = WEAR_WRIST_R;
		if (CAN_WEAR(obj, ITEM_WEAR_FACE))        where = WEAR_FACE;
		if (CAN_WEAR(obj, ITEM_WEAR_FLOAT))       where = WEAR_FLOAT;
    if (CAN_WEAR(obj, ITEM_WEAR_BACK))        where = WEAR_BACK;
		if (CAN_WEAR(obj, ITEM_WEAR_BELT))				where = WEAR_BELT_1;
		if (CAN_WEAR(obj, ITEM_WEAR_OUTSIDE))			where = WEAR_OUTSIDE;
		if (CAN_WEAR(obj, ITEM_WEAR_THROAT))			where = WEAR_THROAT_1;
		if (CAN_WEAR(obj, ITEM_WEAR_WINGS))				where = WEAR_WINGS;
		if (CAN_WEAR(obj, ITEM_WEAR_HORNS))				where = WEAR_HORNS;
		if (CAN_WEAR(obj, ITEM_WEAR_TAIL))				where = WEAR_TAIL;

	} else {

		if (((where = search_block(arg, keywords, FALSE)) < 0) ||
			(*arg=='!')) {
			sprintf(buf, "'%s'?  What part of your body is THAT?\r\n", arg);
			send_to_char(buf, ch);
			return -1;
		}

	}

	return (where);
}


ACMD(do_wear)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	struct obj_data *obj, *next_obj;
	int where, dotmode, items_worn = 0;
	
	two_arguments(argument, arg1, arg2);
	
	if (!*arg1) {
		send_to_char("Wear what?\r\n", ch);
		return;
	}
	dotmode = find_all_dots(arg1);
	
	if (*arg2 && (dotmode != FIND_INDIV)) {
		send_to_char("You can't specify the same body location for more than one item!\r\n", ch);
		return;
	}
	if (dotmode == FIND_ALL) {
		for (obj = ch->carrying; obj; obj = next_obj) {
			next_obj = obj->next_content;
			if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
				items_worn++;
				perform_wear(ch, obj, where, 1);
			}
		}
		if (!items_worn)
			send_to_char("You don't seem to have anything wearable.\r\n", ch);
	} else if (dotmode == FIND_ALLDOT) {
		if (!*arg1) {
			send_to_char("Wear all of what?\r\n", ch);
			return;
		}
		if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
			sprintf(buf, "You don't seem to have any %ss.\r\n", arg1);
			send_to_char(buf, ch);
		} else
			while (obj) {
				next_obj = get_obj_in_list_vis(ch, arg1, NULL, obj->next_content);
				if ((where = find_eq_pos(ch, obj, 0)) >= 0)
					perform_wear(ch, obj, where, 1);
				else
					act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
				obj = next_obj;
			}
	} else {
		if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
			sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
			send_to_char(buf, ch);
		}	else {
			if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
				perform_wear(ch, obj, where, 1);
			else if (!*arg2)
				act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
		}
	}
}


ACMD(do_wield)
{
	struct obj_data *obj;
	struct obj_data *held = GET_EQ(ch, WEAR_HOLD);

	one_argument(argument, arg);

	if (!*arg)
		send_to_char("Wield what?\r\n", ch);
	else if (GET_EQ(ch, WEAR_DWIELD))
		send_to_char ("You are getting confused. Remove the secondary weapon first.\r\n", ch); /* this should never happen, but just in case:)  -spl */
	else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
	} else {
		if (held) {
			if (OBJ_FLAGGED(held, ITEM_TWO_HANDED) && GET_EQ(ch, WEAR_HOLD))
				send_to_char("You are already holding something with both your hands.\r\n", ch);
		}
		if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
			send_to_char("You can't wield that.\r\n", ch);
		else if (OBJ_FLAGGED(obj, ITEM_TWO_HANDED) && GET_EQ(ch, WEAR_HOLD))
			send_to_char("You have something in your other hand, please remove it first.\r\n", ch);
		else if (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
			send_to_char("It's too heavy for you to use.\r\n", ch);
		else
			perform_wear(ch, obj, WEAR_WIELD, 1);
	}
}


ACMD(do_grab)
{
	struct obj_data *obj;
	struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
	
	one_argument(argument, arg);
	
	if (!*arg)
		send_to_char("Hold what?\r\n", ch);
	/* begin KARDYMODS - dualwield.snippet */
	else if (GET_EQ(ch, WEAR_DWIELD))
		send_to_char ("You cannot dual wield AND hold something.\n\r", ch);
	/* end KARDYMODS - dualwield.snippet */
	else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
	} else {
		if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
			perform_wear(ch, obj, WEAR_LIGHT, 1);
		else if ((GET_EQ(ch, WEAR_WIELD) && OBJ_FLAGGED(wielded, ITEM_TWO_HANDED))) {
			send_to_char("You cannot hold something as you wield a two handed weapon.\r\n", ch);
			return;
		} else if (((GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_WIELD)) && OBJ_FLAGGED(obj, ITEM_TWO_HANDED))) {
			send_to_char("You need both hands free to hold that object.\r\n", ch);
			return;
		} else {
			if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND &&
					GET_OBJ_TYPE(obj) != ITEM_STAFF && GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
					GET_OBJ_TYPE(obj) != ITEM_POTION)
				send_to_char("You can't hold that.\r\n", ch);
			else
				perform_wear(ch, obj, WEAR_HOLD, 1);
		}
	}
}


void perform_remove(struct char_data *ch, int pos)
{
	struct obj_data *obj;

	if (!(obj = GET_EQ(ch, pos))) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "perform_remove: bad pos %d passed.", pos);
    return;
  }	else if (OBJ_FLAGGED(obj, ITEM_NODROP) && !IS_BUILDER(ch)) {
		act("You can't remove $p, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }	else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
		act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  } else if ((pos == WEAR_BODY) || (pos == WEAR_ABOUT)) {
    if (GET_EQ(ch, WEAR_BACK)) {
      send_to_char("You find the need to remove what's on your back first.\r\n", ch);
      perform_remove(ch, WEAR_BACK);
    }
  }
  if (!remove_otrigger(obj, ch))
    return;
  obj_to_char(unequip_char(ch, pos), ch);
  act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
  act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
}


ACMD(do_remove)
{
	int i, dotmode, found;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Remove what?\r\n", ch);
		return;
	}
	dotmode = find_all_dots(arg);

	if (dotmode == FIND_ALL) {
		found = 0;
		for (i = 0; i < NUM_WEARS; i++)
			if (GET_EQ(ch, i)) {
				perform_remove(ch, i);
				found = 1;
			}
		if (!found)
			send_to_char("You're not using anything.\r\n", ch);
	} else if (dotmode == FIND_ALLDOT) {
		if (!*arg)
			send_to_char("Remove all of what?\r\n", ch);
		else {
			found = 0;
			for (i = 0; i < NUM_WEARS; i++)
				if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) &&
						isname(arg, GET_EQ(ch, i)->name)) {
					perform_remove(ch, i);
					found = 1;
				}
			if (!found) {
				sprintf(buf, "You don't seem to be using any %ss.\r\n", arg);
				send_to_char(buf, ch);
			}
		}
	} else {
		/* Returns object pointer but we don't need it, just true/false. */
		if ((i = get_obj_pos_in_equip_vis(ch, arg, NULL, ch->equipment)) < 0) {
			sprintf(buf, "You don't seem to be using %s %s.\r\n", AN(arg), arg);
			send_to_char(buf, ch);
		} else
			perform_remove(ch, i);
			if (!GET_EQ(ch, WEAR_WIELD) && (GET_EQ(ch, WEAR_DWIELD))) {
				send_to_char("Since you are not wielding a weapon, you can't dual wield too!\r\n", ch);
				perform_remove(ch, WEAR_DWIELD);
			}
	}
}


void perform_sac_gold(struct char_data *ch, int amount)
{
	if (amount <= 0)
		send_to_char("Heh heh heh.. we are jolly funny today, eh?\r\n", ch);
	else if (GET_GOLD(ch) < amount)
		send_to_char("You don't have that many coins!\r\n", ch);
	else {
		sprintf(buf, "$n sacrifices %s which disappears in a puff of smoke!",
						money_desc(amount));
		act(buf, FALSE, ch, 0, 0, TO_ROOM);
		send_to_char("You sacrifice some gold which disappears in a puff of smoke!\r\n", ch);
		GET_GOLD(ch) -= amount;
	}
}


void perform_sac_message(struct char_data *ch, struct obj_data *obj, int amount)
{
	char *number = get_buffer(64);

	sprintf(number, "(%d) ", amount);
	sprintf(buf, "You sacrifice %s$p to your deity.", (amount > 1) ? number : "");
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	sprintf(buf, "$n sacrifices %s$p.", (amount > 1) ? number : "");
	act(buf, TRUE, ch, obj, 0, TO_ROOM);

	release_buffer(number);
}

int	perform_sac(struct char_data *ch, struct obj_data *obj)
{
	int value;

	if (!drop_otrigger(obj, ch))
		return 0;

	if (!obj) /* obj may be purged */
		return 0;

	if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE))) {
		act("You can't sacrifice $p.", FALSE, ch, obj, 0, TO_CHAR);
		return 0;
	}

	obj_from_room(obj);

	value = MAX(1, MIN(20, GET_OBJ_COST(obj) / 32));
	return (value);
}


/* 5.7.04 - added ability to sacrifice alldot modes
 * Catherine Gore of Arcane Realms
 * now supports the following:
 * 
 * sac all
 * sac all.item
 * sac item
 */
ACMD(do_sac)
{
	struct obj_data *obj, *next_obj;
	int dotmode, amount = 0, multi, total = 0, result = 0;

	one_argument(argument, arg);
	
	if (!*arg) {
		send_to_char("What do you want to sacrifice?\r\n", ch);
		return;
	} else if (is_number(arg)) {
		multi = atoi(arg);
		one_argument(argument, arg);
		if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
			perform_sac_gold(ch, multi);
		else if (multi <= 0)
			send_to_char("Yeah, that makes sense.\r\n", ch);
		else if (!*arg) {
			sprintf(buf, "What do you want to sacrifice %d of?\r\n", multi);
			send_to_char(buf, ch);
		} else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
			sprintf(buf, "There aren't any %ss here to sacrifice.\r\n", arg);
			send_to_char(buf, ch);
		} else {
			do {
				next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
				result = perform_sac(ch, obj);
				if (result != 0) {
					amount += result;
					total++;
				}
				if ((!next_obj || (strcmp(next_obj->name, obj->name))) && total != 0) {
					perform_sac_message(ch, obj, total);
					total = 0;
				}
				
				if (result != 0)
					extract_obj(obj);

				obj = next_obj;
			} while (obj && --multi);
		}
	} else {
		dotmode = find_all_dots(arg);

		if (dotmode == FIND_ALL) {
			if (!world[IN_ROOM(ch)].contents)
				send_to_char("There's nothing here to sacrifice.\r\n", ch);
			else {
				for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj) {
					next_obj = obj->next_content;
					result = perform_sac(ch, obj);
					if (result != 0) {
						amount += result;
						total++;
					}
					if ((!next_obj || (strcmp(next_obj->name, obj->name))) && total != 0) {
						perform_sac_message(ch, obj, total);
						total = 0;
					}
					if (result != 0)
						extract_obj(obj);
				}
			}
		} else if (dotmode == FIND_ALLDOT) {
			if (!*arg) {
				send_to_char("What do you want to sacrifice all of?\r\n", ch);
				return;
			}
			if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
				sprintf(buf, "There doesn't seem to be any %ss here.\r\n", arg);
				send_to_char(buf, ch);
			}
			while (obj) {
				next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
				result = perform_sac(ch, obj);
				if (result != 0) {
					amount += result;
					total++;
				}
				if ((!next_obj || (strcmp(next_obj->name, obj->name))) && total != 0) {
					perform_sac_message(ch, obj, total);
					total = 0;
				}
				if (result != 0)
					extract_obj(obj);

				obj = next_obj;
			}
		} else {
			if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
				sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
				send_to_char(buf, ch);
			} else {
				result = perform_sac(ch, obj);
				if (result != 0) {
					amount += result;
					total++;
				}
				if (total != 0) {
					perform_sac_message(ch, obj, total);
					total = 0;
				}
				if (result != 0)
					extract_obj(obj);

			}
		}
	}

	if (amount) {
		send_to_charf(ch, "Your deity gives you %d gold coin%s for your humility.\r\n", amount, (amount > 1) ? "s":"");
		act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
		GET_GOLD(ch) += amount;
	}
}


/* My dual wield function  -spl */
ACMD(do_dwield)
{
	struct obj_data *obj;
	
	one_argument(argument, arg);
	
	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_DUAL))
		send_to_char("You have no idea how to do that.\r\n", ch);
	else if (!skill_check(ch, SKILL_DUAL, 0))
		send_to_char("You try but it falls out of your hand.\r\n", ch);
	else if (!*arg)
		send_to_char("Dual wield what?\r\n", ch);
	else if (!GET_EQ(ch, WEAR_WIELD))
		send_to_char ("You must choose a primary weapon.\r\n", ch);
	else if (FIGHTING(ch))
		send_to_char("You are too busy fighting to attempt that!\r\n", ch);
	else if (GET_EQ(ch, WEAR_SHIELD))
		send_to_char ("You cannot dual wield while using a shield.\r\n", ch);
	else if (GET_EQ(ch, WEAR_LIGHT))
		send_to_char ("You cannot dual wield when holding a light source.\r\n", ch);
	else if (GET_EQ(ch, WEAR_HOLD))
		send_to_char ("You can't dual wield while holding an item.\r\n", ch);
	else if (OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD), ITEM_TWO_HANDED))
		send_to_char ("You can't wield a second weapon when you are wielding a two-handed weapon.\r\n", ch);
	else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
	} else {
		if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
			send_to_char("You can't wield that.\r\n", ch);
		else if (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
			send_to_char("It's too heavy for you to use.\r\n", ch);
		else
			perform_wear(ch, obj, WEAR_DWIELD, 1);
	}
}


ACMD(do_compare)
{
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	struct obj_data *obj1;
	struct obj_data *obj2;
	int value1;
	int value2;
	char *msg;

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );
	if (arg1[0] == '\0') {
		send_to_char( "Compare what to what?\n\r", ch );
		return;
	}

	if (!(obj1 = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
		send_to_char( "You do not have that item.\n\r", ch );
		return;
	}

	if (arg2[0] == '\0') {
		for (obj2 = ch->carrying; obj2; obj2 = obj2->next_content) {
			if ( !((obj2->obj_flags.type_flag == ITEM_WEAPON) ||
						 (obj2->obj_flags.type_flag == ITEM_FIREWEAPON) ||
						 (obj2->obj_flags.type_flag == ITEM_ARMOR) ||
						 (obj2->obj_flags.type_flag == ITEM_WORN))
			&&   CAN_SEE_OBJ(ch, obj2)
			&&   obj1->obj_flags.type_flag == obj2->obj_flags.type_flag
			&& CAN_GET_OBJ(ch, obj2) )
				break;
		}

		if (!obj2) {
			send_to_char( "You aren't wearing anything comparable.\n\r", ch );
			return;
		}
	} else {
		if (!(obj2 = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
			send_to_char( "You do not have that item.\n\r", ch );
			return;
		}
	}

	msg         = NULL;
	value1      = 0;
	value2      = 0;

	if (obj1 == obj2)
		msg = "You compare $p to itself.  It looks about the same.";
	else if ( obj1->obj_flags.type_flag != obj2->obj_flags.type_flag )
		msg = "You can't compare $p and $P.";
	else {
		switch (obj1->obj_flags.type_flag) {
		default:
			msg = "You can't compare $p and $P.";
			break;

		case ITEM_ARMOR:
			value1 = obj1->obj_flags.value[0];
			value2 = obj2->obj_flags.value[0];
			break;

		case ITEM_WEAPON:
			value1 = obj1->obj_flags.value[1] + obj1->obj_flags.value[2];
			value2 = obj2->obj_flags.value[1] + obj2->obj_flags.value[2];
			break;
		}
	}

	if (!msg) {
		if (value1 == value2)
			msg = "$p and $P look about the same.";
		else if (value1  > value2)
			msg = "$p looks better than $P.";
		else
			msg = "$p looks worse than $P.";
	}

	act(msg, FALSE, ch, obj1, obj2, TO_CHAR);
	return;
}


/*
 * Draw command for blade weapons worn at belt.
 * Torgny Bjers <artovil@arcanerealms.org>, 2002-12-16
 * Modified by Catherine Gore to work with the new sheathes
 */
void perform_get_from_sheath(struct char_data *ch, struct obj_data *obj)
{
	int wear;
	
	if ((GET_EQ(ch, WEAR_WIELD) && OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD), ITEM_TWO_HANDED)) || (GET_EQ(ch, WEAR_WIELD) && GET_EQ(ch, WEAR_DWIELD)))
		send_to_char ("You are already wielding weapons in both hands.\n\r", ch);
	else {
		/* Default to dwield, if not, then wield normally. */
		if (!GET_EQ(ch, WEAR_DWIELD) && GET_EQ(ch, WEAR_WIELD)) {
			if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_DUAL)) {
				send_to_char ("You are already wielding a weapon.\r\n", ch);
				return;
			}
			wear = WEAR_DWIELD;
		} else
			wear = WEAR_WIELD;

		/* Must be a weapon to be able to draw it. */
		if (!CAN_WEAR(obj, ITEM_WEAR_WIELD) && !CAN_WEAR(obj, ITEM_WEAR_DWIELD))
			send_to_char("You cannot wield that.\r\n", ch);
		/* Make sure that they can actually carry it too. */
		else if (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
			send_to_char("It's too heavy for you to use.\r\n", ch);
		else {
			if (!remove_otrigger(obj, ch))
				return;
			/* Remove the object from sheath. */
			obj_from_obj(obj);
			obj_to_char(obj, ch);
			/* Wield the object. */
			perform_wear(ch, obj, wear, 0);
			/* Let the player know that they did something. */
			act("You draw $p.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n draws $p.", TRUE, ch, obj, 0, TO_ROOM);
		}
	}
}
