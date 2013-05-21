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

/**
 *
 * These are definitions that both collabclient and collabclientui
 * should share, but which are not really intended to be public. This file
 * should only be included from the aforementioned files.
 *
 */
#ifndef _ALREADY_INCLUDED_FF_COLLAB_CLIENT_PRIV_H
#define _ALREADY_INCLUDED_FF_COLLAB_CLIENT_PRIV_H

#define GTimer GTimer_GTK
#include <glib.h>
#undef GTimer

#include "inc/fontforge-config.h"
#include "inc/ustring.h"
#include "collabclient.h"
#include "inc/gfile.h"
#include "views.h"
#include "inc/gwidget.h"
#include "inc/gnetwork.h"


#ifdef BUILD_COLLAB
#include "collab/zmq_kvmsg.h"
#include "czmq.h"
#endif

#define MAGIC_VALUE 0xbeef
#define SUBTREE "/client/"

#define DEBUG zclock_log

#define MSG_TYPE_SFD  "msg_type_sfd"
#define MSG_TYPE_UNDO "msg_type_undo"

//
// Set to true if you want to see the raw SFD undo fragments which
// are moving to/from the server.
//
#define DEBUG_SHOW_SFD_CHUNKS 0



typedef struct {

    int magic_number;    //  Magic number to test if the pointer is likely valid

#ifdef BUILD_COLLAB
    zctx_t *ctx;         //  Main zeromq Context
    zloop_t *loop;       //  main zloop reactor
    char* address;       //  address of the server
    int port;            //  Base port we're working on, we use
                         //  base+n for all other sockets.
    zhash_t *kvmap;      //  Key-value store that we manage
    int64_t sequence;    //  How many updates we're at

    Undoes* preserveUndo; //< Used by collabclient_sendRedo to only send an undo if one has
                          // been made

    /*
     * The roundTripTimer fires X milliseconds after we send a redo to
     * the server. The roundTripTimerWaitingSeq is a cache of the
     * sequence that we sent to the server. If we don't get a reply to
     * the update that we sent on subscriber within the timeout then
     * we assume there is a network problem and alert the user.
     */ 
    BackgroundTimer_t* roundTripTimer; 
    int                roundTripTimerWaitingSeq;         
    
    //
    // A DEALER that we receive snapshot requests from the server on
    void *snapshot;

    // A SUB that we listen to the server telling us about each new
    // update from other clients
    void *subscriber;

    // We send our updates to the server (and thus to all other
    // client's subscriber sockets) on this socket.
    void *publisher;

    // Monotonically increasing number used to tag outgoing msgs
    // to the publisher
    int publisher_sendseq;         

#endif    
} cloneclient_t;



#endif
