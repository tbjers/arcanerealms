/* ************************************************************************
*		File: shop.c                                        Part of CircleMUD *
*	 Usage: shopkeepers: loading config files, spec procs.                  *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001-2002, Torgny Bjers.                                  *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
************************************************************************ */
/* $Id: shop.c,v 1.25 2003/01/17 21:36:00 arcanere Exp $ */

/***
 * The entire shop rewrite for Circle 3.0 was done by Jeff Fink.  Thanks Jeff!
 ***/

#define	__SHOP_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "shop.h"
#include "constants.h"
#include "spells.h"
#include "dg_scripts.h"
#include "screen.h"
#include "color.h"

/* External variables */
extern struct time_info_data time_info;
extern struct shop_data *shop_index;
extern int top_shop;
extern zone_rnum top_of_zone_table;

/* Forward/External function declarations */
ACMD(do_tell);
ACMD(do_action);
ACMD(do_echo);
ACMD(do_say);
void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode);
void clean_up (char *in);

/* Local variables */
int	cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_puke;

/* local functions */
void shopping_list(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
void shopping_value(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
void shopping_preview(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
void shopping_sell(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
void sort_keeper_objs(struct char_data *keeper, int shop_nr);
struct obj_data *get_selling_obj(struct char_data *ch, char *name, struct char_data *keeper, int shop_nr, int msg);
struct obj_data *slide_obj(struct obj_data *obj, struct char_data *keeper, int shop_nr);
void shopping_buy(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
struct obj_data *get_purchase_obj(struct char_data *ch, char *arg, struct char_data *keeper, int shop_nr, int msg);
struct obj_data *get_hash_obj_vis(struct char_data *ch, char *name, struct obj_data *list);
struct obj_data *get_slide_obj_vis(struct char_data *ch, char *name, struct obj_data *list);
void parse_shop(MYSQL_ROW shop_row, unsigned long *fieldlength, int vnum);
void assign_the_shopkeepers(void);
char *customer_string(int shop_nr, int detailed);
int	zdelete_check(int zone);
void list_all_shops(struct char_data *ch);
void handle_detailed_list(char *buf, char *buf1, struct char_data *ch);
void list_detailed_shop(struct char_data *ch, int shop_nr);
void show_shops(struct char_data *ch, char *arg);
int	is_ok_char(struct char_data *keeper, struct char_data *ch, int shop_nr);
int	is_open(struct char_data *keeper, int shop_nr, int msg);
int	is_ok(struct char_data *keeper, struct char_data *ch, int shop_nr);
void push(struct stack_data *stack, int pushval);
int	top(struct stack_data *stack);
int	pop(struct stack_data *stack);
void evaluate_operation(struct stack_data *ops, struct stack_data *vals);
int	find_oper_num(char token);
int	evaluate_expression(struct obj_data *obj, char *expr);
int	trade_with(struct obj_data *item, int shop_nr);
int	same_obj(struct obj_data *obj1, struct obj_data *obj2);
int	shop_producing(struct obj_data *item, int shop_nr);
int	transaction_amt(char *arg);
char *times_message(struct obj_data *obj, char *name, int num);
int	buy_price(struct obj_data *obj, int shop_nr);
int	sell_price(struct char_data *ch, struct obj_data *obj, int shop_nr);
char *list_object(struct obj_data *obj, int cnt, int index, int shop_nr);
int ok_shop_room(int shop_nr, room_vnum room);
SPECIAL(shop_keeper);
int	ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);
void destroy_shops(void);

/* config arrays */
const	char *operator_str[] = {
	"[({",
	"])}",
	"|+",
	"&*",
	"^'"
}	;

/* Constant list for printing out who we sell to */
const	char *trade_letters[] = {
	"Good",                 /* First, the alignment based ones */
	"Evil",
	"Neutral",
	"Magi",                 /* Then the class based ones */
	"Cleric",
	"Thief",
	"Warrior",
	"\n"
};


const	char *shop_bits[] = {
	"WILL_FIGHT",
	"USES_BANK",
	"\n"
};


int	is_ok_char(struct char_data *keeper, struct char_data *ch, int shop_nr)
{
	char buf[200];

	if (!(CAN_SEE(keeper, ch))) {
		do_say(keeper, MSG_NO_SEE_CHAR, cmd_say, 0);
		return (FALSE);
	}
	if (IS_GOD(ch))
		return (TRUE);

	if ((IS_GOOD(ch) && NOTRADE_GOOD(shop_nr)) ||
			(IS_EVIL(ch) && NOTRADE_EVIL(shop_nr)) ||
			(IS_NEUTRAL(ch) && NOTRADE_NEUTRAL(shop_nr))) {
		sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_SELL_ALIGN);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}
	if (IS_NPC(ch))
		return (TRUE);

	if ((IS_MAGI_TYPE(ch) && NOTRADE_MAGI_TYPE(shop_nr)) ||
			(IS_CLERIC_TYPE(ch) && NOTRADE_CLERIC_TYPE(shop_nr)) ||
			(IS_THIEF_TYPE(ch) && NOTRADE_THIEF_TYPE(shop_nr)) ||
			(IS_WARRIOR_TYPE(ch) && NOTRADE_WARRIOR_TYPE(shop_nr))) {
		sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_SELL_CLASS);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}
	return (TRUE);
}


int	is_open(struct char_data *keeper, int shop_nr, int msg)
{
	char buf[200];

	*buf = 0;
	if (SHOP_OPEN1(shop_nr) > time_info.hours)
		strcpy(buf, MSG_NOT_OPEN_YET);
	else if (SHOP_CLOSE1(shop_nr) < time_info.hours) {
		if (SHOP_OPEN2(shop_nr) > time_info.hours)
			strcpy(buf, MSG_NOT_REOPEN_YET);
		else if (SHOP_CLOSE2(shop_nr) < time_info.hours)
			strcpy(buf, MSG_CLOSED_FOR_DAY);
	}
	if (!*buf)
		return (TRUE);
	if (msg)
		do_say(keeper, buf, cmd_tell, 0);
	return (FALSE);
}


int	is_ok(struct char_data *keeper, struct char_data *ch, int shop_nr)
{
	if (is_open(keeper, shop_nr, TRUE))
		return (is_ok_char(keeper, ch, shop_nr));
	else
		return (FALSE);
}


void push(struct stack_data *stack, int pushval)
{
	S_DATA(stack, S_LEN(stack)++) = pushval;
}


int	top(struct stack_data *stack)
{
	if (S_LEN(stack) > 0)
		return (S_DATA(stack, S_LEN(stack) - 1));
	else
		return (NOTHING);
}


int	pop(struct stack_data *stack)
{
	if (S_LEN(stack) > 0)
		return (S_DATA(stack, --S_LEN(stack)));
	else {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Illegal expression %d in shop keyword list.", S_LEN(stack));
		return (0);
	}
}


void evaluate_operation(struct stack_data *ops, struct stack_data *vals)
{
	int oper;

	if ((oper = pop(ops)) == OPER_NOT)
		push(vals, !pop(vals));
	else {
		int val1 = pop(vals),
	val2 = pop(vals);

		/* Compiler would previously short-circuit these. */
		if (oper == OPER_AND)
			push(vals, val1 && val2);
		else if (oper == OPER_OR)
			push(vals, val1 || val2);
	}
}


int	find_oper_num(char token)
{
	int index;

	for (index = 0; index <= MAX_OPER; index++)
		if (strchr(operator_str[index], token))
			return (index);
	return (NOTHING);
}


int	evaluate_expression(struct obj_data *obj, char *expr)
{
	struct stack_data ops, vals;
	char *ptr, *end, name[200];
	int temp, index;

	if (!expr || !*expr)	/* Allows opening ( first. */
		return (TRUE);

	ops.len = vals.len = 0;
	ptr = expr;
	while (*ptr) {
		if (isspace(*ptr))
			ptr++;
		else {
			if ((temp = find_oper_num(*ptr)) == NOTHING) {
	end = ptr;
	while (*ptr && !isspace(*ptr) && find_oper_num(*ptr) == NOTHING)
		ptr++;
	strncpy(name, end, ptr - end);
	name[ptr - end] = '\0';
	for (index = 0; *extra_bits[index] != '\n'; index++)
		if (!str_cmp(name, extra_bits[index])) {
			push(&vals, OBJ_FLAGGED(obj, 1 << index));
			break;
		}
	if (*extra_bits[index] == '\n')
		push(&vals, isname(name, obj->name));
			} else {
	if (temp != OPER_OPEN_PAREN)
		while (top(&ops) > temp)
			evaluate_operation(&ops, &vals);

	if (temp == OPER_CLOSE_PAREN) {
		if ((temp = pop(&ops)) != OPER_OPEN_PAREN) {
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "Illegal parenthesis in shop keyword expression.");
			return (FALSE);
		}
	} else
		push(&ops, temp);
	ptr++;
			}
		}
	}
	while (top(&ops) != NOTHING)
		evaluate_operation(&ops, &vals);
	temp = pop(&vals);
	if (top(&vals) != NOTHING) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Extra operands left on shop keyword expression stack.");
		return (FALSE);
	}
	return (temp);
}


