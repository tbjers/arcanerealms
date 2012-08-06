/* *************************************************************************
*  File: roleplay.h                               Extension to CircleMUD   *
*  Usage: Header file for roleplay-specific code                          *
*                                                                          *
*  This file made exclusively for Arcane Realms MUD by Torgny Bjers.       *
*  Copyright (C) 1999-2002, Torgny Bjers, and Catherine Gore.              *
*                                                                          *
*	 All rights reserved.  See license.doc for complete information.         *
*                                                                          *
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University  *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
************************************************************************** */
/* $Id: roleplay.h,v 1.6 2003/01/13 01:43:44 arcanere Exp $ */

#define RPXP_SAY			0.01
#define RPXP_EMOTE		0.06
#define RPXP_PEMOTE		0.07
#define RPXP_CEMOTE		0.05
#define RPXP_POSE			0.07

#define RPXP_CAP			1000000

/* local functions in roleplay.c */
bool rpxp_reward (struct char_data *ch, unsigned int multiplier);
void add_to_rplog(struct char_data *ch, char *emote_text);
void parse_emote(const char *input, char *output, const char color, int gecho);
int parse_pemote(char *argument, char *vict_arg, char *room_arg);
void tog_rp(struct char_data *ch);
void free_doing(struct char_data *ch);
