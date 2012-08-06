/* *************************************************************************
*  File: search.c                                        Part of CircleMUD *
* Usage: search tool for imms to find vnums of mobs/objects with the given *
*        search criteria                                                   *
*                                                                          *
* Copyright (c) 2001, Catherine "Cheron" Gore of Arcane Realms             *
************************************************************************* */

#define __SEARCH_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"
#include "events.h"
#include "spells.h"
#include "skills.h"

#define MOB 1
#define OBJ 2

/* extern variables */
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct spell_info_type *spell_info;
extern struct attack_hit_type attack_hit_text[];

/* local functions */
int search_mobile(struct char_data *ch, int mode, int value);
int search_object(struct char_data *ch, int mode, int value);
void list_perm_affects(struct char_data *ch);
void list_spells(struct char_data *ch);
void list_obj_types(struct char_data *ch);
ACMD(do_find);

/* array for stats to search for mobs */
const char *mob_search[] = {
  "difficulty",
  "damroll",
  "hitroll",
  "\n",
};

/* array for stats to search for objects */
const char *obj_search[] = {
  "perm",
  "type",
  "\n",
};


/* main search for mobs */
int search_mobile(struct char_data *ch, int mode, int value)
{
  int nr, found = 0;
  
  switch (mode) {
  case 0:  /* difficulty */
    for (nr = 0; nr <= top_of_mobt; nr++) {
      if (mob_proto[nr].mob_specials.difficulty == value) {
        sprintf(buf, "%3d. [%5d] %s\r\n", ++found, mob_index[nr].vnum,
          mob_proto[nr].player.short_descr);
        send_to_char(buf, ch);
      }
    }
    break;
  case 1:  /* damroll */
    for (nr = 0; nr <= top_of_mobt; nr++) {
      if (mob_proto[nr].points.damroll >= value) {
        sprintf(buf, "%3d. [%5d] %s\r\n", ++found, mob_index[nr].vnum,
          mob_proto[nr].player.short_descr);
        send_to_char(buf, ch);
      }
    }
    break;
  case 2:  /* hitroll */
    for (nr = 0; nr <= top_of_mobt; nr++) {
      if (mob_proto[nr].points.hitroll >= value) {
        sprintf(buf, "%3d. [%5d] %s\r\n", ++found, mob_index[nr].vnum,
          mob_proto[nr].player.short_descr);
        send_to_char(buf, ch);
      }
    }
    break;
  default:
    send_to_char("Can't search on that!\r\n", ch);
    return (0);
  }
  
  return (found);
}


/* some funcs to display the list of available perm affects, spells, and types - used with the 
 * arrays found in constants.c
 */

/* permanent affects */
void list_perm_affects(struct char_data *ch)
{
  int counter, columns = 0;
  
  for (counter = 0; counter < NUM_AFF_FLAGS; counter++) {
    sprintf(buf, "%d) %-20.20s %s", counter, affected_bits[counter], !(++columns % 2) ? "\r\n" : "");
    send_to_char(buf, ch);
  }
}

/* spells are currently not implemented in this search */
void list_spells(struct char_data *ch)
{
  int counter = 1, columns = 0;
  struct spell_info_type *sptr;

	for (sptr = spell_info; sptr; sptr = sptr->next) {
		if (sptr->number == 0) continue;
    sprintf(buf, "%d) %-20.20s %s", counter++, sptr->name, !(++columns % 3) ? "\r\n" : "");
    send_to_char(buf, ch);
  }
}


/* item types */
void list_obj_types(struct char_data *ch)
{
  int counter, columns = 0;
  
  for (counter = 0; counter < NUM_ITEM_TYPES; counter++) {
    sprintf(buf, "%d) %-20.20s %s", counter, item_types[counter], !(++columns % 2) ? "\r\n" : "");
    send_to_char(buf, ch);
  }
}


/* main search for objects */
int search_object(struct char_data *ch, int mode, int value)
{
  int nr, found = 0;
  
  switch (mode) {
  case 0:  /* perm */
    if (value == -1) {
      list_perm_affects(ch);
      send_to_char("\r\nSearch with the given value.\r\n", ch);
      return (0);
    }
    for (nr = 0; nr <= top_of_objt; nr++) {
      if (obj_proto[nr].obj_flags.bitvector == (1 << value)) {
        sprintf(buf, "%3d. [%5d] %s\r\n", ++found, obj_index[nr].vnum, 
          obj_proto[nr].short_description);
        send_to_char(buf, ch);
      }
    }
    break;
  case 1:  /* type */
    if (value == -1) {
      list_obj_types(ch);
      send_to_char("\r\nSearch with the given value.\r\n", ch);
      return (0);
    }
    for (nr = 0; nr <= top_of_objt; nr++) {
      if (obj_proto[nr].obj_flags.type_flag == value) {
        sprintf(buf, "%3d. [%5d] %s\r\n", ++found, obj_index[nr].vnum, 
          obj_proto[nr].short_description);
        send_to_char(buf, ch);
      }
    }
    break;
  default:
    send_to_char("Can't search on that!\r\n", ch);
    return (0);
  }
  
  return (found);
}


