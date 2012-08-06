/***************************************************************************
*	 File: travels.c                               An addition to CircleMUD  *
*																																					 *
*	 Usage: booting, saving, and editing of                                  *
*					immortal travels.                                                *
*																																					 *
*	 By D. Tyler Barnes                                                      *
***************************************************************************/
/* $Id: travels.c,v 1.6 2002/03/28 11:54:01 arcanere Exp $ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "constants.h"

/* Local functions */
char **trav_pointer_addr(struct char_data *ch, int tnum);
void list_travels(struct char_data *ch);
void free_travels(struct char_data *ch);
void boot_travels(struct char_data *ch);
void save_travels(struct char_data *ch);
ACMD(do_travel);

void list_travels(struct char_data *ch) {

	char **temp, *trav;
	int i, tchar = 0;

	sprintf(buf,
	 "Current travel lines for %s\r\n"
	 "----------------------------------------\r\n", GET_NAME(ch)
	);
	send_to_char(buf, ch);
	
	for (i = 0; i < NUM_TRAVELS; i++) {
	
		switch(i) {
			case TRAV_TIN:
			case TRAV_TOUT:
				tchar = 'T';
				break;
			case TRAV_MIN:
			case TRAV_MOUT:
				tchar = 'M';
				break;
			case TRAV_GIN:
			case TRAV_GOUT:
				tchar = 'G';
				break;
			case TRAV_IIN:
			case TRAV_IOUT:
				tchar = 'I';
				break;
		}
		
		if ((temp = trav_pointer_addr(ch, i)))
			trav = *temp;
		else
			trav = NULL;
		
		sprintf(buf, "%c%-3s: %s", tchar, i % 2 ? "OUT" : "IN",
		 trav ? trav : travel_defaults[i]);
		 
		if (i == TRAV_MOUT && !trav)
			strcat(buf, "*dir*.");

		act(buf, TRUE, ch, 0, 0, TO_CHAR | TO_SLEEP);
	}
	send_to_char("----------------------------------------\r\n", ch);
}

ACMD(do_travel)	{

	char *text, **to_modify;
	bool free_struct = FALSE;

	text = one_argument(argument, arg);
	skip_spaces(&text);
	
	if (!*arg) {
		/* Travel Menu */
		send_to_char(
		"Travel Types\r\n"
		"------------------------------\r\n"
		"tin  - Teleport In\r\n"
		"tout - Teleport Out\r\n"
		"min  - Move In\r\n"
		"mout - Move Out: Use *dir* for direction of movement.\r\n"
		"gin  - Game In\r\n"
		"gout - Game Out\r\n"
		"iin  - Invis In\r\n"
		"iout - Invis Out\r\n"
		"------------------------------\r\n"
		"Example Syntax: travel gin Poe has entered the game.\r\n"
		"                travel mout Poe sprouts wings and flies *dir*.\r\n"
		"                travel mout\r\n"
		"(Specifying a travel type with no travel reverts it to default)\r\n"
		"Type 'travel list' to show all currently defined travels.\r\n"
		, ch); 
		return;
	}
	
	if (!GET_TRAVELS(ch)) {
		CREATE(GET_TRAVELS(ch), struct travel_data, 1);
		free_struct = TRUE;
	}
	
	if (!str_cmp(arg, "tin"))
		to_modify = &GET_TRAVELS(ch)->tin;
	else if (!str_cmp(arg, "tout"))
		to_modify = &GET_TRAVELS(ch)->tout;
	else if (!str_cmp(arg, "min"))
		to_modify = &GET_TRAVELS(ch)->min;
	else if (!str_cmp(arg, "mout"))  
		to_modify = &GET_TRAVELS(ch)->mout;
	else if (!str_cmp(arg, "gin"))
		to_modify = &GET_TRAVELS(ch)->gin;
	else if (!str_cmp(arg, "gout"))
		to_modify = &GET_TRAVELS(ch)->gout;
	else if (!str_cmp(arg, "iin"))
		to_modify = &GET_TRAVELS(ch)->iin;
	else if (!str_cmp(arg, "iout"))
		to_modify = &GET_TRAVELS(ch)->iout;
	else {
		if (!str_cmp(arg, "list"))
			list_travels(ch);
		else
			send_to_char("Invalid travel type specified!\r\n", ch);
			
		if (free_struct) {
			free(GET_TRAVELS(ch));
			GET_TRAVELS(ch) = NULL;
		}
		return;
	}
	
	if (*to_modify)
		free(*to_modify);
		
	if (!*text) {
		*to_modify = NULL;
		send_to_char("Travel reverted to default.\r\n", ch);
	} else {
		if (!strstr(text, GET_NAME(ch)))
			send_to_char("A travel MUST contain your player's name capitalized properly.\r\n", ch);
		else {
			*to_modify = str_dup(text);
			send_to_char(OK, ch);
		}
	}

	save_travels(ch);
}


