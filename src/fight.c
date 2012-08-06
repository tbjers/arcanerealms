/* ************************************************************************
*		File: fight.c                                       Part of CircleMUD *
*	 Usage: Combat system                                                   *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: fight.c,v 1.45 2003/06/11 02:42:40 cheron Exp $ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "events.h"
#include "spells.h"
#include "skills.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "quest.h"
#include "characters.h"

/* Structures */
struct char_data *combat_list = NULL;        /* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern struct aq_data *aquest_table;
extern struct weapon_prof_data wprof[];
extern struct descriptor_data *descriptor_list;
extern struct char_data *mob_proto;
extern int pk_allowed;        /* see config.c */
extern int auto_save;         /* see config.c -- not used in this file */
extern int max_exp_gain;      /* see config.c */
extern int max_exp_loss;      /* see config.c */
extern int max_npc_corpse_time, max_pc_corpse_time;
extern int summon_allowed;    /*       "      */
extern int sleep_allowed;     /*       "      */
extern int charm_allowed;     /*       "      */
extern int free_rent;
extern int make_player_corpse;
extern int soft_player_death;

/* External procedures */
char *fread_action(FILE * fl, int nr);
int	ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);
int	apply_pd(struct char_data *ch, int eq_pos);
ACMD(do_flee);
ACMD(do_sac);
ACMD(do_get);
ACMD(do_split);

/* local functions */
void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
void dam_message_brief(int dam, struct char_data *ch, struct char_data *victim,int w_type);
void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type);
void appear(struct char_data *ch);
void load_messages(void);
void free_messages(void);
void free_messages_type(struct msg_type *msg);
void make_corpse(struct char_data *ch);
void kill_player(struct char_data *ch);
void death_cry(struct char_data *ch);
void raw_kill(struct char_data *ch, struct char_data *killer);
void die(struct char_data *ch, struct char_data *killer);
void group_gain(struct char_data *ch, struct char_data *victim);
void solo_gain(struct char_data *ch, struct char_data *victim);
void perform_violence(void);
void diag_char_to_char(struct char_data *i, struct char_data *ch);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
int	compute_armor_class(struct char_data *ch);
void hit2(struct char_data *ch, struct char_data *victim, int type);
int	thaco(struct char_data *ch);
int calculate_attack_score(struct char_data *ch, struct obj_data *wielded, int w_type, int skill);
int calculate_defense_score(struct char_data *victim, int position);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
	{"hit", "hits"},                /* 0 */
	{"sting", "stings"},
	{"whip", "whips"},
	{"slash", "slashes"},
	{"bite", "bites"},
	{"bludgeon", "bludgeons"},        /* 5 */
	{"crush", "crushes"},
	{"pound", "pounds"},
	{"claw", "claws"},
	{"maul", "mauls"},
	{"thrash", "thrashes"},        /* 10 */
	{"pierce", "pierces"},
	{"blast", "blasts"},
	{"punch", "punches"},
	{"stab", "stabs"}
};

#define	IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */

void appear(struct char_data *ch)
{
	if (affected_by_spell(ch, SPELL_INVISIBLE))
		affect_from_char(ch, SPELL_INVISIBLE);

	REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);

	if (IS_NPC(ch) || IS_MORTAL(ch))
		act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
	else {
		if (GET_TRAVELS(ch) && GET_TRAVELS(ch)->iin)
			 strcpy(buf, GET_TRAVELS(ch)->iin);
		else
			 strcpy(buf, travel_defaults[TRAV_IIN]);
		act(buf, FALSE, ch, 0, 0, TO_ROOM);
	}
}


int	compute_armor_class(struct char_data *ch)
{
	int armorclass = GET_PD(ch);

	if (AWAKE(ch))
		armorclass += dex_app[GET_AGILITY(ch) / 100].defensive * 10;

	if (!IS_NPC(ch)) {
		if (GET_POS(ch) == POS_DODGE) /* bonus if dodging */
			armorclass -= ((GET_SKILL(ch, SKILL_DODGE)/100) / (number(1, 10)));

		if (GET_POS(ch) == POS_DEFEND) /* bonus if defending */
			armorclass -= ((GET_SKILL(ch, SKILL_DEFEND)/100) / (number(1, 5)));

		if (GET_POS(ch) == POS_WATCHING) /* bonus if alert */
			armorclass -= ((GET_SKILL(ch, SKILL_WATCH)/100) / (number(1, 3)));
	}

	return (MAX(-100, armorclass));      /* -100 is lowest */
}


char *fread_action(FILE * fl, int nr)
{
	char buf[MAX_STRING_LENGTH];

	fgets(buf, MAX_STRING_LENGTH, fl);
	if (feof(fl)) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "fread_action: unexpected EOF near action #%d", nr);
		kill_mysql();
		exit(1);
	}
	if (*buf == '#')
		return (NULL);

	buf[strlen(buf) - 1] = '\0';
	return (str_dup(buf));
}


void free_messages_type(struct msg_type *msg)
{
	if (msg->attacker_msg)	free(msg->attacker_msg);
	if (msg->victim_msg)		free(msg->victim_msg);
	if (msg->room_msg)		free(msg->room_msg);
}


