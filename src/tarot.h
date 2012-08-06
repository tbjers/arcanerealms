/* ************************************************************************
*		File: tarot.h                                    A CircleMUD addition *
*	 Usage: Defines and structures for the Tarot system.                    *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 2001, Cathy Gore and Torgny Bjers.  All Rights reserved. *
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: tarot.h,v 1.2 2001/11/09 00:12:11 arcanere Exp $ */

/* Arcana */
#define	MAJOR_ARCANA 0
#define	MINOR_ARCANA 1

/* Suits */
#define	TRUMP     0
#define	HEARTS    1
#define	PENTACLES 2
#define	SWORDS    3
#define	WANDS     4

/* Quest type */
#define	TAROT_UNDEFINED  -1  /* Undefined quest type */
#define	TAROT_OBJ_FIND   0  /* Find an object    */
#define	TAROT_ROOM_ENT   1  /* Enter a room      */
#define	TAROT_MOB_FIND   2  /* Find a mob        */
#define	TAROT_MOB_KILL   3  /* Kill a mob        */
#define	TAROT_MOB_SAVE   4  /* Save a mob        */
#define	TAROT_OBJ_RETN   5  /* Return and object */

#define	NUM_TAROT_QUESTS   NUM_AQ_TYPES
#define	NUM_TAROT_CARDS    78

/* Structure for the tarot cards 
 * { Arcana, "Card Name", Suit, Quest Relation }
 */

struct tarot {
	int arcana;
	char *name;
	int suit;
	int questtype;
};
