/* ************************************************************************
*  Generic OLC Library - Shops / genshp.c                            v1.0 *
*  Copyright 1996 by Harvey Gilpin                                        *
*  Copyright 1997-1999 by George Greer (greerga@circlemud.org)            *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001-2002, Torgny Bjers.                                  *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
************************************************************************ */
/* $Id: genshp.c,v 1.11 2002/12/28 16:17:02 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genshp.h"
#include "genzon.h"

extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct shop_data *shop_index;
extern struct zone_data *zone_table;
extern int top_shop;
extern zone_rnum top_of_zone_table;

/*-------------------------------------------------------------------*/

void copy_shop(struct shop_data *tshop, struct shop_data *fshop)
{
	/*
	 * Copy basic information over.
	 */
	S_NUM(tshop) = S_NUM(fshop);
	S_KEEPER(tshop) = S_KEEPER(fshop);
	S_OPEN1(tshop) = S_OPEN1(fshop);
	S_CLOSE1(tshop) = S_CLOSE1(fshop);
	S_OPEN2(tshop) = S_OPEN2(fshop);
	S_CLOSE2(tshop) = S_CLOSE2(fshop);
	S_BANK(tshop) = S_BANK(fshop);
	S_BROKE_TEMPER(tshop) = S_BROKE_TEMPER(fshop);
	S_BITVECTOR(tshop) = S_BITVECTOR(fshop);
	S_NOTRADE(tshop) = S_NOTRADE(fshop);
	S_SORT(tshop) = S_SORT(fshop);
	S_BUYPROFIT(tshop) = S_BUYPROFIT(fshop);
	S_SELLPROFIT(tshop) = S_SELLPROFIT(fshop);
	S_FUNC(tshop) = S_FUNC(fshop);

	/*
	 * Copy lists over.
	 */
	copy_list(&(S_ROOMS(tshop)), S_ROOMS(fshop));
	copy_list(&(S_PRODUCTS(tshop)), S_PRODUCTS(fshop));
	copy_type_list(&(tshop->type), fshop->type);

	/*
	 * Copy notification strings over.
	 */
	/* free_shop_strings(tshop); */
	S_NOITEM1(tshop) = str_dup(S_NOITEM1(fshop));
	S_NOITEM2(tshop) = str_dup(S_NOITEM2(fshop));
	S_NOCASH1(tshop) = str_dup(S_NOCASH1(fshop));
	S_NOCASH2(tshop) = str_dup(S_NOCASH2(fshop));
	S_NOBUY(tshop) = str_dup(S_NOBUY(fshop));
	S_BUY(tshop) = str_dup(S_BUY(fshop));
	S_SELL(tshop) = str_dup(S_SELL(fshop));
}

/*-------------------------------------------------------------------*/

/*
 * Copy a -1 terminated integer array list.
 */
void copy_list(sh_int **tlist, sh_int *flist)
{
	int num_items, i;

	if (*tlist)
		free(*tlist);

	/*
	 * Count number of entries.
	 */
	for (i = 0; flist[i] != -1; i++);
	num_items = i + 1;

	/*
	 * Make space for entries.
	 */
	CREATE(*tlist, sh_int, num_items);

	/*
	 * Copy entries over.
	 */
	for (i = 0; i < num_items; i++)
		(*tlist)[i] = flist[i];
}

/*-------------------------------------------------------------------*/

/*
 * Copy a -1 terminated (in the type field) shop_buy_data 
 * array list.
 */
void copy_type_list(struct shop_buy_data **tlist, struct shop_buy_data *flist)
{
	int num_items, i;

	if (*tlist)
		free_type_list(tlist);

	/*
	 * Count number of entries.
	 */
	for (i = 0; BUY_TYPE(flist[i]) != -1; i++);
	num_items = i + 1;

	/*
	 * Make space for entries.
	 */
	CREATE(*tlist, struct shop_buy_data, num_items);

	/*
	 * Copy entries over.
	 */
	for (i = 0; i < num_items; i++) {
		(*tlist)[i].type = flist[i].type;
		if (BUY_WORD(flist[i]))
			BUY_WORD((*tlist)[i]) = str_dup(BUY_WORD(flist[i]));
	}
}