void free_messages(void)
{
	int i;

	for (i = 0; i < MAX_MESSAGES; i++)
		while (fight_messages[i].msg) {
			struct message_type *former = fight_messages[i].msg;

			free_messages_type(&former->die_msg);
			free_messages_type(&former->miss_msg);
			free_messages_type(&former->hit_msg);
			free_messages_type(&former->god_msg);

			fight_messages[i].msg = fight_messages[i].msg->next;
			free(former);
		}
}


void load_messages(void)
{
	FILE *fl;
	int i, type;
	struct message_type *messages;
	char chk[128];

	if (!(fl = fopen(MESS_FILE, "r"))) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
		kill_mysql();
		exit(1);
	}
	for (i = 0; i < MAX_MESSAGES; i++) {
		fight_messages[i].a_type = 0;
		fight_messages[i].number_of_attacks = 0;
		fight_messages[i].msg = NULL;
	}


	fgets(chk, 128, fl);
	while (!feof(fl) && (*chk == '\n' || *chk == '*'))
		fgets(chk, 128, fl);

	while (*chk == 'M') {
		fgets(chk, 128, fl);
		sscanf(chk, " %d\n", &type);
		for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
				 (fight_messages[i].a_type); i++);
		if (i >= MAX_MESSAGES) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Too many combat messages.  Increase MAX_MESSAGES and recompile.");
			kill_mysql();
			exit(1);
		}
		CREATE(messages, struct message_type, 1);
		fight_messages[i].number_of_attacks++;
		fight_messages[i].a_type = type;
		messages->next = fight_messages[i].msg;
		fight_messages[i].msg = messages;

		messages->die_msg.attacker_msg = fread_action(fl, i);
		messages->die_msg.victim_msg = fread_action(fl, i);
		messages->die_msg.room_msg = fread_action(fl, i);
		messages->miss_msg.attacker_msg = fread_action(fl, i);
		messages->miss_msg.victim_msg = fread_action(fl, i);
		messages->miss_msg.room_msg = fread_action(fl, i);
		messages->hit_msg.attacker_msg = fread_action(fl, i);
		messages->hit_msg.victim_msg = fread_action(fl, i);
		messages->hit_msg.room_msg = fread_action(fl, i);
		messages->god_msg.attacker_msg = fread_action(fl, i);
		messages->god_msg.victim_msg = fread_action(fl, i);
		messages->god_msg.room_msg = fread_action(fl, i);
		fgets(chk, 128, fl);
		while (!feof(fl) && (*chk == '\n' || *chk == '*'))
			fgets(chk, 128, fl);
	}

	fclose(fl);
}


void update_pos(struct char_data *victim)
{
	if (GET_HIT(victim) <= 5) {
		if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_TARGET))	{
				GET_POS(victim) = POS_REPLACE;
				return;
		}
	}

	if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
		return;
	else if (GET_HIT(victim) > 0) {
		if (GET_POS(victim) == POS_DEFEND)
			return;
		else
			GET_POS(victim) = POS_STANDING;
	} else if (GET_HIT(victim) > 0 && GET_POS(victim) == POS_DEFEND)
		GET_POS(victim) = POS_DEFEND;
	else if (GET_HIT(victim) <= -11)
		GET_POS(victim) = POS_DEAD;
	else if (GET_HIT(victim) <= -6)
		GET_POS(victim) = POS_MORTALLYW;
	else if (GET_HIT(victim) <= -3) 
		GET_POS(victim) = POS_INCAP;
	else {
		GET_POS(victim) = POS_STUNNED;
	}
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
	if (ch == vict)
		return;

	if (FIGHTING(ch)) {
		core_dump();
		return;
	}

	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_TARGET))
		return;

	ch->next_fighting = combat_list;
	combat_list = ch;

	if (AFF_FLAGGED(ch, AFF_SLEEP))
		affect_from_char(ch, SPELL_SLEEP);

	FIGHTING(ch) = vict;
	GET_POS(ch) = POS_FIGHTING;
}


/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
	struct char_data *temp;

	if (ch == next_combat_list)
		next_combat_list = ch->next_fighting;

	REMOVE_FROM_LIST(ch, combat_list, next_fighting);
	ch->next_fighting = NULL;
	FIGHTING(ch) = NULL;
	GET_POS(ch) = POS_STANDING;
	update_pos(ch);
}


