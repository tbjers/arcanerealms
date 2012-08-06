/* ************************************************************************
*  File: guild_parser.h                                 Part of CircleMUD *
*	 Usage: header file for guild database handling                         *
*																																					*
*  Created: Sat May 13 2000 by Yohay Etsion (yohay_e@netvision.net.il)    *
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
/* $Id: guild_parser.h,v 1.1 2002/06/26 14:37:13 arcanere Exp $ */

/* Struct defines */

struct rank_info {
	char *name;
	long num;
	struct rank_info *next;
};

struct sponsorer_info {
	long idnum;
	char *name;
	struct sponsorer_info *next;
};

struct guildie_info {
	char *name;
	long idnum;
	long rank_num;
	struct rank_info *rank;
	char *subrank;
	long perm;
	long status;
	long deposited;
	long withdrew;
	struct char_data *ch;
	struct sponsorer_info *sponsorers;
	struct gequip_info *gequipsent;
	struct guildie_info *next;
};

struct gskill_info {
	int skill;
	int maximum_set;
	struct gskill_info *next;
};

struct gequip_info {
	int vnum;
	struct gequip_info *next;
};

struct gzone_info {
	int zone;
	struct gzone_info *next;
};

struct ghelp_info {
	char *keyword;
	char *entry;
	struct ghelp_info *next;
};

struct guild_info {
	int id;
	int type;
	char *name;
	char *description;
	char *requirements;
	char *gossip;
	char *gl_title;
	char *guildie_titles;
	char *guild_filename;
	char *gossip_name;
	char *gchan_name;
	char *gchan_color;
	int gchan_type;
	struct rank_info *ranks;
	struct guildie_info *guildies;
	struct gskill_info *gskills;
	struct gzone_info *gzones;
	struct gequip_info *gequip;
	struct ghelp_info *ghelp;
	long gflags;
	int guildwalk_room;
	int gold;
	struct guild_info *next;
};

struct char_guild_element {
	struct guild_info *guild;
	struct guildie_info *guildie;
	struct char_guild_element *next;
};
