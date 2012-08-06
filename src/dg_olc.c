/* ************************************************************************
*  dg_olc.c: this source file is used in extending Oasis style OLC for    *
*  dg-scripts onto a CircleMUD that already has dg-scripts (as released   *
*  by Mark Heilpern on 1/1/98) implemented.                               *
*                                                                         *
*  Parts of this file by Chris Jacobson of _Aliens vs Predator: The MUD_  *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001, Torgny Bjers.                                       *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
************************************************************************ */
/* $Id: dg_olc.c,v 1.24 2002/11/06 19:29:42 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "spells.h"
#include "dg_olc.h"
#include "events.h"
#include "oasis.h"
#include "genolc.h"
#include "constants.h"

/* declare externally defined globals */
extern struct index_data **trig_index;
extern struct descriptor_data *descriptor_list;
extern struct trig_data *trigger_list;
extern int top_of_trigt;
extern struct zone_data *zone_table;
int sprintascii(char *out, bitvector_t bits);

/* prototype externally defined functions */
void trig_data_copy(trig_data *this_data, const trig_data *trg);
void free_varlist(struct trig_var_data *vd);

void trigedit_disp_menu(struct descriptor_data *d);
void trigedit_save(struct descriptor_data *d);
void trigedit_disp_types(struct descriptor_data *d);

/* copy an entire script from one holder (mob/obj/room) to another */
void script_copy(void *dst, void *src, int type)
{
	struct script_data *s_src = NULL;
	struct script_data *s_dst = NULL;
	trig_data *t_src, *t_dst;

	/* find the scripts of the source and destination */
	switch (type) {
	case MOB_TRIGGER:
		s_src = SCRIPT((struct char_data *)src);
		s_dst = SCRIPT((struct char_data *)dst);
		((struct char_data *)dst)->proto_script =
			((struct char_data *)src)->proto_script;
	 break;
	case OBJ_TRIGGER:
		s_src = SCRIPT((struct obj_data *)src);
		s_dst = SCRIPT((struct obj_data *)dst);
		((struct obj_data *)dst)->proto_script =
			((struct obj_data *)src)->proto_script;
		break;
	case WLD_TRIGGER:
		s_src = SCRIPT((struct room_data *)src);
		s_dst = SCRIPT((struct room_data *)dst);
		((struct room_data *)dst)->proto_script =
			((struct room_data *)src)->proto_script;
		break;
	default:
		mlog("SYSERR: Unknown type code sent to script_copy()!");
		break;
	}

	/*
	 * make sure the dst doesnt already have a script
	 * if it does, delete it
	 */
	if (s_dst) extract_script(s_dst);

	/* copy the scrip data */
	s_dst->types = s_src->types;
	t_src = TRIGGERS(s_src);
	while (t_src) {
		CREATE(t_dst, trig_data, 1);
		if (!TRIGGERS(s_dst)) TRIGGERS(s_dst) = t_dst;
		trig_data_copy(t_dst, t_src);
		t_dst = t_dst->next;
		t_src = t_src->next;
	}

}


/* 
 * called when a mob or object is being saved to disk, so its script can
 * be saved
 */
void script_save_to_disk(int id, int znum, void *item, int type)
{
	struct trig_proto_list *t;

	if (type==MOB_TRIGGER)
		t = ((struct char_data *)item)->proto_script;
	else if (type==OBJ_TRIGGER)
		t = ((struct obj_data *)item)->proto_script;
	else if (type==WLD_TRIGGER)
		t = ((struct room_data *)item)->proto_script;
	else {
		mlog("SYSERR: (%s)%s:%d: Invalid type passed", __FILE__, __FUNCTION__, __LINE__);
		return;
	}

	mysqlWrite("DELETE FROM %s WHERE owner = %d AND type = %d;", TABLE_TRG_ASSIGNS, id, type);

	while (t) {
		mysqlWrite("INSERT INTO %s (owner, znum, vnum, type) VALUES(%d, %d, %d, %d);", TABLE_TRG_ASSIGNS, id, znum, t->vnum, type);
		t = t->next;
	}
}