void make_corpse(struct char_data *ch)
{
  struct obj_data *corpse, *o;
	struct obj_data *money;
  int i, j = 0, k = 0;

	/*
	 * If this corpse has been flagged MOB_RESOURCE, we try to load the corpse
	 * object for the mobile if possible, otherwise we default to the normal
	 * way of creating corpses.
	 */
	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_RESOURCE) && GET_MOB_VAL(ch, 0) > 0 && (corpse = read_object(GET_MOB_VAL(ch, 0), VIRTUAL)) != NULL) {

		IN_ROOM(corpse) = NOWHERE;

	} else {

		corpse = create_obj();

		corpse->item_number = NOTHING;
		IN_ROOM(corpse) = NOWHERE;
		corpse->name = str_dup("corpse");

		sprintf(buf2, "The corpse of %s is lying here.", GET_NAME(ch));
		corpse->description = str_dup(buf2);

		sprintf(buf2, "the corpse of %s", GET_NAME(ch));
		corpse->short_description = str_dup(buf2);

	}

	GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
	GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
	GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
	GET_OBJ_VAL(corpse, 0) = 0;        /* You can't store stuff in a corpse */
	GET_OBJ_VAL(corpse, 3) = 1;        /* corpse identifier */
	GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
	GET_OBJ_RENT(corpse) = 100000;
	if (IS_NPC(ch))
		GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
	else
		GET_OBJ_TIMER(corpse) = max_pc_corpse_time;

	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_RESOURCE)) {
		/*
		 * If we have resources, we check if there's a real object, and if so,
		 * we load the object into the inventory of the deceased mobile, which
		 * will later be transfered to the corpse contents.
		 */
		if (GET_MOB_VAL(ch, 1) > 0 && GET_MOB_VAL(ch, 1) < 11) {
			/* value[2] and value[3] contains vnums of resource objects. */
			for (i=2; i < 4; i++) {
				j = number(1, GET_MOB_VAL(ch, 1)); /* Randomize the number of created objects. */
				for (k=0; k < j; k++)
					if (GET_MOB_VAL(ch, i) > 0 && (o = read_object(GET_MOB_VAL(ch, i), VIRTUAL)) != NULL)
						obj_to_char(o, ch); /* This is OK. */
			}
		}
	}

	if (IS_NPC(ch)) {
		/* transfer mobile's inventory to the corpse */
		corpse->contains = ch->carrying;
		for (o = corpse->contains; o != NULL; o = o->next_content)
			o->in_obj = corpse;
		object_list_new_owner(corpse, NULL);

		/* transfer mobile's equipment to the corpse */
		for (i = 0; i < NUM_WEARS; i++)
			if (GET_EQ(ch, i))
				obj_to_obj(unequip_char(ch, i), corpse);
	}

	/* transfer gold to corpse */
	if (GET_GOLD(ch) > 0) {
		/*
		 * following 'if' clause added to fix gold duplication loophole
		 * The above line apparently refers to the old "partially log in,
		 * kill the game character, then finish login sequence" duping
		 * bug. The duplication has been fixed (knock on wood) but the
		 * test below shall live on, for a while. -gg 3/3/2002
		 */
		if ((IS_NPC(ch) && !IS_ANIMAL(ch)) || ch->desc) {
			money = create_money(GET_GOLD(ch));
			obj_to_obj(money, corpse);
		}
		GET_GOLD(ch) = 0;
	}

	if (!IS_NPC(ch)) {
		/* We save the PC's equipment */
		if (free_rent)
			crash_datasave(ch, 0, RENT_RENTED);
	} else {
		/* Empty the mob's inventory and reset their weights. */
		ch->carrying = NULL;
		IS_CARRYING_N(ch) = 0;
		IS_CARRYING_W(ch) = 0;
	}

	obj_to_room(corpse, IN_ROOM(ch));
}

/*
 * ... wtf is this?
 * Why are we removing their gold?
 * extract_char(_soft) takes care of saving ... anyway ...
 * There's no use for this?
 * -Cheron
 */
void kill_player(struct char_data *ch)
{
	GET_GOLD(ch) = 0;

	/* We save the PC's equipment */
	if (free_rent)
		crash_datasave(ch, 0, RENT_RENTED);
}


void death_cry(struct char_data *ch)
{
	int door;

	if (GET_ALIGNMENT(ch) > 300)
		act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);
	else if (GET_ALIGNMENT(ch) < -300)
		act("You take delight in $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);

	for (door = 0; door < NUM_OF_DIRS; door++)
		if (CAN_GO(ch, door))
			send_to_room("You hear someone's death cry.\r\n",
									 world[IN_ROOM(ch)].dir_option[door]->to_room);
}


