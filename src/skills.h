/* ************************************************************************
 * File: skills.h                                       Part of CircleMUD *
 * Usage: Header file with defines for the AR skill system.               *
 *                                                                        *
 * Copyright (C) 2001, Arcane Realms                                      *
 *********************************************************************** */
/* $Id: skills.h,v 1.40 2004/04/23 15:18:10 cheron Exp $ */

#define	SKILL(name)	\
	 void name(struct char_data *ch, struct char_data *vict, struct obj_data *obj, char *argument, bool success, int subcmd)

#define SSTATE_DECREASE					(-1)
#define SSTATE_LOCK							0
#define SSTATE_INCREASE					1

#define SKILL_STATES						3

/*
 * ATYPE_x defines:
 * For use with skill selection during creation.
 */
#define ATYPE_COMMON						FALSE
#define ATYPE_EXCLUSIVE					TRUE

#define	TOP_SKILL_DEFINE				299

#define GROUP_COMMUNICATION			1
#define GROUP_ANIMAL						2
#define GROUP_ATHLETIC					3
#define GROUP_SOCIAL						4
#define GROUP_OBSERVATION				5
#define GROUP_COMBAT						6
#define GROUP_HUNTING						7
#define GROUP_ART								8
#define GROUP_MUSIC							9
#define GROUP_FABRICS						10
#define GROUP_CULINARY					11
#define GROUP_MEDICAL						12
#define GROUP_MACHINEMAKING			13
#define GROUP_STONEWORKING			14
#define GROUP_WOODWORKING				15
#define GROUP_METALWORKING			16
#define GROUP_LORE							17
#define GROUP_MAGERY						18
#define GROUP_DIVINATION				19
#define GROUP_ALCHEMY						20
#define GROUP_SCHOLASTIC				21
#define GROUP_SECRECY						22
#define GROUP_THIEVERY					23

#define NUM_GROUPS							23

/* PLAYER SKILLS - Numbered from NUM_GROUPS + 1 to MAX_SKILLS */
#define	SKILL_BACKSTAB					24	/* 1 */
#define	SKILL_BASH							25
#define	SKILL_HIDE							26
#define	SKILL_KICK							27
#define	SKILL_PICK_LOCK					28	/* 5 */
#define	SKILL_PUNCH							29
#define	SKILL_RESCUE						30
#define	SKILL_SNEAK							31
#define	SKILL_STEAL							32
#define	SKILL_TRACK							33	/* 10 */
#define	SKILL_SCAN							34
#define	SKILL_DUAL							35
#define	SKILL_DISARM						36
#define	SKILL_PARRY							37
#define	SKILL_RETREAT						38	/* 15 */
#define	SKILL_BREW							39
#define	SKILL_SCRIBE						40
#define	SKILL_FISHING						41
#define	SKILL_HUNTING						42
#define	SKILL_LUMBERJACK				43	/* 20 */
#define	SKILL_BANDAGE						44
#define SKILL_WRITING						45
#define SKILL_FORGERY						46
#define SKILL_AMINAL_TAMING			47
#define SKILL_ANIMAL_TRAINING		48	/* 25 */
#define SKILL_RIDING_LAND				49
#define SKILL_RIDING_AIR				50
#define SKILL_DANCING						51
#define SKILL_SWIMMING					52
#define SKILL_GAMBLING					53	/* 30 */
#define SKILL_TEACHING					54
#define SKILL_BEGGING						55
#define SKILL_DETECTION					56
#define SKILL_FIND_HIDDEN				57
#define SKILL_DETECT_TRAP				58	/* 35 */
#define SKILL_MELEE							59
#define SKILL_AXES							60
#define SKILL_SWORDS						61
#define SKILL_DAGGERS						62
#define SKILL_MACES							63	/* 40 */
#define SKILL_BOWS							64
#define SKILL_POLE_ARMS					65
#define SKILL_THROWN						66
#define SKILL_STRATEGY					67
#define SKILL_TACTICS						68	/* 45 */
#define SKILL_PAINTING					69
#define SKILL_POTTERY						70
#define SKILL_INSTRUMENTS				71
#define SKILL_ENTICEMENT				72
#define SKILL_PEACEMAKING				73	/* 50 */
#define SKILL_LEATHERWORKING		74
#define SKILL_TAILORING					75
#define SKILL_WEAVING						76
#define SKILL_COOKING						77
#define SKILL_BUTCHERING				78	/* 55 */
#define SKILL_BREWING						79
#define SKILL_POISON_FOOD				80
#define SKILL_MEDICAL_AID				81
#define SKILL_SURGERY						82
#define SKILL_NURSING						83	/* 60 */
#define SKILL_LOCK_SMITHERY			84
#define SKILL_TRAP_MAKING				85
#define SKILL_CLOCK_SMITHERY		86
#define SKILL_GEM_CUTTING				87
#define SKILL_SCULPTING					88	/* 65 */
#define SKILL_MINING						89
#define SKILL_CARPENTRY					90
#define SKILL_BOWYER						91
#define SKILL_WOODCARVING				92
#define SKILL_BOTANY						93	/* 70 */
#define SKILL_GEOGRAPHY					94
#define SKILL_ZOOLOGY						95
#define SKILL_REALM_LORE				96
#define SKILL_ARCANE_LORE				97
#define SKILL_OCCULT						98	/* 75 */
#define SKILL_THEOLOGY					99
#define SKILL_ANTHROPOLOGY			100
#define SKILL_LINGUISTICS				101
#define SKILL_STREETWISE				102
#define SKILL_SURVIVAL					103	/* 80 */
#define SKILL_RESIST_MAGIC			104
#define SKILL_DETECT_AURA				105
#define SKILL_DIRECT_SPELLS			106
#define SKILL_HUNG_SPELLS				107
#define SKILL_INVOKED_MSPELLS		108	/* 85 */
#define SKILL_ASTROLOGY					109
#define SKILL_TAROT							110
#define SKILL_INVOKED_SSPELLS		111
#define SKILL_PRAYER						112
#define SKILL_STEALTH						113	/* 90 */
#define SKILL_DISGUISE					114
#define SKILL_SHADOWING					115
#define SKILL_DODGE							116
#define SKILL_DEFEND						117
#define SKILL_WATCH							118	/* 95 */
#define SKILL_MISSILES					119
#define SKILL_MEDITATE					120
#define SKILL_BAKING						121
#define SKILL_WEAPONSMITHING		122
#define SKILL_BLADEFORGING			123	/* 100 */
#define SKILL_ARMORSMITHING			124
#define SKILL_JEWELRYSMITHING		125
#define SKILL_SMELTING					126
#define SKILL_HERBALISM					127
#define SKILL_GLASSBLOWING			128	/* 105 */
#define SKILL_GATHER						129
#define SKILL_CHASE							130
#define SKILL_DYEING						131
#define SKILL_DYE_MAKING				132