void trigedit_setup_new(struct descriptor_data *d)
{
	struct trig_data *trig;
	
	/*
	 * Allocate a scratch trigger structure
	 */
	CREATE(trig, struct trig_data, 1);

	trig->nr = -1;

	/*
	 * Set up some defaults
	 */ 
	trig->name = str_dup("new trigger");
	trig->trigger_type = MTRIG_GREET;

	/* cmdlist will be a large char string until the trigger is saved */
	CREATE(OLC_STORAGE(d), char, MAX_CMD_LENGTH);
	strcpy(OLC_STORAGE(d),
		"say My trigger commandlist is not complete!\r\n");
	trig->narg = 100;

	OLC_TRIG(d) = trig;
	OLC_VAL(d) = 0;  /* Has changed flag. (It hasn't so far, we just made it.) */
		
	trigedit_disp_menu(d);
}


void trigedit_setup_existing(struct descriptor_data *d, int rtrg_num)
{
	struct trig_data *trig;
	struct cmdlist_element *c;

	/*
	 * Allocate a scratch trigger structure
	 */
	CREATE(trig, struct trig_data, 1);

	trig_data_copy(trig, trig_index[rtrg_num]->proto);

	/* convert cmdlist to a char string */
	c = trig->cmdlist;
	CREATE(OLC_STORAGE(d), char, MAX_CMD_LENGTH);
	strcpy(OLC_STORAGE(d), "");
	
	while (c) {
		strcat(OLC_STORAGE(d), c->cmd);
		strcat(OLC_STORAGE(d), "\r\n");
		c = c->next;
	}
	/*
	 * now trig->cmdlist is something to pass to the text editor
	 * it will be converted back to a real cmdlist_element list later
	 */
	OLC_TRIG(d) = trig;
	OLC_VAL(d) = 0;  /* Has changed flag. (It hasn't so far, we just made it.) */
		
	trigedit_disp_menu(d);
}


void trigedit_disp_menu(struct descriptor_data *d)
{
	struct trig_data *trig = OLC_TRIG(d);
	char *attach_type;
	char trgtypes[256];

	get_char_colors(d->character);
	clear_screen(d);

	if (trig->attach_type==OBJ_TRIGGER) {
		attach_type = "Objects";
		sprintbit(GET_TRIG_TYPE(trig), otrig_types, trgtypes, sizeof(trgtypes));
	} else if (trig->attach_type==WLD_TRIGGER) {
		attach_type = "Rooms";
		sprintbit(GET_TRIG_TYPE(trig), wtrig_types, trgtypes, sizeof(trgtypes));
	} else {
		attach_type = "Mobiles";
		sprintbit(GET_TRIG_TYPE(trig), trig_types, trgtypes, sizeof(trgtypes));
	}
			
	write_to_output(d, TRUE,
#if	defined(CLEAR_SCREEN)
		"^[[H^[[J"
#endif
		"Trigger Editor [%s%d%s]\r\n\r\n"
		"%s1)%s Name         : %s%s\r\n"
		"%s2)%s Intended for : %s%s\r\n"
		"%s3)%s Trigger types: %s%s\r\n"
		"%s4)%s Numberic Arg : %s%d\r\n"
		"%s5)%s Arguments    : %s%s\r\n"
		"%s6)%s Commands:\r\n%s",	
		grn, OLC_NUM(d), nrm,														/* vnum on the title line */
		grn, nrm, yel, GET_TRIG_NAME(trig),							/* name                   */
		grn, nrm, yel, attach_type,											/* attach type            */
		grn, nrm, yel, trgtypes,												/* greet/drop/etc         */
		grn, nrm, yel, trig->narg,											/* numeric arg            */
		grn, nrm, yel, trig->arglist?trig->arglist:"",	/* strict arg             */
		grn, nrm, cyn
	);
		
	write_to_output(d, FALSE,
		"%s\r\n",
		OLC_STORAGE(d)																	/* the command list       */
	);

	write_to_output(d, TRUE,
		"%sQ)%s Quit\r\n"
		"Enter Choice :",
		grn, nrm																				/* quit colors            */
	);

	OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
}