int	trade_with(struct obj_data *item, int shop_nr)
{
	int counter;

	if (GET_OBJ_COST(item) < 1)
		return (OBJECT_NOVAL);

	if (OBJ_FLAGGED(item, ITEM_NOSELL))
		return (OBJECT_NOTOK);

	for (counter = 0; SHOP_BUYTYPE(shop_nr, counter) != NOTHING; counter++)
		if (SHOP_BUYTYPE(shop_nr, counter) == GET_OBJ_TYPE(item)) {
			if (GET_OBJ_VAL(item, 2) == 0 &&
				(GET_OBJ_TYPE(item) == ITEM_WAND ||
				 GET_OBJ_TYPE(item) == ITEM_STAFF))
	return (OBJECT_DEAD);
			else if (evaluate_expression(item, SHOP_BUYWORD(shop_nr, counter)))
	return (OBJECT_OK);
		}
	return (OBJECT_NOTOK);
}


int	same_obj(struct obj_data *obj1, struct obj_data *obj2)
{
	int index;

	if (!obj1 || !obj2)
		return (obj1 == obj2);

	if (GET_OBJ_RNUM(obj1) != GET_OBJ_RNUM(obj2))
		return (FALSE);

	if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
		return (FALSE);

	if (GET_OBJ_EXTRA(obj1) != GET_OBJ_EXTRA(obj2))
		return (FALSE);

	for (index = 0; index < MAX_OBJ_AFFECT; index++)
		if ((obj1->affected[index].location != obj2->affected[index].location) ||
	(obj1->affected[index].modifier != obj2->affected[index].modifier))
			return (FALSE);

	return (TRUE);
}


int	shop_producing(struct obj_data *item, int shop_nr)
{
	int counter;

	if (GET_OBJ_RNUM(item) == NOTHING)
		return (FALSE);

	for (counter = 0; SHOP_PRODUCT(shop_nr, counter) != NOTHING; counter++)
		if (same_obj(item, &obj_proto[SHOP_PRODUCT(shop_nr, counter)]))
			return (TRUE);
	return (FALSE);
}


int	transaction_amt(char *arg)
{
	int num;
	char *buywhat;

	/*
	 * If we have two arguments, it means 'buy 5 3', or buy 5 of #3.
	 * We don't do that if we only have one argument, like 'buy 5', buy #5.
	 * Code from Andrey Fidrya <andrey@ALEX-UA.COM>
	 */
	buywhat = one_argument(arg, buf);
	if (*buywhat && *buf && is_number(buf)) {
		num = atoi(buf);
		strcpy(arg, arg + strlen(buf) + 1);
		return (num);
	}
	return (1);
}


