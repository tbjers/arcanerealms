/************************************************************************
 * Generic OLC Library - General / genolc.h                        v1.0 *
 * Original author: Levork                                              *
 * Copyright 1996 by Harvey Gilpin                                      *
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)          *
 ************************************************************************/
/* $Id: genolc.h,v 1.7 2002/12/28 16:17:02 arcanere Exp $ */

#if !defined(_CIRCLEMUD) || !defined(CIRCLEMUD_VERSION)
# error "This version of GenOLC only supports CircleMUD 3.0 bpl15 or later."
#else
# if _CIRCLEMUD < CIRCLEMUD_VERSION(3,0,15)
#  error "This version of GenOLC only supports CircleMUD 3.0 bpl15 or later."
# endif
#endif

#define	STRING_TERMINATOR       '~'

#define	CONFIG_GENOLC_MOBPROG	0

int	genolc_checkstring(struct descriptor_data *d, const char *arg);
int	remove_from_save_list(zone_vnum, int type);
int	add_to_save_list(zone_vnum, int type);
void strip_cr(char *);
void do_show_save_list(struct char_data *);
int	save_all(void);
int	save_all_zone(zone_vnum zone);
char *str_udup(const char *);
void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from);
void free_ex_descriptions(struct extra_descr_data *head);

struct save_list_data {
	int zone;
	int type;
	struct save_list_data *next;
};

extern struct save_list_data *save_list;
extern int top_shop_offset;

/* save_list_data.type */
#define	SL_MOB	0
#define	SL_OBJ	1
#define	SL_SHP	2
#define	SL_WLD	3
#define	SL_ZON	4
#define	SL_ACT  5
#define	SL_HLP  6
#define	SL_QST  7
#define	SL_MAX	7

#define	ZCMD(zon, cmds)	zone_table[(zon)].cmd[(cmds)]

room_vnum genolc_zone_bottom(zone_rnum rznum);
room_vnum genolc_zonep_bottom(struct zone_data *zone);
