/************************************************************************
 * OasisOLC - General / oasis.c                                    v2.0 *
 * Original author: Levork                                              *
 * Copyright 1996 by Harvey Gilpin                                      *
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)          *
 ************************************************************************/
/* $Id: oasis.c,v 1.32 2004/03/23 17:59:53 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "shop.h"
#include "house.h"
#include "genolc.h"
#include "genmob.h"
#include "genshp.h"
#include "genzon.h"
#include "genwld.h"
#include "genobj.h"
#include "genhouse.h"
#include "oasis.h"
#include "screen.h"
#include "dg_olc.h"
#include "spells.h"
#include "spedit.h"
#include "guild.h"
#include "guild_parser.h"
#include "tutor.h"

const	char *nrm, *grn, *cyn, *yel, *red;

/*
 * External data structures.
 */
extern int top_of_socialt;
extern int top_of_commandt;
extern int top_of_houset;
extern int is_name(const char *str, const char *namelist);
extern int real_quest(int vnum);
extern void free_action(struct social_messg *action);
extern void free_command(struct command_info *command);
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct room_data *world;
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct spell_info_type *spell_info;
extern struct social_messg *soc_mess_list;
extern zone_rnum top_of_zone_table;
extern struct guild_info *guilds_data;
extern struct command_info *cmd_info;
extern struct tutor_info_type *tutor_info;
extern struct house_data *house_index;

/*
 * Local functions.
 */
void dig_new_room (struct char_data *ch, int zbottom);
ACMD(do_oasis);

/*
 * Internal data structures.
 */
struct olc_scmd_info_t {
	const char *text;
	int con_type;
}	olc_scmd_info[] = {
	{	"room",			CON_REDIT			},
	{	"object",		CON_OEDIT			},
	{	"zone",			CON_ZEDIT			},
	{	"mobile",		CON_MEDIT			},
	{	"shop",			CON_SEDIT			},
	{	"action",		CON_AEDIT			},
	{	"trigger",	CON_TRIGEDIT	},
	{	"assedit",	CON_ASSEDIT		},
	{	"spell",		CON_SPEDIT		},
	{	"quest",		CON_QEDIT			},
	{	"guild",		CON_GEDIT			},
	{ "command",	CON_COMEDIT		},
	{	"tutor",		CON_TUTOREDIT	},
	{	"house",		CON_HEDIT			},
	{	"\n",				-1						}
};

/* -------------------------------------------------------------------------- */

/*
 * Only player characters should be using OLC anyway.
 */
void clear_screen(struct descriptor_data *d)
{
	if (PRF_FLAGGED(d->character, PRF_CLS))
		send_to_char("[H[J", d->character);
}

/* -------------------------------------------------------------------------- */

void dig_new_room (struct char_data *ch, int zbottom)
{
	struct room_data *room;

	/* Set up data for add_room function */
	CREATE(room, struct room_data, 1);
	room->name = str_dup("Zone Description");
	sprintf(buf,	"This unfinished zone was created by %s.\r\n"
								"&YThis room is a placeholder where you should put in your zone description.&g\r\n"
								"Write a brief synopsis of what the zone does, any quests it contains, and you\r\n"
								"can also add a list of all key objects with vnums and what they unlock.\r\n"
								"You may start building from room &Y%d&g and on.\r\n", GET_NAME(ch), zbottom + 1);
	room->description = str_dup(buf);
	room->number = zbottom;
	room->zone = real_zone_by_thing(zbottom);

	/* Add the room */
	add_room(room);

	/* Memory cleanup */
	free(room->name);
	free(room->description);
	free(room);
}


/*
 * Exported ACMD do_oasis function.
 *
 * This function is the OLC interface.  It deals with all the 
 * generic OLC stuff, then passes control to the sub-olc sections.
 */
