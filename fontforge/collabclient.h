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

#ifndef _ALREADY_INCLUDED_FF_COLLAB_CLIENT_H
#define _ALREADY_INCLUDED_FF_COLLAB_CLIENT_H

#include "baseviews.h"
#include "views.h"

extern int pref_collab_sessionJoinTimeoutMS;
extern int pref_collab_roundTripTimerMS;


/**
 * Sometimes code *might* have created a new undo in the process of a
 * complex set of tests and nested calls. To help that code, the
 * CVAddUndo() calls this function to inform the collab code that a
 * new undo has infact been created.
 * 
 * So the client code can just call sendRedo() and if there was no new
 * undo created then nothing is sent to the collab session. On the
 * other hand if an undo was created, then it will have called here
 * during the process of being added to the undo stack, so the
 * sendRedo() will send this new undo to ther server.
 *
 * A classic case for this chain of events is clicking on the charview
 * canvas. If there was nothing selected and the user clicks on the
 * background then there will be no new undo created. If the user
 * picks a new node then there *will* be a new undo created. Either
 * way, the client code can safely just call sendRedo() and if there
 * was a new undo created it will be sent, if not then nothing will be
 * sent.
 */
extern void collabclient_CVPreserveStateCalled( CharViewBase *cv );
extern void collabclient_SCPreserveStateCalled( SplineChar *sc );


/**
 * Some parts of the system like charview's "undo" menu item might need
 * to override this setting so that UI redraws happen by default.
 */
extern void collabclient_setGeneratingUndoForWire( int v );


/**
 * If an undo takes place, it needs to know if it should repaint
 * the window to reflect the changes. If collabclient_generatingUndoForWire
 * returns true, then we don't really want to repaint just yet.
 *
 * Consider the process of events
 *
 * (a) User performs moving the top node in A left a bit
 * (b) the collab code runs "undo" so it can get the redo information
 * (c) the undo repaints the window
 * (d) the redo info is sent to the server
 * (e) delay
 * (f) server publishes this redo to all clients (including us)
 * (g) we "redo" the information from the server
 * (h) the redo repaints the window
 *
 * We would give a better user experience if (c) doesn't happen. So
 * the user doesn't see the older state appear and cancelled out again
 * moments later.
 *
 * So an undo should check this function and
 * if( collabclient_generatingUndoForWire() )
 * {
 *    dont repaint UI
 * }
 */
extern int collabclient_generatingUndoForWire( CharViewBase *cv );



/**
 * Return >0 if this charview is part of a collab session
 */
extern int collabclient_inSession(   CharViewBase *cv );
extern int collabclient_inSessionFV( FontViewBase* fv );


/**
 * Get the state as a enum or string. This can tell the user if we
 * have never connected, are running a local server, or are simply a
 * client. Also we can let them know if we were once connected but are
 * now disconnected.
 */
extern enum collabState_t collabclient_getState( FontViewBase* fv );
extern char*         collabclient_stateToString( enum collabState_t s );


#endif

