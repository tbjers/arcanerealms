/* ******************************************************************** *
 * FILE	: assemblies.h			Copyright (C) 1999 Geoff Davis              *
 * USAGE: Definitions, constants and prototypes for assembly engine.    *
 * -------------------------------------------------------------------- *
 * 1999 MAY 07	gdavis/azrael@laker.net	Initial implementation.         *
 * 2002 JUNE 23 tbjers/artovil@arcanerealms.org Event hooks             *
 * -------------------------------------------------------------------- *
 * Skill additions Copyright (C) 2001-2002, Torgny Bjers.               *
 * artovil@arcanerealms.org | http://www.arcanerealms.org/              *
 * ******************************************************************** */
/* $Id: assemblies.h,v 1.20 2003/01/05 23:26:03 arcanere Exp $ */

#if	!defined( __ASSEMBLIES_H__ )
#define	__ASSEMBLIES_H__

/* ******************************************************************** *
 * Preprocessor constants.                                              *
 * ******************************************************************** */

/* Assembly type: Used in ASSEMBLY.iAssemblyType */
#define	ASSM_ASSEMBLE		0		// Assembly must be assembled.
#define	ASSM_BAKE				1		// Assembly must be baked.
#define	ASSM_BREW				2		// Assembly must be brewed.
#define	ASSM_CRAFT			3		// Assembly must be crafted.
#define	ASSM_FLETCH			4		// Assembly must be fletched.
#define	ASSM_KNIT				5		// Assembly must be knitted.
#define	ASSM_MAKE				6		// Assembly must be made.
#define	ASSM_MIX				7		// Assembly must be mixed.
#define	ASSM_THATCH			8		// Assembly must be thatched.
#define	ASSM_WEAVE			9		// Assembly must be woven.
#define	ASSM_COOK				10	// Assembly must be cooked.
#define	ASSM_SEW				11	// Assembly must be sown.
#define ASSM_BUTCHER		12	// Assembly must be butchered.
#define ASSM_TAN				13	// Assembly must be tanned.
#define ASSM_SMELT			14	// Assembly must be smelted.
#define ASSM_CUT				15	// Assembly must be cut.
#define ASSM_DIVIDE			16	// Assembly must be divided.
#define ASSM_FORGE			17	// Assembly must be forged.

#define	MAX_ASSM				18	// Number of assembly types.

/* Assembly type words: Used in AssemblyTypeNames[][] */
#define ASSM_TYPE_TO		0
#define ASSM_TYPE_DOES	1
#define ASSM_TYPE_DID		2

/* Assembly byproducts load arguments */
#define ASSM_BYPROD_FAILURE	0
#define ASSM_BYPROD_SUCCESS	1
#define ASSM_BYPROD_ALWAYS	2

/* */
#define COMPONENT_COLOR			1
#define COMPONENT_ITEMS			2
#define COMPONENT_RESOURCE	3

/* ******************************************************************** *
 * Type aliases.                                                        *
 * ******************************************************************** */

typedef	struct assembly_data	ASSEMBLY;
typedef	struct component_data	COMPONENT;
typedef	struct alt_comp_data	ALTCOMPONENT;
typedef	struct byproduct_data	BYPRODUCT;

/* ******************************************************************** *
 * Structure definitions.                                               *
 * ******************************************************************** */

/* Assembly structure definition. */
struct assembly_data {
	long lVnum;													/* Vnum of the object assembled.	*/
	long lNumComponents;								/* Number of components.					*/
	long lNumAltComponents;							/* Number of alternate components.*/
	long lNumByproducts;								/* Number of by-products.					*/
	long lSkill;												/* Skill required to assemble.		*/
	long lTime;													/* Ticks to assembly complete.		*/
	long lSkReq;												/* Skill required to assemble.		*/
	long lProduces;											/* Number of items to produce.		*/
	bool bHidden;												/* Assembly is hidden from lists.	*/
	unsigned char	uchAssemblyType;			/* Type of assembly (ASSM_xxx).		*/
	struct component_data *pComponents;	/* Array of component info.				*/
	struct alt_comp_data *aComponents;	/* Array of alt-component info.		*/
	struct byproduct_data *pByproducts;	/* Array of by-product info.			*/
};

