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
extern void collabclient_sessionJoin( void* ccvp, FontView *fv );

/**
 * Something of interest has happened in fontforge which can be
 * undone. Send a "redo" event describing the local changes to the
 * server. Thus the server will publish these changes to all clients.
 */
extern void collabclient_sendRedo( CharViewBase *cv );

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
extern int collabclient_inSession( CharViewBase *cv );


#endif

