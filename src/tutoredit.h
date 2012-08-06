/* ************************************************************************
*   File: tutoredit.h                           An extension to CircleMUD *
*  Usage: Spell Editor header file                                        *
*                                                                         *
*  This file by Torgny Bjers <artovil@arcanerealms.org>                   *
*  Copyright (C) 2002 by Arcane Realms MUD, www.arcanerealms.org.         *
************************************************************************ */
/* $Id: tutoredit.h,v 1.2 2002/08/28 14:01:39 arcanere Exp $ */

#ifndef	_TUTOREDIT_H_
#define	_TUTOREDIT_H_

void tutoredit_disp_menu(struct descriptor_data * d);
void tutoredit_parse(struct descriptor_data *d, char *arg);
void modify_string(char **str, char *newstr);

#endif
