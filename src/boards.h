/* ************************************************************************
*		File: boards.h                                      Part of CircleMUD *
*	 Usage: header file for bulletin boards                                 *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: boards.h,v 1.11 2002/12/16 17:42:24 arcanere Exp $ */

#define	NUM_OF_BOARDS					22			/* change if needed! */
#define	MAX_BOARD_MESSAGES		300			/* arbitrary -- change if needed */
#define	MAX_MESSAGE_LENGTH		32768		/* arbitrary -- change if needed */

#define	INDEX_SIZE	   ((NUM_OF_BOARDS*MAX_BOARD_MESSAGES) + 5)

struct board_msginfo {
	 int	slot_num;     /* pos of message in "master index" */
	 char	*heading;     /* pointer to message's heading */
	 bitvector_t rights;/* rights of poster */
	 char *author;			/* pointer to message's author */
	 int	heading_len;  /* size of header (for file write) */
	 int	message_len;  /* size of message text (for file write) */
	 int	author_len;		/* size of author (for file write) */
};

struct board_info_type {
	 obj_vnum vnum;	/* vnum of this board */
	 bitvector_t	read_rgt;	/* rights to read messages on this board */
	 bitvector_t	write_rgt;	/* rights to write messages on this board */
	 bitvector_t	remove_rgt;	/* rights to remove messages from this board */
	 char	filename[50];	/* file to save this board to */
	 obj_rnum rnum;	/* rnum of this board */
};

#define	BOARD_VNUM(i) (board_info[i].vnum)
#define	READ_RGT(i) (board_info[i].read_rgt)
#define	WRITE_RGT(i) (board_info[i].write_rgt)
#define	REMOVE_RGT(i) (board_info[i].remove_rgt)
#define	FILENAME(i) (board_info[i].filename)
#define	BOARD_RNUM(i) (board_info[i].rnum)

#define	NEW_MSG_INDEX(i) (msg_index[i][num_of_msgs[i]])
#define	MSG_HEADING(i, j) (msg_index[i][j].heading)
#define	MSG_SLOTNUM(i, j) (msg_index[i][j].slot_num)
#define	MSG_RIGHTS(i, j) (msg_index[i][j].rights)
#define MSG_AUTHOR(i,j) (msg_index[i][j].author)

int	Board_display_msg(int board_type, struct char_data *ch, char *arg, struct obj_data *board);
int	Board_show_board(int board_type, struct char_data *ch, char *arg, struct obj_data *board);
int	Board_remove_msg(int board_type, struct char_data *ch, char *arg, struct obj_data *board);
int	Board_write_message(int board_type, struct char_data *ch, char *arg, struct obj_data *board);
void	Board_save_board(int board_type);
void	Board_load_board(int board_type);
void	Board_reset_board(int board_type);
void	Board_clear_board(int board_type);
void	Board_clear_all(void);
