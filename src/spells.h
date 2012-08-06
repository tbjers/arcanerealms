/* ************************************************************************
*		File: spells.h                                      Part of CircleMUD *
*	 Usage: header file: constants and fn prototypes for spell system       *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: spells.h,v 1.14 2002/12/21 12:08:27 arcanere Exp $ */

#define	DEFAULT_STAFF_LVL							35
#define	DEFAULT_WAND_LVL							35

#define	CAST_UNDEFINED								(-1)
#define	CAST_SPELL										0
#define	CAST_POTION										1
#define	CAST_WAND											2
#define	CAST_STAFF										3
#define	CAST_SCROLL										4

/* Magic Routines */
#define	MAG_DAMAGE										(1 << 0)
#define	MAG_AFFECTS										(1 << 1)
#define	MAG_UNAFFECTS									(1 << 2)
#define	MAG_POINTS										(1 << 3)
#define	MAG_ALTER_OBJS								(1 << 4)
#define	MAG_GROUPS										(1 << 5)
#define	MAG_MASSES										(1 << 6)
#define	MAG_AREAS											(1 << 7)
#define	MAG_SUMMONS										(1 << 8)
#define	MAG_CREATIONS									(1 << 9)
#define	MAG_MANUAL										(1 << 10)

#define NUM_ROUTINES									11

#define	TYPE_UNDEFINED								(-1)
#define	SPELL_RESERVED_DBC						0  /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

#define	SPELL_ARMOR										1		/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_TELEPORT								2		/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_BLESS										3		/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_BLINDNESS								4		/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_BURNING_HANDS						5		/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_CALL_LIGHTNING					6		/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_CHARM										7		/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_CHILL_TOUCH							8		/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_CLONE										9		/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_COLOR_SPRAY							10	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_CONTROL_WEATHER					11	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_CREATE_FOOD							12	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_CREATE_WATER						13	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_CURE_BLIND							14	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_CURE_CRITIC							15	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_CURE_LIGHT							16	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_CURSE										17	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_DETECT_ALIGN						18	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_DETECT_INVIS						19	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_DETECT_MAGIC						20	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_DETECT_POISON						21	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_DISPEL_EVIL							22	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_EARTHQUAKE							23	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_ENCHANT_WEAPON					24	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_ENERGY_DRAIN						25	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_FIREBALL								26	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_HARM										27	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_HEAL										28	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_INVISIBLE								29	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_LIGHTNING_BOLT					30	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_LOCATE_OBJECT						31	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_MAGIC_MISSILE						32	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_POISON									33	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_PROT_FROM_EVIL					34	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_REMOVE_CURSE						35	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_SANCTUARY								36	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_SHOCKING_GRASP					37	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_SLEEP										38	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_STRENGTH								39	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_SUMMON									40	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_VENTRILOQUATE						41	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_WORD_OF_RECALL					42	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_REMOVE_POISON						43	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_SENSE_LIFE							44	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_ANIMATE_DEAD						45	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_DISPEL_GOOD							46	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_GROUP_ARMOR							47	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_GROUP_HEAL							48	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_GROUP_RECALL						49	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_INFRAVISION							50	/* Reserved Skill[] DO NOT CHANGE */
#define	SPELL_WATERWALK								51	/* Reserved Skill[] DO NOT CHANGE */

/* Insert new spells here, up to MAX_SPELLS */
#define	SPELL_MINOR_IDENTIFY					52
#define	SPELL_PORTAL									53
#define	SPELL_BARKSKIN								54
#define	SPELL_STONESKIN								55
#define	SPELL_STEELSKIN								56
#define	SPELL_GALEFORCE								57	/* Paladin spell, multi-mob attack     */
#define	SPELL_SHROUD									58	/* Paladin spell, higher AC for mo     */
#define	SPELL_ORB											59	/* Paladin spell, weaker sanctuary     */
#define	SPELL_DIVINE_ARROW						60	/* Paladin spell, equiv. Magic Missile */

