/* ************************************************************************
*		File: constants.c                                   Part of CircleMUD *
*	 Usage: Numeric and string contants used by the MUD                     *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: constants.c,v 1.113 2004/04/20 16:30:13 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "interpreter.h"	/* alias_data */

cpp_extern const char *circlemud_version =
	"CircleMUD, version 3.00 beta patchlevel 21";

/* strings corresponding to ordinals/bitvectors in structs.h ***********/

/* RIGHTS_x user rights */
const char *user_rights[] =
{
	"NONE",
	"MEMBER",
	"QUESTS",
	"GUILDS",
	"NEWBIES",
	"ACTIONS",
	"HELPFILES",
	"IMMORTAL",
	"QUESTOR",
	"PLAYERS",
	"BUILDING",
	"TRIGGERS",
	"ADMIN",
	"DEVELOPER",
	"HEADBUILDER",
	"IMPLEMENTOR",
	"OWNER",
	"\n"
};


/* SYSL_x extended syslogs */
const char *extended_syslogs[] =
{
	"BUGS",
	"CONTACT",
	"DEATHS",
	"FLAGS",
	"GENERAL",
	"OLC",
	"LINKS",
	"LOAD",
	"LOGINS",
	"MEMCHECK",
	"MOBDEATHS",
	"NEWBIES",
	"WARKILLS",
	"NOHELP",
	"PENALTIES",
	"PLAYERLOG",
	"QUESTING",
	"RENT",
	"RESETS",
	"RPEXP",
	"RPMONITOR",
	"SECURE",
	"PLAYERS",
	"SITES",
	"SNOOPS",
	"SPAM",
	"SQL",
	"SWITCHES",
	"THEFTS",
	"TICKS",
	"TRIGGERS",
	"BUFFER",
	"GUILD",
	"HOUSES",
	"\n"
};


/* SESS_x session toggles */
const char *session_toggles[] =
{
	"IC",
	"AFK",
	"AFW",
	"REPLYLOCK",
	"HAVETELLS",
	"\n"
};


/* cardinal directions */
const	char *dirs[] =
{
	"north",
	"east",
	"south",
	"west",
	"up",
	"down",
	"northeast",
	"southwest",
	"northwest",
	"southeast",
	"in",
	"out",
	"\n"
};


const	char *abbr_dirs[] =
{
	"n",
	"e",
	"s",
	"w",
	"u",
	"d",
	"ne",
	"sw",
	"nw",
	"se",
	"in",
	"out",
	"\n" /* Cannot be " ", then search_block bugs out! */
};


int	rev_dir[] =
{
 /* North */  SOUTH,
 /* East  */  WEST,
 /* South */  NORTH,
 /* West  */  EAST,
 /* Up    */  DOWN,
 /* Down  */  UP,
 /* NE    */  SOUTHWEST,
 /* SW    */  NORTHEAST,
 /* NW    */  SOUTHEAST,
 /* SE    */  NORTHWEST,
 /* In    */  OUTDIR,
 /* Out   */  INDIR
};


/* ZONE_x */
const	char *zone_bits[] = {
	"OPEN",
	"CLOSED",
	"NO_RECALL",
	"NO_SUMMON",
	"REMORTS",
	"NO_TELEPORT",
	"HOMETOWN",
	"OOC",
	"\n"
};

const	char *zone_bits_short[] = {
	"+",
	"-",
	"!R",
	"!S",
	"R",
	"!T",
	"H",
	"O",
	"\n"
};

/* Room_x */
const	char *room_bits[] = {
	"DARK",
	"DEATH",
	"NO_MOB",
	"INDOORS",
	"PEACEFUL",
	"SOUNDPROOF",
	"NO_TRACK",
	"NO_MAGIC",
	"TUNNEL",
	"PRIVATE",
	"GODROOM",
	"HOUSE",
	"HCRSH",
	"ATRIUM",
	"OLC",
	"*",				/* BFS MARK */
	"FAERIE",
	"INFERNAL",
	"DIVINE",
	"ANCIENT",
	"IS_PARSED",
	"ALWAYS_LIT",
	"NO_MAP",
	"\n"
};


/* EX_x */
const	char *exit_bits[] = {
	"DOOR",
	"CLOSED",
	"LOCKED",
	"PICKPROOF",
	"\n"
};


/* SECT_ */
const	char *sector_types[] = {
	"Inside",
	"City",
	"Field",
	"Forest",
	"Hills",
	"Mountains",
	"Water (Swim)",
	"Water (No Swim)",
	"In Flight",
	"Underwater",
	"Faerie Regio",
	"Infernal Regio",
	"Divine Regio",
	"Ancient Regio",
	"Shore",
	"Highway",
	"\n"
};


/*
 * SEX_x
 * Not used in sprinttype() so no \n.
 */
const	char *genders[] =
{
	"neutral",
	"male",
	"female",
	"\n"
};


/* POS_x */
const	char *position_types[] = {
	"Dead",
	"Mortally wounded",
	"Incapacitated",
	"Stunned",
	"Sleeping",
	"Meditating",
	"Resting",
	"Sitting",
	"Fighting",
	"Dodging",
	"Defending",
	"Standing",
	"Watching",
	"!TARGET!",
	"\n"
};


/* PLR_x */
const	char *player_bits[] = {
	"KILLER",
	"THIEF",
	"FROZEN",
	"DONTSET",
	"WRITING",
	"MAILING",
	"CSH",
	"SITEOK",
	"NOSHOUT",
	"NOTITLE",
	"DELETED",
	"LOADRM",
	"NO_WIZL",
	"NO_DEL",
	"INVST",
	"CRYO",
	"OLC",
	"NOTDEADYET",
	"WAR",
	"MUTED",
	"NORIGHTS",
	"NEWBIE",
	"F_RENAME",
	"\n"
};