void raw_kill(struct char_data *ch, struct char_data *killer)
{
  struct follow_type *group;

	if (FIGHTING(ch))
		stop_fighting(ch);

	while (ch->affected)
		affect_remove(ch, ch->affected);

	/* To make ordinary commands work in scripts.  welcor*/
	GET_POS(ch) = POS_STANDING; 

	if (killer) {
		if (death_mtrigger(ch, killer))
			death_cry(ch);
		if (!IS_NPC(killer) && GET_TELLS(killer, 0) && SESS_FLAGGED(killer, SESS_HAVETELLS))
			send_to_char("\007&RYou have buffered tells.&n\r\n", killer);
	} else
			death_cry(ch);
	
	if (killer)
		autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL);    

	/*
	 * Cheron did not want corpse creation.
	 * If !make_player_corpse then only mobiles create corpses.
	 * Artovil, 2003-06-01
	 *
	 * Uh, we either make a corpse or not.  What's this kill_player thing?
	 * Cheron, 2003-06-10
	 */
	if (IS_NPC(ch) || make_player_corpse)
		make_corpse(ch);

	/*
	 * Should we just move them to the OOC lounge when they die?
	 * Or boot them to the main menu the old-fashioned way?
	 * Artovil, 2003-06-02
	 * 
	 * Of course, NPCs are gonna get extracted regardless.
	 * Cheron, 2003-06-10
	 */
	if (!IS_NPC(ch) && soft_player_death)
		extract_char_soft(ch);
	else
		extract_char(ch);
	
	if (killer)
		autoquest_trigger_check(killer, NULL, NULL, AQ_MOB_SAVE);

  if (killer) {
    if (AFF_FLAGGED(killer, AFF_GROUP)) {
      if (killer->master) {
        stop_fighting(killer->master);
        for (group = killer->master->followers; group; group = group->next) {
          if (AFF_FLAGGED(group->follower, AFF_GROUP) && IN_ROOM(group->follower) == IN_ROOM(killer))
            stop_fighting(group->follower);
        }
      } else {
        stop_fighting(killer);
        for (group = killer->followers; group; group = group->next) {
          if (AFF_FLAGGED(group->follower, AFF_GROUP) && IN_ROOM(group->follower) == IN_ROOM(killer))
            stop_fighting(group->follower);
        }
      }
    } else
      stop_fighting(killer);
  }
}


void die(struct char_data *ch, struct char_data *killer)
{
	/* This void die by the Wintermute guys. Credit where credit is due... (artovil) */

	if (IS_NPC(ch))	{
		if (MOB_FLAGGED(ch, MOB_TARGET))
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Target mob died...");
	}
	
	if (!IS_NPC(ch) && IS_NPC(killer)) {
    gain_exp(ch, -MIN(max_exp_loss,(number( (GET_EXP(ch) / 5), (GET_EXP(ch) / 2) ))));
		GET_MOVE(ch) = 0;
	}
	
	if (!IS_NPC(ch)) {
		REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
		REMOVE_BIT(SESSION_FLAGS(ch), SESS_IC);
	}
	
	raw_kill(ch, killer);
}


void perform_group_gain(struct char_data *ch, int base,
														 struct char_data *victim)
{
	int share;

	share = MIN(max_exp_gain, MAX(1, base));

	gain_exp(ch, share);
}


void group_gain(struct char_data *ch, struct char_data *victim)
{
	int tot_members, base, tot_gain;
	struct char_data *k;
	struct follow_type *f;

	if (!IS_NPC(victim))
		return;

	if (!(k = ch->master))
		k = ch;

	if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
		tot_members = 1;
	else
		tot_members = 0;

	for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch))
			tot_members++;

	/* round up to the next highest tot_members */
	tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;

	/* prevent illegal xp creation when killing players */
	if (!IS_NPC(victim))
		tot_gain = MIN(max_exp_loss * 2 / 3, tot_gain);

	if (tot_members >= 1)
		base = MAX(1, tot_gain / tot_members);
	else
		base = 0;

	if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch))
		perform_group_gain(k, base, victim);

	for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch))
			perform_group_gain(f->follower, base, victim);
}


void solo_gain(struct char_data *ch, struct char_data *victim)
{
	int exp;

	if (!IS_NPC(victim))
		return;

	exp = MIN(max_exp_gain, GET_EXP(victim) / 3);

	/* Calculate difference bonus */
  //LEVEL: replace with MAX(0, (exp * MIN(4, (GET_FAME(victim) - GET_FAME(ch)) * GET_DIFFICULTY(victim))) /8)
	if (IS_NPC(ch))
		exp += MAX(0, (exp * MIN(4, ((GET_SKILL(victim, SKILL_MELEE)/100) - (GET_SKILL(ch, SKILL_MELEE)/100)))) / 8);
	else {
		if (IS_NPC(victim))
			exp += MAX(0, (exp * MIN(8, ((GET_SKILL(victim, SKILL_MELEE)/100) - (GET_SKILL(ch, SKILL_MELEE)/100))) * GET_DIFFICULTY(victim)) / 8);
		else
			exp += MAX(0, (exp * MIN(8, ((GET_SKILL(victim, SKILL_MELEE)/100) - (GET_SKILL(ch, SKILL_MELEE)/100)))) / 8);
	}

	exp = MAX(exp, 1);

	gain_exp(ch, exp);
}


