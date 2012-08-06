/************************************************************************
 * OasisOLC - Shops / sedit.c                                      v2.0 *
 * Copyright 1996 Harvey Gilpin                                         *
 * Copyright 1997-1999 George Greer (greerga@circlemud.org)             *
 ************************************************************************/
/* $Id: sedit.c,v 1.13 2002/11/16 17:23:09 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genshp.h"
#include "oasis.h"

/*-------------------------------------------------------------------*/

/*
 * External variable declarations.
 */
extern struct shop_data *shop_index;
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern struct room_data *world;
extern struct zone_data *zone_table;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern const char *trade_letters[];
extern const char *shop_bits[];
extern const char *item_types[];

/*-------------------------------------------------------------------*/

/*
 * External functions.
 */
SPECIAL(shop_keeper);

/*
 * Should check more things.
 */
void sedit_save_internally(struct descriptor_data *d)
{
	OLC_SHOP(d)->vnum = OLC_NUM(d);
	add_shop(OLC_SHOP(d));
}

void sedit_save_to_disk(int num)
{
	save_shops(num);
}

/*-------------------------------------------------------------------*\
	utility functions 
\*-------------------------------------------------------------------*/

void sedit_setup_new(struct descriptor_data *d)
{
	struct shop_data *shop;

	/*
	 * Allocate a scratch shop structure.
	 */
	CREATE(shop, struct shop_data, 1);

	/*
	 * Fill in some default values.
	 */
	S_KEEPER(shop) = -1;
	S_CLOSE1(shop) = 28;
	S_BUYPROFIT(shop) = 1.0;
	S_SELLPROFIT(shop) = 1.0;
	/*
	 * Add a spice of default strings.
	 */
	S_NOITEM1(shop) = str_dup("%s Sorry, I don't stock that item.");
	S_NOITEM2(shop) = str_dup("%s You don't seem to have that.");
	S_NOCASH1(shop) = str_dup("%s I can't afford that!");
	S_NOCASH2(shop) = str_dup("%s You are too poor!");
	S_NOBUY(shop) = str_dup("%s I don't trade in such items.");
	S_BUY(shop) = str_dup("%s That'll be %d coins, thanks.");
	S_SELL(shop) = str_dup("%s I'll give you %d coins for that.");
	/*
	 * Stir the lists lightly.
	 */
	CREATE(S_PRODUCTS(shop), obj_vnum, 1);

	S_PRODUCT(shop, 0) = -1;
	CREATE(S_ROOMS(shop), room_rnum, 1);

	S_ROOM(shop, 0) = -1;
	CREATE(S_NAMELISTS(shop), struct shop_buy_data, 1);

	S_BUYTYPE(shop, 0) = -1;

	/*
	 * Presto! A shop.
	 */
	OLC_SHOP(d) = shop;
	sedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void sedit_setup_existing(struct descriptor_data *d, int rshop_num)
{
	/*
	 * Create a scratch shop structure.
	 */
	CREATE(OLC_SHOP(d), struct shop_data, 1);

	copy_shop(OLC_SHOP(d), shop_index + rshop_num);
	sedit_disp_menu(d);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

void sedit_products_menu(struct descriptor_data *d)
{
	struct shop_data *shop;
	int i;

	shop = OLC_SHOP(d);
	get_char_colors(d->character);

	clear_screen(d);
	write_to_output(d, TRUE, "##     VNUM     Product\r\n");
	for (i = 0; S_PRODUCT(shop, i) != -1; i++)
		write_to_output(d, TRUE, "%2d - [%s%5d%s] - %s%s%s\r\n", i,
			cyn, obj_index[S_PRODUCT(shop, i)].vnum, nrm,
			yel, obj_proto[S_PRODUCT(shop, i)].short_description, nrm);
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new product.\r\n"
		"%sD%s) Delete a product.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = SEDIT_PRODUCTS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_compact_rooms_menu(struct descriptor_data *d)
{
	struct shop_data *shop;
	int i, count = 0;

	shop = OLC_SHOP(d);
	get_char_colors(d->character);

	clear_screen(d);
	for (i = 0; S_ROOM(shop, i) != -1; i++)
		write_to_output(d, TRUE, "%2d - [%s%5d%s]  | %s", i, cyn, S_ROOM(shop, i), nrm,
			!(++count % 5) ? "\r\n" : "");
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new room.\r\n"
		"%sD%s) Delete a room.\r\n"
		"%sL%s) Long display.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_rooms_menu(struct descriptor_data *d)
{
	struct shop_data *shop;
	int i;

	shop = OLC_SHOP(d);
	get_char_colors(d->character);

	clear_screen(d);
	write_to_output(d, TRUE, "##     VNUM     Room\r\n\r\n");
	for (i = 0; S_ROOM(shop, i) != -1; i++)
		write_to_output(d, TRUE, "%2d - [%s%5d%s] - %s%s%s\r\n", i, cyn, S_ROOM(shop, i), nrm,
			yel, world[real_room(S_ROOM(shop, i))].name, nrm);
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new room.\r\n"
		"%sD%s) Delete a room.\r\n"
		"%sC%s) Compact Display.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm);

	OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_namelist_menu(struct descriptor_data *d)
{
	struct shop_data *shop;
	int i;

	shop = OLC_SHOP(d);
	get_char_colors(d->character);

	clear_screen(d);
	write_to_output(d, TRUE, "##              Type   Namelist\r\n\r\n");
	for (i = 0; S_BUYTYPE(shop, i) != -1; i++)
		write_to_output(d, TRUE, "%2d - %s%15s%s - %s%s%s\r\n", i, cyn,
			item_types[S_BUYTYPE(shop, i)], nrm, yel,
			S_BUYWORD(shop, i) ? S_BUYWORD(shop, i) : "<None>", nrm);
	write_to_output(d, TRUE, "\r\n"
		"%sA%s) Add a new entry.\r\n"
		"%sD%s) Delete an entry.\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ", grn, nrm, grn, nrm, grn, nrm);
	OLC_MODE(d) = SEDIT_NAMELIST_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_shop_flags_menu(struct descriptor_data *d)
{
	int i, count = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (i = 0; i < NUM_SHOP_FLAGS; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s   %s", grn, i + 1, nrm, shop_bits[i],
			!(++count % 2) ? "\r\n" : "");
	sprintbit(S_BITVECTOR(OLC_SHOP(d)), shop_bits, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nCurrent Shop Flags : %s%s%s\r\nEnter choice : ",
		cyn, buf1, nrm);
	OLC_MODE(d) = SEDIT_SHOP_FLAGS;
}

/*-------------------------------------------------------------------*/

void sedit_no_trade_menu(struct descriptor_data *d)
{
	int i, count = 0;

	get_char_colors(d->character);
	clear_screen(d);
	for (i = 0; i < NUM_TRADERS; i++)
		write_to_output(d, TRUE, "%s%2d%s) %-20.20s   %s", grn, i + 1, nrm, trade_letters[i],
			!(++count % 2) ? "\r\n" : "");
	sprintbit(S_NOTRADE(OLC_SHOP(d)), trade_letters, buf1, sizeof(buf1));
	write_to_output(d, TRUE, "\r\nCurrently won't trade with: %s%s%s\r\n"
		"Enter choice : ", cyn, buf1, nrm);
	OLC_MODE(d) = SEDIT_NOTRADE;
}

/*-------------------------------------------------------------------*/

void sedit_types_menu(struct descriptor_data *d)
{
	struct shop_data *shop;
	int i, count = 0;

	shop = OLC_SHOP(d);
	get_char_colors(d->character);

	clear_screen(d);
	for (i = 0; i < NUM_ITEM_TYPES; i++)
		write_to_output(d, TRUE, "%s%2d%s) %s%-20s%s  %s", grn, i, nrm, cyn, item_types[i],
			nrm, !(++count % 3) ? "\r\n" : "");
	write_to_output(d, TRUE, "%sEnter choice : ", nrm);
	OLC_MODE(d) = SEDIT_TYPE_MENU;
}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void sedit_disp_menu(struct descriptor_data *d)
{
	struct shop_data *shop;

	shop = OLC_SHOP(d);
	get_char_colors(d->character);

	clear_screen(d);
	sprintbit(S_NOTRADE(shop), trade_letters, buf1, sizeof(buf1));
	sprintbit(S_BITVECTOR(shop), shop_bits, buf2, sizeof(buf2));
	write_to_output(d, TRUE,
		"-- Shop Number : [%s%d%s]\r\n"
		"%s0%s) Keeper      : [%s%d%s] %s%s\r\n"
	 "%s1%s) Open 1      : %s%4d%s          %s2%s) Close 1     : %s%4d\r\n"
	 "%s3%s) Open 2      : %s%4d%s          %s4%s) Close 2     : %s%4d\r\n"
		"%s5%s) Sell rate   : %s%1.2f%s          %s6%s) Buy rate    : %s%1.2f\r\n"
		"%s7%s) Keeper no item : %s\\c98%s\\c99\r\n"
		"%s8%s) Player no item : %s\\c98%s\\c99\r\n"
		"%s9%s) Keeper no cash : %s\\c98%s\\c99\r\n"
		"%sA%s) Player no cash : %s\\c98%s\\c99\r\n"
		"%sB%s) Keeper no buy  : %s\\c98%s\\c99\r\n"
		"%sC%s) Buy sucess     : %s\\c98%s\\c99\r\n"
		"%sD%s) Sell sucess    : %s\\c98%s\\c99\r\n"
		"%sE%s) No Trade With  : %s\\c98%s\\c99\r\n"
		"%sF%s) Shop flags     : %s\\c98%s\\c99\r\n"
		"%sR%s) Rooms Menu\r\n"
		"%sP%s) Products Menu\r\n"
		"%sT%s) Accept Types Menu\r\n"
		"%sQ%s) Quit\r\n"
		"Enter Choice : ",

		cyn, OLC_NUM(d), nrm,
		grn, nrm, cyn, S_KEEPER(shop) == -1 ? -1 : mob_index[S_KEEPER(shop)].vnum,
		nrm, yel, S_KEEPER(shop) == -1 ? "None" : mob_proto[S_KEEPER(shop)].player.short_descr,
		grn, nrm, cyn, S_OPEN1(shop), nrm,
		grn, nrm, cyn, S_CLOSE1(shop),
		grn, nrm, cyn, S_OPEN2(shop), nrm,
		grn, nrm, cyn, S_CLOSE2(shop),
		grn, nrm, cyn, S_BUYPROFIT(shop), nrm,
		grn, nrm, cyn, S_SELLPROFIT(shop),
		grn, nrm, yel, S_NOITEM1(shop),
		grn, nrm, yel, S_NOITEM2(shop),
		grn, nrm, yel, S_NOCASH1(shop),
		grn, nrm, yel, S_NOCASH2(shop),
		grn, nrm, yel, S_NOBUY(shop),
		grn, nrm, yel, S_BUY(shop),
		grn, nrm, yel, S_SELL(shop),
		grn, nrm, cyn, buf1,
		grn, nrm, cyn, buf2,
		grn, nrm, grn, nrm, grn, nrm, grn, nrm
	);

	OLC_MODE(d) = SEDIT_MAIN_MENU;
}

/**************************************************************************
	The GARGANTUAN event handler
 **************************************************************************/

void sedit_parse(struct descriptor_data *d, char *arg)
{
	int i;

	if (OLC_MODE(d) > SEDIT_NUMERICAL_RESPONSE) {
		if (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))) {
			write_to_output(d, TRUE, "Field must be numerical, try again : ");
			return;
		}
	}
	switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
	case SEDIT_CONFIRM_SAVESTRING:
		switch (*arg) {
		case 'y':
		case 'Y':
			write_to_output(d, TRUE, "Saving shop to memory.\r\n");
			sedit_save_internally(d);
			sedit_save_to_disk(OLC_ZNUM(d));
			extended_mudlog(BRF, SYSL_OLC, TRUE, "%s edits shop %d.", GET_NAME(d->character), OLC_NUM(d));
			cleanup_olc(d, CLEANUP_STRUCTS);
			return;
		case 'n':
		case 'N':
			cleanup_olc(d, CLEANUP_ALL);
			return;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\nDo you wish to save the shop? : ");
			return;
		}
		break;

/*-------------------------------------------------------------------*/
	case SEDIT_MAIN_MENU:
		i = 0;
		switch (*arg) {
		case 'q':
		case 'Q':
			if (OLC_VAL(d)) {		/* Anything been changed? */
				write_to_output(d, TRUE, "Do you wish to save the changes to the shop? (y/n) : ");
				OLC_MODE(d) = SEDIT_CONFIRM_SAVESTRING;
			} else
				cleanup_olc(d, CLEANUP_ALL);
			return;
		case '0':
			OLC_MODE(d) = SEDIT_KEEPER;
			write_to_output(d, TRUE, "Enter vnum number of shop keeper : ");
			return;
		case '1':
			OLC_MODE(d) = SEDIT_OPEN1;
			i++;
			break;
		case '2':
			OLC_MODE(d) = SEDIT_CLOSE1;
			i++;
			break;
		case '3':
			OLC_MODE(d) = SEDIT_OPEN2;
			i++;
			break;
		case '4':
			OLC_MODE(d) = SEDIT_CLOSE2;
			i++;
			break;
		case '5':
			OLC_MODE(d) = SEDIT_BUY_PROFIT;
			i++;
			break;
		case '6':
			OLC_MODE(d) = SEDIT_SELL_PROFIT;
			i++;
			break;
		case '7':
			OLC_MODE(d) = SEDIT_NOITEM1;
			i--;
			break;
		case '8':
			OLC_MODE(d) = SEDIT_NOITEM2;
			i--;
			break;
		case '9':
			OLC_MODE(d) = SEDIT_NOCASH1;
			i--;
			break;
		case 'a':
		case 'A':
			OLC_MODE(d) = SEDIT_NOCASH2;
			i--;
			break;
		case 'b':
		case 'B':
			OLC_MODE(d) = SEDIT_NOBUY;
			i--;
			break;
		case 'c':
		case 'C':
			OLC_MODE(d) = SEDIT_BUY;
			i--;
			break;
		case 'd':
		case 'D':
			OLC_MODE(d) = SEDIT_SELL;
			i--;
			break;
		case 'e':
		case 'E':
			sedit_no_trade_menu(d);
			return;
		case 'f':
		case 'F':
			sedit_shop_flags_menu(d);
			return;
		case 'r':
		case 'R':
			sedit_rooms_menu(d);
			return;
		case 'p':
		case 'P':
			sedit_products_menu(d);
			return;
		case 't':
		case 'T':
			sedit_namelist_menu(d);
			return;
		default:
			sedit_disp_menu(d);
			return;
		}

		if (i == 0)
			break;
		else if (i == 1)
			write_to_output(d, TRUE, "\r\nEnter new value : ");
		else if (i == -1)
			write_to_output(d, TRUE, "\r\nEnter new text :\r\n] ");
		else
			write_to_output(d, TRUE, "Oops...\r\n");
		return;
/*-------------------------------------------------------------------*/
	case SEDIT_NAMELIST_MENU:
		switch (*arg) {
		case 'a':
		case 'A':
			sedit_types_menu(d);
			return;
		case 'd':
		case 'D':
			write_to_output(d, TRUE, "\r\nDelete which entry? : ");
			OLC_MODE(d) = SEDIT_DELETE_TYPE;
			return;
		case 'q':
		case 'Q':
			break;
		}
		break;
/*-------------------------------------------------------------------*/
	case SEDIT_PRODUCTS_MENU:
		switch (*arg) {
		case 'a':
		case 'A':
			write_to_output(d, TRUE, "\r\nEnter new product vnum number : ");
			OLC_MODE(d) = SEDIT_NEW_PRODUCT;
			return;
		case 'd':
		case 'D':
			write_to_output(d, TRUE, "\r\nDelete which product? : ");
			OLC_MODE(d) = SEDIT_DELETE_PRODUCT;
			return;
		case 'q':
		case 'Q':
			break;
		}
		break;
/*-------------------------------------------------------------------*/
	case SEDIT_ROOMS_MENU:
		switch (*arg) {
		case 'a':
		case 'A':
			write_to_output(d, TRUE, "\r\nEnter new room vnum number : ");
			OLC_MODE(d) = SEDIT_NEW_ROOM;
			return;
		case 'c':
		case 'C':
			sedit_compact_rooms_menu(d);
			return;
		case 'l':
		case 'L':
			sedit_rooms_menu(d);
			return;
		case 'd':
		case 'D':
			write_to_output(d, TRUE, "\r\nDelete which room? : ");
			OLC_MODE(d) = SEDIT_DELETE_ROOM;
			return;
		case 'q':
		case 'Q':
			break;
		}
		break;
/*-------------------------------------------------------------------*/
		/*
		 * String edits.
		 */
	case SEDIT_NOITEM1:
		if (genolc_checkstring(d, arg))
			modify_string(&S_NOITEM1(OLC_SHOP(d)), arg);
		break;
	case SEDIT_NOITEM2:
		if (genolc_checkstring(d, arg))
			modify_string(&S_NOITEM2(OLC_SHOP(d)), arg);
		break;
	case SEDIT_NOCASH1:
		if (genolc_checkstring(d, arg))
			modify_string(&S_NOCASH1(OLC_SHOP(d)), arg);
		break;
	case SEDIT_NOCASH2:
		if (genolc_checkstring(d, arg))
			modify_string(&S_NOCASH2(OLC_SHOP(d)), arg);
		break;
	case SEDIT_NOBUY:
		if (genolc_checkstring(d, arg))
			modify_string(&S_NOBUY(OLC_SHOP(d)), arg);
		break;
	case SEDIT_BUY:
		if (genolc_checkstring(d, arg))
			modify_string(&S_BUY(OLC_SHOP(d)), arg);
		break;
	case SEDIT_SELL:
		if (genolc_checkstring(d, arg))
			modify_string(&S_SELL(OLC_SHOP(d)), arg);
		break;
	case SEDIT_NAMELIST:
		if (genolc_checkstring(d, arg)) {
			struct shop_buy_data new_entry;

			BUY_TYPE(new_entry) = OLC_VAL(d);
			BUY_WORD(new_entry) = str_dup(arg);
			add_to_type_list(&(S_NAMELISTS(OLC_SHOP(d))), &new_entry);
		}
		sedit_namelist_menu(d);
		return;

/*-------------------------------------------------------------------*/
		/*
		 * Numerical responses.
		 */
	case SEDIT_KEEPER:
		i = atoi(arg);
		if ((i = atoi(arg)) != -1)
			if ((i = real_mobile(i)) < 0) {
	write_to_output(d, TRUE, "That mobile does not exist, try again : ");
	return;
			}
		S_KEEPER(OLC_SHOP(d)) = i;
		if (i == -1)
			break;
		/*
		 * Fiddle with special procs.
		 */
		S_FUNC(OLC_SHOP(d)) = mob_index[i].func;
		mob_index[i].func = shop_keeper;
		break;
	case SEDIT_OPEN1:
		S_OPEN1(OLC_SHOP(d)) = LIMIT(atoi(arg), 0, 28);
		break;
	case SEDIT_OPEN2:
		S_OPEN2(OLC_SHOP(d)) = LIMIT(atoi(arg), 0, 28);
		break;
	case SEDIT_CLOSE1:
		S_CLOSE1(OLC_SHOP(d)) = LIMIT(atoi(arg), 0, 28);
		break;
	case SEDIT_CLOSE2:
		S_CLOSE2(OLC_SHOP(d)) = LIMIT(atoi(arg), 0, 28);
		break;
	case SEDIT_BUY_PROFIT:
		sscanf(arg, "%f", &S_BUYPROFIT(OLC_SHOP(d)));
		break;
	case SEDIT_SELL_PROFIT:
		sscanf(arg, "%f", &S_SELLPROFIT(OLC_SHOP(d)));
		break;
	case SEDIT_TYPE_MENU:
		OLC_VAL(d) = LIMIT(atoi(arg), 0, NUM_ITEM_TYPES - 1);
		write_to_output(d, TRUE, "Enter namelist (return for none) :-\r\n] ");
		OLC_MODE(d) = SEDIT_NAMELIST;
		return;
	case SEDIT_DELETE_TYPE:
		remove_from_type_list(&(S_NAMELISTS(OLC_SHOP(d))), atoi(arg));
		sedit_namelist_menu(d);
		return;
	case SEDIT_NEW_PRODUCT:
		if ((i = atoi(arg)) != -1)
			if ((i = real_object(i)) == -1) {
				write_to_output(d, TRUE, "That object does not exist, try again : ");
				return;
			}
		if (i > 0)
			add_to_int_list(&(S_PRODUCTS(OLC_SHOP(d))), i);
		sedit_products_menu(d);
		return;
	case SEDIT_DELETE_PRODUCT:
		remove_from_int_list(&(S_PRODUCTS(OLC_SHOP(d))), atoi(arg));
		sedit_products_menu(d);
		return;
	case SEDIT_NEW_ROOM:
		if ((i = atoi(arg)) != -1)
			if ((i = real_room(i)) < 0) {
	write_to_output(d, TRUE, "That room does not exist, try again : ");
	return;
			}
		if (i >= 0)
			add_to_int_list(&(S_ROOMS(OLC_SHOP(d))), atoi(arg));
		sedit_rooms_menu(d);
		return;
	case SEDIT_DELETE_ROOM:
		remove_from_int_list(&(S_ROOMS(OLC_SHOP(d))), atoi(arg));
		sedit_rooms_menu(d);
		return;
	case SEDIT_SHOP_FLAGS:
		if ((i = LIMIT(atoi(arg), 0, NUM_SHOP_FLAGS)) > 0) {
			TOGGLE_BIT(S_BITVECTOR(OLC_SHOP(d)), 1ULL << (i - 1));
			sedit_shop_flags_menu(d);
			return;
		}
		break;
	case SEDIT_NOTRADE:
		if ((i = LIMIT(atoi(arg), 0, NUM_TRADERS)) > 0) {
			TOGGLE_BIT(S_NOTRADE(OLC_SHOP(d)), 1ULL << (i - 1));
			sedit_no_trade_menu(d);
			return;
		}
		break;

/*-------------------------------------------------------------------*/
	default:
		/*
		 * We should never get here.
		 */
		cleanup_olc(d, CLEANUP_ALL);
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: sedit_parse(): Reached default case!");
		write_to_output(d, TRUE, "Oops...\r\n");
		break;
	}

/*-------------------------------------------------------------------*/

/*
 * END OF CASE 
 * If we get here, we have probably changed something, and now want to
 * return to main menu.  Use OLC_VAL as a 'has changed' flag.
 */
	OLC_VAL(d) = 1;
	sedit_disp_menu(d);
}
