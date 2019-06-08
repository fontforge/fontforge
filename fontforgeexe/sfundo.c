/******************************************************************************
*******************************************************************************
*******************************************************************************

    Copyright (C) 2013 Ben Martin

    This file is part of FontForge.

    FontForge is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation,either version 3 of the License, or
    (at your option) any later version.

    FontForge is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FontForge.  If not, see <http://www.gnu.org/licenses/>.

    For more details see the COPYING.gplv3 file in the root directory of this
    distribution.

*******************************************************************************
*******************************************************************************
******************************************************************************/

#include <fontforge-config.h>

#include "sfundo.h"

#include "fvfonts.h"
#include "sfd.h"
#include "uiinterface.h"
#include "views.h"

#include <string.h>

void SFUndoFreeAssociated( struct sfundoes *undo )
{
    if( undo->sfdchunk )
	free( undo->sfdchunk );
}

void SFUndoFree( struct sfundoes *undo )
{
    SFUndoFreeAssociated( undo );
    free( undo );
}

void SFUndoRemoveAndFree( SplineFont *sf, struct sfundoes *undo )
{
    SFUndoFreeAssociated( undo );
    dlist_erase( (struct dlistnode **)&sf->undoes, (struct dlistnode *)undo );
    free(undo);
}


SFUndoes* SFUndoCreateSFD( enum sfundotype t, char* staticmsg, char* sfdfrag )
{
    SFUndoes* undo = chunkalloc(sizeof(SFUndoes));
    undo->ln.next = 0;
    undo->ln.prev = 0;
    undo->msg  = staticmsg;
    undo->type = t;
    undo->sfdchunk = sfdfrag;
    return undo;
}

char* SFUndoToString( SFUndoes* undo )
{
    FILE* sfd;
    if( (sfd = MakeTemporaryFile()) )
    {
	fprintf(sfd,"BeginFontLevelUndo\n");
	fprintf(sfd,"FontLevelUndoType:%d\n",undo->type);
	fprintf(sfd,"FontLevelUndoMessage:%s\n",undo->msg);
	if( undo->sfdchunk )
	    fprintf(sfd,"%s\n", undo->sfdchunk );
	fprintf(sfd,"EndFontLevelUndo\n");

	char* str = FileToAllocatedString( sfd );
	fclose( sfd );
	return str;
    }
    return 0;
}

static char* findterm( char** str, char* term )
{
    char* p;
    if( (p = strstr( *str, term )) )
    {
	p += strlen( term );
	char* e = p;
	while( *e && *e!='\n')
	    e++;
	if( *e )
	{
	    *e = '\0';
	    (*str) = e+1;
	    return p;
	}
    }
    return NULL;
}

SFUndoes* SFUndoFromString( char* str )
{
    enum sfundotype t =  sfut_fontinfo;
    char* staticmsg = "fixme";
    char* sfdfrag = str;

    if( !strncmp( str, "BeginFontLevelUndo", strlen("BeginFontLevelUndo")))
    {
	char* p;
	if( (p = findterm( &str, "FontLevelUndoType:" )) )
	    t = atoi(p);
	if( (p = findterm( &str, "FontLevelUndoMessage:" )) )
	    staticmsg = p;
    }

    SFUndoes* ret = SFUndoCreateSFD( t, staticmsg, sfdfrag );
    return ret;
}

SFUndoes* SFUndoCreateRedo( SFUndoes* undo, SplineFont* sf )
{
SFUndoes* ret = 0;
char* sfdfrag = 0;

switch(undo->type) {
case sfut_fontinfo:

sfdfrag  = DumpSplineFontMetadata( sf );
ret = SFUndoCreateSFD( sfut_fontinfo,
			   _("Font Information Dialog"),
			   sfdfrag );

break;
}

return ret;
}

void SFUndoPerform( SFUndoes* undo, SplineFont* sf )
{
    char* sfdchunk = 0;
    FILE* sfd = 0;

    switch(undo->type) {
    case sfut_fontinfo:
	sfdchunk = undo->sfdchunk;
//	printf("font level undo, font info sfd:%s\n", sfdchunk );
	sfd = MakeTemporaryFile();
	fwrite( sfdchunk, strlen(sfdchunk), 1, sfd );
	fseek( sfd, 0, SEEK_SET );

	SFD_GetFontMetaDataData d;
	SFD_GetFontMetaDataData_Init( &d );
	visitSFDFragment( sfd, sf, SFD_GetFontMetaDataVoid, &d );
	break;
    case sfut_lookups_kerns:
    case sfut_lookups:
	sfdchunk = undo->sfdchunk;
	if( !sfdchunk ) {
	    ff_post_error(_("Undo information incomplete"),_("There is a splinefont level undo, but it does not contain any information to perform the undo. This is an application error, please report what you last did to the lookup tables so the developers can try to reproduce the issue and fix it."));
	    SFUndoRemoveAndFree( sf, undo );
	    return;
	}

	// Roll it on back!
	sfd = MakeTemporaryFile();
	fwrite( sfdchunk, strlen(sfdchunk), 1, sfd );
	fseek( sfd, 0, SEEK_SET );

	while( 1 ) {
	    char* name = SFDMoveToNextStartChar( sfd );
	    if( !name )
		break;

	    int unienc = 0;
	    SplineChar *sc;
	    sc = SFGetChar( sf, unienc, name );
	    if( !sc ) {
		ff_post_error(_("Bad undo"),_("couldn't find the character %s"), name );
		break;
	    }
	    if( sc ) {
		// Free the stuff in sc->psts so we don't leak it.
		if( undo->type == sfut_lookups ) {
		    PSTFree(sc->possub);
		    sc->possub = 0;
		}
		char tok[2000];
		getname( sfd, tok );
		SFDGetPSTs( sfd, sc, tok );
		SFDGetKerns( sfd, sc, tok );
	    }
	    free(name);
	}

	if( undo->type == sfut_lookups_kerns ) {
	    SFDFixupRefs( sf );
	}
	break;
    default:
        break;
    }
}

void SFUndoPushFront( struct sfundoes ** undoes, SFUndoes* undo )
{
    dlist_pushfront( (struct dlistnode **)undoes, (struct dlistnode *)undo );
}
