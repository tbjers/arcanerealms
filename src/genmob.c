/* ************************************************************************
*  Generic OLC Library - Mobiles / genmob.c                          v1.0 *
*  Copyright 1996 by Harvey Gilpin                                        *
*  Copyright 1997-1999 by George Greer (greerga@circlemud.org)            *
*                                                                         *
*  MySQL C API connection for world files and various former disk based   *
*  storage/loading functions made by Torgny Bjers for Arcane Realms MUD.  *
*  Copyright (C)2001, Torgny Bjers.                                       *
*                                                                         *
*  MYSQL COPYRIGHTS                                                       *
*  The client library, and the GNU getopt library, are covered by the     *
*  GNU LIBRARY GENERAL PUBLIC LICENSE.                                    *
 ************************************************************************/
/* $Id: genmob.c,v 1.31 2002/11/06 19:29:42 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "shop.h"
#include "handler.h"
#include "genolc.h"
#include "genmob.h"
#include "genzon.h"
#include "dg_olc.h"

int	update_mobile_strings(struct char_data *t, struct char_data *f);
void check_mobile_strings(struct char_data *mob);
void check_mobile_string(mob_vnum i, char **string, const char *dscr);
int write_mobile_record(mob_vnum mvnum, zone_vnum vznum, struct char_data *mob);
int	free_mobile_strings(struct char_data *mob);
int	copy_mobile_strings(struct char_data *t, struct char_data *f);

extern int top_shop;
extern mob_rnum top_of_mobt;
extern zone_rnum top_of_zone_table;
extern struct zone_data *zone_table;
extern struct shop_data *shop_index;
extern struct char_data *character_list;
extern struct char_data *mob_proto;
extern struct index_data *mob_index;

int	add_mobile(struct char_data *mob, mob_vnum vnum)
{
	int rnum, i, found = FALSE, shop, cmd_no;
	zone_rnum zone;
	struct char_data *live_mob;

	if ((rnum = real_mobile(vnum)) != NOBODY) {
		/* Copy over the mobile and free() the old strings. */
		copy_mobile(&mob_proto[rnum], mob);

		/* Now re-point all existing mobile strings to here. */
		for (live_mob = character_list; live_mob; live_mob = live_mob->next)
			if (rnum == live_mob->nr)
				update_mobile_strings(live_mob, &mob_proto[rnum]);

		add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);
		extended_mudlog(CMP, SYSL_OLC, TRUE, "add_mobile: Updated existing mobile #%d.", vnum);
		return TRUE;
	}

	RECREATE(mob_proto, struct char_data, top_of_mobt + 2);
	RECREATE(mob_index, struct index_data, top_of_mobt + 2);
	top_of_mobt++;

	for (i = top_of_mobt; i > 0; i--) {
		if (vnum > mob_index[i - 1].vnum) {
			mob_proto[i] = *mob;
			mob_proto[i].nr = i;
			copy_mobile_strings(&mob_proto[i], mob);
			mob_index[i].vnum = vnum;
			mob_index[i].number = 0;
			mob_index[i].func = 0;
			found = i;
			break;
		}
		mob_index[i] = mob_index[i - 1];
		mob_proto[i] = mob_proto[i - 1];
		mob_proto[i].nr++;
	}
	if (!found) {
		mob_proto[0] = *mob;
		mob_proto[0].nr = 0;
		copy_mobile_strings(&mob_proto[0], mob);
		mob_index[0].vnum = vnum;
		mob_index[0].number = 0;
		mob_index[0].func = 0;
	}

	extended_mudlog(CMP, SYSL_OLC, TRUE, "add_mobile: Added mobile %d at index #%d.", vnum, found);

#if	CONFIG_GENOLC_MOBPROG
	GET_MPROG(OLC_MOB(d)) = OLC_MPROGL(d);
	GET_MPROG_TYPE(OLC_MOB(d)) = (OLC_MPROGL(d) ? OLC_MPROGL(d)->type : 0);
	while (OLC_MPROGL(d)) {
		GET_MPROG_TYPE(OLC_MOB(d)) |= OLC_MPROGL(d)->type;
		OLC_MPROGL(d) = OLC_MPROGL(d)->next;
	}
