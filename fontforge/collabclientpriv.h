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
 * should share, but which are not really intended to be public. This
 * file should only be included from the aforementioned files.
 *
 * At the moment this file must be included within a BUILD_COLLAB guard.
 *
 */
#ifndef _ALREADY_INCLUDED_FF_COLLAB_CLIENT_PRIV_H
#define _ALREADY_INCLUDED_FF_COLLAB_CLIENT_PRIV_H


#define MAGIC_VALUE 0xbeef
#define SUBTREE "/client/"

#define DEBUG zclock_log

#define MSG_TYPE_SFD  "msg_type_sfd"
#define MSG_TYPE_UNDO "msg_type_undo"
#define MSG_TYPE_UNDO_FONTLEVEL "msg_type_undo_fontlevel"

//
// Set to true if you want to see the raw SFD undo fragments which
// are moving to/from the server.
//
extern int DEBUG_SHOW_SFD_CHUNKS; // defined in collabclient.c


// See beacon_announce_t for important detail.
#define beacon_announce_protocol_sz     20
#define beacon_announce_uuid_sz         33
#define beacon_announce_username_sz     50
#define beacon_announce_machinename_sz  50
#define beacon_announce_ip_sz           20
#define beacon_announce_fontname_sz     40


#include "ffglib.h"
#include "inc/fontforge-config.h"
#include "inc/ustring.h"
#include "collabclient.h"
#include "inc/gfile.h"
#include "views.h"
#include "inc/gwidget.h"
#include "inc/gnetwork.h"
#include "splinefont.h"


/**
 * Note that these sizes form an explicit binary contract. If you
 * change the length of any of these then you have made a new beacon
 * version and will need to update the sending end of the beacon in
 * fontforge-internal-collab-server.c to a new version and the
 * receiving end of the beacon to handle that new version in addition
 * to all the old versions that the FontForge collab session should
 * still handle.
 *
 * Using a binary structure is a trade off; beacons are meant to be
 * small and simple and used to discover the server so only things
 * like the IP address, user, font etc are interesting because they
 * let a user easily select a session. Note that you can more easily
 * add to the end of beacon_announce_t because you are then backwards
 * compatible in that you are only sending more information than an
 * old client expects, and thus old clients will happily work with
 * newer servers, sans code that takes advantage of the new field.
 */
typedef struct {
    uint8_t protocol   [beacon_announce_protocol_sz];
    uint8_t version;
    uint8_t uuid       [beacon_announce_uuid_sz];
    uint8_t username   [beacon_announce_username_sz];
    uint8_t machinename[beacon_announce_machinename_sz];
    uint16_t port; // network byte order //

    // The following dont have any value to sending, they
    // are only used in the local hash version of this data structure.
    time_t last_msg_from_peer_time;
    uint8_t ip[beacon_announce_ip_sz];
    uint8_t fontname[beacon_announce_fontname_sz];
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



#ifdef BUILD_COLLAB

#include "collab/zmq_kvmsg.h"
#include "czmq.h"
#include <zbeacon.h>

typedef struct {

    int magic_number;    //  Magic number to test if the pointer is likely valid

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

    // When we send off an SFD then we record the uuid of that file here
    // so that we can know if we get a beacon back from the server with
    // the same info or not.
    char   unacknowledged_beacon_uuid[ FF_UUID_STRING_SIZE ];
    time_t unacknowledged_beacon_sendTime;

    // If the session is closing then we return false for 'inSession'
    int sessionIsClosing;

} cloneclient_t;

#else

/*
 * have this type around to allow functions to exist which will only perform
 * work when HAVE_COLLAB is defined. ie, this is a stub struct so that cloneclient_t
 * can be used in function arguments.
 */
typedef struct {
    int magic_number;    //  Magic number to test if the pointer is likely valid
    
    int WARNING_i_am_a_stub_do_not_delete_me;
    
} cloneclient_t;


#endif // build_collab
#endif // already_included_ff_collab_client_priv_h
