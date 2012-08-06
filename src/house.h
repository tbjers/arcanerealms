/* $Id: house.h,v 1.8 2002/12/28 20:15:23 arcanere Exp $ */

#define	HOUSE_MAX_HOUSES		100
#define	HOUSE_MAX_GUESTS		30
#define HOUSE_MAX_COWNERS		3

#define HOUSE_MAX_SECURES		5
#define HOUSE_MAX_LOCKS			100

#define	HOUSE_PRIVATE				0
#define	HOUSE_PUBLIC				1

struct house_data {
	house_vnum vnum;						/* house virtual number						*/
	room_vnum atrium;						/* vnum of atrium									*/
	sh_int exit_num;						/* direction of house's exit			*/
	long owner;									/* idnum of house's owner					*/
	time_t built_on;						/* date this house was built			*/
	time_t last_used;						/* date this house was last used	*/
	time_t last_payment;				/* date of last house payment			*/
	bool prune_safe;						/* house will not be auto-pruned	*/
	ush_int mode;								/* mode of ownership							*/
	ush_int cost;								/* house mortage rate							*/
	ush_int max_secure;					/* max num of secure containers		*/
	ush_int max_locked;					/* max number of locked items			*/
	long *rooms;								/* list of house rooms						*/
	long *cowners;							/* idnums of house's co-owners		*/
	long *guests;								/* idnums of house's guests				*/
};

#define	TOROOM(room, dir) (world[room].dir_option[dir] ? \
					world[room].dir_option[dir]->to_room : NOWHERE)

#define	HOUSE_NUM(i)					(house_index[(i)].vnum)
#define	HOUSE_ATRIUM(i)				(house_index[(i)].atrium)
#define	HOUSE_EXIT(i)					(house_index[(i)].exit_num)
#define	HOUSE_BUILT(i)				(house_index[(i)].built_on)
#define	HOUSE_USED(i)					(house_index[(i)].last_used)
#define	HOUSE_PAID(i)					(house_index[(i)].last_payment)
#define	HOUSE_PRUNE_SAFE(i)		(house_index[(i)].prune_safe)
#define	HOUSE_MODE(i)					(house_index[(i)].mode)
#define	HOUSE_OWNER(i)				(house_index[(i)].owner)

#define	HOUSE_COST(i)					(house_index[(i)].cost)
#define	HOUSE_MAX_SECURE(i)		(house_index[(i)].max_secure)
#define	HOUSE_MAX_LOCKED(i)		(house_index[(i)].max_locked)

#define	HOUSE_ROOMS(i)				(house_index[(i)].rooms)
#define	HOUSE_COWNERS(i)			(house_index[(i)].cowners)
#define	HOUSE_GUESTS(i)				(house_index[(i)].guests)

#define	HOUSE_ROOM(i, num)		(house_index[(i)].rooms[(num)])
#define	HOUSE_COWNER(i, num)	(house_index[(i)].cowners[(num)])
#define	HOUSE_GUEST(i, num)		(house_index[(i)].guests[(num)])

void house_save_all(void);
void house_auto_prune(void);
void house_crashsave_objects(house_vnum vnum);
void house_list_guests(struct char_data *ch, int house_nr, int quiet);
void house_list_cowners(struct char_data *ch, int house_nr, int quiet);
void house_delete_file(house_vnum vnum);
int house_can_enter(struct char_data *ch, room_vnum room);
int house_can_take(struct char_data *ch, room_vnum room);
int	find_house_owner(struct char_data *ch);
int	find_house(room_vnum vnum);