/*-------------------------------------------------------------------*/

void remove_from_type_list(struct shop_buy_data **list, int num)
{
	int i, num_items;
	struct shop_buy_data *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i].type != -1; i++);

	if (num < 0 || num >= i)
		return;
	num_items = i;

	CREATE(nlist, struct shop_buy_data, num_items);

	for (i = 0; i < num_items; i++)
		nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

	free(BUY_WORD((*list)[num]));
	free(*list);
	*list = nlist;
}

/*-------------------------------------------------------------------*/

void add_to_type_list(struct shop_buy_data **list, struct shop_buy_data *newl)
{
	int i, num_items;
	struct shop_buy_data *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i].type != -1; i++);
	num_items = i;

	/*
	 * Make a new list and slot in the new entry.
	 */
	CREATE(nlist, struct shop_buy_data, num_items + 2);

	for (i = 0; i < num_items; i++)
		nlist[i] = (*list)[i];
	nlist[num_items] = *newl;
	nlist[num_items + 1].type = -1;

	/*
	 * Out with the old, in with the new.
	 */
	free(*list);
	*list = nlist;
}

/*-------------------------------------------------------------------*/

void add_to_int_list(sh_int **list, sh_int newi)
{
	sh_int i, num_items, *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i] != -1; i++);
	num_items = i;

	/*
	 * Make a new list and slot in the new entry.
	 */
	CREATE(nlist, sh_int, num_items + 2);

	for (i = 0; i < num_items; i++)
		nlist[i] = (*list)[i];
	nlist[num_items] = newi;
	nlist[num_items + 1] = -1;

	/*
	 * Out with the old, in with the new.
	 */
	free(*list);
	*list = nlist;
}

/*-------------------------------------------------------------------*/

void remove_from_int_list(sh_int **list, sh_int num)
{
	sh_int i, num_items, *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i] != -1; i++);

	if (num >= i || num < 0)
		return;
	num_items = i;

	CREATE(nlist, sh_int, num_items);

	for (i = 0; i < num_items; i++)
		nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

	free(*list);
	*list = nlist;
}

/*-------------------------------------------------------------------*/

/*
 * Free all the notice character strings in a shop structure.
 */
void free_shop_strings(struct shop_data *shop)
{
	if (S_NOITEM1(shop)) {
		free(S_NOITEM1(shop));
		S_NOITEM1(shop) = NULL;
	}
	if (S_NOITEM2(shop)) {
		free(S_NOITEM2(shop));
		S_NOITEM2(shop) = NULL;
	}
	if (S_NOCASH1(shop)) {
		free(S_NOCASH1(shop));
		S_NOCASH1(shop) = NULL;
	}
	if (S_NOCASH2(shop)) {
		free(S_NOCASH2(shop));
		S_NOCASH2(shop) = NULL;
	}
	if (S_NOBUY(shop)) {
		free(S_NOBUY(shop));
		S_NOBUY(shop) = NULL;
	}
	if (S_BUY(shop)) {
		free(S_BUY(shop));
		S_BUY(shop) = NULL;
	}
	if (S_SELL(shop)) {
		free(S_SELL(shop));
		S_SELL(shop) = NULL;
	}
}

/*-------------------------------------------------------------------*/

/*
 * Free a type list and all the strings it contains.
 */
void free_type_list(struct shop_buy_data **list)
{
	int i;

	for (i = 0; (*list)[i].type != -1; i++)
		if (BUY_WORD((*list)[i]))
			free(BUY_WORD((*list)[i]));

	free(*list);
	*list = NULL;
}

/*-------------------------------------------------------------------*/

/*
 * Free up the whole shop structure and it's content.
 */
void free_shop(struct shop_data *shop)
{
	free_shop_strings(shop);
	free_type_list(&(S_NAMELISTS(shop)));
	free(S_ROOMS(shop));
	free(S_PRODUCTS(shop));
	free(shop);
}

/*-------------------------------------------------------------------*/

/*
 * Ew, linear search, O(n)
 */