/* MOB_x */
const	char *action_bits[] = {
	"SPEC",
	"SENTINEL",
	"SCAVENGER",
	"ISNPC",
	"AWARE",
	"AGGR",
	"STAY-ZONE",
	"WIMPY",
	"AGGR_EVIL",
	"AGGR_GOOD",
	"AGGR_NEUTRAL",
	"MEMORY",
	"HELPER",
	"NO_CHARM",
	"NO_SUMMN",
	"NO_SLEEP",
	"NO_BASH",
	"NO_BLIND",
	"NO_SHROUD",
	"SEE_WIZINVIS",
	"AGGR_FAE",
	"AGGR_DIVINE",
	"AGGR_INFERNAL",
	"AGGR_HUMAN",
	"TARGET",
	"CHASE",
	"RESOURCE",
	 /* "NOTDEADYET" */ /* Shouldn't be visible in Medit */
	"\n"
};


/* PRF_x */
const	char *preference_bits[] = {
	"BRIEF",
	"COMPACT",
	"DEAF",
	"NO_TELL",
	"D_HP",
	"D_MANA",
	"D_MOVE",
	"AUTOEX",
	"NO_HASS",
	"QUEST",
	"SUMN",
	"NO_REP",
	"LIGHT",
	"C1",
	"C2",
	"NO_WIZ",
	"L1",
	"L2",
	"NO_GTZ",
	"RMFLG",
	"CLS",
	"AUTOASS",
	"AUTOSAC",
	"AUTOSPLIT",
	"AUTOLOOT",
	"AUTOGOLD",
	"METERBAR",
	"CAN_EMAIL",
	"NO_NEWBIE",
	"NO_CLANTALK",
	"TIPCHAN",
	"SHOWVNUMS",
	"NO_SING",
	"NO_OBSCENE",
	"SKILLGAINS",
	"TIMESTAMPS",
	"HAS_PROMPT",
	"REALSKILLS",
	"\n"
};


/* AFF_x */
const	char *affected_bits[] =
{
	"BLIND",
	"INVIS",
	"DET-ALIGN",
	"DET-INVIS",
	"DET-MAGIC",
	"SENSE-LIFE",
	"WATWALK",
	"SANCT",
	"GROUP",
	"CURSE",
	"INFRA",
	"POISON",
	"PROT-EVIL",
	"PROT-GOOD",
	"SLEEP",
	"NO_TRACK",
	"SHROUD",
	"ORB",
	"SNEAK",
	"HIDE",
	"HASTE",
	"CHARM",
	"BARKSKIN",
	"STONESKIN",
	"STEELSKIN",
	"SEE-FAERIE",
	"SEE-INFERNAL",
	"SEE-DIVINE",
	"SEE-ANCIENT",
	"CHASE",
	"\n"
};


/* This is simply a better formatted version of the above, for use in SCORE */
const	char *affected_flags[] =
{
	"&bBlind ",
	"&BInivisibility ",
	"&mDetect-alignment ",
	"&MDetect-invis ",
	"&yDetect-magic ",
	"&YSense-life ",
	"&gWaterwalk ",
	"&GSanctuary ",
	"&cGrouped ",
	"&CCursed ",
	"&rInfravision ",
	"&RPoison ",
	"&wProt-evil ",
	"&WProt-good ",
	"&bSleep ",
	"&BNo-track ",
	"&mShroud ",
	"&MOrb ",
	"&ySneak ",
	"&YHide ",
	"&gHaste ",
	"&GCharm ",
	"&cBarkskin ",
	"&CStoneskin ",
	"&rSteelskin ",
	"&RSee-faerie ",
	"&wSee-infernal ",
	"&WSee-divine ",
	"&bSee-ancient ",
	"&BChase ",
	"\n"
};

/* CON_x */
const	char *connected_types[] = {
	"Playing",
	"Disconnecting",
	"Get Name",
	"Confirm Name",
	"Get Password",
	"Get new PW",
	"Confirm new PW",
	"Select Gender",
	"Reading MOTD",
	"Main Menu",
	"Changing PW 1",
	"Changing PW 2",
	"Changing PW 3",
	"Self-Delete 1",
	"Self-Delete 2",
	"Disconnecting",
	"Object Edit",
	"Room Edit",
	"Zone Edit",
	"Mobile Edit",
	"Shop Edit",
	"Select Culture",
	"Rolling Stats",
	"Conf. Culture",
	"Action Edit",
	"Trigger Edit",
	"Assembly Edit",
	"Emailing",
	"Select Email",
	"Quest Edit",
	"Select Term",
	"Confirm Email",
	"Select Height",
	"Select Weight",
	"Sel. Eyecolor",
	"Sel. Haircol",
	"Sel. Hairstyl",
	"Sel. Skintone",
  "Changing chars",
	"Conf. E-list",
	"Change Email",
	"Spell Editor",
	"Desc. Editor",
	"Copyover Ready",
	"Forced Rename",
	"ContInfo Edit",
	"Guild Editor",
	"Command Edit",
	"Tutor Edit",
	"Edit Background",
	"Char. Verify",
	"Select Skills",
	"House Editor",
	"\n"
};


/*
 * WEAR_x - for eq list
 * Not use in sprinttype() so no \n.
 */
const	char *wear_where[] = {
	"<used as light>          ",
	"<worn on finger>         ",
	"<worn on finger>         ",
	"<worn around neck>       ",
	"<worn on body>           ",
	"<worn on head>           ",
	"<worn on legs>           ",
	"<worn on feet>           ",
	"<worn on hands>          ",
	"<worn on arms>           ",
	"<worn as shield>         ",
	"<worn about body>        ",
	"<worn about waist>       ",
	"<worn around wrist>      ",
	"<worn around wrist>      ",
	"<wielded>                ",
	"<held>                   ",
	"<dual wielded>           ",
	"<worn on face>           ",
  "<floating nearby>        ",
  "<worn on back>           ",
	"<worn on belt>           ",
	"<worn on belt>           ",
	"<worn outside clothing>  ",
	"<worn around throat>			",
	"<worn around throat>     ",
	"<worn on wings>          ",
	"<worn on horns>          ",
	"<worn on tail>           "
};