/* NEW MAGI CLASSES SPELL ADDITIONS, ARTOVIL/ELDOIN */
#define	SPELL_BELLADONNA							61
#define	SPELL_BLUR										62
#define	SPELL_CALM										63
#define	SPELL_COMBAT_MIND							64
#define	SPELL_CREATE_MACE							65
#define	SPELL_CREATE_POULTICE					66
#define	SPELL_CURE_PLAGUE							67
#define	SPELL_CURE_POISON							68
#define	SPELL_DEATHPOD								69
#define	SPELL_DISPERSE								70
#define	SPELL_ENHANCED_DAMAGE					71
#define	SPELL_ENLARGE									72
#define	SPELL_FEAR										73
#define	SPELL_FIRE_DAEMON							74	/* done */
#define	SPELL_FIRE_IMP								75	/* done */
#define	SPELL_FIRE_SHIELD							76
#define	SPELL_FIRE_WALK								77
#define	SPELL_FLAME_FORGE							78
#define	SPELL_GATE										79
#define	SPELL_GIANT_STRENGTH					80
#define	SPELL_HARROW									81
#define	SPELL_HASTE										82	/* done */
#define	SPELL_HEALTH_TRANSFER					83
#define	SPELL_ILLUSION_OF_GRANDUER		84
#define	SPELL_IMPALE									85
#define	SPELL_IRON_GRIP								86
#define	SPELL_MANA_TRANSFER						87
#define	SPELL_MIND_BLAST							88
#define	SPELL_MUD_SLIDE								89
#define	SPELL_PLANT_FURY							90
#define	SPELL_SHAPECHANGE							91
#define	SPELL_SHAPESHIFT							92
#define	SPELL_SHOCK										93
#define	SPELL_SHRINK									94
#define	SPELL_SLOTH										95
#define	SPELL_SNOOP										96
#define	SPELL_STONE_STRIKE						97
#define	SPELL_SUMMON_PHANTASM					98
#define	SPELL_TRANSFIXATION						99
#define	SPELL_UNWILLING_GIFT					100
#define	SPELL_WALL_OF_FIRE						101
#define	SPELL_VINE_OF_SLEEP						102
#define	SPELL_VINE_TRAP								103

#define	SPELL_ARCANE_WORD							104	/* done */
#define	SPELL_ARCANE_PORTAL						105	/* done */

#define	NUM_SPELLS										105

#define	MAX_SPELLS										200

/*
 *  NON-PLAYER AND OBJECT SPELLS
 *  The practice levels for the spells below are _not_ recorded
 *  in the playerfile; therefore, the intended use is for spells and skills
 *  associated with objects (such as SPELL_IDENTIFY used with scrolls of
 *  identify) or non-players (such as NPC-only spells).
 */

#define	START_NON_PLAYER_SPELLS				201

#define	SPELL_IDENTIFY								201
#define	SPELL_FIRE_BREATH							202
#define	SPELL_GAS_BREATH							203
#define	SPELL_FROST_BREATH						204
#define	SPELL_ACID_BREATH							205
#define	SPELL_LIGHTNING_BREATH				206

#define	NUM_NON_PLAYER_SPELLS					6

#define	TOP_SPELL_DEFINE							299
/* NEW NPC/OBJECT SPELLS can be inserted here up to 299 */

#define	TOP_SPELLS                    ((TOP_SPELL_DEFINE)+1)

#define	SAVING_PARA     0
#define	SAVING_ROD      1
#define	SAVING_PETRI    2
#define	SAVING_BREATH   3
#define	SAVING_SPELL    4

/* TAR_x Target flags */
#define	TAR_IGNORE      (1 << 0)
#define	TAR_CHAR_ROOM   (1 << 1)
#define	TAR_CHAR_WORLD  (1 << 2)
#define	TAR_FIGHT_SELF  (1 << 3)
#define	TAR_FIGHT_VICT  (1 << 4)
#define	TAR_SELF_ONLY   (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define	TAR_NOT_SELF   	(1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define	TAR_OBJ_INV     (1 << 7)
#define	TAR_OBJ_ROOM    (1 << 8)
#define	TAR_OBJ_WORLD   (1 << 9)
#define	TAR_OBJ_EQUIP   (1 << 10)

