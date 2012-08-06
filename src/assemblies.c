/* ******************************************************************** *
 * FILE	: assemblies.c			Copyright (C) 1999 Geoff Davis              *
 * USAGE: Implementation for assembly engine.                           *
 * -------------------------------------------------------------------- *
 * 1999 MAY 07	gdavis/azrael@laker.net	Initial implementation.         *
 * 2002 JUNE 23 tbjers/artovil@arcanerealms.org Event hooks             *
 * -------------------------------------------------------------------- *
 * Skill additions Copyright (C) 2001-2002, Torgny Bjers.               *
 * artovil@arcanerealms.org | http://www.arcanerealms.org/              *
 * ******************************************************************** */
/* $Id: assemblies.c,v 1.28 2003/01/06 11:09:25 arcanere Exp $ */

#define	__ASSEMBLIES_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "assemblies.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "events.h"
#include "skills.h"
#include "color.h"


/* Local global variables. */
long g_lNumAssemblies = 0;
ASSEMBLY *g_pAssemblyTable = NULL;

/* External global variables. */
extern struct obj_data *obj_proto;
extern struct room_data *world;

/* extern functions */
const char *readable_proficiency(int percent);


void assemblyBootAssemblies( void )
{
	char szLine[ MAX_STRING_LENGTH ] = { '\0' };
	char szTag[ MAX_STRING_LENGTH ] = { '\0' };
	char szType[ MAX_STRING_LENGTH ] = { '\0' };
	int iExtract = 0;
	int iInRoom = 0;
	int iType = 0;
	int iSkill = 0;
	int iItemsReq = 1;
	int iWhen = 0;
	long lLineCount = 0;
	long lPartVnum = NOTHING;
	long lAltPartVnum = NOTHING;
	long lVnum = NOTHING;
	long lSkill = 0;
	long lSkReq = NOTHING;
	long lTime = 0;
	int bHidden = FALSE;
	long lProduces = 1;
	FILE *pFile = NULL;

	if( (pFile = fopen( ASSEMBLIES_FILE, "rt" )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyBootAssemblies(): Couldn't open file '%s' for "
			"reading.", ASSEMBLIES_FILE );
		return;
	}
	while( !feof( pFile ) ) {
		lLineCount += get_line( pFile, szLine );
		half_chop( szLine, szTag, szLine );

		if( *szTag == '\0' )
			continue;

		if( str_cmp( szTag, "Component" ) == 0 ) {
			if( sscanf( szLine, "#%ld %d %d %d", &lPartVnum, &iExtract, &iInRoom, &iItemsReq ) != 4 ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Invalid format in file %s, line %ld: szTag=%s, szLine=%s.", ASSEMBLIES_FILE, lLineCount, szTag, szLine );
			} else if( !assemblyAddComponent( lVnum, lPartVnum, iExtract, iInRoom, iItemsReq ) ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Could not add component #%ld to assembly #%ld.", lPartVnum, lVnum );
			}
		} else if( str_cmp( szTag, "AltComponent" ) == 0 ) {
			if( sscanf( szLine, "#%ld %ld", &lPartVnum, &lAltPartVnum ) != 2 ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Invalid format in file %s, line %ld: szTag=%s, szLine=%s.", ASSEMBLIES_FILE, lLineCount, szTag, szLine );
			} else if( !assemblyAddAltComponent( lVnum, lPartVnum, lAltPartVnum ) ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Could not add alternate component #%ld to assembly #%ld.", lAltPartVnum, lVnum );
			}
		} else if( str_cmp( szTag, "Byproduct" ) == 0 ) {
			if( sscanf( szLine, "#%ld %d %d", &lPartVnum, &iWhen, &iItemsReq ) != 3 ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Invalid format in file %s, line %ld: szTag=%s, szLine=%s.", ASSEMBLIES_FILE, lLineCount, szTag, szLine );
			} else if( !assemblyAddByproduct( lVnum, lPartVnum, iWhen, iItemsReq ) ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Could not add byproduct #%ld to assembly #%ld.", lPartVnum, lVnum );
			}
		} else if( str_cmp( szTag, "Vnum" ) == 0 ) {
			if( sscanf( szLine, "#%ld %s", &lVnum, szType ) != 2 ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Invalid format in file %s, line %ld.", ASSEMBLIES_FILE, lLineCount );
				lVnum = NOTHING;
			} else if( (iType = search_block( szType, AssemblyTypes, TRUE )) < 0 ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Invalid type '%s' for assembly vnum #%ld at line %ld.", szType, lVnum, lLineCount );
				lVnum = NOTHING;
			} else if( !assemblyCreate( lVnum, iType ) ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Could not create assembly for vnum #%ld, type %s.", lVnum, szType );
				lVnum = NOTHING;
			}
		} else if( str_cmp( szTag, "Skill" ) == 0 ) {
			if( sscanf( szLine, "#%ld %ld", &lSkill, &lSkReq ) != 2 ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Invalid format in file %s, line %ld.", ASSEMBLIES_FILE, lLineCount );
				lSkill = NOTHING;
			} else if( (iSkill = find_skill(lSkill)) < 0 ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Invalid skill %ld for assembly vnum #%ld at line %ld.", lSkill, lVnum, lLineCount );
				lSkill = NOTHING;
			} else if( lSkReq < 0 || lSkReq > 10000 ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Invalid skill percentage %ld for assembly vnum #%ld at line %ld.", lSkReq, lVnum, lLineCount );
				lSkill = NOTHING;
			} else if( !assemblySetSkill( lVnum, lSkill, lSkReq ) ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Could not set skill for assembly vnum #%ld, skill %ld.", lVnum, lSkill );
			}
		} else if( str_cmp( szTag, "Time" ) == 0 ) {
			if( sscanf( szLine, "#%ld", &lTime ) != 1 ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Invalid format in file %s, line %ld.", ASSEMBLIES_FILE, lLineCount );
				lTime = 0;
			} else if( !assemblySetTime( lVnum, lTime ) ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Could not set creation time for assembly vnum #%ld, time %ld.", lVnum, lTime );
			}
		} else if( str_cmp( szTag, "Hidden" ) == 0 ) {
			if( sscanf( szLine, "#%d", &bHidden ) != 1 ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Invalid format in file %s, line %ld.", ASSEMBLIES_FILE, lLineCount );
				bHidden = FALSE;
			} else if( !assemblySetHidden( lVnum, bHidden ) ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Could not set hidden state for assembly vnum #%ld.", lVnum );
			}
		} else if( str_cmp( szTag, "Produces" ) == 0 ) {
			if( sscanf( szLine, "#%ld", &lProduces ) != 1 ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Invalid format in file %s, line %ld.", ASSEMBLIES_FILE, lLineCount );
				lProduces = 1;
			} else if( !assemblySetProduces( lVnum, lProduces ) ) {
				extended_mudlog(BRF, SYSL_BUGS, TRUE, "bootAssemblies(): Could not set produce # for assembly vnum #%ld.", lVnum );
			}
		} else {
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "Invalid tag '%s' in file %s, line #%ld.", szTag, ASSEMBLIES_FILE, lLineCount );
		}

		*szLine = '\0';
		*szTag = '\0';
	}

	fclose( pFile );
}


