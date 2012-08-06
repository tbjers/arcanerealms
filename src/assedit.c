/* ******************************************************************** *
 * FILE : assedit.c                     Copyright (C) 1999 Del Minturn  *
 * USAGE: Olc for assembly engine by Geoff Davis.                       *
 *        Oasis OLC by George Greer.                                    *
 * -------------------------------------------------------------------- *
 * 1999 July 25 caminturn@earthlink.net                                 *
 * 2003 January 05 artovil@arcanerealms.org                             *
 * -------------------------------------------------------------------- *
 * Skill additions Copyright (C) 2001-2002, Torgny Bjers.               *
 * artovil@arcanerealms.org | http://www.arcanerealms.org/              *
 * ******************************************************************** */
/* $Id: assedit.c,v 1.27 2003/06/02 12:02:46 artovil Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "oasis.h"
#include "assemblies.h"
#include "events.h"
#include "skills.h"
#include "constants.h"
#include "color.h"

extern const	char *unused_spellname;

/*-------------------------------------------------------------------*
 * External data structures.
 *-------------------------------------------------------------------*/
extern struct descriptor_data *descriptor_list;
extern struct obj_data *obj_proto;

extern const char *AssemblyTypes[];

/*-------------------------------------------------------------------*
 * Function prototypes.
 *-------------------------------------------------------------------*/
void assedit_setup(struct descriptor_data *d, int number);
void assedit_disp_menu(struct descriptor_data *d);
void assedit_delete(struct descriptor_data *d);
void assedit_edit_extract(struct descriptor_data *d);
void assedit_edit_inroom(struct descriptor_data *d);
void assedit_edit_itemsreq(struct descriptor_data *d);
void assedit_edit_byproduct_when(struct descriptor_data *d);
void assedit_edit_byproduct_number(struct descriptor_data *d);
void nodigit(struct descriptor_data *d, const char *cmd);
void assedit_parse(struct descriptor_data *d, char *arg);
ACMD(do_assedit);


/*-------------------------------------------------------------------*
 * Nasty internal macros to clean up the code.
 *-------------------------------------------------------------------*/
long lRnum = 0;


/*-------------------------------------------------------------------*
 * Assedit command
 *-------------------------------------------------------------------*/

ACMD (do_assedit)
{
	struct descriptor_data *d = ch->desc;
	
	*buf = '\0';  /* If I run into problems then take this sucker out */
	*buf2 = '\0';
	
	if (IS_NPC(ch))
		return;
	if (!CAN_EDIT_ASSEMBLIES(ch))
		send_to_char("You do not have permission to do that.\r\n", ch);
	
	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_ASSEDIT) {
			send_to_char("Assemblies are already being edited by someone.\r\n", ch);
			return;
		}
	}
	
	two_arguments(argument, buf, buf2);
	
	d= ch->desc;
	
	if(!*buf) {
		nodigit(d, CMD_NAME);
		return;
	}
	
	if (!isdigit(*buf)) {
		if (strn_cmp("new", buf, 3) == 0) {
			if(!isdigit(*buf2))
				nodigit(d, CMD_NAME);
			else {
				assemblyCreate(atoi(buf2), 0);
				send_to_char("Assembly Created.\r\n", d->character);
				assemblySaveAssemblies();
				return;
			}
		}
		else
			if (strn_cmp("delete", buf, 6) == 0) {
				if (!isdigit(*buf2))
					nodigit(d, CMD_NAME);
				else {
					assemblyDestroy(atoi(buf2));
					send_to_char("Assembly Deleted.\r\n", d->character);
					assemblySaveAssemblies();
					return;
				}
			}
		else {
			nodigit(d, CMD_NAME);
			return;
		}
	} else 
		if (isdigit(*buf)) {
			d = ch->desc;
			CREATE (d->olc, struct oasis_olc_data, 1);
			assedit_setup(d, atoi(buf));
			
		}
	return;
}

/*-------------------------------------------------------------------*
 * Assedit Functions
 *-------------------------------------------------------------------*/

