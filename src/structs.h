/* ************************************************************************
*	File: structs.h                                      Part of CircleMUD  *
*	Usage: header file for central structures and contstants                *
*                                                                         *
*	All rights reserved.  See license.doc for complete information.         *
*                                                                         *
*	Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University  *
*	CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
************************************************************************ */
/* $Id: structs.h,v 1.136 2004/04/23 15:18:10 cheron Exp $ */

#include "buffer_opt.h"		/* Catch-22 otherwise. */

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD	0x030015 /* Major/Minor/Patchlevel - MMmmPP */

/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD 3.0 to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ	1  /* TRUE/FALSE aren't defined yet. */

/* preamble *************************************************************/

/*
 * As of bpl20, it should be safe to use unsigned data types for the
 * various virtual and real number data types.  There really isn't a
 * reason to use signed anymore so use the unsigned types and get
 * 65,535 objects instead of 32,768.
 *
 * NOTE: This will likely be unconditionally unsigned later.
 */
#define CIRCLE_UNSIGNED_INDEX	0	/* 0 = signed, 1 = unsigned */

#if CIRCLE_UNSIGNED_INDEX
# define IDXTYPE	ush_int
# define NOWHERE	((IDXTYPE)~0)
# define NOTHING	((IDXTYPE)~0)
# define NOBODY		((IDXTYPE)~0)
#else
# define IDXTYPE	sh_int
# define NOWHERE	(-1)	/* nil reference for rooms	*/
# define NOTHING	(-1)	/* nil reference for objects	*/
# define NOBODY		(-1)	/* nil reference for mobiles	*/
#endif

#define SPECIAL(name) \
	int (name)(struct char_data *ch, void *me, int cmd, char *argument)


/* user rights related defines ******************************************/

/* User rights flags: used by char_data.char_specials.user_rights */
#define RIGHTS_NONE						(1ULL << 0ULL)	// a
#define RIGHTS_MEMBER					(1ULL << 1ULL)	// b
#define RIGHTS_QUESTS					(1ULL << 2ULL)	// c
#define RIGHTS_GUILDS					(1ULL << 3ULL)	// d
#define RIGHTS_NEWBIES				(1ULL << 4ULL)  // e
#define RIGHTS_ACTIONS				(1ULL << 5ULL)	// f
#define RIGHTS_HELPFILES			(1ULL << 6ULL)	// g
#define RIGHTS_IMMORTAL				(1ULL << 7ULL)	// h
#define RIGHTS_QUESTOR				(1ULL << 8ULL)	// i
#define RIGHTS_PLAYERS				(1ULL << 9ULL)	// j
#define RIGHTS_BUILDING				(1ULL << 10ULL)	// k
#define RIGHTS_TRIGGERS				(1ULL << 11ULL)	// l
#define RIGHTS_ADMIN					(1ULL << 12ULL)	// m
#define RIGHTS_DEVELOPER			(1ULL << 13ULL)	// n
#define RIGHTS_HEADBUILDER		(1ULL << 14ULL)	// o
#define RIGHTS_IMPLEMENTOR		(1ULL << 15ULL)	// p
#define RIGHTS_OWNER					(1ULL << 16ULL) // q

#define NUM_USER_RIGHTS				17

/* Syslog type flags: used by char_data.char_specials.syslog */
#define SYSL_BUGS							(1ULL << 0ULL)	// a
#define SYSL_CONTACT					(1ULL << 1ULL)	// b
#define SYSL_DEATHS						(1ULL << 2ULL)	// c
#define SYSL_FLAGS						(1ULL << 3ULL)	// d
#define SYSL_GENERAL					(1ULL << 4ULL)	// e
#define SYSL_OLC							(1ULL << 5ULL)	// f
#define SYSL_LINKS						(1ULL << 6ULL)	// g
#define SYSL_LOAD							(1ULL << 7ULL)	// h
#define SYSL_LOGINS						(1ULL << 8ULL)	// i
#define SYSL_MEMCHECK					(1ULL << 9ULL)	// j
#define SYSL_MOBDEATHS				(1ULL << 10ULL)	// k
#define SYSL_NEWBIES					(1ULL << 11ULL)	// l
#define SYSL_WARKILLS					(1ULL << 12ULL)	// m
#define SYSL_NOHELP						(1ULL << 13ULL)	// n
#define SYSL_PENALTIES				(1ULL << 14ULL)	// o
#define SYSL_PLAYERLOG				(1ULL << 15ULL)	// p
#define SYSL_QUESTING					(1ULL << 16ULL)	// q
#define SYSL_RENT							(1ULL << 17ULL)	// r
#define SYSL_RESETS						(1ULL << 18ULL)	// s
#define SYSL_RPEXP						(1ULL << 19ULL)	// t
#define SYSL_RPMONITOR				(1ULL << 20ULL)	// u
#define SYSL_SECURE						(1ULL << 21ULL)	// v
#define SYSL_PLAYERS					(1ULL << 22ULL)	// w
#define SYSL_SITES						(1ULL << 23ULL)	// x
#define SYSL_SNOOPS						(1ULL << 24ULL)	// y
#define SYSL_SPAM							(1ULL << 25ULL)	// z
#define SYSL_SQL							(1ULL << 26ULL)	// A
#define SYSL_SWITCHES					(1ULL << 27ULL)	// B
#define SYSL_THEFTS						(1ULL << 28ULL)	// C
#define SYSL_TICKS						(1ULL << 29ULL)	// D
#define SYSL_TRIGGERS					(1ULL << 30ULL)	// E
#define SYSL_BUFFER						(1ULL << 31ULL)	// F
#define SYSL_GUILDS						(1ULL << 32ULL)	// G
#define SYSL_HOUSES						(1ULL << 33ULL)	// H

#define NUM_SYSL_FLAGS				33

/* Session type flags: used by char_data.char_specials.session_flags */
#define SESS_IC								(1ULL << 0ULL)
#define SESS_AFK							(1ULL << 1ULL)
#define SESS_AFW							(1ULL << 2ULL)
#define SESS_REPLYLOCK				(1ULL << 3ULL)
#define SESS_HAVETELLS				(1ULL << 4ULL)

#define NUM_SESS_FLAGS				5

/* room-related defines *************************************************/

/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH									0
#define EAST									1
#define SOUTH									2
#define WEST									3
#define UP										4
#define DOWN									5
#define NORTHEAST							6
#define SOUTHWEST							7
#define NORTHWEST							8
#define SOUTHEAST							9
#define INDIR									10
#define OUTDIR								11

#define NUM_OF_DIRS						12  /* number of directions in a room (nsewud) */

/* Zone info: Used in zone_data.zone_flags */
#define ZONE_OPEN							(1ULL << 0ULL)
#define ZONE_CLOSED						(1ULL << 1ULL)
#define ZONE_NORECALL					(1ULL << 2ULL)
#define ZONE_NOSUMMON					(1ULL << 3ULL)
#define ZONE_REMORT_ONLY			(1ULL << 4ULL)
#define ZONE_NOTELEPORT				(1ULL << 5ULL)
#define ZONE_HOMETOWN					(1ULL << 6ULL)
#define ZONE_OOC							(1ULL << 7ULL)

#define NUM_ZONE_FLAGS				8

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK							(1ULL << 0ULL)	/* Dark                      */
#define ROOM_DEATH						(1ULL << 1ULL)	/* Death trap                */
#define ROOM_NOMOB						(1ULL << 2ULL)	/* MOBs not allowed          */
#define ROOM_INDOORS					(1ULL << 3ULL)	/* Indoors                   */
#define ROOM_PEACEFUL					(1ULL << 4ULL)	/* Violence not allowed      */
#define ROOM_SOUNDPROOF				(1ULL << 5ULL)	/* Shouts, gossip blocked    */
#define ROOM_NOTRACK					(1ULL << 6ULL)	/* Track won't go through    */
#define ROOM_NOMAGIC					(1ULL << 7ULL)	/* Magic not allowed         */
#define ROOM_TUNNEL						(1ULL << 8ULL)	/* room for only 1 pers      */
#define ROOM_PRIVATE					(1ULL << 9ULL)	/* Can't teleport in         */
#define ROOM_GODROOM					(1ULL << 10ULL)	/* LVL_GOD+ only allowed     */
#define ROOM_HOUSE						(1ULL << 11ULL)	/* (R) Room is a house       */
#define ROOM_HOUSE_CRASH			(1ULL << 12ULL)	/* (R) House needs saving    */
#define ROOM_ATRIUM						(1ULL << 13ULL)	/* (R) The door to a house   */
#define ROOM_OLC							(1ULL << 14ULL)	/* (R) Modifyable/!compress  */
#define ROOM_BFS_MARK					(1ULL << 15ULL)	/* (R) breath-first srch mrk */
#define ROOM_FAERIE						(1ULL << 16ULL)	/* Faerie room               */
#define ROOM_INFERNAL					(1ULL << 17ULL)	/* Infernal room             */
#define ROOM_DIVINE						(1ULL << 18ULL)	/* Divine room               */
#define ROOM_ANCIENT					(1ULL << 19ULL)	/* Ancient room              */
#define ROOM_PARSED						(1ULL << 20ULL)	/* Room is parsed            */
#define ROOM_ALWAYS_LIT				(1ULL << 21ULL)	/* Room is parsed            */
#define ROOM_NO_MAP				(1ULL << 22ULL) /* No map. */
#define NUM_ROOM_FLAGS				23

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR							(1ULL << 0ULL)	/* Exit is a door            */
#define EX_CLOSED							(1ULL << 1ULL)	/* The door is closed        */
#define EX_LOCKED							(1ULL << 2ULL)	/* The door is locked        */
#define EX_PICKPROOF					(1ULL << 3ULL)	/* Lock can't be picked      */
#define EX_FAERIE							(1ULL << 4ULL)	/* Exit leads to Faerie      */
#define EX_INFERNAL						(1ULL << 5ULL)	/* Exit leads to Infernal    */
#define EX_DIVINE							(1ULL << 6ULL)	/* Exit leads to Divine      */
#define EX_ANCIENT						(1ULL << 7ULL)	/* Exit leads to Ancient     */

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE						0		/* Indoors              */
#define SECT_CITY							1		/* In a city            */
#define SECT_FIELD						2		/* In a field           */
#define SECT_FOREST						3		/* In a forest          */
#define SECT_HILLS						4		/* In the hills         */
#define SECT_MOUNTAIN					5		/* On a mountain        */
#define SECT_WATER_SWIM				6		/* Swimmable water      */
#define SECT_WATER_NOSWIM			7		/* Water - need a boat  */
#define SECT_FLYING						8		/* Wheee!               */
#define SECT_UNDERWATER				9		/* Underwater           */
#define SECT_FAERIE						10	/* Faerie Regio         */
#define SECT_INFERNAL					11	/* Infernal Regio       */
#define SECT_DIVINE						12	/* Divine Regio         */
#define SECT_ANCIENT					13	/* Ancient Regio        */
#define SECT_SHORE						14	/* A shore, for fishing */
#define SECT_HIGHWAY					15  /* On a highway					*/

