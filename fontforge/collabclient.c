/******************************************************************************
*******************************************************************************
*******************************************************************************

    Copyright (C) 2013 Ben Martin
    
    This file is part of FontForge.

    FontForge is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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

int pref_collab_sessionJoinTimeoutMS = 1000;
int pref_collab_roundTripTimerMS = 2000;
char* pref_collab_last_server_connected_to = 0;

#include "collabclientpriv.h"
#include "inc/basics.h"

int DEBUG_SHOW_SFD_CHUNKS = 0;


void collabclient_CVPreserveStateCalled( CharViewBase *cv )
{
#ifdef BUILD_COLLAB

    cloneclient_t* cc = cv->fv->collabClient;
    if( !cc )
	return;

    Undoes *undo = cv->layerheads[cv->drawmode]->undoes;
    cc->preserveUndo = undo;

#endif
}

static int generatingUndoForWire = 1;

void collabclient_setGeneratingUndoForWire( int v )
{
    generatingUndoForWire = v;
}

int collabclient_generatingUndoForWire( CharViewBase *cv )
{
#ifdef BUILD_COLLAB
    if( collabclient_inSession( cv ) )
    {
	return generatingUndoForWire;
    }
#endif
    return 0;
}

int collabclient_inSession( CharViewBase *cv )
{
#ifdef BUILD_COLLAB

    if( !cv )
	return 0;
    if( !cv->fv )
	return 0;
    cloneclient_t* cc = cv->fv->collabClient;
    if( cc && cc->sessionIsClosing )
	return 0;
    return cc!=0;

#endif
    return 0;
}

int collabclient_inSessionFV( FontViewBase* fv )
{
#ifdef BUILD_COLLAB
    
    if( !fv )
	return 0;
    cloneclient_t* cc = fv->collabClient;
    if( cc && cc->sessionIsClosing )
	return 0;
    return cc!=0;

#endif
    return 0;
}



enum collabState_t
collabclient_getState( FontViewBase* fv )
{
#ifdef BUILD_COLLAB

    if( !fv )
	return cs_neverConnected;
    return fv->collabState;

#else

    return cs_neverConnected;

#endif
}

const char*
collabclient_stateToString( enum collabState_t s )
{
    switch( s )
    {
    case cs_neverConnected:
	return "";
    case cs_disconnected:
	return _("Disconnected");
    case cs_server:
	return _("Collab Server");
    case cs_client:
	return _("Collab Client");
    default:
        return _("Unknown Collab State!");
    }
}



void collabclient_SCPreserveStateCalled( SplineChar *sc )
{
#ifdef BUILD_COLLAB

    if( !sc->parent || !sc->parent->fv )
	return;
    
    FontViewBase* fv = sc->parent->fv;
    cloneclient_t* cc = fv->collabClient;
    if( !cc )
	return;

    Undoes *undo = sc->layers[ly_fore].undoes;
    cc->preserveUndo = undo;

#endif
}






