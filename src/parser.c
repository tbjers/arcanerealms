/* $Id: parser.c,v 1.16 2004/03/20 16:29:07 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"

typedef	struct
	{
		int type;
		int value;
	} TOKEN;

#define	TOP 256
#define	WTOP 3
#define	NON_TERMINAL 0
#define	END_OF_INPUT 10
#define	MAX_ANSWER 32767
#define	SCR_WIDTH 79

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "handler.h"
#include "time.h"

void parse_pop (TOKEN * working, TOKEN * stack);
int	HasKet (char *s);
void master_parser (char *parsebuf, struct char_data *ch, struct room_data *room, struct obj_data *obj);
char strip_expression (char *word, char *expression_str, char *offset);
char get_numbers (char *expression_str, int *bracket_i, int *word_picked);
int	lexical_analyser (char *in_string, struct char_data *ch);
char lookup_value (char *variable, int *value, struct char_data *ch, struct room_data *room, struct obj_data *obj);
char calculate_value (char *variable, int *value);
int	syntax_analyser (TOKEN * input);
int	top_most_terminal (TOKEN * stack);
long parser_evaluate_expression (TOKEN * working);
int	power10 (int num);
void clean_up (char *in);
void parse_push (TOKEN * input, TOKEN * stack);

int	mob_present (struct char_data* ch, char *which);
int	obj_present (struct char_data* ch, char *which);

extern struct time_info_data time_info;
extern int is_abbrev(const char *arg1, const char *arg2);
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern mob_rnum real_mobile(mob_vnum vnum);
extern obj_rnum real_object(obj_vnum vnum);
extern char *one_argument(char *argument, char *first_arg);
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;

void parse_push (TOKEN * input, TOKEN * stack)
{
	stack[TOP].type++;
	stack[stack[TOP].type].type = input->type;
	stack[stack[TOP].type].value = input->value;
	return;
}

void parse_pop (TOKEN * working, TOKEN * stack)
{
	working[WTOP].type++;
	working[working[WTOP].type].type = stack[stack[TOP].type].type;
	working[working[WTOP].type].value = stack[stack[TOP].type].value;
	stack[TOP].type--;
	return;
}

int	HasKet (char *s)
{
	int i;

	for (i = 0; s[i] != 0; i++)
		{
			if (s[i] == ']')
				{
					return (1);
				}
		}
	return (0);
}

void master_parser	(char *parsebuf, struct char_data *ch, struct room_data *room, struct obj_data *obj)
{
	char *buf2p;
	char *str[10], *pos[10];
	int wordnum = 0;
	int i, j, k, l, m;
	int bracket_i, word_picked, picked;
	char any_true, offset, result;
	char *printbuf2 = get_buffer(4096);
	char *tmp = get_buffer(2048);
	char *expression_str = get_buffer(1024);
	int selection[25];
		int all_words, poss_words, p_words, bracket;

	bracket = 1;
	/* begin parse */
	while (HasKet (parsebuf))
		{
			wordnum = 0;
			poss_words = 0;
			p_words = 0;
			any_true = NO;
			for (j = 0; (parsebuf[j] != ']') && (parsebuf[j] != '\0'); j++);
			for (i = j; (parsebuf[i] != '[') && (i != 0); i--);
			for (k = 0; k < (j - i - 1); k++)
				printbuf2[k] = parsebuf[k + i + 1];
			printbuf2[k] = '\0';
			str[wordnum] = printbuf2;
			for (buf2p = printbuf2; *buf2p; buf2p++)
				{
					if (*buf2p == '\\')
						{
							*buf2p = '\0';
							wordnum++;
							str[wordnum] = buf2p + 1;
						}
				}
			for (all_words = 0; all_words < wordnum + 1; all_words++)
				{
					if ((result = strip_expression (str[all_words], expression_str,
																				 &offset)))
						{
							str[all_words] += offset;                /* Offset the string past the expression
																								 */
						}
					switch (result)
						{
						case 0:                /* No expression */
							/* Add to group of possibilities */
							str[poss_words] = str[all_words];
							poss_words++;
							any_true = YES;
							break;
						case 1:                /* A '!' */
							pos[p_words] = str[all_words];        /* Save to check later */
							p_words++;
							break;
						case 2:                /* An expression */
							if (lexical_analyser (expression_str, ch))
								{                /* If exp is true
																 */
									/* Add to group of possibilities */
									str[poss_words] = str[all_words];
									poss_words++;
									any_true = YES;
								}
							break;
						case 3:                /* A '#' */
							if (get_numbers (expression_str, &bracket_i, &word_picked))
								{
									/* If the word picked for the 'bracket_i'th bracket was
										 'word_picked' then make this a possibility */
									if (selection[bracket_i] == word_picked)
										{
											str[poss_words] = str[all_words];
											poss_words++;
											any_true = YES;
										}
								}
						}                        /* End Case */
				}                        /* End For */
			/* Now go through all the ! expressions and make them possibilities
				 if none of the other expressions were true */
			if ((!any_true) && (p_words > 0))
				{
					for (all_words = 0; all_words < p_words; all_words++)
						{
							str[poss_words] = pos[all_words];
							poss_words++;
						}
				}
			if (poss_words == 0)
				{
#if	0                                /* RD */
					/* If no words are possible, pick first as default */
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: No words were eligible, picking 1ast as default");
					selection[bracket] = 1;
					bracket++;
					strcpy (tmp, str[0]);
#endif
					*tmp = 0;
				}
			else
				{
					picked = (rand () % poss_words);
					selection[bracket] = picked + 1;
					bracket++;
					strcpy (tmp, str[picked]);
				}
			/* Now, between put tmp back into buf where it belongs. */
			for (l = i; l < (i + strlen (tmp)); l++)
				parsebuf[l] = tmp[l - i];
			for (m = l; (parsebuf[m] != '\0'); m++)
				parsebuf[m] = parsebuf[m + ((j + 1) - l)];	/* crashes here on occasion? (Artovil) */
			parsebuf[m] = '\0';
			/* Now run parsebuf through cleanup to distribute evenly on lines */
		}
	release_buffer(printbuf2);
	release_buffer(tmp);
	release_buffer(expression_str);
	clean_up (parsebuf);
}