#define NUM_ROOM_SECTORS			16

/* Magic Flux types: used in room_data.magic_flux */
#define FLUX_DEAD							0
#define FLUX_POOR							1
#define FLUX_TYPICAL					2
#define FLUX_HIGH							3
#define FLUX_RICH							4

#define NUM_FLUX_TYPES				5

/* char and mob-related defines *****************************************/


/* PC classes */
#define CLASS_UNALLOWED				(-2)
#define CLASS_UNDEFINED				(-1)
#define CLASS_MAGUS_ANIMAL		0
#define CLASS_MAGUS_AQUAM			1
#define CLASS_MAGUS_AURAM			2
#define CLASS_MAGUS_CORPOREM	3
#define CLASS_MAGUS_HERBAM		4
#define CLASS_MAGUS_IGNEM			5
#define CLASS_MAGUS_IMAGONEM	6
#define CLASS_MAGUS_MENTEM		7
#define CLASS_MAGUS_TERRAM		8
#define CLASS_MAGUS_VIM				9
#define CLASS_MONK						10
#define CLASS_BARD						11
#define CLASS_GLADIATOR				12
#define CLASS_WARRIOR					13
#define CLASS_PALADIN					14
#define CLASS_HUNTER					15
#define CLASS_THIEF						16
#define CLASS_ASSASSIN				17

#define NUM_CLASSES						18

/* PC class types */
#define CT_MAGI								0
#define CT_CLERIC							1
#define CT_WARRIOR						2
#define CT_THIEF							3

/* Practice types */
#define PT_SPELL							0
#define PT_SKILL							1
#define PT_BOTH								2

/* Realm Groups */
#define REALM_UNDEFINED				(-1)
#define REALM_MUNDANE					0
#define REALM_FAERIE					1
#define REALM_HEAVEN					2
#define REALM_HELL						3
#define REALM_ANCIENT					4

/* Planes */
#define PLANE_UNDEFINED				(-1)
#define PLANE_MORTAL					0
#define PLANE_ETHEREAL				1
#define PLANE_ASTRAL					2

/* Races */
#define RACE_UNDEFINED				(-1)
#define RACE_HUMAN						0
#define RACE_GRAYELF					1
#define RACE_HIGHELF					2
#define RACE_SATYR						3
#define RACE_PIXIE						4
#define RACE_SPRITE						5
#define RACE_NYMPH						6
#define RACE_GNOME						7
#define RACE_AASIMARI					8
#define RACE_GIANT						9
#define RACE_VAMPIRE					10
#define RACE_TIEFLING					11
#define RACE_NIXIE						12
#define RACE_LEPRECHAUN				13
#define RACE_TROLL						14
#define RACE_GOBLIN						15
#define RACE_BROWNIE					16
#define RACE_DRAGON						17
#define RACE_UNDEAD						18
#define RACE_ETHEREAL					19
#define RACE_ANIMAL						20
#define RACE_LEGENDARY				21
#define RACE_ELEMENTAL				22
#define RACE_MAGICAL					23
#define RACE_SERAPHI					24
#define RACE_ADEPHI						25
#define RACE_DEMON						26
#define RACE_DEITY						27
#define RACE_ENDLESS					28

#define NUM_RACES							29

/* Cultures */
#define CULTURE_UNDEFINED			(-1)

/*
 * In order to add a race to the race list you must add a constant
 * that relates to the value in the column Const in races_list.
 * Torgny Bjers, 2002-06-12.
 */

/* Sex */
#define SEX_NEUTRAL						0
#define SEX_MALE							1
#define SEX_FEMALE						2

#define NUM_GENDERS						3

/* Attributes */
#define ATR_STRENGTH					0
#define ATR_AGILITY						1
#define ATR_PRECISION					2
#define ATR_PERCEPTION				3
#define ATR_HEALTH						4
#define ATR_WILLPOWER					5
#define ATR_INTELLIGENCE			6
#define ATR_CHARISMA					7
#define ATR_LUCK							8
#define ATR_ESSENCE						9

#define NUM_ATTRIBUTES				10

/* Traits */
#define TRAIT_PIETY						0
#define TRAIT_REPUTATION			1
#define TRAIT_SOCIAL_RANK			2
#define TRAIT_MILITARY_RANK		3
#define TRAIT_SANITY					4

#define NUM_TRAITS						5

/* Social Ranks */
#define SRANK_SLAVE						0
#define SRANK_PAUPER					1
#define SRANK_SERF						2
#define SRANK_YEOMAN					3
#define SRANK_TRADESMAN				4
#define SRANK_GUILDSMAN				5
#define SRANK_MAGNATE					6
#define SRANK_PLUTOCRAT				7

#define NUM_SOCIAL_RANKS			8

/* Piety Ranks */
#define PIETY_UNBELIEVER			0
#define PIETY_LAY							1
#define PIETY_BELIEVER				2
#define PIETY_TRUE_BELIEVER		3
#define PIETY_DISCIPLE				4
#define PIETY_DEVOTEE					5
#define PIETY_REVERED					6
#define PIETY_SAINT						7
#define PIETY_MARTYR					8

#define NUM_PIETY_RANKS				9

/* Reputation Ranks */
#define REP_UNKNOWN						0
#define REP_INDIFFERENT				1
#define REP_HERO							2
#define REP_FAMOUS						3
#define REP_CELEBRITY					4
#define REP_RENOWN						5

#define NUM_REPUTATION_RANKS	6

/* Sanity Ranks */
#define SAN_SANE							0
#define SAN_UNBALANCED				1
#define SAN_DERANGED					2
#define SAN_INSANE						3

#define NUM_SANITY_RANKS			4

/* Mob difficulties */
#define MDIF_SIMPLE						0
#define MDIF_EASY							1
#define MDIF_NORMAL						2
#define MDIF_MODERATE					3
#define MDIF_HARD							4

#define NUM_MOB_DIFFICULTIES  5

/* Positions */
#define POS_DEAD							0		/* dead             */
#define POS_MORTALLYW					1		/* mortally wounded */
#define POS_INCAP							2		/* incapacitated    */
#define POS_STUNNED						3		/* stunned          */
#define POS_SLEEPING					4		/* sleeping         */
#define POS_MEDITATING				5		/* meditating				*/
#define POS_RESTING						6		/* resting          */
#define POS_SITTING						7		/* sitting          */
#define POS_FIGHTING					8		/* fighting         */
#define POS_DODGE							9   /* dodging attacks  */
#define POS_DEFEND						10	/* defensive mode   */
#define POS_STANDING					11	/* standing         */
#define POS_WATCHING					12	/* standing alert		*/
#define POS_REPLACE						13	/* needs to be replaced, for MOB_TARGET */

/* Fatigue Levels */
#define FAT_ENERGIZED					0
#define FAT_RESTED						1
#define FAT_LIGHT							2
#define FAT_MODERATE					3
#define FAT_EXTREME						4
#define FAT_EXHAUSTION				5

#define NUM_FATIGUE_LEVELS		6

/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER				(1ULL << 0ULL)	/* Player is a player-killer           */
#define PLR_THIEF					(1ULL << 1ULL)	/* Player is a player-thief            */
#define PLR_FROZEN				(1ULL << 2ULL)	/* Player is frozen                    */
#define PLR_DONTSET				(1ULL << 3ULL)	/* Don't EVER set (ISNPC bit)          */
#define PLR_WRITING				(1ULL << 4ULL)	/* Player writing (board/mail)         */
#define PLR_MAILING				(1ULL << 5ULL)	/* Player is writing mail              */
#define PLR_CRASH					(1ULL << 6ULL)	/* Player needs to be crash-saved      */
#define PLR_SITEOK				(1ULL << 7ULL)	/* Player has been site-cleared        */
#define PLR_NOSHOUT				(1ULL << 8ULL)	/* Player not allowed to shout/goss    */
#define PLR_NOTITLE				(1ULL << 9ULL)	/* Player not allowed to set title     */
#define PLR_DELETED				(1ULL << 10ULL)	/* Player deleted - space reusable     */
#define PLR_LOADROOM			(1ULL << 11ULL)	/* Player uses nonstandard loadroom    */
#define PLR_NOWIZLIST			(1ULL << 12ULL)	/* Player shouldn't be on wizlist      */
#define PLR_NODELETE			(1ULL << 13ULL)	/* Player shouldn't be deleted         */
#define PLR_INVSTART			(1ULL << 14ULL)	/* Player should enter game wizinvis   */
#define PLR_CRYO					(1ULL << 15ULL)	/* Player is cryo-saved (purge prog)   */
#define PLR_OLC						(1ULL << 16ULL)	/* Player writing olc                  */
#define PLR_NOTDEADYET		(1ULL << 17ULL)	/* (R) Player being extracted.         */
#define PLR_WAR						(1ULL << 18ULL)	/* Player is at war                    */
#define PLR_MUTED					(1ULL << 19ULL) /* Player has been muted               */
#define PLR_NORIGHTS			(1ULL << 20ULL) /* Player has been stripped of rights  */
#define PLR_NEWBIE				(1ULL << 21ULL) /* Player is a newbie                  */
#define PLR_FORCE_RENAME	(1ULL << 22ULL) /* Force to set a new name on login    */