void assemblySaveAssemblies( void )
{
	char szType[ MAX_STRING_LENGTH ] = { 
		'\0' 	};
	long i = 0;
	long j = 0;
	ASSEMBLY *pAssembly = NULL;
	FILE *pFile = NULL;

	if( (pFile = fopen( ASSEMBLIES_FILE, "wt" )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblySaveAssemblies(): Couldn't open file '%s' for writing.", ASSEMBLIES_FILE );
		return;
	}

	for( i = 0; i < g_lNumAssemblies; i++) {
		pAssembly = (g_pAssemblyTable + i);
		sprinttype( pAssembly->uchAssemblyType, AssemblyTypes, szType, sizeof(szType) );
		fprintf( pFile, "Vnum                #%ld %s\n", pAssembly->lVnum, szType );
		fprintf( pFile, "Skill               #%ld %ld\n", pAssembly->lSkill, pAssembly->lSkReq );
		fprintf( pFile, "Time                #%ld\n", pAssembly->lTime );
		fprintf( pFile, "Hidden              #%d\n", pAssembly->bHidden );
		fprintf( pFile, "Produces            #%ld\n", pAssembly->lProduces );

		for( j = 0; j < pAssembly->lNumComponents; j++ ) {
			fprintf( pFile, "Component           #%ld %d %d %d\n",
			pAssembly->pComponents[ j ].lVnum,
			(pAssembly->pComponents[ j ].bExtract ? 1 : 0),
			(pAssembly->pComponents[ j ].bInRoom ? 1 : 0),
			(pAssembly->pComponents[ j ].bItemsReq ? pAssembly->pComponents[ j ].bItemsReq : 1) );
		}

		for( j = 0; j < pAssembly->lNumAltComponents; j++ ) {
			fprintf( pFile, "AltComponent        #%ld %ld\n",
			pAssembly->aComponents[ j ].lComponent,
			pAssembly->aComponents[ j ].lVnum);
		}

		for( j = 0; j < pAssembly->lNumByproducts; j++ ) {
			fprintf( pFile, "Byproduct           #%ld %d %d\n",
			pAssembly->pByproducts[ j ].lVnum,
			pAssembly->pByproducts[ j ].iWhen,
			(pAssembly->pByproducts[ j ].bItemsReq ? pAssembly->pByproducts[ j ].bItemsReq : 1) );
		}

		if( i < g_lNumAssemblies - 1 )
			fprintf( pFile, "\n" );
	}

	fclose( pFile );
}


void assemblyListToChar( struct char_data *pCharacter )
{
	char *szBuffer = get_buffer( 65536 );
	char szAssmType[ MAX_INPUT_LENGTH ] = { '\0' };
	long i = 0; //Outer iterator.
	long lRnum = 0; //Object rnum for obj_proto indexing.

	if( pCharacter == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyListAssembliesToChar(): NULL 'pCharacter'." );
		return;
	} else if( g_pAssemblyTable == NULL ) {
		send_to_char( "No assemblies exist.\r\n", pCharacter );
		return;
	}

	/* Send out a "header" of sorts. */
	strcpy( szBuffer, "The following assemblies exists:\r\n" );

	for( i = 0; i < g_lNumAssemblies; i++ ) {
		if( (lRnum = real_object( g_pAssemblyTable[ i ].lVnum )) < 0 ) {
			strcat( szBuffer, "[-----] ***RESERVED***\r\n" );
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyListToChar(): Invalid vnum #%ld in assembly table.", g_pAssemblyTable[i].lVnum);
		} else {
			sprinttype( g_pAssemblyTable[ i ].uchAssemblyType, AssemblyTypes, szAssmType, sizeof(szAssmType) );
			sprintf( szBuffer, "%s[&W%5ld&n] &Y%s&n (%s) [%s]\r\n", szBuffer, g_pAssemblyTable[ i ].lVnum,
			obj_proto[ lRnum ].short_description, szAssmType, skill_name(g_pAssemblyTable[ i ].lSkill) );
		}
	}
	page_string( pCharacter->desc, szBuffer, TRUE );
	release_buffer( szBuffer );
}


void assemblyListGroupToChar( struct char_data *pCharacter, int asmType )
{
	char *szBuffer = get_buffer( 65536 );
	char obj_name[512];
	long i = 0; //Outer iterator.
	long lRnum = 0; //Object rnum for obj_proto indexing.
	long numAsms = 0;

	if( pCharacter == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyListGroupToChar(): NULL 'pCharacter'." );
		return;
	} else if( g_pAssemblyTable == NULL ) {
		send_to_char( "No assemblies exist.\r\n", pCharacter );
		release_buffer( szBuffer );
		return;
	}

	sprintf( szBuffer,	"&c------------------------------------------------------------------------------&n\r\n"
											"&gAvailable items that you can &G%s&g:\r\n"
											"&c==============================================================================&n\r\n"
											"  Item  Name                                      Skill\r\n"
											"&c------------------------------------------------------------------------------&n\r\n", AssemblyTypes[asmType] );

	for( i = 0; i < g_lNumAssemblies; i++ ) {
		if( (lRnum = real_object( g_pAssemblyTable[ i ].lVnum )) >= 0 ) {
			if( g_pAssemblyTable[ i ].uchAssemblyType == asmType && !g_pAssemblyTable[ i ].bHidden ) {
				struct obj_data *temp;
				temp = &obj_proto[ lRnum ];
				/* Strip all color codes for the sake of formatting. */
				strlcpy(obj_name, obj_proto[ lRnum ].short_description, sizeof(obj_name));
				proc_color(obj_name, 0, TRUE, GET_OBJ_COLOR( temp ), GET_OBJ_RESOURCE( temp ));
				numAsms++;
				sprintf( szBuffer, "%s[&c%5ld&n] &y%-40.40s&n [&c%s&n]\r\n", szBuffer, g_pAssemblyTable[ i ].lVnum,
				obj_name, skill_name(g_pAssemblyTable[ i ].lSkill) );
			}
		}
	}

	if ( numAsms > 0 )
		sprintf( szBuffer,	"%s&c------------------------------------------------------------------------------&n\r\n"
												"For detailed item information: components <item number>\r\n"
												"Usage: %s [ <item number> | last ]\r\n", szBuffer, AssemblyTypeNames[asmType][ASSM_TYPE_TO] );
	else
		sprintf( szBuffer, "Currently there are no available items that you can &W%s&n.\r\n", AssemblyTypes[asmType] );

	page_string( pCharacter->desc, szBuffer, TRUE );
	release_buffer( szBuffer );

}


void assemblyListItemToChar( struct char_data *pCharacter, long lVnum )
{
	char *szBuffer = get_buffer( 65536 );
	char obj_name[512];
	long i = 0; //Outer iterator.
	long j = 0; //Inner iterator.
	long lRnum = 0; //Object rnum for obj_proto indexing.
	int numComponents = 0;

	if( pCharacter == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyListGroupToChar(): NULL 'pCharacter'." );
		return;
	} else if( g_pAssemblyTable == NULL ) {
		send_to_char( "No assemblies exist.\r\n", pCharacter );
		release_buffer( szBuffer );
		return;
	}

	for( i = 0; i < g_lNumAssemblies; i++ )
		if( g_pAssemblyTable[ i ].lVnum == lVnum )
			break;

	if( (lRnum = real_object( g_pAssemblyTable[ i ].lVnum )) > 0 && !g_pAssemblyTable[ i ].bHidden ) {
		struct obj_data *temp;
		temp = &obj_proto[ lRnum ];
		strlcpy(obj_name, obj_proto[ lRnum ].short_description, sizeof(obj_name));
		/* Strip all color codes for the sake of formatting. */
		proc_color(obj_name, 0, TRUE, GET_OBJ_COLOR( temp ), GET_OBJ_RESOURCE( temp ));
		sprintf( szBuffer,
			"--Detailed crafting information\r\n"
			"Item number    : &c%-5ld&n\r\n"
			"Item name      : &y%s&n\r\n"
			"Item skill     : &c%s&n\r\n"
			"Skill required : &y%s&n\r\n"
			"Components     :\r\n",
			g_pAssemblyTable[ i ].lVnum,
			obj_name,
			skill_name(g_pAssemblyTable[ i ].lSkill),
			readable_proficiency(g_pAssemblyTable[ i ].lSkReq / 100)
		);

		for( j = 0; j < g_pAssemblyTable[ i ].lNumComponents; j++ ) {
			if( (lRnum = real_object( g_pAssemblyTable[ i ].pComponents[ j ].lVnum )) > 0 ) {
				temp = &obj_proto[ lRnum ];
				strlcpy(obj_name, obj_proto[ lRnum ].short_description, sizeof(obj_name));
				/* Strip all color codes for the sake of formatting. */
				proc_color(obj_name, 0, TRUE, GET_OBJ_COLOR( temp ), GET_OBJ_RESOURCE( temp ));
				numComponents++;
				sprintf( szBuffer, "%s  (%d) %-55.55s  (%s)\r\n", szBuffer,
					(g_pAssemblyTable[ i ].pComponents[ j ].bItemsReq ? g_pAssemblyTable[ i ].pComponents[ j ].bItemsReq : 1),
					obj_name, ((g_pAssemblyTable[ i ].pComponents[ j ].bInRoom) ? "In room" : "In inventory")
				);
			}
		}

		if (!numComponents) {
			sprintf( szBuffer, "%s  no components have been assigned.\r\n", szBuffer );
		}

		page_string( pCharacter->desc, szBuffer, TRUE );
	} else
		send_to_char( "No such item.\r\n", pCharacter );

	release_buffer( szBuffer );

}


bool assemblyAddComponent( long lVnum, long lComponentVnum, bool bExtract, bool bInRoom, int bItemsReq )
{
	ASSEMBLY *pAssembly = NULL;
	COMPONENT *pNewComponents = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyAddComponent(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	} else if( real_object( lComponentVnum ) < 0 ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyAddComponent(): Invalid 'lComponentVnum' #%ld.",
		lComponentVnum );
		return (FALSE);
	} else if( assemblyHasComponent( lVnum, lComponentVnum ) ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyAddComponent(): Assembly for vnum #%ld already has component vnum #%ld.", lVnum, lComponentVnum );
		return (FALSE);
	}

	/* Create a new component table with room for one more entry. */
	CREATE( pNewComponents, COMPONENT, pAssembly->lNumComponents + 1 );

	if( pAssembly->pComponents != NULL ) {
		/* Copy the old table over to the new. */
		memmove( pNewComponents, pAssembly->pComponents, pAssembly->lNumComponents * sizeof( COMPONENT ) );
		free( pAssembly->pComponents );
	}

	/*
		 * Assign the new component table and setup the new component entry. Then
		 * add increment the number of components.
		 */

	pAssembly->pComponents = pNewComponents;
	pAssembly->pComponents[ pAssembly->lNumComponents ].lVnum = lComponentVnum;
	pAssembly->pComponents[ pAssembly->lNumComponents ].bExtract = bExtract;
	pAssembly->pComponents[ pAssembly->lNumComponents ].bInRoom = bInRoom;
	pAssembly->pComponents[ pAssembly->lNumComponents ].bItemsReq = bItemsReq;
	pAssembly->lNumComponents += 1;

	return (TRUE);
}


