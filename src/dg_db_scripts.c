/* ************************************************************************
*	 File: db.script.c                             Part of Death's Gate MUD *
*																																					*
*	 Usage: Contains routines to handle db functions for scripts and trigs  *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: dg_db_scripts.c,v 1.13 2002/11/07 11:13:28 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "spells.h"
#include "dg_scripts.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "events.h"
#include "comm.h"
#include "genolc.h"

/* local functions */
void trig_data_copy(trig_data *this_data, const trig_data *trg);
void trig_data_free(trig_data *this_data);
void free_varlist(struct trig_var_data *vd);
void free_script(struct script_data *sc);
void trig_data_init(trig_data *this_data);

/* external functions */
extern struct index_data **trig_index;
extern int top_of_trigt;
extern struct trig_data *trigger_list;

extern struct index_data *mob_index;
extern struct index_data *obj_index;

extern void half_chop(char *string, char *arg1, char *arg2);
extern bitvector_t asciiflag_conv(char *flag);


void parse_trigger(MYSQL_ROW trigger_row, unsigned long *fieldlength, int vnum)
{
	struct cmdlist_element *cle;
  char *cmds, *s;
	struct index_data *t_index;
	struct trig_data *trig;

	CREATE(trig, trig_data, 1);
	CREATE(t_index, index_data, 1);

	t_index->vnum = atoi(trigger_row[1]);
	t_index->number = 0;
	t_index->func = NULL;
	t_index->proto = trig;

	sprintf(buf2, "trig vnum %d", atoi(trigger_row[1]));

	trig->nr = top_of_trigt;
	trig->name = str_dup(trigger_row[2]);

	trig->attach_type = (byte)atoi(trigger_row[3]);
	trig->trigger_type = (long)asciiflag_conv(trigger_row[4]);
	trig->narg = atoi(trigger_row[5]);

	trig->arglist = str_dup(trigger_row[6]);

	cmds = s = str_dup(trigger_row[7]);

	CREATE(trig->cmdlist, struct cmdlist_element, 1);
	trig->cmdlist->cmd = str_dup(strtok(s, "\n\r"));
	cle = trig->cmdlist;

	while ((s = strtok(NULL, "\n\r"))) {
		CREATE(cle->next, struct cmdlist_element, 1);
		cle = cle->next;
		cle->cmd = str_dup(s);
	}

	free(cmds);

	trig_index[top_of_trigt++] = t_index;
}


/*
 * create a new trigger from a prototype.
 * nr is the real number of the trigger.
 */
trig_data	*read_trigger(int nr)
{
	index_data *t_index;
	trig_data *trig;

	if (nr >= top_of_trigt) return NULL;
	if ((t_index = trig_index[nr]) == NULL)
		return NULL;

	CREATE(trig, trig_data, 1);
	trig_data_copy(trig, t_index->proto);

	t_index->number++;

	return trig;
}


/* release memory allocated for a variable list */
void free_varlist(struct trig_var_data *vd)
{
	struct trig_var_data *i, *j;

	for (i = vd; i;) {
		j = i;
		i = i->next;
		if (j->name)
			free(j->name);
		if (j->value)
			free(j->value);
		free(j);
	}
}


/* release memory allocated for a script */
void free_script(struct script_data *sc)
{
	trig_data *t1, *t2;

	for (t1 = TRIGGERS(sc); t1 ;) {
		t2 = t1;
		t1 = t1->next;
		trig_data_free(t2);
	}

	free_varlist(sc->global_vars);

	free(sc);
}


void trig_data_init(trig_data *this_data)
{
	this_data->nr = NOTHING;
	this_data->data_type = 0;
	this_data->name = NULL;
	this_data->trigger_type = 0;
	this_data->cmdlist = NULL;
	this_data->curr_state = NULL;
	this_data->narg = 0;
	this_data->arglist = NULL;
	this_data->depth = 0;
	this_data->wait_event = NULL;
	this_data->purged = FALSE;
	this_data->var_list = NULL;

	this_data->next = NULL;  
}


void trig_data_copy(trig_data *this_data, const trig_data *trg)
{
	trig_data_init(this_data);

	this_data->nr = trg->nr;
	this_data->attach_type = trg->attach_type;
	this_data->data_type = trg->data_type;
	this_data->name = str_dup(trg->name);
	this_data->trigger_type = trg->trigger_type;
	this_data->cmdlist = trg->cmdlist;
	this_data->narg = trg->narg;
	if (trg->arglist) this_data->arglist = str_dup(trg->arglist);
}


void trig_data_free(trig_data *this_data)
{
	free(this_data->name);
		
	if (*this_data->arglist != '\0');
		free(this_data->arglist);

	free_varlist(this_data->var_list);
	
	if (this_data->wait_event)
		event_cancel(this_data->wait_event);
	
	free(this_data);
}


