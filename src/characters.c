/* *************************************************************************
*  File: character.c                              Extension to CircleMUD   *
*  Usage: Source file for character-specific code                          *
*                                                                          *
*  This file made exclusively for Arcane Realms MUD by Torgny Bjers.       *
*  Copyright (C) 2001, Torgny Bjers, and Catherine Gore.  Formulas made by *
*  Jason Yarber, Copyright (C) 2001, Jason Yarber.                         *
*                                                                          *
*	 All rights reserved.  See license.doc for complete information.         *
*                                                                          *
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University  *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
************************************************************************** */
/* $Id: characters.c,v 1.27 2004/04/16 16:15:03 cheron Exp $ */

#define	__CHARACTERS_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "db.h"
#include "utils.h"
#include "events.h"
#include "interpreter.h" 
#include "constants.h"
#include "screen.h"
#include "comm.h"
#include "handler.h"
#include "pfdefaults.h"
#include "loadrooms.h"
#include "characters.h"
#include "genolc.h"
#include "oasis.h"
#include "spells.h"
#include "skills.h"

/* Global defines */
int num_select_eyes = 6;		/* number of selectable eyecolors */
int num_select_hair = 8;		/* number of selectable haircolors */

/* External functions */
void obj_to_char(struct obj_data *object, struct char_data *ch);
struct obj_data *unequip_char(struct char_data * ch, int pos);
extern int isname(const char *str, const char *namelist);
ACMD(do_help);

/* External variables */
extern int siteok_everyone;
struct race_list_element *race_list;
int	top_of_race_list;

/* Local functions */
bool attributes_menu(struct char_data *ch, char *argument);
int update_attribs(struct char_data *ch);
ACMD(do_description);


const struct race_frames_data race_frames[NUM_RACES] = {
	{	RACE_HUMAN,				FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_GRAYELF,			FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_HIGHELF,			FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_SATYR,				FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_PIXIE,				FRAME_WINGED_HUMANOID,	HITAREA_WINGED_HUMANOID	},
	{	RACE_SPRITE,			FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_NYMPH,				FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_GNOME,				FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_AASIMARI,		FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_GIANT,				FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_VAMPIRE,			FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_TIEFLING,		FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_NIXIE,				FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_LEPRECHAUN,	FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_TROLL,				FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_GOBLIN,			FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_BROWNIE,			FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_DRAGON,			FRAME_DRAGON,						HITAREA_DRAGON					},
	{	RACE_UNDEAD,			FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_ETHEREAL,		FRAME_NONE,							HITAREA_NONE						},
	{	RACE_ANIMAL,			FRAME_ANIMAL,						HITAREA_DEFAULT					},
	{	RACE_LEGENDARY,		FRAME_NONE,							HITAREA_NONE						},
	{	RACE_ELEMENTAL,		FRAME_NONE,							HITAREA_NONE						},
	{	RACE_MAGICAL,			FRAME_NONE,							HITAREA_NONE						},
	{	RACE_SERAPHI,			FRAME_WINGED_HUMANOID,	HITAREA_WINGED_HUMANOID	},
	{	RACE_ADEPHI,			FRAME_HUMANOID,					HITAREA_HUMANOID				},
	{	RACE_DEMON,				FRAME_DEMON,						HITAREA_DEMON						},
	{	RACE_DEITY,				FRAME_NONE,							HITAREA_NONE						},
	{	RACE_ENDLESS,			FRAME_NONE,							HITAREA_NONE						}
};

sh_int hitarea_none[NUM_HITAREAS_NONE] = {
	WEAR_BODY		// 1
};

sh_int hitarea_default[NUM_HITAREAS_DEFAULT] = {
	WEAR_HEAD,	// 1
	WEAR_NECK,
	WEAR_BODY,
	WEAR_ARMS,
	WEAR_LEGS		// 5
};

sh_int hitarea_humanoid[NUM_HITAREAS_HUMANOID] = {
	WEAR_HEAD,	// 1
	WEAR_NECK,
	WEAR_BODY,
	WEAR_ARMS,
	WEAR_HANDS,	// 5
	WEAR_WAIST,
	WEAR_LEGS,
	WEAR_FEET
};

sh_int hitarea_winged_humanoid[NUM_HITAREAS_WINGED_HUMANOID] = {
	WEAR_WINGS,	// 1
	WEAR_HEAD,
	WEAR_NECK,
	WEAR_BODY,
	WEAR_ARMS,	// 5
	WEAR_HANDS,
	WEAR_WAIST,
	WEAR_LEGS,
	WEAR_FEET
};

sh_int hitarea_demon[NUM_HITAREAS_DEMON] = {
	WEAR_WINGS,	// 1
	WEAR_HORNS,
	WEAR_HEAD,
	WEAR_NECK,
	WEAR_BODY,	// 5
	WEAR_ARMS,
	WEAR_HANDS,
	WEAR_TAIL,
	WEAR_WAIST,
	WEAR_LEGS,	// 10
	WEAR_FEET,
};

sh_int hitarea_dragon[NUM_HITAREAS_DRAGON] = {
	WEAR_WINGS,	// 1
	WEAR_HORNS,
	WEAR_HEAD,
	WEAR_NECK,
	WEAR_BODY,	// 5
	WEAR_ARMS,
	WEAR_HANDS,
	WEAR_TAIL,
	WEAR_WAIST,
	WEAR_LEGS,	// 10
	WEAR_FEET
};

sh_int hitarea_animal[NUM_HITAREAS_ANIMAL] = {
	WEAR_HEAD,	// 1
	WEAR_NECK,
	WEAR_BODY,
	WEAR_ARMS,
	WEAR_LEGS		// 5
};

const struct race_hitarea_data race_hitareas[] = {
	{	HITAREA_NONE,							0,																																																									hitarea_none,							NUM_HITAREAS_NONE							},
	{	HITAREA_DEFAULT,					BODY_BODY|BODY_NECK|BODY_HEAD|BODY_LEGS|BODY_ARMS,																																	hitarea_default,					NUM_HITAREAS_DEFAULT					},
	{	HITAREA_HUMANOID,					BODY_BODY|BODY_NECK|BODY_HEAD|BODY_LEGS|BODY_ARMS|BODY_HANDS|BODY_FEET|BODY_WAIST,																	hitarea_humanoid,					NUM_HITAREAS_HUMANOID					},
	{	HITAREA_WINGED_HUMANOID,	BODY_BODY|BODY_NECK|BODY_HEAD|BODY_LEGS|BODY_ARMS|BODY_HANDS|BODY_FEET|BODY_WAIST|BODY_WINGS,												hitarea_winged_humanoid,	NUM_HITAREAS_WINGED_HUMANOID	},
	{	HITAREA_DEMON,						BODY_BODY|BODY_NECK|BODY_HEAD|BODY_LEGS|BODY_ARMS|BODY_HANDS|BODY_FEET|BODY_WAIST|BODY_WINGS|BODY_HORNS|BODY_TAIL,	hitarea_demon,						NUM_HITAREAS_DEMON						},
	{	HITAREA_DRAGON,						BODY_BODY|BODY_NECK|BODY_HEAD|BODY_LEGS|BODY_ARMS|BODY_HANDS|BODY_FEET|BODY_WAIST|BODY_HORNS|BODY_TAIL,							hitarea_dragon,						NUM_HITAREAS_DRAGON						},
	{	HITAREA_ANIMAL,						BODY_BODY|BODY_NECK|BODY_HEAD|BODY_LEGS|BODY_ARMS,																																	hitarea_animal,						NUM_HITAREAS_ANIMAL						}
};