bool assemblyAddAltComponent( long lVnum, long lComponentVnum, long lAltComponentVnum )
{
	ASSEMBLY *pAssembly = NULL;
	ALTCOMPONENT *pNewComponents = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyAddAltComponent(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	} else if( real_object( lComponentVnum ) < 0 ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyAddAltComponent(): Invalid 'lComponentVnum' #%ld.",
		lComponentVnum );
		return (FALSE);
	} else if( real_object( lAltComponentVnum ) < 0 ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyAddAltComponent(): Invalid 'lAltComponentVnum' #%ld.",
		lComponentVnum );
		return (FALSE);
	} else if( assemblyHasAltComponent( lVnum, lAltComponentVnum ) ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyAddAltComponent(): Assembly for vnum #%ld already has alternate component vnum #%ld.", lVnum, lComponentVnum );
		return (FALSE);
	}

	/* Create a new component table with room for one more entry. */
	CREATE( pNewComponents, ALTCOMPONENT, pAssembly->lNumAltComponents + 1 );

	if( pAssembly->aComponents != NULL ) {
		/* Copy the old table over to the new. */
		memmove( pNewComponents, pAssembly->aComponents, pAssembly->lNumAltComponents * sizeof( ALTCOMPONENT ) );
		free( pAssembly->aComponents );
	}

	/*
		 * Assign the new component table and setup the new component entry. Then
		 * add increment the number of components.
		 */

	pAssembly->aComponents = pNewComponents;
	pAssembly->aComponents[ pAssembly->lNumAltComponents ].lVnum = lAltComponentVnum;
	pAssembly->aComponents[ pAssembly->lNumAltComponents ].lComponent = lComponentVnum;
	pAssembly->lNumAltComponents += 1;

	return (TRUE);
}


