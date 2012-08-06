/* ************************************************************************
*		File: boards.c                                      Part of CircleMUD *
*	 Usage: handling of multiple bulletin boards                            *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: boards.c,v 1.21 2002/12/16 17:42:23 arcanere Exp $ */


/* FEATURES & INSTALLATION INSTRUCTIONS ***********************************

This board code has many improvements over the infamously buggy standard
Diku board code.  Features include:

-	Arbitrary number of boards handled by one set of generalized routines.
	Adding a new board is as easy as adding another entry to an array.
-	Safe removal of messages while other messages are being written.
-	Does not allow messages to be removed by someone of rights less than
	the poster's rights.


TO ADD A NEW BOARD, simply follow our easy 4-step program:

1	- Create a new board object in the object files

2	- Increase the NUM_OF_BOARDS constant in boards.h

3	- Add a new line to the board_info array below.  The fields, in order, are:

	Board's virtual number.
	Rights one must have to look at this board or read messages on it.
	Rights one must have to post a message to the board.
	Rights one must have to remove other people's messages from this
		board (but you can always remove your own message).
	Filename of this board, in quotes.
	Last field must always be 0.

4	- In spec_assign.c, find the section which assigns the special procedure
		gen_board to the other bulletin boards, and add your new one in a
		similar fashion.

*/


#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "boards.h"
#include "interpreter.h"
#include "handler.h"
#include "constants.h"

/* Board appearance order. */
#define	NEWEST_AT_TOP     FALSE

extern int is_name(const char *str, const char *namelist);

/*
format:	vnum, read rgt, write rgt, remove rgt, filename, 0 at end
Be sure to also change NUM_OF_BOARDS in board.h
*/
struct board_info_type board_info[NUM_OF_BOARDS] = {
	{3099, RIGHTS_MEMBER, RIGHTS_MEMBER, RIGHTS_PLAYERS, LIB_ETC "board.mort", 0},
	{3098, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_ADMIN, LIB_ETC "board.immort", 0},
	{3097, RIGHTS_IMMORTAL, RIGHTS_ADMIN, RIGHTS_IMPLEMENTOR, LIB_ETC "board.freeze", 0},
	{3096, RIGHTS_MEMBER, RIGHTS_MEMBER, RIGHTS_IMMORTAL, LIB_ETC "board.social", 0},
	{3095, RIGHTS_MEMBER, RIGHTS_MEMBER, RIGHTS_IMMORTAL, LIB_ETC "board.academy", 0},
	{3094, RIGHTS_MEMBER, RIGHTS_MEMBER, RIGHTS_DEVELOPER, LIB_ETC "board.bugs", 0},
	{3093, RIGHTS_MEMBER, RIGHTS_MEMBER, RIGHTS_PLAYERS, LIB_ETC "board.suggestions", 0},
	{3092, RIGHTS_MEMBER, RIGHTS_MEMBER, RIGHTS_PLAYERS, LIB_ETC "board.wanted", 0},
	{1251, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_BUILDING, LIB_ETC "board.corin", 0},
	{1250, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_BUILDING, LIB_ETC "board.lowfai", 0},
	{1249, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_BUILDING, LIB_ETC "board.aramil", 0},
	{1248, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_IMPLEMENTOR, LIB_ETC "board.cheron", 0},
	{1247, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_BUILDING, LIB_ETC "board.tyu", 0},
	{1246, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_BUILDING, LIB_ETC "board.celebrin", 0},
	{1245, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_BUILDING, LIB_ETC "board.diserius", 0},
	{1244, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_BUILDING, LIB_ETC "board.silverking", 0},
	{1243, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_ADMIN, LIB_ETC "board.eldoin", 0},
	{1242, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_HEADBUILDER, LIB_ETC "board.landie", 0},
	{1241, RIGHTS_IMMORTAL, RIGHTS_IMMORTAL, RIGHTS_IMPLEMENTOR, LIB_ETC "board.artovil", 0},
	{326, RIGHTS_MEMBER, RIGHTS_QUESTOR, RIGHTS_QUESTOR, LIB_ETC "board.quest", 0},
	{325, RIGHTS_MEMBER, RIGHTS_MEMBER, RIGHTS_IMMORTAL, LIB_ETC "board.ic", 0},
	{324, RIGHTS_MEMBER, RIGHTS_DEVELOPER, RIGHTS_DEVELOPER, LIB_ETC "board.changes", 0},
};