#define	MIN_LANGUAGES						190
#define	MAX_LANGUAGES						196

/* LANGUAGES - Numbered from 190 to 196 */
#define	LANG_COMMON							190	/* 110 */
#define	LANG_ELVEN							191
#define	LANG_GNOMISH						192
#define	LANG_DWARVEN						193
#define	LANG_ANCIENT						194
#define	LANG_THIEF							195	/* 115 */
#define LANG_SIGN								196

#define	NUM_SKILLS							116


/* WEAPON ATTACK TYPES */
#define	TYPE_HIT								300
#define	TYPE_STING							301
#define	TYPE_WHIP								302
#define	TYPE_SLASH							303
#define	TYPE_BITE								304
#define	TYPE_BLUDGEON						305
#define	TYPE_CRUSH							306
#define	TYPE_POUND							307
#define	TYPE_CLAW								308
#define	TYPE_MAUL								309
#define	TYPE_THRASH							310
#define	TYPE_PIERCE							311
#define	TYPE_BLAST							312
#define	TYPE_PUNCH							313
#define	TYPE_STAB								314

#define	NUM_ATTACK_TYPES				15

/* new attack types can be added here - up to TYPE_SUFFERING */
#define	TYPE_SUFFERING					399


extern const char *skill_groups[NUM_GROUPS+1];
const	char *skill_name(int num);
int	find_skill_num(char *name);
int can_prac_skill(struct char_data *ch, int skill);
int find_skill(int skill);
int	backstab_mult(struct char_data *ch);
bool skill_check(struct char_data *ch, int skl, int modifier);
int skill_gain(struct char_data *ch, int skillno, int chance);
int get_attrib(struct char_data *ch, int attrib);
int get_real_attrib(struct char_data *ch, int attrib);
int set_skill_base(struct char_data *ch, int skl);
void assign_skill_bases(struct char_data *ch);
int skill_cap(struct char_data *ch, int gain, bool show_bonus);
bool decrease_skills(struct char_data *ch, int gain);
void list_skill_titles(struct char_data *ch, bool show_title);
void update_groups(struct char_data *ch);

struct skill_info_type skill_info[TOP_SKILL_DEFINE + 1];
struct skill_data skill_tree[TOP_SKILL_DEFINE+1];

struct skill_data {
	sh_int skill;				/* num of skill name															*/
	sh_int pre;					/* num of pre-requisite skill											*/
	bool selectable;		/* skill selectable at start?											*/
	ubyte cost;					/* cost, in skill points, to raise 1% (100/10000)	*/
	long percent;				/* percent of skill learned (0-10000)							*/
	ubyte level;				/* skill level (not level of char)								*/
	sh_int attrib;			/* skill attribute																*/
	ubyte time;					/* execution time (x ACTION_PULSE)								*/
	bool vict_delay;		/* delay victim while skill executes?							*/
	char *message;			/* message to send out each interval, if any			*/
};

struct skill_info_type {
	byte min_position;   /* Minimum position for skill */
	int group;
	byte violent;
	int targets;         /* See spells.h for use with TAR_XXX  */
	const char *name;
	ubyte learned;
};

/* event object structure for timed skills */
struct skill_event_obj {
	struct char_data *ch;		/* character that uses the skill			*/
	struct char_data *vict;	/* victim character										*/
	struct obj_data *obj;		/* object used in skill								*/
	bool success;						/* success/failure of skill roll			*/
	byte type;							/* ACTION_SKILL | ACTION_SPELL				*/
	int time;								/* number of pulses to queue					*/
	ush_int skill;					/* skill to process										*/
	bool vict_delay;				/* delay victim while skill executes?	*/
	char *argument;
	int subcmd;							/* sub command to pass to skill				*/
};

struct state_struct {
	int state;
	char symbol;
	const char *name;
	const char *color;
};

EVENTFUNC(skill_event);
EVENTFUNC(vict_delay);