char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
	static char buf[256];
	char *cp = buf;

	for (; *str; str++) {
		if (*str == '#') {
			switch (*(++str)) {
			case 'W':
				for (; *weapon_plural; *(cp++) = *(weapon_plural++));
				break;
			case 'w':
				for (; *weapon_singular; *(cp++) = *(weapon_singular++));
				break;
			default:
				*(cp++) = '#';
				break;
			}
		} else
			*(cp++) = *str;

		*cp = 0;
	}                                /* For */

	return (buf);
}


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data *ch, struct char_data *victim,int w_type)
{
  char *actbuf = get_buffer(MAX_STRING_LENGTH);
  /* damage message to damager */
	sprintf(actbuf, "&yYou attack $N:&n [%s]", (dam > 0) ? "&gHIT&n" : "&rMISS&n");
	act(actbuf, FALSE, ch, NULL, victim, TO_CHAR);
  /* damage message to damager group */
	sprintf(actbuf, "&g$n attacks $N:&n [%s]", (dam > 0) ? "&gHIT&n" : "&rMISS&n");
	act(actbuf, FALSE, ch, NULL, victim, TO_GROUP);
  /* damage message to damagee */
	sprintf(actbuf, "&r$n attacks you:&n [%s]", (dam > 0) ? "&gHIT&n" : "&rMISS&n");
	act(actbuf, FALSE, ch, NULL, victim, TO_VICT);
  /* damage message to damagee group */
	sprintf(actbuf, "$N attacks $n: [%s]", (dam > 0) ? "&gHIT&n" : "&rMISS&n");
	act(actbuf, FALSE, victim, NULL, ch, TO_GROUP);
	release_buffer(actbuf);
}


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data *ch, struct char_data *vict, int attacktype)
{
	int i, j, nr;
	struct message_type *msg;
	
	struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);
	
	for (i = 0; i < MAX_MESSAGES; i++) {
		if (fight_messages[i].a_type == attacktype) {
			nr = dice(1, fight_messages[i].number_of_attacks);
			for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
				msg = msg->next;
			
			if (!IS_NPC(vict) && IS_IMMORTAL(vict)) {
				act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
				act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
				act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
			} else if (dam != 0) {
				/*
				 * Don't send redundant color codes for TYPE_SUFFERING & other types
				 * of damage without attacker_msg.
				 */
				if (GET_POS(vict) == POS_DEAD) {
					if (msg->die_msg.attacker_msg) {
						send_to_char(CCYEL(ch, C_CMP), ch);
						act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
						send_to_char(CCNRM(ch, C_CMP), ch);
					}
					
					send_to_char(CCRED(vict, C_CMP), vict);
					act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
					send_to_char(CCNRM(vict, C_CMP), vict);
					
					act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
				} else {
					if (msg->hit_msg.attacker_msg) {
						send_to_char(CCYEL(ch, C_CMP), ch);
						act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
						send_to_char(CCNRM(ch, C_CMP), ch);
					}
					
					send_to_char(CCRED(vict, C_CMP), vict);
					act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
					send_to_char(CCNRM(vict, C_CMP), vict);
					
					act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
				}
			} else if (ch != vict) {        /* Dam == 0 */
				if (msg->miss_msg.attacker_msg) {
					send_to_char(CCYEL(ch, C_CMP), ch);
					act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
					send_to_char(CCNRM(ch, C_CMP), ch);
				}
				
				send_to_char(CCRED(vict, C_CMP), vict);
				act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
				send_to_char(CCNRM(vict, C_CMP), vict);
				
				act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
			}
			return (1);
		}
	}
	return (0);
}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *        < 0        Victim died.
 *        = 0        No damage.
 *        > 0        How much damage done.
 */