// Structure definition for constitution + strength textual returns
const char *short_character_weights[NUM_APPEARANCES][NUM_GENDERS] = {
//		SEX_NEUTRAL		SEX_MALE			SEX_FEMALE
	{	"emaciated",		"emaciated",	"emaciated"		},
	{	"thin",					"thin",				"thin"				},
	{	"slender",			"slender",		"slender"			},
	{	"brawny",				"brawny",			"voluptuous"	},
	{	"heavy-set",		"heavy-set",	"plump"				},
	{	"fat",					"fat",				"fat"					}
};

// Structure definition of character height textual returns
const char *short_character_heights[NUM_APPEARANCES][NUM_GENDERS] = {
//	SEX_NEUTRAL			SEX_MALE				SEX_FEMALE
	{	"very short",		"very short",		"very short"	},
	{	"short",				"short",				"short"				},
	{	"average",			"average",			"average"			},
	{	"tall",					"tall",					"tall"				},
	{	"huge",					"huge",					"huge"				},
	{	"giant",				"giant",				"giant"				}
};

/* character appearances: eye colors */
const char *eye_color[] =
{
	"blue",
	"brown",
	"green",
	"gray",
	"amber",
	"hazel",
	"violet",
	"black",
	"golden",
	"silver",
	"\n"
};

/* character appearances: hair style */
const char *hair_style[] =
{
	"cropped",
	"short",
	"chin length",
	"shoulder length",
	"below shoulders",
	"waist length",
	"below waist",
	"\n"
};

/* character appearances: hair colors */
const char *hair_color[] =
{
	"blonde",
	"brown",
	"auburn",
	"cinnamon",
	"red",
	"black",
	"gray",
	"white",
	"golden",
	"silver",
	"moss-green",
	"violet",
	"indigo",
	"\n"
};

/* character appearances: skin tones */
const char *skin_tone[] =
{
	"porcelain",
	"ivory",
	"peach",
	"ochre",
	"yellowed",
	"parchment",
	"golden",
	"tan",
	"olive",
	"copper",
	"tawny",
	"umber",
	"sienna",
	"taupe",
	"charcoal",
	"black",
	"\n"
};


int parse_features (char arg, int max)
{
	int value = ATTRIB_UNDEFINED;
	switch (arg) {
		case 'a': value = 0; break;
		case 'b': value = 1; break;
		case 'c': value = 2; break;
		case 'd': value = 3; break;
		case 'e': value = 4; break;
		case 'f': value = 5; break;
		case 'g': value = 6; break;
		case 'h': value = 7; break;
		case 'i': value = 8; break;
		case 'j': value = 9; break;
		case 'k': value = 10; break;
		case 'l': value = 11; break;
		case 'm': value = 12; break;
		case 'n': value = 13; break;
		case 'o': value = 14; break;
		case 'p': value = 15; break;
		case 'q': value = 16; break;
		case 'r': value = 17; break;
		case 's': value = 18; break;
		case 't': value = 19; break;
		case 'u': value = 20; break;
		case 'v': value = 21; break;
		case 'w': value = 22; break;
		case 'x': value = 23; break;
		case 'y': value = 24; break;
		case 'z': value = 25; break;
		default: value = ATTRIB_UNDEFINED; break;
	}
	if (value >= max || value == ATTRIB_UNDEFINED)
		return (ATTRIB_UNDEFINED);
	else
		return (value);
}

//	Structure definition of heights by race redesigned.
//	Values need adjusting. -- Jason Yarber
int race_heights[NUM_RACES][6] = {
//	VL			L				M				H				E				FEMALE
	{	130,		165,		185,		210,		230,		5		},	// HUM
	{	160,		180,		200,		220,		250,		5		},	// ELF
	{	170,		190,		210,		230,		260,		10	},	// HEL
	{	100,		120,		130,		160,		190,		0		},	// SAT
	{	20,			40,			60,			80,			100,		3		},	// PIX
	{	80,			110,		130,		150,		180,		2		},	// DWA
	{	70,			90,			120,		140,		170,		2		},	// HAL
	{	130,		150,		180,		210,		230,		5		},	// GNO
	{	160,		190,		220,		240,		300,		5		},	// DRO
	{	250,		290,		330,		370,		410,		10	},	// GIA
	{	170,		200,		230,		250,		310,		15	},	// VAM
	{	100,		130,		160,		190,		210,		0		},	// FIE
	{	100,		130,		160,		190,		210,		3		}		// WOL
};

//	Structure definition of race attributes redesigned.
int race_attribs[NUM_RACES][6] = {
//	VL		L			M			H			E			F
	{	3,		7,		10,		15,		17,		0		},	// HUM
	{	4,		8,		11,		16,		18,		0		},	// ELF
	{	4,		8,		11,		16,		18,		0		},	// HEL
	{	2,		6,		9,		14,		16,		0		},	// SAT
	{	1,		5,		8,		13,		15,		0		},	// PIX
	{	3,		7,		10,		15,		17,		0		},	// DWA
	{	3,		7,		10,		15,		17,		0		},	// HAL
	{	3,		7,		10,		15,		17,		0		},	// GNO
	{	3,		7,		10,		15,		17,		0		},	// DRO
	{	5,		9,		12,		17,		19,		0		},	// GIA
	{	4,		8,		11,		16,		18,		0		},	// VAM
	{	3,		7,		10,		15,		17,		0		},	// FIE
	{	4,		8,		11,		16,		18,		0		}		// WOL
};