/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC						(1ULL << 0ULL)	/* Mob has a callable spec-proc      */
#define MOB_SENTINEL				(1ULL << 1ULL)	/* Mob should not move               */
#define MOB_SCAVENGER				(1ULL << 2ULL)	/* Mob picks up stuff on the ground  */
#define MOB_ISNPC						(1ULL << 3ULL)	/* (R) Automatically set on all Mobs */
#define MOB_AWARE						(1ULL << 4ULL)	/* Mob can't be backstabbed          */
#define MOB_AGGRESSIVE			(1ULL << 5ULL)	/* Mob hits players in the room      */
#define MOB_STAY_ZONE				(1ULL << 6ULL)	/* Mob shouldn't wander out of zone  */
#define MOB_WIMPY						(1ULL << 7ULL)	/* Mob flees if severely injured     */
#define MOB_AGGR_EVIL				(1ULL << 8ULL)	/* auto attack evil PC's             */
#define MOB_AGGR_GOOD				(1ULL << 9ULL)	/* auto attack good PC's             */
#define MOB_AGGR_NEUTRAL		(1ULL << 10ULL)	/* auto attack neutral PC's          */
#define MOB_MEMORY					(1ULL << 11ULL)	/* remember attackers if attacked    */
#define MOB_HELPER					(1ULL << 12ULL)	/* attack PCs fighting other NPCs    */
#define MOB_NOCHARM					(1ULL << 13ULL)	/* Mob can't be charmed              */
#define MOB_NOSUMMON				(1ULL << 14ULL)	/* Mob can't be summoned             */
#define MOB_NOSLEEP					(1ULL << 15ULL)	/* Mob can't be slept                */
#define MOB_NOBASH					(1ULL << 16ULL)	/* Mob can't be bashed (e.g. trees)  */
#define MOB_NOBLIND					(1ULL << 17ULL)	/* Mob can't be blinded              */
#define MOB_NOSHROUD				(1ULL << 18ULL)	/* Mob can't be blinded              */
#define MOB_TUTOR						(1ULL << 19ULL)	/* Mob is a tutor                    */
#define MOB_AGGR_FAE				(1ULL << 20ULL)	/* Mob is agressive towards faerie   */
#define MOB_AGGR_DIVINE			(1ULL << 21ULL)	/* Mob is agressive towards angels   */
#define MOB_AGGR_INFERNAL		(1ULL << 22ULL)	/* Mob is agressive towards demons   */
#define MOB_AGGR_HUMAN			(1ULL << 23ULL)	/* Mob is agressive towards humans   */
#define MOB_TARGET					(1ULL << 24ULL) /* Mob is a non-fighting target      */
#define MOB_CHASE						(1ULL << 25ULL) /* Mob will chase a wimpy player     */
#define	MOB_RESOURCE				(1ULL << 26ULL)	/* Mob leaves resources              */
#define	MOB_NOTDEADYET			(1ULL << 27ULL)	/* (R) Mob being extracted.          */

#define	NUM_MOB_FLAGS				27							/* Should be equal to #flags - 1     */


/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF						(1ULL << 0ULL)  /* Room descs won't normally be shown   */
#define PRF_COMPACT					(1ULL << 1ULL)  /* No extra CRLF pair before prompts    */
#define PRF_DEAF						(1ULL << 2ULL)  /* Can't hear shouts                    */
#define PRF_NOTELL					(1ULL << 3ULL)  /* Can't receive tells                  */
#define PRF_DISPHP					(1ULL << 4ULL)  /* Display hit points in prompt         */
#define PRF_DISPMANA				(1ULL << 5ULL)  /* Display mana points in prompt        */
#define PRF_DISPMOVE				(1ULL << 6ULL)  /* Display move points in prompt        */
#define PRF_AUTOEXIT				(1ULL << 7ULL)  /* Display exits in a room              */
#define PRF_NOHASSLE				(1ULL << 8ULL)  /* Aggr mobs won't attack               */
#define PRF_QUEST						(1ULL << 9ULL)  /* On quest                             */
#define PRF_SUMMONABLE			(1ULL << 10ULL) /* Can be summoned                      */
#define PRF_NOREPEAT				(1ULL << 11ULL) /* No repetition of comm commands       */
#define PRF_HOLYLIGHT				(1ULL << 12ULL) /* Can see in dark                      */
#define PRF_COLOR_1					(1ULL << 13ULL) /* Color (low bit)                      */
#define PRF_COLOR_2					(1ULL << 14ULL) /* Color (high bit)                     */
#define PRF_NOWIZ						(1ULL << 15ULL) /* Can't hear wizline                   */
#define PRF_LOG1						(1ULL << 16ULL) /* On-line System Log (low bit)         */
#define PRF_LOG2						(1ULL << 17ULL) /* On-line System Log (high bit)        */
#define PRF_NOSOC						(1ULL << 18ULL) /* Can't hear grats channel             */
#define PRF_ROOMFLAGS				(1ULL << 19ULL) /* Can see room flags (ROOM_x)          */
#define PRF_CLS							(1ULL << 20ULL) /* Clear screen in OasisOLC             */
#define PRF_AUTOASSIST			(1ULL << 21ULL) /* Auto-assist group leader             */
#define PRF_AUTOSAC					(1ULL << 22ULL) /* Auto-sac if killed a mob             */
#define PRF_AUTOSPLIT				(1ULL << 23ULL) /* Auto-splits loot if killed mob       */
#define PRF_AUTOLOOT				(1ULL << 24ULL) /* Auto-loots the mob                   */
#define PRF_AUTOGOLD				(1ULL << 25ULL) /* Auto-loots the mob                   */
#define PRF_METERBAR				(1ULL << 26ULL) /* Can see meterbar in score            */
#define PRF_EMAIL						(1ULL << 27ULL) /* Player is allowed to email           */
#define PRF_NONEWBIE				(1ULL << 28ULL) /* Can't hear newbie channel            */
#define PRF_TIPCHANNEL			(1ULL << 29ULL)	/* Player can see the Tips and Tricks   */
#define PRF_SHOWVNUMS				(1ULL << 30ULL)	/* Player sees vnums in rooms and inv   */
#define PRF_NOSING					(1ULL << 31ULL) /* Can't hear the sing channel					*/
#define PRF_NOOBSCENE				(1ULL << 32ULL) /* Can't hear the obscene channel				*/
#define PRF_SKILLGAINS			(1ULL << 33ULL) /* Player is Away from Window						*/
#define PRF_TIMESTAMPS			(1ULL << 34ULL) /* Shows timestamps in tells and others	*/
#define PRF_HASPROMPT				(1ULL << 35ULL) /* Does the player have a prompt?				*/
#define PRF_REALSKILLS			(1ULL << 36ULL) /* Show real skill values in skill list	*/
#define PRF_POSEID					(1ULL << 37ULL) /* Shows pose ID after .. poses ...			*/

/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved")   */
#define	AFF_BLIND						(1ULL << 0ULL)	/* (R) Char is blind             */
#define	AFF_INVISIBLE				(1ULL << 1ULL)	/* Char is invisible             */
#define	AFF_DETECT_ALIGN		(1ULL << 2ULL)	/* Char is sensitive to align    */
#define	AFF_DETECT_INVIS		(1ULL << 3ULL)	/* Char can see invis chars      */
#define	AFF_DETECT_MAGIC		(1ULL << 4ULL)	/* Char is sensitive to magic    */
#define	AFF_SENSE_LIFE			(1ULL << 5ULL)	/* Char can sense hidden life    */
#define	AFF_WATERWALK				(1ULL << 6ULL)	/* Char can walk on water        */
#define	AFF_SANCTUARY				(1ULL << 7ULL)	/* Char protected by sanct.      */
#define	AFF_GROUP						(1ULL << 8ULL)	/* (R) Char is grouped           */
#define	AFF_CURSE						(1ULL << 9ULL)	/* Char is cursed                */
#define	AFF_INFRAVISION			(1ULL << 10ULL)	/* Char can see in dark          */
#define	AFF_POISON					(1ULL << 11ULL)	/* (R) Char is poisoned          */
#define	AFF_PROTECT_EVIL		(1ULL << 12ULL)	/* Char protected from evil      */
#define	AFF_PROTECT_GOOD		(1ULL << 13ULL)	/* Char protected from good      */
#define	AFF_SLEEP						(1ULL << 14ULL)	/* (R) Char magically asleep     */
#define	AFF_NOTRACK					(1ULL << 15ULL)	/* Char can't be tracked         */
#define	AFF_SHROUD					(1ULL << 16ULL)	/* Char is shrouded (plus to AC) */
#define	AFF_ORB							(1ULL << 17ULL)	/* Char is protected by orb      */
#define	AFF_SNEAK						(1ULL << 18ULL)	/* Char can move quietly         */
#define	AFF_HIDE						(1ULL << 19ULL)	/* Char is hidden                */
#define	AFF_HASTE						(1ULL << 20ULL)	/* Char can move faster          */
#define	AFF_CHARM						(1ULL << 21ULL)	/* Char is charmed               */
#define	AFF_BARKSKIN				(1ULL << 22ULL)	/* Minor protection              */
#define	AFF_STONESKIN				(1ULL << 23ULL)	/* Medium protection             */
#define	AFF_STEELSKIN				(1ULL << 24ULL)	/* Major protection              */
#define	AFF_SEE_FAERIE			(1ULL << 25ULL)	/* Char can see FAERIE exits     */
#define	AFF_SEE_INFERNAL		(1ULL << 26ULL)	/* Char can see INFERNAL exits   */
#define	AFF_SEE_DIVINE			(1ULL << 27ULL)	/* Char can see DIVINE exits     */
#define	AFF_SEE_ANCIENT			(1ULL << 28ULL)	/* Char can see ANCIENT exits    */
#define AFF_CHASE						(1ULL << 29ULL) /* Player will chase a wimpy mob */

#define	NUM_AFF_FLAGS				30


