/* ************************************************************************
*  Originally written by: Paul Driver - Annuminas.                        *
*  Last known e-mail address: apollos_98@yahoo.com                        *
*                                                                         *
*  Map function concept and look/feel originally created (to the best of  *
*  the author's knowledge) by the now-defunct Eternal Visions mud.  All   *
*  code (prior to any modifications by other parties) is written by Paul  *
*  Driver, and is Copyright (C)2001, Black Pawn Producions.  All circlemud* 
*  function calls and macros are copyright by someone else.  This code has* 
*  been donated for free use to Arcane Realms mud and any distribution by *
*  said party is solely at its discretion and/or the discretion of the    *
*  author.                                                                *
************************************************************************ */
/* $Id: map.c,v 1.1 2004/03/14 06:14:04 annuminas Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"

#define EXITN(room, door)                (world[room].dir_option[door])

/* local functions */
ACMD(do_map);
void map_search(int[21][21],int,int,room_rnum);
void no_map(int[21][21],room_rnum);
void make_map(int[21][21],char*, int);

struct {
	char *back;
	char *fore;
	char *sym;
} scodes[] = {
	{"47", "30;1", "#"},	/*     Inside      */
	{"47", "30;1", "+"},	/*      City       */
  	{"42", "39",   " "},	/*     Field       */
        {"42", "32;1", "|"},	/*     Forest      */
	{"42", "33",   "^"},	/*     Hills       */
	{"47", "30;1", "^"},	/*    Mountains    */
	{"44", "39",   " "},	/*   Water (Swim)  */
	{"44", "33;1", "~"},	/* Water (No Swim) */
	{"46", "39",   " "},	/*    In Flight    */
	{"44", "30;1", "~"},	/*    Underwater   */
	{"35", "33;1", "+"},	/*   Faerie Regio  */
	{"41", "33;1", "+"},	/*  Infernal Regio */
	{"47", "39;1", "+"},	/*   Divine Regio  */
	{"43", "30;1", "+"},	/*  Ancient Regio  */
	{"43", "33;1", ":"},	/*      Shore      */
  	{"35", "39",   " "}	/*     Highway     */
};

struct {
 	int x;
 	int y;
} dirvals[] = {
	{0,-1},		/* north */
	{1,0},		/* east  */
	{0,1},		/* south */
	{-1,0},		/* west  */
	{2,2},		/*  up   */
	{2,2},		/* down  */
	{1,-1},		/*  ne   */
	{-1,1},		/*  sw   */
	{-1,-1},	/*  nw   */
	{1,1},		/*  se   */
	{2,2},		/*  in   */
	{2,2}		/*  out  */
};

ACMD(do_map) {
  int map[21][21];
  int x,y;
  for(x = 0;x<21;x++)
    for(y=0;y<21;y++)
      map[x][y] = 100;
  map[10][10] = 50;
  char *output = get_buffer(16384);
  if(!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NO_MAP)) 
    map_search(map, 10, 10, IN_ROOM(ch));
  else
    no_map(map, IN_ROOM(ch));
  make_map(map, output, SECT(IN_ROOM(ch)));
  send_to_char(output,ch);
  release_buffer(output);
}

void map_search(int map[21][21], int x, int y, room_rnum room) {
  int exit;
  for(exit=0;exit < 12;exit++) {
    if(dirvals[exit].x == 2)
      continue;
    int tx = x + dirvals[exit].x;
    int ty = y + dirvals[exit].y;
    if(tx > 20 || tx < 0 || ty > 20 || ty < 0)
      continue;
    if(EXITN(room, exit)) {
      struct room_direction_data *nexit = EXITN(room,exit);      
      room_rnum nroom = nexit->to_room;
      if(map[tx][ty] == 100 && !EXIT_FLAGGED(nexit, EX_CLOSED)) {
	if(ROOM_FLAGGED(nroom, ROOM_NO_MAP)) {
	  map[tx][ty] = 75;
	} else {
          map[tx][ty] = SECT(nroom);
          map_search(map,tx,ty,nroom);
        }
      }
    }
  }
}

void no_map(int map[21][21], room_rnum room) {
  int exit;
  int x,y;
  for(exit = 0;exit<10;exit++) {
    if(dirvals[exit].x == 2)
      continue;
    x = 10 + dirvals[exit].x;
    y = 10 + dirvals[exit].y;
    if(EXITN(room, exit) && !EXIT_FLAGGED(EXITN(room,exit), EX_CLOSED))
      map[x][y] = 75;    
  }
}

void make_map(int map[21][21], char *output, int chsec) {
  int x,y;
  char *result;
  sprintf(output, "\r\n%s\r\n", "-----------------------");
  for(y=0;y<21;y++) {
    strcat(output, "|");
    for(x=0;x<21;x++) {
      switch(map[x][y]) {
      case 100:
	strcat(output, " ");
	break;
      case 75:
	strcat(output, "\x1B[39;1m:\x1B[0m");
	break;
      case 50:
	result = get_buffer(64);
	sprintf(result, "\x1B[%s;31;1m@\x1B[0m", scodes[chsec].back);
	strcat(output, result);
	release_buffer(result);
	break;
      default:
	result = get_buffer(64);
        sprintf(result,"\x1B[%s;%sm%s\x1B[0m", 
        	scodes[map[x][y]].back, scodes[map[x][y]].fore, 
		scodes[map[x][y]].sym);
	strcat(output, result);
	release_buffer(result);
	break;
      }
    }
    strcat(output, "|\r\n");
  }
  strcat(output,"-----------------------\r\n");
}
