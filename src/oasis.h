/************************************************************************
 * OasisOLC - General / oasis.h                                    v2.0 *
 * Original author: Levork                                              *
 * Copyright 1996 by Harvey Gilpin                                      *
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)          *
 ************************************************************************/
/* $Id: oasis.h,v 1.39 2003/01/05 23:26:03 arcanere Exp $ */

#define _OASISOLC	0x201   /* 2.0.1 */

/*
 * Used to determine what version of OasisOLC is installed.
 *
 * Ex: #if _OASISOLC >= OASIS_VERSION(2,0,0)
 */
#define OASIS_VERSION(x,y,z)	(((x) << 8 | (y) << 4 | (z))

/*
 * Set this to 1 to enable MobProg support.  MobProgs are available on
 * the CircleMUD FTP site in the contrib/code directory.
 */
#define	CONFIG_OASIS_MPROG		0

/* -------------------------------------------------------------------------- */

/*
 * Macros, defines, structs and globals for the OLC suite.  You will need
 * to adjust these numbers if you ever add more.
 */
#define	NUM_POSITIONS					15
#define	NUM_SHOP_FLAGS				2
#define	NUM_TRADERS						7

#define	NUM_HOUSE_FLAGS				2

#if	CONFIG_OASIS_MPROG
/*
 * Define this to how many MobProg scripts you have.
 */
#define	NUM_PROGS							12
#endif
#define	AEDIT_PERMISSION			999
#define	HCHECK_PERMISSION			555

/* -------------------------------------------------------------------------- */

/*
 * Limit information.
 */
#define	MAX_ROOM_NAME					75
#define	MAX_MOB_NAME					50
#define	MAX_OBJ_NAME					50
#define	MAX_ROOM_DESC					2048
#define	MAX_EXIT_DESC					512
#define	MAX_EXTRA_DESC				512
#define	MAX_MOB_DESC					2048
#define	MAX_OBJ_DESC					2048
#define	MAX_QUEST_NAME				50
#define	MAX_QUEST_INFO				2048
#define	MAX_QUEST_ENDING			2048

/* -------------------------------------------------------------------------- */

extern int list_top;

/*
 * Utilities exported from olc.c.
 */
void cleanup_olc(struct descriptor_data *d, byte cleanup_type);
void get_char_colors(struct char_data *ch);

/*
 * OLC structures.
 */

struct oasis_olc_data {
	int mode;
	int zone_num;
	int number;
	int value;
	int value2;
	int total_mprogs;
	struct char_data *mob;
	struct room_data *room;
	struct obj_data *obj;
	struct zone_data *zone;
	struct shop_data *shop;
	struct extra_descr_data *desc;
#if	CONFIG_OASIS_MPROG
	struct mob_prog_data *mprog;
	struct mob_prog_data *mprogl;
#endif
	char *storage; /* for holding commands etc.. */
	struct social_messg *action;
	struct trig_data *trig;
	int script_mode;
	int trigger_position;
	int item_type;
	struct trig_proto_list *script;
	struct assembly_data *OlcAssembly;
	struct email_data *email;
	struct aq_data *quest;
	struct spell_info_type *spell;
	struct guild_info *guild;
	struct command_info *command;
	struct tutor_info_type *tutor;
	struct house_data *house;
};

/*
 * Exported globals.
 */
extern const char *nrm, *grn, *cyn, *yel;

/*
 * Descriptor access macros.
 */