char **trav_pointer_addr(struct char_data *ch, int tnum) {

	switch(tnum) {
		case TRAV_TIN:  return(&GET_TRAVELS(ch)->tin);
		case TRAV_TOUT: return(&GET_TRAVELS(ch)->tout);
		case TRAV_MIN:  return(&GET_TRAVELS(ch)->min);
		case TRAV_MOUT: return(&GET_TRAVELS(ch)->mout);
		case TRAV_GIN:  return(&GET_TRAVELS(ch)->gin);
		case TRAV_GOUT: return(&GET_TRAVELS(ch)->gout);
		case TRAV_IIN:  return(&GET_TRAVELS(ch)->iin);
		case TRAV_IOUT: return(&GET_TRAVELS(ch)->iout);
	}
	mudlog("SYSERR: trav_pointer_addr encountered unknown travel type.",
	 NRM, RIGHTS_DEVELOPER, TRUE);
	return(NULL);
	
}

void boot_travels(struct char_data *ch) {

	FILE *f;
	int i, numstrings;
	char **read_to;
	char fname[MAX_INPUT_LENGTH];
	
	if (GET_TRAVELS(ch))
		return;

	get_filename(GET_NAME(ch), fname, TRAV_FILE);

	if ((f = fopen(fname, "rb"))) {
		CREATE(GET_TRAVELS(ch), struct travel_data, 1);
		
		for (i = 0, numstrings = 0;; i++) {
		
			if (get_line(f, buf)) {
				read_to = trav_pointer_addr(ch, i);
				if (read_to && (strlen(buf) > 1 || buf[0] != '#')) {
					*read_to = strdup(buf);
					numstrings++;
				}
			} else
				break;
		}
		fclose(f);
		
		/* No valid strings read? we shouldn't have had a file to begin with.
		 * Unlink file and free memory.
		 */
		if (!numstrings) {
			unlink(fname);
			free(GET_TRAVELS(ch));
			GET_TRAVELS(ch) = NULL;
		}
	}
}

void save_travels(struct char_data *ch) {

	int i, strings = 0;
	FILE *f;
	char *save_str = NULL, **temp;
	char fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	
	if (IS_NPC(ch))
		return;
		
	get_filename(GET_NAME(ch), fname, TRAV_FILE);

	unlink(fname);

	if (!GET_TRAVELS(ch))
		return;
		
	if ((f = fopen(fname, "wb"))) {
	
		for (i = 0; i < NUM_TRAVELS; i++) {
			if ((temp = trav_pointer_addr(ch, i)))
				save_str = *temp;
			else
				save_str = NULL;
			fprintf(f, "%s\n", save_str ? save_str : "#");
			
			if (save_str)
				strings++;
		}
		fclose(f);
		
		if (!strings) {
			free_travels(ch);
			unlink(fname);
		}
	} else {
		sprintf(buf, "SYSERR: Could not open %s for write.", fname);
		mudlog(buf, NRM, RIGHTS_DEVELOPER, TRUE);
	}
}

/* Free all travel data attached to this character from memory */
void free_travels(struct char_data *ch) {

#if !BUFFER_MEMORY
	int i;
	char **tofree;
#endif

	if (!GET_TRAVELS(ch))
		return;

#if !BUFFER_MEMORY
	for (i = 0; i < NUM_TRAVELS; i++) {
		tofree = trav_pointer_addr(ch, i);
		if (tofree && *tofree)
			free(*tofree);
	}
#endif
	free(GET_TRAVELS(ch));
	GET_TRAVELS(ch) = NULL;
}