void assedit_setup(struct descriptor_data *d, int number)
{
	
	ASSEMBLY    *pOldAssembly = NULL;
	CREATE(OLC_ASSEDIT(d), ASSEMBLY, 1 );
	
	
	if( (pOldAssembly = assemblyGetAssemblyPtr( number )) == NULL ) {
		send_to_char("That assembly does not exist\r\n", d->character);
		cleanup_olc(d, CLEANUP_ALL);
		return;
	} else {
		/* Copy the old assembly. */
		OLC_ASSEDIT(d)->lVnum = pOldAssembly->lVnum;
		OLC_ASSEDIT(d)->lSkill = pOldAssembly->lSkill;
		OLC_ASSEDIT(d)->lTime = pOldAssembly->lTime;
		OLC_ASSEDIT(d)->lSkReq = pOldAssembly->lSkReq;
		OLC_ASSEDIT(d)->bHidden = pOldAssembly->bHidden;
		OLC_ASSEDIT(d)->lProduces = pOldAssembly->lProduces;
		OLC_ASSEDIT(d)->uchAssemblyType = pOldAssembly->uchAssemblyType;
		OLC_ASSEDIT(d)->lNumComponents = pOldAssembly->lNumComponents;
		OLC_ASSEDIT(d)->lNumAltComponents = pOldAssembly->lNumAltComponents;
		OLC_ASSEDIT(d)->lNumByproducts = pOldAssembly->lNumByproducts;
		
		if( OLC_ASSEDIT(d)->lNumComponents > 0 )  {
			CREATE(OLC_ASSEDIT(d)->pComponents, COMPONENT, OLC_ASSEDIT(d)->lNumComponents);
			memmove(OLC_ASSEDIT(d)->pComponents, pOldAssembly->pComponents,
				OLC_ASSEDIT(d)->lNumComponents * sizeof( COMPONENT ) );
		}
		
		if( OLC_ASSEDIT(d)->lNumAltComponents > 0 )  {
			CREATE(OLC_ASSEDIT(d)->aComponents, ALTCOMPONENT, OLC_ASSEDIT(d)->lNumAltComponents);
			memmove(OLC_ASSEDIT(d)->aComponents, pOldAssembly->aComponents,
				OLC_ASSEDIT(d)->lNumAltComponents * sizeof( ALTCOMPONENT ) );
		}
		
		if( OLC_ASSEDIT(d)->lNumByproducts > 0 )  {
			CREATE(OLC_ASSEDIT(d)->pByproducts, BYPRODUCT, OLC_ASSEDIT(d)->lNumByproducts);
			memmove(OLC_ASSEDIT(d)->pByproducts, pOldAssembly->pByproducts,
				OLC_ASSEDIT(d)->lNumByproducts * sizeof( BYPRODUCT ) );
		}
		
	}
	
	/*
	 * At this point, pNewAssembly is now the address of a freshly allocated copy of all
	 * the data contained in the original assembly structure.
	 */
	
	
	if ( (lRnum = real_object( OLC_ASSEDIT(d)->lVnum ) ) < 0)
	 {
		 send_to_char(" Assembled item may not exist, check the vnum and assembles (show assemblies). \r\n",
			d->character);
		 cleanup_olc(d, CLEANUP_ALL);    /* for right now we just get out! */
		 return;
	 }
	
	STATE(d) = CON_ASSEDIT;
	act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
	SET_BIT(PLR_FLAGS(d->character), PLR_OLC);
	assedit_disp_menu(d);
	
}

