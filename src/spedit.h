/* ************************************************************************
*   File: spedit.h                              An extension to CircleMUD *
*  Usage: Spell Editor header file                                        *
*                                                                         *
*  This file by Torgny Bjers <artovil@arcanerealms.org>                   *
*  Copyright (C) 2002 by Arcane Realms MUD, www.arcanerealms.org.         *
************************************************************************ */
/* $Id: spedit.h,v 1.3 2002/03/30 12:02:01 arcanere Exp $ */

#ifndef	_SPEDIT_H_
#define	_SPEDIT_H_

#define	S_REAGENTS(i)						((i)->reagents)
#define	S_FOCUSES(i)						((i)->focuses)
#define	S_TRANSMUTERS(i)				((i)->transmuters)

#define	S_REG_TYPE(i, num)			(SPELL_OBJ_TYPE((i)->reagents[(num)]))
#define	S_REG_LOCATION(i, num)	(SPELL_OBJ_LOCATION((i)->reagents[(num)]))
#define	S_REG_EXTRACT(i, num)		(SPELL_OBJ_EXTRACT((i)->reagents[(num)]))

#define	S_FOC_TYPE(i, num)			(SPELL_OBJ_TYPE((i)->focuses[(num)]))
#define	S_FOC_LOCATION(i, num)	(SPELL_OBJ_LOCATION((i)->focuses[(num)]))
#define	S_FOC_EXTRACT(i, num)		(SPELL_OBJ_EXTRACT((i)->focuses[(num)]))

#define	S_TRA_TYPE(i, num)			(SPELL_OBJ_TYPE((i)->transmuters[(num)]))
#define	S_TRA_LOCATION(i, num)	(SPELL_OBJ_LOCATION((i)->transmuters[(num)]))
#define	S_TRA_EXTRACT(i, num)		(SPELL_OBJ_EXTRACT((i)->transmuters[(num)]))

void remove_from_obj_list(struct spell_obj_data **list, int num);
void add_to_obj_list(struct spell_obj_data **list, struct spell_obj_data *newl);

void spedit_disp_menu(struct descriptor_data * d);
void spedit_free_spell(struct spell_info_type *sptr);
void spedit_parse(struct descriptor_data *d, char *arg);
void spedit_setup_new(struct descriptor_data *d);
void dequeue_spell(int spellnum);
struct spell_info_type *enqueue_spell(void);

#endif
