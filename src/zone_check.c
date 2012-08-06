/**********************************************************************************

ZONE_CHECK.C

function:	 ACMD(do_zcheck)   
Check	certain values in mob, room and object structures to make sure they fit within
game limits.
syntax:	 zcheck 
no arguments, check your current zone
zcheck * 
scan every zone in the game

NOTE:	 The values are initially set to 1 or otherwise ridiculously low values so you
can	see the output for each message and edit it to your liking.

function ACMD(do_checkloadstatus)
calls	void check_load_status (char type, mob_rnum which, struct char_data *ch, char *name)   
Given	a virtual number, checkload will report where and how the object or mob loads
and its	percent chance to load.
syntax:	 checkload m|o <vnum>
				 
************************************************************************************/
/* $Id: zone_check.c,v 1.10 2002/10/16 15:27:28 dev Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "constants.h"

/*	 external vars  */
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto;   
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct char_data *mob_proto;
extern zone_rnum top_of_zone_table;
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern room_rnum top_of_world;
extern int get_weapon_dam (struct obj_data *o);
extern int get_armor_ac (struct obj_data *o);
extern struct char_data *read_mobile(mob_vnum nr, int type);
extern void char_to_room(struct char_data * ch, room_rnum room);

/*local	function*/
void check_load_status (char type, mob_rnum which, struct char_data *ch, char *name);
ACMD(do_zcheck);
ACMD(do_checkloadstatus);

/**************************************************************************************/
/*										Change any value to 0 to prevent checking it                    */
/**************************************************************************************/

/*item limits*/

#define	MAX_DAM_ALLOWED          5    /* for weapons */
#define	MAX_AFFECTS_ALLOWED      1
#define	MAX_MONEY_VALUE_ALLOWED  2000

/* Armor class limits*/
#define	TOTAL_WEAR_CHECKS  (NUM_ITEM_WEARS-4)  /*minus Wield, Dwield and Take*/
struct zcheck_armor {
	bitvector_t bitvector;          /*from Structs.h                       */
	int ac_allowed;                 /* Max. AC allowed for this body part  */
	char *message;                  /* phrase for error message            */
}	zarmor[] = {
	{ITEM_WEAR_FINGER, 0,  "Ring"},
	{ITEM_WEAR_NECK,   5,  "Neck"},
	{ITEM_WEAR_BODY,   14, "Body armor"},
	{ITEM_WEAR_HEAD,   5,  "Head gear"},
	{ITEM_WEAR_FACE,   1,  "Mask"},
	{ITEM_WEAR_LEGS,   8,  "Legwear"},
	{ITEM_WEAR_FEET,   5,  "Footwear"},
	{ITEM_WEAR_HANDS,  5,  "Glove"},
	{ITEM_WEAR_ARMS,   8,  "Armwear"},
	{ITEM_WEAR_SHIELD, 10, "Shield"},
	{ITEM_WEAR_ABOUT,  1,  "Cloak"},
	{ITEM_WEAR_WAIST,  1,  "Belt"},
	{ITEM_WEAR_WRIST,  1,  "Wristwear"},
	{ITEM_WEAR_HOLD,   0,  "Held item"},
  {ITEM_WEAR_BACK,   0,  "Back gear"},
	{ITEM_WEAR_FLOAT,  0,  "Floating"},
	{ITEM_WEAR_BELT,	 0,	 "Belt gear"},
	{ITEM_WEAR_OUTSIDE, 1, "Outerwear"},
	{ITEM_WEAR_THROAT, 1,	 "Throat"}	
};

