/* **********************************************************************
*		File: email.c                                          Mercury 2.0b *
*	 Usage: Email command for in-game characters.                         *
*                                                                       *
*	 Based on original concept by: Brad Carps (Oberon) <leam@euridia.com> *
*	 Developed for Euridia by: Shane P. Lee (Fire)                        *
*	 Modified for Arcane Realms by: Torgny Bjers (Artovil)                *
*	 Copyright 2000 Shane P. Lee <tacodog21@yahoo.com>                    *
***********************************************************************	*/
/* $Id: email.c,v 1.12 2002/12/05 20:59:25 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "boards.h"
#include "genolc.h"
#include "oasis.h"
#include "constants.h"


/* ---------------------------------------------------------------------- */

void email_setup_new(struct descriptor_data *d)
{
	CREATE(OLC_EMAIL(d), struct email_data, 1);
	OLC_EMAIL(d)->from = str_dup(GET_EMAIL(d->character));
	OLC_EMAIL(d)->target = str_dup("Nobody.");
	OLC_EMAIL(d)->subject = str_dup("<no subject>");
	OLC_EMAIL(d)->body = str_dup("This message is blank.\r\n");
	email_disp_menu(d);
	OLC_VAL(d) = 0;
}

/* ---------------------------------------------------------------------- */

#define	SENDMAIL "/usr/bin/sendmail"

void email_save_to_disk(struct descriptor_data *d)
{
	FILE *fp;
	static char foot_buf[MAX_INPUT_LENGTH];
	
	sprintf(foot_buf, "\n\n"
		"+----------------------------------------------------------+\n"
		"| ARCANE REALMS MUD -- Medieval Fantasy Roleplaying        |\n"
		"| http://www.arcanerealms.org/                             |\n"
		"| telnet://arcanerealms.org:3011 -- Arcane Realms          |\n"
		"+----------------------------------------------------------+\n");


	sprintf(buf, "%s %s", SENDMAIL, OLC_EMAIL(d)->target);
	fp = popen(buf, "w");
	fprintf(fp, "From: %s\n", OLC_EMAIL(d)->from);
	fprintf(fp, "To: %s\n", OLC_EMAIL(d)->target);
	fprintf(fp, "Reply-To: %s\n", GET_EMAIL(d->character));
	/* Feel free to edit the following two fields */
	fprintf(fp, "X-Mailer: Arcane Realms MUD Mailing System <http://arcane-realms.dyndns.org/>\n");
	fprintf(fp, "Questions: This e-mail was sent by a player on our MUD.  For more info, please e-mail mudmail@arcane-realms.dyndns.org.\n");
	/* Feel free to edit the above two fields */
	fprintf(fp, "Subject: %s\n", OLC_EMAIL(d)->subject);
	fprintf(fp, "%s", OLC_EMAIL(d)->body);
	fprintf(fp, "%s", foot_buf);
	fprintf(fp, ".\n");
	pclose(fp);
	write_to_output(d, TRUE, "\r\nThank you for using the Mercury E-mail system!\r\n");
	mlog("(GC) %s (%s) emailed <%s>.", GET_NAME(d->character), GET_HOST(d->character), OLC_EMAIL(d)->target);
}

/* --------------------------- The main menu --------------------------- */

void email_disp_menu(struct descriptor_data *d)
{
	struct email_data *mail;
	
	get_char_colors(d->character);
	clear_screen(d);
	mail = OLC_EMAIL(d);
	
	write_to_output(d, TRUE,
		"\r\n--= Mercury Mail 2.0b =--\r\n"
					/* Out of respect, please leave the following messages in. */
		"Coded by: Fire -- Concept by: Oberon\r\n"
		"\r\n"
					/* Thank you :-) -spl */
		"%sF%s) From     : %s%s\r\n"
		"%sT%s) To       : %s%s\r\n"
		"%sS%s) Subject  : %s%s\r\n"
		"%sM%s) Message  : %s\r\n%s\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ",

		grn, nrm, yel, mail->from,
		grn, nrm, yel, mail->target,
		grn, nrm, yel, mail->subject,
		grn, nrm, yel, mail->body,
					grn, nrm
		);

	OLC_MODE(d) = EMAIL_MAIN_MENU;
}

/* ------------------------- The main loop ------------------------- */