char strip_expression (char *word, char *expression_str, char *offset)
{
	/* Strips the expression out of the word passed to it, and returns the
		 expression in expression_str, offset is the value that must be added
		 to the string passed to strip out the expression.  Also returns a
		 char representing what kind of expression there was:
		 0 - No Expression,  1 - !,  2 - ?,  3 - #
	 */
	char type, spaces;
	int i;

	spaces = 0;
	while (*word == ' ')
		{                                /* Skip spaces */
			word++;
			spaces++;
		}
	if (*word != '!' && *word != '?' && *word != '#')
		{
			return (0);                /* No expression */
		}
	/* Zip through string until we hit the ':' or end */
	for (i = 0; word[i] != ':' && word[i]; i++);
	if (word[i] == ':')
		{
			word[i] = 0;                /* Set end of string */
		}
	else
		/* No ':' found */
		{
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: %c found without a matching ':'", word[i]);
			return (0);
		}
	if (*word == '!')
		{
			type = 1;
		}
	else if (*word == '?')
		{
			type = 2;
		}
	else
		{
			type = 3;
		}
	word++;                        /* Skip over first character */
	strcpy (expression_str, word);
	*offset = spaces + i + 1;
	return (type);
}


char get_numbers	(char *expression_str, int *bracket_i, int
						 *word_picked)
{
	char number[3], new_str[20], *exp_str;
	int i;

	i = 0;
	exp_str = new_str;
	while (*expression_str)
		{                                /* Eat spaces, put in exp_str */
			if (*expression_str != ' ')
				{
					new_str[i] = *expression_str;
					i++;
				}
			expression_str++;
		}
	new_str[i] = 0;
	i = 0;
	while (*exp_str && isdigit (*exp_str) && i < 3)
		{                                /* Get first
																	 number */
			number[i] = *exp_str;
			i++;
			exp_str++;
		}
	if (*exp_str == 0)
		{                                /* If we're at  end of string already - error */
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: # Statement doesn't have 2 numbers.");
			return (0);
		}
	if (i == 3 && isdigit (*exp_str))
		{                                /* If number is bigger than 3
																	 digits - error */
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: First number too large in # statement.");
			return (0);
		}
	if (*exp_str != ',')
		{                                /* If numbers not separated by comma - error */
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Illegal character '%c' in # statement.", *exp_str);
			return (0);
		}
	if (i == 0)
		{
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Missing first number in # statement.");
			return (0);
		}
	/* Otherwise, convert to a number */
	number[i] = 0;
	calculate_value (number, bracket_i);
	exp_str++;                        /* Skip by comma */
	i = 0;
	while (*exp_str && isdigit (*exp_str) && i < 3)
		{                                /* Get second number */
			number[i] = *exp_str;
			i++;
			exp_str++;
		}
	if (*exp_str == 0 && i == 0)
		{                                /* If we're at  end of string  - error
																 */
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Second number missing in # statement.");
			return (0);
		}
	if (i == 3 && isdigit (*exp_str))
		{                                /* If number is bigger than 3
																	 digits - error */
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Second number too large in # statement.");
			return (0);
		}
	if (*exp_str != 0)
		{                                /* Illegal character */
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Illegal character '%c' in # statement.", *exp_str);
			return (0);
		}
	/*Otherwise, convert second number */
	number[i] = 0;
	calculate_value (number, word_picked);
	return (1);
}