/*
 * WEAR_x - for eq list, sorted
 * Not used in sprinttype(), so no \n.
 */
const char *wear_sort_where[] = {
  "<floating nearby>        ",
	"<used as light>          ",
	"<worn on horns>          ",
	"<worn on head>           ",
	"<worn on face>           ",
	"<worn around neck>       ",
	"<worn around throat>     ",
	"<worn around throat>     ",
	"<worn on wings>          ",
	"<worn on body>           ",
	"<worn about body>        ",
  "<worn on back>           ",
	"<worn outside clothing>  ",
	"<worn on arms>           ",
	"<worn around wrist>      ",
	"<worn around wrist>      ",
	"<worn on hands>          ",
	"<worn on finger>         ",
	"<worn on finger>         ",
	"<held>                   ",
	"<wielded>                ",
	"<dual wielded>           ",
	"<worn as shield>         ",
	"<worn about waist>       ",
	"<worn on belt>           ",
	"<worn on belt>           ",
	"<worn on tail>           ",
	"<worn on legs>           ",
	"<worn on feet>           "
};


int wear_slots[] = {
	WEAR_FLOAT,
	WEAR_LIGHT,
	WEAR_HORNS,
	WEAR_HEAD,
	WEAR_FACE,
	WEAR_NECK,
	WEAR_THROAT_1,
	WEAR_THROAT_2,
	WEAR_WINGS,
	WEAR_BODY,
	WEAR_ABOUT,
	WEAR_BACK,
	WEAR_OUTSIDE,
	WEAR_ARMS,
	WEAR_WRIST_R,
	WEAR_WRIST_L,
	WEAR_HANDS,
	WEAR_FINGER_R,
	WEAR_FINGER_L,
	WEAR_HOLD,
	WEAR_WIELD,
	WEAR_DWIELD,
	WEAR_SHIELD,
	WEAR_WAIST,
	WEAR_BELT_1,
	WEAR_BELT_2,
	WEAR_TAIL,
	WEAR_LEGS,
	WEAR_FEET
};


/* WEAR_x - for stat */
const	char *equipment_types[] = {
	"Used as light",
	"Worn on right finger",
	"Worn on left finger",
	"First worn around neck",
	"Worn on body",
	"Worn on head",
	"Worn on legs",
	"Worn on feet",
	"Worn on hands",
	"Worn on arms",
	"Worn as shield",
	"Worn about body",
	"Worn around waist",
	"Worn around right wrist",
	"Worn around left wrist",
	"Wielded",
	"Held",
	"Dual wielded",
	"Worn on face",
  "Floating nearby",
  "Worn on back",
	"First worn on belt",
	"Second worn on belt",
	"Worn outside clothing",
	"First worn around throat",
	"Second worn around throat",
	"Worn on wings",
	"Worn on horns",
	"Worn around tail",
	"\n"
};


/* ITEM_x (ordinal object types) */
const	char *item_types[] = {
	"UNDEFINED",
	"LIGHT",
	"SCROLL",
	"WAND",
	"STAFF",
	"WEAPON",
	"FIRE WEAPON",
	"MISSILE",
	"TREASURE",
	"ARMOR",
	"POTION",
	"WORN",
	"OTHER",
	"TRASH",
	"TRAP",
	"CONTAINER",
	"NOTE",
	"LIQ CONTAINER",
	"KEY",
	"FOOD",
	"MONEY",
	"PEN",
	"BOAT",
	"FOUNTAIN",
	"FLIGHT",
	"PORTAL",
	"SPELLBOOK",
	"TOOL",
	"RESOURCE",
	"SHEATH",
	"\n"
};


/* ITEM_WEAR_ (wear bitvector) */
const	char *wear_bits[] = {
	"TAKE",
	"FINGER",
	"NECK",
	"BODY",
	"HEAD",
	"LEGS",
	"FEET",
	"HANDS",
	"ARMS",
	"SHIELD",
	"ABOUT",
	"WAIST",
	"WRIST",
	"WIELD",
	"HOLD",
	"DWIELD",
	"EAR",
	"FACE",
  "FLOAT",
  "BACK",
	"BELT",
	"OUTSIDE",
	"THROAT",
	"WINGS",
	"HORNS",
	"TAIL",
	"\n"
};


/* ITEM_x (extra bits) */
const	char *extra_bits[] = {
	"GLOW",						/* 0 */
	"HUM",						/* 1 */
	"!RENT",
	"!DONATE",
	"!INVIS",
	"INVISIBLE",			/* 5 */
	"MAGIC",
	"!DROP",
	"BLESS",
	"!GOOD",
	"!EVIL",					/* 10 */
	"!NEUTRAL",
	"!MAGI",
	"!CLERIC",
	"!THIEF",
	"!WARRIOR",				/* 15 */
	"!SELL",
	"!DISARM",
	"!FAE",
	"!DEMONIC",
	"!DIVINE",				/* 20 */
	"!VAMPIRE",
	"!LITTLE_FAE",
	"!UNDEAD",
	"OK_ANIMAL",
	"!DIETY",					/* 25 */
	"!HUMAN",
	"!DISPLAY",
	"TWOHANDED",
	"!STEAL",
	"UNIQUE_SAVE",		/* 30 */
	"COLORIZE",
	"\n"
};


/* APPLY_x */
const	char *apply_types[] = {
	"NONE",
	"STRENGTH",
	"AGILITY",
	"PRECISION",
	"PERCEPTION",
	"HEALTH",
	"WILLPOWER",
	"INTELLIGENCE",
	"CHARISMA",
	"LUCK",
	"ESSENCE",
	"CLASS",
	"AGE",
	"CHAR_WEIGHT",
	"CHAR_HEIGHT",
	"MAXMANA",
	"MAXHIT",
	"MAXMOVE",
	"GOLD",
	"EXP",
	"ARMOR",
	"HITROLL",
	"DAMROLL",
	"SAVING_PARA",
	"SAVING_ROD",
	"SAVING_PETRI",
	"SAVING_BREATH",
	"SAVING_SPELL",
	"RACE",
	"\n"
};