#define	OLC(d)		((struct oasis_olc_data *)(d)->olc)
#define	OLC_MODE(d)     (OLC(d)->mode)          /* Parse input mode.	*/
#define	OLC_NUM(d)      (OLC(d)->number)        /* Room/Obj VNUM.	*/
#define	OLC_VAL(d)      (OLC(d)->value)         /* Scratch variable.	*/
#define	OLC_VAL2(d)     (OLC(d)->value2)        /* Second Scratch variable.	*/
#define	OLC_ZNUM(d)     (OLC(d)->zone_num)      /* Real zone number.	*/
#define	OLC_ROOM(d)     (OLC(d)->room)          /* Room structure.	*/
#define	OLC_OBJ(d)      (OLC(d)->obj)           /* Object structure.	*/
#define	OLC_ZONE(d)     (OLC(d)->zone)          /* Zone structure.	*/
#define	OLC_MOB(d)      (OLC(d)->mob)           /* Mob structure.	*/
#define	OLC_SHOP(d)     (OLC(d)->shop)          /* Shop structure.	*/
#define	OLC_DESC(d)     (OLC(d)->desc)          /* Extra description.	*/
#if	CONFIG_OASIS_MPROG
#define	OLC_MPROG(d)    (OLC(d)->mprog)         /* Temporary MobProg.	*/
#define	OLC_MPROGL(d)   (OLC(d)->mprogl)        /* MobProg list.	*/
#define	OLC_MTOTAL(d)   (OLC(d)->total_mprogs)  /* Total mprog number.	*/
#endif
#define	OLC_STORAGE(d)  (OLC(d)->storage)       /* For command storage  */
#define	OLC_ACTION(d)   (OLC(d)->action)        /* Action structure     */
#define	OLC_TRIG(d)     (OLC(d)->trig)          /* Trigger structure.   */
#define	OLC_ASSEDIT(d)  (OLC(d)->OlcAssembly)   /* assembly olc         */
#define	OLC_EMAIL(d)    (OLC(d)->email)         /* Email structure -spl */
#define	OLC_QUEST(d)    (OLC(d)->quest)         /* Quest structure      */
#define	OLC_SPELL(d)    (OLC(d)->spell)         /* spell structure      */
#define	OLC_GUILD(d)    (OLC(d)->guild)         /* guild structure      */
#define OLC_COMMAND(d)	(OLC(d)->command)				/* Command structure		*/
#define	OLC_TUTOR(d)    (OLC(d)->tutor)         /* tutor structure      */
#define	OLC_HOUSE(d)    (OLC(d)->house)         /* house structure      */

/*
 * Other macros.
 */
#define	OLC_EXIT(d)       (OLC_ROOM(d)->dir_option[OLC_VAL(d)])
#define	GET_OLC_ZONE(c)   ((c)->player_specials->saved.olc_zone)

/*
 * Cleanup types.
 */
#define	CLEANUP_ALL       1	/* Free the whole lot.	*/
#define	CLEANUP_STRUCTS   2	/* Don't free strings.	*/

/*
 * Submodes of OEDIT connectedness.
 */
#define	OEDIT_MAIN_MENU									1
#define	OEDIT_EDIT_NAMELIST							2
#define	OEDIT_SHORTDESC									3
#define	OEDIT_LONGDESC									4
#define	OEDIT_ACTDESC										5
#define	OEDIT_TYPE											6
#define	OEDIT_EXTRAS										7
#define	OEDIT_WEAR											8
#define	OEDIT_WEIGHT										9
#define	OEDIT_COST											10
#define	OEDIT_COSTPERDAY								11
#define	OEDIT_TIMER											12
#define	OEDIT_VALUE_1										13
#define	OEDIT_VALUE_2										14
#define	OEDIT_VALUE_3										15
#define	OEDIT_VALUE_4										16
#define	OEDIT_APPLY											17
#define	OEDIT_APPLYMOD									18
#define	OEDIT_EXTRADESC_KEY							19
#define	OEDIT_CONFIRM_SAVEDB						20
#define	OEDIT_CONFIRM_SAVESTRING				21
#define	OEDIT_PROMPT_APPLY							22
#define	OEDIT_EXTRADESC_DESCRIPTION			23
#define	OEDIT_EXTRADESC_MENU						24
#define	OEDIT_PERM											25
#define	OEDIT_SPECPROC									26
#define OEDIT_SIZE											27
#define OEDIT_COLOR											28
#define OEDIT_RESOURCE									29

/*
 * Submodes of REDIT connectedness.
 */
