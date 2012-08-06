/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*	 _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*																																							 *
*	 OasisOLC - qedit.c                                                           *
*																																							 *
*	 Copyright 1997 Mike Steinmann                                          *
*	 Used at Morgaelin (mud.dwango.com 3000)                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*.	Original author: Levork .*/
/* $Id: qedit.c,v 1.10 2002/11/06 18:55:47 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "comm.h"
#include "utils.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "boards.h"
#include "oasis.h"
#include "quest.h"
#include "genolc.h"
#include "constants.h"

/*------------------------------------------------------------------------*/
/*.	External data .*/

extern int                top_of_aquestt;
extern struct                aq_data *aquest_table;
extern struct                zone_data *zone_table;
extern const char        *quest_types[];
extern const char        *aq_flags[];
extern void                smash_tilde(char *str);
extern struct char_data *mob_proto;

/*------------------------------------------------------------------------*/
/* function protos */
void qedit_setup_new(struct descriptor_data *d);
void qedit_setup_existing(struct descriptor_data *d, int real_num);
void qedit_save_internally(struct descriptor_data *d);
void qedit_save_to_disk(int znum);
void free_quest(struct aq_data *quest);
void qedit_disp_type_menu(struct descriptor_data *d);
void qedit_disp_menu(struct descriptor_data * d);
void qedit_parse(struct descriptor_data * d, char *arg);
void qedit_disp_flags_menu(struct descriptor_data *d);
void qedit_disp_val0_menu(struct descriptor_data *d);
void qedit_disp_val1_menu(struct descriptor_data *d);
void qedit_disp_val2_menu(struct descriptor_data *d);
void qedit_disp_val3_menu(struct descriptor_data *d);

/*------------------------------------------------------------------------*\
	Utils and exported functions.
\*------------------------------------------------------------------------*/

/* void smash_tilde(char *str)
{
	for (; *str != '\0'; str++) {
		if (*str == '~')
			*str = '-';
		if (*str == 'M')
			*str = ' ';
	}
					 
	return;
}	*/
													
