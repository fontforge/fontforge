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

    Based on example code in the zguide:
    
Copyright (c) 2010-2011 iMatix Corporation and Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    
*******************************************************************************
*******************************************************************************
******************************************************************************/

#include "fontforge/collabclientpriv.h"
#include "fontforge-internal-collab-server.h"
#include "collab/zmq_kvmsg.h"
#include "progname.h"

#define SUBTREE "/client/"

zbeacon_t *service_beacon = 0;

/**
 * The server is modeled on "Clone server Model Five" from the zguide.
 * https://github.com/imatix/zguide
 *
 * The Model numbers build on each other and the below is a breif overview
 * of the chain:
 * 
 * 5 uses a reactor for handling events
 * 4 has subtrees and allows clients to get at each tree separately
 * 3 takes updates from clients and sends them out to all through
 *   the publisher
 * 2 allows an out of band snapshot to be obtained from the server
 *   by a client
 * 1 is the initial desgin but requires all clients to be started
 *   before the server
 *
 * The next model up uses two servers to allow one to crash without
 * effecting the session. As the server is meant to be a very simple
 * process that maintains the hash of state and talks to clients I am
 * starting with only one server process to make things much simpler.
 *
 * Any complex computation can be done by the server spawning a child
 * process which knows where the server is at and what version or
 * perform it's action on. For example, to collapse the tree of a
 * collab session back so that everything less than sequence number
 * "X" is collapsed into an SFD with number X+1, that slot can be
 * reserved by the server and a subprocess created to collapse the
 * data and push back the new SFD to the server. The complex collapse
 * process is thus handled out of process which insulates this single
 * server from crashing due to issues in processing.
 *
 * Having the server itself out as a separate process allows the local
 * FontForge "server" process itself to crash without breaking the
 * collaboration session. The local fontforge can be restarted and
 * attach to the local server to get the whole state back.
 */

#define DEBUG zclock_log

/**
 * Singleton object for the server.
 */
typedef struct {
    zctx_t *ctx;         //  Main zeromq Context
    zloop_t *loop;       //  main zloop reactor
    int port;            //  Base port we're working on, we use
                         //  base+n for all other sockets.
    zhash_t *kvmap;      //  Key-value store that we manage
    int64_t sequence;    //  How many updates we're at


    // A ROUTER that gets snapshot requests from the client and
    // replies with the current state of kvmap.
    void *snapshot;

    // A PUB that sends changes in kvmap out to all the clients
    void *publisher;

    // A PULL that gets messages from any client with their desired
    // update for kvmap. We then send those updates out on the publisher
    // socket.
    void *collector;

    // A REP socket to reply to client pings 
    void* ping;
    
} clonesrv_t;


/**
 *  Send Snapshots
 *  
 *  Handles ICANHAZ? requests by sending snapshot data to the
 *  client that requested it:
 *
 *  Routing information for a key-value snapshot
 */
typedef struct {
    void *socket;           //  ROUTER socket to send to
    zframe_t *identity;     //  Identity of peer who requested state
    char *subtree;          //  Client subtree specification
} kvroute_t;

/**
 * We call this function for each key-value pair in our hash table
 */
static int
s_send_single (const char *key, void *data, void *args)
{
    kvroute_t *kvroute = (kvroute_t *) args;
    kvmsg_t *kvmsg = (kvmsg_t *) data;
    DEBUG ("I: s_send_single %"PRId64" type:%s", kvmsg_sequence(kvmsg), kvmsg_get_prop (kvmsg, "type") );

    if (strlen (kvroute->subtree) <= strlen (kvmsg_key (kvmsg))
    &&  memcmp (kvroute->subtree,
                kvmsg_key (kvmsg), strlen (kvroute->subtree)) == 0) {
        zframe_send (&kvroute->identity,    //  Choose recipient
            kvroute->socket, ZFRAME_MORE + ZFRAME_REUSE);
        kvmsg_send (kvmsg, kvroute->socket);
    }
    
    return 0;
}

/**
 * Snapshot Handler
 *  This is the reactor handler for the snapshot socket; it accepts
 *  just the ICANHAZ? request and replies with a state snapshot ending
 *  with a KTHXBAI message:
 */