int	damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype)
{
	long local_gold = 0;
	char local_buf[256];

	if (GET_POS(victim) <= POS_DEAD) {
		/* This is "normal"-ish now with delayed extraction. -gg 3/15/2001 */
		if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
			return (-1);

		extended_mudlog(BRF, SYSL_BUGS, TRUE, "Attempt to damage corpse '%s' in room #%d by '%s'.",
								GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
		die(victim, ch);
		return (-1);                        /* -je, 7/7/92 */
	}

	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_TARGET)) {
		send_to_char("Oh yeah, YOU, fight back.  Pshaw.\r\n", ch);
		return (0);
	}

	/* peaceful rooms */
	if (ch->nr != real_mobile(DG_CASTER_PROXY) &&
		ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
		return (0);
	}

	/* shopkeeper protection */
	if (!ok_damage_shopkeeper(ch, victim))
		return (0);

	/* You can't damage an immortal! */
	if (!IS_NPC(victim) && IS_IMMORTAL(victim))
		dam = 0;

	if (victim != ch) {
		/* Start the attacker fighting the victim */
		if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
			set_fighting(ch, victim);
		/* Start the victim fighting the attacker */
		if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
			set_fighting(victim, ch);
			if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
				remember(victim, ch);
		}
	}

	/* If you attack a pet, it hates your guts */
	if (victim->master == ch)
		stop_follower(victim);

	/* If the attacker is invisible, he becomes visible */
	if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE))
		appear(ch);

	/* Cut damage in half if victim has sanct, to a minimum 1 */
	if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
		dam /= 2;

	/* Cut damage in half if victim has sanct, to a minimum 1 */
	if (AFF_FLAGGED(victim, AFF_ORB) && dam >= 2)
		dam *= 0.75;

	/* Set the maximum damage per round and subtract the hit points */
	dam = MAX(MIN(dam, 100), 0);
	GET_HIT(victim) -= dam;

	/* check if the victim has a hitprcnt trigger */
	if (IS_NPC(victim))
		hitprcnt_mtrigger(victim);

	/* Gain exp for the hit */
	if ((ch != victim) && (IS_NPC(victim)))
    gain_exp(ch, MIN(max_exp_gain, (number(1,100) * dam)));

	update_pos(victim);

	/*
	 * skill_message sends a message from the messages file in lib/misc.
	 * dam_message just sends a generic "You hit $n extremely hard.".
	 * skill_message is preferable to dam_message because it is more
	 * descriptive.
	 * 
	 * If we are _not_ attacking with a weapon (i.e. a spell), always use
	 * skill_message. If we are attacking with a weapon: If this is a miss or a
	 * death blow, send a skill_message if one exists; if not, default to a
	 * dam_message. Otherwise, always send a dam_message.
	 */
	if (GET_POS(victim) == POS_DEAD) {
		if (!skill_message(dam, ch, victim, attacktype))
			dam_message(dam, ch, victim, attacktype);
	} else {
		dam_message(dam, ch, victim, attacktype);
	}

	/* Use send_to_char -- act() doesn't send message if you are DEAD. */
	switch (GET_POS(victim)) {
	case POS_MORTALLYW:
		send_to_char("You are mortally wounded, and will die soon, if not aided.\r\n", victim);
		break;
	case POS_INCAP:
		send_to_char("You are incapacitated an will slowly die, if not aided.\r\n", victim);
		break;
	case POS_STUNNED:
		send_to_char("You're stunned, but will probably regain consciousness again.\r\n", victim);
		break;
	case POS_DEAD:
		send_to_char("\r\nYou are dead!  Sorry...\r\n\r\n", victim);
		break;
	case POS_REPLACE:
		act("You shatter $N with a forceful blow.", 0, ch, 0, victim, TO_CHAR);
		act("$n shatters $N with a forceful blow.", 0, ch, 0, victim, TO_NOTVICT);
		act("The groundskeeper comes and replaces $n with a new one.", 0, victim, 0, NULL, TO_ROOM);
		GET_HIT(victim) = GET_MAX_HIT(victim);
		GET_POS(victim) = POS_STANDING;
		stop_fighting(ch);
		break;
		
	default:                        /* >= POSITION SLEEPING */
		if (dam > (GET_MAX_HIT(victim) / 4))
			send_to_char("That really did HURT!\r\n", victim);
		
		if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
			sprintf(buf2, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
				CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
			send_to_char(buf2, victim);
			if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY) && GET_HIT(victim) > 0)
				do_flee(victim, NULL, 0, 0);
		}
		if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
			GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0) {
			send_to_char("You wimp out, and attempt to flee!\r\n", victim);
			do_flee(victim, NULL, 0, 0);
		}
		break;
	}

	/* Help out poor linkless people who are attacked */
	if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED && GET_HIT(victim) > 0) {
		do_flee(victim, NULL, 0, 0);
		if (!FIGHTING(victim)) {
			act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
			GET_WAS_IN(victim) = IN_ROOM(victim);
			char_from_room(victim);
			char_to_room(victim, 0);
		}
	}

	/* stop someone from fighting if they're stunned or worse */
	if ((GET_POS(victim) <= POS_STUNNED) && (FIGHTING(victim) != NULL))
		stop_fighting(victim);
	
	/* Uh oh.  Victim died. */
	if (GET_POS(victim) == POS_DEAD) {
		if ((ch != victim) && (IS_NPC(victim) || victim->desc)) {
			if (AFF_FLAGGED(ch, AFF_GROUP))
				group_gain(ch, victim);
			else
				solo_gain(ch, victim);
		}
		
		if (!IS_NPC(victim)) {
			extended_mudlog(BRF, SYSL_DEATHS, TRUE, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch),
				world[IN_ROOM(victim)].name);
			if (MOB_FLAGGED(ch, MOB_MEMORY))
				forget(ch, victim);
		}
		/* Cant determine GET_GOLD on corpse, so do now and store */
		if (IS_NPC(victim)) {
			local_gold = GET_GOLD(victim);
			sprintf(local_buf,"%ld", (long)local_gold);
			extended_mudlog(BRF, SYSL_MOBDEATHS, FALSE, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch),
				world[IN_ROOM(victim)].name);
		}
		die(victim, ch);
		/* If Autogold enabled, get gold corpse */
		if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOGOLD)) {
			do_get(ch,"gold corpse",0,0);
		}
		/* If Autoloot enabled, get all corpse */
		if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
			do_get(ch,"all corpse",0,0);
		}
		/* If Autogold AND AutoSplit AND we got money, split with group */
		if (IS_AFFECTED(ch, AFF_GROUP) && (local_gold > 0) &&
			PRF_FLAGGED(ch, PRF_AUTOSPLIT) && PRF_FLAGGED(ch,PRF_AUTOGOLD)) {
			do_split(ch,local_buf,0,0);
		}
		/* If Autosac enabled, get all corpse */
		if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOSAC)) {
			do_sac(ch,"corpse",0,0);
		}
		return (-1);
	}
	return (dam);
}


/*
 * Calculates the character's attack score.
 * Coded by Torgny Bjers <artovil@arcanerealms.org>
 * for Arcane Realms MUD, June 4th, 2002.
 * Based on the Circe rules by Bryce Harrington.
 */