/* the main function for find */
ACMD(do_find)
{
  char stat[MAX_INPUT_LENGTH], type[MAX_INPUT_LENGTH], val[MAX_INPUT_LENGTH];
  int mode, value = -1, mobobj, columns = 0;
  
  half_chop(argument, type, argument);
  half_chop(argument, stat, argument);
  
  if (!*type) {
    send_to_char("Usage: find mob|obj <stat> <value>\r\n", ch);
    return;
  }
  
  
  /* Make sure we're looking for a mob or an object */
  if (is_abbrev(type, "mobile"))
    mobobj = MOB;
  else if (is_abbrev(type, "object")) 
    mobobj = OBJ;
  else {
    send_to_char("That'll have to be either 'mob' or 'obj'.\r\n", ch);
    return;
  }
  
  /* get the value, if given */
  half_chop(argument, val, argument);
  if (*val) value = atoi(val);
  else value = -1;
  
  /* find the stat in the list, and perform the search */
  switch (mobobj) {
  case MOB:
    if (!*stat) {  /* list the searchable fields */
      send_to_char("Search for the following:\r\n", ch);
      for (mode = 0; strcmp(mob_search[mode], "\n"); mode++) {
        sprintf(buf, "%s %s", mob_search[mode], !(++columns % 5) ? "\r\n" : " ");
        send_to_char(buf, ch);
      }
      send_to_char("\r\nUsage: find mob|obj <stat> <value>\r\n", ch);
      return;
    }
	if (value < 0) {
		send_to_char("Search, yes, fine, but what value?\r\n", ch);
		return;
	}
    /* find the stat */
    for (mode = 0; strcmp(mob_search[mode], "\n"); mode++)
      if (is_abbrev(stat, mob_search[mode]))
        break;
      sprintf(buf, "Searching mobs for %s (%d).\r\n", mob_search[mode], value);
      send_to_char(buf, ch);
      /* display results */
      if (search_mobile(ch, mode, value))
        return;
      else {
        send_to_char("No mobs found with the given parameters.\r\n", ch);
        return;
      }
      break;
  case OBJ:
 	  if (!*stat) {  /* list the searchable fields */
      send_to_char("Search for the following:\r\n", ch);
      for (mode = 0; strcmp(obj_search[mode], "\n"); mode++) {
        sprintf(buf, "%s %s", obj_search[mode], !(++columns % 5) ? "\r\n" : " ");
        send_to_char(buf, ch);
      }
      send_to_char("\r\nUsage: find mob|obj <stat> <value>\r\n", ch);
      return;
    }
    /* find the stat */
    for (mode = 0; strcmp(obj_search[mode], "\n"); mode++)
      if (is_abbrev(stat, obj_search[mode]))
        break;
	sprintf(buf, "Searching objs for %s (%d).\r\n", obj_search[mode], value);
	send_to_char(buf, ch);
	/* display results */
	if (search_object(ch, mode, value))
		return;
	else {
		send_to_char("No objects found with the given parameters.\r\n", ch);
		return;
		}
	break;
  default:
    /* We should never get here... */
    extended_mudlog(NRM, SYSL_BUGS, TRUE, "default case reached in search.c, ACMD(do_find)");
    return;
  }
}


int	vnum_mobile(char *searchname, struct char_data *ch)
{
	int nr, found = 0;

	for (nr = 0; nr <= top_of_mobt; nr++) {
		if (isname(searchname, mob_proto[nr].player.name)) {
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
							mob_index[nr].vnum,
							mob_proto[nr].player.short_descr);
			send_to_char(buf, ch);
		}
	}

	return (found);
}


int	vnum_object(char *searchname, struct char_data *ch)
{
	int nr, found = 0;

	for (nr = 0; nr <= top_of_objt; nr++) {
		if (isname(searchname, obj_proto[nr].name)) {
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
							obj_index[nr].vnum,
							obj_proto[nr].short_description);
			send_to_char(buf, ch);
		}
	}
	return (found);
}


int	vnum_weapon(int attacktype, struct char_data *ch)
{
	int nr, found = 0;

	for (nr = 0; nr <= top_of_objt; nr++) {
		if (obj_proto[nr].obj_flags.type_flag == ITEM_WEAPON && obj_proto[nr].obj_flags.value[3] == attacktype) {
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
				obj_index[nr].vnum,
				obj_proto[nr].short_description);
			send_to_char(buf, ch);
		}
	}
	return (found);
}


int	find_attack_type(char *arg)
{
	int nr;

	for (nr=0; nr < NUM_ATTACK_TYPES; nr++) {
		if (is_abbrev(arg, attack_hit_text[nr].singular))
			break;
	}

	return (nr);
}


ACMD(do_vnum)
{
	char *printbuf = get_buffer(MAX_INPUT_LENGTH), *printbuf2 = get_buffer(MAX_INPUT_LENGTH);
	half_chop(argument, printbuf, printbuf2);

	if (!*printbuf || !*printbuf2 || (!is_abbrev(printbuf, "mob") && !is_abbrev(printbuf, "obj") && !is_abbrev(printbuf, "weapon"))) {
		send_to_char("Usage: vnum { obj | mob | weapon } <name | attack-type>\r\n", ch);
		release_buffer(printbuf);
		release_buffer(printbuf2);
		return;
	}
	if (is_abbrev(printbuf, "mob"))
		if (!vnum_mobile(printbuf2, ch))
			send_to_char("No mobiles by that name.\r\n", ch);

	if (is_abbrev(printbuf, "obj"))
		if (!vnum_object(printbuf2, ch))
			send_to_char("No objects by that name.\r\n", ch);

	if (is_abbrev(printbuf, "weapon"))
		if (!vnum_weapon(find_attack_type(printbuf2), ch))
			send_to_char("No weapons with that attack-type.\r\n", ch);

	release_buffer(printbuf);
	release_buffer(printbuf2);
}


#undef MOB
#undef OBJ

#undef __SEARCH_C__