char *times_message(struct obj_data *obj, char *name, int num)
{
	static char buf[256];
	char *ptr;
	int len;

	if (obj) {
		strcpy(buf, obj->short_description);
		len = strlen(obj->short_description);
	} else {
		if ((ptr = strchr(name, '.')) == NULL)
			ptr = name;
		else
			ptr++;
		len = sprintf(buf, "%s %s", AN(ptr), ptr);
	}

	if (num > 1)
		sprintf(buf + len, " (x %d)", num);
	return (buf);
}


struct obj_data *get_slide_obj_vis(struct char_data *ch, char *name,
										struct obj_data *list)
{
	struct obj_data *i, *last_match = NULL;
	int j, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp;

	strcpy(tmpname, name);
	tmp = tmpname;
	if (!(number = get_number(&tmp)))
		return (NULL);

	for (i = list, j = 1; i && (j <= number); i = i->next_content)
		if (isname(tmp, i->name))
			if (CAN_SEE_OBJ(ch, i) && !same_obj(last_match, i)) {
	if (j == number)
		return (i);
	last_match = i;
	j++;
			}
	return (NULL);
}


struct obj_data *get_hash_obj_vis(struct char_data *ch, char *name,
									 struct obj_data *list)
{
	struct obj_data *loop, *last_obj = NULL;
	int index;

	if (is_number(name))
		index = atoi(name);
	else if (is_number(name + 1))
		index = atoi(name + 1);
	else
		return (NULL);

	for (loop = list; loop; loop = loop->next_content)
		if (CAN_SEE_OBJ(ch, loop) && GET_OBJ_COST(loop) > 0)
			if (!same_obj(last_obj, loop)) {
	if (--index == 0)
		return (loop);
	last_obj = loop;
			}
	return (NULL);
}


struct obj_data *get_purchase_obj(struct char_data *ch, char *arg,
								struct char_data *keeper, int shop_nr, int msg)
{
	char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
	struct obj_data *obj;

	one_argument(arg, name);
	do {
		if (*name == '#' || is_number(name))
			obj = get_hash_obj_vis(ch, name, keeper->carrying);
		else
			obj = get_slide_obj_vis(ch, name, keeper->carrying);
		if (!obj) {
			if (msg) {
				sprintf(buf, shop_index[shop_nr].no_such_item1, GET_NAME(ch));
				do_tell(keeper, buf, cmd_tell, 0);
			}
			return (NULL);
		}
		if (GET_OBJ_COST(obj) <= 0) {
			extract_obj(obj);
			obj = 0;
		}
	} while (!obj);
	return (obj);
}


int	buy_price(struct obj_data *obj, int shop_nr)
{
	return (GET_OBJ_COST(obj) * SHOP_BUYPROFIT(shop_nr));
}


void shopping_buy(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr)
{
	char tempstr[200], buf[MAX_STRING_LENGTH];
	struct obj_data *obj, *last_obj = NULL;
	int goldamt = 0, buynum, bought = 0;

	if (!(is_ok(keeper, ch, shop_nr)))
		return;

	if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop_nr);

	if ((buynum = transaction_amt(arg)) < 0) {
		sprintf(buf, "%s A negative amount?  Try selling me something.",
			GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	if (!(*arg) || !(buynum)) {
		sprintf(buf, "%s What do you want to buy??", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
		return;

	if ((buy_price(obj, shop_nr) > GET_GOLD(ch)) && !IS_GOD(ch)) {
		sprintf(buf, shop_index[shop_nr].missing_cash2, GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);

		switch (SHOP_BROKE_TEMPER(shop_nr)) {
		case 0:
			/* do_action(keeper, GET_NAME(ch), cmd_puke, 0); */
			do_echo(keeper, "shrugs.", cmd_emote, SCMD_EMOTE);
			return;
		case 1:
			do_echo(keeper, "shrugs.", cmd_emote, SCMD_EMOTE);
			return;
		default:
			return;
		}
	}
	if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
		sprintf(buf, "%s: You can't carry any more items.\r\n",
			fname(obj->name));
		send_to_char(buf, ch);
		return;
	}
	if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch)) {
		sprintf(buf, "%s: You can't carry that much weight.\r\n",
			fname(obj->name));
		send_to_char(buf, ch);
		return;
	}
	while (obj && (GET_GOLD(ch) >= buy_price(obj, shop_nr) || IS_GOD(ch))
	 && IS_CARRYING_N(ch) < CAN_CARRY_N(ch) && bought < buynum
	 && IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch)) {
		bought++;
		/* Test if producing shop ! */
		if (shop_producing(obj, shop_nr)) {
			obj = read_object(GET_OBJ_RNUM(obj), REAL);
			load_otrigger(obj);
			} else {
			obj_from_char(obj);
			SHOP_SORT(shop_nr)--;
		}
		obj_to_char(obj, ch);

		goldamt += buy_price(obj, shop_nr);
		if (!IS_GOD(ch))
			GET_GOLD(ch) -= buy_price(obj, shop_nr);

		last_obj = obj;
		obj = get_purchase_obj(ch, arg, keeper, shop_nr, FALSE);
		if (!same_obj(obj, last_obj))
			break;
	}

	if (bought < buynum) {
		if (!obj || !same_obj(last_obj, obj))
			sprintf(buf, "%s I only have %d to sell you.", GET_NAME(ch), bought);
		else if (GET_GOLD(ch) < buy_price(obj, shop_nr))
			sprintf(buf, "%s You can only afford %d.", GET_NAME(ch), bought);
		else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
			sprintf(buf, "%s You can only hold %d.", GET_NAME(ch), bought);
		else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
			sprintf(buf, "%s You can only carry %d.", GET_NAME(ch), bought);
		else
			sprintf(buf, "%s Something screwy only gave you %d.", GET_NAME(ch),
				bought);
		do_tell(keeper, buf, cmd_tell, 0);
	}
	if (!IS_GOD(ch))
		GET_GOLD(keeper) += goldamt;

	strcpy(tempstr, times_message(ch->carrying, 0, bought));
	sprintf(buf, "$n buys %s.", tempstr);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);

	sprintf(buf, shop_index[shop_nr].message_buy, GET_NAME(ch), goldamt);
	do_tell(keeper, buf, cmd_tell, 0);
	sprintf(buf, "You now have %s.\r\n", tempstr);
	send_to_char(buf, ch);

	if (SHOP_USES_BANK(shop_nr))
		if (GET_GOLD(keeper) > MAX_OUTSIDE_BANK) {
			SHOP_BANK(shop_nr) += (GET_GOLD(keeper) - MAX_OUTSIDE_BANK);
			GET_GOLD(keeper) = MAX_OUTSIDE_BANK;
		}
}