int	real_shop(int vshop_num)
{
	int rshop_num;

	for (rshop_num = 0; rshop_num <= top_shop; rshop_num++)
		if (SHOP_NUM(rshop_num) == vshop_num)
			return rshop_num;

	return NOWHERE;
}

/*-------------------------------------------------------------------*/

/*
 * Generic string modifier for shop keeper messages.
 */
void modify_string(char **str, char *new_s)
{
	char *pointer;

	/*
	 * Check the '%s' is present, if not, add it.
	 */
	if (*new_s != '%') {
		sprintf(buf, "%%s %s", new_s);
		pointer = buf;
	} else
		pointer = new_s;

	if (*str)
		free(*str);
	*str = str_dup(pointer);
}

/*-------------------------------------------------------------------*/

int	add_shop(struct shop_data *nshp)
{
	shop_rnum rshop;
	int found = 0;

	/*
	 * The shop already exists, just update it.
	 */
	if ((rshop = real_shop(S_NUM(nshp))) != NOWHERE) {
		free_shop_strings(&shop_index[rshop]);
		copy_shop(&shop_index[rshop], nshp);
		return rshop;
	}

	if (!shop_index) {
		CREATE(shop_index, struct shop_data, top_shop - top_shop_offset + 1);
	} else {
		top_shop++;
		RECREATE(shop_index, struct shop_data, top_shop - top_shop_offset + 1);
	}

	for (rshop = top_shop - top_shop_offset; rshop > 0; rshop--) {
		if (nshp->vnum > SHOP_NUM(rshop - 1)) {
			found = rshop;

			/* Make a "nofree" variant and remove these later. */
			shop_index[rshop].in_room = NULL;
			shop_index[rshop].producing = NULL;
			shop_index[rshop].type = NULL;

			copy_shop(&shop_index[rshop], nshp);
			break;
		}
		shop_index[rshop] = shop_index[rshop - 1];
	}

	if (!found) {
		/* Make a "nofree" variant and remove these later. */
		shop_index[rshop].in_room = NULL;
		shop_index[rshop].producing = NULL;
		shop_index[rshop].type = NULL;

		copy_shop(&shop_index[0], nshp);
	}

	return rshop;
}

/*-------------------------------------------------------------------*/

