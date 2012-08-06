/*
 * Interface to iSpell
 *	Copyright (c) 1997 Erwin S. Andreasen <erwin@pip.dknet.dk>
 */
/* $Id: ispell.c,v 1.9 2002/11/06 18:55:47 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include <unistd.h>
#include <ctype.h>
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include <sys/wait.h>
#define	Stringify(x) Str(x)
#define	Str(x) #x
#define	ISPELL_DICTIONARY "etc/mud.dictionary"
static FILE *ispell_out;
int	ispell_pid=-1;
static int to[2], from[2];
#define	ISPELL_BUF_SIZE 1024

/* local functions */
void ispell_init(void);
void ispell_done(void);
char*	get_ispell_line (char *word);
ACMD(do_ispell);
bool call_ispell (struct descriptor_data *d, char *argument);


void ispell_init(void)
{
	char *ignore_buf = get_buffer(1024);
#if	0
	if (IS_SET(sysdata.options, OPT_NO_ISPELL))
	{
		ispell_pid=-1;
		return;
	}
#endif
	pipe(to);
	pipe(from);
	ispell_pid = fork();
	if (ispell_pid < 0)
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "ispell_init: fork: %s", strerror(errno));
	else if (ispell_pid == 0) /* child */
	{
		int i;
		dup2 (to[0], 0); /* this is where we read commands from - make it stdin */
		close (to[0]);
		close (to[1]);
		dup2 (from[1], 1); /* this is where we write stuff to */
		close (from[0]);
		close (from[1]);
		/* Close all the other files */
		for (i = 2; i < 255; i++)
			close (i);
		execlp ("ispell", "ispell", "-a", "-p" ISPELL_DICTIONARY, (char *)NULL);
		release_buffer(ignore_buf);
		exit(1);
	}
	else /* ok !*/
	{
		close (to[0]);
		close (from[1]);
		ispell_out = fdopen (to[1], "w");
		setbuf (ispell_out, NULL);
#if	!defined( sun ) /* that ispell on sun gives no (c) msg */
		read (from[0], ignore_buf, 1024);
#endif
	release_buffer(ignore_buf);
	}
}


void ispell_done(void)
{
	if (ispell_pid != -1)
	{
		fprintf (ispell_out, "#\n");
		fclose (ispell_out);  close (from[0]);
		waitpid(ispell_pid, NULL, 0);
		ispell_pid = -1;
	}
}


char*	get_ispell_line (char *word)
{
	static char sbuf[ISPELL_BUF_SIZE];
	char *printbuf2;
	int len;
	if (ispell_pid == -1)
		return NULL;
	if (word)
	{
		fprintf (ispell_out, "^%s\n", word);
		fflush (ispell_out);
	}
	printbuf2 = get_buffer(MAX_STRING_LENGTH);
	len = read (from[0], printbuf2, ISPELL_BUF_SIZE);
	printbuf2[len] = '\0';
	/* Read up to max 1024 characters here */
	if (sscanf (printbuf2, "%" Stringify(ISPELL_BUF_SIZE) "[^\n]\n\n", sbuf) != 1) {
		release_buffer(printbuf2);
		return NULL;
	}
	release_buffer(printbuf2);
	return sbuf;
}


ACMD(do_ispell)
{
	if (!IS_NPC(ch))
		call_ispell(ch->desc, argument);
}


bool call_ispell (struct descriptor_data *d, char *argument)
{
	char *pc;

	if (ispell_pid <= 0) {
		write_to_output(d, TRUE, "&Rispell is not running.&n\r\n");
		return (FALSE);
	}

	skip_spaces(&argument);

	if (!argument[0] || strchr (argument, ' ')) {
		write_to_output(d, TRUE, "&RInvalid input.&n\r\n");
		return (FALSE);
	}
		
	if (argument[0] == '+') {
		for (pc = argument+1;*pc; pc++)
		if (!isalpha(*pc) || *pc == ' ') {
			write_to_output(d, TRUE, "&RERROR: '&W%c&R' is not a letter.&n\r\n", *pc);
			return (FALSE);
		}
		if (IS_ADMIN(d->character)) {
			char *printbuf = get_buffer(MAX_STRING_LENGTH);
			sprintf(printbuf, "*%s\n", argument+1);
			fprintf(ispell_out, printbuf);
			fflush(ispell_out);
			write_to_output(d, TRUE, "Added &W%s&n to the user dictionary.\r\n", argument+1);
			release_buffer(printbuf);
		}
		return (TRUE); /* we assume everything is OK.. better be so! */
	}

	pc = get_ispell_line(argument);

	if (!pc) {
		write_to_output(d, TRUE, "&Rispell: failed.&n\r\n");
		return (FALSE);
	}

	switch (pc[0]) {
		case '*':
		case '+': /* root */
		case '-': /* compound */
			write_to_output(d, TRUE, "Correct.\r\n");
			break;
		case '&': /* miss */
			write_to_output(d, TRUE, "Not found. Possible words: &W%s&n\r\n", strchr(pc, ':')+1);
			break;
		case '?': /* guess */
			write_to_output(d, TRUE, "Not found. Possible words: &W%s&n\r\n", strchr(pc, ':')+1);
			break;
		case '#': /* none */
			write_to_output(d, TRUE, "Unable to find anything that matches.\r\n");
			break;
		default:
			write_to_output(d, TRUE, "&rWeird output from ispell: &R%s&n\r\n", pc);
			return (FALSE);
	}
	return (TRUE);
}