void qedit_setup_new(struct descriptor_data *d)
{	

	CREATE(OLC_QUEST(d), struct aq_data, 1);
	OLC_QUEST(d)->short_desc = str_dup("an unfinished quest");
	OLC_QUEST(d)->desc = str_dup("This is an unfinished quest.");
	OLC_QUEST(d)->info = str_dup("Information for unfinished quest.\r\n");
	OLC_QUEST(d)->ending = str_dup("Ending for unfinished quest.\r\n");
	OLC_QUEST(d)->next_quest = -1;
	qedit_disp_menu(d);
	OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void qedit_setup_existing(struct descriptor_data *d, int real_num)
{	
	struct aq_data *quest;
	int i;
	
	/*. Build a copy of the quest .*/
	CREATE (quest, struct aq_data, 1);
	*quest = aquest_table[real_num];
	/* allocate space for all strings  */
	if (aquest_table[real_num].short_desc)
		quest->short_desc = str_dup (aquest_table[real_num].short_desc);
	if (aquest_table[real_num].desc)
		quest->desc = str_dup (aquest_table[real_num].desc);
	if (aquest_table[real_num].info)
		quest->info = str_dup (aquest_table[real_num].info);
	if (aquest_table[real_num].ending)
		quest->ending = str_dup (aquest_table[real_num].ending);
	quest->type = aquest_table[real_num].type;
	quest->mob_vnum = aquest_table[real_num].mob_vnum;
	quest->flags = aquest_table[real_num].flags;
	quest->target = aquest_table[real_num].target;
	quest->exp = aquest_table[real_num].exp;
	quest->next_quest = aquest_table[real_num].next_quest;
	for (i=0; i < 4; i++)
		quest->value[i] = aquest_table[real_num].value[i];
	/*. Attach room copy to players descriptor .*/
	OLC_QUEST(d) = quest;
	OLC_VAL(d) = 0;
	qedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/
void qedit_save_internally(struct descriptor_data *d)
{	
	int i, quest_num, found = 0;
	struct aq_data *new_quest;

	quest_num = real_quest(OLC_NUM(d));
	if (quest_num >= 0) 
	{ 
		free_quest(aquest_table + quest_num);
		aquest_table[quest_num] = *OLC_QUEST(d);
	} else { 
		/*. Quest doesn't exist, hafta add it .*/
		CREATE(new_quest, struct aq_data, top_of_aquestt + 2);

		/* count thru quest tables */
		for (i = 0; i <= top_of_aquestt; i++) 
		{ 
			if (!found) {
				/*. Is this the place? .*/
				if (aquest_table[i].virtual > OLC_NUM(d)) 
				{ 
					found = 1;

					new_quest[i] = *(OLC_QUEST(d));
					new_quest[i].virtual = OLC_NUM(d);
					quest_num  = i;
					new_quest[i + 1] = aquest_table[i];
					
				} else { 
					new_quest[i] = aquest_table[i];
				}
			} else
			{ /*. Already been found  .*/
				new_quest[i + 1] = aquest_table[i];
			}
		}
		if (!found)
		{ /*. Still not found, insert at top of table .*/
			new_quest[i] = *(OLC_QUEST(d));
			new_quest[i].virtual = OLC_NUM(d);
			quest_num  = i;
		}

		/* copy quest table over */
		free(aquest_table);
		aquest_table = new_quest;
		top_of_aquestt++;

	}
	add_to_save_list(zone_table[OLC_ZNUM(d)].number, SL_QST);
}


/*------------------------------------------------------------------------*/

void qedit_save_to_disk(int znum)
{
	int counter, realcounter;
	FILE *fp;
	struct aq_data *quest;
	char buf3[MAX_STRING_LENGTH];
	char buf4[MAX_STRING_LENGTH];

	sprintf(buf, "%s/%d.qst", QST_PREFIX, zone_table[znum].number);
	if (!(fp = fopen(buf, "w+"))) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "OLC: Cannot open quest file!");
		return;
	}

	for (counter = zone_table[znum].number * 100;
			 counter <= zone_table[znum].top;
			 counter++) 
	{ realcounter = real_quest(counter);
		if (realcounter >= 0) 
		{ 
			quest = (aquest_table + realcounter);
			strcpy(buf1, quest->short_desc ? quest->short_desc : "an unfinished quest");
			smash_tilde(buf1);
			strcpy(buf2, quest->desc ? quest->desc : "This is an unfinished quest.");
			smash_tilde(buf2);
			strcpy(buf3, quest->info ? quest->info : "Information for an unfinished quest.\r\n");
			strip_cr(buf3);
			smash_tilde(buf3);
			strcpy(buf4, quest->ending ? quest->ending : "There is no ending!\r\n");
			strip_cr(buf4);
			smash_tilde(buf4);

			/*. Build a buffer ready to save .*/
			sprintf(buf, "#%d\n%s~\n%s~\n%s~\n%s~\n%d %d %ld %d %d %d\n",
											counter, buf1, buf2, buf3, buf4,
											quest->type, quest->mob_vnum,
											quest->flags, quest->target, quest->exp, quest->next_quest
			);
			/*. Save this section .*/
			fputs(buf, fp);
			
			sprintf(buf, "%d %d %d %d\n", quest->value[0], quest->value[1],
							quest->value[2], quest->value[3]);
			fputs(buf, fp);
			
			fprintf(fp, "S\n");
		}
	}
	/* write final line and close */
	fprintf(fp, "$~\n");
	fclose(fp);
	remove_from_save_list(zone_table[znum].number, SL_QST);
}

/*------------------------------------------------------------------------*/

