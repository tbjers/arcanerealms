/* $Id: specset.c,v 1.13 2003/02/10 16:33:33 arcanere Exp $ */
/************************************************************
 *  SPECSET.C
 *  Code to assign special procedures on the fly
 *  Syntax:  Specset <m|o|r> <vnum> <specnum>
 *
 *  Written by L. Raymond
 *  Comments to raymond@brokerys.com
 *
 *  The code in this file and the accompanying header makes it
 *  as easy to assign a special procedure to a room, object or
 *  mob as it is to GOTO someplace.  With minor alterations to
 *  existing code these specs can be saved to disk and restored
 *  at bootup or, if you prefer not to change anything, they'll
 *  be temporary and exist only until the next reboot.
 *
 *  Changes required to save to disk (details follow):
 *
 *  Add a new int to the structures index_data and room_data:
 *       int specproc
 *  In DB.C:
 *   -  add a new case to parse_room, parse_object and parse_mobile
 *   -  have boot_db() call assign_spec_procs () anytime after
 *      boot_world() [syntax: assign_spec_procs (0,0,0,1);]
 *  In your OLC code, make whatever changes are needed to save
 *  the spec.  (I'll outline my way below)
 *
 *  The functions:
 *
 *  ACMD (do_specset)
 *    The main function.  This asigns the proper integer to
 *    mob_index.specnum, obj_index.specnum or world.specnum then
 *    calls assign_spec_procs () to attach the actual function.
 *    If you choose not to save to disk, this function will need
 *    to be altered to handle the function assignment directly.
 *
 *  void assign_spec_procs
 *    (char type, sh_int vnum, int which, char startup);
 *    This function uses the information you provide in the various
 *    specproc_info structs (defined in this file) to assign
 *    the functions.  In case of error, it just sets the function
 *    pointer to NULL.  If startup = 1 then it will loop through
 *    the entire mob, object and world tables and assign the
 *    functions specified by the specnum int.  This function will
 *    not be required if you choose not to save to disk and
 *    edit specset to handle the assignments.
 *
 *  void get_spec_name (int i, char *specname, char type);
 *    This returns a name for each spec. proc.  I use it in the
 *    various do_stat functions and the speclist report.  Esentially
 *    cosmetic, but very handy.
 *
 *  void do_initial_specs(void);
 *    Called by assign_spec_proc when startup=1.  This is what
 *    does the actual looping through the tables.
 *
 *  void display_spec_options(struct char_data* ch);
 *    Displays a numbered list of specs for reference.  Typing
 *    SPECSET with no paramters displays this list.
 *
 *  ACMD(do_speclist)
 *    This is just a report function that will list every mob, object or
 *    room with a special procedure attached to it, telling you what the
 *    spec proc is.
 *
 *  Saving to disk
 *
 *  No matter what code you use for OLC, when it comes to saving the
 *  info the output file is pretty much the same, so all you need to
 *  do is add another line to the save function for mobs, objects and
 *  rooms.  I decided to designate spec procs with a "P", so mobs
 *  are saved like this:
 *
 *    [last 4 lines of my mob_save_to_disk]
 *     if (GET_CON(mob) != 11)  fprintf(mob_file, "Con: %d\n", GET_CON(mob));
 *     if (GET_CHA(mob) != 11)  fprintf(mob_file, "Cha: %d\n", GET_CHA(mob));
 *     if (GET_NUM_ATTACKS(mob) > 1)
 *       fprintf(mob_file, "Att: %d\n", GET_NUM_ATTACKS(mob));
 *     if (mob_index[GET_MOB_RNUM(mob)].specproc>0)
 *       fprintf(mob_file,"P: %d\r\n",mob_index[GET_MOB_RNUM(mob)].specproc);
 *
 *  In db.c's parse_mobile() I added this line to the switch statement:
 *
 *  case 'S':     // Simple monsters
 *   parse_simple_mob(mob_f, i, nr);
 *   break;
 * case 'E':     // Circle3 Enhanced monsters
 *   parse_enhanced_mob(mob_f, i, nr);
 *   break;
 *  case 'P':   //spec_proc.
 *     get_line(mob_f, line);
 *     if ((j=atoi(line)) <0)
 *       {
 *         sprintf(buf,"SYSERR(DB71): Format error in mob #%d's Spec Proc\n", nr);
 *          log (buf);
 *             exit(1);
 *        }
 *     else mob_index[i].specproc=j;
 *       break;
 *
 *  Do the same for rooms and objects, editing your various save_to_disk
 *  and boot up routines.  After the boot up is complete, call
 *  assign_spec_procs(), specifying startup=1,and it will use this info
 *  to make the actual function assignments.
 *
 *  Using get_spec_name ()
 *
 *    Add this to your various do_stat functions to get output
 *    that tells you what spec. proc is assigned.  Example:
 *       Name: 'a cashcard', Aliases: cashcard card atm
 *       VNum: [ 3036], RNum: [   83], Type: OTHER SpecProc: (Bank)
 *
 *    For this output, edit do_stat_object and replace the code which
 *    prints "Exists" with this:
 *
 *    if (obj_index[GET_OBJ_RNUM(j)].func!=NULL)
 *        get_spec_name(GET_OBJ_RNUM(j), buf2, 'o');
 *    else sprintf(buf2,"none");
 *    sprintf(buf, "SpecProc: (%s%s%s)\r\n",CCGRNB(ch, C_NRM),buf2,CCNRM(ch, C_NRM));
 *    send_to_char(buf, ch);
 *
 *    Be sure to remove the color codes if needed.  This is also
 *    how to change do_stat_character and do_stat_room; just change
 *    the 'o' to 'm' or 'r' as needed in get_spec_name.
 *
 *
 ************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"

#include "specset.h"

/*	 external vars  */
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern room_rnum top_of_world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern struct room_data *world;