//	Declaration of height parsing function.  Not changed, except completed the struct swapping.
int	parse_height(char arg, struct char_data *ch)
{
	int female_mod = 0, value = 0;
	if (GET_SEX(ch) == SEX_FEMALE)
		female_mod = race_heights[(int)GET_RACE(ch)][ATTRIB_FEMALE];
	switch (arg) {
		case 'a':
			value = number((race_heights[(int)GET_RACE(ch)][ATTRIB_VERYLOW] - 20), race_heights[(int)GET_RACE(ch)][ATTRIB_VERYLOW]);
			value -= female_mod;
			break;
		case 'b':
			value = number((race_heights[(int)GET_RACE(ch)][ATTRIB_LOW] - 30), race_heights[(int)GET_RACE(ch)][ATTRIB_LOW]);
			value -= female_mod;
			break;
		case 'c':
			value = number((race_heights[(int)GET_RACE(ch)][ATTRIB_MEDIUM] - 30), race_heights[(int)GET_RACE(ch)][ATTRIB_MEDIUM]);
			value -= female_mod;
			break;
		case 'd':
			value = number((race_heights[(int)GET_RACE(ch)][ATTRIB_HIGH] - 30), race_heights[(int)GET_RACE(ch)][ATTRIB_HIGH]);
			value -= female_mod;
			break;
		case 'e':
			value = number((race_heights[(int)GET_RACE(ch)][ATTRIB_EXTREME] - 30), race_heights[(int)GET_RACE(ch)][ATTRIB_EXTREME]);
			value -= female_mod;
			break;
		case 'f':
			value = number(race_heights[(int)GET_RACE(ch)][ATTRIB_EXTREME], (race_heights[(int)GET_RACE(ch)][ATTRIB_EXTREME] + 20));
			value -= female_mod;
			break;
		default:  return ATTRIB_UNDEFINED;
	}
	return (value);
}

//	Declaration fo height_class function.  Not changed, except for completion of structure swapping.
//		See NOTE 1 -- Jason Yarber
int height_class(struct char_data *ch)
{
	switch (GET_SEX(ch)) {
	case SEX_FEMALE:
		if (GET_HEIGHT(ch) > (race_heights[(int)GET_RACE(ch)][ATTRIB_EXTREME] - race_heights[(int)GET_RACE(ch)][ATTRIB_FEMALE]))
			return 5;
		else if (GET_HEIGHT(ch) > (race_heights[(int)GET_RACE(ch)][ATTRIB_HIGH] - race_heights[(int)GET_RACE(ch)][ATTRIB_FEMALE]))
			return 4;
		else if (GET_HEIGHT(ch) > (race_heights[(int)GET_RACE(ch)][ATTRIB_MEDIUM] - race_heights[(int)GET_RACE(ch)][ATTRIB_FEMALE]))
			return 3;
		else if (GET_HEIGHT(ch) > (race_heights[(int)GET_RACE(ch)][ATTRIB_LOW] - race_heights[(int)GET_RACE(ch)][ATTRIB_FEMALE]))
			return 2;
		else if (GET_HEIGHT(ch) > (race_heights[(int)GET_RACE(ch)][ATTRIB_VERYLOW] - race_heights[(int)GET_RACE(ch)][ATTRIB_FEMALE]))
			return 1;
		else
			return 0;
		break;
	default:
		if (GET_HEIGHT(ch) > race_heights[(int)GET_RACE(ch)][ATTRIB_EXTREME])
			return 5;
		else if (GET_HEIGHT(ch) > race_heights[(int)GET_RACE(ch)][ATTRIB_HIGH])
			return 4;
		else if (GET_HEIGHT(ch) > race_heights[(int)GET_RACE(ch)][ATTRIB_MEDIUM])
			return 3;
		else if (GET_HEIGHT(ch) > race_heights[(int)GET_RACE(ch)][ATTRIB_LOW])
			return 2;
		else if (GET_HEIGHT(ch) > race_heights[(int)GET_RACE(ch)][ATTRIB_VERYLOW])
			return 1;
		else
			return 0;
	}
}

//	Definition of attrib_class function.  Not changed, except completion of structure swapping
//		See NOTE 1 -- Jason Yarber
int attrib_class(struct char_data *ch, int value)                                                                            
{                                                                                                                            
	if (value > race_attribs[(int)GET_RACE(ch)][ATTRIB_EXTREME])
		return 5;
	else if (value > race_attribs[(int)GET_RACE(ch)][ATTRIB_HIGH])
		return 4;
	else if (value > race_attribs[(int)GET_RACE(ch)][ATTRIB_MEDIUM])
		return 3;
	else if (value > race_attribs[(int)GET_RACE(ch)][ATTRIB_LOW])
		return 2;
	else if (value > race_attribs[(int)GET_RACE(ch)][ATTRIB_VERYLOW])
		return 1;
	else
		return 0;
}                                                                                                                            

//	Declaration of parse_weight function.  Rewritten.
//		See NOTE 5 -- Jason Yarber
int	parse_weight(char arg, struct char_data *ch)
{
	int value = 0;
	if (GET_SEX(ch) == SEX_FEMALE)
	{
		switch (arg) {
			case 'a':
				value = number((GET_HEIGHT(ch)*30)/100,(GET_HEIGHT(ch)*36)/100);
				break;
			case 'b':
				value = number((GET_HEIGHT(ch)*37)/100,(GET_HEIGHT(ch)*43)/100);
				break;
			case 'c':
				value = number((GET_HEIGHT(ch)*42)/100,(GET_HEIGHT(ch)*50)/100);
				break;
			case 'd':
				value = number((GET_HEIGHT(ch)*51)/100,(GET_HEIGHT(ch)*57)/100);
				break;
			case 'e':
				value = number((GET_HEIGHT(ch)*58)/100,(GET_HEIGHT(ch)*64)/100);
				break;
			case 'f':
				value = number((GET_HEIGHT(ch)*65)/100,(GET_HEIGHT(ch)*71)/100);
				break;
			default:  return ATTRIB_UNDEFINED;
		}
	}
	else
	{
		switch (arg) {
			case 'a':
				value = number((GET_HEIGHT(ch)*35)/100,(GET_HEIGHT(ch)*41)/100);
				break;
			case 'b':
				value = number((GET_HEIGHT(ch)*42)/100,(GET_HEIGHT(ch)*48)/100);
				break;
			case 'c':
				value = number((GET_HEIGHT(ch)*47)/100,(GET_HEIGHT(ch)*55)/100);
				break;
			case 'd':
				value = number((GET_HEIGHT(ch)*56)/100,(GET_HEIGHT(ch)*62)/100);
				break;
			case 'e':
				value = number((GET_HEIGHT(ch)*63)/100,(GET_HEIGHT(ch)*69)/100);
				break;
			case 'f':
				value = number((GET_HEIGHT(ch)*70)/100,(GET_HEIGHT(ch)*76)/100);
				break;
			default:  return ATTRIB_UNDEFINED;
		}
	}
	return (value);
}

//	Definition of weight_class function. Rewritten.
//		See NOTE 4 -- Jason Yarber
int weight_class(struct char_data *ch)
{
	if (GET_SEX(ch) == SEX_FEMALE)
	{
		if (GET_WEIGHT(ch) >= ((GET_HEIGHT(ch)*65)/100))
			return 5;
		else if (GET_WEIGHT(ch) >= ((GET_HEIGHT(ch)*58)/100))
			return 4;
		else if (GET_WEIGHT(ch) >= ((GET_HEIGHT(ch)*51)/100))
			return 3;
		else if (GET_WEIGHT(ch) >= ((GET_HEIGHT(ch)*42)/100))
			return 2;
		else if (GET_WEIGHT(ch) >= ((GET_HEIGHT(ch)*37)/100))
			return 1;
		else
			return 0;
	}
	else {
		if (GET_WEIGHT(ch) >= ((GET_HEIGHT(ch)*70)/100))
			return 5;
		else if (GET_WEIGHT(ch) >= ((GET_HEIGHT(ch)*63)/100))
			return 4;
		else if (GET_WEIGHT(ch) >= ((GET_HEIGHT(ch)*56)/100))
			return 3;
		else if (GET_WEIGHT(ch) >= ((GET_HEIGHT(ch)*47)/100))
			return 2;
		else if (GET_WEIGHT(ch) >= ((GET_HEIGHT(ch)*42)/100))
			return 1;
		else
			return 0;
	}
}