ACMD(do_oasis)
{
	int number = -1, save = 0, real_num, newspell = FALSE, newguild = FALSE, newtutor = FALSE;
	struct descriptor_data *d;
	struct spell_info_type *sptr = NULL;
	struct tutor_info_type *tptr = NULL;
	struct guild_info *g;
	char *buf3;

	/*
	 * No screwing around as a mobile.
	 */
	if (IS_NPC(ch))
		return;
	
	/*
	 * The command to see what needs to be saved, typically 'olc'.
	 */
	if (subcmd == SCMD_OLC_SAVEINFO) {
		do_show_save_list(ch);
		return;
	}

	/*
	 * Parse any arguments.
	 */
	buf3 = two_arguments(argument, buf1, buf2);
	if (!*buf1) {                /* No argument given. */
		switch (subcmd) {
		case SCMD_OASIS_ZEDIT:
		case SCMD_OASIS_REDIT:
			number = GET_ROOM_VNUM(IN_ROOM(ch));
			break;
		case SCMD_OASIS_OEDIT:
		case SCMD_OASIS_MEDIT:
		case SCMD_OASIS_SEDIT:
		case SCMD_OASIS_TRIGEDIT:
		case SCMD_OASIS_QEDIT:
		case SCMD_OASIS_TUTOREDIT:
		case SCMD_OASIS_HEDIT:
			send_to_charf(ch, "Specify a %s VNUM to edit.\r\n", olc_scmd_info[subcmd].text);
			return;
		case SCMD_OASIS_AEDIT:
			send_to_char("Specify an action to edit.\r\n", ch);
			return;
		case SCMD_OASIS_SPEDIT:
			send_to_char("Usage: spedit <spellnum>\r\n     : spedit new\r\n     : spedit delete <spellnum>\r\n", ch);
			return;
		case SCMD_OASIS_GEDIT:
			send_to_char("Usage: gedit <guildnum>\r\n     : gedit new\r\n     : gedit delete <guildnum>\r\n", ch);
			return;
		case SCMD_OASIS_COMEDIT:
			send_to_char("Specify a command to edit.\r\n", ch);
			return;
		}
	} else if (!isdigit(*buf1)) {
		if (str_cmp("save", buf1) == 0) {
			save = TRUE;
			if (subcmd != SCMD_OASIS_SPEDIT && subcmd != SCMD_OASIS_AEDIT && subcmd != SCMD_OASIS_GEDIT && subcmd != SCMD_OASIS_COMEDIT) {
				if (is_number(buf2))
					number = atoi(buf2);
				else if (GET_OLC_ZONE(ch) >= 0) {
					zone_rnum zlok;
					if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
						number = -1;
					else
					number = genolc_zone_bottom(zlok); /* Or .top or in between. */
				}
				if (number < 0) {
					send_to_char("Save which zone?\r\n", ch);
					return;
				}
			}
		}
		else if (subcmd == SCMD_OASIS_AEDIT || subcmd == SCMD_OASIS_COMEDIT)
			number = 0;
		else if (subcmd == SCMD_OASIS_SPEDIT) {
			if (strncmp("new", buf1, 3) == 0) {
				newspell = TRUE;
			} else if (strncmp("delete", buf1, 3) == 0 && *buf2 && (number = atoi(buf2)) >= 0) {
				if (IS_IMPLEMENTOR(ch)) {
					for (sptr = spell_info; sptr && sptr->number != number; sptr=sptr->next);
						if (sptr && (sptr->number == number) && sptr->number > 0) {
							dequeue_spell(number);
							send_to_char("Spell deleted.\r\n", ch);
						} else {
							send_to_char("There is no such spell.\r\n", ch);
							return;
						}
				} else {
					send_to_char("You are not allowed to delete spells.\r\n", ch);
				}
				return;
			} else {
				send_to_char("Specify a spell to delete.\r\n", ch);
				return;
			}
		} else if (subcmd == SCMD_OASIS_TUTOREDIT) {
			if (IS_HEADBUILDER(ch) || IS_IMPL(ch)) {
				if (strncmp("new", buf1, 3) == 0) {
					newtutor = TRUE;
				} else if (strncmp("delete", buf1, 3) == 0 && *buf2 && (number = atoi(buf2)) >= 0) {
					if (IS_IMPLEMENTOR(ch)) {
						for (tptr = tutor_info; tptr && tptr->number != number; tptr=tptr->next);
							if (tptr && (tptr->number == number) && tptr->number > 0) {
								dequeue_tutor(number);
								sort_tutors();
								send_to_char("Tutor deleted.\r\n", ch);
							} else {
								send_to_char("There is no such tutor.\r\n", ch);
								return;
							}
					} else {
						send_to_char("You are not allowed to delete tutors.\r\n", ch);
					}
					return;
				} else {
					send_to_char("Specify a tutor to delete.\r\n", ch);
					return;
				}
			} else {
				send_to_char("You are not allowed to edit tutors.\r\n", ch);
			}
		} else if (subcmd == SCMD_OASIS_GEDIT) {
			if (strncmp("new", buf1, 3) == 0) {
				if (IS_ADMIN(ch) || IS_IMPL(ch))
					newguild = TRUE;
				else {
					send_to_char("You are not allowed to add guilds.\r\n", ch);
					return;
				}
			} else if (strncmp("delete", buf1, 3) == 0 && *buf2 && (number = atoi(buf2)) >= 0) {
				if (IS_IMPLEMENTOR(ch)) {
					if ((rem_guild(number)) == -1)
						send_to_char("No such guild.\r\n", ch);
					else {
						send_to_char("Guild removed.\r\n", ch);
						save_all_guilds();
					}
				} else {
					send_to_char("You are not allowed to delete guilds.\r\n", ch);
				}
				return;
			} else {
				send_to_char("Specify a guild to delete.\r\n", ch);
				return;
			}
		} else if (subcmd == SCMD_OASIS_ZEDIT && GOT_RIGHTS(ch, RIGHTS_HEADBUILDER)) {
			if (str_cmp("new", buf1) || !buf3 || !*buf3 || !buf2)
				send_to_char("Usage: zedit new <zone> <lower room> <upper room>\r\n", ch);
			else {
				char sbot[MAX_INPUT_LENGTH], stop[MAX_INPUT_LENGTH];
				room_vnum bottom, top;

				skip_spaces(&buf3);	/* actually, atoi() doesn't care... */
				two_arguments(buf3, sbot, stop);

				number = atoi(buf2);
				bottom = atoi(sbot);
				top = atoi(stop);

				if (number == 0 || bottom == 0 || top == 0)
					send_to_char("Usage: zedit new <zone> <lower room> <upper room>\r\n", ch);
				else {
					if (zedit_new_zone(ch, number, bottom, top))
						dig_new_room(ch, bottom);
				}
			}
			return;
		} else {
			send_to_char("Yikes!  Stop that, someone will get hurt!\r\n", ch);
			return;
		}
	}

	/*
	 * If a numeric argument was given (like a room number), get it.
	 */
	if (number == -1)
		number = atoi(buf1);
	
	/*
	 * Check that whatever it is isn't already being edited.
	 */
	for (d = descriptor_list; d; d = d->next)
		if (STATE(d) == olc_scmd_info[subcmd].con_type)
			if (d->olc && OLC_NUM(d) == number) {
				if (subcmd == SCMD_OASIS_AEDIT)
					sprintf(buf, "Actions are already being edited by %s.\r\n",
					(CAN_SEE(ch, d->character) ? GET_NAME(d->character) : "someone"));
				else if (subcmd == SCMD_OASIS_COMEDIT)
					sprintf(buf, "Commands are already being edited by %s.\r\n",
					(CAN_SEE(ch, d->character) ? GET_NAME(d->character) : "someone"));
				else
					sprintf(buf, "That %s is currently being edited by %s.\r\n",
					olc_scmd_info[subcmd].text, PERS(d->character, ch, 1));
				send_to_char(buf, ch);
				return;
			}
			d = ch->desc;
			
	/*
	 * Give descriptor an OLC structure.
	 */
	if (d->olc) {
		extended_mudlog(NRM, SYSL_BUGS, TRUE, "do_oasis: Player already had olc structure.");
		free(d->olc);
	}
	CREATE(d->olc, struct oasis_olc_data, 1);

	/*
	 * Find the zone.
	 */
	if (subcmd != SCMD_OASIS_AEDIT && subcmd != SCMD_OASIS_EMAIL && subcmd != SCMD_OASIS_SPEDIT && 
			subcmd != SCMD_OASIS_GEDIT && subcmd != SCMD_OASIS_COMEDIT && subcmd != SCMD_OASIS_TUTOREDIT &&
			subcmd != SCMD_OASIS_HEDIT) {
		if ((OLC_ZNUM(d) = real_zone_by_thing(number)) == -1) {
			send_to_char("Sorry, there is no zone for that number!\r\n", ch);
			free(d->olc);
			d->olc = NULL;
			return;
		}

		/*
		 * Everyone but IMPLs can only edit zones they have been assigned.
		 */
		if (subcmd != SCMD_OASIS_AEDIT && subcmd != SCMD_OASIS_EMAIL && subcmd != SCMD_OASIS_SPEDIT && subcmd != SCMD_OASIS_TUTOREDIT && subcmd != SCMD_OASIS_COMEDIT) {
			if (!CAN_EDIT_ZONE(ch, OLC_ZNUM(d))) {
				send_to_char("You do not have permission to edit this zone.\r\n", ch);
				free(d->olc);
				d->olc = NULL;
				return;
			}
		}
	} else if (!IS_IMPL(ch)) {
		if (!GOT_RIGHTS(ch, RIGHTS_ACTIONS) && subcmd == SCMD_OASIS_AEDIT) {
			send_to_char("You do not have permission to edit actions.\r\n", ch);
			free(d->olc);
			d->olc = NULL;
			return;
		} else if (!GOT_RIGHTS(ch, RIGHTS_DEVELOPER) && subcmd == SCMD_OASIS_COMEDIT) {
			send_to_char("You do not have permission to edit commands.\r\n", ch);
			free(d->olc);
			d->olc = NULL;
			return;
		} else if (!GOT_RIGHTS(ch, RIGHTS_HEADBUILDER) && subcmd == SCMD_OASIS_TUTOREDIT) {
			send_to_char("You do not have permission to edit tutors.\r\n", ch);
			free(d->olc);
			d->olc = NULL;
			return;
		} else if (!GOT_RIGHTS(ch, RIGHTS_PLAYERS) && subcmd == SCMD_OASIS_HEDIT) {
			send_to_char("You do not have permission to edit houses.\r\n", ch);
			free(d->olc);
			d->olc = NULL;
			return;
		}
	}

	if (save) {
		const char *type = NULL;
 
		if (subcmd >= 0 && subcmd <= (int)(sizeof(olc_scmd_info) / sizeof(struct olc_scmd_info_t) - 1))
			type = olc_scmd_info[subcmd].text;
		else {
			send_to_char("Oops, I forgot what you wanted to save.\r\n", ch);
			return;
		}
		if ((subcmd != SCMD_OASIS_AEDIT) && (subcmd != SCMD_OASIS_SPEDIT) && (subcmd != SCMD_OASIS_GEDIT) && (subcmd != SCMD_OASIS_COMEDIT) && (subcmd != SCMD_OASIS_TUTOREDIT))
			sprintf(buf, "Saving all %ss in zone %d.\r\n", type, zone_table[OLC_ZNUM(d)].number);
		else
			sprintf(buf, "Saving all %s entries.\r\n", type);
		send_to_char(buf, ch);
		if ((subcmd != SCMD_OASIS_AEDIT) && (subcmd != SCMD_OASIS_SPEDIT) && (subcmd != SCMD_OASIS_GEDIT) && (subcmd != SCMD_OASIS_COMEDIT) && (subcmd != SCMD_OASIS_TUTOREDIT))
			extended_mudlog(NRM, SYSL_OLC, TRUE, "%s saves %s info for zone %d.", GET_NAME(ch), type, zone_table[OLC_ZNUM(d)].number);
		else
			extended_mudlog(NRM, SYSL_OLC, TRUE, "%s saves %s info.", GET_NAME(ch), type);

		switch (subcmd) {
			case SCMD_OASIS_REDIT: save_rooms(OLC_ZNUM(d)); break;
			case SCMD_OASIS_ZEDIT: save_zone(OLC_ZNUM(d)); break;
			case SCMD_OASIS_OEDIT: save_objects(OLC_ZNUM(d)); break;
			case SCMD_OASIS_MEDIT: save_mobiles(OLC_ZNUM(d)); break;
			case SCMD_OASIS_SEDIT: save_shops(OLC_ZNUM(d)); break;
			case SCMD_OASIS_QEDIT: qedit_save_to_disk(OLC_ZNUM(d)); break;
			case SCMD_OASIS_SPEDIT: save_spells(); break;
			case SCMD_OASIS_GEDIT: save_all_guilds(); break;
			case SCMD_OASIS_TUTOREDIT: save_tutors(); break;
			case SCMD_OASIS_HEDIT: save_houses(); break;
		}
		free(d->olc);
		d->olc = NULL;
		return;
	}

	if (subcmd != SCMD_OASIS_AEDIT && subcmd != SCMD_OASIS_COMEDIT)
		OLC_NUM(d) = number;
	else {
		OLC_NUM(d) = 0;
		OLC_STORAGE(d) = str_dup(buf1);
	}

	
 /*
	 * Steal player's descriptor and start up subcommands.
	 */
	switch (subcmd) {
	case SCMD_OASIS_TRIGEDIT:
		if ((real_num = real_trigger(number)) >= 0)
			trigedit_setup_existing(d, real_num);
		else
			trigedit_setup_new(d);
		STATE(d) = CON_TRIGEDIT;
		 break;
	case SCMD_OASIS_EMAIL:
		if (!GOT_RIGHTS(ch, RIGHTS_ADMIN)) {
			send_to_char("You do not have permission to use Mercury Email.\r\nPlease contact the implementor for information and permission.\r\n", ch);
			free(d->olc);
			d->olc = NULL;
			return;
		}
		email_setup_new(d);
		STATE(d) = CON_EMAIL;
		break;
	case SCMD_OASIS_REDIT:
		if ((real_num = real_room(number)) >= 0)
			redit_setup_existing(d, real_num);
		else
			redit_setup_new(d);
		STATE(d) = CON_REDIT;
		break;
	case SCMD_OASIS_ZEDIT:
		if ((real_num = real_room(number)) < 0) {
			send_to_char("That room does not exist.\r\n", ch);
			free(d->olc);
			d->olc = NULL;
			return;
		}
		zedit_setup(d, real_num);
		STATE(d) = CON_ZEDIT;
		break;
	case SCMD_OASIS_MEDIT:
		if ((real_num = real_mobile(number)) < 0)
			medit_setup_new(d);
		else
			medit_setup_existing(d, real_num);
		STATE(d) = CON_MEDIT;
		break;
	case SCMD_OASIS_OEDIT:
		if ((real_num = real_object(number)) >= 0)
			oedit_setup_existing(d, real_num);
		else
			oedit_setup_new(d);
		STATE(d) = CON_OEDIT;
		break;
	case SCMD_OASIS_SEDIT:
		if ((real_num = real_shop(number)) < 0)
			sedit_setup_new(d);
		else
			sedit_setup_existing(d, real_num);
		STATE(d) = CON_SEDIT;
		break;
	case SCMD_OASIS_AEDIT:
		for (OLC_ZNUM(d) = 0; (OLC_ZNUM(d) <= top_of_socialt); OLC_ZNUM(d)++)  {
			if (is_abbrev(OLC_STORAGE(d), soc_mess_list[OLC_ZNUM(d)].command))
				break;
		}
		if (OLC_ZNUM(d) > top_of_socialt)  {
			if (find_command(OLC_STORAGE(d)) > NOTHING)  {
				cleanup_olc(d, CLEANUP_ALL);
				send_to_char("That command already exists.\r\n", ch);
				return;
			}
			sprintf(buf, "Do you wish to add the '%s' action? ", OLC_STORAGE(d));
			send_to_char(buf, ch);
			OLC_MODE(d) = AEDIT_CONFIRM_ADD;
		}	else  {
			sprintf(buf, "Do you wish to edit the '%s' action? ", soc_mess_list[OLC_ZNUM(d)].command);
			send_to_char(buf, ch);
			OLC_MODE(d) = AEDIT_CONFIRM_EDIT;
		}
		STATE(d) = CON_AEDIT;
		break;
	case SCMD_OASIS_QEDIT:
		real_num = real_quest(number);
		if (real_num >= 0)
			qedit_setup_existing(d, real_num);
		else
			qedit_setup_new(d);
		STATE(d) = CON_QEDIT;
		break;
	case SCMD_OASIS_SPEDIT:
		if (newspell)
			spedit_setup_new(ch->desc);
		else {
			for (sptr = spell_info; sptr && sptr->number != number; sptr=sptr->next);
			if (sptr && (sptr->number == number) && number > 0) {
				OLC_SPELL(d) = sptr;
				spedit_disp_menu(d);
			} else {
				send_to_char("Usage: spedit <spellnum>\r\n     : spedit new\r\n     : spedit delete <spellnum>\r\n", d->character);
				free(d->olc);
				d->olc = NULL;
				return;
			}
		}
		STATE(d) = CON_SPEDIT;
		break;
	case SCMD_OASIS_TUTOREDIT:
		if (newtutor)
			tutoredit_setup_new(ch->desc);
		else {
			for (tptr = tutor_info; tptr && tptr->number != number; tptr=tptr->next);
			if (tptr && (tptr->number == number) && number > 0) {
				tutoredit_setup_existing(d, tptr);
			} else {
				send_to_char("\r\n&RNo such tutor could be found.&n\r\nUsage: tutoredit <tutornum>\r\n     : tutoredit new\r\n     : tutoredit delete <tutornum>\r\n", d->character);
				free(d->olc);
				d->olc = NULL;
				return;
			}
		}
		STATE(d) = CON_TUTOREDIT;
		break;
	case SCMD_OASIS_GEDIT:
		if (newguild) {
			g = add_guild("New Guild");
			if (!g) {
				send_to_char("Error creating guild, please contact an administrator!\r\n", ch);
				free(d->olc);
				d->olc = NULL;
				return;
			}
			gedit_setup_existing(d, g);
		} else {
			g = guilds_data;
			while (g) {
				if (g->id == number) break;
				g = g->next;
			}	
			if (!g) {
				send_to_char("No such guild!\r\n", ch);
				free(d->olc);
				d->olc = NULL;
				return;
			} else {
				if (!IS_ADMIN(ch)) {
					struct char_guild_element *element;
					/*
					 * If they are not ADMIN and do not have Guild Leader
					 * status, they are not allowed to edit the guild.
					 * Torgny Bjers, 2002-06-28
					 */
					element = GET_CHAR_GUILDS(ch);
					while (element) {
						if (element->guild == g) break;
						element = element->next;
					}
					if (!element) {
						send_to_char("You are not a part of this guild.\r\n", ch);
						free(d->olc);
						d->olc = NULL;
						return;
					}
					if (!STATUS_FLAGGED(element->guildie, STATUS_GL)) {
						send_to_char("You are not a guildleader of this guild!\r\n", ch);
						free(d->olc);
						d->olc = NULL;
						return;
					}
				}
				gedit_setup_existing(d, g);
			}
		}
		STATE(d) = CON_GEDIT;
		break;
	case SCMD_OASIS_COMEDIT:
		skip_spaces(&OLC_STORAGE(d));
		for (OLC_ZNUM(d) = 0; (OLC_ZNUM(d) <= top_of_commandt); OLC_ZNUM(d)++)  {
			if (is_abbrev(OLC_STORAGE(d), cmd_info[OLC_ZNUM(d)].command))
				break;
		}
		if (OLC_ZNUM(d) > top_of_commandt)  {
			if (find_social(OLC_STORAGE(d)) > NOTHING)  {
				cleanup_olc(d, CLEANUP_ALL);
				send_to_char("That command exists as a social.  Use AEDIT to edit this command.\r\n", ch);
				return;
			}
			sprintf(buf, "Do you wish to add the '%s' command? ", OLC_STORAGE(d));
			send_to_char(buf, ch);
			OLC_MODE(d) = COMEDIT_CONFIRM_ADD;
		}	else {
			if (cmd_info[OLC_ZNUM(d)].reserved) {
				send_to_charf(ch, "The '%s' command is reserved, you cannot edit it.\r\n", cmd_info[OLC_ZNUM(d)].command);
				cleanup_olc(d, CLEANUP_ALL);
				return;
			}
			send_to_charf(ch, "Do you wish to edit the '%s' command? ", cmd_info[OLC_ZNUM(d)].command);
			OLC_MODE(d) = COMEDIT_CONFIRM_EDIT;
		}
		STATE(d) = CON_COMEDIT;
		break;
	case SCMD_OASIS_HEDIT:
		if ((real_num = real_house(number)) < 0) {
			if (top_of_houset >= HOUSE_MAX_HOUSES) {
				send_to_char("Max houses already defined.\r\n", ch);
				cleanup_olc(d, CLEANUP_ALL);
				return;
			}
			hedit_setup_new(d);
		} else
			hedit_setup_existing(d, real_num);
		STATE(d) = CON_HEDIT;
		break;
	}
	if (subcmd != SCMD_OASIS_EMAIL) { /* Email is accessible by mortals -spl */
		act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
	} else {
		act("$n opens a nearby E-mail Terminal and starts to create an email.", TRUE, d->character, 0, 0, TO_ROOM);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
	}
}