struct obj_data *get_selling_obj(struct char_data *ch, char *name,
								struct char_data *keeper, int shop_nr, int msg)
{
	char buf[MAX_STRING_LENGTH];
	struct obj_data *obj;
	int result;

	if (!(obj = get_obj_in_list_vis(ch, name, NULL, ch->carrying))) {
		if (msg) {
			sprintf(buf, shop_index[shop_nr].no_such_item2, GET_NAME(ch));
			do_tell(keeper, buf, cmd_tell, 0);
		}
		return (NULL);
	}
	if ((result = trade_with(obj, shop_nr)) == OBJECT_OK)
		return (obj);

	switch (result) {
	case OBJECT_NOVAL:
		sprintf(buf, "%s You've got to be kidding, that thing is worthless!",
			GET_NAME(ch));
		break;
	case OBJECT_NOTOK:
		sprintf(buf, shop_index[shop_nr].do_not_buy, GET_NAME(ch));
		break;
	case OBJECT_DEAD:
		sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_USED_WANDSTAFF);
		break;
	default:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Illegal return value of %d from trade_with() (%s)",
			result, __FILE__);	/* Someone might rename it... */
		sprintf(buf, "%s An error has occurred.", GET_NAME(ch));
		break;
	}
	if (msg)
		do_tell(keeper, buf, cmd_tell, 0);
	return (NULL);
}


int	sell_price(struct char_data *ch, struct obj_data *obj, int shop_nr)
{
	return (GET_OBJ_COST(obj) * SHOP_SELLPROFIT(shop_nr));
}


struct obj_data *slide_obj(struct obj_data *obj, struct char_data *keeper,
									int shop_nr)
/*
	 This function is a slight hack!  To make sure that duplicate items are
	 only listed once on the "list", this function groups "identical"
	 objects together on the shopkeeper's inventory list.  The hack involves
	 knowing how the list is put together, and manipulating the order of
	 the objects on the list.  (But since most of DIKU is not encapsulated,
	 and information hiding is almost never used, it isn't that big a deal) -JF
*/
{
	struct obj_data *loop;
	int temp;

	if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop_nr);

	/* Extract the object if it is identical to one produced */
	if (shop_producing(obj, shop_nr)) {
		temp = GET_OBJ_RNUM(obj);
		extract_obj(obj);
		return (&obj_proto[temp]);
	}
	SHOP_SORT(shop_nr)++;
	loop = keeper->carrying;
	obj_to_char(obj, keeper);
	keeper->carrying = loop;
	while (loop) {
		if (same_obj(obj, loop)) {
			obj->next_content = loop->next_content;
			loop->next_content = obj;
			return (obj);
		}
		loop = loop->next_content;
	}
	keeper->carrying = obj;
	return (obj);
}


void sort_keeper_objs(struct char_data *keeper, int shop_nr)
{
	struct obj_data *list = NULL, *temp;

	while (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper)) {
		temp = keeper->carrying;
		obj_from_char(temp);
		temp->next_content = list;
		list = temp;
	}

	while (list) {
		temp = list;
		list = list->next_content;
		if (shop_producing(temp, shop_nr) &&
					!get_obj_in_list_num(GET_OBJ_RNUM(temp), keeper->carrying)) {
			obj_to_char(temp, keeper);
			SHOP_SORT(shop_nr)++;
		} else
			slide_obj(temp, keeper, shop_nr);
	}
}


void shopping_sell(char *arg, struct char_data *ch,
						struct char_data *keeper, int shop_nr)
{
	char tempstr[200], buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
	struct obj_data *obj;
	int sellnum, sold = 0, goldamt = 0;

	if (!(is_ok(keeper, ch, shop_nr)))
		return;

