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

#include "ffglib.h"
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
#include <zbeacon.h>
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



#ifdef BUILD_COLLAB

#define beacon_announce_protocol_sz     20
#define beacon_announce_uuid_sz         40
#define beacon_announce_username_sz     50
#define beacon_announce_machinename_sz  50 
#define beacon_announce_ip_sz           20 

typedef struct {
    byte protocol   [beacon_announce_protocol_sz];
    byte version;
    byte uuid       [beacon_announce_uuid_sz];
    byte username   [beacon_announce_username_sz];
    byte machinename[beacon_announce_machinename_sz];
    uint16_t port; // network byte order //

    // The following dont have any value to sending, they
    // are only used in the local hash version of this data structure.
    time_t last_msg_from_peer_time;
    byte ip[beacon_announce_ip_sz];
} beacon_announce_t;

/**
 * After collabclient_ensureClientBeacon() is called, a list of
 * servers is collected over time. This function gives full access to
 * that list and should be treated with caution, its ok to use
 * internally in fontforge but it uses the above struct which might
 * change over time, so its not a client API.
 *
 * The return is a hash of beacon_announce_t instances.
 *
 * Do NOT free the return value or any of it's contents, none of it is
 * yours.
 */
extern GHashTable* collabclient_getServersFromBeaconInfomration( void );

#endif


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