void set_race(struct char_data *ch, int race)
{
	int remove, r;
	struct obj_data *obj;

	remove = GET_BODY(ch);

	GET_RACE(ch) = race_frames[race].race;
	GET_BODY(ch) = race_frames[race].body_bits;
	GET_SIZE(ch) = race_list[race].size;

	remove = remove - (remove & GET_BODY(ch));

	for (r = 0; r < NUM_WEARS; r++) {
		if (IS_SET(remove, (1 << r)) && (obj = GET_EQ(ch, r))) {
			if (IS_PLAYING(ch->desc)) {
				sprintf(buf, "Your %s disappears and $p is moved to your inventory!", body_parts[r]);
				act(buf, TRUE, ch, obj, 0, TO_CHAR);
			}
			obj_to_char(unequip_char(ch, r), ch);
		}
	}
}


ACMD(do_description)
{
	char *message;
	int desc_num = 0;

	if (ch->desc == NULL)
		return;

	skip_spaces(&argument);

	switch (subcmd) {

	case SCMD_DESCEDIT:
		/* set the editor up: */
		clear_screen(ch->desc);

		write_to_output(ch->desc, TRUE, "%s", stredit_header);
		/* send the description to the editor: */	
		if (GET_RPDESCRIPTION(ch, GET_ACTIVEDESC(ch)))
			write_to_output(ch->desc, FALSE, "%s", GET_RPDESCRIPTION(ch, GET_ACTIVEDESC(ch)));

		act("$n starts editing $s description.", TRUE, ch, 0, 0, TO_ROOM);

		/* start editing the desc: */	
		string_write(ch->desc, &GET_RPDESCRIPTION(ch, GET_ACTIVEDESC(ch)), 4096, 0, EDIT_RPDESC);
		break;

	case SCMD_DESCSWITCH:
		if (!*argument) {
			message = get_buffer(256);
			sprintf(message, "You currently use &W#%d&n as your active description.\r\nUsage: descswitch <number 1-5>\r\n", GET_ACTIVEDESC(ch) + 1);
			send_to_char(message, ch);
			release_buffer(message);
			return;
		}
		desc_num = atoi(argument);
		--desc_num;
		if (desc_num < 0 || desc_num > NUM_DESCS - 1) {
			send_to_char("Please enter a number between one and five.\r\n", ch);
			return;
		}
		GET_ACTIVEDESC(ch) = desc_num;
		message = get_buffer(128);
		sprintf(message, "Your active description set to &W#%d&n.\r\n", GET_ACTIVEDESC(ch) + 1);
		send_to_char(message, ch);
		release_buffer(message);
		break;

	case SCMD_SDESCEDIT:
		break;

	case SCMD_LDESCEDIT:
		break;

	default:
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "%s:%s(%d): reached default case.", __FILE__, __FUNCTION__, __LINE__);
		break;
	}

}


ACMD(do_contactinfo)
{
	char *arg;
	char *sbuf;
	struct char_data *vict;

	if (ch->desc == NULL)
		return;
	
	arg = get_buffer(256);
	skip_spaces(&argument);
	one_argument(argument, arg);
	
	if (!*arg) {
		sbuf = get_buffer(MAX_STRING_LENGTH);
		sprintf(sbuf, "&c=====[&C%s&c]===========================================&n\r\n%s&n", GET_NAME(ch), GET_CONTACTINFO(ch) ? GET_CONTACTINFO(ch) : "Not Set.\r\n");
		send_to_char(sbuf, ch);
		release_buffer(sbuf);
		release_buffer(arg);
		return;
	}
			
	if (!strcmp(arg, "write")) {
		/* set the editor up: */
		clear_screen(ch->desc);

		/* send the contactinfo to the editor: */	
		if (GET_CONTACTINFO(ch))
			write_to_output(ch->desc, FALSE, "%s", GET_CONTACTINFO(ch));

		act("$n starts editing $s contact information.", TRUE, ch, 0, 0, TO_ROOM);

		/* start editing the info: */	
		string_write(ch->desc, &GET_CONTACTINFO(ch), MAX_STRING_LENGTH, 0, EDIT_CONTACTINFO);
		
		act("$n begins to edit $s contact info.", TRUE, ch, 0, 0, TO_ROOM);
	} else {
		CREATE(vict, struct char_data, 1);
		clear_char(vict);
		CREATE(vict->player_specials, struct player_special_data, 1);
		if (load_char(arg, vict) <  0) {
			send_to_char("There is no such player.\r\n", ch);
			release_buffer(arg);
			free_char(vict);
			return;
		}
		sbuf = get_buffer(MAX_STRING_LENGTH);
		sprintf(sbuf, "&c=====[&C%s&c]===========================================&n\r\n%s&n", GET_NAME(vict), GET_CONTACTINFO(vict) ? GET_CONTACTINFO(vict) : "Not Set.\r\n");
		send_to_char(sbuf, ch);
		free_char(vict);
		release_buffer(sbuf);
	}
	release_buffer(arg);
}


/*
 * Custom point assignment for attributes and traits.
 * Do not be scared, kids.  It's just a bunch of switches and structs. ;)
 * Torgny Bjers <artovil@arcanerealms.org>, 2002-09-26 16:50.
 */
/*
 * Removed the viewing of flux rating - it only confuses people on creation
 * to have it displayed.  Added limit checking. 3.9.04 CG
 */