int assemblyCheckComponents( long lVnum, struct char_data *pCharacter, int iType )
{
	bool bOk = TRUE;
	int iColor = 0, iResource = -1, iResourceType = -1;
	long i = 0, j = 0, k = 0;
	long lRnum = 0, totalNum = 0;
	struct obj_data **ppComponentObjects = NULL;
	ASSEMBLY *pAssembly = NULL;

	if( pCharacter == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "NULL assemblyCheckComponents(): 'pCharacter'." );
		return (FALSE);
	} else if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "NULL assemblyCheckComponents(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	}

	if( pAssembly->pComponents == NULL )
		return (FALSE);
	else if( pAssembly->lNumComponents <= 0 )
		return (FALSE);

	/*
	 * Get the total number of items we are to load into the
	 * ppComponentObjects array.
	 */
	for( i = 0; i < pAssembly->lNumComponents && bOk; i++ ) {
		if( (lRnum = real_object( pAssembly->pComponents[ i ].lVnum )) < 0 )
			bOk = FALSE;
		else {
			if ( pAssembly->pComponents[ i ].bItemsReq )
				totalNum += pAssembly->pComponents[ i ].bItemsReq;
			else
				bOk = FALSE;
		}
	}

	/*
	 * Create a temporary storage for all objects.
	 */
	CREATE( ppComponentObjects, struct obj_data*, totalNum );

	/*
	 * Move all objects from char/room and put them
	 * in the ppComponentObjects storage.
	 */
	for( i = 0; i < pAssembly->lNumComponents && bOk; i++ ) {
		lRnum = real_object( pAssembly->pComponents[ i ].lVnum );
		for( j = 0; j < pAssembly->pComponents[ i ].bItemsReq; j++ ) {
			if( pAssembly->pComponents[ i ].bInRoom ) {
				if ( (ppComponentObjects[ k ] = get_obj_in_list_vnum( pAssembly->pComponents[ i ].lVnum, world[ IN_ROOM( pCharacter ) ].contents )) == NULL ) {
					if ( (ppComponentObjects[ k ] = assemblyCheckAlternates( pAssembly->lVnum, pAssembly->pComponents[ i ].lVnum, world[ IN_ROOM( pCharacter ) ].contents )) == NULL )
						bOk = FALSE;
					else
						obj_from_room( ppComponentObjects[ k ] );
				} else
					obj_from_room( ppComponentObjects[ k ] );
			} else {
				if ( (ppComponentObjects[ k ] = get_obj_in_list_vnum( pAssembly->pComponents[ i ].lVnum, pCharacter->carrying )) == NULL ) {
					if ( (ppComponentObjects[ k ] = assemblyCheckAlternates( pAssembly->lVnum, pAssembly->pComponents[ i ].lVnum, pCharacter->carrying )) == NULL )
						bOk = FALSE;
					else
						obj_from_char( ppComponentObjects[ k ] );
				} else
					obj_from_char( ppComponentObjects[ k ] );
			}
			k++;
		}
	}

	k = 0;

	/*
	 * Loop through all the objects loaded into the
	 * ppComponentObjects array.
	 * Torgny Bjers (artovil@arcanerealms.org), 2003-01-06
	 */
	for( i = 0; i < pAssembly->lNumComponents; i++ ) {
		/*
		 * Loop through the number of required objects per
		 * component.
		 */
		for( j = 0; j < pAssembly->pComponents[ i ].bItemsReq; j++ ) {
			/*
			 * Go away, nasty NULL.
			 */
			if( ppComponentObjects[ k ] == NULL )
				continue;
			/*
			 * Based on iType we either check color or resource,
			 * or we actually go ahead and process components.
			 */
			switch ( iType ) {
			/*
			 * Checking for color
			 */
			case COMPONENT_COLOR:
				if ( pAssembly->pComponents[ i ].bInRoom ) {
					// First, check the room
					if ( GET_OBJ_COLOR( ppComponentObjects[ k ] ) > 0 && OBJ_FLAGGED( ppComponentObjects[ k ], ITEM_COLORIZE ) )
						if ( iColor == 0 )
							iColor = GET_OBJ_COLOR( ppComponentObjects[ k ] );
					obj_to_room( ppComponentObjects[ k ], IN_ROOM( pCharacter ) );
				} else {
					// Then check inventory
					if ( GET_OBJ_COLOR( ppComponentObjects[ k ] ) > 0 && OBJ_FLAGGED( ppComponentObjects[ k ], ITEM_COLORIZE ) )
						if ( iColor == 0 )
							iColor = GET_OBJ_COLOR( ppComponentObjects[ k ] );
					obj_to_char( ppComponentObjects[ k ], pCharacter );
				}
				break;
			/*
			 * Checking for resource type
			 */
			case COMPONENT_RESOURCE:
				if ( pAssembly->pComponents[ i ].bInRoom ) {
					// First, check the room
					if ( GET_OBJ_TYPE( ppComponentObjects[ k ] ) == ITEM_RESOURCE )
						iResource = GET_OBJ_RESOURCE( ppComponentObjects[ k ] );
					obj_to_room( ppComponentObjects[ k ], IN_ROOM( pCharacter ) );
				} else {
					// Then check inventory
					if ( GET_OBJ_TYPE( ppComponentObjects[ k ] ) == ITEM_RESOURCE )
						iResource = GET_OBJ_RESOURCE( ppComponentObjects[ k ] );
					obj_to_char( ppComponentObjects[ k ], pCharacter );
				}
				if ( iResourceType == -1 )
					iResourceType = iResource;
				else if ( iResource != iResourceType )
					iResourceType = -2;
				break;
			/*
			 * We are not checking, either extract on success,
			 * or put the items back if we did not get bOk.
			 */
			default:
				if( pAssembly->pComponents[ i ].bExtract && bOk )
					extract_obj( ppComponentObjects[ k ] );
				else if( pAssembly->pComponents[ i ].bInRoom )
					obj_to_room( ppComponentObjects[ k ], IN_ROOM( pCharacter ) );
				else
					obj_to_char( ppComponentObjects[ k ], pCharacter );
				break;
			} // switch ( iType )
			k++;
		} // pAssembly->pComponents[ i ].bItemsReq
	} // pAssembly->lNumComponents

	free( ppComponentObjects );

	switch ( iType ) {
	case COMPONENT_COLOR: 		return ( iColor );
	case COMPONENT_RESOURCE:	return ( iResourceType );
	default:									return (bOk);
	}
}


