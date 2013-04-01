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
 * Create a new collab client. You can start using the client in two
 * major ways: collabclient_sessionStart() will send your current SFD
 * file on the wire to the server to bootstrap the session. If you
 * instead call collabclient_sessionJoin() then a new fontview is
 * created with the SFD from the server and any pending updates to
 * that SFD are downloaded and applied.
 *
 * Typically, the process that started the server will then call
 * sessionStart() to send the SFD over to that server. Other folks
 * who want to join the collab session will use Join() instead.
 * 
 * Both methods of starting the client will register for published
 * events from the server and apply them automatically to the fontview
 * (and its charviews etc). So the difference of how the session was
 * started becomes much less after the initial call to sessionStart or
 * sessionJoin.
 * 
 */
extern void* collabclient_new( char* address, int port );
/**
 * Like collabclient_new() but takes a single string like
 * hostname:port
 * instead of two explicit arguments.
 */
extern void* collabclient_newFromPackedAddress( char* packed );

/**
 * Close a collab client which is no longer needed. Free the resources
 * and zeromq sockets that are associated with the client.
 */
extern void  collabclient_free( void** ccvp );

/**
 * Start a session by sending the local font to the server as a baseline
 * SFD file. We don't care about the updates the server has, we are the
 * "master" who started the session.
 */
extern void collabclient_sessionStart( void* ccvp, FontView *fv );

/**
 * Create a new FontView with the SFD from the server and apply any collab
 * changes that other folks might have performed on that SFD since the session
 * was started.
 */
extern FontViewBase* collabclient_sessionJoin( void* ccvp, FontView *fv );

/**
 * Reconnect to the collab server. The sockets are all remade, and the
 * SFD and updates will be downloaded from the server to start a fresh
 * fontview.
 */
extern void collabclient_sessionReconnect( void* ccvp );

/**
 * Disconnect from the collab server. Sockets are cleaned up and the
 * state is set to disconnected to remind the user that they are no
 * longer collaborating.
 */
extern void collabclient_sessionDisconnect( FontViewBase* fv );

/**
 * Something of interest has happened in fontforge which can be
 * undone. Send a "redo" event describing the local changes to the
 * server. Thus the server will publish these changes to all clients.
 */
extern void collabclient_sendRedo( CharViewBase *cv );
extern void collabclient_performLocalUndo( CharViewBase *cv );

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

/**
 * Return >0 if this charview is part of a collab session
 */
extern int collabclient_inSession(   CharViewBase *cv );
extern int collabclient_inSessionFV( FontViewBase* fv );

extern int collabclient_reallyPerformUndo( CharViewBase *cv );


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
 * Get the state as a enum or string. This can tell the user if we
 * have never connected, are running a local server, or are simply a
 * client. Also we can let them know if we were once connected but are
 * now disconnected.
 */
extern enum collabState_t collabclient_getState( FontViewBase* fv );
extern char*         collabclient_stateToString( enum collabState_t s );

/**
 * Is there a local fontforge collab server process running on this
 * machine? This might take very short amount of time after startup to
 * be set properly as we ask the server process to reply to a ping.
 */
extern int  collabclient_haveLocalServer( void );

/**
 * At startup you should call this to sniff for a local fontforge
 * server process.
 */
extern void collabclient_sniffForLocalServer( void );

/**
 * Close the local server process. Code would likely want to
 * collabclient_sessionDisconnect() first and then call here to kill
 * off the server itself.
 *
 * It is a good idea to warn the user that this is happening because
 * the server process can normally outlive many fontforge clients and
 * if fontforge itself crashes (on the server too). However, closing
 * the server process will end it.
 */
extern void collabclient_closeLocalServer( FontViewBase* fv );


/**
 * Some parts of the system like charview's "undo" menu item might need
 * to override this setting so that UI redraws happen by default.
 */
extern void collabclient_setGeneratingUndoForWire( int v );


extern int64_t collabclient_getCurrentSequenceNumber(void* ccvp);

#endif

