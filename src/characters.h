/* ********************************************************************************
*		File: character.h                                   Part of CircleMUD         *
*	 Usage: Header file for character-specific code                                 *
*                                                                                 *
*  This file made exclusively for Arcane Realms MUD by Torgny Bjers.              *
*  Copyright (C) 2001, Torgny Bjers and Catherine Gore.                           *
*                                                                                 *
*	 All rights reserved.  See license.doc for complete information.                *
*                                                                                 *
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University         *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.                       *
******************************************************************************** */
/* $Id: characters.h,v 1.9 2004/04/14 17:18:11 cheron Exp $ */

#define NUM_APPEARANCES								6

#define ATTRIB_UNDEFINED							(-1)
#define ATTRIB_VERYLOW								0
#define ATTRIB_LOW										1
#define ATTRIB_MEDIUM									2
#define	ATTRIB_HIGH										3
#define ATTRIB_EXTREME								4
#define ATTRIB_FEMALE									5

#define NUM_EYECOLORS									10
#define NUM_HAIRCOLORS								13
#define NUM_HAIRSTYLES								7
#define NUM_SKINTONES									16

#define FRAME_NONE										0
#define FRAME_DEFAULT									BODY_LIGHT|BODY_NECK|BODY_HEAD|BODY_LEGS|BODY_ARMS|BODY_ABOUT|BODY_FLOAT|BODY_THROAT_1
#define FRAME_HUMANOID								FRAME_DEFAULT|BODY_FINGER_R|BODY_FINGER_L|BODY_BODY|BODY_FEET|BODY_HANDS|BODY_SHIELD|BODY_WAIST|BODY_WRIST_R|BODY_WRIST_L|BODY_WIELD|BODY_HOLD|BODY_DWIELD|BODY_FACE|BODY_FLOAT|BODY_BACK|BODY_BELT_1|BODY_BELT_2|BODY_OUTSIDE|BODY_THROAT_2
#define FRAME_WINGED_HUMANOID					FRAME_HUMANOID|BODY_WINGS
#define FRAME_DEMON										FRAME_WINGED_HUMANOID|BODY_HORNS|BODY_TAIL
#define FRAME_DRAGON									FRAME_DEFAULT|BODY_FINGER_R|BODY_FINGER_L|BODY_HANDS|BODY_WIELD|BODY_HOLD|BODY_DWIELD|BODY_THROAT_2|BODY_WINGS|BODY_HORNS|BODY_TAIL
#define FRAME_ANIMAL									FRAME_DEFAULT

#define HITAREA_NONE									0
#define HITAREA_DEFAULT								1
#define HITAREA_HUMANOID							2
#define HITAREA_WINGED_HUMANOID				3
#define HITAREA_DEMON									4
#define HITAREA_DRAGON								5
#define HITAREA_ANIMAL								6

#define NUM_HITAREAS_NONE							1
#define NUM_HITAREAS_DEFAULT					5
#define NUM_HITAREAS_HUMANOID					8
#define NUM_HITAREAS_WINGED_HUMANOID	9
#define NUM_HITAREAS_DEMON						11
#define NUM_HITAREAS_DRAGON						11
#define NUM_HITAREAS_ANIMAL						5

struct race_frames_data {
	byte race;
	bitvector_t body_bits;
	byte hitarea;
};

struct race_hitarea_data {
	byte hitarea;
	bitvector_t hitarea_bits;
	sh_int *hitareas;
	int num_areas;
};

extern const struct race_frames_data race_frames[NUM_RACES];
extern const struct race_hitarea_data race_hitareas[];

extern const char *short_character_heights[NUM_APPEARANCES][NUM_GENDERS];
extern const char *short_character_weights[NUM_APPEARANCES][NUM_GENDERS];

extern const char *eye_color[];
extern const char *hair_color[];
extern const char *hair_style[];
extern const char *skin_tone[];

extern int num_select_eyes;
extern int num_select_hair;

extern int race_heights[NUM_RACES][6];
extern int race_weights[NUM_RACES][6];
extern int race_attribs[NUM_RACES][6];

extern int parse_features(char arg, int max);

extern int parse_height(char arg, struct char_data *ch);
extern int parse_weight(char arg, struct char_data *ch);

extern int height_class(struct char_data *ch);
extern int weight_class(struct char_data *ch);
extern int weight_height_class(struct char_data *ch);
extern int attrib_class(struct char_data *ch, int value);

void description_string_cleanup(struct descriptor_data *d, int terminator);
void contactinfo_string_cleanup(struct descriptor_data *d, int terminator);

void set_race(struct char_data *ch, int race);