/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING					0		/* Playing - Nominal state			*/
#define CON_CLOSE						1		/* Disconnecting								*/
#define CON_GET_NAME				2		/* By what name ..?							*/
#define CON_NAME_CNFRM			3		/* Did I get that right, x?			*/
#define CON_PASSWORD				4		/* Password:										*/
#define CON_NEWPASSWD				5		/* Give me a password for x			*/
#define CON_CNFPASSWD				6		/* Please retype password:			*/
#define CON_QSEX						7		/* Sex													*/
#define CON_RMOTD						8		/* PRESS RETURN after MOTD			*/
#define CON_MENU						9		/* Your choice: (main menu)			*/
#define CON_CHPWD_GETOLD		10	/* Changing passwd: get old			*/
#define CON_CHPWD_GETNEW		11	/* Changing passwd: get new			*/
#define CON_CHPWD_VRFY			12	/* Verify new password					*/
#define CON_DELCNF1					13	/* Delete confirmation 1				*/
#define CON_DELCNF2					14	/* Delete confirmation 2				*/
#define CON_DISCONNECT			15	/* In-game disconnection				*/
#define CON_OEDIT						16	/* OLC mode - object editor			*/
#define CON_REDIT						17	/* OLC mode - room editor				*/
#define CON_ZEDIT						18	/* OLC mode - zone info editor	*/
#define CON_MEDIT						19	/* OLC mode - mobile editor			*/
#define CON_SEDIT						20	/* OLC mode - shop editor				*/
#define CON_QCULTURE				21	/* Select Culture								*/
#define CON_QROLLSTATS			22	/* Roll stats										*/
#define CON_QCONFIRMCULTURE	23	/* Confirm Race 1								*/
#define CON_AEDIT						24	/* OLC mode - Action editor			*/
#define CON_TRIGEDIT				25	/* OLC mode - Trigger Editor		*/
#define CON_ASSEDIT					26	/* OLC mode - Assemblies				*/
#define CON_EMAIL						27	/* OLC mode - email editor -spl	*/
#define CON_EGET						28	/* Get email addy for x -spl		*/
#define CON_QEDIT						29	/* OLC mode - quest edit				*/
#define CON_QANSI						30	/* ANSI color compatible term?	*/
#define CON_QCONFIRMEMAIL		31	/* Confirm email address				*/
#define CON_QHEIGHT					32	/* Get height										*/
#define CON_QWEIGHT					33	/* Get weight										*/
#define CON_QEYECOLOR				34	/* Get eye color								*/
#define CON_QHAIRSTYLE			35	/* Get hair style								*/
#define CON_QHAIRCOLOR			36	/* Get hair color								*/
#define CON_QSKINTONE				37	/* Get skin tone								*/
#define CON_GET_BECOME			38	/* Who to change into						*/
#define CON_QCONFIRMELIST		39	/* Confirm email-list addition	*/
#define CON_CHANGEEMAIL			40	/* Changing email address				*/
#define CON_SPEDIT					41	/* OLC mode - Spell editor			*/
#define CON_DESCEDIT				42	/* RP Description Editor				*/
#define CON_COPYOVER				43	/* Ready for copyover						*/
#define CON_RENAME					44	/* Forced rename state					*/
#define CON_CINFOEDIT				45	/* Contact Info Editor					*/
#define CON_GEDIT						46	/* Guild editor									*/
#define CON_COMEDIT					47	/* OLC mode - Command Editor		*/
#define CON_TUTOREDIT				48	/* Tutor editor									*/
#define CON_BACKGROUND			49	/* Edit background							*/
#define CON_QVERIFICATION		50	/* In the email verification		*/
#define CON_QSELECTSKILLS		51	/* Selecting starting skills		*/
#define CON_HEDIT						52	/* OLC mode - House editor			*/
#define CON_QHOMETOWN				53	/* Get home town								*/

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
	which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT					0
#define WEAR_FINGER_R				1
#define WEAR_FINGER_L				2
#define WEAR_NECK						3
#define WEAR_BODY						4
#define WEAR_HEAD						5
#define WEAR_LEGS						6
#define WEAR_FEET						7
#define WEAR_HANDS					8
#define WEAR_ARMS						9
#define WEAR_SHIELD					10
#define WEAR_ABOUT					11
#define WEAR_WAIST					12
#define WEAR_WRIST_R				13
#define WEAR_WRIST_L				14
#define WEAR_WIELD					15
#define WEAR_HOLD						16
#define WEAR_DWIELD					17
#define WEAR_FACE						18
#define WEAR_FLOAT					19
#define WEAR_BACK						20
#define WEAR_BELT_1					21
#define WEAR_BELT_2					22
#define WEAR_OUTSIDE				23
#define WEAR_THROAT_1				24
#define WEAR_THROAT_2				25
#define WEAR_WINGS					26
#define WEAR_HORNS					27
#define WEAR_TAIL						28

#define NUM_WEARS						29	/* This must be the # of eq positions!! */

/* Defines for body parts. Used by player.body_bits */
#define BODY_LIGHT					(1 << 0)
#define BODY_FINGER_R				(1 << 1)
#define BODY_FINGER_L				(1 << 2)
#define BODY_NECK						(1 << 3)
#define BODY_BODY						(1 << 4)
#define BODY_HEAD						(1 << 5)
#define BODY_LEGS						(1 << 6)
#define BODY_FEET						(1 << 7)
#define BODY_HANDS					(1 << 8)
#define BODY_ARMS						(1 << 9)
#define BODY_SHIELD					(1 << 10)
#define BODY_ABOUT					(1 << 11)
#define BODY_WAIST					(1 << 12)
#define BODY_WRIST_R				(1 << 13)
#define BODY_WRIST_L				(1 << 14)
#define BODY_WIELD					(1 << 15)
#define BODY_HOLD						(1 << 16)
#define BODY_DWIELD					(1 << 17)
#define BODY_FACE						(1 << 18)
#define BODY_FLOAT					(1 << 19)
#define BODY_BACK						(1 << 20)
#define BODY_BELT_1					(1 << 21)
#define BODY_BELT_2					(1 << 22)
#define BODY_OUTSIDE				(1 << 23)
#define BODY_THROAT_1				(1 << 24)
#define BODY_THROAT_2				(1 << 25)
#define BODY_WINGS					(1 << 26)
#define BODY_HORNS					(1 << 27)
#define BODY_TAIL						(1 << 28)

/* Defines for body sizes. Used by races_list */
#define SIZE_NONE						(-1)
#define SIZE_TINY						0
#define SIZE_SMALL					1
#define SIZE_MEDIUM					2
#define SIZE_LARGE					3
#define SIZE_HUGE						4

#define NUM_SIZES						6

/* Defines for immortal travels */
#define TRAV_TIN						0
#define TRAV_TOUT						1
#define TRAV_MIN						2
#define TRAV_MOUT						3
#define TRAV_GIN						4
#define TRAV_GOUT						5
#define TRAV_IIN						6
#define TRAV_IOUT						7

#define NUM_TRAVELS					8


/* object-related defines ********************************************/


/* Item types: used by obj_data.obj_flags.type_flag              */
#define ITEM_LIGHT					1    /* Item is a light source       */
#define ITEM_SCROLL					2    /* Item is a scroll             */
#define ITEM_WAND						3    /* Item is a wand               */
#define ITEM_STAFF					4    /* Item is a staff              */
#define ITEM_WEAPON					5    /* Item is a weapon             */
#define ITEM_FIREWEAPON			6    /* Unimplemented                */
#define ITEM_MISSILE				7    /* Unimplemented                */
#define ITEM_TREASURE				8    /* Item is a treasure, not gold */
#define ITEM_ARMOR					9    /* Item is armor                */
#define ITEM_POTION					10   /* Item is a potion             */
#define ITEM_WORN						11   /* Unimplemented                */
#define ITEM_OTHER					12   /* Misc object                  */
#define ITEM_TRASH					13   /* Trash - shopkeeps won't buy  */
#define ITEM_TRAP						14   /* Unimplemented                */
#define ITEM_CONTAINER			15   /* Item is a container          */
#define ITEM_NOTE						16   /* Item is note                 */
#define ITEM_DRINKCON				17   /* Item is a drink container    */
#define ITEM_KEY						18   /* Item is a key                */
#define ITEM_FOOD						19   /* Item is food                 */
#define ITEM_MONEY					20   /* Item is money (gold)         */
#define ITEM_PEN						21   /* Item is a pen                */
#define ITEM_BOAT						22   /* Item is a boat               */
#define ITEM_FOUNTAIN				23   /* Item is a fountain           */
#define ITEM_FLIGHT					24   /* Item enables flight          */
#define ITEM_PORTAL					25   /* Item is a portal             */
#define ITEM_SPELLBOOK			26   /* Item is a spellbook          */
#define ITEM_TOOL						27   /* Item is a tool               */
#define ITEM_RESOURCE				28   /* Item is a resource           */
#define ITEM_SHEATH					29	 /* Item is a sheath						 */

#define NUM_ITEM_TYPES			30