void assedit_disp_menu(struct descriptor_data *d)
{
	int i = 0, j = 0, altComp = 0;
	char           szAssmType[ MAX_INPUT_LENGTH ] = { '\0' };
	
	get_char_colors(d->character);
	
	sprinttype( OLC_ASSEDIT(d)->uchAssemblyType, AssemblyTypes, szAssmType, sizeof(szAssmType) );
	
#if	defined(CLEAR_SCREEN)
	sprintf(buf, "%c[H%c[J", 27, 27);
	send_to_char(buf, d->character);
#endif
	
	sprintf(buf,
		"Assembly Number : %s%ld%s\r\n"
		"Assembly Name   : %s%s%s \r\n"
		"Assembly Skill  : %s%s%s\r\n"
		"Skill Required  : %s%d%%%s\r\n"
		"Creation Time   : %s%d%s\r\n"
		"Assembly Type   : %s%s%s\r\n"
		"Hidden in lists : %s%s%s\r\n"
		"Produces #      : %s%d%s\r\n"
		"Components:\r\n",
		cyn, OLC_ASSEDIT(d)->lVnum, nrm,
		yel, obj_proto[ real_object(OLC_ASSEDIT(d)->lVnum) ].short_description, nrm,
		yel, skill_name(OLC_ASSEDIT(d)->lSkill), nrm,
		cyn, (int)(OLC_ASSEDIT(d)->lSkReq / 100), nrm,
		cyn, (int)OLC_ASSEDIT(d)->lTime, nrm,
		yel, szAssmType, nrm,
		yel, YESNO(OLC_ASSEDIT(d)->bHidden), nrm,
		cyn, (int)OLC_ASSEDIT(d)->lProduces, nrm
		);
	send_to_char(buf, d->character);
	
	if (OLC_ASSEDIT(d)->lNumComponents <= 0)
		send_to_char("   < NONE > \r\n", d->character);
	else {
		for( i = 0; i < OLC_ASSEDIT(d)->lNumComponents; i++ ) {
			if ( (lRnum = real_object(OLC_ASSEDIT(d)->pComponents[i].lVnum)) < 0) {
				sprintf(buf,
					"%s %d%s) %s ERROR --- Contact an Implementor %s\r\n",
					grn, i+1, nrm, yel, nrm
				);
			} else {
				char obj_name[512];
				struct obj_data *temp;
				temp = &obj_proto[ lRnum ];
				strlcpy(obj_name, obj_proto[ lRnum ].short_description, sizeof(obj_name));
				proc_color(obj_name, 0, TRUE, GET_OBJ_COLOR( temp ), GET_OBJ_RESOURCE( temp ));
				sprintf(buf, 
					"%s %d%s) [%5ld] %-24.24s  In room:%s %-3.3s%s  Extract:%s %-3.3s%s  Instances:%s %d%s\r\n",
					grn,  i+1, nrm,
					OLC_ASSEDIT(d)->pComponents[i].lVnum,
					obj_name,
					yel, (OLC_ASSEDIT(d)->pComponents[ i ].bInRoom  ? "Yes" : "No"), nrm,
					yel, (OLC_ASSEDIT(d)->pComponents[ i ].bExtract ? "Yes" : "No"), nrm,
					yel, (OLC_ASSEDIT(d)->pComponents[ i ].bItemsReq ? OLC_ASSEDIT(d)->pComponents[ i ].bItemsReq : 1), nrm
				);
			}
			send_to_char(buf, d->character);
			if ( OLC_ASSEDIT(d)->lNumAltComponents > 0 ) {
				for( j = 0; j < OLC_ASSEDIT(d)->lNumAltComponents; j++ ) {
					if ( OLC_ASSEDIT(d)->aComponents[ j ].lComponent == OLC_ASSEDIT(d)->pComponents[ i ].lVnum ) {
						altComp++;
						if ( (lRnum = real_object(OLC_ASSEDIT(d)->aComponents[ j ].lVnum)) < 0) {
							sprintf(buf,
								"%s -- %d%s) %s ERROR --- Contact an Implementor %s\r\n",
								grn, altComp, nrm, yel, nrm
							);
						} else {
							char obj_name[512];
							struct obj_data *temp;
							temp = &obj_proto[ lRnum ];
							strlcpy(obj_name, obj_proto[ lRnum ].short_description, sizeof(obj_name));
							proc_color(obj_name, 0, TRUE, GET_OBJ_COLOR( temp ), GET_OBJ_RESOURCE( temp ));
							sprintf(buf, 
								"%s -- %d%s) [%5ld] %-40.40s\r\n",
								grn,  altComp, nrm,
								OLC_ASSEDIT(d)->aComponents[j].lVnum,
								obj_name
							);
						}
						send_to_char(buf, d->character);
					}
				}
			}
		}
	}
	sprintf(buf, 
		"%sA%s) Add a new component.\r\n"
		"%sE%s) Edit a component.\r\n"
		"%sD%s) Delete a component.\r\n"
		"%sX%s) Add a new alternate component.\r\n"
		"%sY%s) Edit an alternate component.\r\n"
		"%sZ%s) Delete an alternate component.\r\n"
		"Byproducts:\r\n",
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm
	);
	send_to_char(buf, d->character);
	if(OLC_ASSEDIT(d)->lNumByproducts <= 0)
		send_to_char("   < NONE > \r\n", d->character);
	else {
		for( i = 0; i < OLC_ASSEDIT(d)->lNumByproducts; i++ ) {
			if ( (lRnum = real_object(OLC_ASSEDIT(d)->pByproducts[i].lVnum)) < 0) {
				sprintf(buf,
					"%s %d%s) %s ERROR --- Contact an Implementor %s\r\n ",
					grn, i+1, nrm, yel, nrm
				);
			} else {
				sprintf(buf, 
					"%s %d%s) [%5ld] %-20.20s  When to load:%s %-7.7s%s   Instances:%s %d%s\r\n",
					grn,  i+1, nrm,
					OLC_ASSEDIT(d)->pByproducts[i].lVnum,
					obj_proto[ lRnum ].short_description,
					yel, AssemblyByproductLoads[ OLC_ASSEDIT(d)->pByproducts[ i ].iWhen ], nrm,
					yel, (OLC_ASSEDIT(d)->pByproducts[ i ].bItemsReq ? OLC_ASSEDIT(d)->pByproducts[ i ].bItemsReq : 1), nrm
				);
			}
			send_to_char(buf, d->character);
		}
	}
	sprintf(buf, 
		"%sM%s) Add a new byproduct.\r\n"
		"%sN%s) Edit a byproduct.\r\n"
		"%sO%s) Delete a byproduct.\r\n"
		"%sH%s) Hidden in lists?\r\n"
		"%sP%s) Number of items to produce.\r\n"
		"%sS%s) Change Assembly Skill.\r\n"
		"%sR%s) Change Skill %% required.\r\n"
		"%sC%s) Change Creation Time.\r\n"
		"%sT%s) Change Assembly Type.\r\n"
		"%sQ%s) Quit.\r\n"
		"\r\nEnter your choice : ",
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm,
		grn, nrm
	);
	send_to_char(buf, d->character);
	
	OLC_MODE(d) = ASSEDIT_MAIN_MENU;
	
	return;
}


/***************************************************
 Command Parse
 ***************************************************/