/* TRAV_x (Travel defaults)
 * Note: For mout, direction and '.' is appended
 */
const	char *travel_defaults[] = {
	"$n appears with an ear-splitting bang!",
	"$n disappears in a puff of smoke.",
	"$n has arrived.",
	"$n leaves ", /* direction. */
	"$n has entered the game.",
	"$n has left the game.",
	"You feel a strange presence as $n appears, seemingly from nowhere.",
	"You blink and suddenly realize that $n is gone."
};


/* CONT_x */
const	char *container_bits[] = {
	"CLOSEABLE",
	"PICKPROOF",
	"CLOSED",
	"LOCKED",
	"\n",
};


/* LIQ_x */
const	char *drinks[] =
{
	"water",							// 0
	"beer",								// 1
	"wine",								// 2
	"ale",								// 3
	"dark ale",						// 4
	"meat broth",					// 5
	"lemonade",						// 6
	"small beer",					// 7
	"local speciality",		// 8
	"juice",							// 9
	"milk",								// 10
	"tea",								// 11
	"coffee",							// 12
	"blood",							// 13
	"salt water",					// 14
	"clear water",				// 15
	"fine champagne",			// 16
	"mead",								// 17
	"dye color",					// 18
	"\n"
};


/* other constants for liquids ******************************************/


/* one-word alias for each drink */
const	char *drinknames[] =
{
	"water",			// 0.  water
	"beer",				// 1.  beer
	"wine",				// 2.  wine
	"ale",				// 3.  ale
	"ale",				// 4.  dark ale
	"broth",			// 5.  meat broth
	"lemonade",		// 6.  lemonade
	"beer",				// 7.  small beer
	"local",			// 8.  local speciality
	"juice",			// 9.  juice
	"milk",				// 10. milk
	"tea",				// 11. tea
	"coffee",			// 12. coffee
	"blood",			// 13. blood
	"salt",				// 14. salt water
	"water",			// 15. clear water
	"champagne",	// 16. fine champagne
	"mead",				// 17. mead
	"color",			// 18. dye color
	"\n"
};


/* effect of drinks on DRUNK, FULL, and THIRST -- see values.doc */
int	drink_aff[][3] = {
	{	0,	1,	10	},	// 0.  water
	{	5,	2,	3		},	// 1.  beer
	{	4,	2,	2		},	// 2.  wine
	{	5,	2,	3		},	// 3.  ale
	{	6,	2,	4		},	// 4.  dark ale
	{	0,	1,	2		},	// 5.  meat broth
	{	0,	1,	8		},	// 6.  lemonade
	{	7,	0,	0		},	// 7.  small beer
	{	3,	3,	3		},	// 8.  local speciality
	{	0,	2,	7		},	// 9.  juice
	{	0,	3,	6		},	// 10. milk
	{	0,	1,	3		},	// 11. tea
	{	0,	1,	2		},	// 12. coffee
	{	0,	2,	-1	},	// 13. blood
	{	0,	1,	-2	},	// 14. salt water
	{	0,	0,	13	},	// 15. clear water
	{	3,	1,	5		},	// 16. fine champagne
	{	5,	2,	5		},	// 17. mead
	{	0,	0,	0		}		// 18. dye color
};


/* color of the various drinks */
const	char *color_liquid[] =
{
	"clear",					// 0.  water
	"brown",					// 1.  beer
	"clear",					// 2.  wine
	"brown",					// 3.  ale
	"dark",						// 4.  dark ale
	"brown",					// 5.  meat broth
	"red",						// 6.  lemonade
	"golden",					// 7.  small beer
	"clear",					// 8.  local speciality
	"light green",		// 9.  juice
	"white",					// 10. milk
	"brown",					// 11. tea
	"black",					// 12. coffee
	"red",						// 13. blood
	"clear",					// 14. salt water
	"crystal clear",	// 15. clear water
	"golden",					// 16. fine champagne
	"honey brown",		// 17. mead
	"thick",					// 18. color
	"\n"
};


/*
 * level of fullness for drink containers
 * Not used in sprinttype() so no \n.
 */
const	char *fullness[] =
{
	"less than half ",
	"about half ",
	"more than half ",
	""
};


/* str, int, wis, dex, con applies **************************************/


/* [ch] strength apply (all) */
cpp_extern const struct str_app_type str_app[] = {
	{-5, -4, 0, 0},	/* str = 0 */
	{-5, -4, 3, 1},	/* str = 1 */
	{-3, -2, 3, 2},
	{-3, -1, 10, 3},
	{-2, -1, 25, 4},
	{-2, -1, 55, 5},	/* str = 5 */
	{-1, 0, 80, 6},
	{-1, 0, 90, 7},
	{0, 0, 100, 8},
	{0, 0, 100, 9},
	{0, 0, 115, 10},	/* str = 10 */
	{0, 0, 115, 11},
	{0, 0, 140, 12},
	{0, 0, 140, 13},
	{0, 0, 170, 14},
	{0, 0, 170, 15},	/* str = 15 */
	{0, 1, 195, 16},
	{1, 1, 220, 18},
	{1, 2, 255, 20},	/* str = 18 */
	{3, 7, 640, 40},
	{3, 8, 700, 40},	/* str = 20 */
	{4, 9, 810, 40},
	{4, 10, 970, 40},
	{5, 11, 1130, 40},
	{6, 12, 1440, 40},
	{7, 14, 1750, 40}	/* str = 25 */
};