bool assemblySetSkill( long lVnum, long lSkill, long lSkReq )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblySetSkill(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	} else if( find_skill( lSkill ) < 0 ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblySetSkill(): Invalid 'lSkill' #%ld.", lSkill );
		return (FALSE);
	} else if( lSkReq < 0 || lSkReq > 10000 ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblySetSkill(): Invalid skill percentage %ld for assembly vnum #%ld.", lSkReq, lVnum );
		return (FALSE);
	}

	pAssembly->lSkill = lSkill;
	pAssembly->lSkReq = lSkReq;

	return (TRUE);
}


int assemblyCheckSkill( long lVnum, struct char_data *pCharacter )
{
	ASSEMBLY *pAssembly = NULL;

	if( pCharacter == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "NULL assemlyCheckSkill(): 'pCharacter'." );
		return (-1);
	} else if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "NULL assemlyCheckSkill(): Invalid 'lVnum' #%ld.", lVnum );
		return (-1);
	}

	if ( GET_SKILL(pCharacter, pAssembly->lSkill) <= 0 && pAssembly->lSkReq > 0 )
		return (-1);

	if ( pAssembly->lSkReq > GET_SKILL(pCharacter, pAssembly->lSkill) )
		return (-1);

	return ( pAssembly->lSkill );
}


int assemblyGetSkillReq( long lVnum, struct char_data *pCharacter )
{
	ASSEMBLY *pAssembly = NULL;

	if( pCharacter == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "NULL assemlyCheckSkill(): 'pCharacter'." );
		return (-1);
	} else if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "NULL assemlyCheckSkill(): Invalid 'lVnum' #%ld.", lVnum );
		return (-1);
	}

	return ( pAssembly->lSkReq );
}