#define	REDIT_MAIN_MENU									1
#define	REDIT_NAME											2
#define	REDIT_DESC											3
#define	REDIT_FLAGS											4
#define	REDIT_SECTOR										5
#define	REDIT_EXIT_MENU									6
#define	REDIT_CONFIRM_SAVEDB						7
#define	REDIT_CONFIRM_SAVESTRING				8
#define	REDIT_EXIT_NUMBER								9
#define	REDIT_EXIT_DESCRIPTION					10
#define	REDIT_EXIT_KEYWORD							11
#define	REDIT_EXIT_KEY									12
#define	REDIT_EXIT_DOORFLAGS						13
#define	REDIT_EXTRADESC_MENU						14
#define	REDIT_EXTRADESC_KEY							15
#define	REDIT_EXTRADESC_DESCRIPTION			16
#define	REDIT_SPECPROC									17
#define	REDIT_FLUX											18
#define REDIT_RESOURCES_MENU						19
#define REDIT_RESOURCE_FLAGS						20
#define REDIT_RESOURCE_VAL0							21
#define REDIT_RESOURCE_VAL1							22
#define REDIT_RESOURCE_VAL2							23
#define REDIT_RESOURCE_VAL3							24
#define REDIT_RESOURCE_VAL4							25

/*
 * Submodes of ZEDIT connectedness.
 */
#define	ZEDIT_MAIN_MENU									0
#define	ZEDIT_DELETE_ENTRY							1
#define	ZEDIT_NEW_ENTRY									2
#define	ZEDIT_CHANGE_ENTRY							3
#define	ZEDIT_COMMAND_TYPE							4
#define	ZEDIT_IF_FLAG										5
#define	ZEDIT_ARG1											6
#define	ZEDIT_ARG2											7
#define	ZEDIT_ARG3											8
#define	ZEDIT_ZONE_NAME									9
#define	ZEDIT_ZONE_LIFE									10
#define	ZEDIT_ZONE_BOT									11
#define	ZEDIT_ZONE_TOP									12
#define	ZEDIT_ZONE_RESET								13
#define	ZEDIT_CONFIRM_SAVESTRING				14
#define	ZEDIT_ZONE_BUILDERS							15
#define	ZEDIT_ZONE_FLAGS								16
#define	ZEDIT_SARG1											17
#define	ZEDIT_SARG2											18

/*
 * Submodes of MEDIT connectedness.
 */
#define	MEDIT_MAIN_MENU									0
#define	MEDIT_ALIAS											1
#define	MEDIT_S_DESC										2
#define	MEDIT_L_DESC										3
#define	MEDIT_D_DESC										4
#define	MEDIT_NPC_FLAGS									5
#define	MEDIT_AFF_FLAGS									6
#define	MEDIT_CONFIRM_SAVESTRING				7
/*
 * Numerical responses.
 */
#define	MEDIT_NUMERICAL_RESPONSE				10
#define	MEDIT_SEX												11
#define	MEDIT_HITROLL										12
#define	MEDIT_DAMROLL										13
#define	MEDIT_NDD												14
#define	MEDIT_SDD												15
#define	MEDIT_NUM_HP_DICE								16
#define	MEDIT_SIZE_HP_DICE							17
#define	MEDIT_ADD_HP										18
#define	MEDIT_AC												19
#define	MEDIT_EXP												20
#define	MEDIT_GOLD											21
#define	MEDIT_POS												22
#define	MEDIT_DEFAULT_POS								23
#define	MEDIT_ATTACK										24
#define	MEDIT_DIFFICULTY								25
#define	MEDIT_ALIGNMENT									26
#define	MEDIT_SPECPROC									27
#define	MEDIT_RACE											28
#define MEDIT_VALUE_1										29
#define MEDIT_VALUE_2										30
#define MEDIT_VALUE_3										31
#define MEDIT_VALUE_4										32
#if	CONFIG_OASIS_MPROG
#define	MEDIT_MPROG											33
#define	MEDIT_CHANGE_MPROG							34
#define	MEDIT_MPROG_COMLIST							35
#define	MEDIT_MPROG_ARGS								36
#define	MEDIT_MPROG_TYPE								37
#define	MEDIT_PURGE_MPROG								38
#endif

/*
 * Submodes of SEDIT connectedness.
 */
#define	SEDIT_MAIN_MENU									0
#define	SEDIT_CONFIRM_SAVESTRING				1
#define	SEDIT_NOITEM1										2
#define	SEDIT_NOITEM2										3
#define	SEDIT_NOCASH1										4
#define	SEDIT_NOCASH2										5
#define	SEDIT_NOBUY											6
#define	SEDIT_BUY												7
#define	SEDIT_SELL											8
#define	SEDIT_PRODUCTS_MENU							9
#define	SEDIT_ROOMS_MENU								10
#define	SEDIT_NAMELIST_MENU							11
#define	SEDIT_NAMELIST									12
/*
 * Numerical responses.
 */