/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE			(1ULL << 0ULL)  /* Item can be taken         */
#define ITEM_WEAR_FINGER		(1ULL << 1ULL)  /* Can be worn on finger     */
#define ITEM_WEAR_NECK			(1ULL << 2ULL)  /* Can be worn around neck   */
#define ITEM_WEAR_BODY			(1ULL << 3ULL)  /* Can be worn on body       */
#define ITEM_WEAR_HEAD			(1ULL << 4ULL)  /* Can be worn on head       */
#define ITEM_WEAR_LEGS			(1ULL << 5ULL)  /* Can be worn on legs       */
#define ITEM_WEAR_FEET			(1ULL << 6ULL)  /* Can be worn on feet       */
#define ITEM_WEAR_HANDS			(1ULL << 7ULL)  /* Can be worn on hands      */
#define ITEM_WEAR_ARMS			(1ULL << 8ULL)  /* Can be worn on arms       */
#define ITEM_WEAR_SHIELD		(1ULL << 9ULL)  /* Can be used as a shield   */
#define ITEM_WEAR_ABOUT			(1ULL << 10ULL) /* Can be worn about body    */
#define ITEM_WEAR_WAIST			(1ULL << 11ULL) /* Can be worn around waist  */
#define ITEM_WEAR_WRIST			(1ULL << 12ULL) /* Can be worn on wrist      */
#define ITEM_WEAR_WIELD			(1ULL << 13ULL) /* Can be wielded            */
#define ITEM_WEAR_HOLD			(1ULL << 14ULL) /* Can be held               */
#define ITEM_WEAR_DWIELD		(1ULL << 15ULL) /* Can be dwielded           */
#define ITEM_WEAR_EAR				(1ULL << 16ULL) /* Can be worn on ears  		 */
#define ITEM_WEAR_FACE			(1ULL << 17ULL) /* Can be worn on face  		 */
#define ITEM_WEAR_FLOAT			(1ULL << 18ULL) /* Can be floating           */
#define ITEM_WEAR_BACK      (1ULL << 19ULL) /* Can be worn on back       */
#define ITEM_WEAR_BELT			(1ULL << 20ULL) /* Can be worn on a belt		 */
#define ITEM_WEAR_OUTSIDE		(1ULL << 21ULL) /* Can be worn outside of clothing */
#define ITEM_WEAR_THROAT		(1ULL << 22ULL) /* Can be worn around throat */
#define ITEM_WEAR_WINGS			(1ULL << 23ULL) /* Can be worn on wings			 */
#define ITEM_WEAR_HORNS			(1ULL << 24ULL) /* Can be worn on horns			 */
#define ITEM_WEAR_TAIL			(1ULL << 25ULL) /* Can be worn around tail	 */

#define NUM_ITEM_WEARS			26


/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW								(1ULL << 0ULL)	/* Item is glowing               */
#define ITEM_HUM								(1ULL << 1ULL)	/* Item is humming               */
#define ITEM_NORENT							(1ULL << 2ULL)	/* Item cannot be rented         */
#define ITEM_NODONATE						(1ULL << 3ULL)	/* Item cannot be donated        */
#define ITEM_NOINVIS						(1ULL << 4ULL)	/* Item cannot be made invis     */
#define ITEM_INVISIBLE					(1ULL << 5ULL)	/* Item is invisible             */
#define ITEM_MAGIC							(1ULL << 6ULL)	/* Item is magical               */
#define ITEM_NODROP							(1ULL << 7ULL)	/* Item is cursed: can't drop    */
#define ITEM_BLESS							(1ULL << 8ULL)	/* Item is blessed               */

#define ITEM_ANTI_GOOD					(1ULL << 9ULL)	/* Not usable by good people     */
#define ITEM_ANTI_EVIL					(1ULL << 10ULL)	/* Not usable by evil people     */
#define ITEM_ANTI_NEUTRAL				(1ULL << 11ULL)	/* Not usable by neutral people  */
 
#define ITEM_ANTI_MAGI_TYPE			(1ULL << 12ULL)	/* Not usable by mages           */
#define ITEM_ANTI_CLERIC_TYPE		(1ULL << 13ULL)	/* Not usable by clerics         */
#define ITEM_ANTI_THIEF_TYPE		(1ULL << 14ULL)	/* Not usable by thieves         */
#define ITEM_ANTI_WARRIOR_TYPE	(1ULL << 15ULL)	/* Not usable by warriors        */

#define ITEM_NOSELL							(1ULL << 16ULL)	/* Shopkeepers won't touch it    */

#define ITEM_NODISARM						(1ULL << 17ULL)	/* Cannot disarm (weapons only)  */

#define ITEM_ANTI_FAE						(1ULL << 18ULL)	/* Not usable by fae races       */
#define ITEM_ANTI_DEMONIC				(1ULL << 19ULL)	/* Not usable by demonic races   */
#define ITEM_ANTI_DIVINE				(1ULL << 20ULL)	/* Not usable by divine races    */
#define ITEM_ANTI_VAMPIRE				(1ULL << 21ULL)	/* Not usable by dwarves         */
#define ITEM_ANTI_LITTLE_FAE		(1ULL << 22ULL)	/* Not usable by halflings       */
#define ITEM_ANTI_UNDEAD				(1ULL << 23ULL)	/* Not usable by gnomes          */
#define ITEM_OK_ANIMAL					(1ULL << 24ULL)	/* Not usable by giants          */
#define ITEM_ANTI_DEITY					(1ULL << 25ULL)	/* Not usable by wolvens         */
#define ITEM_ANTI_HUMAN					(1ULL << 26ULL)	/* Not usable by humans          */
#define ITEM_NODISPLAY					(1ULL << 27ULL)	/* Item does not show up in room */
#define ITEM_TWO_HANDED					(1ULL << 28ULL)	/* Item is two handed            */

#define ITEM_NOSTEAL						(1ULL << 29ULL)	/* Item cannot be stolen (magic) */

#define ITEM_UNIQUE_SAVE				(1ULL << 30ULL)	/* Item is saved unique          */
#define ITEM_COLORIZE						(1ULL << 31ULL)	/* Item is saved unique          */

#define NUM_ITEM_FLAGS					32


/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE							0  /* No effect                    */
#define APPLY_STRENGTH					1  /* Apply to strength            */
#define APPLY_AGILITY						2  /* Apply to agility	           */
#define APPLY_PRECISION					3  /* Apply to precision		       */
#define APPLY_PERCEPTION				4  /* Apply to perception          */
#define APPLY_HEALTH						5  /* Apply to health				       */
#define APPLY_WILLPOWER					6  /* Apply to willpower           */
#define APPLY_INTELLIGENCE			7	 /* Apply to intelligence				 */
#define APPLY_CHARISMA					8  /* Apply to charisma						 */
#define APPLY_LUCK							9  /* Apply to luck								 */
#define APPLY_ESSENCE						10 /* Apply to essence						 */
#define APPLY_CLASS							11 /* Reserved                     */
#define APPLY_AGE								12 /* Apply to age                 */
#define APPLY_CHAR_WEIGHT				13 /* Apply to weight              */
#define APPLY_CHAR_HEIGHT				14 /* Apply to height              */
#define APPLY_MANA							15 /* Apply to max mana            */
#define APPLY_HIT								16 /* Apply to max hit points      */
#define APPLY_MOVE							17 /* Apply to max move points     */
#define APPLY_GOLD							18 /* Reserved                     */
#define APPLY_EXP								19 /* Reserved                     */
#define APPLY_AC								20 /* Apply to Armor Class         */
#define APPLY_HITROLL						21 /* Apply to hitroll             */
#define APPLY_DAMROLL						22 /* Apply to damage roll         */
#define APPLY_SAVING_PARA				23 /* Apply to save throw: paralz  */
#define APPLY_SAVING_ROD				24 /* Apply to save throw: rods    */
#define APPLY_SAVING_PETRI			25 /* Apply to save throw: petrif  */
#define APPLY_SAVING_BREATH			26 /* Apply to save throw: breath  */
#define APPLY_SAVING_SPELL			27 /* Apply to save throw: spells  */
#define APPLY_RACE							28 /* Apply to race                */

#define NUM_APPLIES							29


/* Container flags - value[1] */
#define CONT_CLOSEABLE					(1ULL << 0ULL)  /* Container can be closed   */
#define CONT_PICKPROOF					(1ULL << 1ULL)  /* Container is pickproof    */
#define CONT_CLOSED							(1ULL << 2ULL)  /* Container is closed       */
#define CONT_LOCKED							(1ULL << 3ULL)  /* Container is locked       */


/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER								0
#define LIQ_BEER								1
#define LIQ_WINE								2
#define LIQ_ALE									3
#define LIQ_DARKALE							4
#define LIQ_WHISKY							5
#define LIQ_LEMONADE						6
#define LIQ_FIREBRT							7
#define LIQ_LOCALSPC						8
#define LIQ_SLIME								9
#define LIQ_MILK								10
#define LIQ_TEA									11
#define LIQ_COFFE								12
#define LIQ_BLOOD								13
#define LIQ_SALTWATER						14
#define LIQ_CLEARWATER					15
#define LIQ_CHAMPAGNE						16
#define LIQ_MEAD								17
#define LIQ_COLOR								18

#define NUM_LIQ_TYPES						19


/* other miscellaneous defines *******************************************/

/* Edit Modes (Replaces flags and connection states and crap) */
#define EDIT_EXDESC							0
#define EDIT_MAIL								1
#define EDIT_BOARD							2
#define EDIT_NOTE								3
#define EDIT_BACKGROUND					4
#define EDIT_RPDESC							5
#define EDIT_CONTACTINFO				6
#define EDIT_GUILD							7
#define EDIT_TEXTEDIT						8


/* Player conditions */
#define DRUNK										0
#define FULL										1
#define THIRST									2


/* Sun state for weather_data */
#define SUN_DARK								0
#define SUN_RISE								1
#define SUN_LIGHT								2
#define SUN_SET									3


/* Sky conditions for weather_data */
#define SKY_CLOUDLESS						0
#define SKY_CLOUDY							1
#define SKY_RAINING							2
#define SKY_LIGHTNING						3


/* Rent codes */
#define RENT_UNDEF							0
#define RENT_CRASH							1
#define RENT_RENTED							2
#define RENT_CRYO								3
#define RENT_FORCED							4
#define RENT_TIMEDOUT						5


/* other #defined constants **********************************************/

/* event types:  */
#define EVENT_SKILL							0
#define EVENT_SPELL 						1
#define EVENT_SYSTEM 						2

/* do_lore */
#define LORE_BOTANY							0
#define LORE_GEOGRAPHY					1
#define LORE_ARCANE							2
#define LORE_THEOLOGY						3
#define LORE_ZOOLOGY						4
#define LORE_LINGUISTICS				5
#define LORE_SURVIVAL						6
#define LORE_REALMS							7
#define LORE_OCCULT							8
#define LORE_ANTHROPOLOGY				9
#define LORE_STREETWISE					10

#define NUM_LORES								11


/* resource related #defined constants **********************************/

#define NUM_RESOURCE_GROUPS			5

#define RESOURCE_ORE						0
#define RESOURCE_GEMS						1
#define RESOURCE_WOOD						2
#define RESOURCE_STONE					3
#define RESOURCE_FABRIC					4

