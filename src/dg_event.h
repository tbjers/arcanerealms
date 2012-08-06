/* $Id: dg_event.h,v 1.2 2001/11/09 00:12:11 arcanere Exp $ */
/*
** how often will heartbeat() call our event function?
*/
#define	PULSE_DG_EVENT 1


/*
** macro used to prototype the callback function for an event
*/
#define	EVENT(function) void (function)(void *info)


/*
** define event related structures
*/
struct event_info {
	int time_remaining;
	EVENT(*func);
	void *info;
	struct event_info *next;  
};


/*
** prototype event functions
*/
struct event_info *add_event(int time, EVENT(*func), void *info);
void remove_event(struct event_info *event);
void process_events(void);