#define	SEDIT_NUMERICAL_RESPONSE				20
#define	SEDIT_OPEN1											21
#define	SEDIT_OPEN2											22
#define	SEDIT_CLOSE1										23
#define	SEDIT_CLOSE2										24
#define	SEDIT_KEEPER										25
#define	SEDIT_BUY_PROFIT								26
#define	SEDIT_SELL_PROFIT								27
#define	SEDIT_TYPE_MENU									28
#define	SEDIT_DELETE_TYPE								29
#define	SEDIT_DELETE_PRODUCT						30
#define	SEDIT_NEW_PRODUCT								31
#define	SEDIT_DELETE_ROOM								32
#define	SEDIT_NEW_ROOM									33
#define	SEDIT_SHOP_FLAGS								34
#define	SEDIT_NOTRADE										35

/* Submodes of AEDIT connectedness	*/
#define	AEDIT_CONFIRM_SAVESTRING				0
#define	AEDIT_CONFIRM_EDIT							1
#define	AEDIT_CONFIRM_ADD								2
#define	AEDIT_MAIN_MENU									3
#define	AEDIT_ACTION_NAME								4
#define	AEDIT_SORT_AS										5
#define	AEDIT_MIN_CHAR_POS							6
#define	AEDIT_MIN_VICT_POS							7
#define	AEDIT_HIDDEN_FLAG								8
#define	AEDIT_NOVICT_CHAR								9
#define	AEDIT_NOVICT_OTHERS							10
#define	AEDIT_VICT_CHAR_FOUND						11
#define	AEDIT_VICT_OTHERS_FOUND					12
#define	AEDIT_VICT_VICT_FOUND						13
#define	AEDIT_VICT_NOT_FOUND						14
#define	AEDIT_SELF_CHAR									15
#define	AEDIT_SELF_OTHERS								16
#define	AEDIT_VICT_CHAR_BODY_FOUND			17
#define	AEDIT_VICT_OTHERS_BODY_FOUND		18
#define	AEDIT_VICT_VICT_BODY_FOUND			19
#define	AEDIT_OBJ_CHAR_FOUND						20
#define	AEDIT_OBJ_OTHERS_FOUND					21

/* Submodes of ASSEDIT connectedness	*/
#define	ASSEDIT_DO_NOT_USE							0
#define	ASSEDIT_MAIN_MENU								1
#define	ASSEDIT_ADD_COMPONENT						2
#define	ASSEDIT_EDIT_COMPONENT					3
#define	ASSEDIT_DELETE_COMPONENT				4
#define	ASSEDIT_EDIT_EXTRACT						5
#define	ASSEDIT_EDIT_INROOM							6
#define	ASSEDIT_EDIT_TYPES							7
#define	ASSEDIT_EDIT_SKILL							8
#define ASSEDIT_EDIT_SKILL_PERCENTAGE		9
#define ASSEDIT_EDIT_CREATION_TIME			10
#define ASSEDIT_EDIT_ITEMS_REQUIRED			11
#define ASSEDIT_EDIT_PRODUCES						12
#define	ASSEDIT_ADD_BYPRODUCT						13
#define	ASSEDIT_EDIT_BYPRODUCT					14
#define	ASSEDIT_DELETE_BYPRODUCT				15
#define	ASSEDIT_EDIT_BYPRODUCT_WHEN			16
#define ASSEDIT_EDIT_BYPRODUCT_NUMBER		17
#define	ASSEDIT_ADD_ALT_COMPONENT				18
#define	ASSEDIT_ALT_COMPONENT_VNUM			19
#define	ASSEDIT_EDIT_ALT_COMPONENT			20
#define	ASSEDIT_DELETE_ALT_COMPONENT		21

/*
 * Submodes of EMAIL connectedness.
 */
#define	EMAIL_MAIN_MENU									1
#define	EMAIL_FROM											2
#define	EMAIL_2WHO											3
#define	EMAIL_SUBJECT										4
#define	EMAIL_BODY											5
#define	EMAIL_DESC											6
#define	EMAIL_CONFIRM_SAVESTRING				7