/* [dex] skill apply (thieves only) */
cpp_extern const struct dex_skill_type dex_app_skill[] = {
	{-99, -99, -90, -99, -60},	/* dex = 0 */
	{-90, -90, -60, -90, -50},	/* dex = 1 */
	{-80, -80, -40, -80, -45},
	{-70, -70, -30, -70, -40},
	{-60, -60, -30, -60, -35},
	{-50, -50, -20, -50, -30},	/* dex = 5 */
	{-40, -40, -20, -40, -25},
	{-30, -30, -15, -30, -20},
	{-20, -20, -15, -20, -15},
	{-15, -10, -10, -20, -10},
	{-10, -5, -10, -15, -5},	/* dex = 10 */
	{-5, 0, -5, -10, 0},
	{0, 0, 0, -5, 0},
	{0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0},		/* dex = 15 */
	{0, 5, 0, 0, 0},
	{5, 10, 0, 5, 5},
	{10, 15, 5, 10, 10},		/* dex = 18 */
	{15, 20, 10, 15, 15},
	{15, 20, 10, 15, 15},		/* dex = 20 */
	{20, 25, 10, 15, 20},
	{20, 25, 15, 20, 20},
	{25, 25, 15, 20, 20},
	{25, 30, 15, 25, 25},
	{25, 30, 15, 25, 25}		/* dex = 25 */
};



/* [dex] apply (all) */
cpp_extern const struct dex_app_type dex_app[] = {
	{-7, -7, 6},		/* dex = 0 */
	{-6, -6, 5},		/* dex = 1 */
	{-4, -4, 5},
	{-3, -3, 4},
	{-2, -2, 3},
	{-1, -1, 2},		/* dex = 5 */
	{0, 0, 1},
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},		/* dex = 10 */
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, -1},		/* dex = 15 */
	{1, 1, -2},
	{2, 2, -3},
	{2, 2, -4},		/* dex = 18 */
	{3, 3, -4},
	{3, 3, -4},		/* dex = 20 */
	{4, 4, -5},
	{4, 4, -5},
	{4, 4, -5},
	{5, 5, -6},
	{5, 5, -6}		/* dex = 25 */
};



/* [con] apply (all) */
cpp_extern const struct con_app_type con_app[] = {
	{-4, 20},		/* con = 0 */
	{-3, 25},		/* con = 1 */
	{-2, 30},
	{-2, 35},
	{-1, 40},
	{-1, 45},		/* con = 5 */
	{-1, 50},
	{0, 55},
	{0, 60},
	{0, 65},
	{0, 70},		/* con = 10 */
	{0, 75},
	{0, 80},
	{0, 85},
	{0, 88},
	{1, 90},		/* con = 15 */
	{2, 95},
	{2, 97},
	{3, 99},		/* con = 18 */
	{3, 99},
	{4, 99},		/* con = 20 */
	{5, 99},
	{5, 99},
	{5, 99},
	{6, 99},
	{6, 99}		/* con = 25 */
};



/* [int] apply (all) */
cpp_extern const struct int_app_type int_app[] = {
	{3},		/* int = 0 */
	{5},		/* int = 1 */
	{7},
	{8},
	{9},
	{10},		/* int = 5 */
	{11},
	{12},
	{13},
	{15},
	{17},		/* int = 10 */
	{19},
	{22},
	{25},
	{30},
	{35},		/* int = 15 */
	{40},
	{45},
	{50},		/* int = 18 */
	{53},
	{55},		/* int = 20 */
	{56},
	{57},
	{58},
	{59},
	{60}		/* int = 25 */
};


/* [wis] apply (all) */
cpp_extern const struct wis_app_type wis_app[] = {
	{0},	/* wis = 0 */
	{0},  /* wis = 1 */
	{0},
	{0},
	{0},
	{0},  /* wis = 5 */
	{0},
	{0},
	{0},
	{0},
	{0},  /* wis = 10 */
	{0},
	{2},
	{2},
	{3},
	{3},  /* wis = 15 */
	{3},
	{4},
	{5},	/* wis = 18 */
	{6},
	{6},  /* wis = 20 */
	{6},
	{6},
	{7},
	{7},
	{7}  /* wis = 25 */
};

const	char *npc_class_types[] = {
	"Normal",
	"Undead",
	"\n"
};

const	char *npc_race_types[] = {
	"Humanoid",
	"Monster",
	"\n"
};


#if	defined(OASIS_MPROG)
/*
 * Definitions necessary for MobProg support in OasisOLC
 */
const	char *mobprog_types[] = {
	"INFILE",
	"ACT",
	"SPEECH",
	"RAND",
	"FIGHT",
	"DEATH",
	"HITPRCNT",
	"ENTRY",
	"GREET",
	"ALL_GREET",
	"GIVE",
	"BRIBE",
	"\n"
};
#endif

/* These points were reconfigured for balance with the level
 * stripping.  Point amounts will now be between 0 and 4
 * instead of 1 and 9.  Most points should be no more than
 * 2.  You'll get it by looking at it for what I was thinking.
 * I hope. 3/8/04 - CG
 */
int	movement_loss[] =
{
	0,	/* Inside			*/
	0,	/* City				*/
	1,	/* Field			*/
	1,	/* Forest			*/
	2,	/* Hills			*/
	3,	/* Mountains	*/
	2,	/* Swimming		*/
	0,	/* Unswimmable - err.. you're in a boat?	*/
	0,	/* Flying			*/
	3,	/* Underwater	*/
	0,	/* Faerie			*/
	4,	/* Infernal		*/
	4,	/* Divine			*/
	4,	/* Ancient		*/
	2,	/* Shore			*/
	2		/* Highway		*/
};


#if	defined(CONFIG_OASIS_MPROG)
/*
 * Definitions necessary for MobProg support in OasisOLC
 */
const	char *mobprog_types[] = {
	"INFILE",
	"ACT",
	"SPEECH",
	"RAND",
	"FIGHT",
	"DEATH",
	"HITPRCNT",
	"ENTRY",
	"GREET",
	"ALL_GREET",
	"GIVE",
	"BRIBE",
	"\n"
};
#endif


