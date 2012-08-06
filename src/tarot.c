/* ************************************************************************
*		File: tarot.c                                    A CircleMUD addition *
*	 Usage: Quest frontend for automatical assignment of Auto Quests.       *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 2001, Cathy Gore and Torgny Bjers.  All Rights reserved. *
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: tarot.c,v 1.6 2002/03/28 11:54:01 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "interpreter.h"
#include "tarot.h"

/* extern functions */
/* local functions */
char *get_suit_name(int suit);
int	draw_card(int qtype);

/* Structure definition for the tarot cards */
struct tarot tarot_cards[NUM_TAROT_CARDS] = {
	/* Major Arcana, Trumps */
	{ MAJOR_ARCANA, "The Fool"            , TRUMP, TAROT_ROOM_ENT },
	{ MAJOR_ARCANA, "The Magician"        , TRUMP, TAROT_OBJ_RETN },
	{ MAJOR_ARCANA, "The High Priestess"  , TRUMP, TAROT_ROOM_ENT },
	{ MAJOR_ARCANA, "The Empress"         , TRUMP, TAROT_MOB_SAVE },
	{ MAJOR_ARCANA, "The Emporer"         , TRUMP, TAROT_MOB_KILL },
	{ MAJOR_ARCANA, "The Hierophant"      , TRUMP, TAROT_OBJ_FIND },
	{ MAJOR_ARCANA, "The Lovers"          , TRUMP, TAROT_OBJ_RETN },
	{ MAJOR_ARCANA, "The Chariot"         , TRUMP, TAROT_MOB_FIND },
	{ MAJOR_ARCANA, "Justice"             , TRUMP, TAROT_MOB_SAVE },
	{ MAJOR_ARCANA, "The Hermit"          , TRUMP, TAROT_MOB_FIND },
	{ MAJOR_ARCANA, "The Wheel of Fortune", TRUMP, TAROT_OBJ_RETN },
	{ MAJOR_ARCANA, "Strength"            , TRUMP, TAROT_OBJ_FIND },
	{ MAJOR_ARCANA, "The Hanged Man"      , TRUMP, TAROT_ROOM_ENT },
	{ MAJOR_ARCANA, "Death"               , TRUMP, TAROT_MOB_KILL },
	{ MAJOR_ARCANA, "Temperance"          , TRUMP, TAROT_OBJ_FIND },
	{ MAJOR_ARCANA, "The Devil"           , TRUMP, TAROT_MOB_KILL },
	{ MAJOR_ARCANA, "The Tower"           , TRUMP, TAROT_MOB_KILL },
	{ MAJOR_ARCANA, "The Star"            , TRUMP, TAROT_MOB_SAVE },
	{ MAJOR_ARCANA, "The Moon"            , TRUMP, TAROT_MOB_FIND },
	{ MAJOR_ARCANA, "The Sun"             , TRUMP, TAROT_MOB_SAVE },
	{ MAJOR_ARCANA, "Judgement"           , TRUMP, TAROT_OBJ_RETN },
	{ MAJOR_ARCANA, "The World"           , TRUMP, TAROT_OBJ_RETN },

	/* Minor Arcana, Hearts */
	{ MINOR_ARCANA, "Ace"    , HEARTS   , TAROT_MOB_SAVE },
	{ MINOR_ARCANA, "Deuce"  , HEARTS   , TAROT_OBJ_FIND },
	{ MINOR_ARCANA, "Three"  , HEARTS   , TAROT_MOB_KILL },
	{ MINOR_ARCANA, "Four"   , HEARTS   , TAROT_ROOM_ENT },
	{ MINOR_ARCANA, "Five"   , HEARTS   , TAROT_OBJ_RETN },
	{ MINOR_ARCANA, "Six"    , HEARTS   , TAROT_MOB_FIND },
	{ MINOR_ARCANA, "Seven"  , HEARTS   , TAROT_ROOM_ENT },
	{ MINOR_ARCANA, "Eight"  , HEARTS   , TAROT_MOB_KILL },
	{ MINOR_ARCANA, "Nine"   , HEARTS   , TAROT_OBJ_FIND },
	{ MINOR_ARCANA, "Ten"    , HEARTS   , TAROT_MOB_SAVE },
	{ MINOR_ARCANA, "Page"   , HEARTS   , TAROT_ROOM_ENT },
	{ MINOR_ARCANA, "Knight" , HEARTS   , TAROT_OBJ_FIND },
	{ MINOR_ARCANA, "Queen"  , HEARTS   , TAROT_ROOM_ENT },
	{ MINOR_ARCANA, "King"   , HEARTS   , TAROT_MOB_FIND },