/*
 * Submodes of QEDIT connectedness.
 */
#define	QEDIT_MAIN_MENU									0
#define	QEDIT_CONFIRM_SAVESTRING				1
#define	QEDIT_NAME											2
#define	QEDIT_DESC											3
#define	QEDIT_INFO											4
#define	QEDIT_ENDING										5
#define	QEDIT_QUESTMASTER								6
#define	QEDIT_TYPE											7
#define	QEDIT_FLAGS											8
#define	QEDIT_TARGET										9
#define	QEDIT_EXP												10
#define	QEDIT_NEXT											11
#define	QEDIT_VALUE_0										12
#define	QEDIT_VALUE_1										13
#define	QEDIT_VALUE_2										14
#define	QEDIT_VALUE_3										15

/*
 * Submodes of SPEDIT connectedness
 */
#define	SPEDIT_MAIN_MENU								0
#define	SPEDIT_CONFIRM_SAVE							1
#define	SPEDIT_NAME											2
#define	SPEDIT_LONG_NAME								3
#define	SPEDIT_CAST_MSG									4
#define	SPEDIT_MUNDANE_MSG							5
#define	SPEDIT_TARGET_MSG								6
#define	SPEDIT_VICTIM_MSG								7
#define	SPEDIT_WEAR_OFF_MSG							8
#define	SPEDIT_LEARNED									9
#define	SPEDIT_VIOLENT									10
#define SPEDIT_REAGENTS_MENU						11
#define SPEDIT_REAGENT_LOCATION					12
#define SPEDIT_REAGENT_EXTRACT					13
#define SPEDIT_FOCUSES_MENU							14
#define SPEDIT_FOCUS_LOCATION						15
#define SPEDIT_FOCUS_EXTRACT						16
#define SPEDIT_TRANSMUTER_LOCATION			17
#define SPEDIT_TRANSMUTER_EXTRACT				18
#define SPEDIT_TRANSMUTERS_MENU					19
#define	SPEDIT_NUMERICAL_RESPONSE				20
#define	SPEDIT_MANA_MIN									21
#define	SPEDIT_MANA_MAX									22
#define	SPEDIT_MIN_POSITION							23
#define	SPEDIT_MASTER_SKILL							24
#define	SPEDIT_TARGETS									25
#define	SPEDIT_ROUTINES									26
#define SPEDIT_NEW_REAGENT							27
#define SPEDIT_DELETE_REAGENT						28
#define SPEDIT_NEW_FOCUS								29
#define SPEDIT_DELETE_FOCUS							30
#define SPEDIT_NEW_TRANSMUTER						31
#define SPEDIT_DELETE_TRANSMUTER				32
#define SPEDIT_TECHNIQUE								33
#define SPEDIT_FORM											34
#define SPEDIT_RANGE										35
#define SPEDIT_DURATION									36
#define SPEDIT_TARGET_GROUP							37

/*
 * Submodes of GEDIT connectedness
 */
#define	GEDIT_MAIN_MENU									0
#define	GEDIT_CONFIRM_SAVE							1
#define	GEDIT_NAME											2
#define	GEDIT_GLTITLE										3
#define	GEDIT_GUILDIES_PRETITLE					4
#define	GEDIT_TYPE											5
#define	GEDIT_GOSSIP_NAME								6
#define	GEDIT_GCHAN_NAME								7
#define	GEDIT_GCHAN_COLOR								8
#define	GEDIT_GCHAN_TYPE								9
#define	GEDIT_GOLD											10
#define	GEDIT_GUILDWALK									11
#define	GEDIT_FLAGS											12
#define	GEDIT_DESCRIPTION								13
#define	GEDIT_REQUIREMENTS							14
#define	GEDIT_GOSSIP										15
#define	GEDIT_RANKS_MENU								16
#define	GEDIT_NEW_RANK_NUMBER						17
#define	GEDIT_NEW_RANK_NAME							18
#define	GEDIT_DELETE_RANK								19
#define	GEDIT_ZONES_MENU								20
#define	GEDIT_NEW_ZONE									21
#define	GEDIT_DELETE_ZONE								22
#define	GEDIT_EQUIPMENT_MENU						23
#define	GEDIT_NEW_EQUIPMENT							24
#define	GEDIT_DELETE_EQUIPMENT					25