#endif

	/*
	 * Update live mobile rnums.
	 */
	for (live_mob = character_list; live_mob; live_mob = live_mob->next)
		GET_MOB_RNUM(live_mob) += (GET_MOB_RNUM(live_mob) >= found);

	/*
	 * Update zone table.
	 */
	for (zone = 0; zone <= top_of_zone_table; zone++)
		for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++)
			if (ZCMD(zone, cmd_no).command == 'M')
	ZCMD(zone, cmd_no).arg1 += (ZCMD(zone, cmd_no).arg1 >= found);

	/*
	 * Update shop keepers.
	 */
	if (shop_index)
		for (shop = 0; shop <= top_shop - top_shop_offset; shop++)
			SHOP_KEEPER(shop) += (SHOP_KEEPER(shop) >= found);

	add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);
	return found;
}

int	copy_mobile(struct char_data *to, struct char_data *from)
{
	free_mobile_strings(to);
	*to = *from;
	check_mobile_strings(from);
	copy_mobile_strings(to, from);
	return TRUE;
}

void extract_mobile_all(mob_vnum vnum)
{
	struct char_data *next, *ch;

	for (ch = character_list; ch; ch = next) {
		next = ch->next;
		if (GET_MOB_VNUM(ch) == vnum)
			extract_char(ch);
	}
}

int	delete_mobile(mob_rnum refpt)
{
	struct char_data *live_mob;
	int vnum, counter, zone, cmd_no;

	if (refpt == NOBODY || refpt > top_of_mobt) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "GenOLC: delete_mobile: Invalid rnum %d.", refpt);
		return FALSE;
	}

	vnum = mob_index[refpt].vnum;
	add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);
	extract_mobile_all(vnum);
	free_mobile_strings(&mob_proto[refpt]);

	for (counter = refpt; counter < top_of_mobt; counter++) {
		mob_index[counter] = mob_index[counter + 1];
		mob_proto[counter] = mob_proto[counter + 1];
		mob_proto[counter].nr--;
	}

	top_of_mobt--;
	RECREATE(mob_index, struct index_data, top_of_mobt + 1);
	RECREATE(mob_proto, struct char_data, top_of_mobt + 1);

	/*
	 * Update live mobile rnums.
	 */
	for (live_mob = character_list; live_mob; live_mob = live_mob->next)
		GET_MOB_RNUM(live_mob) -= (GET_MOB_RNUM(live_mob) >= refpt);

	/*
	 * Update zone table.
	 */
	for (zone = 0; zone <= top_of_zone_table; zone++)
		for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++)
			if (ZCMD(zone, cmd_no).command == 'M')
	ZCMD(zone, cmd_no).arg1 -= (ZCMD(zone, cmd_no).arg1 >= refpt);

	/*
	 * Update shop keepers.
	 */
	if (shop_index)
		for (counter = 0; counter <= top_shop - top_shop_offset; counter++)
			SHOP_KEEPER(counter) -= (SHOP_KEEPER(counter) >= refpt);

	return TRUE;
}

int	copy_mobile_strings(struct char_data *t, struct char_data *f)
{
	if (f->player.name)
		t->player.name = str_dup(f->player.name);
	if (f->player.title)
		t->player.title = str_dup(f->player.title);
	if (f->player.short_descr)
		t->player.short_descr = str_dup(f->player.short_descr);
	if (f->player.long_descr)
		t->player.long_descr = str_dup(f->player.long_descr);
	if (f->player.description)
		t->player.description = str_dup(f->player.description);
	return TRUE;
}

int	update_mobile_strings(struct char_data *t, struct char_data *f)
{
	if (f->player.name)
		t->player.name = f->player.name;
	if (f->player.title)
		t->player.title = f->player.title;
	if (f->player.short_descr)
		t->player.short_descr = f->player.short_descr;
	if (f->player.long_descr)
		t->player.long_descr = f->player.long_descr;
	if (f->player.description)
		t->player.description = f->player.description;
	return TRUE;
}