/* local functions */
SPECIAL(gen_board);
int	find_slot(void);
int	find_board(struct char_data *ch);
void init_boards(void);

char *msg_storage[INDEX_SIZE];
int	msg_storage_taken[INDEX_SIZE];
int	num_of_msgs[NUM_OF_BOARDS];
int	ACMD_READ, ACMD_LOOK, ACMD_EXAMINE, ACMD_WRITE, ACMD_REMOVE;
struct board_msginfo msg_index[NUM_OF_BOARDS][MAX_BOARD_MESSAGES];


int	find_slot(void)
{
	int i;

	for (i = 0; i < INDEX_SIZE; i++)
		if (!msg_storage_taken[i]) {
			msg_storage_taken[i] = 1;
			return (i);
		}
	return (-1);
}


/* search the room ch is standing in to find which board he's looking at */
int	find_board(struct char_data *ch)
{
	struct obj_data *obj;
	int i;

	for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
		for (i = 0; i < NUM_OF_BOARDS; i++)
			if (BOARD_RNUM(i) == GET_OBJ_RNUM(obj))
	return (i);

	return (-1);
}


void init_boards(void)
{
	int i, j, fatal_error = 0;

	for (i = 0; i < INDEX_SIZE; i++) {
		msg_storage[i] = 0;
		msg_storage_taken[i] = 0;
	}

	for (i = 0; i < NUM_OF_BOARDS; i++) {
		if ((BOARD_RNUM(i) = real_object(BOARD_VNUM(i))) == NOTHING) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Fatal board error: board vnum %d does not exist!",
				BOARD_VNUM(i));
			fatal_error = 1;
		}
		num_of_msgs[i] = 0;
		for (j = 0; j < MAX_BOARD_MESSAGES; j++) {
			memset((char *) &(msg_index[i][j]), 0, sizeof(struct board_msginfo));
			msg_index[i][j].slot_num = -1;
		}
		Board_load_board(i);
	}

	if (fatal_error) {
		kill_mysql();
		exit(1);
	}

}


SPECIAL(gen_board)
{
	int board_type;
	static int loaded = 0;
	struct obj_data *board = (struct obj_data *)me;

	if (!loaded) {
		init_boards();
		loaded = 1;
	}
	if (!ch->desc)
		return (0);

	ACMD_READ = find_command("read");
	ACMD_WRITE = find_command("write");
	ACMD_REMOVE = find_command("remove");
	ACMD_LOOK = find_command("look");
	ACMD_EXAMINE = find_command("examine");


	if (cmd != ACMD_WRITE && cmd != ACMD_LOOK && cmd != ACMD_EXAMINE &&
			cmd != ACMD_READ && cmd != ACMD_REMOVE)
		return (0);

	if ((board_type = find_board(ch)) == -1) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "degenerate board!  (what the hell...)");
		return (0);
	}
	if (cmd == ACMD_WRITE)
		return (Board_write_message(board_type, ch, argument, board));
	else if (cmd == ACMD_LOOK || cmd == ACMD_EXAMINE)
		return (Board_show_board(board_type, ch, argument, board));
	else if (cmd == ACMD_READ)
		return (Board_display_msg(board_type, ch, argument, board));
	else if (cmd == ACMD_REMOVE)
		return (Board_remove_msg(board_type, ch, argument, board));
	else
		return (0);
}


int	Board_write_message(int board_type, struct char_data *ch, char *arg, struct obj_data *board)
{
	char *tmstr;
	time_t ct;
	char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

	if (!GOT_RIGHTS(ch, WRITE_RGT(board_type))) {
		send_to_char("&MYou are not holy enough to write on this board.&n\r\n", ch);
		return (1);
	}
	if (num_of_msgs[board_type] >= MAX_BOARD_MESSAGES) {
		send_to_char("&MThe board is full.&n\r\n", ch);
		return (1);
	}
	if ((NEW_MSG_INDEX(board_type).slot_num = find_slot()) == -1) {
		send_to_char("&MThe board is malfunctioning - sorry.&n\r\n", ch);
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Board: failed to find empty slot on write.");
		return (1);
	}
	/* skip blanks */
	skip_spaces(&arg);
	delete_doubledollar(arg);

	/* JE 27 Oct 95 - Truncate headline at 80 chars if it's longer than that */
	arg[80] = '\0';

	if (!*arg) {
		send_to_char("We must have a headline!\r\n", ch);
		return (1);
	}
	ct = time(0);
	tmstr = (char *) asctime(localtime(&ct));
	*(tmstr + strlen(tmstr) - 1) = '\0';

	sprintf(buf2, "&n(&c%s&n)", GET_NAME(ch));
	sprintf(buf, "&y%6.10s&n %-18s :: &W%s&n", tmstr, buf2, arg);
	NEW_MSG_INDEX(board_type).heading = str_dup(buf);
	NEW_MSG_INDEX(board_type).rights = get_max_rights(ch);
	NEW_MSG_INDEX(board_type).author = str_dup(GET_NAME(ch));

	act("$n starts to write a message.", TRUE, ch, 0, 0, TO_ROOM);

	write_to_output(ch->desc, TRUE, "%s", stredit_header);

	string_write(ch->desc, &(msg_storage[NEW_MSG_INDEX(board_type).slot_num]),
		MAX_MESSAGE_LENGTH, board_type, EDIT_BOARD);

	num_of_msgs[board_type]++;
	return (1);
}


