/************************************************************************
 * Generic OLC Library - Houses / genhouse.h                       v1.0 *
 * Copyright 1996 by Harvey Gilpin                                      *
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)          *
 * Copyright 2002-2003 by Torgny Bjers (artovil@arcanerealms.org)       *
 ************************************************************************/
/* $Id: genhouse.h,v 1.3 2002/12/28 16:17:02 arcanere Exp $ */

void copy_house(struct house_data *thouse, struct house_data *fhouse);
void copy_house_list(long **tlist, long *flist);
void add_to_house_list(long **list, long newi);
void remove_from_house_list(long **list, long num);
void free_house(struct house_data *house);
int	real_house(int vhouse_num);
int	add_house(struct house_data *nhouse);
int	save_houses(void);
void delete_house(struct descriptor_data *d, house_vnum vnum);

/*
 * Handy macros.
 */
#define	H_NUM(i)					((i)->vnum)
#define	H_ATRIUM(i)				((i)->atrium)
#define	H_EXIT(i)					((i)->exit_num)
#define	H_BUILT(i)				((i)->built_on)
#define	H_USED(i)					((i)->last_used)
#define	H_PAID(i)					((i)->last_payment)
#define	H_PRUNE_SAFE(i)		((i)->prune_safe)
#define	H_MODE(i)					((i)->mode)
#define	H_OWNER(i)				((i)->owner)

#define	H_COST(i)					((i)->cost)
#define	H_MAX_SECURE(i)		((i)->max_secure)
#define	H_MAX_LOCKED(i)		((i)->max_locked)

#define	H_ROOMS(i)				((i)->rooms)
#define	H_ROOM(i, num)		((i)->rooms[(num)])

#define	H_COWNERS(i)			((i)->cowners)
#define	H_COWNER(i, num)	((i)->cowners[(num)])

#define	H_GUESTS(i)				((i)->guests)
#define	H_GUEST(i, num)		((i)->guests[(num)])