/* Lexical Analyser:  Takes in an input string, converts to tokens, passes
	 tokens onto syntax analyser, then calculates and returns answer */
int	lexical_analyser (char *in_string, struct char_data *ch)
{
	TOKEN output[257];
	char *c, variable[25], token_found, error_found;
	 int  output_i, variable_i, state;
	int value, answer, prev_token, state_to_token_type[22] =
	{0, 7, 7, 18, 12, 11, 0, 0, 0, 16, 14, 13, 15, 5,
	 6, 17, 1, 3, 4, 8, 9, 2};

	state = 0;
	variable_i = 0;
	output_i = 0;
	token_found = NO;
	error_found = NO;
	prev_token = 0;
	c = in_string;
	while (*c != 0 && error_found == NO)
		{
			if (*c != ' ')
				{                        /*Skip spaces */
					switch (state)
						{
						case 0:
							if (isalpha (*c))
								{
									variable[variable_i] = *c;
									variable_i++;
									variable[variable_i] = 0;
									state = 1;
								}
							else if (isdigit (*c))
								{
									variable[variable_i] = *c;
									variable_i++;
									variable[variable_i] = 0;
									state = 2;
								}
							else
								{
									switch (*c)
										{
										case '!':
											state = 3;
											break;
										case '<':
											state = 4;
											break;
										case '>':
											state = 5;
											break;
										case '=':
											state = 6;
											break;
										case '&':
											state = 7;
											break;
										case '|':
											state = 8;
											break;
										case '-':
											state = 15;
											break;
										case '+':
											state = 16;
											break;
										case '*':
											state = 17;
											break;
										case '/':
											state = 18;
											break;
										case '(':
											state = 19;
											break;
										case ')':
											state = 20;
											break;
										default:
											error_found = YES;
										}
								}
							break;                /* end of case 0 */
						case 1:
							if (isalnum (*c))
								{
									variable[variable_i] = *c;
									variable_i++;
									variable[variable_i] = 0;
								}
							else
								{
									variable_i = 0;
									token_found = YES;
								}
							break;
						case 2:
							if (isdigit (*c))
								{
									variable[variable_i] = *c;
									variable_i++;
									variable[variable_i] = 0;
								}
							else
								{
									variable_i = 0;
									token_found = YES;
								}
							break;
						case 3:
						case 4:
						case 5:
							if (*c == '=')
								{
									state += 6;
								}
							else
								{
									token_found = YES;
								}
							break;
						case 6:
							if (*c == '=')
								{
									state = 12;
								}
							else
								{
									error_found = YES;
								}
							break;
						case 7:
							if (*c == '&')
								{
									state = 13;
								}
							else
								{
									error_found = YES;
								}
							break;
						case 8:
							if (*c == '|')
								{
									state = 14;
								}
							else
								{
									error_found = YES;
								}
							break;
						}                        /* end of switch (state) */
				}                        /* end of skip if *c == ' ' */
			if (state > 8 || token_found || *(c + 1) == 0)
				{
					value = 0;
					/* IF '-' is the token, make it a binary minus if previous token
						 was
						 ')' or an id/constant */
					if ((state == 15) && ((prev_token == 9) || (prev_token == 7)))
						{
							state = 21;
						}
					if (state == 1)
						{
				/*Following line was in orig. code, but room/obj not yet implemented*/
							/*error_found = lookup_value (variable, &value, ch, room, obj);*/
									error_found = lookup_value (variable, &value, ch, NULL, NULL);
						}
					if (state == 2)
						{
							error_found = calculate_value (variable, &value);
						}
					if (state_to_token_type[state] == 0)
						{
							error_found = YES;
						}
					else
						{
							output[output_i].type = state_to_token_type[state];
							output[output_i].value = value;
							prev_token = output[output_i].type;
							output_i++;
							state = 0;
						}
				}
			if (!token_found)
				{
					c++;
				}
			token_found = NO;
		}                                /*end of while */
	if (error_found)
		{
			/* Log error here */
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Error found - lexical parse aborted.");
			answer = 0;
		}
	else
		{
			/* add end of stream token */
			output[output_i].type = END_OF_INPUT;
			output[TOP].type = 0;
			if (output_i == 1)
				{                        /* If just 1 token */
					answer = output[0].value;
				}
			else
				{
					answer = syntax_analyser (output);
				}
		}
	return (answer);
}