/* Resource group RESOURCE_ORE */
#define ORES_IRON								(1ULL << 0ULL)	/* Resource type Copper */
#define ORES_COPPER							(1ULL << 1ULL)	/* Resource type Copper */
#define ORES_SILVER							(1ULL << 2ULL)	/* Resource type Silver */
#define ORES_GOLD								(1ULL << 3ULL)	/* Resource type Gold */
#define ORES_TIN								(1ULL << 4ULL)	/* Resource type Tin */
#define ORES_LEAD								(1ULL << 5ULL)	/* Resource type Lead */

/* Resource group RESOURCE_GEMS */
#define GEMS_EMERALDS						(1ULL << 6ULL)	/* Resource type Emerald */
#define GEMS_RUBIES							(1ULL << 7ULL)	/* Resource type Ruby */
#define GEMS_SAPPHIRES					(1ULL << 8ULL)	/* Resource type Sapphire */
#define GEMS_DIAMONDS						(1ULL << 9ULL)	/* Resource type Diamond */
#define GEMS_TOPAZ							(1ULL << 10ULL)	/* Resource type Topaz */
#define GEMS_AMETHYST						(1ULL << 11ULL)	/* Resource type Amethyst */

/* Resource group RESOURCE_WOOD */
#define WOODS_OAK								(1ULL << 12ULL)	/* Resource type Oak */
#define WOODS_PINE							(1ULL << 13ULL)	/* Resource type Pine */
#define WOODS_BEECH							(1ULL << 14ULL)	/* Resource type Beech */
#define WOODS_ASH								(1ULL << 15ULL)	/* Resource type Ash */
#define WOODS_ROWAN							(1ULL << 16ULL)	/* Resource type Rowan */
#define WOODS_YEW								(1ULL << 17ULL)	/* Resource type Yew */

/* Resource group RESOURCE_STONE */
#define STONE_GRANITE						(1ULL << 18ULL)	/* Resource type Granite */
#define STONE_MARBLE						(1ULL << 19ULL)	/* Resource type Marble */
#define STONE_SANDSTONE					(1ULL << 20ULL)	/* Resource type Sandstone */
#define STONE_LIMESTONE					(1ULL << 21ULL)	/* Resource type Limestone */
#define STONE_FLINT							(1ULL << 22ULL)	/* Resource type Flint */
#define STONE_SLATE							(1ULL << 23ULL)	/* Resource type Slate */

/* Resource group RESOURCE_FABRIC */
#define FABRIC_FLEECE						(1ULL << 24ULL)	/* Resource type Fleece */
#define FABRIC_FLAX							(1ULL << 25ULL)	/* Resource type Flax */
#define FABRIC_HEMP							(1ULL << 26ULL)	/* Resource type Hemp */
#define FABRIC_SILK							(1ULL << 27ULL)	/* Resource type Silk */
#define FABRIC_LEATHER					(1ULL << 28ULL)	/* Resource type Leather */
#define FABRIC_FUR							(1ULL << 29ULL)	/* Resource type Fur */

#define NUM_RESOURCES						30
#define NUM_RESOURCE_TYPES			6

/* CRASH_x for house/player object rent files */
#define CRASH_PLAYER						0
#define CRASH_HOUSE							1

/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */
#define EXP_MAX									400000000

#define MAGIC_NUMBER						(0x06)  /* Arbitrary number that won't be in a string */

/*
 * OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset your players
 * as actions (such as large speedwalking chains) take longer to be executed.
 * You shouldn't need to adjust this.
 */
#define OPT_USEC								100000    /* 10 passes per second */
#define PASSES_PER_SEC					(1000000 / OPT_USEC)
#define RL_SEC									* PASSES_PER_SEC

#define PULSE_ZONE							(10 RL_SEC)
#define PULSE_MOBILE						(10 RL_SEC)
#define PULSE_VIOLENCE					( 2 RL_SEC)
#define PULSE_SKILL							( 4 RL_SEC)
#define PULSE_AUTOSAVE					(60 RL_SEC)
#define PULSE_IDLEPWD						(15 RL_SEC)
#define PULSE_SANITY						(30 RL_SEC)
#define PULSE_USAGE							(5 * 60 RL_SEC)  /* 5 mins */
#define PULSE_TIMESAVE					(30 * 60 RL_SEC) /* should be >= SECS_PER_MUD_HOUR */

/* Variables for the output buffering system */
#define MAX_SOCK_BUF						(18 * 1024) /* Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH				256         /* Max length of prompt        */
#define GARBAGE_SPACE						32          /* Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE						4096        /* Static output buffer size   */
/* Max amount of output that can be buffered */
#define LARGE_BUFSIZE						(MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

#define HISTORY_SIZE						5			/* Keep last 5 commands. */
#define MAX_STRING_LENGTH				32768
#define MAX_INPUT_LENGTH				2048	/* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH		4096	/* Max size of *raw* input */
#define MAX_MESSAGES						60
#define MAX_NAME_LENGTH					20		/* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH					32		/* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH				80		/* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH							30		/* Used in char_file_u *DO*NOT*CHANGE* */
#define EXDSCR_LENGTH						1024	/* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TONGUE							3			/* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SKILLS							200		/* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT							32		/* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT					6
#define MAX_NOTE_LENGTH					1000	/* arbitrary */
#define MAX_BACKGROUND_LENGTH		8192
#define MAX_ALIAS_LENGTH				128
#define MAX_COMPLETED_QUESTS		1024

#define MAX_PROFS								100

#define RPLOG_SIZE							10
#define TELLS_SIZE							10
#define COMM_SIZE								15

#define NUM_DESCS								5

#define MAX_RECOGNIZED					20
#define MAX_NOTIFY							20

/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  If you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  If you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */
#if	defined(HAVE_UNSAFE_CRYPT) && MAX_PWD_LENGTH == 10
#error You need to increase MAX_PWD_LENGTH to at least 20.
#error See the comment near these errors for more explanation.
#endif

/**********************************************************************
*	Structures                                                          *
**********************************************************************/


typedef	signed char					sbyte;
typedef	unsigned char				ubyte;
typedef	signed short int		sh_int;
typedef	unsigned short int	ush_int;
typedef signed long					sh_long;
typedef unsigned long				ush_long;

#if	!defined(__cplusplus)  /* Anyone know a portable method? */
typedef	char      bool;
#endif

#if	!defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)  /* Hm, sysdep.h? */
typedef	char      byte;
#endif

/* Various virtual (human-reference) number types. */
typedef IDXTYPE room_vnum;
typedef IDXTYPE obj_vnum;
typedef IDXTYPE mob_vnum;
typedef IDXTYPE zone_vnum;
typedef IDXTYPE shop_vnum;
typedef IDXTYPE house_vnum;

/* Various real (array-reference) number types. */
typedef IDXTYPE room_rnum;
typedef IDXTYPE obj_rnum;
typedef IDXTYPE mob_rnum;
typedef IDXTYPE zone_rnum;
typedef IDXTYPE shop_rnum;
typedef IDXTYPE house_rnum;

/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 * 'unsigned long long' will give you at least 64 bits if you have GCC.
 *
 * Since we don't want to break the pfiles, you'll have to search throughout
 * the code for "bitvector_t" and change them yourself if you'd like this
 * extra flexibility.
 */
typedef	unsigned long long int bitvector_t;

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
	char  *keyword;                 /* Keyword in look/examine          */
	char  *description;             /* What to see                      */
	struct extra_descr_data *next;  /* Next in list                     */
};


/* object-related structures ******************************************/


#define NUM_OBJ_VAL_POSITIONS 4
/* object flags; used in obj_data */
struct obj_flag_data {
	int  value[NUM_OBJ_VAL_POSITIONS];	/* Values of the item (see list)		*/
	byte type_flag; 										/* Type of item											*/
	int  skill;													/* associated skill									*/
	bitvector_t  wear_flags;						/* Where you can wear it						*/
	bitvector_t  extra_flags;						/* If it hums, glows, etc.					*/
	int  weight;												/* Weight what else									*/
	int  cost;													/* Value when sold (gp.)						*/
	int  cost_per_day;									/* Cost to keep pr. real day				*/
	int  timer;													/* Timer for object									*/
	bitvector_t  bitvector;							/* To set chars bits								*/
	byte size;													/* Object was made for this size		*/
	byte color;													/* Defines custom object dye color	*/
	byte resource;											/* Defines custom object dye color	*/
};


struct obj_affected_type {
	byte location;      /* Which ability to change (APPLY_XXX) */
	sbyte modifier;     /* How much it changes by              */
};


/* ================== Memory Structure for Objects ================== */
struct obj_data {
	obj_vnum item_number;						/* Where in data-base								*/
	room_rnum in_room;							/* In what room -1 when conta/carr	*/

	obj_vnum proto_number;					/* used to be in data-base					*/

	struct obj_flag_data obj_flags;	/* Object information								*/
	struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects			*/

	char  *name;										/* Title of object :get etc.				*/
	char  *description;							/* When in room											*/
	char  *short_description;				/* when worn/carry/in cont.					*/
	char  *action_description;			/* What to write when used					*/
	struct extra_descr_data *ex_description; /* extra descriptions			*/
	struct char_data *carried_by;		/* Carried by :NULL in room/conta		*/
	struct char_data *worn_by;			/* Worn by?													*/
	sh_int worn_on;									/* Worn where?											*/

	struct obj_data *in_obj;				/* In what object NULL when none		*/
	struct obj_data *contains;			/* Contains objects									*/

	long id;												/* used by DG triggers							*/
	struct trig_proto_list *proto_script;	/* list of default triggers		*/
	struct script_data *script;			/* script info for the object				*/

	struct obj_data *next_content;	/* For 'contains' lists							*/
	struct obj_data *next;					/* For the object list							*/

	long unique_id;									/* used by Rent Files for storage		*/

};
/* ======================================================================= */


/* ====================== File Element for Objects ======================= */
/*								BEWARE: Changing it will ruin rent files       */

/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
struct rent_info {
	int  time;
	int  rentcode;
	int  net_cost_per_diem;
	int  gold;
	int  account;
	int  nitems;
	int  spare0;
	int  spare1;
	int  spare2;
	int  spare3;
	int  spare4;
	int  spare5;
	int  spare6;
	int  spare7;
};
/* ======================================================================= */