bool assemblyCreate( long lVnum, int iAssembledType )
{
	long lBottom = 0;
	long lMiddle = 0;
	long lTop = 0;
	ASSEMBLY *pNewAssemblyTable = NULL;

	if( lVnum < 0 )
		return (FALSE);
	else if( iAssembledType < 0 || iAssembledType >= MAX_ASSM )
		return (FALSE);

	if( g_pAssemblyTable == NULL ) {
		CREATE( g_pAssemblyTable, ASSEMBLY, 1 );
		g_lNumAssemblies = 1;
	} else {
		lTop = g_lNumAssemblies - 1;

		for( ;; ) {
			lMiddle = (lBottom + lTop) /2;

			if( g_pAssemblyTable[ lMiddle ].lVnum == lVnum )
				return (FALSE);
			else if( lBottom >= lTop )
				break;
			else if( g_pAssemblyTable[ lMiddle ].lVnum > lVnum )
				lTop = lMiddle - 1;
			else
				lBottom = lMiddle + 1;
		}

		if( g_pAssemblyTable[ lMiddle ].lVnum <= lVnum )
			lMiddle += 1;

		CREATE( pNewAssemblyTable, ASSEMBLY, g_lNumAssemblies + 1 );

		if( lMiddle > 0 )
			memmove( pNewAssemblyTable, g_pAssemblyTable, lMiddle * sizeof( ASSEMBLY ) );

		if( lMiddle <= g_lNumAssemblies - 1 )
			memmove( pNewAssemblyTable + lMiddle + 1, g_pAssemblyTable + lMiddle, (g_lNumAssemblies - lMiddle) * sizeof( ASSEMBLY ) );

		free( g_pAssemblyTable );
		g_pAssemblyTable = pNewAssemblyTable;
		g_lNumAssemblies += 1;
	}

	g_pAssemblyTable[ lMiddle ].lNumComponents = 0;
	g_pAssemblyTable[ lMiddle ].lNumByproducts = 0;
	g_pAssemblyTable[ lMiddle ].lVnum = lVnum;
	g_pAssemblyTable[ lMiddle ].pComponents = NULL;
	g_pAssemblyTable[ lMiddle ].pByproducts = NULL;
	g_pAssemblyTable[ lMiddle ].uchAssemblyType = (unsigned char) iAssembledType;
	g_pAssemblyTable[ lMiddle ].lProduces = 1;
	g_pAssemblyTable[ lMiddle ].bHidden = FALSE;

	return (TRUE);
}


bool assemblyDestroy( long lVnum )
{
	long lIndex = 0;
	ASSEMBLY *pNewAssemblyTable = NULL;

	/* Find the real number of the assembled vnum. */
	if( g_pAssemblyTable == NULL || (lIndex = assemblyGetAssemblyIndex( lVnum )) < 0 ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyDestroy(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	}

	/* Deallocate component array. */
	if( g_pAssemblyTable[ lIndex ].pComponents != NULL )
		free( g_pAssemblyTable[ lIndex ].pComponents );

	/* Deallocate byproducts array. */
	if( g_pAssemblyTable[ lIndex ].pByproducts != NULL )
		free( g_pAssemblyTable[ lIndex ].pByproducts );

	if( g_lNumAssemblies > 1 ) {
		/* Create a new table, the same size as the old one less one item. */
		CREATE( pNewAssemblyTable, ASSEMBLY, g_lNumAssemblies - 1 );

		/* Copy all assemblies before the one removed into the new table. */
		if( lIndex > 0 )
			memmove( pNewAssemblyTable, g_pAssemblyTable, lIndex * sizeof( ASSEMBLY ) );

		/* Copy all assemblies after the one removed into the new table. */
		if( lIndex < g_lNumAssemblies - 1 ) {
			memmove( pNewAssemblyTable + lIndex, g_pAssemblyTable + lIndex + 1, (g_lNumAssemblies - lIndex - 1) *
				sizeof( ASSEMBLY ) );
		}
	}

	/* Deallocate the old table. */
	free( g_pAssemblyTable );

	/* Decrement the assembly count and assign the new table. */
	g_lNumAssemblies -= 1;
	g_pAssemblyTable = pNewAssemblyTable;

	return (TRUE);
}


bool assemblyHasComponent( long lVnum, long lComponentVnum )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyHasComponent(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	}

	return (assemblyGetComponentIndex( pAssembly, lComponentVnum ) >= 0);
}


bool assemblyHasAltComponent( long lVnum, long lAltComponentVnum )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyHasAltComponent(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	}

	return (assemblyGetAltComponentIndex( pAssembly, lAltComponentVnum ) >= 0);
}


bool assemblyRemoveComponent( long lVnum, long lComponentVnum )
{
	long lIndex = 0;
	ASSEMBLY *pAssembly = NULL;
	COMPONENT *pNewComponents = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyRemoveComponent(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	} else if( (lIndex = assemblyGetComponentIndex( pAssembly, lComponentVnum )) < 0 ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyRemoveComponent(): Vnum #%ld is not a component of assembled vnum #%ld.", lComponentVnum, lVnum );
		return (FALSE);
	}

	if( pAssembly->pComponents != NULL && pAssembly->lNumComponents > 1 ) {
		CREATE( pNewComponents, COMPONENT, pAssembly->lNumComponents - 1 );

		if( lIndex > 0 )
			memmove( pNewComponents, pAssembly->pComponents, lIndex * sizeof( COMPONENT ) );

		if( lIndex < pAssembly->lNumComponents - 1 ) {
			memmove( pNewComponents + lIndex, pAssembly->pComponents + lIndex + 1,
			(pAssembly->lNumComponents - lIndex - 1) * sizeof( COMPONENT ) );
		}
	}

	free( pAssembly->pComponents );
	pAssembly->pComponents = pNewComponents;
	pAssembly->lNumComponents -= 1;

	return (TRUE);
}


bool assemblyRemoveAltComponent( long lVnum, long lAltComponentVnum )
{
	long lIndex = 0;
	ASSEMBLY *pAssembly = NULL;
	ALTCOMPONENT *pNewComponents = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyRemoveAltComponent(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	} else if( (lIndex = assemblyGetAltComponentIndex( pAssembly, lAltComponentVnum )) < 0 ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyRemoveAltComponent(): Vnum #%ld is not an alternate component of assembled vnum #%ld.", lAltComponentVnum, lVnum );
		return (FALSE);
	}

	if( pAssembly->aComponents != NULL && pAssembly->lNumAltComponents > 1 ) {
		CREATE( pNewComponents, ALTCOMPONENT, pAssembly->lNumAltComponents - 1 );

		if( lIndex > 0 )
			memmove( pNewComponents, pAssembly->aComponents, lIndex * sizeof( ALTCOMPONENT ) );

		if( lIndex < pAssembly->lNumAltComponents - 1 ) {
			memmove( pNewComponents + lIndex, pAssembly->aComponents + lIndex + 1,
			(pAssembly->lNumAltComponents - lIndex - 1) * sizeof( ALTCOMPONENT ) );
		}
	}

	free( pAssembly->aComponents );
	pAssembly->aComponents = pNewComponents;
	pAssembly->lNumAltComponents -= 1;

	return (TRUE);
}