	/* Minor Arcana, Pentacles */
	{ MINOR_ARCANA, "Ace"    , PENTACLES, TAROT_MOB_SAVE },
	{ MINOR_ARCANA, "Deuce"  , PENTACLES, TAROT_MOB_KILL },
	{ MINOR_ARCANA, "Three"  , PENTACLES, TAROT_OBJ_FIND },
	{ MINOR_ARCANA, "Four"   , PENTACLES, TAROT_OBJ_RETN },
	{ MINOR_ARCANA, "Five"   , PENTACLES, TAROT_MOB_FIND },
	{ MINOR_ARCANA, "Six"    , PENTACLES, TAROT_ROOM_ENT },
	{ MINOR_ARCANA, "Seven"  , PENTACLES, TAROT_MOB_FIND },
	{ MINOR_ARCANA, "Eight"  , PENTACLES, TAROT_OBJ_FIND },
	{ MINOR_ARCANA, "Nine"   , PENTACLES, TAROT_MOB_KILL },
	{ MINOR_ARCANA, "Ten"    , PENTACLES, TAROT_MOB_FIND },
	{ MINOR_ARCANA, "Page"   , PENTACLES, TAROT_ROOM_ENT },
	{ MINOR_ARCANA, "Knight" , PENTACLES, TAROT_MOB_KILL },
	{ MINOR_ARCANA, "Queen"  , PENTACLES, TAROT_MOB_SAVE },
	{ MINOR_ARCANA, "King"   , PENTACLES, TAROT_OBJ_RETN },

	/* Minor Arcana, Swords */
	{ MINOR_ARCANA, "Ace"    , SWORDS   , TAROT_MOB_SAVE },
	{ MINOR_ARCANA, "Deuce"  , SWORDS   , TAROT_OBJ_RETN },
	{ MINOR_ARCANA, "Three"  , SWORDS   , TAROT_OBJ_FIND },
	{ MINOR_ARCANA, "Four"   , SWORDS   , TAROT_MOB_KILL },
	{ MINOR_ARCANA, "Five"   , SWORDS   , TAROT_OBJ_FIND },
	{ MINOR_ARCANA, "Six"    , SWORDS   , TAROT_MOB_FIND },
	{ MINOR_ARCANA, "Seven"  , SWORDS   , TAROT_MOB_KILL },
	{ MINOR_ARCANA, "Eight"  , SWORDS   , TAROT_OBJ_RETN },
	{ MINOR_ARCANA, "Nine"   , SWORDS   , TAROT_OBJ_FIND },
	{ MINOR_ARCANA, "Ten"    , SWORDS   , TAROT_MOB_SAVE },
	{ MINOR_ARCANA, "Page"   , SWORDS   , TAROT_ROOM_ENT },
	{ MINOR_ARCANA, "Knight" , SWORDS   , TAROT_OBJ_FIND },
	{ MINOR_ARCANA, "Queen"  , SWORDS   , TAROT_ROOM_ENT },
	{ MINOR_ARCANA, "King"   , SWORDS   , TAROT_MOB_FIND },

	/* Minor Arcana, Wands */
	{ MINOR_ARCANA, "Ace"    , WANDS    , TAROT_MOB_SAVE },
	{ MINOR_ARCANA, "Deuce"  , WANDS    , TAROT_ROOM_ENT },
	{ MINOR_ARCANA, "Three"  , WANDS    , TAROT_MOB_KILL },
	{ MINOR_ARCANA, "Four"   , WANDS    , TAROT_OBJ_RETN },
	{ MINOR_ARCANA, "Five"   , WANDS    , TAROT_MOB_FIND },
	{ MINOR_ARCANA, "Six"    , WANDS    , TAROT_OBJ_FIND },
	{ MINOR_ARCANA, "Seven"  , WANDS    , TAROT_MOB_SAVE },
	{ MINOR_ARCANA, "Eight"  , WANDS    , TAROT_MOB_FIND },
	{ MINOR_ARCANA, "Nine"   , WANDS    , TAROT_OBJ_RETN },
	{ MINOR_ARCANA, "Ten"    , WANDS    , TAROT_MOB_FIND },
	{ MINOR_ARCANA, "Page"   , WANDS    , TAROT_ROOM_ENT },
	{ MINOR_ARCANA, "Knight" , WANDS    , TAROT_MOB_KILL },
	{ MINOR_ARCANA, "Queen"  , WANDS    , TAROT_MOB_SAVE },
	{ MINOR_ARCANA, "King"   , WANDS    , TAROT_OBJ_RETN }
};

char *get_suit_name(int suit)
{
	char *suit_name=NULL;

	switch(suit) {
	case HEARTS: suit_name = str_dup("of Hearts"); break;
	case PENTACLES: suit_name = str_dup("of Pentacles"); break;
	case SWORDS: suit_name = str_dup("of Swords"); break;
	case WANDS: suit_name = str_dup("of Wands"); break;
	case TRUMP: suit_name = NULL; break;
	default: 
		mudlog("Unknown suit passed to get_suit_name.", BRF, RIGHTS_DEVELOPER, TRUE);
		break;
	}

	return(suit_name);
}

/* somewhere in the main function 
	qtype = number(1, NUM_TAROT_QUESTS) - 1;
	qcard1 = draw_card(qtype);
	qcard2 = draw_card(qtype);
	qcard3 = draw_card(qtype);
	
	we can add another thinger to the struct as a message... so that we can have
	sprintf(buf, "Hmmm... I see %s...", tarot_cards[qcard1].message);
	do_say(me, buf, 0, 0); or whatever it is
	or we can make a const char like
	const char *tq_messages[] = {
	"message 1",
	"message 2",
	"message 3", etc...
	"\n"
	};
	and change it to do_say(me, tq_messages[qcard1], 0, 0); blah blah blah...
*/

int	draw_card(int qtype)
{
	int card;

	do {
		card = number(0, NUM_TAROT_CARDS) - 1;
	} while (tarot_cards[card].questtype != qtype);

	return card;
}