void trigedit_disp_types(struct descriptor_data *d)
{
	int i, columns = 0;
	const char **types;

	switch(OLC_TRIG(d)->attach_type) {
	case WLD_TRIGGER:
		types = wtrig_types;
		break;
	case OBJ_TRIGGER:
		types = otrig_types;
		break;
	case MOB_TRIGGER:
	default:
		types = trig_types;
		break;
	}

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i < NUM_TRIG_TYPE_FLAGS; i++) {
	sprintf(buf, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, types[i],
							!(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintbit(GET_TRIG_TYPE(OLC_TRIG(d)), types, buf1, sizeof(buf1));
	sprintf(buf, "\r\nCurrent types : %s%s%s\r\nEnter type (0 to quit) : ",
								cyn, buf1, nrm);
	send_to_char(buf, d->character);

}

void trigedit_parse(struct descriptor_data *d, char *arg)
{
	int i = 0;
	char *backstr = NULL;

	switch (OLC_MODE(d)) {
	case TRIGEDIT_MAIN_MENU:
	 switch (tolower(*arg)) {
	 case 'q':
		 if (OLC_VAL(d)) { /* Anything been changed? */
			 if (!GET_TRIG_TYPE(OLC_TRIG(d))) {
				 send_to_char("Invalid Trigger Type! Answer a to abort quit!\r\n",
					 d->character);     
			 }
			 send_to_char("Do you wish to save the changes to the trigger? (y/n): ",
				 d->character);     
			 OLC_MODE(d) = TRIGEDIT_CONFIRM_SAVESTRING;
		 } else
			 cleanup_olc(d, CLEANUP_ALL);
		 return;
	 case '1':
		 OLC_MODE(d) = TRIGEDIT_NAME;
		 send_to_char("Name: ", d->character);
		 break;
	 case '2':
		 OLC_MODE(d) = TRIGEDIT_INTENDED;
		 send_to_char("0: Mobiles, 1: Objects, 2: Rooms: ", d->character);
		 break;
	 case '3':
		 OLC_MODE(d) = TRIGEDIT_TYPES;
		 trigedit_disp_types(d);
		 break;
	 case '4':
		 OLC_MODE(d) = TRIGEDIT_NARG;
		 send_to_char("Numeric argument: ", d->character);
		 break;
	 case '5':
		 OLC_MODE(d) = TRIGEDIT_ARGUMENT;
		 send_to_char("Argument: ", d->character);
		 break;
	 case '6':
		 OLC_MODE(d) = TRIGEDIT_COMMANDS;

		 clear_screen(d);
		 if (OLC_STORAGE(d)) {
			 write_to_output(d, FALSE, "%s", OLC_STORAGE(d));
			 backstr = str_dup(OLC_STORAGE(d));
		 }
		 string_write(d, &OLC_STORAGE(d), MAX_CMD_LENGTH, 0, STATE(d));
		 OLC_VAL(d) = 1;

		 break;
	 default:
		 trigedit_disp_menu(d);
		 return;
	 }
	 return;
	
	case TRIGEDIT_CONFIRM_SAVESTRING:
		switch(tolower(*arg)) {
		case 'y':
			trigedit_save(d);
			sprintf(buf, "OLC: %s edits trigger %d", GET_NAME(d->character),
				OLC_NUM(d));
			mudlog(buf, CMP, MAX(RIGHTS_TRIGGERS, GET_INVIS_LEV(d->character)), TRUE);
			/* fall through */
		case 'n':
			cleanup_olc(d, CLEANUP_ALL);
			return;
		case 'a': /* abort quitting */
			break;
		default:
			send_to_char("Invalid choice!\r\n", d->character);
			send_to_char("Do you wish to save the trigger? : ", d->character);
			return;
		}
		break;

	case TRIGEDIT_NAME:
		if (OLC_TRIG(d)->name)
			free(OLC_TRIG(d)->name);
		OLC_TRIG(d)->name = str_dup((arg && *arg) ? arg : "undefined");
		OLC_VAL(d)++;
		break;

	case TRIGEDIT_INTENDED:
		if ((atoi(arg)>=MOB_TRIGGER) || (atoi(arg)<=WLD_TRIGGER))
			OLC_TRIG(d)->attach_type = atoi(arg);
		OLC_VAL(d)++;
		break;

	case TRIGEDIT_NARG:
		OLC_TRIG(d)->narg = atoi(arg);
		OLC_VAL(d)++;
		break;

	case TRIGEDIT_ARGUMENT:
		OLC_TRIG(d)->arglist = (*arg?str_dup(arg):NULL);
		OLC_VAL(d)++;
		break;

	case TRIGEDIT_TYPES:
		if ((i = atoi(arg)) == 0)
			break;
		else if (!((i < 0) || (i > NUM_TRIG_TYPE_FLAGS)))
			TOGGLE_BIT((GET_TRIG_TYPE(OLC_TRIG(d))), 1ULL << (i - 1));
		OLC_VAL(d)++;
		trigedit_disp_types(d);
		return;

	case TRIGEDIT_COMMANDS:
		break;

	}

	OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
	trigedit_disp_menu(d);
}


/* save the zone's triggers to internal memory and to disk */
void trigedit_save(struct descriptor_data *d)
{
	char *name=NULL;
	char *trgarglist=NULL;
	char *code=NULL;
	int trig_rnum, i;
	int found = 0;
	char *s;
	trig_data *proto;
	trig_data *trig = OLC_TRIG(d);
	trig_data *live_trig;
	struct cmdlist_element *cmd, *next_cmd;
	struct index_data **new_index;
	struct descriptor_data *dsc;
	int zone, top;
	char buf[MAX_CMD_LENGTH];
	char bitBuf[MAX_INPUT_LENGTH];

	char *trg_replace = "REPLACE INTO %s ("
		"znum, "
		"vnum, "
		"name, "
		"attach_type, "
		"flags, "
		"numarg, "
		"arglist, "
		"code) "
		"VALUES (%d, %d, '%s', %d, '%s', %d, '%s', '%s');";

	if ((trig_rnum = real_trigger(OLC_NUM(d))) != -1) {
		proto = trig_index[trig_rnum]->proto;
		for (cmd = proto->cmdlist; cmd; cmd = next_cmd) { 
			next_cmd = cmd->next;
			if (cmd->cmd)
				free(cmd->cmd);
			free(cmd);
		}

		if (proto->arglist)
			free(proto->arglist);
		if (proto->name)
			free(proto->name);

		/* Recompile the command list from the new script */
		s = OLC_STORAGE(d);

		CREATE(trig->cmdlist, struct cmdlist_element, 1);
		trig->cmdlist->cmd = str_dup(strtok(s, "\n\r"));
		cmd = trig->cmdlist;

		while ((s = strtok(NULL, "\n\r"))) {
			CREATE(cmd->next, struct cmdlist_element, 1);
			cmd = cmd->next;
			cmd->cmd = str_dup(s);
		}

		/* make the prorotype look like what we have */
		trig_data_copy(proto, trig);

		/* go through the mud and replace existing triggers         */
		live_trig = trigger_list;
		while (live_trig)
		{
			if (GET_TRIG_RNUM(live_trig) == trig_rnum) {
				if (live_trig->arglist) {
					free(live_trig->arglist);
					live_trig->arglist = NULL;
				}
				if (live_trig->name) {
					free(live_trig->name);
					live_trig->name = NULL;
				}

				if (proto->arglist)
					live_trig->arglist = str_dup(proto->arglist);
				if (proto->name)
					live_trig->name = str_dup(proto->name);

				live_trig->cmdlist = proto->cmdlist;
				live_trig->curr_state = live_trig->cmdlist;
				live_trig->trigger_type = proto->trigger_type;
				live_trig->attach_type = proto->attach_type;
				live_trig->narg = proto->narg;
				live_trig->data_type = proto->data_type;
				live_trig->depth = 0;
				live_trig->wait_event = NULL;
				if (GET_TRIG_WAIT(live_trig))
					event_cancel(GET_TRIG_WAIT(live_trig));
				free_varlist(live_trig->var_list);
			}

			live_trig = live_trig->next_in_world;
		}
	} else {
		/* this is a new trigger */
		CREATE(new_index, struct index_data *, top_of_trigt + 2);

		/* Recompile the command list from the new script */
		s = OLC_STORAGE(d);
				 
		CREATE(trig->cmdlist, struct cmdlist_element, 1);
		trig->cmdlist->cmd = str_dup(strtok(s, "\n\r"));
		cmd = trig->cmdlist;
																
		while ((s = strtok(NULL, "\n\r"))) {
			CREATE(cmd->next, struct cmdlist_element, 1);
			cmd = cmd->next;
			cmd->cmd = str_dup(s);
		}

		for (i = 0; i < top_of_trigt; i++) {
			if (!found) {
				if (trig_index[i]->vnum > OLC_NUM(d)) {
					found = TRUE;
					trig_rnum = i;
												
					CREATE(new_index[trig_rnum], struct index_data, 1);
					GET_TRIG_RNUM(OLC_TRIG(d)) = trig_rnum;
					new_index[trig_rnum]->vnum = OLC_NUM(d);
					new_index[trig_rnum]->number = 0; 
					new_index[trig_rnum]->func = NULL;
					CREATE(proto, struct trig_data, 1);
					new_index[trig_rnum]->proto = proto;
					trig_data_copy(proto, trig);

					if (trig->name)
						proto->name = str_dup(trig->name);
					if (trig->arglist)
						proto->arglist = str_dup(trig->arglist);  

					new_index[trig_rnum + 1] = trig_index[trig_rnum];

					proto = trig_index[trig_rnum]->proto;
					proto->nr = trig_rnum + 1;
				} else {
					new_index[i] = trig_index[i];
				}
			} else {
				 new_index[i + 1] = trig_index[i];
				 proto = trig_index[i]->proto;
				 proto->nr = i + 1;
			}
		}

		if (!found) {
			trig_rnum = i;
			CREATE(new_index[trig_rnum], struct index_data, 1);
			GET_TRIG_RNUM(OLC_TRIG(d)) = trig_rnum;  
			new_index[trig_rnum]->vnum = OLC_NUM(d);
			new_index[trig_rnum]->number = 0;
			new_index[trig_rnum]->func = NULL;
												
			CREATE(proto, struct trig_data, 1);
			new_index[trig_rnum]->proto = proto;
			trig_data_copy(proto, trig);

			if (trig->name)
				proto->name = str_dup(trig->name);
			if (trig->arglist)
				proto->arglist = str_dup(trig->arglist);  
		}
								
		free(trig_index);
												
		trig_index = new_index;
		top_of_trigt++;         

		/* HERE IT HAS TO GO THROUGH AND FIX ALL SCRIPTS/TRIGS OF HIGHER RNUM */
		for (live_trig = trigger_list; live_trig; live_trig = live_trig->next_in_world)
			if (GET_TRIG_RNUM(live_trig) > trig_rnum)
				GET_TRIG_RNUM(live_trig)++;
				
		/*
		 * Update other trigs being edited.
		 */
		 for (dsc = descriptor_list; dsc; dsc = dsc->next)
			 if (STATE(dsc) == CON_TRIGEDIT)
				 if (GET_TRIG_RNUM(OLC_TRIG(dsc)) >= trig_rnum)
					 GET_TRIG_RNUM(OLC_TRIG(dsc))++;

	}

	/* now write the trigger out to mysql, along with the rest of the */
	/* triggers for this zone, of course                              */
	/* note: we write this to db NOW instead of letting the builder   */
	/* have control because if we lose this after having assigned a   */
	/* new trigger to an item, we will get SYSERR's upton reboot that */
	/* could make things hard to debug.                               */

	zone = zone_table[OLC_ZNUM(d)].number;
	top = zone_table[OLC_ZNUM(d)].top;

	/* Delete all the zone's triggers from the database. */
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_TRG_INDEX, zone);

	for (i = zone_table[OLC_ZNUM(d)].bot; i <= top; i++) {
		if ((trig_rnum = real_trigger(i)) != NOTHING) {
			trig = trig_index[trig_rnum]->proto;

			strcpy(buf, (GET_TRIG_NAME(trig)) ? (GET_TRIG_NAME(trig)) : "unknown trigger");
			SQL_MALLOC(buf, name);
			SQL_ESC(buf, name);

			strcpy(buf, GET_TRIG_ARG(trig) ? GET_TRIG_ARG(trig) : "");
			SQL_MALLOC(buf, trgarglist);
			SQL_ESC(buf, trgarglist);

			/* Build the text for the script */
			strcpy(buf,"");
			for (cmd = trig->cmdlist; cmd; cmd = cmd->next) {
				strcat(buf, cmd->cmd);
				strcat(buf, "\r\n");
			}
			if (!buf[0])
				strcpy(buf, "* Empty script");
			SQL_MALLOC(buf, code);
			SQL_ESC(buf, code);

			sprintascii(bitBuf, GET_TRIG_TYPE(trig));

			if (!(mysqlWrite(
				trg_replace,
				TABLE_TRG_INDEX,
				zone,
				i,
				name,
				trig->attach_type,
				bitBuf,
				GET_TRIG_NARG(trig),
				trgarglist,
				code
			))) {
				extended_mudlog(BRF, SYSL_PLAYERS, TRUE, "Error writing triggers to database.");
				return;
			}

			SQL_FREE(name);
			SQL_FREE(trgarglist);
			SQL_FREE(code);

			*buf = '\0';
		}
	}
				
	send_to_char("Saving Index file\r\n", d->character);
}


void dg_olc_script_free(struct descriptor_data *d)
{
	struct trig_proto_list *editscript, *prevscript;

	editscript = OLC_SCRIPT(d);
	while (editscript) {
		prevscript = editscript;
		editscript = editscript->next;
		free(prevscript);
	}
}


void dg_olc_script_copy(struct descriptor_data *d)
{
	struct trig_proto_list *origscript, *editscript;

	if (OLC_ITEM_TYPE(d)==MOB_TRIGGER)
		origscript = OLC_MOB(d)->proto_script;
	else if (OLC_ITEM_TYPE(d)==OBJ_TRIGGER)
		origscript = OLC_OBJ(d)->proto_script;
	else origscript = OLC_ROOM(d)->proto_script;

	if (origscript) {
		CREATE(editscript, struct trig_proto_list, 1);
		OLC_SCRIPT(d) = editscript;

		while (origscript) {
			editscript->vnum = origscript->vnum;
			origscript = origscript->next;
			if (origscript)
				CREATE(editscript->next, struct trig_proto_list, 1);
			editscript = editscript->next;
		}
	} else
			OLC_SCRIPT(d) = NULL;
}


void dg_script_menu(struct descriptor_data *d)
{
	struct trig_proto_list *editscript;
	int i = 0;

	/* make sure our input parser gets used */
	OLC_MODE(d) = OLC_SCRIPT_EDIT;
	OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;

	get_char_colors(d->character);
	clear_screen(d);

	send_to_char("     Script Editor\r\n\r\n     Trigger List:\r\n", d->character);

	editscript = OLC_SCRIPT(d);
	while (editscript) {
		sprintf(buf,"     %2d) [%s%d%s] %s%s%s", ++i, cyn, 
			editscript->vnum, nrm, cyn,
			trig_index[real_trigger(editscript->vnum)]->proto->name, nrm);
		send_to_char(buf, d->character);
		if (trig_index[real_trigger(editscript->vnum)]->proto->attach_type != OLC_ITEM_TYPE(d))
			sprintf(buf,"   %s** Mis-matched Trigger Type **%s\r\n", grn, nrm);
		else
			sprintf(buf,"\r\n");
		send_to_char(buf, d->character);

		editscript = editscript->next;
	}
	if (i==0) send_to_char("     <none>\r\n", d->character);

	sprintf(buf, "\r\n"
		" %sN%s)  New trigger for this script\r\n"
		" %sD%s)  Delete a trigger in this script\r\n"
		" %sX%s)  Exit Script Editor\r\n\r\n"
		"     Enter choice :",
		grn, nrm, grn, nrm, grn, nrm);
	send_to_char(buf,d->character);
}


int	dg_script_edit_parse(struct descriptor_data *d, char *arg)
{
	struct trig_proto_list *trig, *currtrig;
	int count, pos, vnum;

	switch(OLC_SCRIPT_EDIT_MODE(d)) {
		case SCRIPT_MAIN_MENU:
			switch(tolower(*arg)) {
				case 'x':
					/* this was buggy.
						 First we created a copy of a thing, but maintained pointers to scripts,
						 then if we altered the scripts, we freed the pointers and added new ones
						 to the OLC_THING. If we then chose _NOT_ to save the changes, the 
						 pointers in the original thing pointed to garbage. If we saved changes
						 the pointers were updated correctly.
						 
						 Solution:
						 Here we just point the working copies to the new proto_scripts
						 We only update the original when choosing to save internally,
						 then free the unused memory there.
						 
						 Thanks to 
						 Jeremy Stanley - fungi@yuggoth.org and
						 Torgny Bjers - artovil@arcanerealms.org
						 for the bug report.
					*/
					if (OLC_ITEM_TYPE(d)==MOB_TRIGGER) {
						OLC_MOB(d)->proto_script = OLC_SCRIPT(d);
					} else if (OLC_ITEM_TYPE(d)==OBJ_TRIGGER) {
						OLC_OBJ(d)->proto_script = OLC_SCRIPT(d);
					} else {
						OLC_ROOM(d)->proto_script = OLC_SCRIPT(d);
					}
					return 0;
				case 'n':
					send_to_char("\r\n(&WPositions: 0: Mobiles, 1: Objects, 2: Rooms&n)\r\nPlease enter position, vnum   (ex: 1, 200):",
						d->character);
					OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_NEW_TRIGGER;
					break;
				case 'd':
					send_to_char("     Which entry should be deleted?  0 to abort :",
						d->character);
					OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_DEL_TRIGGER;
					break;
				default:
					dg_script_menu(d);
					break;
			}
			return 1;

		case SCRIPT_NEW_TRIGGER:
			vnum = -1;
			count = sscanf(arg,"%d, %d",&pos, &vnum);
			if (count==1) {
				vnum = pos;
				pos = 999;
			}

			if (pos<=0) break; /* this aborts a new trigger entry */

			if (vnum==0) break; /* this aborts a new trigger entry */

			if (real_trigger(vnum) == NOTHING) {
				send_to_char("\r\n&RInvalid Trigger VNUM!&n\r\n"
						"Please enter position, vnum   (ex: 1, 200):",
						d->character);
				return 1;
			}

			/* add the new info in position */
			currtrig = OLC_SCRIPT(d);
			CREATE(trig, struct trig_proto_list, 1);
			trig->vnum = vnum;

			if (pos==1 || !currtrig) {
				trig->next = OLC_SCRIPT(d);
				OLC_SCRIPT(d) = trig;
			} else {
				while (currtrig->next && --pos) {
					currtrig = currtrig->next;
				}
				trig->next = currtrig->next;
				currtrig->next = trig;
			}
			OLC_VAL(d)++;
			break;

		case SCRIPT_DEL_TRIGGER:
			pos = atoi(arg);
			if (pos<=0) break;
			
			if (pos==1 && OLC_SCRIPT(d)) {
				OLC_VAL(d)++;
				currtrig = OLC_SCRIPT(d);
				OLC_SCRIPT(d) = currtrig->next;
				free(currtrig);
				break;
			}
			
			pos--;
			currtrig = OLC_SCRIPT(d);
			while (--pos && currtrig) currtrig = currtrig->next;
			/* now curtrig points one before the target */
			if (currtrig && currtrig->next) {
				OLC_VAL(d)++;
				trig = currtrig->next;
				currtrig->next = trig->next;
				free(trig);
			}
			break;
	}

	dg_script_menu(d);
	return 1;      
}


/* xyzabc */
void trigedit_string_cleanup(struct descriptor_data *d, int terminator)
{
	switch (OLC_MODE(d)) {
	case TRIGEDIT_COMMANDS:
		trigedit_disp_menu(d);
		break;
	}
}