#define NUM_TARGETS			11

/* M_TECH_x Magic Techniques */
#define M_TECH_ERROR						(-1)
#define M_TECH_CREATE						0
#define M_TECH_PERCEIVE					1
#define M_TECH_TRANSFORM				2
#define M_TECH_DESTROY					3
#define M_TECH_CONTROL					4

#define NUM_SPELL_TECHNIQUES		5

/* M_FORM_x Magic Forms */
#define M_FORM_ERROR						(-1)
#define M_FORM_ANIMAL						0
#define M_FORM_WATER						1
#define M_FORM_AIR							2
#define M_FORM_BODY							3
#define M_FORM_PLANT						4
#define M_FORM_FIRE							5
#define M_FORM_IMAGE						6
#define M_FORM_MIND							7
#define M_FORM_EARTH						8
#define M_FORM_POWER						9

#define NUM_SPELL_FORMS					10

/* M_RANGE_x Spell Ranges */
#define M_RANGE_ERROR						(-1)
#define M_RANGE_BODY						0
#define M_RANGE_SELF						1
#define M_RANGE_TOUCH						2
#define M_RANGE_REACH						3
#define M_RANGE_EYE							4
#define M_RANGE_NEAR						5
#define M_RANGE_SIGHT						6

#define NUM_SPELL_RANGES				7

/* M_DUR_x Spell Durations */
#define M_DUR_ERROR							(-1)
#define M_DUR_INSTANT						0
#define M_DUR_ATTENTION					1
#define M_DUR_RING							2
#define M_DUR_SUN								3
#define M_DUR_MOON							4
#define M_DUR_YEAR							5
#define M_DUR_PERMANENT					6

#define NUM_SPELL_DURATIONS			7

/* M_TAR_x Spell Target Groups */
#define M_TAR_ERROR							(-1)
#define M_TAR_SELF							0
#define M_TAR_SOMEONE						1
#define M_TAR_GATHERING					2
#define M_TAR_LOCATION					3
#define M_TAR_WORLD							4

#define NUM_SPELL_TARGET_GROUPS	5

/* M_SEED_x seed types for spells */
#define M_SEED_ERROR						(-1)
#define M_SEED_POINTS						0
#define M_SEED_RACE							1
#define M_SEED_ATTRIBUTES				2
#define M_SEED_GENDER						3
#define M_SEED_ALIGNMENT				4
#define M_SEED_TRUENAME					5
#define M_SEED_CREATURE					6
#define M_SEED_OBJECT						7
#define M_SEED_LOCATION					8

#define NUM_SPELL_SEED_TYPES		9

/* Spell editor object limits */
#define MAX_SPELL_OBJ						10
#define MAX_REAGENTS						5
#define MAX_FOCUSES							3
#define MAX_TRANSMUTERS					2

/* Types of lists to read */
#define LIST_REAGENTS						0
#define LIST_FOCUSES						1
#define LIST_TRANSMUTERS				2

#define SPELL_REAGENT(sptr, num)		((sptr)->reagents[(num)])
#define SPELL_FOCUS(sptr, num)			((sptr)->focuses[(num)])
#define SPELL_TRANSMUTER(sptr, num)	((sptr)->transmuters[(num)])

#define SPELL_NUM(sptr)							((sptr)->number)

struct spell_obj_data {
	int type;
	ubyte location;
	ubyte extract;
};


/*
 * This structure defines a spell.  It is similar to an object,
 * in that it has generic variables which mean different things in 
 * different situations. In this case, the situation is
 * different kinds of spells
 */
struct spell_val_data {
	int mag_type;
	int val0; 
	int val1;
	int val2;
	char *expr0;
	char *expr1;
	bitvector_t bitv0;
	bitvector_t bitv1;
	struct spell_val_data *next;
};