	if ((sellnum = transaction_amt(arg)) < 0) {
		sprintf(buf, "%s A negative amount?  Try buying something.",
			GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	if (!*arg || !sellnum) {
		sprintf(buf, "%s What do you want to sell??", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	one_argument(arg, name);
	if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
		return;

	if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) < sell_price(ch, obj, shop_nr)) {
		sprintf(buf, shop_index[shop_nr].missing_cash1, GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	while (obj && GET_GOLD(keeper) + SHOP_BANK(shop_nr) >=
			 sell_price(ch, obj, shop_nr) && sold < sellnum) {
		sold++;

		goldamt += sell_price(ch, obj, shop_nr);
		GET_GOLD(keeper) -= sell_price(ch, obj, shop_nr);
	
		obj_from_char(obj);
		slide_obj(obj, keeper, shop_nr);	/* Seems we don't use return value. */
		obj = get_selling_obj(ch, name, keeper, shop_nr, FALSE);
	}

	if (sold < sellnum) {
		if (!obj)
			sprintf(buf, "%s You only have %d of those.", GET_NAME(ch), sold);
		else if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) <
			 sell_price(ch, obj, shop_nr))
			sprintf(buf, "%s I can only afford to buy %d of those.",
				GET_NAME(ch), sold);
		else
			sprintf(buf, "%s Something really screwy made me buy %d.",
				GET_NAME(ch), sold);

		do_tell(keeper, buf, cmd_tell, 0);
	}
	GET_GOLD(ch) += goldamt;
	strcpy(tempstr, times_message(0, name, sold));
	sprintf(buf, "$n sells %s.", tempstr);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);

	sprintf(buf, shop_index[shop_nr].message_sell, GET_NAME(ch), goldamt);
	do_tell(keeper, buf, cmd_tell, 0);
	sprintf(buf, "The shopkeeper now has %s.\r\n", tempstr);
	send_to_char(buf, ch);

	if (GET_GOLD(keeper) < MIN_OUTSIDE_BANK) {
		goldamt = MIN(MAX_OUTSIDE_BANK - GET_GOLD(keeper), SHOP_BANK(shop_nr));
		SHOP_BANK(shop_nr) -= goldamt;
		GET_GOLD(keeper) += goldamt;
	}
}


void shopping_value(char *arg, struct char_data *ch,
						 struct char_data *keeper, int shop_nr)
{
	char buf[MAX_STRING_LENGTH];
	struct obj_data *obj;
	char name[MAX_INPUT_LENGTH];

	if (!is_ok(keeper, ch, shop_nr))
		return;