void assedit_parse(struct descriptor_data *d, char *arg)
{
	int pos = 0, i = 0, j = 0, qend = 0, counter, columns = 0, found = 0;
	char *help;
	
	COMPONENT			*pTComponents = NULL;
	ALTCOMPONENT	*pTAltComponents = NULL;
	BYPRODUCT			*pTByproducts = NULL;
	
	switch (OLC_MODE(d)) {
		
	case ASSEDIT_MAIN_MENU:
		switch (*arg) {
		case 'q':
		case 'Q':	/* do the quit stuff */
			/*
			 * Ok, Time to save it back to the original stuff and get out
			 * due to the infrequent use of this code and restricted use
			 * I decided to copy over changes regarless.
			 */
			assemblyDestroy(OLC_ASSEDIT(d)->lVnum);
			assemblyCreate(OLC_ASSEDIT(d)->lVnum, OLC_ASSEDIT(d)->uchAssemblyType);
			for( i = 0; i < OLC_ASSEDIT(d)->lNumComponents; i++) {
				assemblyAddComponent(OLC_ASSEDIT(d)->lVnum,
					OLC_ASSEDIT(d)->pComponents[i].lVnum,
					OLC_ASSEDIT(d)->pComponents[i].bExtract,
					OLC_ASSEDIT(d)->pComponents[i].bInRoom,
					OLC_ASSEDIT(d)->pComponents[i].bItemsReq
				);
			}
			for( i = 0; i < OLC_ASSEDIT(d)->lNumAltComponents; i++) {
				assemblyAddAltComponent(OLC_ASSEDIT(d)->lVnum,
					OLC_ASSEDIT(d)->aComponents[i].lComponent,
					OLC_ASSEDIT(d)->aComponents[i].lVnum
				);
			}
			for( i = 0; i < OLC_ASSEDIT(d)->lNumByproducts; i++) {
				assemblyAddByproduct(OLC_ASSEDIT(d)->lVnum,
					OLC_ASSEDIT(d)->pByproducts[i].lVnum,
					OLC_ASSEDIT(d)->pByproducts[i].iWhen,
					OLC_ASSEDIT(d)->pByproducts[i].bItemsReq
				);
			}
			assemblySetSkill(OLC_ASSEDIT(d)->lVnum, OLC_ASSEDIT(d)->lSkill, OLC_ASSEDIT(d)->lSkReq);
			assemblySetTime(OLC_ASSEDIT(d)->lVnum, OLC_ASSEDIT(d)->lTime);
			assemblySetHidden(OLC_ASSEDIT(d)->lVnum, OLC_ASSEDIT(d)->bHidden);
			assemblySetProduces(OLC_ASSEDIT(d)->lVnum, OLC_ASSEDIT(d)->lProduces);
			send_to_char("\r\nSaving all assemblies\r\n", d->character);
			assemblySaveAssemblies();
			extended_mudlog(BRF, SYSL_OLC, TRUE, "%s saves all assemblies.", GET_NAME(d->character));
			
			/*
			free(pTComponents);
			free(OLC_ASSEDIT(d));
			*/
			cleanup_olc(d, CLEANUP_ALL);    /* for right now we just get out! */
			break;

		case 't':
		case 'T':
			get_char_colors(d->character);
			clear_screen(d);
			for (counter = 0; counter < MAX_ASSM; counter++) {
				sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
								AssemblyTypes[counter], !(++columns % 2) ? "\r\n" : "");
				send_to_char(buf, d->character);
			}
			send_to_char("\r\nEnter the assembly type : ", d->character);
			OLC_MODE(d) = ASSEDIT_EDIT_TYPES;

			break;
		case 's':
		case 'S':
			get_char_colors(d->character);
			clear_screen(d);
			help = get_buffer(16384);

			strcpy(help, "\r\nSkill being one of the following:\r\n");
			for (qend = 0, i = 0; i <= TOP_SKILL_DEFINE; i++) {
				if (skill_info[i].name == unused_spellname)	/* This is valid. */
					continue;
				sprintf(help + strlen(help), "%s%3d%s) %-20.20s", grn, i, nrm, skill_info[i].name);
				if (qend++ % 3 == 2) {
					strcat(help, "\r\n");
					write_to_output(d, TRUE, "%s", help);
					*help = '\0';
				}
			}
			if (*help)
				write_to_output(d, TRUE, "%s", help);
			write_to_output(d, TRUE, "\r\n");

			release_buffer(help);

			send_to_char("\r\nEnter the skill number (-1 for none) : ", d->character);
			OLC_MODE(d) = ASSEDIT_EDIT_SKILL;

			break;
		case 'r':
		case 'R':
			get_char_colors(d->character);
			clear_screen(d);
			send_to_char("\r\nSkill % required (0 - 100) : ", d->character);
			OLC_MODE(d) = ASSEDIT_EDIT_SKILL_PERCENTAGE;
			break;
		case 'c':
		case 'C':
			get_char_colors(d->character);
			clear_screen(d);
			send_to_char("\r\nCreation time (x Pulses a 6 seconds) : ", d->character);
			OLC_MODE(d) = ASSEDIT_EDIT_CREATION_TIME;
			break;
		case 'h':
		case 'H':
			OLC_ASSEDIT(d)->bHidden = !OLC_ASSEDIT(d)->bHidden;
			assedit_disp_menu(d);
			return;
		case 'p':
		case 'P':
			get_char_colors(d->character);
			clear_screen(d);
			send_to_char("\r\nNumber of instances to produce (1-25) : ", d->character);
			OLC_MODE(d) = ASSEDIT_EDIT_PRODUCES;
			break;
		case 'a':
		case 'A':                /* add a new component */
			send_to_char("\r\nWhat is the vnum of the new component? ", d->character);
			OLC_MODE(d) = ASSEDIT_ADD_COMPONENT;
			break;

		case 'e':
		case 'E':                /* edit a component */
			if (OLC_ASSEDIT(d)->lNumComponents > 0) {
				send_to_char("\r\nEdit which component? ", d->character);
				OLC_MODE(d) = ASSEDIT_EDIT_COMPONENT;
			} else {
				assedit_disp_menu(d);
			}
			break;
		case 'd':
		case 'D':                /* delete a component */
			if (OLC_ASSEDIT(d)->lNumComponents > 0) {
				if ((pos < 0) || pos > OLC_ASSEDIT(d)->lNumComponents) {
					send_to_char("\r\nWhich component do you wish to remove? ", d->character);
					assedit_disp_menu(d);
				} else {
					send_to_char("\r\nWhich component do you wish to remove? ", d->character);
					OLC_MODE(d) = ASSEDIT_DELETE_COMPONENT;
				}
			} else {
				assedit_disp_menu(d);
			}
			break;
		case 'm':
		case 'M':                /* add a new byproduct */
			send_to_char("\r\nWhat is the vnum of the new byproduct? ", d->character);
			OLC_MODE(d) = ASSEDIT_ADD_BYPRODUCT;
			break;

		case 'n':
		case 'N':                /* edit a byproduct */
			if (OLC_ASSEDIT(d)->lNumByproducts > 0) {
				send_to_char("\r\nEdit which byproduct? ", d->character);
				OLC_MODE(d) = ASSEDIT_EDIT_BYPRODUCT;
			} else {
				assedit_disp_menu(d);
			}
			break;
		case 'o':
		case 'O':                /* delete a byproduct */
			if (OLC_ASSEDIT(d)->lNumByproducts > 0) {
				if ((pos < 0) || pos > OLC_ASSEDIT(d)->lNumByproducts) {
					send_to_char("\r\nNo such component.\r\n", d->character);
					assedit_disp_menu(d);
				} else {
					send_to_char("\r\nWhich byproduct do you wish to remove? ", d->character);
					OLC_MODE(d) = ASSEDIT_DELETE_BYPRODUCT;
				}
			} else {
				assedit_disp_menu(d);
			}
			break;
			
		case 'x':
		case 'X':                /* add a new alternate component */
			send_to_char("\r\nSet up an alternate for which component? ", d->character);
			OLC_MODE(d) = ASSEDIT_ADD_ALT_COMPONENT;
			break;

		case 'y':
		case 'Y':                /* edit an alternate component */
			if (OLC_ASSEDIT(d)->lNumAltComponents > 0) {
				send_to_char("\r\nEdit which alternate component? ", d->character);
				OLC_MODE(d) = ASSEDIT_EDIT_ALT_COMPONENT;
			} else {
				assedit_disp_menu(d);
			}
			break;
		case 'z':
		case 'Z':                /* delete an alternate component */
			if (OLC_ASSEDIT(d)->lNumAltComponents > 0) {
				if ((pos < 0) || pos > OLC_ASSEDIT(d)->lNumAltComponents) {
					send_to_char("\r\nNo such alternate component.\r\n", d->character);
					assedit_disp_menu(d);
				} else {
					send_to_char("\r\nWhich alternate component do you wish to remove? ", d->character);
					OLC_MODE(d) = ASSEDIT_DELETE_ALT_COMPONENT;
				}
			} else {
				assedit_disp_menu(d);
			}
			break;

		default:
			assedit_disp_menu(d);
		}
		break;

	case ASSEDIT_EDIT_TYPES:
		if (isdigit(*arg)){
			pos = atoi(arg) - 1;
			if( (pos >= 0) || (pos < MAX_ASSM)) {
				OLC_ASSEDIT(d)->uchAssemblyType = pos;
				assedit_disp_menu(d);
				break;
			}
		}
		else
			assedit_disp_menu(d);  
		break;

	case ASSEDIT_EDIT_SKILL:
		if (isdigit(*arg)){
			pos = atoi(arg);
			if( (pos >= 0) || (pos < MAX_SKILLS)) {
				OLC_ASSEDIT(d)->lSkill = pos;
				assedit_disp_menu(d);
				break;
			}
		}
		else
			assedit_disp_menu(d);  
		break;

	case ASSEDIT_EDIT_SKILL_PERCENTAGE:
		if (isdigit(*arg)){
			pos = atoi(arg);
			if( (pos >= 0) || (pos <= 100)) {
				OLC_ASSEDIT(d)->lSkReq = pos * 100;
				assedit_disp_menu(d);
				break;
			}
		}
		else
			assedit_disp_menu(d);  
		break;

	case ASSEDIT_EDIT_CREATION_TIME:
		if (isdigit(*arg)){
			pos = atoi(arg);
			if( (pos >= 0) || (pos <= 20)) {
				OLC_ASSEDIT(d)->lTime = pos;
				assedit_disp_menu(d);
				break;
			}
		}
		else
			assedit_disp_menu(d);  
		break;

	case ASSEDIT_EDIT_PRODUCES:
		if (isdigit(*arg)){
			pos = atoi(arg);
			OLC_ASSEDIT(d)->lProduces = LIMIT(pos, 1, 25);
			assedit_disp_menu(d);
			break;
		}
		else
			assedit_disp_menu(d);  
		break;
	
	case ASSEDIT_ADD_COMPONENT:              /* add a new component */
		if (isdigit(*arg)) {
			pos = atoi(arg);
			if ((real_object(pos)) < 0) {    /* does the object exist? */
				send_to_char("\r\n&RThat object does not exist, Please try again.&n\r\n", d->character);
				send_to_char("\r\nWhat is the vnum of the new component? ", d->character);
				break;
			}
			
			found = 0;
			for ( i = 0; i < OLC_ASSEDIT(d)->lNumComponents; i++) {
				if(OLC_ASSEDIT(d)->pComponents[i].lVnum == pos) {
					send_to_char("\r\n&RThat object has already been added to the list.&n\r\n", d->character);
					send_to_char("\r\nWhat is the vnum of the new component? ", d->character);
					found = 1;
					break;  //breaks out of the loop
				}
			}
			if (found) break; //breaks out of the case
			
			CREATE( pTComponents, COMPONENT, OLC_ASSEDIT(d)->lNumComponents + 1);
			
			if(OLC_ASSEDIT(d)->pComponents != NULL) {          /* Copy from olc to temp */
				memmove(pTComponents, OLC_ASSEDIT(d)->pComponents,
					OLC_ASSEDIT(d)->lNumComponents * sizeof(COMPONENT) );
				free(OLC_ASSEDIT(d)->pComponents);
			}
			
			OLC_ASSEDIT(d)->pComponents = pTComponents;
			OLC_ASSEDIT(d)->pComponents[ OLC_ASSEDIT(d)->lNumComponents ].lVnum = pos;
			OLC_ASSEDIT(d)->pComponents[ OLC_ASSEDIT(d)->lNumComponents ].bExtract = YES;
			OLC_ASSEDIT(d)->pComponents[ OLC_ASSEDIT(d)->lNumComponents ].bInRoom = NO;
			OLC_ASSEDIT(d)->pComponents[ OLC_ASSEDIT(d)->lNumComponents ].bItemsReq = 1;
			OLC_ASSEDIT(d)->lNumComponents += 1;
			
			assedit_disp_menu(d);
			
		} else {
			send_to_char("That object does not exist, Please try again\r\n", d->character);
			assedit_disp_menu(d);
		}
		break;
		
	case ASSEDIT_EDIT_COMPONENT:
		pos = atoi(arg);
		if (isdigit(*arg)) {
			if (pos <= OLC_ASSEDIT(d)->lNumComponents) {
				pos--;
				OLC_VAL(d) = pos;
				assedit_edit_inroom(d);
				break;
			}
			assedit_disp_menu(d);
		} else
			assedit_disp_menu(d);
		break;
		
	case ASSEDIT_DELETE_COMPONENT:
		
		if (isdigit(*arg)) {
			pos = atoi(arg);

			if (pos > OLC_ASSEDIT(d)->lNumComponents) {
				assedit_disp_menu(d);
				break;
			}

			pos -= 1;

			if ((OLC_ASSEDIT(d)->lNumComponents -1) > 0)
				CREATE( pTComponents, COMPONENT, OLC_ASSEDIT(d)->lNumComponents -1);
			
			if( pos > 0 )
				memmove( pTComponents, OLC_ASSEDIT(d)->pComponents, pos * sizeof( COMPONENT ) );
			
			if( pos < OLC_ASSEDIT(d)->lNumComponents - 1 )
				memmove( pTComponents + pos, OLC_ASSEDIT(d)->pComponents + pos + 1, 
					(OLC_ASSEDIT(d)->lNumComponents - pos - 1) * sizeof(COMPONENT) );
			
			free(OLC_ASSEDIT(d)->pComponents );
			OLC_ASSEDIT(d)->pComponents = pTComponents;
			OLC_ASSEDIT(d)->lNumComponents -= 1;
			
			assedit_disp_menu(d);
			break;
		} else
			assedit_disp_menu(d);
		break;

	case ASSEDIT_ADD_ALT_COMPONENT:              /* add a new alternate component */
		if (isdigit(*arg)) {
			pos = atoi(arg);
			found = 0;
			for ( i = 0; i < OLC_ASSEDIT(d)->lNumComponents; i++) {
				if(i + 1 == pos) {
					found = 1;
					break;  //breaks out of the loop
				}
			}
			if (!found) {
				send_to_char("\r\n&RNo such component found.&n\r\n", d->character);
				send_to_char("\r\nSet up an alternate for which component? ", d->character);
				break; //breaks out of the case
			}

			CREATE( pTAltComponents, ALTCOMPONENT, OLC_ASSEDIT(d)->lNumAltComponents + 1);

			if(OLC_ASSEDIT(d)->aComponents != NULL) {          /* Copy from olc to temp */
				memmove(pTAltComponents, OLC_ASSEDIT(d)->aComponents,
					OLC_ASSEDIT(d)->lNumAltComponents * sizeof(ALTCOMPONENT) );
				free(OLC_ASSEDIT(d)->aComponents);
			}

			OLC_ASSEDIT(d)->aComponents = pTAltComponents;
			OLC_ASSEDIT(d)->aComponents[ OLC_ASSEDIT(d)->lNumAltComponents ].lComponent = OLC_ASSEDIT(d)->pComponents[ i ].lVnum;
			OLC_ASSEDIT(d)->aComponents[ OLC_ASSEDIT(d)->lNumAltComponents ].lVnum = NOTHING;
			OLC_ASSEDIT(d)->lNumAltComponents += 1;

			send_to_char("\r\nWhat is the vnum of the new alternate component? ", d->character);
			OLC_VAL(d) = OLC_ASSEDIT(d)->lNumAltComponents - 1;
			OLC_MODE(d) = ASSEDIT_ALT_COMPONENT_VNUM;

		} else {
			send_to_char("That component does not exist, Please try again\r\n", d->character);
			assedit_disp_menu(d);
		}
		break;

	case ASSEDIT_ALT_COMPONENT_VNUM:
		if (isdigit(*arg)) {
			j = atoi(arg);
			if ((real_object(pos)) < 0) {    /* does the object exist? */
				send_to_char("\r\n&RThat object does not exist, Please try again.&n\r\n", d->character);
				send_to_char("\r\nWhat is the vnum of the new alternate component? ", d->character);
				break;
			}
			found = 0;
			for ( i = 0; i < OLC_ASSEDIT(d)->lNumAltComponents; i++) {
				if(OLC_ASSEDIT(d)->aComponents[i].lVnum == j) {
					send_to_char("\r\n&RThat object has already been added to the list.&n\r\n", d->character);
					send_to_char("\r\nWhat is the vnum of the new alternate component? ", d->character);
					found = 1;
					break;  //breaks out of the loop
				}
			}
			if (!found) {
				OLC_ASSEDIT(d)->aComponents[OLC_VAL(d)].lVnum = j;
				assedit_disp_menu(d);
			}
		} else {
			send_to_char("\r\n&RThat object does not exist, Please try again.&n\r\n", d->character);
			send_to_char("\r\nWhat is the vnum of the new alternate component? ", d->character);
		}
		break;
		
	case ASSEDIT_EDIT_ALT_COMPONENT:
		pos = atoi(arg);
		if (isdigit(*arg)) {
			if (pos <= OLC_ASSEDIT(d)->lNumAltComponents) {
				pos--;
				OLC_VAL(d) = pos;
				send_to_char("\r\nWhat is the vnum of the new alternate component? ", d->character);
				OLC_MODE(d) = ASSEDIT_ALT_COMPONENT_VNUM;
				break;
			}
			assedit_disp_menu(d);
		} else
			assedit_disp_menu(d);
		break;
		
	case ASSEDIT_DELETE_ALT_COMPONENT:
		
		if (isdigit(*arg)) {
			pos = atoi(arg);

			if (pos > OLC_ASSEDIT(d)->lNumAltComponents) {
				assedit_disp_menu(d);
				break;
			}

			pos -= 1;

			if ((OLC_ASSEDIT(d)->lNumAltComponents -1) > 0)
				CREATE( pTAltComponents, ALTCOMPONENT, OLC_ASSEDIT(d)->lNumAltComponents -1);
			
			if( pos > 0 )
				memmove( pTAltComponents, OLC_ASSEDIT(d)->aComponents, pos * sizeof( ALTCOMPONENT ) );
			
			if( pos < OLC_ASSEDIT(d)->lNumAltComponents - 1 )
				memmove( pTAltComponents + pos, OLC_ASSEDIT(d)->aComponents + pos + 1, 
					(OLC_ASSEDIT(d)->lNumAltComponents - pos - 1) * sizeof(ALTCOMPONENT) );
			
			free(OLC_ASSEDIT(d)->aComponents );
			OLC_ASSEDIT(d)->aComponents = pTAltComponents;
			OLC_ASSEDIT(d)->lNumAltComponents -= 1;
			
			assedit_disp_menu(d);
			break;
		} else
			assedit_disp_menu(d);
		break;
	
	case ASSEDIT_ADD_BYPRODUCT:              /* add a new component */
		if (isdigit(*arg)) {
			pos = atoi(arg);
			if ((real_object(pos)) < 0) {    /* does the object exist? */
				send_to_char("\r\n&RThat object does not exist, Please try again.&n\r\n", d->character);
				send_to_char("\r\nWhat is the vnum of the new component? ", d->character);
				break;
			}
			
			found = 0;
			for ( i = 0; i < OLC_ASSEDIT(d)->lNumByproducts; i++) {
				if(OLC_ASSEDIT(d)->pByproducts[i].lVnum == pos) {
					send_to_char("\r\n&RThat object has already been added to the list.&n\r\n", d->character);
					send_to_char("\r\nWhat is the vnum of the new component? ", d->character);
					found = 1;
					break;  //breaks out of the loop
				}
			}
			if (found) break; //breaks out of the case
			
			CREATE( pTByproducts, BYPRODUCT, OLC_ASSEDIT(d)->lNumByproducts + 1);
			
			if(OLC_ASSEDIT(d)->pByproducts != NULL) {          /* Copy from olc to temp */
				memmove(pTByproducts, OLC_ASSEDIT(d)->pByproducts,
					OLC_ASSEDIT(d)->lNumByproducts * sizeof(BYPRODUCT) );
				free(OLC_ASSEDIT(d)->pByproducts);
			}
			
			OLC_ASSEDIT(d)->pByproducts = pTByproducts;
			OLC_ASSEDIT(d)->pByproducts[ OLC_ASSEDIT(d)->lNumByproducts ].lVnum = pos;
			OLC_ASSEDIT(d)->pByproducts[ OLC_ASSEDIT(d)->lNumByproducts ].iWhen = ASSM_BYPROD_FAILURE;
			OLC_ASSEDIT(d)->pByproducts[ OLC_ASSEDIT(d)->lNumByproducts ].bItemsReq = 1;
			OLC_ASSEDIT(d)->lNumByproducts += 1;
			
			assedit_disp_menu(d);
			
		} else {
			send_to_char("That object does not exist, Please try again\r\n", d->character);
			assedit_disp_menu(d);
		}
		break;
		
	case ASSEDIT_EDIT_BYPRODUCT:
		pos = atoi(arg);
		if (isdigit(*arg)) {
			pos--;
			OLC_VAL(d) = pos;
			assedit_edit_byproduct_when(d);
			break;
		}
		else
			assedit_disp_menu(d);
		break;
		
	case ASSEDIT_DELETE_BYPRODUCT:
		
		if (isdigit(*arg)) {
			pos = atoi(arg);

			if (pos > OLC_ASSEDIT(d)->lNumByproducts) {
				assedit_disp_menu(d);
				break;
			}

			pos -= 1;

			if ((OLC_ASSEDIT(d)->lNumByproducts -1) > 0)
				CREATE( pTByproducts, BYPRODUCT, OLC_ASSEDIT(d)->lNumByproducts -1);
			
			if( pos > 0 )
				memmove( pTByproducts, OLC_ASSEDIT(d)->pByproducts, pos * sizeof( BYPRODUCT ) );
			
			if( pos < OLC_ASSEDIT(d)->lNumByproducts - 1 )
				memmove( pTByproducts + pos, OLC_ASSEDIT(d)->pByproducts + pos + 1, 
					(OLC_ASSEDIT(d)->lNumByproducts - pos - 1) * sizeof(BYPRODUCT) );
			
			free(OLC_ASSEDIT(d)->pByproducts );
			OLC_ASSEDIT(d)->pByproducts = pTByproducts;
			OLC_ASSEDIT(d)->lNumByproducts -= 1;
			
			assedit_disp_menu(d);
			break;
		} else
			assedit_disp_menu(d);
		break;
		
	case ASSEDIT_EDIT_EXTRACT:
		switch (*arg) {
		case 'y':
		case 'Y':
			OLC_ASSEDIT(d)->pComponents[ OLC_VAL(d) ].bExtract = TRUE;
			assedit_edit_itemsreq(d);
			break; 
			
		case 'n':
		case 'N':
			OLC_ASSEDIT(d)->pComponents[ OLC_VAL(d) ].bExtract = FALSE;
			assedit_edit_itemsreq(d);
			break;
			
		default:
			send_to_char("Is the item to be extracted when the assembly is created? (Y/N) ", d->character);
			break;
		}
		break;

	case ASSEDIT_EDIT_INROOM:
		switch (*arg) {
		case 'y':
		case 'Y':
			OLC_ASSEDIT(d)->pComponents[ OLC_VAL(d) ].bInRoom = TRUE;
			assedit_edit_extract(d);
			break;
			
		case 'n':
		case 'N':
			OLC_ASSEDIT(d)->pComponents[ OLC_VAL(d) ].bInRoom = FALSE;
			assedit_edit_extract(d);
			break;

		 default:
			send_to_char("Object in the room when assembly is created? (n =  in inventory) : ", d->character);
			break;
			}
		break;

	case ASSEDIT_EDIT_ITEMS_REQUIRED:
		if (isdigit(*arg)){
			pos = atoi(arg);
			if( (pos >= 1) || (pos <= 20)) {
				OLC_ASSEDIT(d)->pComponents[ OLC_VAL(d) ].bItemsReq = pos;
				assedit_disp_menu(d);
				break;
			}
		}
		else
			send_to_char("Number of object instances required? (1-20) : ", d->character);
		break;		

	case ASSEDIT_EDIT_BYPRODUCT_WHEN:
		if (isdigit(*arg))
			pos = atoi(arg);
		else
			pos = NOTHING;
		switch (pos) {
		case ASSM_BYPROD_SUCCESS:
		case ASSM_BYPROD_FAILURE:
		case ASSM_BYPROD_ALWAYS:
			OLC_ASSEDIT(d)->pByproducts[ OLC_VAL(d) ].iWhen = pos;
			assedit_edit_byproduct_number(d);
			break;
		default:
			assedit_edit_byproduct_when(d);
			return;
		}
		break;

	case ASSEDIT_EDIT_BYPRODUCT_NUMBER:
			if (isdigit(*arg)) {
				pos = atoi(arg);
				if( (pos >= 1) || (pos <= 20)) {
					OLC_ASSEDIT(d)->pByproducts[ OLC_VAL(d) ].bItemsReq = pos;
					assedit_disp_menu(d);
					break;
				}
			}
			else
				send_to_char("Number of byproducts to create? (1-20) : ", d->character);
			break;		

	default:	/* default for whole assedit parse function */
		/* we should never get here */
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "OLC assedit_parse(): Reached default case!");
		send_to_char("Oops...\r\n", d->character);
		STATE(d) = CON_PLAYING;
		break;
	}
}
/* End of Assedit Parse */