/*
 * Submodes of COMEDIT connectedness
 */
#define COMEDIT_MAIN_MENU								1
#define COMEDIT_CONFIRM_SAVESTRING			2
#define COMEDIT_CONFIRM_DELETE					3
#define	COMEDIT_CONFIRM_EDIT						4
#define	COMEDIT_CONFIRM_ADD							5
#define COMEDIT_COMMAND_NAME						6
#define COMEDIT_SORT_AS									7
#define COMEDIT_MIN_POS									8
#define COMEDIT_COMMAND_FUNC						9
#define COMEDIT_RIGHTS									10
#define COMEDIT_SUBCMD									11

/*
 * Submodes of TUTOREDIT connectedness
 */
#define TUTOREDIT_MAIN_MENU							0
#define TUTOREDIT_CONFIRM_SAVE					1
#define TUTOREDIT_NOSKILL								2
#define TUTOREDIT_NOREQ									3
#define TUTOREDIT_SKILLED								4
#define TUTOREDIT_NOCASH								5
#define TUTOREDIT_BUYSUCCESS						6
#define TUTOREDIT_NUMERICAL_RESPONSE		7
#define TUTOREDIT_SKILLS_MENU						8
#define TUTOREDIT_NEW_SKILL							9
#define TUTOREDIT_DELETE_SKILL					10
#define TUTOREDIT_SKILL_PROFICIENCY			11
#define TUTOREDIT_SKILL_COST						12
#define TUTOREDIT_TUTOR									13

/*
 * Submodes of SEDIT connectedness.
 */
#define	HEDIT_MAIN_MENU									0
#define	HEDIT_CONFIRM_SAVESTRING				1
#define	HEDIT_ROOMS_MENU								2
#define	HEDIT_COWNERS_MENU							3
#define	HEDIT_GUESTS_MENU								4
#define	HEDIT_OWNER											5
#define	HEDIT_NEW_COWNER								6
#define	HEDIT_NEW_GUEST									7
/*
 * Numerical responses.
 */
#define	HEDIT_NUMERICAL_RESPONSE				8
#define	HEDIT_ATRIUM										9
#define	HEDIT_EXIT											10
#define	HEDIT_MODE											11
#define	HEDIT_PRUNE_SAFE								12
#define	HEDIT_COST											13
#define	HEDIT_MAX_SECURE								14
#define	HEDIT_MAX_LOCKED								15
#define	HEDIT_DELETE_ROOM								16
#define	HEDIT_NEW_ROOM									17
#define	HEDIT_DELETE_COWNER							18
#define	HEDIT_DELETE_GUEST							19

/*
 * Prototypes to keep.
 */
void clear_screen(struct descriptor_data *);

/* medit prototypes */
void medit_free_mobile(struct char_data *mob);
void medit_setup_new(struct descriptor_data *d);
void medit_setup_existing(struct descriptor_data *d, int rmob_num);
void init_mobile(struct char_data *mob);
void medit_save_internally(struct descriptor_data *d);
void medit_save_to_disk(zone_vnum zone_num);
void medit_disp_positions(struct descriptor_data *d);
void medit_disp_mprog(struct descriptor_data *d);
void medit_change_mprog(struct descriptor_data *d);
void medit_disp_mprog_types(struct descriptor_data *d);
void medit_disp_specproc(struct descriptor_data *d);
void medit_disp_race(struct descriptor_data *d);
void medit_disp_sex(struct descriptor_data *d);
void medit_disp_attack_types(struct descriptor_data *d);
void medit_disp_mob_flags(struct descriptor_data *d);
void medit_disp_aff_flags(struct descriptor_data *d);
void medit_disp_menu(struct descriptor_data *d);
void medit_parse(struct descriptor_data *d, char *arg);
void medit_string_cleanup(struct descriptor_data *d, int terminator);
void medit_disp_val1_menu(struct descriptor_data *d);
void medit_disp_val2_menu(struct descriptor_data *d);
void medit_disp_val3_menu(struct descriptor_data *d);
void medit_disp_val4_menu(struct descriptor_data *d);