/*These	two are strictly boolean*/
#define	CAN_WEAR_WEAPONS         0     /* toggle - can a weapon also be a piece of armor? */
#define	MAX_APPLIES_LIMIT        1     /*toggle - is there a limit at all?                */
/*
Applies	limits
!! Very Important:  Keep these in the same order as in Structs.h
These	will be ignored if MAX_APPLIES_LIMIT = 0
*/
struct zcheck_affs {
	int aff_type;    /*from Structs.h*/
	int max_aff;     /*max. allowed value*/
	char *message;   /*phrase for error message*/
}	zaffs[] = {
	{APPLY_NONE,              0, "None"},
	{APPLY_STRENGTH,          1, "Strength"},
	{APPLY_AGILITY,           1, "Agility"},
	{APPLY_PRECISION,         1, "Precision"},
	{APPLY_PERCEPTION,        1, "Perception"},
	{APPLY_HEALTH,            1, "Health"},
	{APPLY_WILLPOWER,         1, "Willpower"},
	{APPLY_INTELLIGENCE,      1, "Intelligence"},
	{APPLY_CHARISMA,          2, "Charisma"},
	{APPLY_LUCK,              1, "Luck"},
	{APPLY_ESSENCE,           1, "Essence"},
	{APPLY_CLASS,             0, "Class"},
	{APPLY_AGE,               0, "Age"},
	{APPLY_CHAR_WEIGHT,       0, "Character weight"},
	{APPLY_CHAR_HEIGHT,       0, "Character height"},
	{APPLY_MANA,              10, "Mana points"},
	{APPLY_HIT,               10, "Hit points"},
	{APPLY_MOVE,              10, "Move points"},
	{APPLY_GOLD,              0, "Gold"},
	{APPLY_EXP,               0, "Experience Points"},
	{APPLY_AC,                10, "Armor Class"},
	{APPLY_HITROLL,           5, "Hit roll"},
	{APPLY_DAMROLL,           5, "Damage roll"},
	{APPLY_SAVING_PARA,       3, "Saving throw (Paralysis)"},
	{APPLY_SAVING_ROD,        3, "Saving throw (Rods)"},
	{APPLY_SAVING_PETRI,      3, "Saving throw (Petrify)"},
	{APPLY_SAVING_BREATH,     3, "Saving throw (Breath)"},
	{APPLY_SAVING_SPELL,      3, "Saving throw (Spell)"},
	{APPLY_RACE,              0, "Race"}
};

/*mob	limits*/
#define	MAX_DAMROLL_ALLOWED      0
#define	MAX_HITROLL_ALLOWED      0
#define	MAX_EXP_ALLOWED          25000
#define	MAX_GOLD_ALLOWED         2000
#define	MAX_NUM_ATTACKS_ALLOWED  2

/*room limits*/
#define MAX_FLUX_ALLOWED				FLUX_TYPICAL
#define REGIOS_ALLOWED					FALSE
/* Off limit zones are any zones a player should NOT be able to walk to (ex. Limbo) */
#define	TOTAL_OFF_ZONES       3 /* What is the maximum zone offset? */
const	int offlimit_zones[] = {-1,0,1,2,3,10,11,12,13,14};  /*what zones can no room connect to (virtual num) */



/***************************************************************************************/
/***************************************************************************************/
/***************************************************************************************/