/* Local functions */
ACMD(do_specset);
ACMD(do_speclist);
void assign_spec_procs (char type, sh_int vnum, int which, char startup);
void get_spec_name (int i, char *specname, char type);
void do_initial_specs(void);
void display_spec_options(struct char_data* ch);

/***********************************************************************/

struct specproc_info mob_specproc_info[] = {
	{NULL, "None"},
	{postmaster, "Postmaster"},
	{cityguard, "City guard"},
	{receptionist, "Receptionist"},
	{cryogenicist, "Cryogenicist"},
	{puff, "Puff"},
	{fido, "Fido"},
	{janitor, "Janitor"},
	{mayor, "Mayor"},
	{snake, "Snake"},
	{thief, "Thief"},
	{magic_user, "Magic user"},
	{questmaster, "Quest Master"},
	{temple_cleric, "Temple Cleric"},
	{summoned, "Summoned Mob"},
	{shop_keeper, "Shop Keeper"},
	{tutor, "Tutor"},
	{banker, "Banker"},
};

struct specproc_info obj_specproc_info[] = {
	{NULL, "None"},
	{bank, "Bank"},
	{gen_board, "Bulletin Board"},
	{portal, "Portal"},
	{start_portal, "Game Start Portal"},
	{dye_fill, "Color Dye"},
};

struct specproc_info room_specproc_info[] = {
	{NULL, "None"},
	{dump, "Dump"},
	{pet_shops, "Petshop"},
};

/***********************************************************************/

void get_spec_name (int i, char *specname, char type)
{
	int j;

	switch (toupper(type)) {
	case 'M':
		for (j=0; j<NUM_MOB_SPECS;j++) {
			if (mob_index[i].func == mob_specproc_info[j].sfunc) {
				strcpy(specname,mob_specproc_info[j].name);
				break;
			}
		}
		if ((j==NUM_MOB_SPECS) && (mob_index[i].func != NULL)) {
			/*
			 * This could also be an error, but unless you prefer to enumerate every
			 * spec in Castle.c, so you'll just have to be able to recognize a
			 * legitimate "Welmar spec" from an error.
			 */
			strcpy(specname,"Error!");
		}
		break;
	case 'O':
		for (j=0; j<NUM_OBJ_SPECS;j++) {
			if (obj_index[i].func == obj_specproc_info[j].sfunc) {
				strcpy(specname,obj_specproc_info[j].name);
				break;
			}
		}
		if ((j==NUM_OBJ_SPECS) && (obj_index[i].func != NULL)) {
			strcpy(specname,"Error!");
		}
		break;

	case 'R':
		for (j=0; j<NUM_ROOM_SPECS;j++) {
			if (world[i].func == room_specproc_info[j].sfunc) {
				strcpy(specname,room_specproc_info[j].name);
				break;
			}
		}
		if ((j==NUM_ROOM_SPECS) && (world[i].func != NULL)) {
			strcpy(specname,"Error!");
		}
		break;
	}
}


/* Only call this at boot up*/
void do_initial_specs(void)
{
	int i=0;

	for (i=0; i<top_of_world;i++)
		if (world[i].specproc>0) assign_spec_procs('r', world[i].number, world[i].specproc, 0);

	for (i=0; i<top_of_mobt;i++)
		if (mob_index[i].specproc>0) assign_spec_procs('m', mob_index[i].vnum, mob_index[i].specproc, 0);

	for (i=0; i<top_of_objt;i++)
		if (obj_index[i].specproc>0) assign_spec_procs('o', obj_index[i].vnum, obj_index[i].specproc, 0);
};


void assign_spec_procs (char type, sh_int vnum, int which, char startup)
{
	if (startup) { /*startup should always = 0 except one time at boot up*/
		do_initial_specs();
		return;
	} else {
		switch (toupper(type)) {
		case 'M':
			if (which < 0 || which > NUM_MOB_SPECS)  mob_index[real_mobile(vnum)].func=NULL;
			else ASSIGNMOB(vnum, mob_specproc_info[which].sfunc);
			break;
		case 'O':
			 if (which < 0 || which > NUM_OBJ_SPECS)  obj_index[real_object(vnum)].func=NULL;
			 else ASSIGNOBJ(vnum, obj_specproc_info[which].sfunc);
			 break;
		case 'R':
			 if (which < 0 || which > NUM_ROOM_SPECS)  world[real_room(vnum)].func=NULL;
			 else ASSIGNROOM(vnum, room_specproc_info[which].sfunc);
			 break;
		}  /*switch type*/
	}  /*else !startup*/
}