int	free_mobile_strings(struct char_data *mob)
{
	if (mob->player.name)
		free(mob->player.name);
	if (mob->player.title)
		free(mob->player.title);
	if (mob->player.short_descr)
		free(mob->player.short_descr);
	if (mob->player.long_descr)
		free(mob->player.long_descr);
	if (mob->player.description)
		free(mob->player.description);
	return TRUE;
}

/*
 * Free a mobile structure that has been edited.
 * Take care of existing mobiles and their mob_proto!
 */
int	free_mobile(struct char_data *mob)
{
	int i;

	if (mob == NULL)
		return FALSE;

	/*
	 * Non-prototyped mobile.  Also known as new mobiles.
	 */
	if ((i = GET_MOB_RNUM(mob)) == NOBODY)
		free_mobile_strings(mob);
	else {	/* Prototyped mobile. */
		if (mob->player.name && mob->player.name != mob_proto[i].player.name)
			free(mob->player.name);
		if (mob->player.title && mob->player.title != mob_proto[i].player.title)
			free(mob->player.title);
		if (mob->player.short_descr && mob->player.short_descr != mob_proto[i].player.short_descr)
			free(mob->player.short_descr);
		if (mob->player.long_descr && mob->player.long_descr != mob_proto[i].player.long_descr)
			free(mob->player.long_descr);
		if (mob->player.description && mob->player.description != mob_proto[i].player.description)
			free(mob->player.description);
	}
	while (mob->affected)
		affect_remove(mob, mob->affected);

	if (mob->proto_script && mob->proto_script != mob_proto[i].proto_script)
		free_proto_script(mob, MOB_TRIGGER);

	free(mob);
	return TRUE;
}

int	save_mobiles(zone_rnum rznum)
{
	zone_vnum vznum;
	room_vnum i;
	mob_rnum rmob;

	if (rznum == NOWHERE || rznum > top_of_zone_table) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "GenOLC: save_mobiles: Invalid real zone number %d. (0-%d)", rznum, top_of_zone_table);
		return FALSE;
	}

	vznum = zone_table[rznum].number;

	/* Delete all the zone's mobs from the database. */
	mysqlWrite("DELETE FROM %s WHERE znum = %d;", TABLE_MOB_INDEX, vznum);

	/* Delete all the zone's mob trigger assignments from the database. */
	mysqlWrite("DELETE FROM %s WHERE znum = %d AND type = %d;", TABLE_TRG_ASSIGNS, vznum, MOB_TRIGGER);

	for (i = genolc_zone_bottom(rznum); i <= zone_table[rznum].top; i++) {
		if ((rmob = real_mobile(i)) == NOBODY)
			continue;
		check_mobile_strings(&mob_proto[rmob]);
		if (write_mobile_record(i, vznum, &mob_proto[rmob]) < 0)
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "GenOLC: Error writing mobile #%d.", i);
	}

	remove_from_save_list(vznum, SL_MOB);
	return TRUE;
}