/*************************************************************************
*																																				 *
*	 LOOKUP_VALUE:  Compares the string 'variable' passed to it with all   *
*	 the strings in 'var_list', if it finds a match, it uses a co-respond- *
*	 ing Macro to change the int '*value' passed to it; otherwise it sets  *
*	 '*value' to zero.  Returns 1 if no value set, or 0 if all is ok.      *
*																																				 *
*************************************************************************/
char
lookup_value (char *variable, int *value, struct char_data *ch, struct room_data *room, struct obj_data *obj)
#define	NUMBER_OF_VARS  36
#define	MOBCHOICE  500      /*just an arbitrary number*/
#define	OBJCHOICE  501      /*just an arbitrary number*/
{
	 char *var_list[NUMBER_OF_VARS] = {
			"strength", "agility"		, "precision"		, "perception", /* 0-3   */
			"health"	, "willpower"	, "intelligence", "charisma"	, /* 4-7   */
			"luck"		, "essence"		, "hour"				, "align"			, /* 8-11  */
			"cleric"	, "thief"			, "height"			, "weight"		, /* 12-15 */
			"night"		, "day"				, "warr"				, "magi"			,	/* 16-19 */
			"female"	, "male"			, "good"				, "evil"			,	/* 20-23 */
			"neutral"	, "age"				, "hunter"			, "winter"		,	/* 24-27 */
			"spring"	, "summer"		, "autumn"			, "faerie"		,	/* 28-31 */
			"divine"	, "infernal"	, "ancient"			,	"isic"				/* 32-34 */
	 };  
	 int i;
	 
/* If match found, break out of the loop - Note that variables
 * are case-sensitive, with strcmp  */

	 if (is_abbrev ("mob", variable)) i=MOBCHOICE;       /* added LR */
	 else if (is_abbrev ("obj", variable)) i=OBJCHOICE;  /* added LR */
	 else
		 {
			 for (i = 0; i < NUMBER_OF_VARS; i++) 
				 {
					 if (!strncmp (variable, var_list[i], strlen(variable))) 
						 {
									 break;
						 }
				}
		 }
	 /* Place Macros here to calculate value of variable, 0 is the first
		* variable, etc.  Note, may have to pass ch to this function to find
		* variables relating to character - such as str, dex, int, etc.     */
	 
	 switch (i) {
		case 0:
			*value = GET_STRENGTH(ch) / 100;
			break;
		case 1:
			*value = GET_AGILITY(ch) / 100;
			break;
		case 2:
			*value = GET_PRECISION(ch) / 100;
			break;
		case 3:
			*value = GET_PERCEPTION(ch) / 100;
			break;
		case 4:
			*value = GET_HEALTH(ch) / 100;
			break;
		case 5:
			*value = GET_WILLPOWER(ch) / 100;
			break;
		case 6:
			*value = GET_INTELLIGENCE(ch) / 100;
			break;
		case 7:
			*value = GET_CHARISMA(ch) / 100;
			break;
		case 8:
			*value = GET_LUCK (ch) / 100;
			break;
		case 9:
			*value = GET_ESSENCE(ch) / 100;
			break;
		case 10:
			*value = time_info.hours;
			break;
		case 11:
			*value = GET_ALIGNMENT(ch);
			break;
		case 12:
			*value = (IS_CLERIC_TYPE(ch));
			break;
		case 13:
			*value = (IS_THIEF_TYPE(ch));
			break;
		case 14:
			*value = GET_HEIGHT(ch);
			break;
		case 15:
			*value = GET_WEIGHT(ch);
			break;
		case 16:   /*night*/
			 *value = (weather_info.sunlight != SUN_LIGHT);
			break;
		case 17:   /*day*/
			 *value =  (weather_info.sunlight != SUN_DARK);
			break;
		case 18:
			*value = (IS_WARRIOR_TYPE(ch));
			break;
		case 19:
			*value = (IS_MAGI_TYPE(ch));
			break;
		case 20:
			*value = (GET_SEX(ch) == SEX_FEMALE);
			break;
		case 21:
			*value = (GET_SEX(ch) == SEX_MALE);
			break;
		case 22:   
			*value = IS_GOOD(ch);
		case 23:   
			*value = IS_EVIL(ch);
		case 24:   
			*value = IS_NEUTRAL(ch);
			break;
		case 25:
			*value = GET_AGE(ch);
			break;
		case 26:
			*value = (IS_THIEF_TYPE(ch));
			break;
		case 27:
			*value = in_season(SEASON_WINTER);
			break;
		case 28:
			*value = in_season(SEASON_SPRING);
			break;
		case 29:
			*value = in_season(SEASON_SUMMER);
			break;
		case 30:
			*value = in_season(SEASON_AUTUMN);
			break;
		case 31:
			*value = CAN_SEE_FAERIE(ch);
			break;
		case 32:
			*value = CAN_SEE_DIVINE(ch);
			break;
		case 33:
			*value = CAN_SEE_INFERNAL(ch);
			break;
		case 34:
			*value = CAN_SEE_ANCIENT(ch);
			break;
		case 35:
			*value = IS_IC(ch);
			break;
		case MOBCHOICE:  /* added LR */
			*value = mob_present(ch, variable);
			break;
		case OBJCHOICE: /* added LR */
			*value = obj_present(ch, variable);
			break;

		default:
			*value = 0;
			/* Log error or whatever here */
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Couldn't find variable - '%s'", variable);
			return (YES);
	 }
	 return (NO);
}