/* room-related structures ************************************************/

/* structure for resources */
struct resource_data {
	bitvector_t resources;
	ush_int current[NUM_RESOURCE_GROUPS];
	ush_int max[NUM_RESOURCE_GROUPS];
};

struct room_direction_data {
	char  *general_description;       /* When look DIR.      */

	char  *keyword;    /* for open/close      */

	bitvector_t exit_info;  /* Exit info      */
	obj_vnum key;    /* Key's number (-1 for no key)    */
	room_rnum to_room;    /* Where direction leads (NOWHERE)  */
};


/* ================== Memory Structure for email ======================= */
struct email_data {
	char *from;      /* Return address       */
	char  *target;    /* To whom it gets sent     */
	char *subject;    /* Message subject       */
	char *body;      /* Message body       */
};


/* ================== Memory Structure for room ======================= */
struct room_data {
	room_vnum number;							/* Rooms number  (vnum)								*/
	zone_rnum zone;								/* Room zone (for resetting)          */
	int  sector_type;							/* sector type (move/hide)            */
	char  *name;									/* Rooms name 'You are ...'						*/
	char  *description;						/* Shown when entered									*/
	struct extra_descr_data *ex_description; /* for examine/look				*/
	struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions	*/
	bitvector_t room_flags;				/* DEATH,DARK ... etc */
	
	byte light;										/* Number of lightsources in room			*/
	int specproc;
	SPECIAL(*func);

	ush_int	magic_flux;						/* magic flux type										*/
	ush_int	available_flux;				/* amount of magical flux available		*/

	struct resource_data resources;	/* resource data for the location	*/
	
	struct trig_proto_list *proto_script; /* list of default triggers		*/
	struct script_data *script;		/* script info for the object					*/
	
	struct obj_data *contents;		/* List of items in room							*/
	struct char_data *people;			/* List of NPC / PC in room           */
};
/* ====================================================================== */

/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
	long  id;
	struct memory_rec_struct *next;
};

typedef	struct memory_rec_struct memory_rec;


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
	int hours, day, month, year;
};


/* These data contain information about a players time data */
struct time_data {
	time_t birth;    /* This represents the characters age                */
	time_t logon;    /* Time of the last logon (used to calculate played) */
	int	played;     /* This is the total accumulated time played in secs */
};


/* The pclean_criteria_data is set up in config.c and used in db.c to
	determine the conditions which will cause a player character to be
	deleted from disk if the automagic pwipe system is enabled (see config.c).
*/
struct pclean_criteria_data {
	bitvector_t rights;    /* max rights for this time limit  */
	int days;							 /* time limit in days      */
}; 


/* general player-related info, usually PC's and NPC's */
struct char_player_data {
	char  passwd[MAX_PWD_LENGTH+1];	/* character's password					*/
	char  *name;						/* PC / NPC s name (kill ...  )					*/
	char  *short_descr;			/* for NPC 'actions'										*/
	char  *long_descr;			/* for 'look'														*/
	char  *description;			/* Extra descriptions										*/
	char  *title;						/* PC / NPC's title											*/
	char email[MAX_INPUT_LENGTH];	/* PC's email address -spl				*/
	byte sex;								/* PC / NPC's sex												*/
	byte chclass;						/* PC / NPC's class											*/
	byte race;							/* PC / NPC's race											*/
	bitvector_t body_bits;	/* PC / NPC's race parts								*/
	byte size;							/* PC / NPC's body frame size						*/
	byte culture;						/* PC / NPC's culture										*/
	int  hometown;					/* PC s Hometown (zone)									*/
	struct time_data time;  /* PC's AGE in days											*/
	ubyte weight;						/* PC / NPC's weight										*/
	ubyte height;						/* PC / NPC's height										*/
	int active;							/* PC / NPC's active status (approved?)	*/
	int eyecolor;						/* PC eye color													*/
	int haircolor;					/* PC hair color												*/
	int hairstyle;					/* PC hair style												*/
	int skintone;						/* PC skintone													*/
};


/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data {
	ush_int strength;
	ush_int agility;
	ush_int precision;
	ush_int perception;
	ush_int health;
	ush_int willpower;
	ush_int intelligence;
	ush_int charisma;
	ush_int luck;
	ush_int essence;
};


/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data {

	sh_int piety;
	sh_int reputation;
	sh_int social_rank;
	sh_int military_rank;
	sh_int sanity;
	sh_int flux;
	sh_int max_flux;
	sh_int fatigue;

	sh_int mana;
	sh_int max_mana;    /* Max mana for PC/NPC         */
	sh_int hit;
	sh_int max_hit;     /* Max hit for PC/NPC          */
	sh_int move;
	sh_int max_move;    /* Max move for PC/NPC         */

	int skillcap;				/* Max skill points for PC/NPC	*/

	sh_int armor;										/* Passive Defense rating of character	 */
	sh_int damage_reduction;        /* Damage reduction rating of character	 */
	int  gold;					/* Money carried                           */
	int  bank_gold;			/* Gold the char has in a bank account     */
	int  exp;						/* The experience of the player            */

	sbyte hitroll;      /* Any bonus or penalty to the hit roll    */
	sbyte damroll;      /* Any bonus or penalty to the damage roll */
};


struct char_body_parts {
	sh_int hit_head;
	sh_int hit_neck;
	sh_int hit_torso;
	sh_int hit_abdomen;
	sh_int hit_right_arm;
	sh_int hit_left_arm;
	sh_int hit_right_hand;
	sh_int hit_left_hand;
	sh_int hit_right_leg;
	sh_int hit_left_leg;
	sh_int hit_right_foot;
	sh_int hit_left_foot;

	sh_int hit_head_max;
	sh_int hit_neck_max;
	sh_int hit_torso_max;
	sh_int hit_abdomen_max;
	sh_int hit_right_arm_max;
	sh_int hit_left_arm_max;
	sh_int hit_right_hand_max;
	sh_int hit_left_hand_max;
	sh_int hit_right_leg_max;
	sh_int hit_left_leg_max;
	sh_int hit_right_foot_max;
	sh_int hit_left_foot_max;
};


/* 
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved {
	int  alignment;								/* +-1000 for alignments										*/
	long idnum;										/* player's idnum; -1 for mobiles						*/
	bitvector_t act;								/* act flag for NPC's; player flag for PC's	*/
	bitvector_t affected_by;				/* Bitvector for spells/skills affected by	*/
	sh_int apply_saving_throw[5];	/* Saving throw (Bonuses)										*/
};


/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {
	struct char_data *fighting;  /* Opponent        */
	struct char_data *hunting;  /* Char hunted by this char    */

	byte position;    /* Standing, fighting, sleeping, etc.  */

	int  carry_weight;    /* Carried weight      */
	byte carry_items;    /* Number of items carried    */
	int  timer;      /* Timer for update      */

	struct char_special_data_saved saved; /* constants saved in plrfile  */
	struct char_guild_element *guilds_data; /* Guilds data */
};


struct wprof_data {
	int vnum;
	int prof;
};