static int
s_snapshots (zloop_t *loop, zmq_pollitem_t *poller, void *args)
{
    clonesrv_t *self = (clonesrv_t *) args;

    zframe_t *identity = zframe_recv (poller->socket);
    if (identity) {
        //  Request is in second frame of message
        char *request = zstr_recv (poller->socket);
        char *subtree = NULL;
        if (streq (request, "ICANHAZ?")) {
            free (request);
            subtree = zstr_recv (poller->socket);
        }
        else
            printf ("E: bad request, aborting\n");

        if (subtree) {
            //  Send state socket to client
            kvroute_t routing = { poller->socket, identity, subtree };
	    DEBUG("I: hash size:%ld", zhash_size(self->kvmap));

            zhash_foreach (self->kvmap, s_send_single, &routing);

            //  Now send END message with sequence number
            DEBUG ("I: sending shapshot=%d", (int) self->sequence);
            zframe_send (&identity, poller->socket, ZFRAME_MORE);
            kvmsg_t *kvmsg = kvmsg_new (self->sequence);
            kvmsg_set_key  (kvmsg, "KTHXBAI");
            kvmsg_set_body (kvmsg, (byte *) subtree, 0);
            kvmsg_send     (kvmsg, poller->socket);
            kvmsg_destroy (&kvmsg);
            free (subtree);
        }
        zframe_destroy(&identity);
    }
    return 0;
}

/**
 *  COllect Updates
 *  
 *  We store each update with a new sequence number, and if necessary, a
 *  time-to-live. We publish updates immediately on our publisher socket:
 */
static int
s_collector (zloop_t *loop, zmq_pollitem_t *poller, void *args)
{
    clonesrv_t *self = (clonesrv_t *) args;

    DEBUG("I: s_collector");
    
    kvmsg_t *kvmsg = kvmsg_recv (poller->socket);
    if (kvmsg) {
        kvmsg_set_sequence (kvmsg, ++self->sequence);
	kvmsg_fmt_key(kvmsg, "%s%d", SUBTREE, self->sequence-1 );

	if( !strcmp(MSG_TYPE_SFD, kvmsg_get_prop (kvmsg, "type")))
	{
	    // setup the beacon
	    //  Broadcast on the zyre port
	    beacon_announce_t ba;
	    memset( &ba, 0, sizeof(ba));
	    strcpy( ba.protocol, "fontforge-collab" );
	    ba.version = 2;
	    char* uuid = kvmsg_get_prop (kvmsg, "collab_uuid" );
	    if( uuid ) {
		strcpy( ba.uuid, uuid );
	    } else {
		ff_uuid_generate( ba.uuid );
	    }
	    strncpy( ba.username,    GetAuthor(), beacon_announce_username_sz );
	    ba.username[beacon_announce_username_sz-1] = '\0';
	    ff_gethostname( ba.machinename, beacon_announce_machinename_sz );
	    ba.port = htons( self->port );
	    strcpy( ba.fontname, "" );

	    DEBUG("I: adding beacon, payloadsz:%zu user:%s machine:%s",
		  sizeof(beacon_announce_t), ba.username, ba.machinename );

	    
	    char* fontname = kvmsg_get_prop (kvmsg, "fontname" );
	    if( fontname )
	    {
		strcpy( ba.fontname, fontname );
	    }


	    service_beacon = zbeacon_new( self->ctx, 5670 );
	    zbeacon_set_interval (service_beacon, 300 );
	    zbeacon_publish (service_beacon, (byte*)&ba, sizeof(ba));
	}
	
	
        kvmsg_send (kvmsg, self->publisher);
//        int ttl = atoi (kvmsg_get_prop (kvmsg, "ttl"));
//        if (ttl)
//            kvmsg_set_prop (kvmsg, "ttl",
//                "%" PRId64, zclock_time () + ttl * 1000);
        DEBUG ("I: publishing update=%d type:%s", (int) self->sequence,kvmsg_get_prop (kvmsg, "type"));
	DEBUG("I:x hash size:%ld", zhash_size(self->kvmap));
	
        kvmsg_store( &kvmsg, self->kvmap );
	
    }
    return 0;
}