/*************************************************************************
*																																				 *
*	 CALCULATE_VALUE:  Converts the string 'variable' passed to it into a  *
*	 number which it stores in '*variable' - returns YES if the number     *
*	 exceeded 32767, and sets the value to 32767, otherwise, it returns NO *
*																																				 *
*************************************************************************/
char
calculate_value	(char *variable, int *value)
{
	char was_error;
	int len, i;
	long calc_value;

	was_error = NO;
	len = strlen (variable);
	calc_value = 0;
	for (i = len - 1; i >= 0; i--)
		{
			calc_value += ((variable[i] - 48) * power10 (len - i - 1));
			if (calc_value > MAX_ANSWER)
				{
					/* Log error message here */
					extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Varaible overflow - setting to MAX_ANSWER");
					calc_value = MAX_ANSWER;
					/* was_error = YES ; */
					break;
				}
		}
	*value = calc_value;
	return (was_error);
}

int
syntax_analyser	(TOKEN * input)
{
	TOKEN stack[257], working[4], answer;
	char f[19] =
	{0, 11, 11, 13, 13, 5, 3, 16, 0, 16, 0, 9, 9, 9, 9, 7, 7, 14, 14};
	char g[19] =
	{0, 10, 10, 12, 12, 4, 2, 17, 17, 0, 0, 8, 8, 8, 8, 6, 6, 15, 15};

	stack[TOP].type = 0;
	stack[0].type = END_OF_INPUT;
	while (input->type != END_OF_INPUT || stack[TOP].type > 1)
		{
			if (f[top_most_terminal (stack)] <= g[input->type])
				{
					parse_push (input, stack);
					input++;
				}
			else
				{
					working[WTOP].type = -1;
					parse_pop (working, stack);
					while (f[top_most_terminal (stack)] >=
								 g[working[working[WTOP].type].type])
						{
							parse_pop (working, stack);
						}
					if (stack[stack[TOP].type].type == NON_TERMINAL)
						{
							parse_pop (working, stack);
						}
					answer.type = NON_TERMINAL;
					answer.value = parser_evaluate_expression (working);
					parse_push (&answer, stack);
				}
		}
	return (answer.value);
}

