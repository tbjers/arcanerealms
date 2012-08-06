/************************************************************************
 * Generic OLC Library - Zones / genzon.h                          v1.0 *
 * Copyright 1996 by Harvey Gilpin                                      *
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)          *
 ************************************************************************/
/* $Id: genzon.h,v 1.4 2002/11/06 19:29:42 arcanere Exp $ */

zone_rnum create_new_zone(zone_vnum vzone_num, room_vnum bottom, room_vnum top, const char **error);
void create_world_index(int znum, const char *type);
void remove_room_zone_commands(zone_rnum rznum, room_rnum rrnum);
int	save_zone(zone_rnum zone_num);
int	count_commands(struct reset_com *list);
void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos);
void remove_cmd_from_list(struct reset_com **list, int pos);
int	new_command(struct zone_data *zone, int pos);
void delete_command(struct zone_data *zone, int pos);
zone_rnum	real_zone(zone_vnum vznum);
zone_rnum	real_zone_by_thing(room_vnum vznum);

/* Make delete_zone() */