	if (!*arg) {
		sprintf(buf, "%s What do you want me to evaluate?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	one_argument(arg, name);
	if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
		return;

	sprintf(buf, "%s I'll give you %d gold coins for that!", GET_NAME(ch),
		sell_price(ch, obj, shop_nr));
	do_tell(keeper, buf, cmd_tell, 0);

	return;
}


#define SHOW_OBJ_LONG			0
#define SHOW_OBJ_SHORT		1
#define SHOW_OBJ_ACTION		2

void shopping_preview(char *arg, struct char_data *ch,
						 struct char_data *keeper, int shop_nr)
{
	char buf[MAX_STRING_LENGTH];
	struct obj_data *obj;
	struct extra_descr_data *extradesc;
	char name[MAX_INPUT_LENGTH];
	int buynum;

	if (!is_ok(keeper, ch, shop_nr))
		return;

	if ((buynum = transaction_amt(arg)) < 0) {
		sprintf(buf, "%s What do you want me to show you?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	if (!(*arg) || !(buynum)) {
		sprintf(buf, "%s What do you want me to show you?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	one_argument(arg, name);
	if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
		return;

	if ((extradesc = obj->ex_description) != NULL) {
		if (extradesc->keyword && extradesc->description) {
			sprintf(buf, "%s Here, take a look.", GET_NAME(ch));
			do_tell(keeper, buf, cmd_tell, 0);
			strncpy(buf, extradesc->description, sizeof(buf));
			proc_color(buf, COLOR_LEV(ch), TRUE, GET_OBJ_COLOR(obj), GET_OBJ_RESOURCE(obj));
			clean_up(buf);
			send_to_charf(ch, "%s\r\n", buf);
		}
	} else {
		sprintf(buf, "%s Here, take a look.", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		show_obj_to_char(obj, ch, SHOW_OBJ_ACTION);
	}

	return;
}

#undef SHOW_OBJ_LONG
#undef SHOW_OBJ_SHORT
#undef SHOW_OBJ_ACTION


char *list_object(struct obj_data *obj, int cnt, int index, int shop_nr)
{
	static char buf[256];
	char buf2[300], buf3[200];

	if (shop_producing(obj, shop_nr))
		strcpy(buf2, "Unlimited   ");
	else
		sprintf(buf2, "%5d       ", cnt);
	sprintf(buf, " %2d)  %s", index, buf2);

	/* Compile object name and information */
	strcpy(buf3, obj->short_description);
	proc_color(buf3, CMP, TRUE, GET_OBJ_COLOR(obj), GET_OBJ_RESOURCE(obj));
	if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON && GET_OBJ_VAL(obj, 1))
		sprintf(END_OF(buf3), " of %s", drinks[GET_OBJ_VAL(obj, 2)]);

	/* FUTURE: */
	/* Add glow/hum/etc */

	if (GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF)
		if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1))
			strcat(buf3, " (partially used)");

	sprintf(buf, "%s%6d   %-48.48s\r\n", buf, buy_price(obj, shop_nr), CAP(buf3));
	return (buf);
}


void shopping_list(char *arg, struct char_data *ch,
						struct char_data *keeper, int shop_nr)
{
	char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
	struct obj_data *obj, *last_obj = NULL;
	int cnt = 0, index = 0, found = FALSE;
	/* cnt is the number of that particular object available */

	if (!is_ok(keeper, ch, shop_nr))
		return;

	if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop_nr);

	one_argument(arg, name);
	strcpy(buf, " ##   Available     Cost   Item\r\n");
	strcat(buf, "-------------------------------------------------------------------------\r\n");
	if (keeper->carrying)
		for (obj = keeper->carrying; obj; obj = obj->next_content)
			if (CAN_SEE_OBJ(ch, obj) && GET_OBJ_COST(obj) > 0) {
	if (!last_obj) {
		last_obj = obj;
		cnt = 1;
	} else if (same_obj(last_obj, obj))
		cnt++;
	else {
		index++;
		if (!*name || isname(name, last_obj->name)) {
			strcat(buf, list_object(last_obj, cnt, index, shop_nr));
			found = TRUE;
		}
		cnt = 1;
		last_obj = obj;
	}
			}
	index++;
	if (!last_obj) /* we actually have nothing in our list for sale, period */
		strcpy(buf, "Currently, there is nothing for sale.\r\n");
	else if (!*name || isname(name, last_obj->name)) /* show last obj */
		strcat(buf, list_object(last_obj, cnt, index, shop_nr));
	else if (!found) /* nothing the char was looking for was found */
		strcpy(buf, "Presently, none of those are for sale.\r\n");

	page_string(ch->desc, buf, TRUE);
}


int ok_shop_room(int shop_nr, room_vnum room)
{
	int index;

	for (index = 0; SHOP_ROOM(shop_nr, index) != NOWHERE; index++)
		if (SHOP_ROOM(shop_nr, index) == room)
			return (TRUE);
	return (FALSE);
}


SPECIAL(shop_keeper)
{
	char argm[MAX_INPUT_LENGTH];
	struct char_data *keeper = (struct char_data *) me;
	int shop_nr;

	for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
		if (SHOP_KEEPER(shop_nr) == keeper->nr)
			break;

	if (shop_nr > top_shop)
		return (FALSE);

	if (SHOP_FUNC(shop_nr))	/* Check secondary function */
		if ((SHOP_FUNC(shop_nr)) (ch, me, cmd, argument))
			return (TRUE);

	if (keeper == ch) {
		if (cmd)
			SHOP_SORT(shop_nr) = 0;	/* Safety in case "drop all" */
		return (FALSE);
	}
	if (!ok_shop_room(shop_nr, GET_ROOM_VNUM(IN_ROOM(ch))))
		return (0);

	if (!AWAKE(keeper))
		return (FALSE);

	if (CMD_IS("steal")) {
		char *arg1 = get_buffer(MAX_INPUT_LENGTH), *arg2 = get_buffer(MAX_INPUT_LENGTH);
		skip_spaces(&argument);
		two_arguments(argument, arg1, arg2);
		if (isname(arg2, keeper->player.name)) {
			sprintf(argm, "$N shouts '%s'", MSG_NO_STEAL_HERE);
			act(argm, FALSE, ch, 0, keeper, TO_CHAR);
			release_buffer(arg1);
			release_buffer(arg2);
			return (TRUE);
		}
		release_buffer(arg1);
		release_buffer(arg2);
	}

	if (CMD_IS("buy")) {
		shopping_buy(argument, ch, keeper, shop_nr);
		return (TRUE);
	} else if (CMD_IS("sell")) {
		shopping_sell(argument, ch, keeper, shop_nr);
		return (TRUE);
	} else if (CMD_IS("value")) {
		shopping_value(argument, ch, keeper, shop_nr);
		return (TRUE);
	} else if (CMD_IS("preview")) {
		shopping_preview(argument, ch, keeper, shop_nr);
		return (TRUE);
	} else if (CMD_IS("list")) {
		shopping_list(argument, ch, keeper, shop_nr);
		return (TRUE);
	}
	return (FALSE);
}


int	ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim)
{
	char buf[200];
	int index;

	/* Prevent "invincible" shopkeepers if they're charmed. */
	if (AFF_FLAGGED(victim, AFF_CHARM))
		return (TRUE);

	if (IS_MOB(victim) && mob_index[GET_MOB_RNUM(victim)].func == shop_keeper)
		for (index = 0; index <= top_shop; index++)
			if (GET_MOB_RNUM(victim) == SHOP_KEEPER(index) && !SHOP_KILL_CHARS(index)) {
				do_action(victim, GET_NAME(ch), cmd_slap, 0);
				sprintf(buf, "%s %s", GET_NAME(ch), MSG_CANT_KILL_KEEPER);
				do_tell(victim, buf, cmd_tell, 0);
				return (FALSE);
			}
	return (TRUE);
}


void assign_the_shopkeepers(void)
{
	int index;

	cmd_say = find_command("say");
	cmd_tell = find_command("tell");
	cmd_emote = find_command("emote");
	cmd_slap = find_command("slap");
	cmd_puke = find_command("puke");
	for (index = 0; index <= top_shop; index++) {
		if (SHOP_KEEPER(index) == NOBODY)
			continue;
		if (mob_index[SHOP_KEEPER(index)].func)
			SHOP_FUNC(index) = mob_index[SHOP_KEEPER(index)].func;
		mob_index[SHOP_KEEPER(index)].func = shop_keeper;
	}
}


char *customer_string(int shop_nr, int detailed)
{
	int index, cnt = 1;
	static char buf[256];

	*buf = 0;
	for (index = 0; *trade_letters[index] != '\n'; index++, cnt *= 2)
		if (!(SHOP_TRADE_WITH(shop_nr) & cnt)) {
			if (detailed) {
				if (*buf)
					strcat(buf, ", ");
				strcat(buf, trade_letters[index]);
			} else
				sprintf(END_OF(buf), "%c", *trade_letters[index]);
		} else if (!detailed)
			strcat(buf, "_");

	return (buf);
}


int	zdelete_check(int zone)
{
	int shop_nr;

	for (shop_nr = 0; shop_nr < top_shop; shop_nr++) {
		if ((SHOP_NUM(shop_nr) / 100) == zone)
			return(1);
		}
	return(0);
}

void list_all_shops(struct char_data *ch)
{
	char *printbuf = get_buffer(65536);
	int shop_nr;

	for (shop_nr = 0; shop_nr <= top_shop; shop_nr++) {
		if (!(shop_nr % (GET_PAGE_LENGTH(ch) - 2))) {
			strcat(printbuf, " ##   Virtual   Where    Keeper    Buy   Sell   Customers\r\n");
			strcat(printbuf, "---------------------------------------------------------\r\n");
		}
		sprintf(buf2, "%3d   %6d   %6d    ", shop_nr + 1, SHOP_NUM(shop_nr),
			SHOP_ROOM(shop_nr, 0));
		if (SHOP_KEEPER(shop_nr) == NOBODY)
			strcpy(buf1, "<NONE>");
		else
			sprintf(buf1, "%6d", mob_index[SHOP_KEEPER(shop_nr)].vnum);
		sprintf(END_OF(buf2), "%s   %3.2f   %3.2f    ", buf1,
			SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr));
		strcat(buf2, customer_string(shop_nr, FALSE));
		sprintf(END_OF(printbuf), "%s\r\n", buf2);
	}