struct wild_spell_info {
	int tech; /* Technique */
	int form; /* Form */
	int range; /* Range */
	int duration; /* Duration */
	int target_group; /* Target Group */
 	int seed; /* Seed type */
};

#define SPELL_VAL_MAG_TYPE(i)		((i).mag_type)
#define SPELL_VAL_INT_0(i)			((i).val0)
#define SPELL_VAL_INT_1(i)			((i).val1)
#define SPELL_VAL_INT_2(i)			((i).val2)
#define SPELL_VAL_EXPR_0(i)			((i).expr0)
#define SPELL_VAL_EXPR_1(i)			((i).expr1)
#define SPELL_VAL_BITV_0(i)			((i).bitv0)
#define SPELL_VAL_BITV_1(i)			((i).bitv1)

#define SPELL_OBJ_TYPE(i)			((i).type)
#define SPELL_OBJ_LOCATION(i)	((i).location)
#define SPELL_OBJ_EXTRACT(i)	((i).extract)

struct spell_info_type {
	sh_int number;
	int skill;
	byte min_position;   /* Position for caster	 */
	int mana_min;        /* Min amount of mana used by a spell (highest lev) */
	int mana_max;        /* Max amount of mana used by a spell (lowest lev) */
	int technique;
	int form;
	int range;
	int duration;
	int target_group;
	int routines;
	byte violent;
	int targets;         /* See below for use with TAR_XXX  */
	char *name;
	char *long_name;
	char *cast_msg;
	char *mundane_msg;
	char *target_msg;
	char *victim_msg;
	char *wear_off_msg;
	ubyte learned;
	struct spell_obj_data *reagents;
	struct spell_obj_data *focuses;
	struct spell_obj_data *transmuters;
	struct spell_val_data *values;
	struct spell_info_type *next;
};

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

#define	SPELL_TYPE_SPELL    0
#define	SPELL_TYPE_POTION   1
#define	SPELL_TYPE_WAND     2
#define	SPELL_TYPE_STAFF    3
#define	SPELL_TYPE_SCROLL   4


/* Attacktypes with grammar */

struct attack_hit_type {
	 const char	*singular;
	 const char	*plural;
};


#define	ASPELL(spellname) \
	void	spellname(int level, struct char_data *ch, \
				struct char_data *victim, struct obj_data *obj, \
				char *tar_str)

#define	MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict, tar_str);

ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_detect_poison);
ASPELL(spell_minor_identify);
ASPELL(spell_portal);
ASPELL(spell_arcane_word);
ASPELL(spell_arcane_portal);


/* basic magic calling functions */

char *spell_name(int num);
int	find_spell_num(char *name);

int	mag_damage(int level, struct char_data *ch, struct char_data *victim,
	struct spell_info_type *sptr, int savetype);

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
	struct spell_info_type *sptr, int savetype);

void mag_groups(int level, struct char_data *ch, struct spell_info_type *sptr,
	int savetype);

void mag_masses(int level, struct char_data *ch, struct spell_info_type *sptr,
	int savetype);

void mag_areas(int level, struct char_data *ch, struct spell_info_type *sptr,
	int savetype);

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
 struct spell_info_type *sptr, int savetype);

void mag_points(int level, struct char_data *ch, struct char_data *victim,
 struct spell_info_type *sptr, int savetype);

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
	struct spell_info_type *sptr, int type);

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
	struct spell_info_type *sptr, int type);

void mag_creations(int level, struct char_data *ch, struct spell_info_type *sptr);

int	call_magic(struct char_data *caster, struct char_data *cvict,
			 struct obj_data *ovict, struct spell_info_type *sptr, int level,
			 int casttype, char *tar_str);

void	mag_objectmagic(struct char_data *ch, struct obj_data *obj,
			char *argument);

int	cast_spell(struct char_data *ch, struct char_data *tch,
	struct obj_data *tobj, struct spell_info_type *sptr, char *tar_str);

void class_spells_index(int chclass, char *str);

struct spell_info_type *get_spell(int spellnum, const char *file, const char *function);

void parse_wild_spell(const char * input_string, struct wild_spell_info *info);