/* Assembly component structure definition. */
struct component_data {
	bool	bExtract;		/* Extract the object after use. */
	bool	bInRoom;		/* Component in room, not inven. */
	int		bItemsReq;	/* Number of instances required. */
	long	lVnum;			/* Vnum of the component object. */
};

/* Assembly byproduct structure definition. */
struct byproduct_data {
	long		lVnum;			/* Vnum of the byproduct object. */
	sh_int	bItemsReq;	/* Number of instances to produce. */
	sh_int	iWhen;			/* Produce byproduct on failure/success. */
};

/* Assembly alternate component structure definition. */
struct alt_comp_data {
	long		lVnum;			/* Vnum of the alternate component object. */
	long		lComponent;	/* Vnum of the component object. */
};

/* ******************************************************************** *
 * Prototypes for assemblies.c.                                         *
 * ******************************************************************** */

void assemblyBootAssemblies( void );
void assemblySaveAssemblies( void );
void assemblyListToChar( struct char_data *pCharacter );
void assemblyListGroupToChar( struct char_data *pCharacter, int asmType );
void assemblyListItemToChar( struct char_data *pCharacter, long lVnum );

bool assemblyAddComponent( long lVnum, long lComponentVnum, bool bExtract, bool bInRoom, int bItemsReq );
bool assemblyAddAltComponent( long lVnum, long lComponentVnum, long lAltComponentVnum );
int assemblyCheckComponents( long lVnum, struct char_data *pCharacter, int iType );
bool assemblyCreate( long lVnum, int iAssembledType );
bool assemblyDestroy( long lVnum );
bool assemblyHasComponent( long lVnum, long lComponentVnum );
bool assemblyHasAltComponent( long lVnum, long lAltComponentVnum );
bool assemblyRemoveComponent( long lVnum, long lComponentVnum );
bool assemblyRemoveAltComponent( long lVnum, long lAltComponentVnum );
bool assemblySetSkill( long lVnum, long lSkill, long lSkReq );
int assemblyCheckSkill( long lVnum, struct char_data *pCharacter );
bool assemblySetTime( long lVnum, long lTime );
int assemblyGetTime( long lVnum );
bool assemblySetHidden( long lVnum, bool bHidden );
int assemblyGetHidden( long lVnum );
bool assemblySetProduces( long lVnum, long lProduces );
int assemblyGetProduces( long lVnum );

bool assemblyAddByproduct( long lVnum, long lByproductVnum, int bItemsReq, int iWhen );
bool assemblyCheckByproducts( long lVnum, struct char_data *pCharacter );
bool assemblyHasByproduct( long lVnum, long lByproductVnum );
bool assemblyRemoveByproduct( long lVnum, long lByproductVnum );

int assemblyGetType( long lVnum );

long assemblyCountComponents( long lVnum );
long assemblyCountByproducts( long lVnum );
long assemblyFindAssembly( long pszAssemblyVnum );
long assemblyGetAssemblyIndex( long lVnum );
long assemblyGetComponentIndex( ASSEMBLY *pAssembly, long lComponentVnum );
long assemblyGetAltComponentIndex( ASSEMBLY *pAssembly, long lAltComponentVnum );
long assemblyGetByproductIndex( ASSEMBLY *pAssembly, long lByproductVnum );
bool assemblyLoadByproducts( long lVnum, struct char_data *pCharacter, bool bSuccess );

ASSEMBLY*	assemblyGetAssemblyPtr( long lVnum );
bool assemblySkillCheck( int skill, struct char_data *ch );
int assemblyGetSkillReq( long lVnum, struct char_data *pCharacter );

struct obj_data *assemblyCheckAlternates( long lVnum, long lComponent, struct obj_data *list );

/* ******************************************************************** */

/* event object structure for timed crafting */
struct assembly_event_obj {
	struct char_data *ch;		/* character that uses the skill			*/
	int method;							/* crafting method 										*/
	int time;								/* number of pulses to queue					*/
	long lVnum;							/* assembly vnum											*/
	long lProduces;					/* number of items to produce					*/
	ush_int skill;					/* skill to process										*/
	ush_int iColor;					/* color of the finished object				*/
	ush_int iResource;			/* resource used in the finished obj	*/
	const char *cmd;
};

#endif
