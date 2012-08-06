/* ************************************************************************
*		File: comm.h                                        Part of CircleMUD *
*	 Usage: header file: prototypes of public communication functions       *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: comm.h,v 1.7 2002/12/31 01:00:19 arcanere Exp $ */

#define	NUM_RESERVED_DESCS	8
#define	COPYOVER_FILE "copyover.dat"

/* comm.c */
#if USE_CIRCLE_BUFFERS
void	send_to_all(const char *messg);
void	send_to_char(const char *messg, struct char_data *ch);
void	send_to_room(const char *messg, room_rnum room);
void	send_to_outdoor(const char *messg);
void	perform_to_all(const char *messg, struct char_data *ch);
void	close_socket(struct descriptor_data *d);
#endif
size_t	send_to_charf(struct char_data *ch, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
void	send_to_allf(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
void	send_to_roomf(room_rnum room, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
void	send_to_outdoorf(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));

void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj,
								const void *vict_obj, const struct char_data *to, int use_name);

void	act(const char *str, int hide_invisible, struct char_data *ch,
		struct obj_data *obj, const void *vict_obj, int type);

#define	TO_ROOM				1
#define	TO_VICT				2
#define	TO_NOTVICT		3
#define	TO_CHAR				4
#define TO_GROUP			5
#define TO_OOC				64
#define	TO_SLEEP			128	/* to char, even if sleeping */

/* I/O functions */
int	write_to_descriptor(socket_t desc, const char *txt);
void	write_to_q(const char *txt, struct txt_q *queue, int aliased);
size_t write_to_output(struct descriptor_data *t, int color, const char *txt, ...);
size_t vwrite_to_output(struct descriptor_data *t, int color, const char *format, va_list args);
void	string_add(struct descriptor_data *d, char *str);
void string_write(struct descriptor_data *d, char **writeto, size_t len, long mailto, int mode);

#define	PAGE_LENGTH	22
#define	PAGE_WIDTH	80
void	page_string(struct descriptor_data *d, char *str, int keep_internal);

#define	USING_SMALL(d)	((d)->output == (d)->small_outbuf)
#define	USING_LARGE(d)  ((d)->output == (d)->large_outbuf)

typedef	RETSIGTYPE sigfunc(int);

#define	SENDOK(ch)  ((((ch)->desc && !EDITING((ch)->desc)) || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
										 (to_sleeping || AWAKE(ch)) && \
										 (IS_NPC(ch) || !PLR_FLAGGED((ch), PLR_WRITING) || !PLR_FLAGGED((ch), PLR_OLC)))