int calculate_attack_score(struct char_data *ch, struct obj_data *wielded, int w_type, int skill)
{
	int score = 0;

	if (!IS_NPC(ch)) {
		/*
		 * Attack Score = (Skill rating) + (1/4 Melee skill rating) +
		 * (1/8 Combat skill rating) + (1/4 attribute)
		 */
		score = GET_SKILL(ch, skill_tree[skill].skill) / 100;
		score += (GET_SKILL(ch, skill_tree[skill].pre) / 100) / 4;
		if (skill_tree[skill].skill != SKILL_MELEE)
			score += (GET_SKILL(ch, find_skill(skill_tree[skill].pre)) / 100) / 8;
		score += get_attrib(ch, skill_tree[skill].attrib) / 4;
	} else { /* IS_NPC(ch) */
		score = (GET_DIFFICULTY(ch) + 1) * 15;
	}

	return (score);
}


/*
 * Calculates the victim's defense score.
 * Coded by Torgny Bjers <artovil@arcanerealms.org>
 * for Arcane Realms MUD, June 4th, 2002.
 * Partly based on the Circe rules by Bryce Harrington.
 */
int calculate_defense_score(struct char_data *victim, int position)
{
	int active_defense = 0, passive_defense = 0, score = 0;

	if (!IS_NPC(victim)) {
		/*
		 * Defense Score = 1/2 Tactics skill rating + 1/2 attribute
		 */
		active_defense = GET_AGILITY(victim) / 100 / 2;
		active_defense += (GET_SKILL(victim, SKILL_TACTICS) / 100) / 2;
	} else { /* IS_NPC(ch) */
		active_defense = (GET_DIFFICULTY(victim) + 1) * 10;
	}

	if (position >= 0)
		passive_defense = apply_pd(victim, position);

	if (AWAKE(victim))
		score += active_defense + passive_defense;
	else
		score = passive_defense / 2;

	if (GET_POS(victim) < POS_FIGHTING)
		score /= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

	return (score - 20);
}


void hit(struct char_data *ch, struct char_data *victim, int type)
{
	struct obj_data *wielded;
	int w_type, attack_score, defense_score;
	int dam;
	int diceroll = 0, skl = 0, skill = 0;
	int hitarea, position = -1;

	/* peaceful rooms */
	if (ch->nr != real_mobile(DG_CASTER_PROXY) &&
		ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
		stop_fighting(ch);
		return;
	}

	/* Sanity check.. if we got this far and a shopkeeper was attacked, let's stop it */
	if (!ok_damage_shopkeeper(ch, victim)) {
		stop_fighting(ch);
		return;
	}

	/* Check if the character has a fight trigger */
	fight_mtrigger(ch);

	/* Do some sanity checking, in case someone flees, etc. */
	if (IN_ROOM(ch) != IN_ROOM(victim)) {
		if (FIGHTING(ch) && FIGHTING(ch) == victim)
			stop_fighting(ch);
		return;
	}

	if (type == SKILL_DUAL)
		wielded = GET_EQ(ch, WEAR_DWIELD);
	else
		wielded = GET_EQ(ch, WEAR_WIELD);

	/* Check if this is a dual hit. */
	if (type == SKILL_DUAL)
		if ((!wielded || GET_OBJ_TYPE(wielded) != ITEM_WEAPON) && !skill_check(ch, SKILL_DUAL, 25))
			return;

	/* Find the weapon type (for skills and display) */
	if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
		w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
	else {
		if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
			w_type = ch->mob_specials.attack_type + TYPE_HIT;
		else
			w_type = TYPE_HIT;
	}

	if (!IS_NPC(ch)) {
		if (w_type > TYPE_HIT && wielded)
			skl = GET_OBJ_VAL(wielded, 0);
		else
			skl = SKILL_MELEE;
		/* Find the skill, if not, just log it and return. */
		if (!(skill = find_skill(skl))) {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Skill #%d not found in %s, line %d, player [%s].", skl, __FUNCTION__, __LINE__, GET_NAME(ch));
			return;
		}
	}

	/*
	 * CIRCE COMBAT SYSTEM 1.9
	 * Copyright 1997-2000 Bryce Harrington
	 * Available under the Gnu General Documentation License
	 * Attack roll:
	 * Hit = (attack score - d100) >= Defense score
	 * Defense score = Passive defense rating + Active Defense Rating - 20
	 * Damage = weapon damage roll - defender's damage reduction
	 *
	 * Modified for Arcane Realms MUD by
	 * Torgny Bjers <artovil@arcanerealms.org>
	 * June 4th, 2002.
	 */

	/* Attack score */
	attack_score = calculate_attack_score(ch, wielded, w_type, skill);

	/* d100 roll */
	if (AWAKE(victim)) /* You don't miss the side of a barn. */
		diceroll = number(1, 100);

	if (!IS_NPC(ch))
		if (diceroll < attack_score && diceroll > 0)
			GET_SKILL(ch, skill_tree[skill].skill) = skill_gain(ch, skill, 101 - diceroll);

	/*
	 * Find a proper area to hit the opponent, since we need to
	 * calculate the PD for the piece of armor in question.
	 */
	hitarea = race_frames[(int)GET_RACE(victim)].hitarea;
	position = race_hitareas[hitarea].hitareas[number(0, race_hitareas[hitarea].num_areas - 1)];

	if (GET_EQ(victim, position) == NULL)
		position = -1;

	/* Defense score */
	defense_score = calculate_defense_score(victim, position);

	/* Decide whether this is a hit or a miss */
	if (attack_score - diceroll >= defense_score) {
		if (!IS_NPC(victim) && skill_check(victim, SKILL_PARRY, diceroll) && AWAKE(victim)) {
			act("&c$N parries your attack.&n", FALSE, ch, 0, victim, TO_CHAR);
			act("&cYou parry $n's attack.&n", FALSE, ch, 0, victim, TO_VICT);
		} else {
			/*
			 * The hit has been determined, and no parry were in
			 * effect, now we need to calculate damage and subtract
			 * the damage reduction of the victim's armor.
			 */
			dam = 1 + ((GET_STRENGTH(ch) / 100) / 4);
			
			/* Maybe holding arrow? */
			if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
				/* Add weapon-based damage if a weapon is being wielded */
				dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
			} else {
				/* If no weapon, add bare hand damage instead */
				if (IS_NPC(ch)) {
					dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
				} else {
					dam += number(1, 4); /* bare hand damage for players */
				}
			}

			/*
			 * Include a damage multiplier if victim isn't ready to fight:
			 *
			 * Position sitting  1.33 x normal
			 * Position resting  1.66 x normal
			 * Position sleeping 2.00 x normal
			 * Position stunned  2.33 x normal
			 * Position incap    2.66 x normal
			 * Position mortally 3.00 x normal
			 *
			 * Note, this is a hack because it depends on the particular
			 * values of the POSITION_XXX constants.
			 */
			if (GET_POS(victim) < POS_FIGHTING)
				dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

			/*
			 * Damage reduction of armor:
			 */
			dam -= GET_REDUCTION(victim);

			/*
			 * Catch 22: Have to do at least one point damage or the
			 * hit will be reported as a miss by the damage code.
			 */
			dam = MAX(1, dam);

			/* Backstabbing multiplier? Else normal damage. */
			if (type == SKILL_BACKSTAB) {
				dam *= backstab_mult(ch);
				damage(ch, victim, dam, SKILL_BACKSTAB);
			} else
				damage(ch, victim, dam, w_type);
		}
  } else {
		/* the attacker missed the victim */
		if (type == SKILL_BACKSTAB)
			damage(ch, victim, 0, SKILL_BACKSTAB);
		else
			damage(ch, victim, 0, w_type);
	}
}