/* for mobs and rooms: */
void dg_read_trigger(MYSQL_ROW triggers_row, void *proto, int type)
{
	int vnum, rnum;
	char_data *mob;
	room_data *room;
	struct trig_proto_list *trg_proto, *new_trg;

	vnum = atoi(triggers_row[3]);
	rnum = real_trigger(vnum);

	if (rnum < 0) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Trigger vnum #%d asked for but non-existant!", vnum);
		return;
	}

	switch(type) {
		case MOB_TRIGGER:
			CREATE(new_trg, struct trig_proto_list, 1);
			new_trg->vnum = vnum;
			new_trg->next = NULL;

			mob = (char_data *)proto;
			trg_proto = mob->proto_script;
			if (!trg_proto) {
				mob->proto_script = trg_proto = new_trg;
			} else {
				while (trg_proto->next) trg_proto = trg_proto->next;
				trg_proto->next = new_trg;
			}
			break;
		case WLD_TRIGGER:
			CREATE(new_trg, struct trig_proto_list, 1);
			new_trg->vnum = vnum;
			new_trg->next = NULL;
			room = (room_data *)proto;
			trg_proto = room->proto_script;
			if (!trg_proto) {
				room->proto_script = trg_proto = new_trg;
			} else {
				while (trg_proto->next) trg_proto = trg_proto->next;
				trg_proto->next = new_trg;
			}

			if (rnum>=0) {
				if (!(room->script))
					CREATE(room->script, struct script_data, 1);
				add_trigger(SCRIPT(room), read_trigger(rnum), -1);
			} else {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "non-existant trigger #%d assigned to room #%d", vnum, room->number);
			}
			break;
		default:
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Trigger vnum #%d assigned to non-mob/obj/room", vnum);
	}
}


void dg_obj_trigger(MYSQL_ROW triggers_row, struct obj_data *obj)
{
	int vnum, rnum;
	struct trig_proto_list *trg_proto, *new_trg;

	vnum = atoi(triggers_row[3]);
	rnum = real_trigger(vnum);

	if (rnum < 0) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Trigger vnum #%d asked for but non-existant!", vnum);
		return;
	}

	CREATE(new_trg, struct trig_proto_list, 1);
	new_trg->vnum = vnum;
	new_trg->next = NULL;

	trg_proto = obj->proto_script;
	if (!trg_proto) {
		obj->proto_script = trg_proto = new_trg;
	} else {
		while (trg_proto->next) trg_proto = trg_proto->next;
		trg_proto->next = new_trg;
	}
}


void assign_triggers(void *i, int type)
{
	char_data *mob;
	obj_data *obj;
	struct room_data *room;
	int rnum;
	struct trig_proto_list *trg_proto;

	switch (type)
	{
		case MOB_TRIGGER:
			mob = (char_data *)i;
			trg_proto = mob->proto_script;
			while (trg_proto) {
				rnum = real_trigger(trg_proto->vnum);
				if (rnum==-1) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "trigger #%d non-existant, for mob #%d",
						trg_proto->vnum, mob_index[mob->nr].vnum);
				} else {
					if (!SCRIPT(mob))
						CREATE(SCRIPT(mob), struct script_data, 1);
					add_trigger(SCRIPT(mob), read_trigger(rnum), -1);
				}
				trg_proto = trg_proto->next;
			}
			break;
		case OBJ_TRIGGER:
			obj = (obj_data *)i;
			trg_proto = obj->proto_script;
			while (trg_proto) {
				rnum = real_trigger(trg_proto->vnum);
				if (rnum==-1) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "trigger #%d non-existant, for obj #%d",
						trg_proto->vnum, obj_index[obj->item_number].vnum);
				} else {
					if (!SCRIPT(obj))
						CREATE(SCRIPT(obj), struct script_data, 1);
					add_trigger(SCRIPT(obj), read_trigger(rnum), -1);
				}
				trg_proto = trg_proto->next;
			}
			break;
		case WLD_TRIGGER:
			room = (struct room_data *)i;
			trg_proto = room->proto_script;
			while (trg_proto) {
				rnum = real_trigger(trg_proto->vnum);
				if (rnum==-1) {
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "trigger #%d non-existant, for room #%d",
						trg_proto->vnum, room->number);
				} else {
					if (!SCRIPT(room))
						CREATE(SCRIPT(room), struct script_data, 1);
					add_trigger(SCRIPT(room), read_trigger(rnum), -1);
				}
				trg_proto = trg_proto->next;
			}
			break;
		default:
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "unknown type for assign_triggers()");
			break;
	}
}