/**
 * Reply to a ping request from FontForge on localhost
 */
static int
s_ping (zloop_t *loop, zmq_pollitem_t *poller, void *args)
{
    clonesrv_t *self = (clonesrv_t *) args;
    char* p = zstr_recv_nowait( self->ping );
    if( p && !strcmp(p,"quit"))
    {
	if( service_beacon )
	    zbeacon_destroy( &service_beacon );
	exit(0);
    }
    zstr_send( self->ping, "pong" );
    return 0;
}

//  .split flush ephemeral values
//  At regular intervals we flush ephemeral values that have expired. This
//  could be slow on very large data sets:

//  If key-value pair has expired, delete it and publish the
//  fact to listening clients.
/* static int */
/* s_flush_single (const char *key, void *data, void *args) */
/* { */
/*     clonesrv_t *self = (clonesrv_t *) args; */

/*     kvmsg_t *kvmsg = (kvmsg_t *) data; */
/*     int64_t ttl; */
/*     sscanf (kvmsg_get_prop (kvmsg, "ttl"), "%" PRId64, &ttl); */
/*     if (ttl && zclock_time () >= ttl) { */
/*         kvmsg_set_sequence (kvmsg, ++self->sequence); */
/*         kvmsg_set_body (kvmsg, (byte *) "", 0); */
/*         kvmsg_send (kvmsg, self->publisher); */
/*         kvmsg_store (&kvmsg, self->kvmap); */
/*         DEBUG ("I: publishing delete=%d", (int) self->sequence); */
/*     } */
/*     return 0; */
/* } */

static int
s_flush_ttl (zloop_t *loop, int unused, void *args)
{
//    clonesrv_t *self = (clonesrv_t *) args;
//    if (self->kvmap)
//        zhash_foreach (self->kvmap, s_flush_single, args);
    return 0;
}


int main (int argc, char** argv)
{
    int port = 5556;

#ifdef FF_USE_LIBGC
    GC_INIT();
    set_program_name (argv[0]);
#endif

    if( argc >= 2 )
	port = atoi( argv[1] );
	
    clonesrv_t *self = (clonesrv_t *) zmalloc (sizeof (clonesrv_t));
    self->port = port;
    self->ctx = zctx_new ();
    self->kvmap = zhash_new ();
    self->loop = zloop_new ();
    zloop_set_verbose (self->loop, false);

    //  Set up our clone server sockets
    self->snapshot  = zsocket_new (self->ctx, ZMQ_ROUTER);
    zsocket_bind (self->snapshot,  "tcp://*:%d", self->port + socket_srv_offset_snapshot );
    self->publisher = zsocket_new (self->ctx, ZMQ_PUB);
    zsocket_bind (self->publisher, "tcp://*:%d", self->port + socket_srv_offset_publisher );
    self->collector = zsocket_new (self->ctx, ZMQ_PULL);
    zsocket_bind (self->collector, "tcp://*:%d", self->port + socket_srv_offset_collector );
    self->ping = zsocket_new (self->ctx, ZMQ_REP);
    zsocket_bind (self->ping,      "tcp://*:%d", self->port + socket_srv_offset_ping );

    //  Register our handlers with reactor
    zmq_pollitem_t poller = { 0, 0, ZMQ_POLLIN };
    poller.socket = self->snapshot;
    zloop_poller (self->loop, &poller, s_snapshots, self);
    poller.socket = self->collector;
    zloop_poller (self->loop, &poller, s_collector, self);
    zloop_timer (self->loop, 1000, 0, s_flush_ttl, self);
    poller.socket = self->ping;
    zloop_poller (self->loop, &poller, s_ping, self);

    
    
    DEBUG ("I: server up and running on port:%d...", port );
    //  Run reactor until process interrupted
    zloop_start (self->loop);

    zloop_destroy (&self->loop);
    zhash_destroy (&self->kvmap);
    zctx_destroy (&self->ctx);
    free (self);
    return 0;
}