int assemblyGetType( long lVnum )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyGetType(): Invalid 'lVnum' #%ld.", lVnum );
		return (-1);
	}

	return ((int) pAssembly->uchAssemblyType);
}


long assemblyCountComponents( long lVnum )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyCountComponents(): Invalid 'lVnum' #%ld.", lVnum );
		return (0);
	}

	return (pAssembly->lNumComponents);
}


long assemblyFindAssembly( long pszAssemblyVnum )
{
	long i = 0;
	long lRnum = NOTHING;

	if( g_pAssemblyTable == NULL )
		return (-1);
	else if( pszAssemblyVnum <= 0 )
		return (-1);

	for( i = 0; i < g_lNumAssemblies; i++ ) {
		if( (lRnum = real_object( g_pAssemblyTable[ i ].lVnum )) < 0 )
			extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyFindAssembly(): Invalid vnum #%ld in assembly table.", g_pAssemblyTable[i].lVnum );
		else if( pszAssemblyVnum == g_pAssemblyTable[ i ].lVnum )
			return (g_pAssemblyTable[ i ].lVnum);
	}

	return (-1);
}


long assemblyGetAssemblyIndex( long lVnum )
{
	long lBottom = 0;
	long lMiddle = 0;
	long lTop = 0;

	lTop = g_lNumAssemblies - 1;

	for( ;; ) {
		lMiddle = (lBottom + lTop) /2;

		if( g_pAssemblyTable[ lMiddle ].lVnum == lVnum )
			return (lMiddle);
		else if( lBottom >= lTop )
			return (-1);
		else if( g_pAssemblyTable[ lMiddle ].lVnum > lVnum )
			lTop = lMiddle - 1;
		else
			lBottom = lMiddle + 1;
	}
}


long assemblyGetComponentIndex( ASSEMBLY *pAssembly, long lComponentVnum )
{
	long i = 0;

	if( pAssembly == NULL )
		return (-1);

	for( i = 0; i < pAssembly->lNumComponents; i++ ) {
		if( pAssembly->pComponents[ i ].lVnum == lComponentVnum )
			return (i);
	}

	return (-1);
}


long assemblyGetAltComponentIndex( ASSEMBLY *pAssembly, long lAltComponentVnum )
{
	long i = 0;

	if( pAssembly == NULL )
		return (-1);

	for( i = 0; i < pAssembly->lNumAltComponents; i++ ) {
		if( pAssembly->aComponents[ i ].lVnum == lAltComponentVnum )
			return (i);
	}

	return (-1);
}


ASSEMBLY* assemblyGetAssemblyPtr( long lVnum )
{
	long lIndex = 0;

	if( g_pAssemblyTable == NULL )
		return (NULL);

	if( (lIndex = assemblyGetAssemblyIndex( lVnum )) >= 0 )
		return (g_pAssemblyTable + lIndex);

	return (NULL);
}


bool assemblySkillCheck( int skill, struct char_data *ch )
{
	if (!GET_SKILL(ch, skill))
		return (FALSE);
	else
		return (TRUE);
}


bool assemblySetTime( long lVnum, long lTime )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "%s: Invalid 'lVnum' #%ld.", __FUNCTION__, lVnum );
		return (FALSE);
	} else if( lTime < 0 || lTime > 20 ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "%s: Invalid time %ld for assembly vnum #%ld.", __FUNCTION__, lTime, lVnum );
		return (FALSE);
	}

	pAssembly->lTime = lTime;

	return (TRUE);
}


int assemblyGetTime( long lVnum )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "%s: Invalid 'lVnum' #%ld.", __FUNCTION__, lVnum );
		return (FALSE);
	}

	return(pAssembly->lTime);

}


bool assemblySetHidden( long lVnum, bool bHidden )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "%s: Invalid 'lVnum' #%ld.", __FUNCTION__, lVnum );
		return (FALSE);
	}

	pAssembly->bHidden = bHidden;

	return (TRUE);
}


int assemblyGetHidden( long lVnum )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "%s: Invalid 'lVnum' #%ld.", __FUNCTION__, lVnum );
		return (FALSE);
	}

	return(pAssembly->bHidden);

}


bool assemblySetProduces( long lVnum, long lProduces )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "%s: Invalid 'lVnum' #%ld.", __FUNCTION__, lVnum );
		return (FALSE);
	}

	pAssembly->lProduces = LIMIT(lProduces, 1, 25);

	return (TRUE);
}


int assemblyGetProduces( long lVnum )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "%s: Invalid 'lVnum' #%ld.", __FUNCTION__, lVnum );
		return (FALSE);
	}

	return(LIMIT(pAssembly->lProduces, 1, 25));

}


bool assemblyAddByproduct( long lVnum, long lByproductVnum, int bItemsReq, int iWhen )
{
	ASSEMBLY *pAssembly = NULL;
	BYPRODUCT *pNewByproducts = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyAddByproduct(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	} else if( real_object( lByproductVnum ) < 0 ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyAddByproduct(): Invalid 'lComponentVnum' #%ld.", lByproductVnum );
		return (FALSE);
	} else if( assemblyHasByproduct( lVnum, lByproductVnum ) ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyAddByproduct(): Assembly for vnum #%ld already has byproduct vnum #%ld.", lVnum, lByproductVnum );
		return (FALSE);
	}

	/* Create a new component table with room for one more entry. */
	CREATE( pNewByproducts, BYPRODUCT, pAssembly->lNumByproducts + 1 );

	if( pAssembly->pByproducts != NULL ) {
		/* Copy the old table over to the new. */
		memmove( pNewByproducts, pAssembly->pByproducts, pAssembly->lNumByproducts * sizeof( BYPRODUCT ) );
		free( pAssembly->pByproducts );
	}

	/*
		 * Assign the new component table and setup the new component entry. Then
		 * add increment the number of components.
		 */

	pAssembly->pByproducts = pNewByproducts;
	pAssembly->pByproducts[ pAssembly->lNumByproducts ].lVnum = lByproductVnum;
	pAssembly->pByproducts[ pAssembly->lNumByproducts ].bItemsReq = bItemsReq;
	pAssembly->pByproducts[ pAssembly->lNumByproducts ].iWhen = iWhen;
	pAssembly->lNumByproducts += 1;

	return (TRUE);
}