/* Constants for Assemblies    *****************************************/
const	char *AssemblyTypes[] = {
	"assemble",
	"bake",
	"brew",
	"craft",
	"fletch",
	"knit",
	"make",
	"mix",
	"thatch",
	"weave",
	"cook",
	"sew",
	"butcher",
	"tan",
	"smelt",
	"cut",
	"divide",
	"forge",
	"\n"
};


const	char *AssemblyTypeNames[][3] = {
	{	"assemble",	"assembles",	"assembled"	},
	{	"bake",			"bakes",			"baked"			},
	{	"brew",			"brews",			"brewn"			},
	{	"craft",		"crafts",			"crafted"		},
	{	"fletch",		"fletches",		"fletched"	},
	{	"knit",			"knits",			"knitted"		},
	{	"make",			"makes",			"made"			},
	{	"mix",			"mixes",			"mixed"			},
	{	"thatch",		"thatches",		"thatched"	},
	{	"weave",		"weaves",			"woven"			},
	{	"cook",			"cooks",			"cooked"		},
	{	"sew",			"sews",				"sewn"			},
	{	"butcher",	"butchers",		"butchered"	},
	{	"tan",			"tans",				"tanned"		},
	{	"smelt",		"smelts",			"smelted"		},
	{	"cut",			"cuts",				"cut"				},
	{	"divide",		"divides",		"divided"		},
	{	"forge",		"forges",			"forged"		},
	{	"\n",				"\n",					"\n"				}
};


const	char *AssemblyByproductLoads[] = {
	"failure",
	"success",
	"always",
	"\n"
};


/* Constants for Foraging skills    **************************/
const	char *forage_skill[] = {
	"fish",
	"hunt",
	"gather food",
	"chop lumber",
	"mine ore"
};


const	char *forage_method[] = {
	"caught",
	"killed",
	"found",
	"harvested",
	"dug out"
};


const	struct weapon_prof_data wprof[] = {
/*	 HR   AC  ADDDAM  */
	{  0,   0,  90}, /* Not Used     */
	{  1,   0,  4},  /* prof <= 20   */
	{  2,   0,  4},  /* prof <= 40   */
	{  3,   0,  3},  /* prof <= 60   */
	{  4,   0,  3},  /* prof <= 80   */
	{  6,  -1,  2},  /* prof <= 85   */
	{  7,  -2,  2},  /* prof <= 90   */
	{  8,  -3,  2},  /* prof <= 95   */
	{  9,  -4,  1},  /* prof <= 99   */
	{ 10,  -5,  1},  /* prof == 100  */
	{ -1,   0,  90}   /* prof == 0    */

/* for Add dam, it's player level divided by that number is added */
};


const char *mob_difficulty_type[] = {
	"simple",
	"easy",
	"normal",
	"moderate",
	"hard"
};


const char *event_modes[] = {
	"TASK",
	"EVENT"
};


const char *magic_flux_types[] = {
	"Dead (no magic)",
	"Poor (5)",
	"Typical (10)",
	"High (15)",
	"Rich (20)",
	"\n"
};


const char *hometowns[] = {
	"!NOWHERE!",
	"Caerdydd",
	"Arianoch",
	"Abertawe",
	"Woodward",
	"Cambrian",
	"Immortals",
	"Start Zone",
	"OOC Lounge",
	"\n"
};


const char *lore_methods[] = {
	"botany",
	"geography",
	"arcane",
	"theology",
	"zoology",
	"linguistics",
	"survival",
	"realms",
	"occult",
	"anthropology",
	"streetwise",
	"\n"
};


const char *resource_bits[] = {
	"IRON",			// 1
	"COPPER",
	"SILVER",
	"GOLD",
	"TIN",
	"LEAD",			// 6
	"EMERALD",
	"RUBY",
	"SAPPHIRE",
	"DIAMOND",
	"TOPAZ",
	"AMETHYST",	// 12
	"OAK",
	"PINE",
	"BEECH",
	"ASH",
	"ROWAN",
	"YEW",			// 18
	"GRANITE",
	"MARBLE",
	"SANDSTONE",
	"LIMESTONE",
	"FLINT",
	"SLATE",		// 24
	"FLEECE",
	"FLAX",
	"HEMP",
	"SILK",
	"LEATHER",
	"FUR",			// 30
	"\n"
};


const char *resource_names[] = {
	"",
	"iron",			// 1
	"copper",
	"silver",
	"gold",
	"tin",
	"lead",			// 6
	"emerald",
	"ruby",
	"sapphire",
	"diamond",
	"topaz",
	"amethyst",	// 12
	"oak",
	"pine",
	"beech",
	"ash",
	"rowan",
	"yew",			// 18
	"granite",
	"marble",
	"sandstone",
	"limestone",
	"flint",
	"slate",		// 24
	"fleece",
	"flax",
	"hemp",
	"silk",
	"leather",
	"fur",			// 30
	"\n"
};


const char *resource_types[NUM_RESOURCE_GROUPS][NUM_RESOURCE_TYPES] = {
	{	"iron",			"copper",	"silver",			"gold",				"tin",			"lead",			},
	{	"emerald",	"ruby",		"sapphire",		"diamond",		"topaz",		"amethyst",	},
	{	"oak",			"pine",		"beech",			"ash",				"rowan",		"yew",			},
	{	"granite",	"marble",	"sandstone",	"limestone",	"flint",		"slate",		},
	{	"fleece",		"flax",		"hemp",				"silk",				"leather",	"fur",			}
};


const char *resource_groups[] = {
	"ore",
	"gems",
	"wood",
	"stone",
	"fabric",
	"\n"
};


/* Player attributes */
const char *attributes[] = {
	"strength",
	"agility",
	"precision",
	"perception",
	"health",
	"willpower",
	"intelligence",
	"charisma",
	"luck",
	"essence",
	"\n"
};