void assedit_delete(struct descriptor_data *d)
{
	send_to_char("Which item number do you wish to delete from this assembly? ", d->character);
	OLC_MODE(d) = ASSEDIT_DELETE_COMPONENT;
 return;
}


void assedit_edit_extract(struct descriptor_data *d)
{
	send_to_char("Is the item to be extracted when the assembly is created? (Y/N) : ", d->character);
	OLC_MODE(d) = ASSEDIT_EDIT_EXTRACT;
	return;
}


void assedit_edit_inroom(struct descriptor_data *d)
{
	send_to_char("Should the object be in the room when assembly is created (n =  in inventory)? ", d->character);
	OLC_MODE(d) = ASSEDIT_EDIT_INROOM;
	return;
}


void assedit_edit_itemsreq(struct descriptor_data *d)
{
	send_to_char("Number of object instances required? (1-20) :  ", d->character);
	OLC_MODE(d) = ASSEDIT_EDIT_ITEMS_REQUIRED;
	return;
}


void assedit_edit_byproduct_when(struct descriptor_data *d)
{
	int i;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i < 3; i++) {
		sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, AssemblyByproductLoads[i]);
		send_to_char(buf, d->character);
	}
	send_to_char("\r\nWhen should the byproduct load : ", d->character);
	OLC_MODE(d) = ASSEDIT_EDIT_BYPRODUCT_WHEN;
	return;
}


void assedit_edit_byproduct_number(struct descriptor_data *d)
{
	send_to_char("Number of byproducts to create? (1-20) : ", d->character);
	OLC_MODE(d) = ASSEDIT_EDIT_BYPRODUCT_NUMBER;
	return;
}


void nodigit(struct descriptor_data *d, const char *cmd)
{
	send_to_charf(d->character, "Usage: %s <vnum>\r\n", cmd);
	send_to_charf(d->character, "     : %s new <vnum>\r\n", cmd);
	send_to_charf(d->character, "     : %s delete <vnum>\r\n", cmd);
	return;
}