bool assemblyHasByproduct( long lVnum, long lByproductVnum )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyHasByproduct(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	}

	return (assemblyGetByproductIndex( pAssembly, lByproductVnum ) >= 0);
}


bool assemblyRemoveByproduct( long lVnum, long lByproductVnum )
{
	long lIndex = 0;
	ASSEMBLY *pAssembly = NULL;
	BYPRODUCT *pNewByproducts = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyRemoveByproduct(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	} else if( (lIndex = assemblyGetByproductIndex( pAssembly, lByproductVnum )) < 0 ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyRemoveByproduct(): Vnum #%ld is not a component of assembled vnum #%ld.", lByproductVnum, lVnum );
		return (FALSE);
	}

	if( pAssembly->pByproducts != NULL && pAssembly->lNumByproducts > 1 ) {
		CREATE( pNewByproducts, BYPRODUCT, pAssembly->lNumByproducts - 1 );

		if( lIndex > 0 )
			memmove( pNewByproducts, pAssembly->pByproducts, lIndex * sizeof( BYPRODUCT ) );

		if( lIndex < pAssembly->lNumByproducts - 1 ) {
			memmove( pNewByproducts + lIndex, pAssembly->pByproducts + lIndex + 1,
			(pAssembly->lNumByproducts - lIndex - 1) * sizeof( BYPRODUCT ) );
		}
	}

	free( pAssembly->pByproducts );
	pAssembly->pByproducts = pNewByproducts;
	pAssembly->lNumByproducts -= 1;

	return (TRUE);
}


long assemblyCountByproducts( long lVnum )
{
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "assemblyCountComponents(): Invalid 'lVnum' #%ld.", lVnum );
		return (0);
	}

	return (pAssembly->lNumByproducts);
}


long assemblyGetByproductIndex( ASSEMBLY *pAssembly, long lByproductVnum )
{
	long i = 0;

	if( pAssembly == NULL )
		return (-1);

	for( i = 0; i < pAssembly->lNumByproducts; i++ ) {
		if( pAssembly->pByproducts[ i ].lVnum == lByproductVnum )
			return (i);
	}

	return (-1);
}


bool assemblyLoadByproducts( long lVnum, struct char_data *pCharacter, bool bSuccess )
{
	bool bOk = TRUE;
	long i = 0, j = 0;
	ASSEMBLY *pAssembly = NULL;
	struct obj_data *pObject = NULL;

	if( pCharacter == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "NULL assemblyLoadByproducts(): 'pCharacter'." );
		return (FALSE);
	} else if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "NULL assemblyLoadByproducts(): Invalid 'lVnum' #%ld.", lVnum );
		return (FALSE);
	}

	if( pAssembly->pByproducts == NULL )
		return (FALSE);
	else if( pAssembly->lNumByproducts <= 0 )
		return (FALSE);

	for( i = 0; i < pAssembly->lNumByproducts && bOk; i++ ) {
		if ( ( pObject = read_object( pAssembly->pByproducts[ i ].lVnum, VIRTUAL ) ) != NULL ) {
			for ( j = 0; j < pAssembly->pByproducts[ i ].bItemsReq; j++ ) {
				if ( j > 0 ) /* we already created the first object above, need to create one for each further iteration. */
					pObject = read_object( pAssembly->pByproducts[ i ].lVnum, VIRTUAL );
				if ( pAssembly->pByproducts[ i ].iWhen == bSuccess || pAssembly->pByproducts[ i ].iWhen == ASSM_BYPROD_ALWAYS ) {
					obj_to_char( pObject, pCharacter );
					sprintf(buf, "%s $p (%d).", (bSuccess) ? "You also get" : "Although you get", pAssembly->pByproducts[ i ].bItemsReq);
					act(buf, FALSE, pCharacter, pObject, NULL, TO_CHAR);
				}
			}
		} else {
			bOk = FALSE;
		}
	}

	return (bOk);
}


/*
 * Search for alternate components in a list,
 * either inventory or room.
 * Torgny Bjers (artovil@arcanerealms.org), 2003-01-06
 */
struct obj_data *assemblyCheckAlternates( long lVnum, long lComponent, struct obj_data *list )
{
	struct obj_data *i;
	long j;
	ASSEMBLY *pAssembly = NULL;

	if( (pAssembly = assemblyGetAssemblyPtr( lVnum )) == NULL ) {
		extended_mudlog(BRF, SYSL_BUGS, TRUE, "NULL %s(): Invalid 'lVnum' #%ld.", __FUNCTION__, lVnum );
		return (NULL);
	}

	if( pAssembly->aComponents == NULL )
		return (NULL);
	else if( pAssembly->lNumAltComponents <= 0 )
		return (NULL);

	/*
	 * Nasty recursive search through all alternate components
	 * and then search through the object list, if found, return.
	 */
	for ( j = 0; j < pAssembly->lNumAltComponents; j++ )
		if ( pAssembly->aComponents[ j ].lComponent == lComponent )
			for ( i = list; i; i = i->next_content )
				if ( GET_OBJ_PROTOVNUM(i) == pAssembly->aComponents[ j ].lVnum )
					return ( i );
			
	return (NULL);
}

#undef __ASSEMBLIES_C__