int	Board_show_board(int board_type, struct char_data *ch, char *arg, struct obj_data *board)
{
	int i;
	char tmp[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];

	if (!ch->desc)
		return (0);

	one_argument(arg, tmp);

	if (!*tmp || !isname(tmp, board->name))
		return (0);

	if (!GOT_RIGHTS(ch, READ_RGT(board_type))) {
		send_to_char("You try but fail to understand the holy words.\r\n", ch);
		return (1);
	}
	act("$n studies the board.", TRUE, ch, 0, 0, TO_ROOM);

	strcpy(buf,
	 "This is a bulletin board.  Usage: READ/REMOVE <messg #>, WRITE <header>.\r\n"
	 "You will need to look at the board to save your message.\r\n");
	if (!num_of_msgs[board_type])
		strcat(buf, "&WThe board is empty.&n\r\n");
	else {
		sprintf(buf + strlen(buf), "There are %d messages on the board.\r\n",
			num_of_msgs[board_type]);
#if	NEWEST_AT_TOP
		for (i = num_of_msgs[board_type] - 1; i >= 0; i--)
#else
		for (i = 0; i < num_of_msgs[board_type]; i++)
#endif
		{
			if (MSG_HEADING(board_type, i))
#if	NEWEST_AT_TOP
	sprintf(buf + strlen(buf), "&c%-2d &n: %s&n\r\n",
		num_of_msgs[board_type] - i, MSG_HEADING(board_type, i));
#else
	sprintf(buf + strlen(buf), "&c%-2d &n: %s&n\r\n", i + 1, MSG_HEADING(board_type, i));
#endif
			else {
	extended_mudlog(BRF, SYSL_BUGS, TRUE, "The board is fubar'd.");
	send_to_char("&MSorry, the board isn't working.&n\r\n", ch);
	return (1);
			}
		}
	}
	page_string(ch->desc, buf, 1);

	return (1);
}