int	save_shops(zone_rnum zone_num)
{
	int i, j, rshop, vzone, top;
	char *no_such_item1=NULL;
	char *no_such_item2=NULL;
	char *do_not_buy=NULL;
	char *missing_cash1=NULL;
	char *missing_cash2=NULL;
	char *message_buy=NULL;
	char *message_sell=NULL;
	struct shop_data *shop;
	char *shop_replace = "REPLACE INTO %s ("
		"znum, "
		"vnum, "
		"no_such_item1, "
		"no_such_item2, "
		"do_not_buy, "
		"missing_cash1, "
		"missing_cash2, "
		"message_buy, "
		"message_sell, "
		"temper, "
		"bitvector, "
		"keeper, "
		"trade_with, "
		"shop_open1, "
		"shop_close1, "
		"shop_open2, "
		"shop_close2, "
		"buy_profit, "
		"sell_profit) "
		"VALUES (%d, %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, %llu, %d, %d, %d, %d, %d, %d, %1.2f, %1.2f);";
	char *product_insert = "INSERT INTO %s ("
		"znum, "
		"vnum, "
		"item) "
		"VALUES (%d, %d, %d);";
	char *keyword_insert = "INSERT INTO %s ("
		"znum, "
		"vnum, "
		"item, "
		"keyword) "
		"VALUES (%d, %d, %d, '%s');";
	char *room_insert = "INSERT INTO %s ("
		"znum, "
		"vnum, "
		"room) "
		"VALUES (%d, %d, %d);";

	if (zone_num < 0 || zone_num > top_of_zone_table) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "GenOLC: save_shops: Invalid real zone number %d. (0-%d)", zone_num, top_of_zone_table);
		return FALSE;
	}

	vzone = zone_table[zone_num].number;
	top = zone_table[zone_num].top;

	/* Delete all the zone's shops from the database. */
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_SHP_INDEX, vzone);

	/* Delete all the zone's shop products from the database. */
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_SHP_PRODUCTS, vzone);

	/* Delete all the zone's shop keywords from the database. */
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_SHP_KEYWORDS, vzone);

	/* Delete all the zone's shop rooms from the database. */
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_SHP_ROOMS, vzone);

	/*
	 * Search database for shops in this zone.
	 */
	for (i = genolc_zone_bottom(zone_num); i <= zone_table[zone_num].top; i++) {
		if ((rshop = real_shop(i)) != -1) {

			shop = shop_index + rshop;

			strcpy(buf, S_NOITEM1(shop) ? S_NOITEM1(shop) : "%s Ke?!");
			SQL_MALLOC(buf, no_such_item1);
			SQL_ESC(buf, no_such_item1);

			strcpy(buf, S_NOITEM2(shop) ? S_NOITEM2(shop) : "%s Ke?!");
			SQL_MALLOC(buf, no_such_item2);
			SQL_ESC(buf, no_such_item2);

			strcpy(buf, S_NOBUY(shop) ? S_NOBUY(shop) : "%s Ke?!");
			SQL_MALLOC(buf, do_not_buy);
			SQL_ESC(buf, do_not_buy);

			strcpy(buf, S_NOCASH1(shop) ? S_NOCASH1(shop) : "%s Ke?!");
			SQL_MALLOC(buf, missing_cash1);
			SQL_ESC(buf, missing_cash1);

			strcpy(buf, S_NOCASH2(shop) ? S_NOCASH2(shop) : "%s Ke?!");
			SQL_MALLOC(buf, missing_cash2);
			SQL_ESC(buf, missing_cash2);

			strcpy(buf, S_BUY(shop) ? S_BUY(shop) : "%s Ke?!");
			SQL_MALLOC(buf, message_buy);
			SQL_ESC(buf, message_buy);

			strcpy(buf, S_SELL(shop) ? S_SELL(shop) : "%s Ke?!");
			SQL_MALLOC(buf, message_sell);
			SQL_ESC(buf, message_sell);

			if (!(mysqlWrite(
				shop_replace,
				TABLE_SHP_INDEX,
				vzone,
				i,
				no_such_item1,
				no_such_item2,
				do_not_buy,
				missing_cash1,
				missing_cash2,
				message_buy,
				message_sell,
				S_BROKE_TEMPER(shop),
				S_BITVECTOR(shop),
				mob_index[S_KEEPER(shop)].vnum,
				S_NOTRADE(shop),
				S_OPEN1(shop),
				S_CLOSE1(shop),
				S_OPEN2(shop),
				S_CLOSE2(shop),
				S_BUYPROFIT(shop),
				S_SELLPROFIT(shop)
			))) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing shop #%d to database.", i);
				return FALSE;
			}

			SQL_FREE(no_such_item1);
			SQL_FREE(no_such_item2);
			SQL_FREE(do_not_buy);
			SQL_FREE(missing_cash1);
			SQL_FREE(missing_cash2);
			SQL_FREE(message_buy);
			SQL_FREE(message_sell);

			/*
			 * Save the products.
			 */
			for (j = 0; S_PRODUCT(shop, j) != -1; j++)
				if (!(mysqlWrite(product_insert, TABLE_SHP_PRODUCTS, vzone, i, obj_index[S_PRODUCT(shop, j)].vnum))) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing products for shop #%d to database.", i);
					return FALSE;
				}

			/*
			 * Save the buy types and namelists.
			 */
			for (j = 0; S_BUYTYPE(shop, j) != -1; j++)
				if (!(mysqlWrite(keyword_insert, TABLE_SHP_KEYWORDS, vzone, i, S_BUYTYPE(shop, j),
								S_BUYWORD(shop, j) ? S_BUYWORD(shop, j) : ""))) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing keywords for shop #%d to database.", i);
					return FALSE;
				}

			/*
			 * Save the rooms.
			 */
			for (j = 0; S_ROOM(shop, j) != -1; j++)
				if (!(mysqlWrite(room_insert, TABLE_SHP_ROOMS, vzone, i, S_ROOM(shop, j)))) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing rooms for shop #%d to database.", i);
					return FALSE;
				}
		}
	}

	return TRUE;
}