void email_parse(struct descriptor_data *d, char *arg)
{
	switch (OLC_MODE(d)) {
	case EMAIL_CONFIRM_SAVESTRING:
		switch (*arg) {
		case 'y':
		case 'Y':
			email_save_to_disk(d);
			cleanup_olc(d, CLEANUP_STRUCTS);
			break;
		case 'n':
		case 'N':
			cleanup_olc(d, CLEANUP_ALL);
			break;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\nDo you wish to send this email? : ");
			break;
		}
		return;

	case EMAIL_MAIN_MENU:
		switch (*arg) {
		case 'q':
		case 'Q':
			if (OLC_VAL(d)) { /* Something has been modified. */
				write_to_output(d, TRUE, "Do you wish to send this email? : ");
				OLC_MODE(d) = EMAIL_CONFIRM_SAVESTRING;
			} else
				cleanup_olc(d, CLEANUP_ALL);
			return;
		case 'f':
		case 'F':
			write_to_output(d, TRUE, "Enter the name you wish to appear on the From field:-\r\n] ");
			OLC_MODE(d) = EMAIL_FROM;
			OLC_VAL(d) = 1;
			break;
		case 't':
		case 'T':
			write_to_output(d, TRUE, "Enter address of recipient:-\r\n] ");
			OLC_MODE(d) = EMAIL_2WHO;
			OLC_VAL(d) = 1;
			break;
		case 's':
		case 'S':
			write_to_output(d, TRUE, "Enter the message subject:-\r\n]");
			OLC_MODE(d) = EMAIL_SUBJECT;
			OLC_VAL(d) = 1;
			break;
		case 'm':
		case 'M':
			OLC_MODE(d) = EMAIL_BODY;
			clear_screen(d);
			write_to_output(d, TRUE, "%s", stredit_header);
			if (OLC_EMAIL(d)->body)
				write_to_output(d, TRUE, "%s", OLC_EMAIL(d)->body);
			string_write(d, &OLC_EMAIL(d)->body, MAX_STRING_LENGTH, 0, STATE(d));
			OLC_VAL(d) = 1;
			break;
		default:
			write_to_output(d, TRUE, "Invalid choice!\r\n");
			/* Seems setting a descriptor's mode to the same
			 * one as already exists crashes the game, so we
			 * fake it out and set it different, so email_disp_menu
			 * can set it all right again. -spl
			*/
			OLC_MODE(d) = EMAIL_CONFIRM_SAVESTRING;
			email_disp_menu(d);
			break;
		}
		return;

	case EMAIL_FROM:
		if (!genolc_checkstring(d, arg))
			break;
		if (OLC_EMAIL(d)->from)
			free(OLC_EMAIL(d)->from);
		/* Let's use redit's built-in limiter here */
		arg[MAX_ROOM_NAME - 1] = '\0';
		OLC_EMAIL(d)->from = str_udup(arg);
		break;

	case EMAIL_2WHO:
		if (!genolc_checkstring(d, arg))
			break;
		if (OLC_EMAIL(d)->target)
			free(OLC_EMAIL(d)->target);
		/* Let's use redit's built-in limiter here */
		arg[MAX_ROOM_NAME - 1] = '\0';
		OLC_EMAIL(d)->target = str_udup(arg);
		break;

	case EMAIL_SUBJECT:
		if (!genolc_checkstring(d, arg))
			break;
		if (OLC_EMAIL(d)->subject)
			free(OLC_EMAIL(d)->subject);
		/* Let's use redit's built-in limiter again */
		arg[MAX_ROOM_NAME - 1] = '\0';
		OLC_EMAIL(d)->subject = str_udup(arg);
		break;

	case EMAIL_BODY:
		/*
		 * We will NEVER get here, we hope.
		 */
		mudlog("SYSERR: Reached EMAIL_BODY case in Mercury! Report this to Head Coder!", BRF, RIGHTS_DEVELOPER, TRUE);
		break;

	default:
		/*
		 * We should never get here.
		 */
		mudlog("SYSERR: Reached default case in Mercury! Report this to Head Coder!", BRF, RIGHTS_DEVELOPER, TRUE);
		break;
	}
	/*
	 * If we get this far, something has been changed.
	 */
	OLC_VAL(d) = 1;
	email_disp_menu(d);
}

void email_string_cleanup(struct descriptor_data *d, int terminator)
{
	switch (OLC_MODE(d)) {
	case EMAIL_BODY:
		email_disp_menu(d);
		break;
	}
}

const	char *EMAIL_MENU =
	"\r\n"
	"You may receive electronic mail from Arcane Realms from time to time\r\n"
	"concerning the state of our MUD.  These mailings have a very low\r\n"
	"frequency, and contain important information regarding our server and\r\n"
	"its rules.\r\n";

const	char *MAILINGLIST_MENU =
	"\r\n"
	"On occasion we send out email regarding in-game events, and we use our\r\n"
	"internal mailing system to send out mail to our players before such events.\r\n"
	"This list is in no way mandatory.  &RWe do not share email addresses or\r\n"
	"other player info with third parties.&n\r\n";

const char *VERIFICATION_MENU =
	"\r\n"
	"At the end of creation an email will be sent out to the address you\r\n"
	"specify.  This mail will contain a link which you must click in order to\r\n"
	"activate your account.\r\n"
	"For more information: &Whttp://www.arcanerealms.org/email/&n\r\n";

const char *VERIFICATION_SENT =
	"\r\n"
	"&MAn email has been sent to you with a verification code.  Please follow the\r\n"
	"link in this email in order to activate your account.&n\r\n";