/*------------------------------------------------------------*\
 Exported utilities 
\*------------------------------------------------------------*/

/*
 * Set the colour string pointers for that which this char will
 * see at color level NRM.  Changing the entries here will change 
 * the colour scheme throughout the OLC.
 */
void get_char_colors(struct char_data *ch)
{
	nrm = CCNRM(ch, C_NRM);
	grn = CCGRN(ch, C_NRM);
	cyn = CCCYN(ch, C_NRM);
	yel = CCYEL(ch, C_NRM);
	red = CCRED(ch, C_NRM);
}

/*
 * This procedure frees up the strings and/or the structures
 * attatched to a descriptor, sets all flags back to how they
 * should be.
 */
void cleanup_olc(struct descriptor_data *d, byte cleanup_type)
{
	/*
	 * Clean up WHAT?
	 */
	if (d->olc == NULL)
		return;

		 /*. Check for storage -- aedit patch -- M. Scott .*/
		 if (OLC_STORAGE(d))
			 free(OLC_STORAGE(d));

	/*
	 * Check for a room. free_room doesn't perform
	 * sanity checks, we must be careful here.
	 */
	if (OLC_ROOM(d)) {
		switch (cleanup_type) {
		case CLEANUP_ALL:
			free_room(OLC_ROOM(d));
			break;
		case CLEANUP_STRUCTS:
			free(OLC_ROOM(d));
			break;
		default: /* The caller has screwed up. */
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "cleanup_olc: Unknown type!");
			break;
		}
	}


	/*
	 * Check for an existing object in the OLC.  The strings
	 * aren't part of the prototype any longer.  They get added
	 * with str_dup().
	 */
	if (OLC_OBJ(d)) {
		free_object_strings(OLC_OBJ(d));
		free(OLC_OBJ(d));
	}

	/*
	 * Check for a mob.  free_mobile() makes sure strings are not in
	 * the prototype.
	 */
	if (OLC_MOB(d))
		free_mobile(OLC_MOB(d));

	/*
	 * Check for a zone.  cleanup_type is irrelevant here, free() everything.
	 */
	if (OLC_ZONE(d)) {
		free(OLC_ZONE(d)->name);
		free(OLC_ZONE(d)->cmd);
		free(OLC_ZONE(d));
	}

	/*
	 * Check for a shop.  free_shop doesn't perform sanity checks, we must
	 * be careful here.
	 */
	if (OLC_SHOP(d)) {
		switch (cleanup_type) {
		case CLEANUP_ALL:
			free_shop(OLC_SHOP(d));
			break;
		case CLEANUP_STRUCTS:
			free(OLC_SHOP(d));
			break;
		default:
			/* The caller has screwed up but we already griped above. */
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "cleanup_olc: Unknown type!");
			break;
		}
	}

	/*. Check for aedit stuff -- M. Scott */
	if (OLC_ACTION(d))  {
		switch(cleanup_type)  {
		case CLEANUP_ALL:
			free_action(OLC_ACTION(d));
			break;
		case CLEANUP_STRUCTS:
			free(OLC_ACTION(d));
			break;
		default:
			/* Caller has screwed up */
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "cleanup_olc: Unknown type!");
			break;
		}
	}

	if(OLC_QUEST(d))
	 { /*. free_quest performs no sanity checks, must be carefull here .*/
		 switch(cleanup_type)
			{ case CLEANUP_ALL:
				free_quest(OLC_QUEST(d));
				break;
			 case CLEANUP_STRUCTS:
				free(OLC_QUEST(d));
				break;
			 default:
				/*. Caller has screwed up .*/
				extended_mudlog(NRM, SYSL_BUGS, TRUE, "cleanup_olc: Unknown type!");
				break;
			}
	 }
		 
	 if (OLC_SPELL(d)) {
		 switch(cleanup_type) {
		 case CLEANUP_ALL:
			 spedit_free_spell(OLC_SPELL(d));
			 break;
		 case CLEANUP_STRUCTS:
			 break;
		 default:
		 /* Caller has screwed up */
		 extended_mudlog(NRM, SYSL_BUGS, TRUE, "cleanup_olc: Unknown type!");
		 break;
		 }
		 OLC_SPELL(d) = NULL;
	 }
	 
	/*
	 * Check for a guild.
	 */
	if (OLC_GUILD(d)) {
		switch (cleanup_type) {
		case CLEANUP_ALL:
			free(OLC_GUILD(d));
			break;
		case CLEANUP_STRUCTS:
			free(OLC_GUILD(d));
			break;
		default: /* The caller has screwed up. */
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "cleanup_olc: Unknown type!");
			break;
		}
	}

	/*
	 * Check for command.
	 */
	if (OLC_COMMAND(d))  {
		switch(cleanup_type)  {
		case CLEANUP_ALL:
			free_command(OLC_COMMAND(d));
			break;
		case CLEANUP_STRUCTS:
			free(OLC_COMMAND(d));
			break;
		default:
			/* Caller has screwed up */
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "cleanup_olc: Unknown type!");
			break;
		}
	}

	/*
	 * Check for a tutor.
	 */
	if (OLC_TUTOR(d)) {
		switch (cleanup_type) {
		case CLEANUP_ALL:
			free_tutor(OLC_TUTOR(d));
			break;
		case CLEANUP_STRUCTS:
			break;
		default:
			/* The caller has screwed up but we already griped above. */
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "cleanup_olc: Unknown type!");
			break;
		}
	}

	/*
	 * Check for a house.  free_house doesn't perform sanity checks, we must
	 * be careful here.
	 */
	if (OLC_HOUSE(d)) {
		switch (cleanup_type) {
		case CLEANUP_ALL:
			free_house(OLC_HOUSE(d));
			break;
		case CLEANUP_STRUCTS:
			free(OLC_HOUSE(d));
			break;
		default:
			/* The caller has screwed up but we already griped above. */
			extended_mudlog(NRM, SYSL_BUGS, TRUE, "cleanup_olc: Unknown type!");
			break;
		}
	}


	/*
	 * Restore descriptor playing status.
	 */
	if (d->character) {
		REMOVE_BIT(PLR_FLAGS(d->character), PLR_OLC);
		if (!OLC_EMAIL(d)) {
			STATE(d) = CON_PLAYING;
			act("$n stops using OLC.", TRUE, d->character, NULL, NULL, TO_ROOM);
		} else {
			STATE(d) = CON_PLAYING;
			act("$n finishes $s e-mail and closes the terminal.", TRUE, d->character, NULL, NULL, TO_ROOM);
		}
		if (GET_TELLS(d->character, 0) && SESS_FLAGGED(d->character, SESS_HAVETELLS))
			write_to_output(d, TRUE, "\007&RYou have buffered tells.&n\r\n");
	}

	free(d->olc);
	d->olc = NULL;
}