ACMD (do_zcheck)
{	 
	zone_vnum zone;
	struct obj_data *obj;
	struct char_data *mob;
	room_vnum exroom=0;
	int ac=0;
	int affs=0;
	int i, j, k;
	
	one_argument(argument, buf);
	zone = world[IN_ROOM(ch)].number/100;  /*get virtual number of zone to check*/  
	
	
	
	/************** Check mobs *****************/
	sprintf (buf, "Checking Mobs for limits...\r\n");
	/*check mobs first*/
	for (i=0; i<top_of_mobt;i++)
	{                   
		if ((mob_index[i].vnum/100) == zone || zone < 0)   /*is mob in this zone?*/
		{
			mob=read_mobile(mob_index[i].vnum,VIRTUAL);          
			char_to_room(mob,0);
			if (MAX_DAMROLL_ALLOWED)  
			{             
				if (GET_DAMROLL(mob)>MAX_DAMROLL_ALLOWED)
					sprintf(buf+strlen(buf),"[%5d] %-30s : Damroll of %d is too high (limit: %d)\r\n",
					GET_MOB_VNUM(mob), GET_NAME(mob), GET_DAMROLL(mob), MAX_DAMROLL_ALLOWED);
			}
			if (MAX_HITROLL_ALLOWED)  
			{
				 if (GET_HITROLL(mob)>MAX_HITROLL_ALLOWED)
					 sprintf(buf+strlen(buf),"[%5d] %-30s : Hitroll of %d is too high (limit: %d)\r\n",
					 GET_MOB_VNUM(mob), GET_NAME(mob), GET_HITROLL(mob), MAX_HITROLL_ALLOWED);
			}
			if (MAX_GOLD_ALLOWED)  
			{
				if (GET_GOLD(mob)>MAX_GOLD_ALLOWED)
					sprintf(buf+strlen(buf),"[%5d] %-30s : Has %d gold (limit: %d)\r\n",
					GET_MOB_VNUM(mob), GET_NAME(mob), GET_GOLD(mob), MAX_GOLD_ALLOWED);
			}
			if (MAX_EXP_ALLOWED)  
			{
				if (GET_EXP(mob)>MAX_EXP_ALLOWED)
					sprintf(buf+strlen(buf),"[%5d] %-30s : Has %d experience (limit: %d)\r\n",
					GET_MOB_VNUM(mob), GET_NAME(mob), GET_EXP(mob), MAX_EXP_ALLOWED);
			}
			/*if (MAX_NUM_ATTACKS_ALLOWED)
			{
				 if (GET_NUM_ATTACKS(mob)>MAX_NUM_ATTACKS_ALLOWED)
					 sprintf(buf+strlen(buf),"[%5d] %-30s : Has %d attacks (limit: %d)\r\n",
					 GET_MOB_VNUM(mob), GET_NAME(mob), GET_NUM_ATTACKS(mob), MAX_NUM_ATTACKS_ALLOWED);
			}*/
			/*****ADDITIONAL MOB CHECKS HERE*****/
			extract_char(mob);  
		}   /*mob is in zone*/
	}  /*check mobs*/ 
	
	/************** Check objects *****************/
	sprintf (buf+strlen(buf), "\r\nChecking Objects for limits...\r\n");
	for (i=0; i<top_of_objt; i++)
	{
		if ((obj_index[i].vnum/100) == zone || zone < 0)   /*is object in this zone?*/
		{
			obj=read_object(obj_index[i].vnum,VIRTUAL);            
			switch (GET_OBJ_TYPE(obj)) 
			{       
			case ITEM_MONEY:
				 if (MAX_MONEY_VALUE_ALLOWED)
					 if (GET_OBJ_VAL(obj, 0)>MAX_MONEY_VALUE_ALLOWED)
						 sprintf(buf+strlen(buf), "[%5d] %-30s : Object is worth %d (money limit %d)\r\n",
						 GET_OBJ_VNUM(obj), obj->short_description, 
						 GET_OBJ_VAL(obj, 0), MAX_MONEY_VALUE_ALLOWED);
					 break;             
			 case ITEM_WEAPON:
				 if (MAX_DAM_ALLOWED)
					 if (get_weapon_dam(obj)>MAX_DAM_ALLOWED)
						 sprintf(buf+strlen(buf), "[%5d] %-30s : Damroll is %d (limit %d)\r\n",
						 GET_OBJ_VNUM(obj), obj->short_description,
						 (int)(((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)),
						 MAX_DAM_ALLOWED);
					 
					 if (!CAN_WEAR_WEAPONS)
					 {
						 /*first remove legitimate weapon bits*/
						 if (OBJWEAR_FLAGGED(obj,ITEM_WEAR_TAKE)) TOGGLE_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
						 if (OBJWEAR_FLAGGED(obj, ITEM_WEAR_WIELD)) TOGGLE_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_WIELD);
						 if (OBJWEAR_FLAGGED(obj, ITEM_WEAR_HOLD)) TOGGLE_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_HOLD);
			 if (OBJWEAR_FLAGGED(obj, ITEM_WEAR_DWIELD)) TOGGLE_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_DWIELD);
						 if (obj->obj_flags.wear_flags)  /*any bits still on?*/                        
							 sprintf (buf+strlen(buf), "[%5d] %-30s : Weapons cannot be worn as armor.\r\n",
							 GET_OBJ_VNUM(obj), obj->short_description);    
					 }  /*wear weapons*/ 
					 break;
			 case ITEM_ARMOR:
				 ac=GET_OBJ_VAL(obj,0);
				 for (j=0; j<TOTAL_WEAR_CHECKS;j++)
				 {
					 if (CAN_WEAR(obj,zarmor[j].bitvector) && (ac>zarmor[j].ac_allowed))
						 sprintf(buf+strlen(buf), "[%5d] %-30s : Item has AC %d (%s limit is %d)\r\n",
						 GET_OBJ_VNUM(obj), obj->short_description, 
						 ac, zarmor[j].message, zarmor[j].ac_allowed);
				 }
				 break;             
			}  /*switch on Item_Type*/
			
			/*first check for over-all affections*/
			if (MAX_AFFECTS_ALLOWED)
			{
				affs=0;
				for (j = 0; j < MAX_OBJ_AFFECT; j++) if (obj->affected[j].modifier) affs++;
				if (affs>MAX_AFFECTS_ALLOWED)
					sprintf(buf+strlen(buf), "[%5d] %-30s : Has %d affects (limit %d).\r\n",
					GET_OBJ_VNUM(obj), obj->short_description, affs, MAX_AFFECTS_ALLOWED);
			}
			
			if (MAX_APPLIES_LIMIT)
			{   
				for (j=0;j<MAX_OBJ_AFFECT;j++)
				{
					if (zaffs[(int)obj->affected[j].location].max_aff>0)  /*only check ones with limits*/
					{
						if (obj->affected[j].modifier > zaffs[(int)obj->affected[j].location].max_aff)
						{                      
							sprintf(buf+strlen(buf), "[%5d] %-30s : Apply to %s is %d (limit %d).\r\n",
								GET_OBJ_VNUM(obj), obj->short_description, 
								zaffs[(int)obj->affected[j].location].message,
								obj->affected[j].modifier,
								zaffs[(int)obj->affected[j].location].max_aff); 
						}
					}
				}
			}
			
			/*****ADDITIONAL OBJ CHECKS HERE*****/
			extract_obj(obj);
		}   /*object is in zone*/
	} /*check objects*/
	
	
	/************** Check rooms *****************/
	sprintf (buf+strlen(buf), "\r\nChecking Rooms for limits...\r\n");
	for (i=0; i<top_of_world;i++)
	{
		if (zone_table[world[i].zone].number==zone || zone < 0)
		{
			for (j = 0; j < NUM_OF_DIRS; j++)
			{ /*check for exit, but ignore off limits if you're in an offlimit zone*/
				if (world[i].dir_option[j] && world[i].dir_option[j]->to_room>(-1) &&
					(world[world[i].dir_option[j]->to_room].number/100)!=zone) 
				{
					exroom=world[i].dir_option[j]->to_room;   /*get zone it connects to*/
					exroom=world[exroom].number;
					for (k=0;k<TOTAL_OFF_ZONES;k++)
					{
						/*don't send warning if the room itself is in the same off-limit zone.*/
						if (((exroom/100)==offlimit_zones[k]) && (world[i].number/100 != offlimit_zones[k]))
							sprintf(buf+strlen(buf), "[%5d] Exit %s cannot connect to %d (off limits).\r\n",
							world[i].number, dirs[j],exroom);
					} /*bad exit?*/
				}  /*exit exist?*/
			}
			if (world[i].magic_flux > MAX_FLUX_ALLOWED)
				sprintf(buf+strlen(buf), "[%5d] magical flux of %d is too high, max is %d.\r\n",
							world[i].number, world[i].magic_flux, MAX_FLUX_ALLOWED);
			if (REGIOS_ALLOWED == FALSE && IS_REGIO(real_room(world[i].number)))
				sprintf(buf+strlen(buf), "[%5d] regio sector used.\r\n",
							world[i].number);
		}  /*is room in this zone?*/
	} /*checking rooms*/  
	page_string (ch->desc,buf,1); 
}




