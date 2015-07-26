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

#ifndef _ALREADY_INCLUDED_FF_COLLAB_CLIENT_UI_H
#define _ALREADY_INCLUDED_FF_COLLAB_CLIENT_UI_H

#include "collabclient.h"


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
extern void collabclient_sendRedo_SC( SplineChar *sc, int layer );
extern void collabclient_sendRedo( CharViewBase *cv );
extern void collabclient_sendFontLevelRedo( SplineFont* sf );
extern void collabclient_performLocalUndo( CharViewBase *cv );
extern void collabclient_performLocalRedo( CharViewBase *cv );


extern int collabclient_reallyPerformUndo( CharViewBase *cv );



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
extern void collabclient_closeLocalServer( int port );

/**
 * We don't care about the server really, so it might as well die right now
 * Rather than having it floating around and possibly having multiple servers
 */
extern void collabclient_closeAllLocalServersForce( void );



/**
 * Every message send from the server has a monotonically increasing
 * SequenceNumber. This function gets what that value is right now and
 * can be used to check if any messages have been received from the
 * server by calling again and comparing the return value.
 */
extern int64_t collabclient_getCurrentSequenceNumber(void* ccvp);

extern void collabclient_ensureClientBeacon(void);

extern int collabclient_getBasePort(void* ccvp);
extern void collabclient_trimOldBeaconInformation( int secondsCutOff );
extern int collabclient_isAddressLocal( char* address );


extern char* Collab_getLastChangedName( void );
extern int Collab_getLastChangedPos( void );
extern int Collab_getLastChangedCodePoint( void );


/**
 * See https://developer.gnome.org/gobject/stable/signal.html
 * for implied parameters in GLib callback functions.
 */
typedef void (*collabclient_notification_cb)( gpointer instance, FontViewBase* fv, gpointer user_data );
extern void collabclient_addSessionJoiningCallback( collabclient_notification_cb func );
extern void collabclient_addSessionLeavingCallback( collabclient_notification_cb func );


#endif
