/* ************************************************************************
 * File: skills.c                                       Part of CircleMUD *
 * Usage: New skill file for use with RP style of Arcane Realms.          *
 * The main array of skills, as well as functions to go along with it.    *
 *                                                                        *
 * Copyright (C) 2001, Arcane Realms                                      *
 *********************************************************************** */
/* $Id: skills.c,v 1.88 2004/05/04 17:19:07 cheron Exp $ */

/*
 *  ARCANE REALMS SKILL TREE version 1.0
 *  Partly based on the Circe Roleplaying Game, which has been
 *  released under the GNU General Documentation License.
 *  Coded by Cathy Gore <cheron@arcanerealms.org> and
 *  Torgny Bjers <artovil@arcanerealms.org>
 *  Copyright (C) 2001, Arcane Realms MUD.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "loadrooms.h"
#include "time.h"
#include "screen.h"

/* External variables */
extern const	char *unused_spellname;
extern struct	race_list_element *race_list;
extern int	top_of_race_list;

/* External functions */
ACMD(do_help);
struct char_data *bandage_vict(struct char_data * ch, char *arg);
int	ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);

/* Skill functions (defined in act.skills.c) */
SKILL(skl_bandage);
SKILL(skl_hide);
SKILL(skl_sneak);
SKILL(skl_streetwise);
SKILL(skl_find_hidden);
SKILL(skl_scan);
SKILL(skl_tutor);
SKILL(skl_forage);
SKILL(skl_dyeing);
SKILL(skl_steal);

/* Local functions */
void _skillo(int skl, const char *name, byte min_position, int targets, byte violent, int group, ubyte learned);
void unused_skill(int skl);
void assign_skills(void);
void gain_attrib(struct char_data *ch, int attrib);
int get_attrib (struct char_data *ch, int attrib);
ACMD(do_listskills);
void sort_skills(void);
int	compare_skills(const void *x, const void *y);
void skill_result(struct char_data *ch, struct char_data *vict, struct obj_data *obj, char *argument, int skill, bool success, int subcmd);
bool decrease_skills(struct char_data *ch, int skill_gain);

/* Local variables */
int	skill_sort_info[MAX_SKILLS + 1];

/* Local defines */
#define	SKINFO skill_info[skillnum]
#define	SKTREE skill_tree[skillno]

struct state_struct skill_state[] = {
	{	SSTATE_DECREASE,	'-',	"decrease",	"&R"	},
	{	SSTATE_LOCK,			'#',	"lock",			"&Y"	},
	{	SSTATE_INCREASE,	'+',	"increase",	"&G"	}
};

struct skill_data skill_tree[] = {
//	skill,									pre-requisite,				selectable,	cost,		%,		lvl,	atr,							time,	vict,	msg

	{ GROUP_COMMUNICATION,		-1,										NO,					0,			0,		0,		ATR_INTELLIGENCE,	0,		0,		""	},	// 0
	{ LANG_ELVEN,							GROUP_COMMUNICATION,	NO,					3,			2000,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ LANG_GNOMISH,						GROUP_COMMUNICATION,	NO,					3,			1500,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ LANG_DWARVEN,						GROUP_COMMUNICATION,	NO,					3,			800,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ LANG_ANCIENT,						GROUP_COMMUNICATION,	NO,					3,			4500,	3,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ LANG_COMMON,						GROUP_COMMUNICATION,	NO,					1,			100,	1,		ATR_INTELLIGENCE,	0,		0,		""	},	// 5
	{ LANG_THIEF,							GROUP_COMMUNICATION,	NO,					3,			1000,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_WRITING,					GROUP_COMMUNICATION,	YES,				3,			2500,	2,		ATR_PRECISION,		2,		0,		""	},
	{ SKILL_FORGERY,					SKILL_WRITING,				NO,					3,			1000,	2,		ATR_PRECISION,		5,		0,		""	},
	{ LANG_SIGN,							GROUP_COMMUNICATION,	NO,					3,			3000,	2,		ATR_PRECISION,		0,		0,		""	},

	{ GROUP_ANIMAL,						-1,										YES,				0,			0,		0,		ATR_AGILITY,			0,		0,		""	},	// 10
	{ SKILL_AMINAL_TAMING,		GROUP_ANIMAL,					NO,					3,			3000,	2,		ATR_AGILITY,			7,		0,		""	},
	{ SKILL_ANIMAL_TRAINING,	SKILL_AMINAL_TAMING,	NO,					3,			4000,	2,		ATR_AGILITY,			10,		0,		""	},
	{ SKILL_RIDING_LAND,			GROUP_ANIMAL,					NO,					3,			500,	2,		ATR_AGILITY,			1,		0,		""	},
	{ SKILL_RIDING_AIR,				GROUP_ANIMAL,					NO,					3,			1500,	2,		ATR_AGILITY,			2,		0,		""	},

	{ GROUP_ATHLETIC,					-1,										YES,				0,			0,		0,		ATR_AGILITY,			0,		0,		""	},	// 15
	{ SKILL_DANCING,					GROUP_ATHLETIC,				NO,					3,			500,	2,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_SWIMMING,					GROUP_ATHLETIC,				NO,					3,			500,	2,		ATR_AGILITY,			0,		0,		""	},

	{ GROUP_SOCIAL,						-1,										YES,				0,			0,		0,		ATR_CHARISMA,			0,		0,		""	},
	{ SKILL_GAMBLING,					GROUP_SOCIAL,					NO,					3,			500,	2,		ATR_LUCK,					0,		0,		""	},
	{ SKILL_TEACHING,					GROUP_SOCIAL,					NO,					3,			500,	2,		ATR_INTELLIGENCE,	5,		1,		""	},	// 20
	{ SKILL_BEGGING,					GROUP_SOCIAL,					YES,				3,			500,	2,		ATR_CHARISMA,			0,		0,		""	},

	{ GROUP_OBSERVATION,			-1,										YES,				0,			0,		0,		ATR_PERCEPTION,		0,		0,		""	},
	{ SKILL_SCAN,							GROUP_OBSERVATION,		NO,					2,			500,	1,		ATR_PERCEPTION,		1,		0,		"You scan the area...\r\n"	},
	{ SKILL_DETECTION,				GROUP_OBSERVATION,		NO,					1,			500,	3,		ATR_PERCEPTION,		0,		0,		""	},
	{ SKILL_FIND_HIDDEN,			GROUP_OBSERVATION,		NO,					1,			500,	3,		ATR_PERCEPTION,		4,		0,		"You scrutinize your surroundings.\r\n"	},	// 25
	{ SKILL_DETECT_TRAP,			GROUP_OBSERVATION,		NO,					1,			500,	3,		ATR_PERCEPTION,		0,		0,		""	},

	{ GROUP_COMBAT,						-1,										YES,				0,			0,		0,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_MELEE,						GROUP_COMBAT,					NO,					1,			500,	1,		ATR_STRENGTH,			0,		0,		""	},
	{	SKILL_MISSILES,					GROUP_COMBAT,					YES,				1,			500,	2,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_AXES,							SKILL_MELEE,					YES,				1,			500,	3,		ATR_STRENGTH,			0,		0,		""	},
	{ SKILL_SWORDS,						SKILL_MELEE,					YES,				1,			500,	2,		ATR_STRENGTH,			0,		0,		""	},	// 30
	{ SKILL_DAGGERS,					SKILL_MELEE,					YES,				1,			500,	1,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_MACES,						SKILL_MELEE,					YES,				1,			500,	2,		ATR_STRENGTH,			0,		0,		""	},
	{ SKILL_BOWS,							SKILL_MISSILES,				YES,				1,			500,	3,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_POLE_ARMS,				SKILL_MELEE,					YES,				1,			500,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_THROWN,						SKILL_MISSILES,				YES,				1,			500,	3,		ATR_AGILITY,			0,		0,		""	},	// 35
	{ SKILL_STRATEGY,					GROUP_COMBAT,					YES,				1,			500,	3,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_TACTICS,					GROUP_COMBAT,					YES,				1,			500,	3,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_PUNCH,						SKILL_MELEE,					NO,					1,			500,	1,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_BASH,							SKILL_MELEE,					NO,					2,			1000,	2,		ATR_STRENGTH,			0,		0,		""	},
	{ SKILL_KICK,							SKILL_MELEE,					NO,					2,			500,	1,		ATR_AGILITY,			0,		0,		""	},	// 40
	{ SKILL_RETREAT,					SKILL_STRATEGY,				NO,					2,			500,	2,		ATR_PERCEPTION,		0,		0,		""	},
	{ SKILL_RESCUE,						SKILL_TACTICS,				NO,					3,			2000,	2,		ATR_LUCK,					0,		0,		""	},
	{ SKILL_PARRY,						SKILL_MELEE,					YES,				2,			500,	2,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_DISARM,						SKILL_PARRY,					YES,				3,			3000,	2,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_DUAL,							SKILL_MELEE,					YES,				2,			500,	2,		ATR_AGILITY,			0,		0,		""	},	// 45
	{ SKILL_DODGE,						SKILL_MELEE,					NO,					2,			500,	2,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_DEFEND,						SKILL_DODGE,					NO,					2,			1000,	2,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_WATCH,						SKILL_DODGE,					NO,					3,			2000, 2,		ATR_PERCEPTION,		0,		0,		""	},
	{ SKILL_CHASE,						GROUP_COMBAT,					NO,					2,			1000,	1,		ATR_PERCEPTION,		0,		0,		""	},

	{ GROUP_HUNTING,					-1,										YES,				0,			0,		0,		ATR_AGILITY,			0,		0,		""	},	// 50
	{ SKILL_FISHING,					GROUP_HUNTING,				YES,				1,			500,	1,		ATR_PERCEPTION,		6,		0,		"You begin to fish...\r\n"	},
	{ SKILL_HUNTING,					GROUP_HUNTING,				YES,				1,			1500,	2,		ATR_PERCEPTION,		8,		0,		"You begin hunting...\r\n"	},
	{ SKILL_TRACK,						SKILL_SCAN,						YES,				2,			2000,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_GATHER,						GROUP_HUNTING,				YES,				1,			1500,	2,		ATR_PRECISION,		5,		0,		"You begin to gather food...\r\n"	},

	{ GROUP_ART,							-1,										YES,				0,			0,		0,		ATR_PRECISION,		0,		0,		""	},	// 55
	{ SKILL_PAINTING,					GROUP_ART,						NO,					2,			2000,	2,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_POTTERY,					GROUP_ART,						YES,				2,			2000,	2,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_GLASSBLOWING,			GROUP_ART,						NO,					3,			2000,	3,		ATR_PRECISION,		0,		0,		""	},

	{ GROUP_MUSIC,						-1,										YES,				0,			0,		0,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_INSTRUMENTS,			GROUP_MUSIC,					YES,				2,			2000,	2,		ATR_PRECISION,		0,		0,		""	},	// 60
	{ SKILL_ENTICEMENT,				GROUP_MUSIC,					NO,					2,			2000,	2,		ATR_CHARISMA,			0,		0,		""	},
	{ SKILL_PEACEMAKING,			GROUP_MUSIC,					NO,					2,			2000,	2,		ATR_CHARISMA,			0,		0,		""	},

	{ GROUP_FABRICS,					-1,										YES,				0,			0,		0,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_LEATHERWORKING,		GROUP_FABRICS,				NO,					1,			1000,	3,		ATR_STRENGTH,			0,		0,		""	},
	{ SKILL_TAILORING,				GROUP_FABRICS,				YES,				1,			1500,	2,		ATR_PRECISION,		0,		0,		""	},	// 65
	{ SKILL_WEAVING,					GROUP_FABRICS,				NO,					1,			2000,	2,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_DYEING,						GROUP_FABRICS,				YES,				1,			1000,	2,		ATR_PERCEPTION,		4,		0,		"You begin to dye a garment...\r\n"	},

	{ GROUP_CULINARY,					-1,										YES,				0,			0,		0,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_COOKING,					GROUP_CULINARY,				YES,				1,			500,	1,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_BUTCHERING,				GROUP_CULINARY,				NO,					2,			1000,	2,		ATR_STRENGTH,			0,		0,		""	},
	{ SKILL_BREWING,					GROUP_CULINARY,				YES,				2,			1000,	3,		ATR_PRECISION,		0,		0,		""	},	// 70
	{ SKILL_POISON_FOOD,			GROUP_CULINARY,				NO,					3,			2000,	4,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_BAKING,						GROUP_CULINARY,				YES,				3,			2000,	3,		ATR_PRECISION,		0,		0,		""	},