/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
	struct char_data *ch;

	for (ch = combat_list; ch; ch = next_combat_list) {
		next_combat_list = ch->next_fighting;
		
		if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
			stop_fighting(ch);
			continue;
		}
		
		if (IS_NPC(ch)) {
			if (GET_MOB_WAIT(ch) > 0) {
				GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
				continue;
			}
			GET_MOB_WAIT(ch) = 0;
			if (GET_POS(ch) < POS_FIGHTING) {
				GET_POS(ch) = POS_FIGHTING;
				act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
			}
		}

		if (GET_POS(ch) < POS_FIGHTING) {
			send_to_char("You can't fight while sitting!!\r\n", ch);
			continue;
		}

		switch (GET_POS(ch)) {
			case POS_DODGE: 
				send_to_char("While you dodge out of the way, you find you don't have time to get a hit in!\r\n", ch);
				break;
			case POS_DEFEND:
				send_to_char("While defending, you find you don't have time to get a hit in!\r\n", ch);
				break;
			default:
				hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
				break;
		}

		/* XXX: Need to see if they can handle "" instead of NULL. */
		if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != NULL)
			(mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");
			/*
			 * Check if the player can dual wield.
			 */
		if (!IS_NPC(ch)) {
			if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
				stop_fighting(ch);
				continue;
			} else {
				if (!(GET_POS(ch) == POS_DODGE || GET_POS(ch) == POS_DEFEND)) {
					struct obj_data *wielded = GET_EQ(ch, WEAR_DWIELD);
					if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON && GET_SKILL(ch, SKILL_DUAL) > 99)
						hit(ch, FIGHTING(ch), SKILL_DUAL);
				}
			}
			if (GET_POS(ch) == POS_DODGE)
				GET_POS(ch) = POS_FIGHTING;
		}
		if (!IS_NPC(ch) && FIGHTING(ch))
			diag_char_to_char(FIGHTING(ch), ch);
	}

}

/* THAC0 computer */
int	thaco(struct char_data *ch)
{
	int t = 20;
	
	t -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
	t -= GET_HITROLL(ch);
	t -= (int) ((GET_INTELLIGENCE(ch) - 13) / 1.5);        /* Intelligence helps! */
	t -= (int) ((GET_PERCEPTION(ch) - 13) / 1.5);        /* So does perception! */

	if (IS_IMMORTAL(ch))
		t = 1;
	
	if (t < 1)
		t = 1;
	
	return t;
}