/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved {
	long skills[MAX_SKILLS+1];				/* array of skills plus skill 0				*/
	byte skill_states[MAX_SKILLS+1];	/* array of skill sets plus set 0			*/
	byte PADDING0;										/* used to be spells_to_learn					*/
	bool talks[MAX_TONGUE];						/* PC s Tongues 0 for NPC							*/
	int  wimp_level;									/* Below this # of hit points, flee!  */
	bitvector_t invis_level;  			  /* level of invisibility							*/
	room_vnum load_room;							/* Which room to place char in				*/
	bitvector_t pref;									/* preference flags for PC's.					*/
	bitvector_t user_rights;					/* Bitvector for userrights						*/
	bitvector_t syslog;								/* Bitvector for extended syslog			*/
	bitvector_t session_flags;				/* Bitvector for session flags				*/
	ubyte bad_pws;										/* number of bad password attemps			*/
	sbyte conditions[3];							/* Drunk, full, thirsty								*/

	ubyte spheres[4];
	int spells_to_learn;    /* How many can you learn yet this level*/
	int olc_zone;
	int speaking;					/* spoken language */

	int bday_year;
	int bday_month;
	int bday_day;
	int bday_hours;

  char *contact;      /* name of immortal contact */
	bool mailinglist;		/* wants email? */

  char *approved_by;      /* name of immortal contact */

	char *true_name;
	
	unsigned int rpxp;

	char colorpref_echo;	/* Echo speech color preference */
	char colorpref_emote;   /* Emote speech color preference */
	char colorpref_pose;    /* Pose speech color preference */
	char colorpref_say;     /* Say color preference */
	char colorpref_osay;    /* Osay color preference */
	
	ush_int page_length;

	struct travel_data *travels;
  char *rplog[RPLOG_SIZE]; /* array of ten pointers for rplog */
  char *tells[TELLS_SIZE]; /* array of ten pointers for rplog */

	char *rpdescription[5]; /* array of five pointers */
	ush_int activedesc;
	char *away_message;
	char *doing;
	char *contactinfo;
	char *replyto;
	char *keywords;

	long recognized[MAX_RECOGNIZED];
	long notifylist[MAX_NOTIFY];

	struct event *player_events[3];

	unsigned int questpoints;

	ubyte guild;

	int last_crafting;

	ush_int creation_points;

	bool verified;
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */
struct player_special_data {
	struct player_special_data_saved saved;
	
	struct alias_data *aliases;						/* Character's aliases      */
	long last_tell;												/* idnum of last tell from    */
	void *last_olc_targ;									/* olc control        */
	int last_olc_mode;										/* olc control        */
	char *host;														/* player host        */
	struct wprof_data profs[MAX_PROFS];		/* Character's Proficencies */
};


/* Specials used by NPCs, not PCs */
struct mob_special_data {
	byte last_direction;	/* The last direction the monster went	*/
	int  attack_type;			/* The Attack Type Bitvector for NPC's	*/
	byte default_pos;			/* Default position for NPC							*/
	memory_rec *memory;		/* List of attackers to remember				*/
	byte damnodice;				/* The number of damage dice's					*/
	byte damsizedice;			/* The size of the damage dice's				*/
	ubyte difficulty;			/* Difficulty of the mob								*/
	byte size;						/* Body frame size of the mob						*/
	int  value[4];				/* Values of the mob (see list)					*/
};


/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type {
	sh_int type;          /* The type of spell that caused this      */
	sh_int duration;      /* For how long its effects will last      */
	sbyte modifier;       /* This is added to apropriate ability     */
	byte location;        /* Tells which ability to change(APPLY_XXX)*/
	bitvector_t  bitvector; /* Tells which bits to set (AFF_XXX) */

	struct affected_type *next;
};


/* Structure used for chars following other chars */
struct follow_type {
	struct char_data *follower;
	struct follow_type *next;
};

/* PC Immortal travels */
struct travel_data {
	char *tin;       /* teleport in */
	char *tout;      /* teleport out */
	char *min;       /* move in */
	char *mout;      /* move out */
	char *gin;       /* game in */
	char *gout;      /* game out */
	char *iin;       /* invis in */
	char *iout;      /* invis out */
};


/* ================== Structure for player/non-player ===================== */
struct char_data {
	int pfilepos;       /* playerfile pos      */
	mob_rnum nr;                          /* Mob's rnum                    */
	room_rnum in_room;                    /* Location (real room number)    */
	room_rnum was_in_room;     /* location for linkdead people  */
	int wait;         /* wait for how many loops    */

	struct char_player_data player;       /* Normal data                   */
	struct char_ability_data real_abils;   /* Abilities without modifiers   */
	struct char_ability_data aff_abils;   /* Abils with spells/stones/etc  */
	struct char_point_data points;        /* Points                        */
	struct char_body_parts bodyparts;        /* Body parts Points                        */
	struct char_special_data char_specials;  /* PC/NPC specials    */
	struct player_special_data *player_specials; /* PC specials      */
	struct mob_special_data mob_specials;  /* NPC specials      */

	struct affected_type *affected;       /* affected by what spells       */
	struct obj_data *equipment[NUM_WEARS];/* Equipment array               */

	struct obj_data *carrying;            /* Head of list                  */
	struct descriptor_data *desc;         /* NULL for mobiles              */

	long id;                            /* used by DG triggers             */
	struct trig_proto_list *proto_script; /* list of default triggers      */
	struct script_data *script;         /* script info for the object      */
	struct script_memory *memory;       /* for mob memory triggers         */

	struct char_data *next_in_room;     /* For room->people - list         */
	struct char_data *next;             /* For either monster or ppl-list  */
	struct char_data *next_fighting;    /* For fighting list               */
	
	sh_int *completed_quests;                /* Quests completed               */
	int    num_completed_quests;             /* Number completed               */
	int    current_quest;                    /* vnum of current quest          */

	struct follow_type *followers;        /* List of chars followers       */
	struct char_data *master;             /* Who is char following?        */
};
/* ====================================================================== */


/* descriptor-related structures ******************************************/

struct txt_block {
	char  *text;
	int aliased;
	struct txt_block *next;
};


struct txt_q {
	struct txt_block *head;
	struct txt_block *tail;
};


struct descriptor_data {
	socket_t  descriptor;  /* file descriptor for socket    */
	char  host[HOST_LENGTH+1];  /* hostname        */
	byte  bad_pws;    /* number of bad pw attemps this login  */
	byte idle_tics;    /* tics idle at password prompt    */
	int  connected;    /* mode of 'connectedness'    */
	int  desc_num;    /* unique num assigned to desc    */
	time_t login_time;    /* when the person connected    */
	char *showstr_head;    /* for keeping track of an internal str  */
	char **showstr_vector;  /* for paging through texts    */
	int  showstr_count;    /* number of pages to page through  */
	int  showstr_page;    /* which page are we currently showing?  */
	struct text_edit_data *textedit; /* data used by text editor */
	int  has_prompt;    /* is the user at a prompt?             */
	char  inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input    */
	char  last_input[MAX_INPUT_LENGTH]; /* the last input      */
	char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer    */
	char *output;    /* ptr to the current output buffer  */
	char **history;    /* History of commands, for ! mostly.  */
	int  history_pos;    /* Circular array position.    */
	int  bufptr;      /* ptr to end of current output    */
	int  bufspace;    /* space left in the output buffer  */
	buffer *large_outbuf;	/* ptr to large buffer, if we need it	*/
	struct txt_q input;    /* q of unprocessed input    */
	struct char_data *character;  /* linked to char      */
	struct char_data *original;  /* original char if switched    */
	struct descriptor_data *snooping; /* Who is this char snooping  */
	struct descriptor_data *snoop_by; /* And who is snooping this char  */
	struct descriptor_data *next; /* link to next descriptor    */
	void *olc;
};


/* other miscellaneous structures ***************************************/

/* Data allocated only when editing text */
struct text_edit_data {
	char **str;							/* Final destination of string being modified */
	char *smod;							/* string being modified */
	size_t max_str;					/* Maximum length of string */
	long  mail_to;					/* name for mail system       */
	int mode;								/* Editor Mode */
};

/* command structure */
struct command_info {
	char *command;					/* holds a copy of activating command */
	char *sort_as;					/* holds a copy of similar command or abbrev
													 * to sort by for the parser */
	byte minimum_position;	/* minimum position ch must be in to perform */
	int command_num;				/* number of ACMD command is attached to in command_list */
	bitvector_t rights;			/* rights needed for command */
	int subcmd;							/* subcommand of command */
	byte copyover;					/* if command can be performed during a copyover */
	byte enabled;						/* if command is enabled or not */
	byte reserved;					/* if command is reserved or not */
};

/* master command structure  - this should mirror command_info except command_num should be the void */
struct master_command_info {
	char *command;
	char *sort_as;
	byte minimum_position;
	void (*command_pointer) (struct char_data *ch, char *argument, int cmd, int subcmd);
	bitvector_t rights;
	int subcmd;
	byte copyover;
	byte enabled;
	byte reserved;
};

/* used in the socials */
struct social_messg {
	int id;
	int act_nr;
	char *command;            /* holds copy of activating command */
	char *sort_as;            /* holds a copy of a similar command or
														* abbreviation to sort by for the parser */
	int hide;                 /* hide if invisible */
	int min_victim_position;  /* Position of victim */
	int min_char_position;    /* Position of char */

	/* No argument was supplied */
	char *char_no_arg;
	char *others_no_arg;

	/* An argument was there, and a victim was found */
	char *char_found;
	char *others_found;
	char *vict_found;

	/* An argument was there, as well as a body part, and a victim was found */
	char *char_body_found;
	char *others_body_found;
	char *vict_body_found;

	/* An argument was there, but no victim was found */
	char *not_found;

	/* The victim turned out to be the character */
	char *char_auto;
	char *others_auto;

	/* If the char cant be found search the char's inven and do these: */
	char *char_obj_found;
	char *others_obj_found;
};


struct help_index_element {
	int id;
	char *keywords;
	char *entry;
};


struct msg_type {
	char  *attacker_msg;  /* message to attacker */
	char  *victim_msg;    /* message to victim   */
	char  *room_msg;      /* message to room     */
};


struct message_type {
	struct msg_type die_msg;  /* messages when death      */
	struct msg_type miss_msg;  /* messages when miss      */
	struct msg_type hit_msg;  /* messages when hit      */
	struct msg_type god_msg;  /* messages when hit on god    */
	struct message_type *next;  /* to next messages of this kind.  */
};


struct message_list {
	int  a_type;      /* Attack type        */
	int  number_of_attacks;  /* How many attack messages to chose from. */
	struct message_type *msg;  /* List of messages.      */
};


struct dex_skill_type {
	sh_int p_pocket;
	sh_int p_locks;
	sh_int traps;
	sh_int sneak;
	sh_int hide;
};


struct dex_app_type {
	sh_int reaction;
	sh_int miss_att;
	sh_int defensive;
};


struct str_app_type {
	sh_int tohit;    /* To Hit (THAC0) Bonus/Penalty        */
	sh_int todam;    /* Damage Bonus/Penalty                */
	sh_int carry_w;  /* Maximum weight that can be carrried */
	sh_int wield_w;  /* Maximum weight that can be wielded  */
};


struct wis_app_type {
	byte bonus;       /* how many practices player gains per lev */
};


struct int_app_type {
	byte learn;       /* how many % a player learns a spell/skill */
};


struct con_app_type {
	sh_int hitp;
	sh_int shock;
};


struct save_modifier_type {
	byte paralyze;
	byte rod;
	byte petrify;
	byte breath;
	byte spell;
};


struct weather_data {
	int  pressure;  /* How is the pressure ( Mb ) */
	int  change;  /* How fast and what way does it change. */
	int  sky;  /* How is the sky. */
	int  sunlight;  /* And how much sun. */
};


/* element in monster and object index-tables   */
struct index_data {
	sh_int  vnum;  /* virtual number of this mob/obj    */
	int  number;    /* number of existing units of this mob/obj  */
	int specproc;
	SPECIAL(*func);
	
	char *farg;         /* string argument for special function     */
	struct trig_data *proto;     /* for triggers... the trigger     */
};


/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
	int vnum;                             /* vnum of the trigger   */
	struct trig_proto_list *next;         /* next trigger          */
};


struct weapon_prof_data {
	sh_int to_hit;
	sh_int to_ac;
	sh_int add_dam;
};


struct race_list_element {
	int num;
	char *abbr;
	char *name;
	bool magic;
	bool selectable;
	ush_int cost;
	sh_int plane;
	sh_int realm;
	char *description;
	sh_int constant;
	sh_int size;
	ush_int min_attrib;
	ush_int max_attrib;
};


struct culture_list_element {
	int num;
	char *abbr;
	char *name;
	bool selectable;
	ush_int cost;
	char *description;
	sh_int constant;
};


struct log_rights {
	char *cmd;
	bitvector_t rights;
};


/* event object structure for delays in player events */
struct delay_event_obj {
	struct char_data *vict;	/* victim character						*/
	byte type;							/* EVENT_x										*/
	int time;								/* number of pulses to queue	*/
};