/* Abbreviated Player attributes */
const char *abbr_attributes[] = {
	"str",
	"agi",
	"prc",
	"per",
	"hea",
	"wil",
	"int",
	"cha",
	"lck",
	"ess",
	"\n"
};


/* Arcane word / true pattern syllables */
const char *arc_sylls[5][2][11] = {
	{	/* Ten Thousands */
		{"ba", "bo", "di", "fo", "gu", "hy", "ja", "ke", "li", "mu", "\n"},
		{"ny", "cu", "qe", "ri", "so", "tu", "vy", "wa", "xe", "zi", "\n"},
	},
	{	/* Thousands */
		{"b", "c", "d", "f", "g", "h", "j", "k", "l", "m", "\n"},
		{"n", "p", "q", "r", "s", "t", "v", "w", "x", "z", "\n"},
	},
	{	/* Hundreds */
		{"ab", "ae", "ai", "ao", "au", "ay", "ea", "ec", "ei", "eo", "\n"},
		{"oi", "of", "ou", "oy", "ac", "ec", "ed", "ug", "uz", "uq", "\n"},
	},
	{	/* Tens */
		{"z", "c", "x", "f", "w", "h", "v", "k", "s", "m", "\n"},
		{"n", "b", "q", "d", "k", "t", "j", "g", "r", "p", "\n"},
	},
	{	/* Ones */
		{"aq", "as", "av", "au", "an", "ef", "eg", "et", "en", "ep", "\n"},
		{"re", "ru", "us", "ut", "at", "ur", "yr", "yr", "er", "ea", "\n"},
	}
};


/* TAR_x Target Types */
const char *targets[] = {
	"IGNORE",
	"CHAR_ROOM",
	"CHAR_WORLD",
	"FIGHT_SELF",
	"FIGHT_VICT",
	"SELF_ONLY",
	"NOT_SELF",
	"OBJ_INV",
	"OBJ_ROOM",
	"OBJ_WORLD",
	"OBJ_EQUIP",
	"\n"
};


/* MAG_x Magic Routines */
const char *magic_routines[] = {
	"DAMAGE",
	"AFFECTS",
	"UNAFFECTS",
	"POINTS",
	"ALTER_OBJS",
	"GROUPS",
	"MASSES",
	"AREAS",
	"SUMMONS",
	"CREATIONS",
	"MANUAL",
	"\n"
};


const struct log_rights log_fields[] = {
	{	"bugs",					RIGHTS_DEVELOPER		},	/* 0 */
	{	"contact",			RIGHTS_ADMIN				},
	{	"deaths",				RIGHTS_IMMORTAL			},
	{	"flags",				RIGHTS_ADMIN				},
	{	"general",			RIGHTS_IMMORTAL			},
	{	"olc",					RIGHTS_BUILDING			},	/* 5 */
	{	"links",				RIGHTS_IMMORTAL			},
	{	"load",					RIGHTS_IMMORTAL			},
	{	"logins",				RIGHTS_IMMORTAL			},
	{	"memcheck",			RIGHTS_DEVELOPER		},
	{	"mobdeaths",		RIGHTS_IMMORTAL			},	/* 10 */
	{	"newbies",			RIGHTS_IMMORTAL			},
	{	"warkills",			RIGHTS_IMMORTAL			},
	{	"nohelp",				RIGHTS_HELPFILES		},
	{	"penalties",		RIGHTS_PLAYERS			},
	{	"playerlog",		RIGHTS_PLAYERS			},	/* 15 */
	{	"questing",			RIGHTS_QUESTOR			},
	{	"rent",					RIGHTS_IMMORTAL			},
	{	"resets",				RIGHTS_IMMORTAL			},
	{	"rpexp",				RIGHTS_PLAYERS			},
	{	"rpmonitor",		RIGHTS_PLAYERS  		},	/* 20 */
	{	"secure",				RIGHTS_DEVELOPER		},
	{	"players",			RIGHTS_ADMIN				},
	{	"sites",				RIGHTS_ADMIN				},
	{	"snoops",				RIGHTS_ADMIN				},
	{	"spam",					RIGHTS_PLAYERS			},	/* 25 */
	{	"sql",					RIGHTS_DEVELOPER		},
	{	"switches",			RIGHTS_ADMIN				},
	{	"thefts",				RIGHTS_IMMORTAL			},
	{	"ticks",				RIGHTS_DEVELOPER		},
	{	"triggers",			RIGHTS_TRIGGERS			},	/* 30 */
	{	"buffer",				RIGHTS_DEVELOPER		},
	{	"guilds",				RIGHTS_ADMIN				},
	{	"houses",				RIGHTS_PLAYERS			},
	{	"\n",						0,									}
};


/* EDIT_x */
const char *edit_modes[] = {
	"Writing extra description", /* not used anyhow but hey */
	"Mailing", /* Why not append name of player we're mailing at display time? */
	"Writing on board",
	"Writing note",
	"Writing background",
	"Writing description",
	"Writing contactinfo",
	"Editing guilds",
	"Editing textfile"
};


const char *spell_techniques[][3] = {
	{	"creo",					"cr",		"create"		},
	{	"intellego",		"in",		"perceive"	},
	{	"muto",					"mu",		"transform"	},
	{	"perdo",				"pe",		"destroy"		},
	{	"rego",					"re",		"control"		},
	{	"\n",						"\n",		"\n"				}
};


const char *spell_forms[][3] = {
	{	"animal",				"an",		"animal"	},
	{	"aquam",				"aq",		"water"		},
	{	"auram",				"au",		"air"			},
	{	"corporem",			"co",		"body"		},
	{	"herbam",				"he",		"plant"		},
	{	"ignem",				"ig",		"fire"		},
	{	"imagonem",			"im",		"image"		},
	{	"mentem",				"me",		"mind"		},
	{	"terram",				"te",		"earth"		},
	{	"vim",					"vi",		"power"		},
	{	"\n",						"\n",		"\n"			}
};


