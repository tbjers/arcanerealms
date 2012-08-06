/* ************************************************************************
*  File: guild.h                                        Part of CircleMUD *
*	 Usage: header file for guild database handling                         *
*																																					*
*  Created: Sun May 14 2000 by Yohay Etsion (yohay_e@netvision.net.il)    *
*  This is part of Dragons Fang, which is based on CircleMUD              *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001-2002, Torgny Bjers.                                  *
*                                                                         *
*  Converted: 2002-06-25, by Torgny Bjers (artovil@arcanerealms.org)      *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
************************************************************************ */
/* $Id: guild.h,v 1.3 2002/06/28 21:03:51 arcanere Exp $ */

/* Guild Types */

#define GUILD_UNDEFINED			0
#define GUILD_FACTION				1
#define GUILD_ORGANIZATION	2
#define GUILD_GUILD					3
#define GUILD_GROUP					4
#define GUILD_OTHER					5

#define MAX_GUILD_TYPES			5
#define GUILD_CLUB					15

/* Guild flags */

#define GLD_EXCLUSIVE				(1 << 0)	/* A person can join only 1 of those */
#define GLD_IC							(1 << 1)	/* An IC guild, not OOC */
#define GLD_GLTITLE					(1 << 2)	/* The GL has an IMM title */
#define GLD_GUILDIESTITLE		(1 << 3)	/* Guildies have an IMM title (Ogier) */
#define GLD_GOSSIP					(1 << 4)	/* Guild has a gossip file */
#define GLD_NOGCHAN					(1 << 5)	/* Guild has no guild channel */
#define GLD_SECRETIVE				(1 << 6)	/* Secretive guild */
#define GLD_NOGLIST					(1 << 7)	/* Guildies can't use glist */
#define GLD_NOGWHO					(1 << 8)	/* Guildies can't use gwho */
#define GLD_NOGUILDWALK			(1 << 9)	/* No guildwalk for this guild */
#define GLD_NOBANK					(1 << 10)	/* Guild has no bank account */
#define GLD_DARK						(1 << 11)	/* Dark guild */

#define NUM_GUILD_FLAGS			12

/* Guildie's status flags */

#define STATUS_SEEKING			(1 << 0)	/* Seeker */
#define STATUS_AUTHORIZED		(1 << 1)	/* BG approved */
#define STATUS_MEMBER				(1 << 2)	/* Guilded */
#define STATUS_GL						(1 << 3)	/* Guildleader */
#define STATUS_CHAN_ON			(1 << 4)	/* Guildie has gchan on */
#define STATUS_SUBRANK			(1 << 5)	/* Guildie has a subrank */
#define STATUS_TAG					(1 << 6)	/* Guildie sees a tag in the gchannel */

/* Guildie's perm flags */

#define PERM_GUILD					(1 << 0)	/* Guildie can guild others */
#define PERM_WITHDRAW				(1 << 1)	/* Guildie can withdraw gold from the guild */
#define PERM_GOSSIP_WRITE		(1 << 2)	/* Guildie can add to the guild's gossip */
#define PERM_SPONSOR				(1 << 3)	/* Guildie can sponsor seekers */
#define PERM_AUTHORIZE			(1 << 4)	/* Guildie can approve seekers' bgs */
#define PERM_GSKILLSET			(1 << 5)	/* Guildie can gskillset */

/* Guild channel types */
#define GCHAN_UNDEFINED			0					/* No channel type defined */
#define GCHAN_FULL					1					/* Normal gchan_type */
#define GCHAN_SECRETIVE			2					/* Either Someone or Subrank */
#define GCHAN_SECRETRANKS		3					/* Someone|Subrank and Rank title */

#define MAX_GCHAN_TYPE			3

/* MACROs */

#define GLD_FLAGGED(gld, flag)				(IS_SET(gld->gflags, flag))
#define STATUS_FLAGGED(guildie, flag)	(IS_SET(guildie->status, flag))
#define PERM_FLAGGED(guildie, flag)		(IS_SET(guildie->perm, flag))
#define GLD_TYPE(gld)									(gld->type)
#define GET_CHAR_GUILDS(ch)						((ch)->char_specials.guilds_data)

void save_all_guilds(void);
void do_guild_channel(struct char_data *ch, struct char_guild_element *element, char *argument);
void do_guild_gossip(struct char_data *ch, struct char_guild_element *element, char *argument);
char *get_title(struct char_data *ch);
struct guild_info *add_guild(char *name);
int rem_guild(int id);

extern const char *guild_types[17];
extern const char *guild_flags[];
extern const char *guild_chantypes[];