int
top_most_terminal	(TOKEN * stack)
{
	int index;

	index = stack[TOP].type;
	while (stack[index].type == NON_TERMINAL)
		{
			index--;
		}
	return (stack[index].type);
}

long
parser_evaluate_expression (TOKEN * working)
{
	long answer = 0;

	if (working[WTOP].type == 2)
		{                                /* 3 operators */
			if (working[0].type == NON_TERMINAL)
				{                        /* It's an E op E */
					switch (working[1].type)
						{
						case 1:
							answer = (long) working[2].value + (long) working[0].value;
							if (answer > MAX_ANSWER)
								{
									extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Answer would exceed Max, truncating...");
									answer = MAX_ANSWER;
								}
							if (answer < -MAX_ANSWER)
								{
									extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Answer would be below -Max, truncating...");
									answer = -MAX_ANSWER;
								}
							break;
						case 2:
							answer = (long) working[2].value - (long) working[0].value;
							if (answer > MAX_ANSWER)
								{
									extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Answer would exceed Max, truncating...");
									answer = MAX_ANSWER;
								}
							if (answer < -MAX_ANSWER)
								{
									extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Answer would be below -Max, truncating...");
									answer = -MAX_ANSWER;
								}
							break;
						case 3:
							answer = (long) working[2].value * (long) working[0].value;
							if (answer > MAX_ANSWER)
								{
									extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Answer would exceed Max, truncating...");
									answer = MAX_ANSWER;
								}
							if (answer < -MAX_ANSWER)
								{
									extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Answer would be below -Max, truncating...");
									answer = -MAX_ANSWER;
								}
							break;
						case 4:
							if (working[0].value == 0)
								{
									extended_mudlog(BRF, SYSL_BUGS, TRUE, "SPARSER: Cannot divide by zero, setting answer to 0...");
									answer = 0;
								}
							else
								{
									answer = working[2].value / working[0].value;
								}
							break;
						case 5:
							answer = working[2].value && working[0].value;
							break;
						case 6:
							answer = working[2].value || working[0].value;
							break;
						case 11:
							answer = working[2].value > working[0].value;
							break;
						case 12:
							answer = working[2].value < working[0].value;
							break;
						case 13:
							answer = working[2].value >= working[0].value;
							break;
						case 14:
							answer = working[2].value <= working[0].value;
							break;
						case 15:
							answer = working[2].value == working[0].value;
							break;
						case 16:
							answer = working[2].value != working[0].value;
							break;
						}                        /* End switch */
				}
			else
				/* It's (E)  */
				{
					answer = working[1].value;
				}
		}
	if (working[WTOP].type == 1)
		{                                /* 2 operators */
			if (working[1].type == 17)
				{                        /* Unary Minus */
					answer = -working[0].value;
				}
			else
				/* ! sign */
				{
					answer = !working[0].value;
				}
		}
	if (working[WTOP].type == 0)
		{                                /* 1 operator */
			answer = working[0].value;
		}
	return (answer);
}