/* This simply displays a numbered list of available functions*/
void display_spec_options(struct char_data* ch)
{
	int i=1;
	const int columns=3;

	sprintf(buf,"\r\nRoom Specs:\r\n");
	for (i=1; i<NUM_ROOM_SPECS;i++) {
		sprintf(buf+strlen(buf),"%2d) %-15s",i,room_specproc_info[i].name);
		if (!(i%columns)) strcat(buf,"\r\n");
	}

	sprintf(buf+strlen(buf),"\r\n\r\nMob Specs:\r\n");
	for (i=1; i<NUM_MOB_SPECS;i++) {
		sprintf(buf+strlen(buf),"%2d) %-15s",i,mob_specproc_info[i].name);
		if (!(i%columns)) strcat(buf,"\r\n");
	}
	sprintf(buf+strlen(buf),"\r\n\r\nObject Specs:\r\n");
	for (i=1; i<NUM_OBJ_SPECS;i++) {
		sprintf(buf+strlen(buf),"%2d) %-15s",i,obj_specproc_info[i].name);
		if (!(i%columns)) strcat(buf,"\r\n");
	}

	send_to_char(buf,ch);
}

#define	SPECSETFORMAT "Usage: specset <m|o|r> <vnum> <specnum>. Use 0 to remove spec.\r\n"

/*
 * do_specset assigns an integer to the specnum field of mob_index and obj_index, and to
 */
ACMD (do_specset)
{
	char type;
	sh_int vnum;
	int specnum;
	sh_int rnum;

	if (!*argument) {
		send_to_char (SPECSETFORMAT,ch);
		display_spec_options(ch);
		return;
	}	/*no argument*/

	argument++;
	type=*argument;
	argument++;
	two_arguments (argument, buf1, buf2);

	if ((!*buf1) || (!*buf2)) {
		send_to_char (SPECSETFORMAT,ch);
		display_spec_options(ch);
		return;
	}

	vnum=atoi(buf1);
	specnum=atoi(buf2);

	switch (type) {
	case 'm':
	case 'M':
		if (!(rnum=real_mobile(vnum))) {
			send_to_char ("There is no mob with that vnum.\r\n",ch);
			return;
		} else mob_index[rnum].specproc=specnum;
		break;
	case 'o':
	case 'O':
		if (!(rnum=real_object(vnum))) {
			send_to_char ("There is no obj with that vnum.\r\n",ch);
			return;
		} else obj_index[rnum].specproc=specnum;
		break;
	case 'r':
	case 'R':
		if (!(rnum=real_room(vnum))) {
			send_to_char ("There is no room with that vnum.\r\n",ch);
			return;
		} else world[rnum].specproc=specnum;
		break;
	default:
		send_to_char ("Usage: specset <m|o|r> vnum specnum.\r\n",ch);
		return;
	}  /*switch type*/
	assign_spec_procs (type, vnum, specnum,0);
	send_to_char(OK, ch);
}



/*Check	for a <type>.specproc.  If specproc>0 display name.*/
ACMD(do_speclist)
{
	int i=0;
	char type;
	char specname[50];

	if (!*argument) {
		send_to_char ("Usage:  Speclist <room | mob | obj>",ch);
		return;
	}

	skip_spaces(&argument);
	type=*argument;

	strcpy(buf, "List of Special Procedures --\r\n");

	switch (type) {
	case 'M':
	case 'm':
		for (i=0; i<top_of_mobt;i++) {
			if (mob_index[i].func!=NULL) {
				specname[0]='\0';
				get_spec_name (i, specname,'m');
				sprintf(buf+strlen(buf),"[&c%5d&n] &y%-45s&n %s\r\n",
				mob_index[i].vnum, mob_proto[i].player.short_descr, specname);
			};
		}
		break;
	case 'O':
	case 'o':
		for (i=0; i<top_of_objt;i++) {
			if (obj_index[i].func!=NULL) {
				specname[0]='\0';
				get_spec_name (i, specname,'o');
				sprintf(buf+strlen(buf),"[&c%5d&n] &y%-45s&n %s\r\n",
				obj_index[i].vnum, obj_proto[i].short_description, specname);
			}
		}
		break;
	case 'R':
	case 'r':
		for (i=0; i<top_of_world;i++) {
			if (world[i].func!=NULL) {
				specname[i]='\0';
				get_spec_name (i, specname,'r');
				sprintf(buf+strlen(buf),"[&c%5d&n] &y%-45s&n %s\r\n",
				world[i].number, world[i].name, specname);
			}
		}
		break;
	default:
		send_to_char ("Usage:  Speclist <w | m | o>",ch);
		return;
	}	/*switch*/
	page_string (ch->desc, buf, 1);
}