	page_string(ch->desc, printbuf, TRUE);
	release_buffer(printbuf);
}


void handle_detailed_list(char *buf, char *buf1, struct char_data *ch)
{
	if (strlen(buf1) + strlen(buf) < 78 || strlen(buf) < 20)
		strcat(buf, buf1);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
		sprintf(buf, "            %s", buf1);
	}
}


void list_detailed_shop(struct char_data *ch, int shop_nr)
{
	struct obj_data *obj;
	struct char_data *k;
	int index, temp;

	sprintf(buf, "Vnum:       [%5d], Rnum: [%5d]\r\n", SHOP_NUM(shop_nr),
		shop_nr + 1);
	send_to_char(buf, ch);

	strcpy(buf, "Rooms:      ");
	for (index = 0; SHOP_ROOM(shop_nr, index) != NOWHERE; index++) {
		if (index)
			strcat(buf, ", ");
		if ((temp = real_room(SHOP_ROOM(shop_nr, index))) != NOWHERE)
			sprintf(buf1, "%s (#%d)", world[temp].name, GET_ROOM_VNUM(temp));
		else
			sprintf(buf1, "<UNKNOWN> (#%d)", SHOP_ROOM(shop_nr, index));
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)
		send_to_char("Rooms:      None!\r\n", ch);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}

	strcpy(buf, "Shopkeeper: ");
	if (SHOP_KEEPER(shop_nr) != NOBODY) {
		sprintf(END_OF(buf), "%s (#%d), Special Function: %s\r\n",
			GET_NAME(&mob_proto[SHOP_KEEPER(shop_nr)]),
				mob_index[SHOP_KEEPER(shop_nr)].vnum, YESNO(SHOP_FUNC(shop_nr)));
		if ((k = get_char_num(SHOP_KEEPER(shop_nr)))) {
			send_to_char(buf, ch);
			sprintf(buf, "Coins:      [%9d], Bank: [%9d] (Total: %d)\r\n",
			 GET_GOLD(k), SHOP_BANK(shop_nr), GET_GOLD(k) + SHOP_BANK(shop_nr));
		}
	} else
		strcat(buf, "<NONE>\r\n");
	send_to_char(buf, ch);

	strcpy(buf1, customer_string(shop_nr, TRUE));
	sprintf(buf, "Customers:  %s\r\n", (*buf1) ? buf1 : "None");
	send_to_char(buf, ch);

	strcpy(buf, "Produces:   ");
	for (index = 0; SHOP_PRODUCT(shop_nr, index) != NOTHING; index++) {
		obj = &obj_proto[SHOP_PRODUCT(shop_nr, index)];
		if (index)
			strcat(buf, ", ");
		sprintf(buf1, "%s (#%d)", obj->short_description,
			obj_index[SHOP_PRODUCT(shop_nr, index)].vnum);
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)
		send_to_char("Produces:   Nothing!\r\n", ch);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}

	strcpy(buf, "Buys:       ");
	for (index = 0; SHOP_BUYTYPE(shop_nr, index) != NOTHING; index++) {
		if (index)
			strcat(buf, ", ");
		sprintf(buf1, "%s (#%d) ", item_types[SHOP_BUYTYPE(shop_nr, index)],
			SHOP_BUYTYPE(shop_nr, index));
		if (SHOP_BUYWORD(shop_nr, index))
			sprintf(END_OF(buf1), "[%s]", SHOP_BUYWORD(shop_nr, index));
		else
			strcat(buf1, "[all]");
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)
		send_to_char("Buys:       Nothing!\r\n", ch);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}

	sprintf(buf, "Buy at:     [%4.2f], Sell at: [%4.2f], Open: [%d-%d, %d-%d]%s",
		 SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr), SHOP_OPEN1(shop_nr),
	 SHOP_CLOSE1(shop_nr), SHOP_OPEN2(shop_nr), SHOP_CLOSE2(shop_nr), "\r\n");

	send_to_char(buf, ch);

	sprintbit(SHOP_BITVECTOR(shop_nr), shop_bits, buf1, sizeof(buf1));
	sprintf(buf, "Bits:       %s\r\n", buf1);
	send_to_char(buf, ch);
}


void show_shops(struct char_data *ch, char *arg)
{
	int shop_nr;

	if (!*arg)
		list_all_shops(ch);
	else {
		if (!str_cmp(arg, ".")) {
			for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
	if (ok_shop_room(shop_nr, GET_ROOM_VNUM(IN_ROOM(ch))))
		break;

			if (shop_nr > top_shop) {
				send_to_char("This isn't a shop!\r\n", ch);
				return;
			}
		} else if (is_number(arg))
			shop_nr = atoi(arg) - 1;
		else
			shop_nr = -1;

		if (shop_nr < 0 || shop_nr > top_shop) {
			send_to_char("Illegal shop number.\r\n", ch);
			return;
		}
		list_detailed_shop(ch, shop_nr);
	}
}