int	Board_display_msg(int board_type, struct char_data *ch, char *arg, struct obj_data *board)
{
	char number[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];
	int msg, ind;

	one_argument(arg, number);
	if (!*number)
		return (0);
	if (isname(number, board->name))	/* so "read board" works */
		return (Board_show_board(board_type, ch, arg, board));
	if (!is_number(number))	/* read 2.mail, look 2.sword */
		return (0);
	if (!(msg = atoi(number)))
		return (0);

	if (!GOT_RIGHTS(ch, READ_RGT(board_type))) {
		send_to_char("&MYou try but fail to understand the holy words.&n\r\n", ch);
		return (1);
	}
	if (!num_of_msgs[board_type]) {
		send_to_char("&WThe board is empty!&n\r\n", ch);
		return (1);
	}
	if (msg < 1 || msg > num_of_msgs[board_type]) {
		send_to_char("&MThat message exists only in your imagination.&n\r\n",
		 ch);
		return (1);
	}
#if	NEWEST_AT_TOP
	ind = num_of_msgs[board_type] - msg;
#else
	ind = msg - 1;
#endif
	if (MSG_SLOTNUM(board_type, ind) < 0 ||
			MSG_SLOTNUM(board_type, ind) >= INDEX_SIZE) {
		send_to_char("Sorry, the board is not working.\r\n", ch);
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Board is screwed up. (Room #%d)", GET_ROOM_VNUM(IN_ROOM(ch)));
		return (1);
	}
	if (!(MSG_HEADING(board_type, ind))) {
		send_to_char("&MThat message appears to be screwed up.&n\r\n", ch);
		return (1);
	}
	if (!(msg_storage[MSG_SLOTNUM(board_type, ind)])) {
		send_to_char("&MThat message seems to be empty.&n\r\n", ch);
		return (1);
	}
	sprintf(buffer, "Message &c%d &n: %s&n\r\n\r\n%s\r\n", msg,
		MSG_HEADING(board_type, ind),
		msg_storage[MSG_SLOTNUM(board_type, ind)]);

	page_string(ch->desc, buffer, 1);

	return (1);
}

int	Board_remove_msg(int board_type, struct char_data *ch, char *arg, struct obj_data *board)
{
	int ind, msg, slot_num;
	char number[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
	struct descriptor_data *d;

	one_argument(arg, number);

	if (!*number || !is_number(number))
		return (0);
	if (!(msg = atoi(number)))
		return (0);

	if (!num_of_msgs[board_type]) {
		send_to_char("&WThe board is empty!&n\r\n", ch);
		return (1);
	}
	if (msg < 1 || msg > num_of_msgs[board_type]) {
		send_to_char("&MThat message exists only in your imagination.&n\r\n", ch);
		return (1);
	}
#if	NEWEST_AT_TOP
	ind = num_of_msgs[board_type] - msg;
#else
	ind = msg - 1;
#endif
	if (!MSG_HEADING(board_type, ind)) {
		send_to_char("&MThat message appears to be screwed up.&n\r\n", ch);
		return (1);
	}
	if (!is_name(GET_NAME(ch), board->short_description)) {
		if (strcmp(GET_NAME(ch), MSG_AUTHOR(board_type, ind))) {
			if (!GOT_RIGHTS(ch, REMOVE_RGT(board_type))) {
				send_to_char("&MYou are not holy enough to remove other people's messages.&n\r\n", ch);
				return (1);
			}
		}
		if (!GOT_RIGHTS(ch, MSG_RIGHTS(board_type, ind))) {
			send_to_char("&MYou can't remove a message holier than yourself.&n\r\n", ch);
			return (1);
		}
	}
	slot_num = MSG_SLOTNUM(board_type, ind);
	if (slot_num < 0 || slot_num >= INDEX_SIZE) {
		send_to_char("&MThat message is majorly screwed up.&n\r\n", ch);
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "The board is seriously screwed up. (Room #%d)", GET_ROOM_VNUM(IN_ROOM(ch)));
		return (1);
	}
	for (d = descriptor_list; d; d = d->next) {
		if (EDITING(d) && d->textedit->str == &(msg_storage[slot_num])) {
			send_to_char("&MAt least wait until the author is finished before removing it!&n\r\n", ch);
			return (1);
		}
	}

	if (msg_storage[slot_num])
		free(msg_storage[slot_num]);
	msg_storage[slot_num] = 0;
	msg_storage_taken[slot_num] = 0;
	if (MSG_HEADING(board_type, ind))
		free(MSG_HEADING(board_type, ind));
	if (MSG_AUTHOR(board_type, ind))
		free(MSG_AUTHOR(board_type, ind));

	for (; ind < num_of_msgs[board_type] - 1; ind++) {
		MSG_HEADING(board_type, ind) = MSG_HEADING(board_type, ind + 1);
		MSG_SLOTNUM(board_type, ind) = MSG_SLOTNUM(board_type, ind + 1);
		MSG_RIGHTS(board_type, ind) = MSG_RIGHTS(board_type, ind + 1);
		MSG_AUTHOR(board_type, ind) = MSG_AUTHOR(board_type, ind + 1);
	}
	num_of_msgs[board_type]--;
	send_to_char("Message removed.\r\n", ch);
	sprintf(buf, "$n just removed message %d.", msg);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
	Board_save_board(board_type);

	return (1);
}


void Board_save_board(int board_type)
{
	FILE *fl;
	int i;
	char *tmp1, *tmp2 = NULL, *tmp3;

	if (!num_of_msgs[board_type]) {
		remove(FILENAME(board_type));
		return;
	}
	if (!(fl = fopen(FILENAME(board_type), "wb"))) {
		perror("SYSERR: Error writing board");
		return;
	}
	fwrite(&(num_of_msgs[board_type]), sizeof(int), 1, fl);

	for (i = 0; i < num_of_msgs[board_type]; i++) {
		if ((tmp1 = MSG_HEADING(board_type, i)) != NULL)
			msg_index[board_type][i].heading_len = strlen(tmp1) + 1;
		else
			msg_index[board_type][i].heading_len = 0;

		if (MSG_SLOTNUM(board_type, i) < 0 ||
	MSG_SLOTNUM(board_type, i) >= INDEX_SIZE ||
	(!(tmp2 = msg_storage[MSG_SLOTNUM(board_type, i)])))
			msg_index[board_type][i].message_len = 0;
		else
			msg_index[board_type][i].message_len = strlen(tmp2) + 1;

		if ((tmp3 = MSG_AUTHOR(board_type, i)) != NULL)
			msg_index[board_type][i].author_len = strlen(tmp3) + 1;
		else
			msg_index[board_type][i].author_len = 0;

		fwrite(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
		if (tmp1)
			fwrite(tmp1, sizeof(char), msg_index[board_type][i].heading_len, fl);
		if (tmp2)
			fwrite(tmp2, sizeof(char), msg_index[board_type][i].message_len, fl);
		if (tmp3)
			fwrite(tmp3, sizeof(char), msg_index[board_type][i].author_len, fl);
	}

	fclose(fl);
}


void Board_load_board(int board_type)
{
	FILE *fl;
	int i, len1, len2, len3;
	char *tmp1, *tmp2, *tmp3;

	if (!(fl = fopen(FILENAME(board_type), "rb"))) {
		if (errno != ENOENT)
			perror("SYSERR: Error reading board");
		return;
	}
	fread(&(num_of_msgs[board_type]), sizeof(int), 1, fl);
	if (num_of_msgs[board_type] < 1 || num_of_msgs[board_type] > MAX_BOARD_MESSAGES) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Board file %d corrupt.  Resetting.", board_type);
		Board_reset_board(board_type);
		return;
	}
	for (i = 0; i < num_of_msgs[board_type]; i++) {
		fread(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
		if ((len1 = msg_index[board_type][i].heading_len) <= 0) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Board file %d corrupt! (Header)  Resetting.", board_type);
			Board_reset_board(board_type);
			return;
		}
		CREATE(tmp1, char, len1);
		fread(tmp1, sizeof(char), len1, fl);
		MSG_HEADING(board_type, i) = tmp1;

		if ((MSG_SLOTNUM(board_type, i) = find_slot()) == -1) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Out of slots booting board %d!  Resetting...", board_type);
			Board_reset_board(board_type);
			return;
		}
		if ((len2 = msg_index[board_type][i].message_len) > 0) {
			CREATE(tmp2, char, len2);
			fread(tmp2, sizeof(char), len2, fl);
			msg_storage[MSG_SLOTNUM(board_type, i)] = tmp2;
		} else
			msg_storage[MSG_SLOTNUM(board_type, i)] = NULL;

		if ((len3 = msg_index[board_type][i].author_len) <= 0) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Board file %d corrupt! (Author)  Resetting.", board_type);
			Board_reset_board(board_type);
			return;
		}
		CREATE(tmp3, char, len3);
		fread(tmp3, sizeof(char), len3, fl);
		MSG_AUTHOR(board_type, i) = tmp3;

	}

	fclose(fl);
}


/* When shutting down, clear all boards. */
void Board_clear_all(void)
{
	int i;

	for (i = 0; i < NUM_OF_BOARDS; i++)
		Board_clear_board(i);
}


/* Clear the in-memory structures. */
void Board_clear_board(int board_type)
{
	int i;

	for (i = 0; i < MAX_BOARD_MESSAGES; i++) {
		if (MSG_HEADING(board_type, i))
			free(MSG_HEADING(board_type, i));
		if (msg_storage[MSG_SLOTNUM(board_type, i)])
			free(msg_storage[MSG_SLOTNUM(board_type, i)]);
		if (MSG_AUTHOR(board_type, i))
			free(MSG_AUTHOR(board_type, i));
		msg_storage_taken[MSG_SLOTNUM(board_type, i)] = 0;
		memset((char *)&(msg_index[board_type][i]),0,sizeof(struct board_msginfo));
		msg_index[board_type][i].slot_num = -1;
	}
	num_of_msgs[board_type] = 0;
}


/* Destroy the on-disk and in-memory board. */
void Board_reset_board(int board_type)
{
  Board_clear_board(board_type);
	remove(FILENAME(board_type));
}
