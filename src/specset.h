/****************************************************
 * Code needed for specset and related functions
 *
 * There is no reason this has to be separated from specset.c;
 * I have it this way because of several special OLC functions
 * I've made.
 ****************************************************/
/* $Id: specset.h,v 1.13 2003/02/10 16:33:33 arcanere Exp $ */

/* External Functions */

/*mobs*/
SPECIAL(postmaster);
SPECIAL(cityguard);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(questmaster);
SPECIAL(temple_cleric);
SPECIAL(shop_keeper);
SPECIAL(summoned);
SPECIAL(tutor);
SPECIAL(banker);

#define	NUM_MOB_SPECS  18   /* actual spec. count + 1 */

/*objects*/
SPECIAL(bank);
SPECIAL(gen_board);
SPECIAL(portal);
SPECIAL(start_portal);
SPECIAL(dye_fill);

#define	NUM_OBJ_SPECS  6    /* actual spec. count + 1 */

/*rooms*/
SPECIAL(dump);
SPECIAL(pet_shops);	 /*NOTE:  just assigning this to a room will NOT make it a functioning pet store.*/

#define	NUM_ROOM_SPECS 3    /* actual spec. count + 1 */

/*
	sfunc = the function name from SPECIAL(x)
	name = what you want to have appear in lists, stats etc.
*/
struct specproc_info {
	int (* sfunc)(struct char_data *,void *,int ,char *);
	char *name;
};

extern struct specproc_info mob_specproc_info[];
extern struct specproc_info obj_specproc_info[];
extern struct specproc_info room_specproc_info[];

void ASSIGNROOM(room_vnum room, SPECIAL(fname));
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));

void get_spec_name (int i, char *specname, char type);