/* oedit prototypes */
void oedit_setup_new(struct descriptor_data *d);
void oedit_setup_existing(struct descriptor_data *d, int real_num);
void oedit_save_internally(struct descriptor_data *d);
void oedit_save_to_disk(int zone_num);
void oedit_disp_specproc(struct descriptor_data *d);
void oedit_disp_container_flags_menu(struct descriptor_data *d);
void oedit_disp_extradesc_menu(struct descriptor_data *d);
void oedit_disp_prompt_apply_menu(struct descriptor_data *d);
void oedit_liquid_type(struct descriptor_data *d);
void oedit_disp_apply_menu(struct descriptor_data *d);
void oedit_disp_weapon_menu(struct descriptor_data *d);
void oedit_disp_spells_menu(struct descriptor_data *d);
void oedit_disp_val1_menu(struct descriptor_data *d);
void oedit_disp_val2_menu(struct descriptor_data *d);
void oedit_disp_val3_menu(struct descriptor_data *d);
void oedit_disp_val4_menu(struct descriptor_data *d);
void oedit_disp_type_menu(struct descriptor_data *d);
void oedit_disp_extra_menu(struct descriptor_data *d);
void oedit_disp_wear_menu(struct descriptor_data *d);
void oedit_disp_menu(struct descriptor_data *d);
void oedit_parse(struct descriptor_data *d, char *arg);
void oedit_disp_perm_menu(struct descriptor_data *d);
void oedit_string_cleanup(struct descriptor_data *d, int terminator);
void oedit_disp_sizes(struct descriptor_data *d);
void oedit_disp_colors(struct descriptor_data *d);

/* redit prototypes */
void redit_string_cleanup(struct descriptor_data *d, int terminator);
void redit_setup_new(struct descriptor_data *d);
void redit_setup_existing(struct descriptor_data *d, int real_num);
void redit_save_internally(struct descriptor_data *d);
void redit_save_to_disk(zone_vnum zone_num);
void redit_disp_specproc(struct descriptor_data *d);
void redit_disp_extradesc_menu(struct descriptor_data *d);
void redit_disp_exit_menu(struct descriptor_data *d);
void redit_disp_exit_flag_menu(struct descriptor_data *d);
void redit_disp_flag_menu(struct descriptor_data *d);
void redit_disp_sector_menu(struct descriptor_data *d);
void redit_disp_flux_menu(struct descriptor_data *d);
void redit_disp_resources_menu(struct descriptor_data *d);
void redit_disp_resource_flag_menu(struct descriptor_data *d);
void redit_disp_menu(struct descriptor_data *d);
void redit_parse(struct descriptor_data *d, char *arg);
void free_room(struct room_data *room);

/* sedit prototypes */
void sedit_setup_new(struct descriptor_data *d);
void sedit_setup_existing(struct descriptor_data *d, int rshop_num);
void sedit_save_internally(struct descriptor_data *d);
void sedit_save_to_disk(int zone_num);
void sedit_products_menu(struct descriptor_data *d);
void sedit_compact_rooms_menu(struct descriptor_data *d);
void sedit_rooms_menu(struct descriptor_data *d);
void sedit_namelist_menu(struct descriptor_data *d);
void sedit_shop_flags_menu(struct descriptor_data *d);
void sedit_no_trade_menu(struct descriptor_data *d);
void sedit_types_menu(struct descriptor_data *d);
void sedit_disp_menu(struct descriptor_data *d);
void sedit_parse(struct descriptor_data *d, char *arg);

/* aedit prototypes */
void aedit_parse(struct descriptor_data *d, char *arg);
void aedit_disp_positions(struct descriptor_data *d);

/* zedit prototypes */
void zedit_setup(struct descriptor_data *d, int room_num);
void zedit_create_index(int znum, char *type);
void zedit_save_internally(struct descriptor_data *d);
void zedit_save_to_disk(int zone_num);
void zedit_disp_menu(struct descriptor_data *d);
void zedit_disp_comtype(struct descriptor_data *d);
void zedit_disp_arg1(struct descriptor_data *d);
void zedit_disp_arg2(struct descriptor_data *d);
void zedit_disp_arg3(struct descriptor_data *d);
void zedit_parse(struct descriptor_data *d, char *arg);
void zedit_disp_flag_menu(struct descriptor_data *d);
int zedit_new_zone(struct char_data *ch, zone_vnum vzone_num, room_vnum bottom, room_vnum top);

