/* ************************************************************************
*		File: regio.c                                    A CircleMUD addition *
*	 Usage: Magical regio handling for Arcane Realms                        *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 2001, Torgny Bjers.  All Rights reserved.                *
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: regio.c,v 1.4 2002/06/25 14:22:42 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "genolc.h"
#include "genwld.h"
#include "genzon.h"
#include "dg_scripts.h"
#include "loadrooms.h"