void free_quest(struct aq_data *quest)
{	
	if (quest->desc)
		free(quest->desc);
	if (quest->short_desc)
		free(quest->short_desc);
	if (quest->info)
		free(quest->info);
	if (quest->ending)
		free(quest->ending);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

void qedit_disp_flags_menu(struct descriptor_data * d)
{
	int counter, columns = 0;

	get_char_colors(d->character);
	clear_screen(d);
#if	defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif

	for (counter = 0; counter < NUM_AQ_FLAGS; counter++) 
	{ sprintf(buf, "%s%2d%s) %-20.20s ",
						grn, counter + 1, nrm, aq_flags[counter]);
		if(!(++columns % 2))
			strcat(buf, "\r\n");
		send_to_char(buf, d->character);
	}
	sprintbit(OLC_QUEST(d)->flags, aq_flags, buf1, sizeof(buf1));
	sprintf(buf, 
				"\r\nQuest flags: %s%s%s\r\n"
					"Enter quest flags, 0 to quit : ",
				cyn, buf1, nrm
	);
	send_to_char(buf, d->character);
	OLC_MODE(d) = QEDIT_FLAGS;
}


void qedit_disp_type_menu(struct descriptor_data *d)
{
	int counter, columns = 0;
	
	send_to_char("[H[J", d->character);
	for (counter = 0; counter < NUM_AQ_TYPES; counter++) {
		sprintf(buf, "%s%2d%s) %-20.20s ",
								grn, counter, nrm, quest_types[counter]);
		if(!(++columns % 2))
			strcat(buf, "\r\n");
		send_to_char(buf, d->character);
	}
	send_to_char("\r\nEnter quest type : ", d->character);
	OLC_MODE(d) = QEDIT_TYPE;
}

void qedit_disp_val0_menu(struct descriptor_data *d)
{
	OLC_MODE(d) = QEDIT_VALUE_0;
	switch(OLC_QUEST(d)->type) {
		case AQ_RETURN_OBJ:
			send_to_char("Enter vnum of mob to receive object: ", d->character);
			break;
		default:
			qedit_disp_menu(d);
	}
}

void qedit_disp_val1_menu(struct descriptor_data *d)
{
	OLC_MODE(d) = QEDIT_VALUE_1;
	switch(OLC_QUEST(d)->type) {
		default:
			qedit_disp_menu(d);
	}
}

void qedit_disp_val2_menu(struct descriptor_data *d)
{
	OLC_MODE(d) = QEDIT_VALUE_2;
	switch(OLC_QUEST(d)->type) {
		default:
			qedit_disp_menu(d);
	}
}

void qedit_disp_val3_menu(struct descriptor_data *d)
{
	OLC_MODE(d) = QEDIT_VALUE_3;
	switch(OLC_QUEST(d)->type) {
		default:
			qedit_disp_menu(d);
	}
}

/* the main menu */
void qedit_disp_menu(struct descriptor_data * d)
{
	struct aq_data *quest;

	get_char_colors(d->character);
	clear_screen(d);
#if	defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif

	quest = OLC_QUEST(d);
	
	sprintbit(quest->flags, aq_flags, buf1, sizeof(buf1));
	sprintf(buf2, "%d %d %d %d", quest->value[0], quest->value[1], quest->value[2],
					quest->value[3]);
	sprintf(buf,
					"[H[J"
				"-- Quest number : [%s%d%s]          Quest zone: [%s%d%s]\r\n"
				"%s1%s) Name        : %s%s%s\r\n"
				"%s2%s) Description :\r\n%s%s%s\r\n"
				"%s3%s) Information :\r\n%s%s%s"
				"%s4%s) Ending      :\r\n%s%s%s"
				"%s5%s) Questmaster : %s%d%s -- %s%s%s\r\n"
				"%s6%s) Type        : %s%s%s\r\n"
				"%s7%s) Flags       : %s%s%s\r\n"
				"%s8%s) Target vnum : %s%d%s\r\n"
				"%s9%s) Experience  : %s%d%s\r\n"
				"%sA%s) Next quest  : %s%d%s\r\n"
				"%sB%s) Values      : %s%s%s\r\n"
					"%sQ%s) Quit\r\n"
					"Enter choice : ",

				cyn, OLC_NUM(d), nrm,
				cyn, zone_table[OLC_ZNUM(d)].number, nrm,
				grn, nrm, yel, quest->short_desc, nrm,
				grn, nrm, yel, quest->desc, nrm,
				grn, nrm, yel, quest->info, nrm,
				grn, nrm, yel, quest->ending, nrm,
				grn, nrm, cyn, quest->mob_vnum, nrm, cyn, 
				real_mobile(quest->mob_vnum) > -1 ? mob_proto[real_mobile(quest->mob_vnum)].player.short_descr : "None", nrm,
				grn, nrm, cyn, quest_types[quest->type], nrm,
				grn, nrm, cyn, buf1, nrm,
				grn, nrm, cyn, quest->target, nrm,
				grn, nrm, cyn, quest->exp, nrm,
				grn, nrm, cyn, quest->next_quest, nrm,
				grn, nrm, cyn, buf2, nrm,
				grn, nrm
	);
	send_to_char(buf, d->character);

	OLC_MODE(d) = QEDIT_MAIN_MENU;
}



/**************************************************************************
	The main loop
 **************************************************************************/

void qedit_parse(struct descriptor_data * d, char *arg)
{	
	/* extern struct aq_data *aquest_table; */
	int number;

	switch (OLC_MODE(d)) {
	case QEDIT_CONFIRM_SAVESTRING:
		switch (*arg) {
		case 'y':
		case 'Y':
			qedit_save_internally(d);
			qedit_save_to_disk(OLC_ZNUM(d));
			extended_mudlog(NRM, SYSL_OLC, TRUE, "%s edits quest %d.", GET_NAME(d->character), OLC_NUM(d));
			/*. Do NOT free strings! just the room structure .*/
			cleanup_olc(d, CLEANUP_STRUCTS);
			write_to_output(d, TRUE, "Quest saved to memory and disk.\r\n");
			break;
		case 'n':
		case 'N':
			/* free everything up, including strings etc */
			cleanup_olc(d, CLEANUP_ALL);
			break;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\nDo you wish to save this quest? : ");
			break;
		}
		return;

	case QEDIT_MAIN_MENU:
		switch (*arg) {
		case 'q':
		case 'Q':
			if (OLC_VAL(d)) { /*. Something has been modified .*/
				write_to_output(d, TRUE, "Do you wish to save this quest? : ");
				OLC_MODE(d) = QEDIT_CONFIRM_SAVESTRING;
			} else
				cleanup_olc(d, CLEANUP_ALL);
			return;
		case '1':
			write_to_output(d, TRUE, "Enter quest name:-\r\n| ");
			OLC_MODE(d) = QEDIT_NAME;
			break;
		case '2':
			write_to_output(d, TRUE, "Enter quest description:-\r\n| ");
			OLC_MODE(d) = QEDIT_DESC;
			break;
		case '3':
			OLC_MODE(d) = QEDIT_INFO;
			clear_screen(d);
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_QUEST(d)->info)
				write_to_output(d, TRUE, "%s", OLC_QUEST(d)->info);
			string_write(d, &OLC_QUEST(d)->info, MAX_QUEST_INFO, 0, STATE(d));
			OLC_VAL(d) = 1;
			break;
		case '4':
			OLC_MODE(d) = QEDIT_ENDING;
			clear_screen(d);
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_QUEST(d)->ending)
				write_to_output(d, TRUE, "%s", OLC_QUEST(d)->ending);
			string_write(d, &OLC_QUEST(d)->ending, MAX_QUEST_ENDING, 0, STATE(d));
			OLC_VAL(d) = 1;
			break;      
		case '5':
			send_to_char("Enter questmaster vnum: ", d->character);
			OLC_MODE(d) = QEDIT_QUESTMASTER;
			break;
		case '6':
			qedit_disp_type_menu(d);
			break;
		case '7':
			qedit_disp_flags_menu(d);
			break;
		case '8':
			send_to_char("Enter target vnum: ", d->character);
			OLC_MODE(d) = QEDIT_TARGET;
			break;
		case '9':
			send_to_char("Enter experience: ", d->character);
			OLC_MODE(d) = QEDIT_EXP;
			break;
		case 'a':
		case 'A':
			send_to_char("Next quest (-1 to end): ", d->character);
			OLC_MODE(d) = QEDIT_NEXT;
			break;
		case 'b':
		case 'B':
			OLC_QUEST(d)->value[0] = 0;
			OLC_QUEST(d)->value[1] = 0;
			OLC_QUEST(d)->value[2] = 0;
			OLC_QUEST(d)->value[3] = 0;
			qedit_disp_val0_menu(d);
			break;
		default:
			send_to_char("Invalid choice!", d->character);
			qedit_disp_menu(d);
			break;
		}
		return;
		
	case QEDIT_NAME:
		if (OLC_QUEST(d)->short_desc)
			free(OLC_QUEST(d)->short_desc);
		if (strlen(arg) > MAX_QUEST_NAME)
			arg[MAX_QUEST_NAME -1] = 0;
		OLC_QUEST(d)->short_desc = str_dup(arg);
		break;
		
	case QEDIT_DESC:
		if (OLC_QUEST(d)->desc)
			free(OLC_QUEST(d)->desc);
		if (strlen(arg) > 80)
			arg[79] = 0;
		OLC_QUEST(d)->desc = str_dup(arg);
		break;    
		
	case QEDIT_INFO:
		/*
		 * We will NEVER get here, we hope.
		 */
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached QEDIT_INFO case in parse_qedit");
		break;
		
	case QEDIT_ENDING:
		/*
		 * We will NEVER get here, we hope.
		 */
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached QEDIT_ENDING case in parse_qedit");
		break;    
															
	case QEDIT_QUESTMASTER:
		number = atoi(arg);
		if (real_mobile(number) >= 0)
			OLC_QUEST(d)->mob_vnum = number;
		else
			OLC_QUEST(d)->mob_vnum = -1;
		break;
		
	case QEDIT_TYPE:
		number = atoi(arg);
		if (number < 0 || number >= NUM_AQ_TYPES) {
			send_to_char("Invalid choice!", d->character);
			qedit_disp_type_menu(d);
			return;
		} else
			OLC_QUEST(d)->type = number;
		break;
		
	case QEDIT_FLAGS:
		number = atoi(arg);
		if ((number < 0) || (number > NUM_AQ_FLAGS)) {
			send_to_char("That's not a valid choice!\r\n", d->character);
			qedit_disp_flags_menu(d);
		} else {
			if (number == 0)
				break;
			else {
				/* toggle bits */
				if (IS_SET(OLC_QUEST(d)->flags, 1ULL << (number - 1)))
					REMOVE_BIT(OLC_QUEST(d)->flags, 1ULL << (number - 1));
				else
					SET_BIT(OLC_QUEST(d)->flags, 1ULL << (number - 1));
				qedit_disp_flags_menu(d);
			}
		}
		return;
		
	case QEDIT_TARGET:
		number = atoi(arg);
		OLC_QUEST(d)->target = MAX(0, number);
		break;

	case QEDIT_EXP:
		number = atoi(arg);
		OLC_QUEST(d)->exp = MAX(0, MIN(number, 100000));
		break;
		
	case QEDIT_NEXT:
		number = atoi(arg);
		if (real_quest(number) >= 0)
			OLC_QUEST(d)->next_quest = number;
		else
			OLC_QUEST(d)->next_quest = -1;
		break;
		
	case QEDIT_VALUE_0:
		number = atoi(arg);
		switch (OLC_QUEST(d)->type) {
			case AQ_RETURN_OBJ:
				if (real_mobile(number) < 0)
					number = 0;
				break;
		}
		OLC_QUEST(d)->value[0] = number;
		qedit_disp_val1_menu(d);
		return;
		
	case QEDIT_VALUE_1:
		OLC_QUEST(d)->value[1] = atoi(arg);
		qedit_disp_val2_menu(d);
		return;  
		
	case QEDIT_VALUE_2:
		OLC_QUEST(d)->value[2] = atoi(arg);
		qedit_disp_val3_menu(d);
		return;  
		
	case QEDIT_VALUE_3:
		OLC_QUEST(d)->value[3] = atoi(arg);
		break;    
		
	default:
		/* we should never get here */
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "Reached default case in parse_qedit");
		break;
	}
	/*. If we get this far, something has be changed .*/
	OLC_VAL(d) = 1;
	qedit_disp_menu(d);
}

void qedit_string_cleanup(struct descriptor_data *d, int terminator)
{
	switch (OLC_MODE(d)) {
	case QEDIT_INFO:
		qedit_disp_menu(d);
		break;
	case QEDIT_ENDING:
		qedit_disp_menu(d);
		break;
	}
}