	{ GROUP_MEDICAL,					-1,										YES,				0,			0,		0,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_MEDICAL_AID,			GROUP_MEDICAL,				YES,				1,			1000,	2,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_SURGERY,					GROUP_MEDICAL,				NO,					3,			3500,	3,		ATR_PRECISION,		0,		0,		""	},	// 75
	{ SKILL_NURSING,					GROUP_MEDICAL,				NO,					2,			1000,	2,		ATR_CHARISMA,			0,		0,		""	},
	{ SKILL_BANDAGE,					GROUP_MEDICAL,				YES,				2,			500,	1,		ATR_PRECISION,		1,		1,		""	},

	{ GROUP_MACHINEMAKING,		-1,										YES,				0,			0,		0,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_LOCK_SMITHERY,		GROUP_MACHINEMAKING,	NO,					3,			2000,	3,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_TRAP_MAKING,			GROUP_MACHINEMAKING,	NO,					3,			1000,	3,		ATR_PRECISION,		0,		0,		""	},	// 80
	{ SKILL_CLOCK_SMITHERY,		GROUP_MACHINEMAKING,	NO,					4,			2500,	3,		ATR_PRECISION,		0,		0,		""	},

	{ GROUP_STONEWORKING,			-1,										YES,				0,			0,		0,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_GEM_CUTTING,			GROUP_STONEWORKING,		NO,					3,			2500,	4,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_SCULPTING,				GROUP_STONEWORKING,		NO,					1,			1500,	3,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_MINING,						GROUP_STONEWORKING,		YES,				2,			2000,	3,		ATR_INTELLIGENCE,	9,		0,		"You start mining...\r\n"	},	// 85

	{ GROUP_WOODWORKING,			-1,										YES,				0,			0,		0,		ATR_STRENGTH,			0,		0,		""	},
	{ SKILL_LUMBERJACK,				GROUP_WOODWORKING,		YES,				1,			500,	1,		ATR_STRENGTH,			7,		0,		"You start lumberjacking...\r\n"	},
	{ SKILL_CARPENTRY,				GROUP_WOODWORKING,		YES,				2,			1000,	2,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_BOWYER,						GROUP_WOODWORKING,		YES,				3,			1500,	3,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_WOODCARVING,			GROUP_WOODWORKING,		NO,					2,			1500,	3,		ATR_PRECISION,		0,		0,		""	},	// 90

	{ GROUP_METALWORKING,			-1,										YES,				0,			0,		0,		ATR_STRENGTH,			0,		0,		""	},
	{ SKILL_WEAPONSMITHING,		GROUP_METALWORKING,		NO,					2,			500,	2,		ATR_STRENGTH,			2,		0,		""	},	// 90
	{ SKILL_BLADEFORGING,			GROUP_METALWORKING,		NO,					5,			1000,	2,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_ARMORSMITHING,		GROUP_METALWORKING,		NO,					2,			1500,	3,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_JEWELRYSMITHING,	GROUP_METALWORKING,		NO,					4,			1500,	3,		ATR_PRECISION,		0,		0,		""	},	// 95
	{ SKILL_SMELTING,					GROUP_METALWORKING,		YES,				3,			1500,	3,		ATR_INTELLIGENCE,	0,		0,		""	},

	{ GROUP_LORE,							-1,										YES,				0,			0,		0,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_BOTANY,						GROUP_LORE,						NO,					1,			1000,	1,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_GEOGRAPHY,				GROUP_LORE,						NO,					1,			1000,	1,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_ZOOLOGY,					GROUP_LORE,						NO,					1,			1500,	1,		ATR_INTELLIGENCE,	0,		0,		""	},	// 100
	{ SKILL_REALM_LORE,				GROUP_LORE,						NO,					4,			3000,	3,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_ARCANE_LORE,			GROUP_LORE,						NO,					3,			2500,	3,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_OCCULT,						GROUP_LORE,						NO,					2,			2000,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_THEOLOGY,					GROUP_LORE,						NO,					2,			2000,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_ANTHROPOLOGY,			GROUP_LORE,						NO,					2,			1500,	1,		ATR_INTELLIGENCE,	0,		0,		""	},	// 105
	{ SKILL_LINGUISTICS,			GROUP_LORE,						NO,					2,			1500,	1,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_STREETWISE,				GROUP_LORE,						YES,				1,			100,	1,		ATR_INTELLIGENCE,	1,		0,		"You attempt to find the way...\r\n"	},
	{ SKILL_SURVIVAL,					GROUP_LORE,						NO,					2,			500,	1,		ATR_INTELLIGENCE,	0,		0,		""	},

	{ GROUP_MAGERY,						-1,										YES,				0,			0,		0,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_RESIST_MAGIC,			GROUP_MAGERY,					NO,					2,			500,	2,		ATR_ESSENCE,			0,		0,		""	},	// 110
	{ SKILL_DETECT_AURA,			GROUP_MAGERY,					NO,					2,			500,	2,		ATR_ESSENCE,			0,		0,		""	},
	{ SKILL_DIRECT_SPELLS,		GROUP_MAGERY,					NO,					2,			500,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_HUNG_SPELLS,			GROUP_MAGERY,					NO,					2,			500,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_INVOKED_MSPELLS,	GROUP_MAGERY,					NO,					2,			500,	2,		ATR_ESSENCE,			0,		0,		""	},

	{ GROUP_DIVINATION,				-1,										YES,				0,			0,		0,		ATR_ESSENCE,			0,		0,		""	},	// 115
	{ SKILL_ASTROLOGY,				GROUP_DIVINATION,			NO,					2,			500,	2,		ATR_ESSENCE,			0,		0,		""	},
	{ SKILL_TAROT,						GROUP_DIVINATION,			NO,					2,			500,	2,		ATR_ESSENCE,			0,		0,		""	},

	{ GROUP_ALCHEMY,					-1,										YES,				0,			0,		0,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_BREW,							GROUP_ALCHEMY,				NO,					2,			500,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_HERBALISM,				GROUP_ALCHEMY,				NO,					1,			250,	2,		ATR_INTELLIGENCE,	0,		0,		""	},
	{ SKILL_DYE_MAKING,				GROUP_ALCHEMY,				NO,					1,			800,	2,		ATR_PERCEPTION,		0,		0,		""	},

	{ GROUP_SCHOLASTIC,				-1,										YES,				0,			0,		0,		ATR_ESSENCE,			0,		0,		""	},
	{ SKILL_SCRIBE,						GROUP_SCHOLASTIC,			NO,					2,			500,	2,		ATR_PRECISION,		0,		0,		""	},
	{ SKILL_INVOKED_SSPELLS,	GROUP_SCHOLASTIC,			NO,					2,			500,	2,		ATR_ESSENCE,			0,		0,		""	},
	{ SKILL_MEDITATE,					GROUP_SCHOLASTIC,			NO,					2,			500,	2,		ATR_ESSENCE,			0,		0,		""	},
	{ SKILL_PRAYER,						SKILL_MEDITATE,				NO,					2,			500,	2,		ATR_ESSENCE,			0,		0,		""	},	// 125

	{ GROUP_SECRECY,					-1,										YES,				0,			0,		0,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_HIDE,							GROUP_SECRECY,				YES,				2,			500,	1,		ATR_AGILITY,			1,		0,		"You attempt to hide yourself.\r\n"	},	// 125
	{ SKILL_SNEAK,						SKILL_HIDE,						YES,				3,			1000,	2,		ATR_AGILITY,			1,		0,		"You try to move silently for a while.\r\n"	},
	{ SKILL_STEALTH,					SKILL_SNEAK,					NO,					3,			1000,	3,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_BACKSTAB,					GROUP_SECRECY,				YES,				2,			500,	2,		ATR_AGILITY,			0,		0,		""	},	// 130

	{ GROUP_THIEVERY,					-1,										YES,				0,			0,		0,		ATR_AGILITY,			0,		0,		""	},
	{ SKILL_STEAL,						GROUP_THIEVERY,				YES,				1,			500,	2,		ATR_LUCK,					0,		0,		""	},
	{ SKILL_PICK_LOCK,				GROUP_THIEVERY,				NO,					2,			500,	2,		ATR_PRECISION,		0,		0,		""	},

};


/*
 * skill_groups[] has to be one above NUM_GROUPS since the skill groups
 * start at 1, not 0 like an array does.
 */
const char *skill_groups[NUM_GROUPS+1] = {
	"\n",
	"communication skills",	// 1
	"animal skills",
	"athletic skills",
	"social skills",
	"observation skills",		// 5
	"combat skills",
	"game and hunting",
	"artistic skills",
	"musical skills",
	"fabric working",				// 10
	"culinary skills",
	"medical skills",
	"machine making",
	"stone working",
	"woodworking",					// 15
	"metalworking",
	"lore and knowledge",
	"magery",
	"divination",
	"alchemy",							// 20
	"scholastic arts",
	"secrecy",
	"thievery"
};

const	char *prac_types[] = {
	"spell",
	"skill",
	"spell/skill"
};


int	find_skill_num(char *name)
{
	int skillnum, ok;
	char *temp, *temp2;
	char first[256], first2[256];

	for (skillnum = 1; skillnum <= TOP_SKILL_DEFINE; skillnum++) {
		if (is_abbrev(name, SKINFO.name))
			return (skillnum);

		ok = TRUE;
		/* It won't be changed, but other uses of this function elsewhere may. */
		temp = any_one_arg((char *)SKINFO.name, first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok) {
			if (!is_abbrev(first2, first))
				ok = FALSE;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}

		if (ok && !*first2)
			return (skillnum);
	}

	return (-1);
}


/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE.
 */
const	char *skill_name(int skillnum)
{
	if (skillnum > 0 && skillnum <= TOP_SKILL_DEFINE)
		return (SKINFO.name);
	else if (skillnum == -1)
		return ("UNUSED");
	else
		return ("UNDEFINED");
}


/* Assign the skills on boot up */
#define	skillo(a,b,c,d,e,f) _skillo((a),(b),(c),(d),(e),(f),0)
#define	skillo_group(a,b) _skillo((a),(b),POS_DEAD,TAR_IGNORE,FALSE,-1,0)
#define	skillo_learned(a,b,c,d,e,f) _skillo((a),(b),(c),(d),(e),(f),1)
void _skillo(int skillnum, const char *name, byte min_position, int targets, byte violent, int group, ubyte learned)
{
	int i;

	for (i = 0; i < NUM_CLASSES; i++)
	SKINFO.min_position = min_position;
	SKINFO.group = group;
	SKINFO.violent = violent;
	SKINFO.targets = targets;
	SKINFO.name = name;
	SKINFO.learned = learned;
}


void unused_skill(int skillnum)
{
	int i;

	for (i = 0; i < NUM_CLASSES; i++)
	SKINFO.min_position = 0;
	SKINFO.group = 0;
	SKINFO.violent = 0;
	SKINFO.targets = 0;
	SKINFO.name = unused_spellname;
	SKINFO.learned = 0;
}