/* trigedit prototypes */
void trigedit_string_cleanup(struct descriptor_data *d, int terminator);
void trigedit_parse(struct descriptor_data *d, char *arg);
void trigedit_setup_new(struct descriptor_data *d);
void trigedit_setup_existing(struct descriptor_data *d, int rtrg_num);
int	real_trigger(int vnum);

/* email prototypes */
void email_setup_new(struct descriptor_data *d);
void email_save_to_disk(struct descriptor_data *d);
void email_disp_menu(struct descriptor_data *d);
void email_parse(struct descriptor_data *d, char *arg);
void email_string_cleanup(struct descriptor_data *d, int terminator);
void email_parse(struct descriptor_data *d, char *arg);

/* qedit prototypes */
void qedit_setup_new(struct descriptor_data *d);
void qedit_setup_existing(struct descriptor_data *d, int real_num);
void qedit_save_internally(struct descriptor_data *d);
void qedit_save_to_disk(int znum);
void free_quest(struct aq_data *quest);
void qedit_disp_type_menu(struct descriptor_data *d);
void qedit_disp_menu(struct descriptor_data * d);
void qedit_parse(struct descriptor_data * d, char *arg);
void qedit_disp_flags_menu(struct descriptor_data *d);
void qedit_disp_val0_menu(struct descriptor_data *d);
void qedit_disp_val1_menu(struct descriptor_data *d);
void qedit_disp_val2_menu(struct descriptor_data *d);
void qedit_disp_val3_menu(struct descriptor_data *d);
void qedit_string_cleanup(struct descriptor_data *d, int terminator);

/* spedit prototypes */
void spedit_disp_menu(struct descriptor_data * d);
void spedit_disp_routine_flags(struct descriptor_data *d);
void spedit_disp_target_flags(struct descriptor_data *d);
void spedit_parse(struct descriptor_data *d, char *arg);
void spedit_free_spell(struct spell_info_type *sptr);
void spedit_setup_new(struct descriptor_data *d);
void spedit_string_cleanup(struct descriptor_data *d, int terminator);
void save_spells(void);
void load_spells(void);

/* gedit prototypes */
void gedit_disp_menu(struct descriptor_data * d);
void gedit_parse(struct descriptor_data *d, char *arg);
void gedit_setup_existing(struct descriptor_data *d, struct guild_info *guild);
void gedit_string_cleanup(struct descriptor_data *d, int terminator);

/* comedit prototypes */
void comedit_setup_new(struct descriptor_data *d);
void comedit_setup_existing(struct descriptor_data *d, int real_num);
void comedit_save_internally(struct descriptor_data *d);
void comedit_mysql_save(struct descriptor_data *d);
void comedit_disp_positions(struct descriptor_data *d);
void comedit_disp_menu(struct descriptor_data * d);
void comedit_parse(struct descriptor_data * d, char *arg);
void comedit_disp_functions(struct descriptor_data *d);
void comedit_delete_command(struct descriptor_data *d);

/* tutoredit prototypes */
void tutoredit_disp_menu(struct descriptor_data * d);
void tutoredit_parse(struct descriptor_data *d, char *arg);
void tutoredit_setup_new(struct descriptor_data *d);
void tutoredit_setup_existing(struct descriptor_data *d, struct tutor_info_type *t);

/* hedit prototypes */
void hedit_setup_new(struct descriptor_data *d);
void hedit_setup_existing(struct descriptor_data *d, int rhouse_num);
void hedit_save_internally(struct descriptor_data *d);
void hedit_save_to_disk(void);
void hedit_compact_rooms_menu(struct descriptor_data *d);
void hedit_rooms_menu(struct descriptor_data *d);
void hedit_namelist_menu(struct descriptor_data *d);
void hedit_house_flags_menu(struct descriptor_data *d);
void hedit_disp_menu(struct descriptor_data *d);
void hedit_parse(struct descriptor_data *d, char *arg);