const char *spell_ranges[][3] = {
	{	"corpus",				"cor",	"body"	},
	{	"sui",					"sui",	"self"	},
	{	"contactus",		"con",	"touch"	},
	{	"pervenio",			"per",	"reach"	},
	{	"oculus",				"ocu",	"eye"		},
	{	"propinquus",		"pro",	"near"	},
	{	"visum",				"vis",	"sight"	},
	{	"\n",						"\n",		"\n"		}
};


const char *spell_duration[][3] = {
	{	"statim",				"st",		"instant"		},
	{	"respicio",			"re",		"attention"	},
	{	"orbis",				"or",		"ring"			},
	{	"phoebus",			"ph",		"sun"				},
	{	"luna",					"lu",		"moon"			},
	{	"annus",				"an",		"year"			},
	{	"permaneo",			"pe",		"permanent"	},
	{	"\n",						"\n",		"\n"				}
};


const char *spell_target_groups[][3] = {
	{	"ego",					"eg",		"self"			},
	{	"aliquis",			"al",		"someone"		},
	{	"gelamen",			"ge",		"gathering"	},
	{	"locus",				"lo",		"location"	},
	{	"universitas",	"un",		"world"			},
	{	"\n",						"\n",		"\n"				}
};


const char *spell_seed_types[][3] = {
	{	"monstro",			"mon",	"points"			},
	{	"gens",					"gen",	"race"				},
	{	"delego",				"del",	"attributes"	},
	{	"sexus",				"sex",	"gender"			},
	{	"sanctimonia",	"san",	"alignment"		},
	{	"nominatim",		"nom",	"truename"		},
	{	"creatura",			"cre",	"creature"		},
	{	"res",					"res",	"object"			},
	{	"locus",				"loc",	"location"		},
	{	"\n",						"\n",		"\n"					}
};


/* Tutor proficiency: human-readable */
const char *tutor_proficiency[] = {
	"mediocre",			// 0-25%
	"average",			// 26-50%
	"expert",				// 51-75%
	"master",				// 75-99%
	"grand-master",	// 100%
	"\n"
};


const char *social_ranks[] = {
	"slave",
	"pauper",
	"serf",
	"yeoman",
	"tradesman",
	"guildsman",
	"magnate",
	"plutocrat",
	"\n"
};


const char *piety_ranks[] = {
	"unbeliever",
	"lay",
	"believer",
	"true believer",
	"disciple",
	"devotee",
	"revered",
	"saint",
	"martyr",
	"\n"
};


const char *reputation_ranks[][2] = {
	/* "good"					"evil"				*/
	{	"unknown",			"unknown"			},
	{	"indifferent",	"indifferent"	},
	{	"hero",					"knave"				},
	{	"famous",				"infamous"		},
	{	"celebrity",		"villain"			},
	{	"renown",				"archvillain"	},
	{	"\n",						"\n"					}
};


const char *realms[] = {
	"!UNDEFINED!",
	"mundane",
	"faerie",
	"heaven",
	"hell",
	"ancient",
	"\n"
};


const char *planes[] = {
	"!UNDEFINED!",
	"mortal",
	"ethereal",
	"astral",
	"\n"
};


const char *race_size[] = {
	"none",
	"tiny",
	"small",
	"medium",
	"large",
	"huge",
	"\n"
};


const char *body_parts[] = {
	"light",
	"left finger",
	"right finger",
	"neck",
	"body",
	"head",
	"legs",
	"feet",
	"hands",
	"arms",
	"shield",
	"about wear location",
	"waist",
	"left wrist",
	"right wrist",
	"wield ability",
	"hold ability",
	"dual wield ability",
	"face",
	"float ability",
	"back",
	"left belt pouch",
	"right belt pouch",
	"outside wear location",
	"upper throat wear location",
	"lower throat wear location",
	"wings",
	"horns",
	"tail",
	"\n"
};


/* mob trigger types */
const char *trig_types[] = {
	"Global", 
	"Random",
	"Command",
	"Speech",
	"Act",
	"Death",
	"Greet",
	"Greet-All",
	"Entry",
	"Receive",
	"Fight",
	"HitPrcnt",
	"Bribe",
	"Load",
	"Memory",
	"Cast",
	"Leave",
	"Leave-All",
	"Door",
	"\n"
};


/* obj trigger types */
const char *otrig_types[] = {
	"Global",
	"Random",
	"Command",
	"Cast",
	"UNUSED",
	"Timer",
	"Get",
	"Drop",
	"Give",
	"Wear",
	"UNUSED",
	"Remove",
	"UNUSED",
	"Load",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"\n"
};


/* wld trigger types */
const char *wtrig_types[] = {
	"Global",
	"Random",
	"Command",
	"Speech",
	"Cast",
	"Zone Reset",
	"Enter",
	"Drop",
	"Leave",
	"Door",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"\n"
};


/* House types */
const char *house_types[] = {
	"PRIVATE",
	"PUBLIC",
	"\n"
};


/* fatigue levels */
const char *fatigue_levels[] = {
	"energized",
	"rested",
	"lightly fatigued",
	"moderately fatigued",
	"extremely fatigued",
	"exhausted",
	"\n"
};

/* --- End of constants arrays. --- */

/*
 * Various arrays we count so we can check the world files.  These
 * must be at the bottom of the file so they're pre-declared.
 */
size_t	room_bits_count = sizeof(room_bits) / sizeof(room_bits[0]) - 1,
	action_bits_count = sizeof(action_bits) / sizeof(action_bits[0]) - 1,
	affected_bits_count = sizeof(affected_bits) / sizeof(affected_bits[0]) - 1,
	extra_bits_count = sizeof(extra_bits) / sizeof(extra_bits[0]) - 1,
	wear_bits_count = sizeof(wear_bits) / sizeof(wear_bits[0]) - 1;