bool attributes_menu(struct char_data *ch, char *argument)
{
	const char *usage = "Usage: { list | help | <stat> <value> | { sdesc | ldesc } <string> | done }";
	char *printbuf = get_buffer(MAX_STRING_LENGTH);
	int i;
	bool done = FALSE;
	struct stats_struct {
		const char *name;
		sh_int min;
		sh_int max;
	} stats[] = {
		{	"strength",			1,	21								},	// 0
		{	"agility",			1,	21								},	// 1
		{	"precision",		1,	21								},
		{	"perception",		1,	21								},
		{	"health",				1,	30								},
		{	"will power",		1,	21								},	// 5
		{	"intelligence",	1,	21								},
		{	"charisma",			1,	21								},
		{	"luck",					1,	21								},
		{	"essence",			1,	21								},
		{	"social rank",	0,	NUM_SOCIAL_RANKS	},	// 10
		{	"piety rank",		0,	NUM_PIETY_RANKS		},
		{	"\n",						0,	0									}
	};
	struct attr_costs_struct {
		sh_int value;
		sh_int price;
	} attr_costs[] = {
		{	25,	7	},
		{	24,	6	},
		{	23,	5	},
		{	21,	4	},
		{	17,	3	},
		{	14,	2	},
		{	3,	1	},
		{	-1,	0	}
	};
	struct rank_costs_struct {
		sh_int value;
		sh_int price;
	} rank_costs[] = {
		{	10,	8	},
		{	4,	6	},
		{	2,	4	},
		{	0,	2	},
		{	-1,	0	}
	};

	//                   1        10        20        30        40        50        60        70        80
	//                   |--------+---------+---------+---------+---------+---------+---------+---------|
	sprintf(printbuf, "\r\n"
										"%s.------------------------[ %sCHARACTER CUSTOMIZATION %s]------------------------.\r\n", QCYN, QHCYN, QCYN);
	sprintf(printbuf, "%s|  &nStrength     : &Y%3d   &nAgility    : &Y%3d   &nSocial Rank : &Y%-17.17s  &c|\r\n", printbuf, GET_STRENGTH(ch), GET_AGILITY(ch), social_ranks[GET_SOCIAL_RANK(ch)]);
	sprintf(printbuf, "%s|  &nPrecision    : &Y%3d   &nPerception : &Y%3d   &nPiety Rank  : &Y%-17.17s  &c|\r\n", printbuf, GET_PRECISION(ch), GET_PERCEPTION(ch), piety_ranks[GET_PIETY(ch)]);
	sprintf(printbuf, "%s|  &nHealth       : &Y%3d   &nWill Power : &Y%3d                                    &c|\r\n", printbuf, GET_HEALTH(ch), GET_WILLPOWER(ch));
	sprintf(printbuf, "%s|  &nIntelligence : &Y%3d   &nCharisma   : &Y%3d                                    &c|\r\n", printbuf, GET_INTELLIGENCE(ch), GET_CHARISMA(ch));
	sprintf(printbuf, "%s|  &nLuck         : &Y%3d   &nEssence    : &Y%3d                                    &c|\r\n", printbuf, GET_LUCK(ch), GET_ESSENCE(ch));
	sprintf(printbuf, "%s|---------------------------------------------------------------------------|\r\n", printbuf);
	sprintf(printbuf, "%s|  &nShort description : &y%-52.52s&n &c|\r\n", printbuf, GET_SDESC(ch) ? GET_SDESC(ch) : "<Not set>");
	sprintf(printbuf, "%s|  &nLong description  :-                                                     &c|\r\n", printbuf);
	sprintf(printbuf, "%s|  &y%-72.72s&n &c|\r\n", printbuf, GET_LDESC(ch) ? GET_LDESC(ch) : "<Not set>");
	sprintf(printbuf, "%s'---------------------------------------------------------------------------'%s\r\n", printbuf, QNRM);
	sprintf(printbuf, "%sIf you new further help, type HELP CREATION at the prompt.\r\n\r\n", printbuf);

	if (*argument) {

		char *command = get_buffer(MAX_INPUT_LENGTH);
		char *input = get_buffer(MAX_INPUT_LENGTH);

		half_chop(argument, command, input);

		skip_spaces(&input);

		for (i = 0; *(stats[i].name) != '\n'; i++)
			if (isname(command, stats[i].name)) break;

		// List
		if (isname(command, "list")) {
			sprintf(printbuf, "%s%d Creation Points > ", printbuf, GET_CREATION_POINTS(ch));
		// Done
		} else if (isname(command, "done")) {
			if (!*(GET_SDESC(ch)) || !*(GET_LDESC(ch))) {
				sprintf(printbuf, "\r\n&RYou must set your Short and Long description before you can exit.&n\r\n\r\n%d Creation Points > ", GET_CREATION_POINTS(ch));
			} else {
				done = TRUE;
				// Have to concatenate linefeed onto L-Desc.
				sprintf(input, "%s", create_keywords(GET_LDESC(ch)));
				if (GET_KEYWORDS(ch))
					free (GET_KEYWORDS(ch));
				GET_KEYWORDS(ch) = str_dup(input);
				sprintf(input, "%s\r\n", CAP(GET_LDESC(ch)));
				free(GET_LDESC(ch));
				GET_LDESC(ch) = str_dup(input);
				if (GET_CREATION_POINTS(ch) > 0) {
					GET_QP(ch) += (GET_CREATION_POINTS(ch));
					sprintf(printbuf, "\r\n%d Creation Points converted to %d Quest Points.\r\n", GET_CREATION_POINTS(ch), GET_QP(ch));
					send_to_char(printbuf, ch);
					GET_CREATION_POINTS(ch) = 0;
				}
			}
		// Help
		} else if (isname(command, "help")) {
			if (!*input) {
				sprintf(printbuf, "\r\nVALID STATS     RANGES     VALID STATS     RANGES\r\n");
				for (i = 0; *(stats[i].name) != '\n'; i++)
					sprintf(printbuf, "%s%-15.15s [%1d-%-2d]%s", printbuf, stats[i].name, stats[i].min, stats[i].max, i % 2 == 1 ? "\r\n" : "     ");
				if (i % 2 == 1)
					sprintf(printbuf, "%s\r\n", printbuf);
				sprintf(printbuf, "%s\r\nIn order to get help on these stats, please type in the exact phrase.", printbuf);
				sprintf(printbuf, "%s\r\nThe higher you raise a stat, the more it costs.\r\n", printbuf);
				sprintf(printbuf, "%s\r\nS-Desc and L-Desc can be set with the commands SDESC and LDESC.", printbuf);
				sprintf(printbuf, "%s\r\nYou must set these two before you are allowed to proceed.\r\n\r\n", printbuf);
				sprintf(printbuf, "%s%s\r\n\r\n%d Creation Points > ", printbuf, usage, GET_CREATION_POINTS(ch));
			} else {
				/* temporarily set rights to member for help */
				USER_RIGHTS(ch) = RIGHTS_MEMBER;
				do_help(ch, input, 0, 0);
				/* and set them back */
				USER_RIGHTS(ch) = RIGHTS_NONE;
				sprintf(printbuf, "\r\n%d Creation Points > ", GET_CREATION_POINTS(ch));
			}
		// Short description
		} else if (isname(command, "sdesc")) {
			if (!*input) {
				if (GET_SDESC(ch)) {
					free(GET_SDESC(ch));
					GET_SDESC(ch) = NULL;
				}
				sprintf(printbuf, "S-Desc has been cleared.\r\n\r\n%d Creation Points > ", GET_CREATION_POINTS(ch));
			} else {
				if (strlen(input) > 45) {
					sprintf(printbuf, "\r\nYour S-Desc should not be longer than 45 characters.\r\n");
					sprintf(printbuf, "%s\r\n%d Creation Points > ", printbuf, GET_CREATION_POINTS(ch));
				} else if (input[strlen(input) - 1] != '.' && input[strlen(input) - 2] != '.' && input[strlen(input) - 2] != ',') {
					if (GET_SDESC(ch))
						free(GET_SDESC(ch));
					GET_SDESC(ch) = str_dup(LOWERALL(input));
					sprintf(printbuf, "\r\nS-Desc set to: &y%s&n\r\n", GET_SDESC(ch));
					sprintf(printbuf, "%s\r\n%d Creation Points > ", printbuf, GET_CREATION_POINTS(ch));
				} else {
					sprintf(printbuf, "\r\nYour S-Desc should be without periods and should be lowercase.\r\n");
					sprintf(printbuf, "%s\r\n%d Creation Points > ", printbuf, GET_CREATION_POINTS(ch));
				}
			}
		// Long description
		} else if (isname(command, "ldesc")) {
			if (!*input) {
				if (GET_LDESC(ch)) {
					free(GET_LDESC(ch));
					GET_LDESC(ch) = NULL;
				}
				sprintf(printbuf, "L-Desc has been cleared.\r\n\r\n%d Creation Points > ", GET_CREATION_POINTS(ch));
			} else {
				if (strlen(input) > 79) {
					sprintf(printbuf, "\r\nYour L-Desc should not be longer than 79 characters.\r\n");
					sprintf(printbuf, "%s\r\n%d Creation Points > ", printbuf, GET_CREATION_POINTS(ch));
				} else if (input[strlen(input) - 1] == '.' && input[strlen(input) - 2] != '.' && input[strlen(input) - 2] != ',') {
					if (GET_LDESC(ch))
						free(GET_LDESC(ch));
					GET_LDESC(ch) = str_dup(CAP(input));
					sprintf(printbuf, "\r\nL-Desc set to:-\r\n&y%s\r\n&n", GET_LDESC(ch));
					sprintf(printbuf, "%s\r\n%d Creation Points > ", printbuf, GET_CREATION_POINTS(ch));
				} else {
					sprintf(printbuf, "\r\nYou have to finish your L-Desc with a period.\r\n");
					sprintf(printbuf, "%s\r\n%d Creation Points > ", printbuf, GET_CREATION_POINTS(ch));
				}
			}
		// No command
		} else if (!*command) {
			sprintf(printbuf, "\r\n%s\r\n\r\n%d Creation Points > ", usage, GET_CREATION_POINTS(ch));
		// No input
		} else if (!*input) {
			int j = 0;
			switch (i) {
			case 10: // Social rank
				strcpy(printbuf, "\r\n");
				for (j = 0; j < NUM_SOCIAL_RANKS; j++)
					sprintf(printbuf, "%s[%d] %s\r\n", printbuf, j, social_ranks[j]);
				sprintf(printbuf, "%s\r\n%s\r\n\r\n%d Creation Points > ", printbuf, usage, GET_CREATION_POINTS(ch));
				break;
			case 11: // Piety rank
				strcpy(printbuf, "\r\n");
				for (j = 0; j < NUM_PIETY_RANKS; j++)
					sprintf(printbuf, "%s[%d] %s\r\n", printbuf, j, piety_ranks[j]);
				sprintf(printbuf, "%s\r\n%s\r\n\r\n%d Creation Points > ", printbuf, usage, GET_CREATION_POINTS(ch));
				break;
			default:
				sprintf(printbuf, "\r\n%s\r\n\r\n%d Creation Points > ", usage, GET_CREATION_POINTS(ch));
				break;
			}
		// Did not find any matches.
		} else if (stats[i].max == 0) {
			sprintf(printbuf, "\r\n%d Creation Points > ", GET_CREATION_POINTS(ch));
		// no errors, go ahead and change stats/traits.
		} else {
			int value = atoi(input), diff = 0, j = 0, k = 0, cost = 0, attrib = 0, newvalue = 0, race = 0;
			/* check for limits */
			for (race = 0; race < top_of_race_list; race++)
				if (race_list[race].constant == GET_RACE(ch))
					break;
			switch (i) {
			case 10:  // Social rank
				newvalue = LIMIT(value, 0, (NUM_SOCIAL_RANKS - 1));
				break;
			case 11:  // Piety rank
				newvalue = LIMIT(value, 0, (NUM_PIETY_RANKS - 1));
				break;
			default:
				newvalue = LIMIT((value*100), race_list[race].min_attrib, race_list[race].max_attrib);
				newvalue = newvalue/100;
			}

			if (newvalue < value)
				send_to_charf(ch, "Maximum value is %d.\r\n", newvalue);
			else if (newvalue > value)
				send_to_charf(ch, "Minimum value is %d.\r\n", newvalue);

			value = newvalue;
			
			switch (i) {
			case ATR_STRENGTH:
				attrib = ch->real_abils.strength;
				break;
			case ATR_AGILITY:
				attrib = ch->real_abils.agility;
				break;
			case ATR_PRECISION:
				attrib = ch->real_abils.precision;
				break;
			case ATR_PERCEPTION:
				attrib = ch->real_abils.perception;
				break;
			case ATR_HEALTH:
				attrib = ch->real_abils.health;
				break;
			case ATR_WILLPOWER:
				attrib = ch->real_abils.willpower;
				break;
			case ATR_INTELLIGENCE:
				attrib = ch->real_abils.intelligence;
				break;
			case ATR_CHARISMA:
				attrib = ch->real_abils.charisma;
				break;
			case ATR_LUCK:
				attrib = ch->real_abils.luck;
				break;
			case ATR_ESSENCE:
				attrib = ch->real_abils.essence;
				break;
			case 10:
				attrib = GET_SOCIAL_RANK(ch);
				break;
			case 11:
				attrib = GET_PIETY(ch);
				break;
			default:
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unhandled case in %s:%s(%d).", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			diff = attrib - value;
			if (diff < 0 && value <= stats[i].max) {
				// This will cost the player dearly.
				if (i < NUM_ATTRIBUTES) { // attributes
					for (j = attrib; j < value; j++) {
						for (k = 0; attr_costs[k].value != -1; k++) {
							if (j >= attr_costs[k].value) {
								if (i == ATR_HEALTH)
									cost += LIMIT(1, (attr_costs[k].price / 3), 500);
								else
									cost += attr_costs[k].price;
								break;
							}
						}
					}
				} else { // social/piety rank
					for (j = attrib; j < value; j++) {
						for (k = 0; rank_costs[k].value != -1; k++) {
							if (j >= rank_costs[k].value) {
								cost += rank_costs[k].price;
								break;
							}
						}
					}
				}
				if (cost > GET_CREATION_POINTS(ch)) {
					sprintf(printbuf, "\r\nYou do not have enough Creation Points for that.\r\n");
				} else {
					switch (i) {
					case ATR_STRENGTH:
						ch->real_abils.strength = value;
						break;
					case ATR_AGILITY:
						ch->real_abils.agility = value;
						break;
					case ATR_PRECISION:
						ch->real_abils.precision = value;
						break;
					case ATR_PERCEPTION:
						ch->real_abils.perception = value;
						break;
					case ATR_HEALTH:
						ch->real_abils.health = value;
						break;
					case ATR_WILLPOWER:
						ch->real_abils.willpower = value;
						break;
					case ATR_INTELLIGENCE:
						ch->real_abils.intelligence = value;
						break;
					case ATR_CHARISMA:
						ch->real_abils.charisma = value;
						break;
					case ATR_LUCK:
						ch->real_abils.luck = value;
						break;
					case ATR_ESSENCE:
						ch->real_abils.essence = value;
						break;
					case 10:
						GET_SOCIAL_RANK(ch) = value;
						break;
					case 11:
						GET_PIETY(ch) = value;
						break;
					default:
						extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unhandled case in %s:%s(%d).", __FILE__, __FUNCTION__, __LINE__);
						break;
					}
					sprintf(printbuf, "\r\nRaised %s to %d for %d cp.\r\n", stats[i].name, value, cost);
					GET_CREATION_POINTS(ch) -= cost;
					GET_MAX_FLUX(ch) = 0;
					for (k = 0; k <= top_of_race_list; k++)
						if (race_list[k].constant == GET_RACE(ch) && race_list[k].magic)
							GET_MAX_FLUX(ch) = (ch->real_abils.essence + ch->real_abils.willpower) / 2;
					ch->aff_abils = ch->real_abils;
				}
			} else if (diff > 0 && value >= stats[i].min) {
				// We have to pay the player back.
				if (i < NUM_ATTRIBUTES) { // attributes
					for (j = attrib - 1; j >= value; j--) {
						for (k = 0; attr_costs[k].value != -1; k++) {
							if (j >= attr_costs[k].value) {
								if (i == ATR_HEALTH)
									cost += LIMIT(1, (attr_costs[k].price / 3), 500);
								else
									cost += attr_costs[k].price;
								break;
							}
						}
					}
				} else { // social/piety rank
					for (j = attrib - 1; j >= value; j--) {
						for (k = 0; rank_costs[k].value != -1; k++) {
							if (j >= rank_costs[k].value) {
								cost += rank_costs[k].price;
								break;
							}
						}
					}
				}
				switch (i) {
				case ATR_STRENGTH:
					ch->real_abils.strength = value;
					break;
				case ATR_AGILITY:
					ch->real_abils.agility = value;
					break;
				case ATR_PRECISION:
					ch->real_abils.precision = value;
					break;
				case ATR_PERCEPTION:
					ch->real_abils.perception = value;
					break;
				case ATR_HEALTH:
					ch->real_abils.health = value;
					break;
				case ATR_WILLPOWER:
					ch->real_abils.willpower = value;
					break;
				case ATR_INTELLIGENCE:
					ch->real_abils.intelligence = value;
					break;
				case ATR_CHARISMA:
					ch->real_abils.charisma = value;
					break;
				case ATR_LUCK:
					ch->real_abils.luck = value;
					break;
				case ATR_ESSENCE:
					ch->real_abils.essence = value;
					break;
				case 10:
					GET_SOCIAL_RANK(ch) = value;
					break;
				case 11:
					GET_PIETY(ch) = value;
					break;
				default:
					extended_mudlog(NRM, SYSL_BUGS, TRUE, "Unhandled case in %s:%s(%d).", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				sprintf(printbuf, "\r\nLowered %s to %d returning %d cp.\r\n", stats[i].name, value, cost);
				GET_CREATION_POINTS(ch) += cost;
				GET_MAX_FLUX(ch) = 0;
				for (k = 0; k <= top_of_race_list; k++)
					if (race_list[k].constant == GET_RACE(ch) && race_list[k].magic)
						GET_MAX_FLUX(ch) = (ch->real_abils.essence + ch->real_abils.willpower) / 2;
				ch->aff_abils = ch->real_abils;
			} else {
				sprintf(printbuf, "\r\nYou have to set %s, which is at %d, between %d and %d.\r\n", stats[i].name, attrib, stats[i].min, stats[i].max);
			}
			sprintf(printbuf, "%s\r\n%d Creation Points > ", printbuf, GET_CREATION_POINTS(ch));
		}

		release_buffer(input);
		release_buffer(command);

	} else
		sprintf(printbuf, "\r\n%s\r\n\r\n%d Creation Points > ", usage, GET_CREATION_POINTS(ch));

	if (!done)
		send_to_char(printbuf, ch);
	release_buffer(printbuf);

	if (done)
		return (TRUE);

	return (FALSE);
}


int update_attribs(struct char_data *ch)
{
	if (!ch)
		return (FALSE);
	ch->real_abils.strength *= 100;
	ch->real_abils.agility *= 100;
	ch->real_abils.precision *= 100;
	ch->real_abils.perception *= 100;
	ch->real_abils.health *= 100;
	ch->real_abils.willpower *= 100;
	ch->real_abils.intelligence *= 100;
	ch->real_abils.charisma *= 100;
	ch->real_abils.luck *= 100;
	ch->real_abils.essence *= 100;
	return (TRUE);
}


void assign_base_stats(struct char_data *ch)
{
	int k;

	ch->real_abils.strength			= 12;
	ch->real_abils.agility			= 12;
	ch->real_abils.precision		= 12;
	ch->real_abils.perception		= 12;
	ch->real_abils.health				= 18;
	ch->real_abils.willpower		= 12;
	ch->real_abils.intelligence	= 12;
	ch->real_abils.charisma			= 12;
	ch->real_abils.luck					= 12;
	ch->real_abils.essence			= 12;

	GET_MAX_FLUX(ch) = 0;
	for (k = 0; k <= top_of_race_list; k++)
		if (race_list[k].constant == GET_RACE(ch) && race_list[k].magic)
			ch->points.flux = ch->points.max_flux = (ch->real_abils.essence + ch->real_abils.willpower) / 2;

	ch->aff_abils = ch->real_abils;

	ch->points.piety						= PIETY_UNBELIEVER;
	ch->points.reputation				= REP_UNKNOWN;
	ch->points.social_rank			= SRANK_YEOMAN;
	ch->points.military_rank		= NOTHING;
	ch->points.sanity						= SAN_SANE;

	GET_CREATION_POINTS(ch) = 30;

}


/*
 * Roll the 10 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(struct char_data *ch)
{
	int i, j, k, temp;
	ubyte table[10];
	ubyte rolls[4];

	for (i = 0; i < 10; i++)
		table[i] = 0;

	for (i = 0; i < 10; i++) {
		for (j = 0; j < 4; j++) {
			rolls[j] = number(1, 6);
		}
		temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
			MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

		for (k = 0; k < 10; k++)
			if (table[k] < temp) {
				temp ^= table[k];
				table[k] ^= temp;
				temp ^= table[k];
			}
	}

	GET_PRACTICES(ch) = number(2, 4);

	ch->real_abils.strength = table[0];
	ch->real_abils.agility = table[1];
	ch->real_abils.health = table[2];
	ch->real_abils.perception = table[3];
	ch->real_abils.willpower = table[4];
	ch->real_abils.precision = table[5];
	ch->real_abils.luck = table[6];
	ch->real_abils.intelligence = table[7];
	ch->real_abils.charisma = table[8];
	ch->real_abils.essence = table[9];
	
	ch->real_abils.health = dice(5, 6);

	switch (GET_RACE(ch)) {
		case RACE_HUMAN:
			break;
		case RACE_GRAYELF:
			SET_SKILL(ch, LANG_ELVEN, set_skill_base(ch, LANG_ELVEN));
			ch->real_abils.strength -= 2;
			ch->real_abils.agility += 1;
			ch->real_abils.precision += 1;
			ch->real_abils.health -= 1;
			ch->real_abils.intelligence += 2;
			ch->real_abils.charisma += 1;
			ch->real_abils.essence += 2;
			break;
		case RACE_HIGHELF:
			SET_SKILL(ch, LANG_ELVEN, set_skill_base(ch, LANG_ELVEN));
			SET_SKILL(ch, LANG_ANCIENT, set_skill_base(ch, LANG_ANCIENT));
			ch->real_abils.strength -= 2;
			ch->real_abils.precision += 1;
			ch->real_abils.health -= 1;
			ch->real_abils.willpower += 1;
			ch->real_abils.intelligence += 1;
			ch->real_abils.essence += 2;
			break;
		case RACE_SATYR:
			SET_SKILL(ch, LANG_ELVEN, set_skill_base(ch, LANG_ELVEN));
			SET_SKILL(ch, LANG_ANCIENT, set_skill_base(ch, LANG_ANCIENT));
			ch->real_abils.strength -= 1;
			ch->real_abils.perception -= 1;
			ch->real_abils.intelligence -= 1;
			ch->real_abils.charisma += 3;
			ch->real_abils.luck -= 1;
			ch->real_abils.essence += 1;
			break;
		case RACE_PIXIE:
		case RACE_NIXIE:
		case RACE_NYMPH:
			SET_SKILL(ch, LANG_ELVEN, set_skill_base(ch, LANG_ELVEN));
			SET_SKILL(ch, LANG_ANCIENT, set_skill_base(ch, LANG_ANCIENT));
			ch->real_abils.strength -= 2;
			ch->real_abils.agility += 2;
			ch->real_abils.precision += 1;
			ch->real_abils.perception += 1;
			ch->real_abils.health -= 2;
			ch->real_abils.willpower -= 1;
			ch->real_abils.intelligence += 1;
			ch->real_abils.charisma += 1;
			ch->real_abils.luck -= 1;
			ch->real_abils.essence += 2;
			break;
		case RACE_GOBLIN:
			SET_SKILL(ch, LANG_DWARVEN, set_skill_base(ch, LANG_DWARVEN));
			ch->real_abils.strength += 2;
			ch->real_abils.agility -= 1;
			ch->real_abils.precision += 1;
			ch->real_abils.health += 2;
			ch->real_abils.willpower += 2;
			ch->real_abils.intelligence -= 1;
			ch->real_abils.charisma -= 2;
			break;
		case RACE_GNOME:
			SET_SKILL(ch, LANG_GNOMISH, set_skill_base(ch, LANG_GNOMISH));
			SET_SKILL(ch, LANG_ANCIENT, set_skill_base(ch, LANG_ANCIENT));
			ch->real_abils.strength -= 3;
			ch->real_abils.precision += 1;
			ch->real_abils.intelligence += 1;
			ch->real_abils.charisma += 1;
			ch->real_abils.essence += 2;
			break;
		case RACE_TROLL:
			ch->real_abils.strength += 3;
			ch->real_abils.agility -= 2;
			ch->real_abils.health += 2;
			ch->real_abils.intelligence -= 4;
			ch->real_abils.charisma -= 4;
			ch->real_abils.essence -= 4;
			break;
		case RACE_GIANT:
			ch->real_abils.strength += 4;
			ch->real_abils.agility -= 2;
			ch->real_abils.health += 2;
			ch->real_abils.intelligence -= 2;
			ch->real_abils.charisma -= 1;
			ch->real_abils.essence -= 2;
			break;
		case RACE_VAMPIRE:
			SET_SKILL(ch, LANG_ANCIENT, set_skill_base(ch, LANG_ANCIENT));
			ch->real_abils.strength -= 1;
			ch->real_abils.health -= 1;
			ch->real_abils.willpower += 1;
			ch->real_abils.intelligence += 1;
			ch->real_abils.charisma += 1;
			ch->real_abils.luck -= 1;
			ch->real_abils.essence -= 1;
			break;
		case RACE_TIEFLING:
			SET_SKILL(ch, LANG_ANCIENT, set_skill_base(ch, LANG_ANCIENT));
			ch->real_abils.strength += 1;
			ch->real_abils.agility += 2;
			ch->real_abils.perception -= 2;
			ch->real_abils.health += 1;
			ch->real_abils.willpower += 1;
			ch->real_abils.intelligence -= 2;
			ch->real_abils.charisma -= 3;
			ch->real_abils.essence += 1;
			break;
		default:
			break;
	 }

	ch->aff_abils = ch->real_abils;
}


/* Some initializations for characters */
void do_start(struct char_data *ch)
{
	GET_EXP(ch) = 1;
	set_title(ch, NULL);

	ch->points.max_hit = ch->real_abils.health / 100;
	GET_MAX_HIT(ch) = ch->real_abils.health / 100;

	if (GET_TRUENAME(ch))
		free(GET_TRUENAME(ch));
	GET_TRUENAME(ch) = str_dup(true_name(GET_ID(ch)));

	GET_GOLD(ch) = 200;

	GET_HIT(ch) = GET_MAX_HIT(ch);
	GET_MANA(ch) = GET_MAX_MANA(ch);
	GET_MOVE(ch) = GET_MAX_MOVE(ch);

	GET_COND(ch, THIRST) = 24;
	GET_COND(ch, FULL) = 24;
	GET_COND(ch, DRUNK) = 0;

	ch->player.time.played = 0;
	ch->player.time.logon = time(0);

	SET_BIT(PRF_FLAGS(ch), PRF_TIPCHANNEL);

	if (siteok_everyone)
		SET_BIT(PLR_FLAGS(ch), PLR_SITEOK);
}


void set_default_ldesc(struct char_data *ch)
{
	if (!ch)
		return;
	else {
		char *printbuf = get_buffer(256);
		sprintf(printbuf, "A %s %s with %s %s hair and %s eyes stands here.",
			genders[(int)GET_SEX(ch)], race_list[(int)GET_RACE(ch)].name,
			hair_style[(int)GET_HAIRSTYLE(ch)], hair_color[(int)GET_HAIRCOLOR(ch)], eye_color[(int)GET_EYECOLOR(ch)]);
		if (ch->player.long_descr)
			free(ch->player.long_descr);
		ch->player.long_descr = str_dup(printbuf);
		release_buffer(printbuf);
	}
}