int
power10	(int num)
{
	int i;
	int value;

	value = 1;

	for (i = 0; i < num; i++)
		{
			value *= 10;
		}

	return (value);

}


#define	RETURN_CHAR '|'

void clean_up (char *in)
{
	char *parsebuf2 = get_buffer(MAX_STRING_LENGTH);
	int notdone, linelen, bufi, ini, wordlen, whitespacelen, exit, word,
		letter, endspacelen, colorlen = 0;
	int width = 0;

	if (!width)
		width = SCR_WIDTH;

	notdone = 1;
	linelen = 0;
	bufi = 0;
	ini = 0;
	word = 0;
	colorlen = 0;
	strcpy (parsebuf2, in);

	while (notdone) {

		wordlen = 0;
		whitespacelen = 0;
		endspacelen = 0;
		colorlen = 0;
		word++;

		/* snarf initial whitespace */       
		while (parsebuf2[bufi] == ' ') {
			in[ini++] = parsebuf2[bufi++];
			whitespacelen++;
		}

		exit = 0;
		letter = 0;

		while (parsebuf2[bufi] != ' ' && !exit) {

			letter++;

			switch (parsebuf2[bufi]) {
			case '\r':
			case '\n':
				if (letter > 1 && word > 1) {
					in[ini++] = ' ';
					whitespacelen++;
				}
				endspacelen++;
				bufi++;
				while (parsebuf2[bufi] == '\r' || parsebuf2[bufi] == '\n') {
					bufi++;
					endspacelen++;
				}
				exit = 1;
				break;
			case '\0':
				notdone = 0;
				linelen = 0;
				in[ini] = '\0';
				exit = 1;
				break;
			case RETURN_CHAR:
				if (letter > 1)
					exit = 1;
				else {
					in[ini++] = '\r';
					in[ini++] = '\n';
					bufi++;
					word = 0;
					linelen = 0;
					endspacelen = 0;
					exit = 1;
				}
				break;
			case '&':
				if (parsebuf2[bufi + 1] != '&')
					colorlen += 2;
				else
					colorlen++;
				in[ini++] = parsebuf2[bufi++];
				wordlen++;
				break;
			default:
				in[ini++] = parsebuf2[bufi++];
				wordlen++;
			}

		}

		linelen += (wordlen + whitespacelen - colorlen);

		if (linelen >= width) {
			bufi -= (wordlen + endspacelen);
			ini -= (wordlen + whitespacelen);

			in[ini++] = '\r';
			in[ini++] = '\n';
			linelen = 0;
			word = 0;
		}

	} /* while (notdone) */

	release_buffer(parsebuf2);

}



/***************************************************************/
/* My additions (Linda Raymond) */

int	mob_present (struct char_data* ch, char *which)
{
	mob_vnum i=0;
	char mobname[50];
	char *tempstr;
	mob_rnum exist=0;
	struct char_data *junk=NULL;

	which=which+3;   /*remove "MOB" from variable passed*/
	i=atoi(which);   /*i now = vnum of mob to check for*/

	exist=real_mobile(i);
	if (exist<1) return (0);  /*real_x returns -1 on failure*/

	tempstr=str_dup(mob_proto[exist].player.name);      
	one_argument (tempstr, mobname);

	junk = get_char_vis(ch, mobname, NULL, FIND_CHAR_ROOM, 0);

	if (junk) return (1);   
	else return(0);
}


int	obj_present (struct char_data* ch, char *which)
{
	obj_vnum i=0;
	char *tempstr;
	char objname[50];
	obj_rnum exist=0;
	int isthere=0;
	struct obj_data *junk=NULL;
	struct char_data *ch2;

	which=which+3;   /*remove "OBJ" from variable passed*/
	i=atoi(which);   /*i now = vnum of obj to check for*/

	exist=real_object(i);
	
	if (exist<1) return (0);  /*real_x returns -1 on failure*/

	tempstr=str_dup(obj_proto[exist].name);      
	one_argument (tempstr, objname);

	isthere=generic_find(objname, FIND_OBJ_ROOM, ch, &ch2, &junk);      

	if (isthere) return (1);   
		else return(0);
}

