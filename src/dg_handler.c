/* $Id: dg_handler.c,v 1.7 2002/11/06 19:29:41 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"
		

#include "structs.h"
#include "buffer.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "events.h"
#include "genolc.h"

/* external variables */
extern struct index_data **trig_index;
extern struct trig_data *trigger_list;

/* local functions */
void trig_data_free(trig_data *this_data);
void free_trigger(struct trig_data *trig);
void extract_trigger(struct trig_data *trig);
char *skill_percent(struct char_data *ch, char *skill);


/* return memory used by a trigger */
void free_trigger(struct trig_data *trig)
{
	/* threw this in for minor consistance in names with the rest of circle */
	trig_data_free(trig);
}


/* remove a single trigger from a mob/obj/room */
void extract_trigger(struct trig_data *trig)
{
	struct trig_data *temp;

	if (GET_TRIG_WAIT(trig)) {
		event_cancel(GET_TRIG_WAIT(trig));
		GET_TRIG_WAIT(trig) = NULL;
	}

	trig_index[trig->nr]->number--; 

	/* walk the trigger list and remove this one */
	REMOVE_FROM_LIST(trig, trigger_list, next_in_world);

	free_trigger(trig);
}

/* remove all triggers from a mob/obj/room */
void extract_script(struct script_data *sc)
{
	struct trig_data *trig, *next_trig;

	for (trig = TRIGGERS(sc); trig; trig = next_trig) {
		next_trig = trig->next;
		extract_trigger(trig);
	}
	TRIGGERS(sc) = NULL;
}

/* erase the script memory of a mob */
void extract_script_mem(struct script_memory *sc)
{
	struct script_memory *next;
	while (sc) {
		next = sc->next;
		if (sc->cmd) free(sc->cmd);
		free(sc);
		sc = next;
	}
}

/* perhaps not the best place for this, but I didn't want a new file */
char *skill_percent(struct char_data *ch, char *skill)
{
	static char retval[16];
	int skillnum;

	skillnum = find_skill_num(skill);
	if (skillnum<=0) return("unknown skill");

	sprintf(retval,"%ld",GET_SKILL(ch, skillnum));
	return retval;
}

/* Remove all scripts/triggers from a mob/obj/room, then
   unconditionally free memory allocated. 
   
   Used in extract_char() and free_mobile()
*/
void free_proto_script(void *thing, int type)
{
	struct trig_proto_list *proto = NULL, *fproto;
	char_data *mob;
	obj_data *obj;
	room_data *room;

	switch (type) {
	case MOB_TRIGGER:
		mob = (struct char_data *)thing;
		if (SCRIPT(mob))
			extract_script(SCRIPT(mob));
		SCRIPT(mob) = NULL;
		proto = mob->proto_script;
		mob->proto_script = NULL;
		break;
	case OBJ_TRIGGER:
		obj = (struct obj_data *)thing;
		if (SCRIPT(obj))
			extract_script(SCRIPT(obj));
		SCRIPT(obj) = NULL;
		proto = obj->proto_script;
		obj->proto_script = NULL;
		break;
	case WLD_TRIGGER:
		room = (struct room_data *)thing;
		if (SCRIPT(room))
			extract_script(SCRIPT(room));
		SCRIPT(room) = NULL;
		proto = room->proto_script;
		room->proto_script = NULL;
		break;
	}
	while (proto) {
		fproto = proto;
		proto = proto->next;
		free(fproto);
	}
}  