int write_mobile_record(mob_vnum mvnum, zone_vnum vznum, struct char_data *mob)
{
	char *short_desc=NULL;
	char *long_desc=NULL;
	char *description=NULL;
	char *alias=NULL;

	char *mob_replace = "INSERT INTO %s (" // 0
		 "znum, "						// 1
		 "vnum, "
		 "alias, "
		 "sdesc, "
		 "ldesc, "					// 5
		 "ddesc, "
		 "flags, "
		 "aff_flags, "
		 "alignment, "
		 "difficulty, "			// 10
		 "hitroll, "   
		 "AC, "
		 "hit, "
		 "mana, "
		 "move, "						// 15				
		 "ndd, "
		 "sdd, "
		 "damroll, "
		 "gold, "
		 "pos, "						// 20
		 "default_pos, "
		 "sex, "
		 "BareHandAttack, "
		 "Strength, "
		 "Agility, "				// 25
		 "`Precision`, "
		 "Perception, "
		 "Health, "
		 "Willpower, "
		 "Intelligence, "		// 30
		 "Charisma, "
		 "Luck, "
		 "Essence, "
		 "Spec, "
		 "Race, "						// 35
		 "val0, "
		 "val1, "
		 "val2, "
		 "val3) "						// 39
		"VALUES("
		 "%d, "					// 1
		 "%d, "
		 "'%s', "
		 "'%s', "
		 "'%s', "				// 5
		 "'%s', "
		 "%llu, "
		 "%llu, "
		 "%d, "
		 "%d, "					// 10
		 "%d, "
		 "%d, "
		 "%d, "
		 "%d, "
		 "%d, " 				// 15
		 "%d, "	
		 "%d, "
		 "%d, "
		 "%d, "
		 "%d, "					// 20
		 "%d, "
		 "%d, "
		 "%d, "
		 "%d, "
		 "%d, "					// 25
		 "%d, "
		 "%d, "
		 "%d, "
		 "%d, "
		 "%d, " 				// 30
		 "%d, "
		 "%d, "
		 "%d, "
		 "%d, "
		 "%d, "					// 35
		 "%d, "
		 "%d, "
		 "%d, "
		 "%d);";				// 39

	SQL_MALLOC(GET_SDESC(mob), short_desc);
	SQL_MALLOC(GET_ALIAS(mob), alias);
	SQL_MALLOC(GET_LDESC(mob), long_desc);
	SQL_MALLOC(GET_DDESC(mob), description);
	SQL_ESC(GET_SDESC(mob), short_desc);
	SQL_ESC(GET_ALIAS(mob), alias);
	SQL_ESC(GET_LDESC(mob), long_desc);
	SQL_ESC(GET_DDESC(mob), description);

	if (!(mysqlWrite(
		mob_replace,
		TABLE_MOB_INDEX,								// 0
		vznum,													// 1
		mvnum,
		alias,
		short_desc,
		long_desc,											// 5
		description,
		MOB_FLAGS(mob),
		AFF_FLAGS(mob),
		GET_ALIGNMENT(mob),
		GET_DIFFICULTY(mob),					// 10
		20 - GET_HITROLL(mob),
		GET_PD(mob),
		GET_HIT(mob),
		GET_MANA(mob),
		GET_MOVE(mob),								// 15
		GET_NDD(mob),
		GET_SDD(mob),
		GET_DAMROLL(mob),
		GET_GOLD(mob),
		GET_POS(mob),									// 20
		GET_DEFAULT_POS(mob),
		GET_SEX(mob),
		GET_ATTACK(mob),
		GET_STRENGTH(mob),
		GET_AGILITY(mob),							// 25
		GET_PRECISION(mob),
		GET_PERCEPTION(mob),
		GET_HEALTH(mob),
		GET_WILLPOWER(mob),
		GET_INTELLIGENCE(mob),				// 30
		GET_CHARISMA(mob),
		GET_LUCK(mob),
		GET_ESSENCE(mob),
		mob_index[GET_MOB_RNUM(mob)].specproc > 0 ? mob_index[GET_MOB_RNUM(mob)].specproc : 0,
		GET_RACE(mob),								// 35
		GET_MOB_VAL(mob, 0),
		GET_MOB_VAL(mob, 1),
		GET_MOB_VAL(mob, 2),
		GET_MOB_VAL(mob, 3)						// 39
	))) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error writing mobile to database.");
		return FALSE;
	}

	SQL_FREE(short_desc);
	SQL_FREE(alias);
	SQL_FREE(long_desc);
	SQL_FREE(description);

	script_save_to_disk(GET_MOB_VNUM(mob), vznum, mob, MOB_TRIGGER);

	return TRUE;
}

void check_mobile_strings(struct char_data *mob)
{
	mob_vnum mvnum = mob_index[mob->nr].vnum;
	check_mobile_string(mvnum, &GET_LDESC(mob), "long description");
	check_mobile_string(mvnum, &GET_DDESC(mob), "detailed description");
	check_mobile_string(mvnum, &GET_ALIAS(mob), "alias list");
	check_mobile_string(mvnum, &GET_SDESC(mob), "short description");
}

void check_mobile_string(mob_vnum i, char **string, const char *dscr)
{
	if (*string == NULL || **string == '\0') {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "GenOLC: Mob #%d has an invalid %s.", i, dscr);
		if (*string)
			free(*string);
		*string = str_dup("An undefined string.");
	}
}
