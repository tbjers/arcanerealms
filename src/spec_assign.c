/* ************************************************************************
*		File: spec_assign.c                                 Part of CircleMUD *
*	 Usage: Functions to assign function pointers to objs/mobs/rooms        *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: spec_assign.c,v 1.5 2002/07/03 15:38:07 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "specset.h"

/* external globals */
extern int dts_are_dumps;
extern int mini_mud;

/* local functions */
void assign_dts(void);

/* functions to perform assignments */

void ASSIGNMOB(mob_vnum mob, SPECIAL(fname))
{
	mob_rnum rnum;

	if ((rnum = real_mobile(mob)) != NOBODY)
		mob_index[rnum].func = fname;
	else if (!mini_mud)
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Attempt to assign spec to non-existant mob #%d", mob);
}

void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname))
{
	obj_rnum rnum;

	if ((rnum = real_object(obj)) != NOTHING)
		obj_index[rnum].func = fname;
	else if (!mini_mud)
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Attempt to assign spec to non-existant obj #%d", obj);
}

void ASSIGNROOM(room_vnum room, SPECIAL(fname))
{
	room_rnum rnum;

	if ((rnum = real_room(room)) != NOWHERE)
		world[rnum].func = fname;
	else if (!mini_mud)
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Attempt to assign spec to non-existant room #%d", room);
}


/* ********************************************************************
*	 Assignments                                                        *
******************************************************************** */

/* assign death trap dumps */
void assign_dts(void)
{
	room_rnum i;

	if (dts_are_dumps)
		for (i = 0; i <= top_of_world; i++)
			if (ROOM_FLAGGED(i, ROOM_DEATH))
				world[i].func = dump;
}
