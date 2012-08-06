/************************************************************************
 * Generic OLC Library - Shops / genshp.h                          v1.0 *
 * Copyright 1996 by Harvey Gilpin                                      *
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)          *
 ************************************************************************/
/* $Id: genshp.h,v 1.2 2001/11/09 00:12:11 arcanere Exp $ */

void copy_shop(struct shop_data *tshop, struct shop_data *fshop);
void copy_list(sh_int **tlist, sh_int *flist);
void copy_type_list(struct shop_buy_data **tlist, struct shop_buy_data *flist);
void remove_from_type_list(struct shop_buy_data **list, int num);
void remove_from_int_list(sh_int **list, sh_int num);
void add_to_type_list(struct shop_buy_data **list, struct shop_buy_data *newl);
void add_to_int_list(sh_int **tlist, sh_int newi);
void free_shop_string(struct shop_data *shop);
void free_type_list(struct shop_buy_data **list);
void free_shop(struct shop_data *shop);
void free_shop_strings(struct shop_data *shop);
int	real_shop(int vshop);
void modify_string(char **str, char *newstr);
int	add_shop(struct shop_data *shop);
int	save_shops(zone_rnum zone_num);

/*
 * Handy macros.
 */
#define	S_NUM(i)		((i)->vnum)
#define	S_KEEPER(i)		((i)->keeper)
#define	S_OPEN1(i)		((i)->open1)
#define	S_CLOSE1(i)		((i)->close1)
#define	S_OPEN2(i)		((i)->open2)
#define	S_CLOSE2(i)		((i)->close2)
#define	S_BANK(i)		((i)->bankAccount)
#define	S_BROKE_TEMPER(i)	((i)->temper1)
#define	S_BITVECTOR(i)		((i)->bitvector)
#define	S_NOTRADE(i)		((i)->with_who)
#define	S_SORT(i)		((i)->lastsort)
#define	S_BUYPROFIT(i)		((i)->profit_buy)
#define	S_SELLPROFIT(i)		((i)->profit_sell)
#define	S_FUNC(i)		((i)->func)

#define	S_ROOMS(i)		((i)->in_room)
#define	S_PRODUCTS(i)		((i)->producing)
#define	S_NAMELISTS(i)		((i)->type)
#define	S_ROOM(i, num)		((i)->in_room[(num)])
#define	S_PRODUCT(i, num)	((i)->producing[(num)])
#define	S_BUYTYPE(i, num)	(BUY_TYPE((i)->type[(num)]))
#define	S_BUYWORD(i, num)	(BUY_WORD((i)->type[(num)]))

#define	S_NOITEM1(i)		((i)->no_such_item1)
#define	S_NOITEM2(i)		((i)->no_such_item2)
#define	S_NOCASH1(i)		((i)->missing_cash1)
#define	S_NOCASH2(i)		((i)->missing_cash2)
#define	S_NOBUY(i)		((i)->do_not_buy)
#define	S_BUY(i)		((i)->message_buy)
#define	S_SELL(i)		((i)->message_sell)