/* New version of shop loading for MySQL by Torgny Bjers, 2002-04-06. */
void parse_shop(MYSQL_ROW shop_row, unsigned long *fieldlength, int vnum)
{
	MYSQL_RES *products, *keywords, *rooms;
	MYSQL_ROW products_row, keywords_row, rooms_row;
	static int shop_nr = 0, zone = 0;
	int row_count, count = 0;
	bool error = FALSE;

	if (vnum <= (zone ? zone_table[zone - 1].top : -1)) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Shop #%d is below zone %d.", vnum, zone);
		kill_mysql();
		exit(1);
	}
	while (vnum > zone_table[zone].top) {
		if (++zone > top_of_zone_table) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Shop %d is outside of any zone.", vnum);
			kill_mysql();
			exit(1);
		}
	}

	SHOP_NUM(shop_nr) = vnum;

	shop_index[shop_nr].no_such_item1 = str_dup(shop_row[2]);
	shop_index[shop_nr].no_such_item2 = str_dup(shop_row[3]);
	shop_index[shop_nr].do_not_buy = str_dup(shop_row[4]);
	shop_index[shop_nr].missing_cash1 = str_dup(shop_row[5]);
	shop_index[shop_nr].missing_cash2 = str_dup(shop_row[6]);
	shop_index[shop_nr].message_buy = str_dup(shop_row[7]);
	shop_index[shop_nr].message_sell = str_dup(shop_row[8]);

	SHOP_BROKE_TEMPER(shop_nr) = atoi(shop_row[9]);
	SHOP_BITVECTOR(shop_nr) = atoi(shop_row[10]);
	SHOP_KEEPER(shop_nr) = atoi(shop_row[11]);

	SHOP_KEEPER(shop_nr) = real_mobile(SHOP_KEEPER(shop_nr));

	SHOP_TRADE_WITH(shop_nr) = atoi(shop_row[12]);

	SHOP_OPEN1(shop_nr) = atoi(shop_row[13]);
	SHOP_CLOSE1(shop_nr) = atoi(shop_row[14]);
	SHOP_OPEN2(shop_nr) = atoi(shop_row[15]);
	SHOP_CLOSE2(shop_nr) = atoi(shop_row[16]);

	sscanf(shop_row[17], "%f", &SHOP_BUYPROFIT(shop_nr));
	sscanf(shop_row[18], "%f", &SHOP_SELLPROFIT(shop_nr));

	/* open MySQL table for shop products */
	if (!(products = mysqlGetResource(TABLE_SHP_PRODUCTS, "SELECT * FROM %s WHERE vnum = %d ORDER BY item ASC;", TABLE_SHP_PRODUCTS, vnum)))
		error = TRUE;
	/* open MySQL table for shop buy keywords */
	if (!(keywords = mysqlGetResource(TABLE_SHP_KEYWORDS, "SELECT * FROM %s WHERE vnum = %d ORDER BY id ASC;", TABLE_SHP_KEYWORDS, vnum)))
		error = TRUE;
	/* open MySQL table for shop rooms */
	if (!(rooms = mysqlGetResource(TABLE_SHP_ROOMS, "SELECT * FROM %s WHERE vnum = %d ORDER BY room ASC;", TABLE_SHP_ROOMS, vnum)))
		error = TRUE;

	if (error) {
		kill_mysql();
		exit(1);
	}

	/*
	 * Load the product list from MySQL,
	 * this list has to be terminated with -1.
	 */
	row_count = mysql_num_rows(products);
	CREATE(shop_index[shop_nr].producing, obj_vnum, row_count + 1);
	while ((products_row = mysql_fetch_row(products)))
		SHOP_PRODUCT(shop_nr, count++) = real_object(atoi(products_row[3]));
	SHOP_PRODUCT(shop_nr, count) = NOTHING;

	mysql_free_result(products);

	count = 0;

	/*
	 * Load the keyword buy list from MySQL,
	 * this list has to be terminated with -1.
	 */
	row_count = mysql_num_rows(keywords);
	CREATE(shop_index[shop_nr].type, struct shop_buy_data, row_count + 1);
	while ((keywords_row = mysql_fetch_row(keywords)))
	{
		SHOP_BUYTYPE(shop_nr, count) = atoi(keywords_row[3]);
		SHOP_BUYWORD(shop_nr, count++) = str_dup(keywords_row[4]);
	}
	SHOP_BUYTYPE(shop_nr, count) = NOTHING;
	SHOP_BUYWORD(shop_nr, count) = 0;

	mysql_free_result(keywords);

	count = 0;

	/*
	 * Load the room list from MySQL,
	 * this list has to be terminated with -1.
	 */
	row_count = mysql_num_rows(rooms);
	CREATE(shop_index[shop_nr].in_room, room_rnum, row_count + 1);
	while ((rooms_row = mysql_fetch_row(rooms)))
		SHOP_ROOM(shop_nr, count++) = atoi(rooms_row[3]);
	SHOP_ROOM(shop_nr, count) = NOTHING;
	
	mysql_free_result(rooms);

	SHOP_BANK(top_shop) = 0;
	SHOP_SORT(top_shop) = 0;
	SHOP_FUNC(top_shop) = NULL;

	top_shop = shop_nr++;
}


void destroy_shops(void)
{
	ssize_t cnt, itr;

	if (!shop_index)
		return;

	for (cnt = 0; cnt <= top_shop; cnt++) {
		if (shop_index[cnt].no_such_item1)
			free(shop_index[cnt].no_such_item1);
		if (shop_index[cnt].no_such_item2)
			free(shop_index[cnt].no_such_item2);
		if (shop_index[cnt].missing_cash1)
			free(shop_index[cnt].missing_cash1);
		if (shop_index[cnt].missing_cash2)
			free(shop_index[cnt].missing_cash2);
		if (shop_index[cnt].do_not_buy)
			free(shop_index[cnt].do_not_buy);
		if (shop_index[cnt].message_buy)
			free(shop_index[cnt].message_buy);
		if (shop_index[cnt].message_sell)
			free(shop_index[cnt].message_sell);
		if (shop_index[cnt].in_room)
			free(shop_index[cnt].in_room);
		if (shop_index[cnt].producing)
			free(shop_index[cnt].producing);

		if (shop_index[cnt].type) {
			for (itr = 0; BUY_TYPE(shop_index[cnt].type[itr]) != NOTHING; itr++)
				if (BUY_WORD(shop_index[cnt].type[itr]))
					free(BUY_WORD(shop_index[cnt].type[itr]));
			free(shop_index[cnt].type);
		}
	}

	free(shop_index);
	shop_index = NULL;
	top_shop = -1;
}