/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

#define	ZCMD zone_table[zone].cmd[cmd_no]  /*fom DB.C*/

/* 
type = 'O' or 'M'
which=real num.
name = common name of mob or object  
*/
void check_load_status (char type, mob_rnum which, struct char_data *ch, char *name)
{
	zone_rnum zone;
	int cmd_no;
	room_vnum lastroom_v=0;
	room_rnum lastroom_r=0;
	mob_rnum lastmob_r=0;
	char mobname[100];
	struct char_data *mob;  
	
	sprintf(buf, "Checking load info for %s...\r\n", name);
	
	for (zone=0; zone <= top_of_zone_table; zone++)
	{    
		for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) 
		{
			if (type == 'm' || type == 'M')
			{
				switch (ZCMD.command) 
				 {
				case 'M':                   /* read a mobile */
					if (ZCMD.arg1 == which)
					{
						sprintf(buf+strlen(buf), "  [%5d] %s (%d MAX)\r\n",world[ZCMD.arg3].number, world[ZCMD.arg3].name/*, (100-ZCMD.arg4)*/, ZCMD.arg2);
					}
					break;
				}
			}
			else
			{
				switch (ZCMD.command) 
				{
				case 'M':
					lastroom_v = world[ZCMD.arg3].number;
					lastroom_r = ZCMD.arg3;
					lastmob_r = ZCMD.arg1;
					mob = read_mobile(lastmob_r, REAL);
					char_to_room(mob, 0);
					strcpy(mobname, GET_NAME(mob));
					extract_char(mob);
					break;
				case 'O':                   /* read an object */
					lastroom_v = world[ZCMD.arg3].number;
					lastroom_r = ZCMD.arg3;
					if (ZCMD.arg1 == which)
					{                       
						sprintf(buf+strlen(buf), "  [%5d] %s (%d Max)\r\n",lastroom_v, world[lastroom_r].name/*, (100-ZCMD.arg4)*/, ZCMD.arg2);
					}
					break;
					
				case 'P':                   /* object to object */
					if (ZCMD.arg1 == which)
					{
						sprintf(buf+strlen(buf), "  [%5d] %s (Put in another object [%d Max])\r\n",lastroom_v, world[lastroom_r].name/*, (100-ZCMD.arg4)*/, ZCMD.arg2);
					}
					break;
					
				case 'G':                   /* obj_to_char */
					if (ZCMD.arg1 == which)
					{
						sprintf(buf+strlen(buf), "  [%5d] %s (Given to %s [%d][%d Max])\r\n",lastroom_v, world[lastroom_r].name, mobname, mob_index[lastmob_r].vnum/*, (100-ZCMD.arg4)*/, ZCMD.arg2);
					}
					break;
					
				case 'E':                   /* object to equipment list */
					if (ZCMD.arg1 == which)
					{
						sprintf(buf+strlen(buf), "  [%5d] %s (Equipped to %s [%d][%d Max])\r\n",lastroom_v, world[lastroom_r].name, mobname, mob_index[lastmob_r].vnum/*, (100-ZCMD.arg4)*/, ZCMD.arg2);
					}
					break;
					
				case 'R': /* rem obj from room */
					lastroom_v = world[ZCMD.arg1].number;
					lastroom_r = ZCMD.arg1;
					if (ZCMD.arg2 == which)
					{
						sprintf(buf+strlen(buf), "  [%5d] %s (Removed from room)\r\n",lastroom_v, world[lastroom_r].name);                    
					}
					break;
				}/*switch for object*/
			}  /*else...*/
		} /*for cmd_no......*/
	}  /*for zone...*/
	page_string (ch->desc, buf, 1);
}



ACMD(do_checkloadstatus)
{	 
	mob_rnum which;
	struct char_data *mob;
	struct obj_data *obj;
	
	two_arguments(argument, buf1, buf2);
	if ((!*buf1) || (!*buf2) || (!isdigit(*buf2)))
	{
		send_to_char("Checkload <M | O> <vnum>\r\n", ch);
		return;
	}   
	
	/*These lines do nothing but look up the name of the object or mob
	so the output is more user friendly*/
	if (*buf1 == 'm')
	{          
		which = real_mobile(atoi(buf2));    
		if (which < 0)
		{
			send_to_char("That mob does not exist.\r\n", ch);
			return;
		}
		mob = read_mobile(which, REAL);
		char_to_room(mob, 0);
		strcpy(buf2, GET_NAME(mob));
		extract_char(mob);
	}
	else
	{
		which = real_object(atoi(buf2)); 
		if (which < 0)
		{
			send_to_char("That object does not exist.\r\n", ch);
			return;
		}
		obj = read_object(which, REAL);
		strcpy(buf2, obj->short_description);
		extract_obj(obj);
	}   
	
	/*end name look up*/
	
	check_load_status(buf1[0], which, ch, buf2);  
}