void assign_skills(void)
{
	int i;

	/* Do not change the loop below. */
	for (i = 0; i <= TOP_SKILL_DEFINE; i++)
		unused_skill(i);
	/* Do not change the loop above. */

	/*
	 * Declaration of skills - this actually doesn't do anything except
	 * set it up so that immortals can use these skills by default.  The
	 * min level to use the skill for other classes is set up in class.c.
	 */

	/* Possible Targets:

		bit 0 : IGNORE TARGET
		bit 1 : PC/NPC in room
		bit 2 : PC/NPC in world
		bit 3 : Object held
		bit 4 : Object in inventory
		bit 5 : Object in room
		bit 6 : Object in world
		bit 7 : If fighting, and no argument, select tar_char as self
		bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
		bit 9 : If no argument, select self, if argument check that it IS self.

	*/

	// Skill Groups
	skillo_group(GROUP_COMMUNICATION, "communication skills");
	skillo_group(GROUP_ANIMAL, "animal skills");
	skillo_group(GROUP_ATHLETIC, "athletic skills");
	skillo_group(GROUP_SOCIAL, "social skills");
	skillo_group(GROUP_OBSERVATION, "observation skills");
	skillo_group(GROUP_COMBAT, "combat skills");
	skillo_group(GROUP_HUNTING, "game and hunting");
	skillo_group(GROUP_ART, "artistic skills");
	skillo_group(GROUP_MUSIC, "musical skills");
	skillo_group(GROUP_FABRICS, "fabric working");
	skillo_group(GROUP_CULINARY, "culinary skills");
	skillo_group(GROUP_MEDICAL, "medical skills");
	skillo_group(GROUP_MACHINEMAKING, "machine making");
	skillo_group(GROUP_STONEWORKING, "stone working");
	skillo_group(GROUP_WOODWORKING, "woodworking");
	skillo_group(GROUP_METALWORKING, "metalworking");
	skillo_group(GROUP_LORE, "lore and knowledge");
	skillo_group(GROUP_MAGERY, "magery");
	skillo_group(GROUP_DIVINATION, "divination");
	skillo_group(GROUP_ALCHEMY, "alchemy");
	skillo_group(GROUP_SCHOLASTIC, "scholastic arts");
	skillo_group(GROUP_SECRECY, "secrecy");
	skillo_group(GROUP_THIEVERY, "thievery");

	// GROUP_COMMUNICATION
	skillo(LANG_COMMON, "common", POS_RESTING, TAR_IGNORE, FALSE, GROUP_COMMUNICATION);
	skillo_learned(LANG_ELVEN, "elven", POS_RESTING, TAR_IGNORE, FALSE, GROUP_COMMUNICATION);
	skillo_learned(LANG_GNOMISH, "gnomish", POS_RESTING, TAR_IGNORE, FALSE, GROUP_COMMUNICATION);
	skillo_learned(LANG_DWARVEN, "dwarven", POS_RESTING, TAR_IGNORE, FALSE, GROUP_COMMUNICATION);
	skillo_learned(LANG_ANCIENT, "ancient", POS_RESTING, TAR_IGNORE, FALSE, GROUP_COMMUNICATION);
	skillo_learned(LANG_THIEF, "thief", POS_RESTING, TAR_IGNORE, FALSE, GROUP_COMMUNICATION);
	skillo_learned(SKILL_WRITING, "writing", POS_RESTING, TAR_IGNORE, FALSE, GROUP_COMMUNICATION);
	skillo_learned(SKILL_FORGERY, "forgery", POS_RESTING, TAR_IGNORE, FALSE, GROUP_COMMUNICATION);
	skillo_learned(LANG_SIGN, "sign language", POS_RESTING, TAR_IGNORE, FALSE, GROUP_COMMUNICATION);

	// GROUP_ANIMAL
	skillo_learned(SKILL_AMINAL_TAMING, "animal taming", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ANIMAL);
	skillo_learned(SKILL_ANIMAL_TRAINING, "animal training", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ANIMAL);
	skillo_learned(SKILL_RIDING_LAND, "riding land", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ANIMAL);
	skillo_learned(SKILL_RIDING_AIR, "riding air", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ANIMAL);

	// GROUP_ATHLETIC
	skillo_learned(SKILL_DANCING, "dancing", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ATHLETIC);
	skillo_learned(SKILL_SWIMMING, "swimming", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ATHLETIC);

	// GROUP_SOCIAL
	skillo_learned(SKILL_GAMBLING, "gambling", POS_STANDING, TAR_IGNORE, FALSE, GROUP_SOCIAL);
	skillo_learned(SKILL_TEACHING, "teaching", POS_STANDING, TAR_IGNORE, FALSE, GROUP_SOCIAL);
	skillo_learned(SKILL_BEGGING, "begging", POS_STANDING, TAR_IGNORE, FALSE, GROUP_SOCIAL);

	// GROUP_OBSERVATION
	skillo_learned(SKILL_SCAN, "scan", POS_STANDING, TAR_IGNORE, FALSE, GROUP_OBSERVATION);
	skillo_learned(SKILL_DETECTION, "detection", POS_STANDING, TAR_IGNORE, FALSE, GROUP_OBSERVATION);
	skillo_learned(SKILL_FIND_HIDDEN, "find hidden", POS_STANDING, TAR_IGNORE, FALSE, GROUP_OBSERVATION);
	skillo_learned(SKILL_DETECT_TRAP, "detect trap", POS_STANDING, TAR_IGNORE, FALSE, GROUP_OBSERVATION);

	// GROUP_COMBAT
	skillo(SKILL_MELEE, "melee", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_MISSILES, "missile weapons", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_AXES, "axes", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_SWORDS, "swords", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_DAGGERS, "daggers", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_MACES, "maces", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_BOWS, "bows", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_POLE_ARMS, "polearms", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_THROWN, "thrown", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_STRATEGY, "strategy", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_TACTICS, "tactics", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo(SKILL_PUNCH, "punch", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_BASH, "bash", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo(SKILL_KICK, "kick", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_RETREAT, "retreat", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_RESCUE, "rescue", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo(SKILL_PARRY, "parry", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_DISARM, "disarm", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_DUAL, "dual wield", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_DODGE, "dodge", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_DEFEND, "defend", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_WATCH, "watch", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);
	skillo_learned(SKILL_CHASE, "chase", POS_STANDING, TAR_IGNORE, TRUE, GROUP_COMBAT);

	// GROUP_HUNTING
	skillo_learned(SKILL_FISHING, "fishing", POS_STANDING, TAR_IGNORE, FALSE, GROUP_HUNTING);
	skillo_learned(SKILL_HUNTING, "hunting", POS_STANDING, TAR_IGNORE, FALSE, GROUP_HUNTING);
	skillo_learned(SKILL_TRACK, "track", POS_STANDING, TAR_IGNORE, FALSE, GROUP_HUNTING);
	skillo_learned(SKILL_GATHER, "gather", POS_STANDING, TAR_IGNORE, FALSE, GROUP_HUNTING);

	// GROUP_ART
	skillo_learned(SKILL_PAINTING, "painting", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ART);
	skillo_learned(SKILL_POTTERY, "pottery", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ART);
	skillo_learned(SKILL_GLASSBLOWING, "glassblowing", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ART);

	// GROUP_MUSIC
	skillo_learned(SKILL_INSTRUMENTS, "instruments", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MUSIC);
	skillo_learned(SKILL_ENTICEMENT, "enticement", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MUSIC);
	skillo_learned(SKILL_PEACEMAKING, "peacemaking", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MUSIC);

	// GROUP_FABRICS
	skillo_learned(SKILL_LEATHERWORKING, "leatherworking", POS_STANDING, TAR_IGNORE, FALSE, GROUP_FABRICS);
	skillo_learned(SKILL_TAILORING, "tailoring", POS_STANDING, TAR_IGNORE, FALSE, GROUP_FABRICS);
	skillo_learned(SKILL_WEAVING, "weaving", POS_STANDING, TAR_IGNORE, FALSE, GROUP_FABRICS);
	skillo_learned(SKILL_DYEING, "dyeing", POS_STANDING, TAR_IGNORE, FALSE, GROUP_FABRICS);

	// GROUP_CULINARY
	skillo_learned(SKILL_COOKING, "cooking", POS_STANDING, TAR_IGNORE, FALSE, GROUP_CULINARY);
	skillo_learned(SKILL_BUTCHERING, "butchering", POS_STANDING, TAR_IGNORE, FALSE, GROUP_CULINARY);
	skillo_learned(SKILL_BREWING, "brewing", POS_STANDING, TAR_IGNORE, FALSE, GROUP_CULINARY);
	skillo_learned(SKILL_POISON_FOOD, "poisoning food", POS_STANDING, TAR_IGNORE, FALSE, GROUP_CULINARY);
	skillo_learned(SKILL_BAKING, "baking", POS_STANDING, TAR_IGNORE, FALSE, GROUP_CULINARY);

	// GROUP_MEDICAL
	skillo_learned(SKILL_MEDICAL_AID, "medical aid", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MEDICAL);
	skillo_learned(SKILL_SURGERY, "surgery", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MEDICAL);
	skillo_learned(SKILL_NURSING, "nursing", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MEDICAL);
	skillo_learned(SKILL_BANDAGE, "bandage", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MEDICAL);

	// GROUP_MACHINEMAKING
	skillo_learned(SKILL_LOCK_SMITHERY, "lock smithery", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MACHINEMAKING);
	skillo_learned(SKILL_TRAP_MAKING, "trap making", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MACHINEMAKING);
	skillo_learned(SKILL_CLOCK_SMITHERY, "clock smithery", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MACHINEMAKING);

	// GROUP_STONEWORKING
	skillo_learned(SKILL_GEM_CUTTING, "gem cutting", POS_STANDING, TAR_IGNORE, FALSE, GROUP_STONEWORKING);
	skillo_learned(SKILL_SCULPTING, "sculpting", POS_STANDING, TAR_IGNORE, FALSE, GROUP_STONEWORKING);
	skillo_learned(SKILL_MINING, "mining", POS_STANDING, TAR_IGNORE, FALSE, GROUP_STONEWORKING);

	// GROUP_WOODWORKING
	skillo_learned(SKILL_LUMBERJACK, "lumberjacking", POS_STANDING, TAR_IGNORE, FALSE, GROUP_WOODWORKING);
	skillo_learned(SKILL_CARPENTRY, "carpentry", POS_STANDING, TAR_IGNORE, FALSE, GROUP_WOODWORKING);
	skillo_learned(SKILL_BOWYER, "bowyer", POS_STANDING, TAR_IGNORE, FALSE, GROUP_WOODWORKING);
	skillo_learned(SKILL_WOODCARVING, "woodcarving", POS_STANDING, TAR_IGNORE, FALSE, GROUP_WOODWORKING);

	// GROUP_METALWORKING
	skillo_learned(SKILL_WEAPONSMITHING, "weaponsmithing", POS_STANDING, TAR_IGNORE, FALSE, GROUP_METALWORKING);
	skillo_learned(SKILL_BLADEFORGING, "bladeforging", POS_STANDING, TAR_IGNORE, FALSE, GROUP_METALWORKING);
	skillo_learned(SKILL_ARMORSMITHING, "armorsmithing", POS_STANDING, TAR_IGNORE, FALSE, GROUP_METALWORKING);
	skillo_learned(SKILL_JEWELRYSMITHING, "jewelrysmithing", POS_STANDING, TAR_IGNORE, FALSE, GROUP_METALWORKING);
	skillo_learned(SKILL_SMELTING, "smelting", POS_STANDING, TAR_IGNORE, FALSE, GROUP_METALWORKING);

	// GROUP_LORE
	skillo_learned(SKILL_BOTANY, "botany", POS_STANDING, TAR_IGNORE, FALSE, GROUP_LORE);
	skillo_learned(SKILL_GEOGRAPHY, "geography", POS_STANDING, TAR_IGNORE, FALSE, GROUP_LORE);
	skillo_learned(SKILL_ZOOLOGY, "zoology", POS_STANDING, TAR_IGNORE, FALSE, GROUP_LORE);
	skillo_learned(SKILL_REALM_LORE, "realm lore", POS_STANDING, TAR_IGNORE, FALSE, GROUP_LORE);
	skillo_learned(SKILL_ARCANE_LORE, "arcane lore", POS_STANDING, TAR_IGNORE, FALSE, GROUP_LORE);
	skillo_learned(SKILL_OCCULT, "occult", POS_STANDING, TAR_IGNORE, FALSE, GROUP_LORE);
	skillo_learned(SKILL_THEOLOGY, "theology", POS_STANDING, TAR_IGNORE, FALSE, GROUP_LORE);
	skillo_learned(SKILL_ANTHROPOLOGY, "anthropology", POS_STANDING, TAR_IGNORE, FALSE, GROUP_LORE);
	skillo_learned(SKILL_LINGUISTICS, "linguistics", POS_STANDING, TAR_IGNORE, FALSE, GROUP_LORE);
	skillo_learned(SKILL_STREETWISE, "streetwise", POS_STANDING, TAR_IGNORE, FALSE, GROUP_LORE);
	skillo_learned(SKILL_SURVIVAL, "survival", POS_STANDING, TAR_IGNORE, FALSE, GROUP_LORE);

	// GROUP_MAGERY
	skillo_learned(SKILL_RESIST_MAGIC, "resist magic", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MAGERY);
	skillo_learned(SKILL_DETECT_AURA, "detect aura", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MAGERY);
	skillo_learned(SKILL_DIRECT_SPELLS, "direct spells", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MAGERY);
	skillo_learned(SKILL_HUNG_SPELLS, "hung spells", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MAGERY);
	skillo_learned(SKILL_INVOKED_MSPELLS, "invoked magery spells", POS_STANDING, TAR_IGNORE, FALSE, GROUP_MAGERY);

	// GROUP_DIVINATION
	skillo_learned(SKILL_ASTROLOGY, "astrology", POS_STANDING, TAR_IGNORE, FALSE, GROUP_DIVINATION);
	skillo_learned(SKILL_TAROT, "tarot", POS_STANDING, TAR_IGNORE, FALSE, GROUP_DIVINATION);

	// GROUP_ALCHEMY
	skillo_learned(SKILL_BREW, "potionbrewing", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ALCHEMY);
	skillo_learned(SKILL_HERBALISM, "herbalism", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ALCHEMY);
	skillo_learned(SKILL_DYE_MAKING, "dye-making", POS_STANDING, TAR_IGNORE, FALSE, GROUP_ALCHEMY);

	// GROUP_SCHOLASTIC
	skillo_learned(SKILL_SCRIBE, "scribing", POS_STANDING, TAR_IGNORE, FALSE, GROUP_SCHOLASTIC);
	skillo_learned(SKILL_INVOKED_SSPELLS, "invoked scholastic spells", POS_STANDING, TAR_IGNORE, FALSE, GROUP_SCHOLASTIC);
	skillo_learned(SKILL_MEDITATE, "meditate", POS_STANDING, TAR_IGNORE, FALSE, GROUP_SCHOLASTIC);
	skillo_learned(SKILL_PRAYER, "prayer", POS_STANDING, TAR_IGNORE, FALSE, GROUP_SCHOLASTIC);

	// GROUP_SECRECY
	skillo_learned(SKILL_HIDE, "hide", POS_STANDING, TAR_IGNORE, FALSE, GROUP_SECRECY);
	skillo_learned(SKILL_SNEAK, "sneak", POS_STANDING, TAR_IGNORE, FALSE, GROUP_SECRECY);
	skillo_learned(SKILL_STEALTH, "stealth", POS_STANDING, TAR_IGNORE, FALSE, GROUP_SECRECY);
	skillo_learned(SKILL_STEAL, "steal", POS_STANDING, TAR_IGNORE, FALSE, GROUP_THIEVERY);
	skillo_learned(SKILL_PICK_LOCK, "pick lock", POS_STANDING, TAR_IGNORE, FALSE, GROUP_THIEVERY);
	skillo_learned(SKILL_BACKSTAB, "backstab", POS_STANDING, TAR_IGNORE, FALSE, GROUP_SECRECY);

}


int find_skill(int skill)
{
	int skillno;

	for (skillno = 0; skillno < MAX_SKILLS; skillno++) {
		if (SKTREE.skill == skill)
			return (skillno);
	}

	return (-1);

}


#define ALL_SKILLS			0
#define KNOWN_SKILLS		1
#define SKILL_GROUP			2
#define LIST_GROUPS			3
#define LIST_INCREASE		4
#define LIST_DECREASE		5
#define LIST_LOCK				6

ACMD(do_listskills)
{
	int skillnum, i, qend = 0, skillno, skill_num = 0, gotskills = 0, type = KNOWN_SKILLS, numskills = 0;
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char *skills = get_buffer(MAX_STRING_LENGTH);
	char *name = get_buffer(MAX_INPUT_LENGTH);
	char *skillname = get_buffer(128);
	char *level = get_buffer(32);
	bool show_bonus = TRUE;

	argument = one_argument(argument, name);

	if (!strcmp(name, "all"))
		type = ALL_SKILLS;
	else if (!strcmp(name, "list"))
		type = LIST_GROUPS;
	else if (*name == '+')
		type = LIST_INCREASE;
	else if (*name == '-')
		type = LIST_DECREASE;
	else if (*name == '#')
		type = LIST_LOCK;
	else if (!strcmp(name, "help")) {
		send_to_char("Usage: skills [ all | list | + | - | # | help ] (defaults to known skills)\r\n", ch);
		release_buffer(printbuf);
		release_buffer(skills);
		release_buffer(name);
		release_buffer(skillname);
		release_buffer(level);
		return;
	}

	skill_num = find_skill_num(name);
	if (skill_num > 0 && skill_num <= NUM_GROUPS + 1)
		type = SKILL_GROUP;

	if (!*name)
		type = KNOWN_SKILLS;

	sprintf(printbuf, "\r\n");

	for (i = 1; i < NUM_GROUPS + 1; i++) {

		if (type == SKILL_GROUP && skill_num != i && *name)
			continue;

		gotskills = 0;
		*skills = '\0';

		for (qend = 0, skillnum = 0; skillnum <= TOP_SKILL_DEFINE; skillnum++) {
			skillno = find_skill(skillnum);
			if (SKINFO.name == unused_spellname || SKINFO.group != i)	/* This is valid. */
				continue;
			if (type == KNOWN_SKILLS && (GET_SKILL(ch, SKTREE.skill) < 100 || (GET_SKILL(ch, SKTREE.pre) <= 0 && SKTREE.pre != -1)))
				continue;
			if (type == LIST_INCREASE && (GET_SKILL_STATE(ch, SKTREE.skill) != SSTATE_INCREASE))
				continue;
			if (type == LIST_DECREASE && (GET_SKILL_STATE(ch, SKTREE.skill) != SSTATE_DECREASE))
				continue;
			if (type == LIST_LOCK && (GET_SKILL_STATE(ch, SKTREE.skill) != SSTATE_LOCK))
				continue;
			gotskills++;
			numskills++;
			if (GET_SKILL(ch, skillnum) > 100) {
				if (PRF_FLAGGED(ch, PRF_REALSKILLS)) {
					sprintf(level, "%3d%%", (int)(GET_SKILL(ch, skillnum) / 100));
				} else {
					sprintf(level, "%3d%%", (int)((GET_SKILL(ch, skillnum) / 100) + (get_attrib(ch, SKTREE.attrib) / 4)));
				}
			} else {
				strcpy(level, "----");
			}
			sprintf(skills, "%s&c%-25.25s %c(&C%s&c)(&G%1d%%&c)%s", skills, SKINFO.name, skill_state[(int)GET_SKILL_STATE(ch, skillnum) + 1].symbol, level, (get_attrib(ch, SKTREE.attrib) / 4), (qend++ % 2 == 1) ? "&n\r\n" : "   ");
		}
		if (qend % 2 == 1) {
			strcat(skills, "&n\r\n\r\n");
		} else {
			strcat(skills, "&n\r\n");
		}

		if (gotskills && type != LIST_GROUPS) {
			if (GET_SKILL(ch, i) > 100)
				sprintf(level, "%3d%%", (int)(GET_SKILL(ch, i) / 100));
			else
				strcpy(level, "----");
			sprintf(skillname, "%s", skill_groups[i]);
			sprintf(printbuf, "%s&C%s &c(&C%s&c)&n\r\n&c%s", printbuf, ALLCAP(skillname), level, skills);
		} else if (type == LIST_GROUPS) {
			sprintf(skillname, "%s", skill_groups[i]);
			sprintf(printbuf, "%s%s\r\n", printbuf, CAP(skillname));
		}

	}

	if (!numskills)
		sprintf(printbuf, "%sNo skills could be found.\r\n", printbuf);

	if (PRF_FLAGGED(ch, PRF_REALSKILLS))
		sprintf(printbuf, "%sShowing real skill values.\r\n", printbuf);
	else
		sprintf(printbuf, "%sShowing skill values with bonuses applied.\r\n", printbuf);

	if (PRF_FLAGGED(ch, PRF_REALSKILLS))
		show_bonus = FALSE;

	if (!IS_IMMORTAL(ch) && type != LIST_GROUPS)
		sprintf(printbuf, "%sTotal skill points: &W%d&n/%d\r\n", printbuf, skill_cap(ch, 0, show_bonus) / 100, GET_SKILLCAP(ch) / 100);

	page_string(ch->desc, printbuf, TRUE);

	release_buffer(printbuf);
	release_buffer(skills);
	release_buffer(name);
	release_buffer(skillname);
	release_buffer(level);
}

#undef ALL_SKILLS
#undef KNOWN_SKILLS
#undef SKILL_GROUP
#undef LIST_GROUPS
#undef LIST_INCREASE
#undef LIST_DECREASE
#undef LIST_LOCK


/*
 * Modifier in this case is negative, it is added to the
 * total size of the dize that is rolled.  So if we have a 
 * modifier of 30, we will automatically fail if we roll
 * over 100.  Could make the modifier negative, in that
 * it will make the skill check easier. (TB)
 */
bool skill_check(struct char_data *ch, int skl, int modifier)
{
	int skillno = 0, score = 0, diceroll = 0;

	if (!(skillno = find_skill(skl)))
		return (FALSE);

	score = GET_SKILL(ch, SKTREE.skill) / 100;

	if (GET_SKILL(ch, SKTREE.pre) == NOTHING)
		score += get_attrib(ch, SKTREE.attrib) / 2;
	else score += (GET_SKILL(ch, SKTREE.pre) / 1000);

	/* 101 is a complete failure, no matter what the proficiency. */
	diceroll = number(0, 101 + modifier);

	/*
	 * if the dice roll is lower than the modified score,
	 * the player may gain percentage in the skill.
	 */
	GET_SKILL(ch, SKTREE.skill) = skill_gain(ch, skillno, 101 - diceroll);
	update_groups(ch);

	if (diceroll > score)
		return (FALSE);

	return (TRUE);
}


int skill_gain(struct char_data *ch, int skillno, int chance)
{
	int skl, diceroll = 0;
	int max_skill_level = 10000;  // sets the max for skill level
	int max_skill_gain = 20;      // sets the max for gains
	int skill_level_percent;
	int skill_gain_temp;
	int skill_gain = 0;
	bool gained_pre = FALSE;

	if (IS_NPC(ch))
		return (0);

	skl = GET_SKILL(ch, SKTREE.skill);

	if (skl >= max_skill_level)
		return(max_skill_level);

	skill_level_percent = (skl * 100) / max_skill_level;						// gets skill level percentage
	skill_gain_temp = (skill_level_percent * max_skill_gain) / 150;	// gets real gains from gain percentage
	skill_gain = max_skill_gain - skill_gain_temp;

	/*
	 * Weapon skills are used so often that they gain much
	 * slower than normal skills.
	 */
	if (SKTREE.pre == GROUP_COMBAT ||
			SKTREE.pre == SKILL_MELEE ||
			SKTREE.pre == SKILL_MISSILES)
		skill_gain = number(1, LIMIT(skill_gain / 2, 1, 100));

	/*
	 * This skill has been locked, or set to decrease, or the player
	 * is out of character, cannot gain.
	 */
	if (GET_SKILL_STATE(ch, SKTREE.skill) != SSTATE_INCREASE || IS_OOC(ch))
		return (skl);

	diceroll = number(1, 101);

	/*
	 * Slow the gain rate down a little by checking the diceroll
	 * against the current skill level.
	 */
	if (GET_SKILL(ch, SKTREE.skill) / 100 > diceroll)
		return (skl);

	/*
	 * If the player has capped their skills, we need to decrease
	 * other skills, and if we cannot, the player won't gain.
	 */
	if ((skill_cap(ch, skill_gain, FALSE)) >= GET_SKILLCAP(ch))
		if (!(decrease_skills(ch, skill_gain)))
			return (skl);

	if (SKTREE.pre != -1 && diceroll < chance && SKTREE.pre < max_skill_level && GET_SKILL(ch, SKTREE.pre) < max_skill_level)
		gained_pre = TRUE;

	/* If the player wants to see the skill gain messages */
	if (PRF_FLAGGED(ch, PRF_SKILLGAINS)) {
		if (skl + skill_gain >= max_skill_level) {
			send_to_charf(ch, "\r\n&cSKILLS: You have mastered &C%s&c!&n\r\n\r\n", skill_name(SKTREE.skill));
			return (max_skill_level);
		}
		if ((int)((skl + skill_gain) / 100) > (int)((skl) / 100))
			send_to_charf(ch, "\r\n&cSKILLS: You have gained in &C%s&c (%d%%)&n\r\n\r\n", skill_name(SKTREE.skill), (int)((skl + skill_gain) / 100));
		if (gained_pre && ((int)((GET_SKILL(ch, SKTREE.pre) + 1) / 100) > (int)((GET_SKILL(ch, SKTREE.pre)) / 100)))
			send_to_charf(ch, "\r\n&cSKILLS: You have gained in &C%s&c (%d%%)&n\r\n\r\n", skill_name(SKTREE.pre), (int)((GET_SKILL(ch, SKTREE.pre) + 1) / 100));
	}

	if (gained_pre && GET_SKILL_STATE(ch, SKTREE.pre) == SSTATE_INCREASE) {
		GET_SKILL(ch, SKTREE.pre) = (LIMIT(GET_SKILL(ch, SKTREE.pre) + 1, 1, max_skill_level));
		if (get_attrib(ch, SKTREE.attrib) != NOTHING) {
			gain_attrib(ch, SKTREE.attrib);
		}
	}
	
	return (LIMIT(skl + skill_gain, 1, max_skill_level));
}


void update_groups(struct char_data *ch)
{
	long num_skills = 0;
	long amount = 0;
	int skillnum, group, group_orig;

  for (group = 1; group <= NUM_GROUPS; group++) {
		group_orig = GET_SKILL(ch, group);
		for (skillnum = 0; skillnum <= TOP_SKILL_DEFINE; skillnum++) {		
			if (SKINFO.group != group) continue;	
			num_skills++;
			amount += GET_SKILL(ch, skillnum);
		}
		if (num_skills != 0)
			GET_SKILL(ch, group) = amount / num_skills;
		if ((group_orig/100) < (GET_SKILL(ch, group)/100) && STATE(ch->desc) == CON_PLAYING) {
			send_to_charf(ch, "&cSKILLS: You have gained in &C%s&c (%d%%)&n\r\n", skill_name(group), (int)((GET_SKILL(ch, group)) / 100));
		}
		amount = 0;
		num_skills = 0;
	}
}


/*
 * Initial version of gain_attrib() for attribute gains related to skills.
 * This should perhaps be simplified somehow, but right now this works and
 * we can work on code optimization later.
 * Torgny Bjers <artovil@arcanerealms.org>, 2002-12-10
 */
void gain_attrib(struct char_data *ch, int attrib)
{
	int i;
	int min_attrib, max_attrib;

	if (IS_OOC(ch))
		return;

	// Find the min/max in the race list.
	for (i = 0; i < top_of_race_list; i++)
		if (race_list[i].constant == GET_RACE(ch))
			break;

	min_attrib = race_list[i].min_attrib;
	max_attrib = race_list[i].max_attrib;

	switch (attrib) {
	case ATR_STRENGTH:
		if (ch->real_abils.strength + 1 > max_attrib) {
			ch->real_abils.strength = max_attrib;
			return;
		}
		if ((int)((ch->real_abils.strength + 1) / 100) > (int)((ch->real_abils.strength) / 100))
			send_to_charf(ch, "\r\n&cATTRIBS: You have gained in &C%s&c (%d)&n\r\n\r\n", attributes[attrib], (int)((ch->real_abils.strength + 1) / 100));
		ch->real_abils.strength = LIMIT(ch->real_abils.strength + 1, min_attrib, max_attrib);
		gain_attrib(ch, ATR_HEALTH);
		break;
	case ATR_AGILITY:
		if (ch->real_abils.agility + 1 > max_attrib) {
			ch->real_abils.agility = max_attrib;
			return;
		}
		if ((int)((ch->real_abils.agility + 1) / 100) > (int)((ch->real_abils.agility) / 100))
			send_to_charf(ch, "\r\n&cATTRIBS: You have gained in &C%s&c (%d)&n\r\n\r\n", attributes[attrib], (int)((ch->real_abils.agility + 1) / 100));
		ch->real_abils.agility = LIMIT(ch->real_abils.agility + 1, min_attrib, max_attrib);
		gain_attrib(ch, ATR_HEALTH);
		break;
	case ATR_PRECISION:
		if (ch->real_abils.precision + 1 > max_attrib) {
			ch->real_abils.precision = max_attrib;
			return;
		}
		if ((int)((ch->real_abils.precision + 1) / 100) > (int)((ch->real_abils.precision) / 100))
			send_to_charf(ch, "\r\n&cATTRIBS: You have gained in &C%s&c (%d)&n\r\n\r\n", attributes[attrib], (int)((ch->real_abils.precision + 1) / 100));
		ch->real_abils.precision = LIMIT(ch->real_abils.precision + 1, min_attrib, max_attrib);
		break;
	case ATR_PERCEPTION:
		if (ch->real_abils.perception + 1 > max_attrib) {
			ch->real_abils.perception = max_attrib;
			return;
		}
		if ((int)((ch->real_abils.perception + 1) / 100) > (int)((ch->real_abils.perception) / 100))
			send_to_charf(ch, "\r\n&cATTRIBS: You have gained in &C%s&c (%d)&n\r\n\r\n", attributes[attrib], (int)((ch->real_abils.perception + 1) / 100));
		ch->real_abils.perception = LIMIT(ch->real_abils.perception + 1, min_attrib, max_attrib);
		break;
	case ATR_HEALTH:
		if (ch->real_abils.health + 5 > 32000) {
			ch->real_abils.health = 32000;
			return;
		}
		if ((int)((ch->real_abils.health + 5) / 100) > (int)((ch->real_abils.health) / 100))
			send_to_charf(ch, "\r\n&cATTRIBS: You have gained in &C%s&c (%d)&n\r\n\r\n", attributes[attrib], (int)((ch->real_abils.health + 5) / 100));
		ch->real_abils.health = LIMIT(ch->real_abils.health + 5, min_attrib, 32000);
		GET_MAX_HIT(ch) = ch->real_abils.health / 100;
		break;
	case ATR_WILLPOWER:
		if (ch->real_abils.willpower + 1 > max_attrib) {
			ch->real_abils.willpower = max_attrib;
			return;
		}
		if ((int)((ch->real_abils.willpower + 1) / 100) > (int)((ch->real_abils.willpower) / 100))
			send_to_charf(ch, "\r\n&cATTRIBS: You have gained in &C%s&c (%d)&n\r\n\r\n", attributes[attrib], (int)((ch->real_abils.willpower + 1) / 100));
		ch->real_abils.willpower = LIMIT(ch->real_abils.willpower + 1, min_attrib, max_attrib);
		gain_attrib(ch, ATR_HEALTH);
		break;
	case ATR_INTELLIGENCE:
		if (ch->real_abils.intelligence + 1 > max_attrib) {
			ch->real_abils.intelligence = max_attrib;
			return;
		}
		if ((int)((ch->real_abils.intelligence + 1) / 100) > (int)((ch->real_abils.intelligence) / 100))
			send_to_charf(ch, "\r\n&cATTRIBS: You have gained in &C%s&c (%d)&n\r\n\r\n", attributes[attrib], (int)((ch->real_abils.intelligence + 1) / 100));
		ch->real_abils.intelligence = LIMIT(ch->real_abils.intelligence + 1, min_attrib, max_attrib);
		break;
	case ATR_CHARISMA:
		if (ch->real_abils.charisma + 1 > max_attrib) {
			ch->real_abils.charisma = max_attrib;
			return;
		}
		if ((int)((ch->real_abils.charisma + 1) / 100) > (int)((ch->real_abils.charisma) / 100))
			send_to_charf(ch, "\r\n&cATTRIBS: You have gained in &C%s&c (%d)&n\r\n\r\n", attributes[attrib], (int)((ch->real_abils.charisma + 1) / 100));
		ch->real_abils.charisma = LIMIT(ch->real_abils.charisma + 1, min_attrib, max_attrib);
		break;
	case ATR_LUCK:
		if (ch->real_abils.luck + 1 > max_attrib) {
			ch->real_abils.luck = max_attrib;
			return;
		}
		if ((int)((ch->real_abils.luck + 1) / 100) > (int)((ch->real_abils.luck) / 100))
			send_to_charf(ch, "\r\n&cATTRIBS: You have gained in &C%s&c (%d)&n\r\n\r\n", attributes[attrib], (int)((ch->real_abils.luck + 1) / 100));
		ch->real_abils.luck = LIMIT(ch->real_abils.luck + 1, min_attrib, max_attrib);
		break;
	case ATR_ESSENCE:
		if (ch->real_abils.essence + 1 > max_attrib) {
			ch->real_abils.essence = max_attrib;
			return;
		}
		if ((int)((ch->real_abils.essence + 1) / 100) > (int)((ch->real_abils.essence) / 100))
			send_to_charf(ch, "\r\n&cATTRIBS: You have gained in &C%s&c (%d)&n\r\n\r\n", attributes[attrib], (int)((ch->real_abils.essence + 1) / 100));
		ch->real_abils.essence = LIMIT(ch->real_abils.essence + 1, min_attrib, max_attrib);
		break;
	}
	ch->aff_abils = ch->real_abils;
}


int get_attrib(struct char_data *ch, int attrib)
{
	switch (attrib) {
	case ATR_STRENGTH:
		return (ch->real_abils.strength / 100);
	case ATR_AGILITY:
		return (ch->real_abils.agility / 100);
	case ATR_PRECISION:
		return (ch->real_abils.precision / 100);
	case ATR_PERCEPTION:
		return (ch->real_abils.precision / 100);
	case ATR_HEALTH:
		return (ch->real_abils.health / 100);
	case ATR_WILLPOWER:
		return (ch->real_abils.willpower / 100);
	case ATR_INTELLIGENCE:
		return (ch->real_abils.intelligence / 100);
	case ATR_CHARISMA:
		return (ch->real_abils.charisma / 100);
	case ATR_LUCK:
		return (ch->real_abils.luck / 100);
	case ATR_ESSENCE:
		return (ch->real_abils.essence / 100);
	default:
		break;
	}
	return (NOTHING);
}


int get_real_attrib(struct char_data *ch, int attrib)
{
	switch (attrib) {
	case ATR_STRENGTH:
		return (ch->real_abils.strength);
	case ATR_AGILITY:
		return (ch->real_abils.agility);
	case ATR_PRECISION:
		return (ch->real_abils.precision);
	case ATR_PERCEPTION:
		return (ch->real_abils.precision);
	case ATR_HEALTH:
		return (ch->real_abils.health);
	case ATR_WILLPOWER:
		return (ch->real_abils.willpower);
	case ATR_INTELLIGENCE:
		return (ch->real_abils.intelligence);
	case ATR_CHARISMA:
		return (ch->real_abils.charisma);
	case ATR_LUCK:
		return (ch->real_abils.luck);
	case ATR_ESSENCE:
		return (ch->real_abils.essence);
	default:
		break;
	}
	return (NOTHING);
}


/*
 * This simply calculates the backstab multiplier based on a character's
 * skill level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int	backstab_mult(struct char_data *ch)
{
	int skill = (GET_SKILL(ch, SKILL_BACKSTAB)/100);

	if (IS_IMMORTAL(ch))
		return 20;

	if (skill <= 1)
		return 1;   /* <= 1% */
	else if (skill <= 10)
		return 2;	  /* <= 10% */
	else if (skill <= 30)
		return 3;	  /* <= 30% */
	else if (skill <= 60)
		return 4;	  /* <= 60% */
	else if (skill <= 90)
		return 5;	  /* <= 90% */
	else 
		return 6;	  /* <= 100% */
}


int	compare_skills(const void *x, const void *y)
{
	int	a = *(const int *)x,
			b = *(const int *)y;

	return strcmp(skill_info[a].name, skill_info[b].name);
}


void sort_skills(void)
{
	int a;

	/* initialize array, avoiding reserved. */
	for (a = 1; a <= MAX_SKILLS; a++)
		skill_sort_info[a] = a;

	qsort(&skill_sort_info[1], MAX_SKILLS, sizeof(int), compare_skills);
}


/* Set the skill base of a startup skill */
int set_skill_base(struct char_data *ch, int skl)
{
	int skillno = 0;

	if (!ch->desc)
		return (0);

	if (IS_NPC(ch))
		return (0);

	if (!(skillno = find_skill(skl)))
		return (0);

	return (get_attrib(ch, SKTREE.attrib) * 75);
}


/* Set the skill base of all basic skills, including skill groups */
void assign_skill_bases(struct char_data *ch)
{
	int skillnum = 0, skillno = 0, qend = 0;

	if (!ch->desc)
		return;

	if (IS_NPC(ch))
		return;

	for (qend = 0, skillnum = 0; skillnum <= TOP_SKILL_DEFINE; skillnum++) {
		skillno = find_skill(skillnum);
		if (SKINFO.name == unused_spellname)	/* This is valid. */
			continue;
		if ((SKTREE.pre == -1 && SKTREE.attrib != NOTHING) || (SKTREE.pre < NUM_GROUPS && SKTREE.level <= 1 && SKTREE.attrib != NOTHING && SKTREE.selectable == NO))
			GET_SKILL(ch, skillnum) = get_attrib(ch, SKTREE.attrib) * 25;
	}

}


EVENTFUNC(skill_event)
{
	struct skill_event_obj *seo = (struct skill_event_obj *) event_obj;
	struct char_data *ch;
	int type;

	ch = seo->ch;			/* pointer to ch			*/
	type = seo->type;	/* skill type					*/
	--seo->time;			/* subtract from time	*/

	switch(type) {
	case EVENT_SKILL:
		if (seo->time <= 0) {
			/*
			 * The event time has expired, now we call the proper
			 * function and clean up.
			 */
			skill_result(ch, seo->vict, seo->obj, seo->argument, seo->skill, seo->success, seo->subcmd);
			free(event_obj);
			GET_PLAYER_EVENT(ch, type) = NULL;
			return (0);
		}
		break;
	default:
		extended_mudlog(NRM, SYSL_GENERAL, TRUE, "Invalid type #%d passed to %s, %s:%d.", type, __FILE__, __FUNCTION__, __LINE__);
		return (0);
	}

	return (PULSE_SKILL);

}


EVENTFUNC(vict_delay)
{
	struct delay_event_obj *seo = (struct delay_event_obj *) event_obj;
	struct char_data *vict;
	int type;

	vict = seo->vict;	/* pointer to vict		*/
	type = seo->type;	/* skill type					*/
	--seo->time;			/* subtract from time	*/

	switch(type) {
	case EVENT_SKILL:
		if (seo->time <= 0) {
			free(event_obj);
			GET_PLAYER_EVENT(vict, type) = NULL;
			return (0);
		}
		break;
	default:
		extended_mudlog(NRM, SYSL_GENERAL, TRUE, "Invalid type #%d passed to %s, %s:%d.", type, __FILE__, __FUNCTION__, __LINE__);
		free(event_obj);
		GET_PLAYER_EVENT(vict, type) = NULL;
		return (0);
	}

	return (PULSE_SKILL);
}


/*
 * Skill wrapper for event controlled skill execution.
 * By Torgny Bjers <artovil@arcanerealms.org>, 2002-06-14
 */
ACMD(do_skills)
{
	struct obj_data *obj = NULL;
	struct char_data *vict = NULL;
	int skillno, check = TRUE, success = FALSE;
	char method[MAX_INPUT_LENGTH], hometown[MAX_INPUT_LENGTH];
	int skillcmd = 0, home;

	skip_spaces(&argument);

	/*
	 * First, find the proper skillno as a sanity check, and if we cannot find
	 * it, abort and log the error.
	 */
	if (!(skillno = find_skill(subcmd))) {
		send_to_char("You have no idea how to do that.\r\n", ch);
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unknown skill #%d passed to %s, %s:%d.", subcmd, __FILE__, __FUNCTION__, __LINE__);
		return;
	}

	/*
	 * You may only use your skills when in character.
	 */
	if (IS_OOC(ch)) {
		send_to_char("You cannot use skills when OOC.\r\n", ch);
		return;
	}

	/*
	 * Mob "skills" has to be handled as spec_procs, since they do not
	 * have the skill structure.
	 */
	if (IS_NPC(ch) || !GET_SKILL(ch, SKTREE.skill)) {
		send_to_char("You have no idea how to do that.\r\n", ch);
		return;
	}

	if (GET_PLAYER_EVENT(ch, EVENT_SKILL)) {
		/*
		 * The player already has an action event, and thus, has to wait
		 * until the event has been completed or cancelled.
		 */
		send_to_char("You cannot use another skill right now.\r\n", ch);
		return;
	} else {
		/*
		 * Preliminary setup for special skills, here you can place messages to
		 * room, affect changes, and other things that should be taken care of
		 * before the skill will be put on queue as an event. (TB, 2002-06-13)
		 */
		switch (SKTREE.skill) {

		case SKILL_BANDAGE:
			// Find a bandage object in the character's inventory.
			obj = get_obj_in_list_vis(ch, "bandage", NULL, ch->carrying);
			argument = one_argument(argument, arg);
			if (!*arg) {
				send_to_char("Apply bandages on who?\r\n", ch);
				return;
			}
			// If we couldn't find the victim we abort.
			if ((vict = bandage_vict(ch, arg)) == NULL)
				return;
			// Mobiles cannot be bandaged.
			if (IS_NPC(vict)) {
				send_to_char("Mobs don't need bandages.\r\n", ch);
				return;
			}
			// If there are no bandages in inventory we abort.
			if (!obj) {
				send_to_char("You cannot bandage without bandages.\r\n", ch); 
				return;
			}
			// No need to bandage somebody that is at full health.
			if (GET_HIT(vict) >= GET_MAX_HIT(vict)) {
				if (ch == vict)
					send_to_char("You are already healthy enough!\r\n", ch);
				else
					send_to_char("Your patient is already healthy enough!\r\n", ch);
				return;
			}
			if (ch == vict) {
				act("You begin to bandage yourself with $p.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n begins to apply bandages on $mself with $p.", FALSE, ch, obj, 0, TO_ROOM);
			} else {
				act("You begin to apply bandages on $N with $p.", FALSE, ch, obj, vict, TO_CHAR);
				act("$n begins to apply bandages on you with $p.", FALSE, ch, obj, vict, TO_VICT);
				act("$n begins to apply bandages on $N with $p.", TRUE, ch, obj, vict, TO_NOTVICT);
			}
			break;

		case SKILL_HIDE:
			if (AFF_FLAGGED(ch, AFF_HIDE))
				REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
			if (FIGHTING(ch)) {
				send_to_char("You can't hide in the middle of a fight!\r\n", ch);
				return;
			}
			break;

		case SKILL_SNEAK:
			if (AFF_FLAGGED(ch, AFF_SNEAK))
				affect_from_char(ch, SKILL_SNEAK);
			break;

		case SKILL_FIND_HIDDEN:
			act("$n scrutinizes $s surroundings.", FALSE, ch, 0, 0, TO_ROOM);
			break;

		case SKILL_SCAN:
			if (IS_AFFECTED(ch, AFF_BLIND)) {
				act("You can't see anything, you're blind!", TRUE, ch, 0, 0, TO_CHAR);
				return;
			}
			if ((GET_MOVE(ch) < 2) && IS_MORTAL(ch)) {
				act("You are too exhausted.", TRUE, ch, 0, 0, TO_CHAR);
				return;
			}
			act("$n scans the area.", FALSE, ch, 0, 0, TO_ROOM);
			break;

		case GROUP_LORE:
			if (!*argument) {
				send_to_char("Usage: lore <method>\r\n", ch);
				return;
			}
			// Look for a lore method argument.
			half_chop(argument, method, buf);
			if (!*method) {
				send_to_char("You must provide a lore method first!\r\n", ch);
				return;
			}
			for (skillcmd = 0; skillcmd < NUM_LORES; skillcmd++)
				if (!strncmp(method, lore_methods[skillcmd], strlen(method)))
					break;
			switch (skillcmd) {
			case LORE_BOTANY:
				skillno = find_skill(SKILL_BOTANY);
				send_to_char("This lore method is not yet implemented.\r\n", ch);
				return;
			case LORE_GEOGRAPHY:
				skillno = find_skill(SKILL_GEOGRAPHY);
				send_to_char("This lore method is not yet implemented.\r\n", ch);
				return;
			case LORE_ARCANE:
				skillno = find_skill(SKILL_ARCANE_LORE);
				send_to_char("This lore method is not yet implemented.\r\n", ch);
				return;
			case LORE_THEOLOGY:
				skillno = find_skill(SKILL_THEOLOGY);
				send_to_char("This lore method is not yet implemented.\r\n", ch);
				return;
			case LORE_ZOOLOGY:
				skillno = find_skill(SKILL_ZOOLOGY);
				send_to_char("This lore method is not yet implemented.\r\n", ch);
				return;
			case LORE_LINGUISTICS:
				skillno = find_skill(SKILL_LINGUISTICS);
				send_to_char("This lore method is not yet implemented.\r\n", ch);
				return;
			case LORE_SURVIVAL:
				skillno = find_skill(SKILL_SURVIVAL);
				send_to_char("This lore method is not yet implemented.\r\n", ch);
				return;
			case LORE_REALMS:
				skillno = find_skill(SKILL_REALM_LORE);
				send_to_char("This lore method is not yet implemented.\r\n", ch);
				return;
			case LORE_OCCULT:
				skillno = find_skill(SKILL_OCCULT);
				send_to_char("This lore method is not yet implemented.\r\n", ch);
				return;
			case LORE_ANTHROPOLOGY:
				skillno = find_skill(SKILL_ANTHROPOLOGY);
				send_to_char("This lore method is not yet implemented.\r\n", ch);
				return;
			case LORE_STREETWISE:
				half_chop(buf, hometown, buf);
				if (!*hometown) {
					send_to_char("Usage: lore streetwise <hometown>\r\n", ch);
					send_to_char("Where hometown is one of the following:\r\n", ch);
					for (home = 1; home <= NUM_MORTAL_STARTROOMS; home++) {
						sprintf(buf, "&W%s&n\r\n", hometowns[home]);
						send_to_char(buf, ch);
					}
					return;
				}
				skillno = find_skill(SKILL_STREETWISE);
				break;
			default:
				send_to_char("That is not a known lore method.\r\n", ch);
				return;
			}
			break;

		case SKILL_FISHING:
		case SKILL_HUNTING:
		case SKILL_GATHER:
		case SKILL_LUMBERJACK:
		case SKILL_MINING:
			// Make sure they have enough move points to forage.
			if (GET_MOVE(ch) < GET_MAX_MOVE(ch) / 20) {
				send_to_char("You are too weary right now.\r\n", ch);
				return;
			}
			if (IS_MORTAL(ch) && IS_DARK(IN_ROOM(ch))) {
				send_to_char("It is dark outside, you have to wait until morning.\r\n", ch);
				return;
			} else {
				struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
				struct obj_data *held = GET_EQ(ch, WEAR_HOLD);

				switch (subcmd) {
				case SKILL_FISHING:
					skillcmd = SCMD_FORAGE_FISHING;
					// Has to be in the right sector to fish.
					if (SECT(IN_ROOM(ch)) != SECT_SHORE) {
						sprintf(buf, "You cannot %s here.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					// Check for a held item.
					if (!held) {
						sprintf(buf, "You have to hold a fishing-rod to %s.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					// Check for proper tool.
					if (held && !(isname("fishing-rod", GET_OBJ_NAME(held)))) {
						sprintf(buf, "You cannot %s without holding a fishing-rod.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					break;
				case SKILL_HUNTING:
					skillcmd = SCMD_FORAGE_HUNTING;
					// Has to be in the right sector to hunt.
					if (SECT(IN_ROOM(ch)) != SECT_FIELD && SECT(IN_ROOM(ch)) != SECT_FOREST &&
						SECT(IN_ROOM(ch)) != SECT_MOUNTAIN) {
						sprintf(buf, "You cannot %s here.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					// Check for a wielded item.
					if (!held) {
						sprintf(buf, "You have to hold a bow to %s.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					// Check for proper tool.
					if (held && !(isname("hunting-bow", GET_OBJ_NAME(held)))) {
						sprintf(buf, "You cannot %s without holding a bow.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					break;
				case SKILL_GATHER:
					skillcmd = SCMD_FORAGE_GATHER;
					// Has to be in the right sector to gather.
					if (SECT(IN_ROOM(ch)) != SECT_FIELD && SECT(IN_ROOM(ch)) != SECT_FOREST &&
						SECT(IN_ROOM(ch)) != SECT_HILLS && SECT(IN_ROOM(ch)) != SECT_HIGHWAY) {
						sprintf(buf, "You cannot %s here.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					// Cannot be winter when we gather, since nothing will live.
					if (in_season(SEASON_WINTER)) {
						sprintf(buf, "You cannot %s during winter.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					break;
				case SKILL_LUMBERJACK:
					skillcmd = SCMD_FORAGE_LUMBERJACK;
					// Has to be in the right sector to lumberjack.
					if (SECT(IN_ROOM(ch)) != SECT_FOREST && SECT(IN_ROOM(ch)) != SECT_MOUNTAIN) {
						sprintf(buf, "You cannot %s here.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					// Check for a wielded weapon.
					if (!wielded) {
						sprintf(buf, "You have to wield an axe to %s.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					// Check for proper tool.
					if (wielded && !(isname("lumberjacker", GET_OBJ_NAME(wielded)) && isname("axe", GET_OBJ_NAME(wielded)))) {
						sprintf(buf, "You are not wielding the proper type of axe for %sing.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					// Check for available resources.
					if (GET_ROOM_RESOURCE(IN_ROOM(ch), RESOURCE_WOOD) < 6) {
						sprintf(buf, "It seems that you cannot %s further here!\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					break;
				case SKILL_MINING:
					skillcmd = SCMD_FORAGE_MINING;
					// Has to be in the right sector to mine.
					if (SECT(IN_ROOM(ch)) != SECT_MOUNTAIN) {
						sprintf(buf, "You cannot %s here.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					// Check for a wielded weapon.
					if (!wielded) {
						sprintf(buf, "You have to wield a pickaxe to %s.\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					// Check for proper tool.
					if (wielded && !(isname("pickaxe", GET_OBJ_NAME(wielded)))) {
						send_to_char("You are not holding the proper type of tool for mining.\r\n", ch);
						return;
					}
					// Check for available resources.
					if (GET_ROOM_RESOURCE(IN_ROOM(ch), RESOURCE_ORE) < 3) {
						sprintf(buf, "It seems that you cannot %s further here!\r\n", forage_skill[skillcmd]);
						send_to_char(buf, ch);
						return;
					}
					break;
				}

			}

			break;

		case SKILL_TEACHING:
			argument = one_argument(argument, arg);
			if (!*arg) {
				send_to_char("Tutor who?\r\n", ch);
				return;
			}
			/* If we couldn't find the victim we abort. */
			if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM, 0))) {
				send_to_char(NOPERSON, ch);
				return;
			} else {
				char *sklname = get_buffer(MAX_INPUT_LENGTH);
				int qend = 0, sklnum = 0;
				skip_spaces(&argument);
				/* If there are no chars in argument */
				if (!*argument) {
					send_to_char("Skill name expected.\r\n", ch);
					release_buffer(sklname);
					return;
				}
				if (*argument != '\'') {
					send_to_char("Skill must be enclosed in: ''\r\n", ch);
					release_buffer(sklname);
					return;
				}
				/* Locate the last quote and lowercase the magic words (if any) */
				for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
					argument[qend] = LOWER(argument[qend]);
				if (argument[qend] != '\'') {
					send_to_char("Skill must be enclosed in: ''\r\n", ch);
					release_buffer(sklname);
					return;
				}
				strcpy(sklname, (argument + 1));
				sklname[qend - 1] = '\0';
				if ((sklnum = find_skill_num(sklname)) <= 0) {
					send_to_char("Unrecognized skill.\r\n", ch);
					release_buffer(sklname);
					return;
				}
				strcpy(argument, sklname);
				release_buffer(sklname);
				if (GET_SKILL(ch, sklnum) < 5000) {
					send_to_char("You are not sufficiently proficient in that skill.\r\n", ch);
					return;
				}
				if (GET_SKILL(vict, sklnum) > GET_SKILL(ch, sklnum) / 2) {
					act("You cannot teach $N further in that skill!", FALSE, ch, 0, vict, TO_CHAR);
					act("$n cannot teach you further in that skill!", FALSE, ch, 0, vict, TO_VICT);
					return;
				}
			}
			act("You begin to tutor $N.", FALSE, ch, 0, vict, TO_CHAR);
			act("$n begins to tutor you.", FALSE, ch, 0, vict, TO_VICT);
			act("$n begins to tutor $N.", TRUE, ch, 0, vict, TO_NOTVICT);
			break;

		/*
		 * Only resource objects of the type fabric can be dyed.
		 * Torgny Bjers (artovil@arcanerealms.org) 2002-11-15
		 */
		case SKILL_DYEING:
			if (!*argument) {
				send_to_char("Usage: dye <vat name> <fabric name>\r\n", ch);
				return;
			} else {
				char vat_name[MAX_INPUT_LENGTH];
				char garb_name[MAX_INPUT_LENGTH];
				// Look for an object name.
				half_chop(argument, vat_name, buf);
				if (!*vat_name) {
					send_to_char("You must specify which item you get your dye from!\r\n", ch);
					return;
				}
				if (!(obj = get_obj_in_list_vis(ch, vat_name, NULL, ch->carrying))) {
					send_to_char("You have no such object in your inventory.\r\n", ch);
					return;
				}
				// Make some checks on the vat object.
				if (GET_OBJ_TYPE(obj) != ITEM_DRINKCON) {
					act("$p does not appear to be a liquid container.", FALSE, ch, obj, 0, TO_CHAR);
					return;
				}
				if (GET_OBJ_VAL(obj, 2) != LIQ_COLOR) {
					act("$p does not appear to contain dye.", FALSE, ch, obj, 0, TO_CHAR);
					return;
				}
				if (GET_OBJ_VAL(obj, 1) < 1) {
					act("$p appears to be empty, fill it with a dye mix first.", FALSE, ch, obj, 0, TO_CHAR);
					return;
				}
				if (GET_OBJ_COLOR(obj) == 0) {
					act("$p appears to contain something, but it has no color.", FALSE, ch, obj, 0, TO_CHAR);
					return;
				}
				// Find the reosurce object to dye.
				half_chop(buf, garb_name, buf);
				if (!*garb_name) {
					send_to_char("Usage: dye <vat name> <fabric name>\r\n", ch);
					send_to_char("You must specify which fabric you wish to dye!\r\n", ch);
					return;
				} else {
					struct obj_data *target;
					int col_requirement = 1;
					if (!(target = get_obj_in_list_vis(ch, garb_name, NULL, ch->carrying))) {
						send_to_char("You have no such fabric in your inventory.\r\n", ch);
						return;
					}
					if (target == obj) {
						act("You cannot dye $p with itself, silly.", FALSE, ch, target, 0, TO_CHAR);
						return;
					}
					if (GET_OBJ_TYPE(target) != ITEM_RESOURCE) {
						act("$p does not appear to be something you can dye.", FALSE, ch, target, 0, TO_CHAR);
						return;
					}
					if (GET_OBJ_VAL(target, 0) != RESOURCE_FABRIC) {
						act("$p does not appear to be a fabric.", FALSE, ch, target, 0, TO_CHAR);
						return;
					}
					if (!OBJ_FLAGGED(target, ITEM_COLORIZE)) {
						act("$p cannot be dyed.\r\n", FALSE, ch, target, 0, TO_CHAR);
						return;
					}
					if (GET_OBJ_COLOR(target) > 0) {
						act("$p has already been dyed once.", FALSE, ch, target, 0, TO_CHAR);
						return;
					}
					if (GET_OBJ_SIZE(target) > 0)
						col_requirement = GET_OBJ_SIZE(target);

					if (col_requirement > GET_OBJ_VAL(obj, 1)) {
						act("$p does not contain enough color to dye $P.", FALSE, ch, obj, target, TO_CHAR);
						return;
					}
					act("$n begins to dye something...", FALSE, ch, target, 0, TO_ROOM);
				}
			}
			break;

		case SKILL_STEAL:
			/*
			 * There's some duplicate code in here and in skl_steal,
			 * which was sort of necessary to prevent the player from
			 * gaining off of objects that didn't even exist on the
			 * target player.
			 * Torgny Bjers (artovil@arcanerealms.org), 2003-01-13.
			 */
			if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
				send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
				return;
			}
			if (!*argument) {
				send_to_char("Usage: steal <object> <person>\r\n", ch);
				return;
			} else {
				char obj_name[MAX_INPUT_LENGTH];
				char vict_name[MAX_INPUT_LENGTH];
				int eq_pos = 0;
				two_arguments(argument, obj_name, vict_name);
				// Find the victim first.
				if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM, 0))) {
					send_to_char("Steal what from who?\r\n", ch);
					return;
				} else if (vict == ch) {
					send_to_char("Come on now, that's rather stupid!\r\n", ch);
					return;
				}
				// Find the target object.
				if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {
					if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying))) {
						// The object was not present in inventory.
						for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++) {
							if (HAS_BODY(vict, eq_pos) && GET_EQ(vict, eq_pos) &&
								(isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
								CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
								obj = GET_EQ(vict, eq_pos);
								break;
							}
						}
						if (!obj) {
							// No object found at all.
							act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
							return;
						} else {
							// It existed in inventory.
							if ((GET_POS(vict) > POS_STUNNED)) {
								// Trying to steal from somebody awake.
								send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
								return;
							}
						}
					}
				}
			}
			break;

		}

		/*
		 * If no message can be found in the skill_tree, just send OK.
		 */
		if (SKTREE.message)
			send_to_char(SKTREE.message, ch);
		else
			send_to_char(OK, ch);

		if (SKTREE.time > 0) {
			struct skill_event_obj *seo;
			CREATE(seo, struct skill_event_obj, 1);
			seo->ch = ch;																				/* pointer to ch									*/
			seo->vict = vict;																		/* pointer to ch									*/
			seo->obj = obj;																			/* pointer to ch									*/
			seo->type = EVENT_SKILL;														/* action type										*/
			seo->time = SKTREE.time;														/* event time * PULSE_SKILL				*/
			seo->skill = SKTREE.skill;													/* the skill we are using					*/
			seo->vict_delay = SKTREE.vict_delay;								/* delay victim during execution?	*/
			if (check)
				seo->success = skill_check(ch, SKTREE.skill, 0);	/* skill roll as boolean					*/
			else
				seo->success = TRUE;
			seo->argument = strdup(argument);										/* argument cannot be local ref		*/
			seo->subcmd = skillcmd;															/* sub command										*/
			GET_PLAYER_EVENT(ch, EVENT_SKILL) = event_create(skill_event, seo, PULSE_SKILL);
			/*
			 * Do we delay the victim until event occurs?
			 * If so, then we create a vict_delay event for them.
			 * Torgny Bjers <artovil@arcanerealms.org>, 2002-06-14
			 */
			if (vict && seo->vict_delay) {
				struct delay_event_obj *deo;
				CREATE(deo, struct delay_event_obj, 1);
				deo->vict = vict;																	/* pointer to ch									*/
				deo->type = EVENT_SKILL;													/* action type										*/
				deo->time = SKTREE.time;													/* event time * PULSE_SKILL				*/
				GET_PLAYER_EVENT(vict, EVENT_SKILL) = event_create(vict_delay, deo, PULSE_SKILL);
			}
		} else {
			/*
			 * Skills that are instant have 0 time, and thus, are not passed as events.
			 * Torgny Bjers <artovil@arcanerealms.org>, 2002-06-13.
			 */
			if (check)
				success = skill_check(ch, SKTREE.skill, 0);
			else
				success = TRUE;
			skill_result(ch, vict, obj, argument, SKTREE.skill, success, skillcmd);
		}
	}

}


void skill_result(struct char_data *ch, struct char_data *vict, struct obj_data *obj, char *argument, int skill, bool success, int subcmd)
{
	/*
	 * Call the proper skill function when the event has occured.
	 */
	switch (skill) {
	case SKILL_BANDAGE:
		skl_bandage(ch, vict, obj, argument, success, subcmd);
		break;
	case SKILL_HIDE:
		skl_hide(ch, vict, obj, argument, success, subcmd);
		break;
	case SKILL_SNEAK:
		skl_sneak(ch, vict, obj, argument, success, subcmd);
		break;
	case SKILL_FIND_HIDDEN:
		skl_find_hidden(ch, vict, obj, argument, success, subcmd);
		break;
	case SKILL_SCAN:
		skl_scan(ch, vict, obj, argument, success, subcmd);
		break;
	case SKILL_STREETWISE:
		skl_streetwise(ch, vict, obj, argument, success, subcmd);
		break;
	case SKILL_TEACHING:
		skl_tutor(ch, vict, obj, argument, success, subcmd);
		break;
	case SKILL_HUNTING:
	case SKILL_FISHING:
	case SKILL_LUMBERJACK:
	case SKILL_MINING:
	case SKILL_GATHER:
		skl_forage(ch, vict, obj, argument, success, subcmd);
		break;
	case SKILL_DYEING:
		skl_dyeing(ch, vict, obj, argument, success, subcmd);
		break;
	case SKILL_STEAL:
		skl_steal(ch, vict, obj, argument, success, subcmd);
		break;
	default:
		extended_mudlog(NRM, SYSL_GENERAL, TRUE, "Unknown skill #%d passed to %s, %s:%d.", skill, __FILE__, __FUNCTION__, __LINE__);
		return;
	}
}


int skill_cap(struct char_data *ch, int gain, bool show_bonus)
{
	int skillnum = 0, skillno = 0, total = 0;

	if (!ch->desc)
		return (0);

	if (IS_NPC(ch))
		return (0);

	for (skillnum = 0; skillnum <= TOP_SKILL_DEFINE; skillnum++) {
		skillno = find_skill(skillnum);
		if (SKINFO.name == unused_spellname)
			continue;
		if (SKTREE.skill >= MIN_LANGUAGES && SKTREE.skill <= MAX_LANGUAGES)
			continue;
		if (SKTREE.skill <= NUM_GROUPS)
			continue;
		if (show_bonus)
			total += LIMIT(GET_SKILL(ch, SKTREE.skill) + ((GET_SKILL(ch, SKTREE.skill)) ? (get_real_attrib(ch, SKTREE.attrib) / 4) : 0), 0 , 10000);
		else
			total += LIMIT(GET_SKILL(ch, SKTREE.skill), 0 , 10000);
	}

	total += gain;

	return (total);
}


bool decrease_skills(struct char_data *ch, int gain)
{
	int skillnum, skl = 0, skillbase = 0;

	if (!ch->desc)
		return (FALSE);

	if (IS_NPC(ch))
		return (FALSE);

	for (skillnum = 1; skillnum <= MAX_SKILLS; skillnum++) {
		skillbase = set_skill_base(ch, skillnum);
		if (GET_SKILL_STATE(ch, skillnum) == SSTATE_DECREASE && GET_SKILL(ch, skillnum) > skillbase) {
			skl = GET_SKILL(ch, skillnum);
			GET_SKILL(ch, skillnum) = LIMIT(skl - gain, skillbase, 10000);
			if (PRF_FLAGGED(ch, PRF_SKILLGAINS)) {
				if ((skl - gain) / 100 < skl / 100) {
					char *sklmsg = get_buffer(256);
					sprintf(sklmsg, "\r\n&cSKILLS: &C%s&c have decreased. (%d%%)&n\r\n\r\n", skill_name(skillnum), (skl - gain) / 100);
					send_to_char(sklmsg, ch);
					release_buffer(sklmsg);
				}
			}
			return (TRUE);
		}
	}

	return (FALSE);
}


ACMD(do_skill_state)
{
	char *args;
	int qend = 0, skillnum = 0, state = SKILL_STATES;

	if (!ch->desc)
		return;

	if (IS_NPC(ch))
		return;

	skip_spaces(&argument);

	/* If there are no chars in argument */
	if (!*argument) {
		send_to_char("&RSkill name expected.&n\r\nUsage: skillstate '<skill name>' [- | # | +]\r\n", ch);
		return;
	}

	/* Locate the first quote */
	if (*argument != '\'') {
		send_to_char("&RSkill must be enclosed in: ''&n\r\nUsage: skillstate '<skill name>' [- | # | +]\r\n", ch);
		return;
	}

	/* Locate the last quote and lowercase the magic words (if any) */
	for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
		argument[qend] = LOWER(argument[qend]);

	if (argument[qend] != '\'') {
		send_to_char("&RSkill must be enclosed in: ''&n\r\nUsage: skillstate '<skill name>' [- | # | +]\r\n", ch);
		return;
	}

	args = get_buffer(MAX_INPUT_LENGTH);

	/* Locate the skill state, either +, -, or # */
	strcpy(args, (argument + 1));
	args[qend - 1] = '\0';
	if ((skillnum = find_skill_num(args)) <= 0) {
		send_to_char("&RUnrecognized skill.&n\r\nUsage: skillstate '<skill name>' [- | # | +]\r\n", ch);
		release_buffer(args);
		return;
	}

	one_argument(argument + qend + 1, args);

	for (state = 0; state < SKILL_STATES; state++)
		if (*args == skill_state[state].symbol || !strncmp(skill_state[state].name, args, strlen(args)))
			break;

	if (state == SKILL_STATES) {
		send_to_char("&RUnrecognized skill state.&n\r\nUsage: skillstate '<skill name>' [- | # | +]\r\n", ch);
		return;
		release_buffer(args);
	}

	GET_SKILL_STATE(ch, skillnum) = skill_state[state].state;
	sprintf(args, "Skill [&W%s&n] has been set to [%s%s&n]\r\n", skill_name(skillnum), skill_state[state].color, skill_state[state].name);
	send_to_char(args, ch);

	release_buffer(args);
}


/*
 * Function to show skill titles for a character.
 * Probably shouldn't be called for an imm, since it will
 * spam the screen with all the grand-master listings.
 * Torgny Bjers (artovil@arcanerealms.org), 2002-12-29
 */
void list_skill_titles(struct char_data *ch, bool show_title)
{
	int skillnum, qend = 0, skillno, gotskills = 0;
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char *skills = get_buffer(MAX_STRING_LENGTH);
	char *skillname = get_buffer(128);
	char *level = get_buffer(128);

	for (qend = 0, skillnum = 0; skillnum <= TOP_SKILL_DEFINE; skillnum++) {
		skillno = find_skill(skillnum);
		if (SKINFO.name == unused_spellname)	/* This is valid. */
			continue;
		if (SKTREE.skill >= MIN_LANGUAGES && SKTREE.skill < MAX_LANGUAGES)
			continue;
		if (SKTREE.skill <= NUM_GROUPS)
			continue;
		if (GET_SKILL(ch, SKTREE.skill) / 100 < 51)
			continue;
		gotskills++;
		sprintf(level, "%s", readable_proficiency(GET_SKILL(ch, SKTREE.skill) / 100));
		sprintf(skillname, "%s", SKINFO.name);
		sprintf(skills, "%s%s in %s\r\n", skills, CAP(level), CAP(skillname));
	}

	release_buffer(skillname);
	release_buffer(level);

	if (gotskills) {
		if (show_title)
			sprintf(printbuf, "&c==[ &CSkill Information &c]=====================================================&n\r\n"
												"%s", skills);
		else
			sprintf(printbuf, "%s", skills);
		page_string(ch->desc, printbuf, TRUE);
	}

	release_buffer(printbuf);
	release_buffer(skills);
}


void generate_skill_points(struct char_data *ch)
{
	int points = 0;

	if (!ch)
		return;

	points = (ch->real_abils.intelligence) / 100;
	
	GET_CREATION_POINTS(ch) = points;
}


bool skill_selection_menu(struct char_data *ch, char *argument)
{
	const char *usage = "Usage: { list | help | { add | remove } '<skill name>' | done }\r\nYou MUST enclose the skillname in single quotes (').";
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	char value[8];
	int skillno, skillnum, qend = 0, i = 0;
	bool done = FALSE;
	bool selected = FALSE;

	sprintf(printbuf, "\r\n"
										"%s.----------------------------[ %sSKILL SELECTION %s]----------------------------.\r\n", QCYN, QHCYN, QCYN);
	for (skillno = 0; skillno < MAX_SKILLS; skillno++) {
		skillnum = SKTREE.skill;
		selected = FALSE;
		if (SKTREE.selectable && SKINFO.name != unused_spellname && SKTREE.pre != -1) {
			if (GET_SKILL(ch, SKTREE.skill) == 1)
				selected = TRUE;
			sprintf(value, "&K%d&n", SKTREE.cost);
			sprintf(printbuf, "%s%s[%s] %-18.18s  %s", printbuf, ((qend) % 3 == 0) ? "|&n  " : "", selected ? "&Wx&n" : value, SKINFO.name, (qend % 3 == 2) ? " &c|\r\n" : "");
			qend++;
		}
	}
	if (qend % 3 != 0) {
		for (i = qend % 3; i < 3; i++)
			strcat(printbuf, "                        ");
		strcat(printbuf, " &c|\r\n");
	}
	sprintf(printbuf, "%s'---------------------------------------------------------------------------'%s\r\n", printbuf, QNRM);
	sprintf(printbuf, "%sIf you need further help, you may use the help files at the prompt.\r\n%s\r\n\r\n", printbuf, usage);

	if (*argument) {

		char *command = get_buffer(MAX_INPUT_LENGTH);
		char *input = get_buffer(MAX_INPUT_LENGTH);

		half_chop(argument, command, input);

		if (isname(command, "add") || isname(command, "remove")) {
			char *sklname = get_buffer(MAX_INPUT_LENGTH);
			int qend = 0, sklnum = 0, gotquotes = TRUE;	
	
			if (*input != '\'')
				gotquotes = FALSE;

			if (gotquotes) {
				/* Locate the last quote and lowercase the magic words (if any) */
				for (qend = 1; input[qend] && input[qend] != '\''; qend++)
					input[qend] = LOWER(input[qend]);
			
				if (input[qend] != '\'')
					gotquotes = FALSE;
		
				if (gotquotes) {
					strcpy(sklname, (input + 1));
					sklname[qend - 1] = '\0';
					if ((sklnum = find_skill_num(sklname)) <= 0) {
						sprintf(printbuf, "\r\nUnrecognized skill.\r\n");
					} else {
						// Got a proper skill name.
						int skillno = find_skill(sklnum);
						if (SKTREE.selectable == NO || SKTREE.pre == -1) {
							// Not one of the selectable skills.
							sprintf(printbuf, "\r\nUnrecognized skill.\r\n");
						} else {
							// Got a selectable skill. Either add or remove.
							if (isname(command, "remove")) {
								// Can't remove if it hasn't been added.
								if (GET_SKILL(ch, sklnum) == 1) {
									GET_SKILL(ch, sklnum) = 0;
									GET_CREATION_POINTS(ch) += SKTREE.cost;
									sprintf(printbuf, "\r\nRemoved '%s', returning %d sp.\r\n", skill_name(sklnum), SKTREE.cost);
								} else {
									sprintf(printbuf, "\r\nCannot remove '%s' since it was never added.\r\n", skill_name(sklnum));
								}
							} else if (isname(command, "add") && GET_CREATION_POINTS(ch) - SKTREE.cost >= 0) {
								// Can't add if it's already been added.
								if (GET_SKILL(ch, sklnum) == 0) {
									GET_SKILL(ch, sklnum) = 1;
									GET_CREATION_POINTS(ch) -= SKTREE.cost;
									sprintf(printbuf, "\r\nAdded '%s', subtracting %d sp.\r\n", skill_name(sklnum), SKTREE.cost);
								} else {
									sprintf(printbuf, "\r\nThe skill '%s' has already been added.\r\n", skill_name(sklnum));
								}
							} else if (GET_CREATION_POINTS(ch) - SKTREE.cost < 0) {
								sprintf(printbuf, "\r\nYou do not have enough skill points.\r\n");
							}
						}
						// Lastly, add the prompt.
						sprintf(printbuf, "%s\r\n%d Skill Points > ", printbuf, GET_CREATION_POINTS(ch));
					}
				} else {
					sprintf(printbuf, "\r\n%s\r\n\r\n%d Skill Points > ", usage, GET_CREATION_POINTS(ch));
				}
			} else {
				sprintf(printbuf, "\r\n%s\r\n\r\n%d Skill Points > ", usage, GET_CREATION_POINTS(ch));
			}
			release_buffer(sklname);
		} else if (isname(command, "list")) {
			sprintf(printbuf, "%s%d Skill Points > ", printbuf, GET_CREATION_POINTS(ch));
		} else if (isname(command, "done")) {
			done = TRUE;
			// Assign the skills.
			for (i = 1; i <= MAX_SKILLS; i++) {
				if (GET_SKILL(ch, i) == 1) {
					skillno = find_skill(i);
					SET_SKILL(ch, i, set_skill_base(ch, i));
					SET_SKILL(ch, SKTREE.pre, set_skill_base(ch, SKTREE.pre) / 1.5);
				}
			}
			// Set the base language skill.
			SET_SKILL(ch, LANG_COMMON, 10000);
			SPEAKING(ch) = MIN_LANGUAGES;
			if (GET_CREATION_POINTS(ch) > 0) {
				GET_QP(ch) += (GET_CREATION_POINTS(ch));
				sprintf(printbuf, "\r\n%d Skill Points converted to %d Quest Points.\r\n", GET_CREATION_POINTS(ch), GET_QP(ch));
				send_to_char(printbuf, ch);
				GET_CREATION_POINTS(ch) = 0;
			}
			// Set up the initial groups.
			update_groups(ch);
		} else if (isname(command, "help")) {
			/* temporarily set rights to member for help */
			USER_RIGHTS(ch) = RIGHTS_MEMBER;
			do_help(ch, input, 0, 0);
			/* and set them back */
			USER_RIGHTS(ch) = RIGHTS_NONE;
			sprintf(printbuf, "\r\n%d Skill Points > ", GET_CREATION_POINTS(ch));
		} else {
			sprintf(printbuf, "\r\n%s\r\n\r\n%d Skill Points > ", usage, GET_CREATION_POINTS(ch));
		} 

		release_buffer(command);
		release_buffer(input);

	} else
		sprintf(printbuf, "\r\n%s\r\n\r\n%d Skill Points > ", usage, GET_CREATION_POINTS(ch));

	if (!done)
		send_to_char(printbuf, ch);
	release_buffer(printbuf);

	if (done)
		return (TRUE);

	return (FALSE);
}
