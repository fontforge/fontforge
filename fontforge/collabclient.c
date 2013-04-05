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

#define GTimer GTimer_GTK
#include <glib.h>
#undef GTimer

#include "inc/ustring.h"
#include "collabclient.h"
#include "inc/gfile.h"
#include "views.h"
#include "inc/gwidget.h"




#ifndef _NO_LIBZMQ
#define BUILD_COLLAB
#endif

#ifdef BUILD_COLLAB
#include "collab/zmq_kvmsg.h"
#include "czmq.h"
#endif


#define MAGIC_VALUE 0xbeef
#define SUBTREE "/client/"

#define DEBUG zclock_log

#define MSG_TYPE_SFD  "msg_type_sfd"
#define MSG_TYPE_UNDO "msg_type_undo"


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


int  collabclient_setHaveLocalServer( int v );

#ifdef BUILD_COLLAB

static zctx_t* obtainMainZMQContext()
{
    static zctx_t* ctx = 0;
    if( !ctx )
    {
	ctx = zctx_new ();
    }
    return ctx;
}

/**
 * Convert address + port to a string like
 * tcp://localhost:5556
 * 
 * you do not own the return value, do not free it. 
 */
static char* makeAddressString( char* address, int port )
{
    static char ret[PATH_MAX+1];
    snprintf(ret,PATH_MAX,"tcp://%s:%d",address,port);
    return ret;
}

/**
 * Process the given kvmsg from the server. If create is set and we do
 * not have any charview for a changed glyph then we first create a
 * charview for it. This allows the updates from a server to be
 * processed at startup time, getting us up to speed with any glyphs
 * that have changed.
 *
 * This function is mainly called in response to an update which is
 * published from the server. However, in sessionJoin() we also call
 * here to handle the incremental updates to glyphs that have occurred
 * after the SFD was sent to the server.
 */
static void zeromq_subscriber_process_update( cloneclient_t* cc, kvmsg_t *kvmsg, int create )
{
    cc->sequence = kvmsg_sequence (kvmsg);
    if( cc->sequence >= cc->roundTripTimerWaitingSeq )
	cc->roundTripTimerWaitingSeq = 0;
		    

    char* uuid = kvmsg_get_prop (kvmsg, "uuid" );
    byte* data = kvmsg_body (kvmsg);
    size_t data_size = kvmsg_size (kvmsg);

    printf("uuid:%s\n", uuid );
    FontView* fv = FontViewFind( FontViewFind_byXUIDConnected, uuid );
    printf("fv:%p\n", fv );
    if( fv )
    {
	if( !data_size )
	{
	    printf("WARNING: zero length message!\n" );
	    return;
	}
			
	SplineFont *sf = fv->b.sf;
	if( !sf )
	{
	    printf("ERROR: font view does not have the splinefont set!\n" );
	    return;
	}
	
	char* pos = kvmsg_get_prop (kvmsg, "pos" );
	printf("pos:%s\n", pos );
	SplineChar *sc = sf->glyphs[ atoi(pos) ];
	printf("sc:%p\n", sc );
	if( !sc )
	{
	    printf("WARNING: font view does not have a glyph for pos:%s\n", pos );
	    return;
	}
	
	printf("sc.name:%s\n", sc->name );
	printf("data.size:%ld\n", data_size );
		    
	int current_layer = 0;

	if( !sc->views && create )
	{
	    CharView* cv = CharViewCreate( sc, fv, -1 );
	    printf("created charview:%p\n", cv );
	}
	
	for( CharViewBase* cv = sc->views; cv; cv = cv->next )
	{
	    printf("have charview:%p\n", cv );
	    
	    char filename[PATH_MAX];
	    snprintf(filename, PATH_MAX, "/tmp/fontforge-collab-inx-%d.sfd", getpid() );
	    GFileWriteAll( filename, (char*)data);
	    FILE* file = fopen( filename, "r" );
	    Undoes* undo = SFDGetUndo( sf, file, sc,
				       "UndoOperation",
				       "EndUndoOperation",
				       current_layer );
	    fclose(file);
	    if( !undo )
	    {
		printf("***** ERROR ****** reading back undo instance!\n");
		printf("data: %s\n\n", data );
	    }
	    if( undo )
	    {
		undo->next = 0;
		cv->layerheads[cv->drawmode]->redoes = undo;
		CVDoRedo( cv );
	    }
			    
	    break;
	}
    }
		    
    printf ("I: processed update=%d\n", (int) cc->sequence);
}

/**
 * A callback function that should be invoked when there is input in
 * the zeromq fd zeromq_fd. The collabclient is taken as the user data
 * pointer.
 *
 * Add this callback using GDrawAddReadFD()
 */
static void zeromq_subscriber_fd_callback(int zeromq_fd, void* datas )
{
    cloneclient_t* cc = (cloneclient_t*)datas;
    
//    printf("zeromq_subscriber_fd_callback(1)\n");

    int opt = 0;
    size_t optsz = sizeof(int);
    zmq_getsockopt( cc->subscriber, ZMQ_EVENTS, &opt, &optsz );

    if( opt & ZMQ_POLLIN )
    {
	printf("zeromq_subscriber_fd_callback() have message! cc:%p\n",cc);

	while( 1 )
	{
	    kvmsg_t *kvmsg = kvmsg_recv_full (cc->subscriber, ZMQ_DONTWAIT);
	    if (kvmsg)
	    {
		int64_t msgseq = kvmsg_sequence (kvmsg);

		//  Discard out-of-sequence kvmsgs, incl. heartbeats
		if ( msgseq > cc->sequence)
		{
		    int create = 0;
		    zeromq_subscriber_process_update( cc, kvmsg, create );
		    kvmsg_store (&kvmsg, cc->kvmap);
		    printf ("I: received update=%d\n", (int) cc->sequence);
		}
		else
		{
		    kvmsg_destroy (&kvmsg);
		}
	    }
	    else
		break;
	}
    }
}


/**
 * A timeout function which is called after a given idle period to
 * alert the user if we have not received a reply from the server yet.
 *
 * If we don't get a reply in time then the user experience will
 * suffer greatly (UI elements jumping around etc) so we ask the user
 * if they want to start again or disconnect.
 */
static void collabclient_roundTripTimer( void* ccvp )
{
    cloneclient_t *cc = (cloneclient_t*)ccvp;

    printf("collabclient_roundTripTimer() cc: %p\n", cc );
    printf("collabclient_roundTripTimer() waitingseq: %d\n", cc->roundTripTimerWaitingSeq );

    if( cc->roundTripTimerWaitingSeq )
    {
	cc->roundTripTimerWaitingSeq = 0;
	
	char *buts[3];
	buts[0] = _("_Reconnect");
	buts[1] = _("_Disconnect");
	buts[2] = NULL;

	if ( gwwv_ask(_("Network Issue"),
		      (const char **) buts,0,1,
		      _("FontForge expected some input from the server by now.\nWould you like to try to reconnect to the collaboration session?"))==1 )
	{
	    // break session
	    FontView* fv = FontViewFind( FontViewFind_byCollabPtr, cc );
	    if( fv )
	    {
		printf("fv:%p\n", fv );
		fv->b.collabState = cs_disconnected;
		fv->b.collabClient = 0;
		FVTitleUpdate( &fv->b );
	    }
	    
	    collabclient_free( (void**)&cc );
	    return;
	}
	
	collabclient_sessionReconnect( cc );
    }
}
#endif

void collabclient_sessionDisconnect( FontViewBase* fv )
{
#ifdef BUILD_COLLAB
    
    cloneclient_t *cc = fv->collabClient;

    fv->collabState = cs_disconnected;
    fv->collabClient = 0;
    FVTitleUpdate( fv );
    
    collabclient_free( (void**)&cc );

#endif
}


#ifdef BUILD_COLLAB

static void
collabclient_remakeSockets( cloneclient_t *cc )
{
    cc->snapshot = zsocket_new (cc->ctx, ZMQ_DEALER);
    zsocket_connect (cc->snapshot,
		     makeAddressString( cc->address,
					cc->port + socket_offset_snapshot));
    
    cc->subscriber = zsocket_new (cc->ctx, ZMQ_SUB);
    zsockopt_set_subscribe (cc->subscriber, "");
    zsocket_connect (cc->subscriber,
		     makeAddressString( cc->address,
					cc->port + socket_offset_subscriber));
    zsockopt_set_subscribe (cc->subscriber, SUBTREE);

    cc->publisher = zsocket_new (cc->ctx, ZMQ_PUSH);
    zsocket_connect (cc->publisher,
		     makeAddressString( cc->address,
					cc->port + socket_offset_publisher));

    int fd = 0;
    size_t fdsz = sizeof(fd);
    int rc = zmq_getsockopt( cc->subscriber, ZMQ_FD, &fd, &fdsz );
    printf("rc:%d fd:%d\n", rc, fd );
    GDrawAddReadFD( 0, fd, cc, zeromq_subscriber_fd_callback );
    
}

#endif


void* collabclient_new( char* address, int port )
{
#ifdef BUILD_COLLAB
    
    cloneclient_t *cc = 0;
    cc = (cloneclient_t *) zmalloc (sizeof (cloneclient_t));
    cc->magic_number = MAGIC_VALUE;
    cc->ctx   = obtainMainZMQContext();
    cc->loop  = 0;
    cc->address = copy( address );
    cc->port  = port;
    cc->kvmap = zhash_new();
    cc->publisher_sendseq = 1;
    cc->sequence = 0;
    cc->preserveUndo = 0;
    cc->roundTripTimerWaitingSeq = 0;

    collabclient_remakeSockets( cc );

    int32 roundTripTimerMS = 2000;
    cc->roundTripTimer = BackgroundTimer_new( roundTripTimerMS, 
					      collabclient_roundTripTimer,
					      cc );
    
    return cc;

#endif

    return 0;
}

void collabclient_free( void** ccvp )
{
#ifdef BUILD_COLLAB
    
    cloneclient_t* cc = (cloneclient_t*)*ccvp;
    if( !cc )
	return;
    if( cc->magic_number != MAGIC_VALUE )
    {
	fprintf(stderr,"collabclient_free() called on an invalid client object!\n");
	return;
    }
    cc->magic_number = MAGIC_VALUE + 1;

    {
	int fd = 0;
	size_t fdsz = sizeof(fd);
	int rc = zmq_getsockopt( cc->subscriber, ZMQ_FD, &fd, &fdsz );
	GDrawRemoveReadFD( 0, fd, cc );
    }
    
    
    zsocket_destroy( cc->ctx, cc->snapshot   );
    zsocket_destroy( cc->ctx, cc->subscriber );
    zsocket_destroy( cc->ctx, cc->publisher  );
    BackgroundTimer_remove( cc->roundTripTimer );

    FontView* fv = FontViewFind( FontViewFind_byCollabPtr, cc );
    if( fv )
    {
	fv->b.collabClient = 0;
    }
        
    zhash_destroy (&cc->kvmap);
    free(cc->address);
    free(cc);
    *ccvp = 0;

#endif
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef BUILD_COLLAB

static void collabclient_sendSFD( void* ccvp, char* sfd )
{
    cloneclient_t* cc = (cloneclient_t*)ccvp;

    kvmsg_t *kvmsg = kvmsg_new(0);
    kvmsg_fmt_key  (kvmsg, "%s%d", SUBTREE, cc->publisher_sendseq++);
    kvmsg_set_body (kvmsg, sfd, strlen(sfd));
    kvmsg_set_prop (kvmsg, "type", MSG_TYPE_SFD );
//    kvmsg_set_prop (kvmsg, "ttl", "%d", randof (30));
    kvmsg_send     (kvmsg, cc->publisher);
    kvmsg_destroy (&kvmsg);
    DEBUG("Sent a SFD of %d bytes to the server\n",strlen(sfd));
    free(sfd);
}

static int collabclient_sessionJoin_processmsg_foreach_fn( kvmsg_t* msg, void *argument )
{
    cloneclient_t* cc = (cloneclient_t*)argument;
    printf("processmsg_foreach() seq:%ld\n", kvmsg_sequence (msg) );
    int create = 1;
    zeromq_subscriber_process_update( cc, msg, create );
    
    return 0;
}

static void
collabclient_sendRedo_Internal( CharViewBase *cv, Undoes *undo )
{
    printf("collabclient_sendRedo_Internal()\n");
    cloneclient_t* cc = cv->fv->collabClient;
    if( !cc )
	return;
    char* uuid = cv->fv->sf->xuid;
    printf("uuid:%s\n", uuid );
    
    int idx = 0;
    char filename[PATH_MAX];
    snprintf(filename, PATH_MAX, "/tmp/fontforge-collab-x.sfd");
    FILE* f = fopen( filename, "w" );
    SFDDumpUndo( f, cv->sc, undo, "Undo", idx );
    fclose(f);
    char* sfd = GFileReadAll( filename );
//    printf("SENDING: %s\n\n", sfd );

    cc->roundTripTimerWaitingSeq = cc->publisher_sendseq;
    BackgroundTimer_touch( cc->roundTripTimer );

    kvmsg_t *kvmsg = kvmsg_new(0);
    kvmsg_fmt_key  (kvmsg, "%s%d", SUBTREE, cc->publisher_sendseq++);
    kvmsg_set_body (kvmsg, sfd, strlen(sfd) );
    kvmsg_set_prop (kvmsg, "type", MSG_TYPE_UNDO );
    kvmsg_set_prop (kvmsg, "uuid", uuid );
    char pos[100];
    sprintf(pos, "%d", cv->sc->orig_pos );
    printf("pos:%s\n", pos );
    printf("sc.name:%s\n", cv->sc->name );
    
    size_t data_size = kvmsg_size (kvmsg);
    printf("data.size:%ld\n", data_size );

    kvmsg_set_prop (kvmsg, "pos", pos );
    kvmsg_send     (kvmsg, cc->publisher);
    kvmsg_destroy (&kvmsg);
    DEBUG("Sent a undo chunk of %d bytes to the server\n",strlen(sfd));
    free(sfd);
}

typedef struct collabclient_sniffForLocalServer_struct
{
    void* socket;
    BackgroundTimer_t* timer;
    int haveServer;
} collabclient_sniffForLocalServer_t;

collabclient_sniffForLocalServer_t collabclient_sniffForLocalServer_singleton;

static void collabclient_sniffForLocalServer_timer( void* udata )
{
    collabclient_sniffForLocalServer_t* cc = (collabclient_sniffForLocalServer_t*)udata;
//    printf("collabclient_sniffForLocalServer_timer()\n");

    char* p = zstr_recv_nowait( cc->socket );
    if( p )
    {
	printf("collabclient_sniffForLocalServer_timer() p:%s\n", p);
	if( !strcmp(p,"pong"))
	{
	    printf("******* have local server!\n");
	    cc->haveServer = 1;
	}
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void collabclient_sessionStart( void* ccvp, FontView *fv )
{
#ifdef BUILD_COLLAB

    cloneclient_t* cc = (cloneclient_t*)ccvp;

    //
    // Fire up the fontforge-internal-collab-server process...
    //
    {
	char command_line[PATH_MAX+1];
	sprintf(command_line,
		"%s/FontForgeInternal/fontforge-internal-collab-server",
		getGResourceProgramDir() );
	printf("command_line:%s\n", command_line );
	GError * error = 0;
	if(!getenv("FONTFORGE_USE_EXISTING_SERVER"))
	    g_spawn_command_line_async( command_line, &error );
    }
    
    printf("Starting a session, sending it the current SFD as a baseline...\n");
    int s2d = 0;
    char filename[PATH_MAX];
    snprintf(filename, PATH_MAX, "/tmp/fontforge-collab-start-%d.sfd", getpid());
    int ok = SFDWrite(filename,fv->b.sf,fv->b.map,fv->b.normal,s2d);
    printf("connecting to server...3 ok:%d\n",ok);
    if ( ok )
    {
	char* sfd = GFileReadAll( filename );
	printf("connecting to server...4 sfd:%p\n", sfd );
	collabclient_sendSFD( cc, sfd );
    }
    GFileUnlink(filename);
    printf("connecting to server...sent the sfd for session start.\n");
    fv->b.collabState = cs_server;
    FVTitleUpdate( &fv->b );

    collabclient_setHaveLocalServer( 1 );
    
#endif
}





void collabclient_sessionJoin( void* ccvp, FontView *fv )
{
#ifdef BUILD_COLLAB
    
    cloneclient_t* cc = (cloneclient_t*)ccvp;
    printf("collabclient_sessionJoin(top)\n");

    //  Get state snapshot
    cc->sequence = 0;
    zstr_sendm (cc->snapshot, "ICANHAZ?");
    zstr_send  (cc->snapshot, SUBTREE);
    kvmsg_t* lastSFD = 0;
    while (true) {
        kvmsg_t *kvmsg = kvmsg_recv (cc->snapshot);
        if (!kvmsg)
            break;          //  Interrupted
        if (streq (kvmsg_key (kvmsg), "KTHXBAI")) {
            cc->sequence = kvmsg_sequence (kvmsg);
            printf ("I: received snapshot=%d\n", (int) cc->sequence);
            kvmsg_destroy (&kvmsg);
            break;          //  Done
        }
	printf ("I: storing seq=%ld ", kvmsg_sequence (kvmsg));
	if( kvmsg_get_prop (kvmsg, "type") )
	    printf(" type:%s", kvmsg_get_prop (kvmsg, "type") );
	printf ("\n");
	if( !strcmp(kvmsg_get_prop (kvmsg, "type"), MSG_TYPE_SFD ))
	{
	    if( !lastSFD ||
		kvmsg_sequence (kvmsg) > kvmsg_sequence (lastSFD))
	    {
		lastSFD = kvmsg;
	    }
	    size_t data_size = kvmsg_size(lastSFD);
	    printf("data_size:%ld\n", data_size );
	}
	kvmsg_store (&kvmsg, cc->kvmap);
    }
    printf("collabclient_sessionJoin(done with netio getting snapshot)\n");
    printf("collabclient_sessionJoin() lastSFD:%p\n", lastSFD );

//    int rc = 0;

//    void* out = 0;
//    rc = zhash_foreach ( cc->kvmap, collabclient_sessionJoin_findSFD_foreach_fn, &out );
    
//    printf("collabclient_sessionJoin() last sfd:%p\n", out );

    if( !lastSFD )
    {
	gwwv_post_error(_("FontForge Collaboration"), _("No Font Snapshot received from the server"));
	return;
    }
    
    if( lastSFD )
    {
	int openflags = 0;
	char filename[PATH_MAX];
	snprintf(filename, PATH_MAX, "/tmp/fontforge-collab-import-%d.sfd",getpid());
	GFileWriteAll( filename, kvmsg_body (lastSFD) );

	/*
	 * Since we are creating a new fontview window for the collab session
	 * we "hand over" the collabClient from the current fontview used to join
	 * the session to the fontview which owns the sfd from the wire.
	 */ 
	FontViewBase* newfv = ViewPostScriptFont( filename, openflags );
	newfv->collabClient = cc;
	fv->b.collabClient = 0;
	newfv->collabState = cs_client;
	FVTitleUpdate( newfv );
	
	/* cloneclient_t* newc = collabclient_new( cc->address, cc->port ); */
	/* collabclient_sessionJoin( newc, fv ); */
    }

    
    printf("applying updates from server that were performed after the SFD snapshot was done...\n");
    
    kvmap_visit( cc->kvmap, kvmsg_sequence (lastSFD),
		 collabclient_sessionJoin_processmsg_foreach_fn, cc );

    /* struct collabclient_sessionJoin_processUpdates_foreach_args args; */
    /* args.cc = cc; */
    /* args.lastSFD = lastSFD; */
    /* rc = zhash_foreach ( cc->kvmap, collabclient_sessionJoin_processUpdates_foreach_fn, &args ); */
    
    

    printf("collabclient_sessionJoin(complete)\n");

#endif
}


void collabclient_sessionReconnect( void* ccvp )
{
#ifdef BUILD_COLLAB

    cloneclient_t* cc = (cloneclient_t*)ccvp;

    zsocket_destroy( cc->ctx, cc->snapshot   );
    zsocket_destroy( cc->ctx, cc->subscriber );
    zsocket_destroy( cc->ctx, cc->publisher  );
    collabclient_remakeSockets( cc );
    cc->publisher_sendseq = 1;

    FontView* fv = FontViewFind( FontViewFind_byCollabPtr, cc );
    if( fv )
    {
	collabclient_sessionJoin( cc, fv );
    }

#endif
}




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


void collabclient_sendRedo( CharViewBase *cv )
{
#ifdef BUILD_COLLAB
    
    cloneclient_t* cc = cv->fv->collabClient;
    if( !cc )
	return;

    printf("collabclient_sendRedo() cv:%p\n", cv );
    printf("collabclient_sendRedo() dm:%d\n", cv->drawmode );
    printf("collabclient_sendRedo() preserveUndo:%p\n", cc->preserveUndo );
    if( !cc->preserveUndo )
	return;
    
    CVDoUndo( cv );
    Undoes *undo = cv->layerheads[cv->drawmode]->redoes;
    printf("collabclient_sendRedo() undo:%p\n", undo );

    /* if( undo ) */
    /* 	collabclient_sendRedo_Internal( cv, undo ); */
    /* CVDoRedo( cv ); */

    if( undo )
    {
	cv->layerheads[cv->drawmode]->redoes = undo->next;
	collabclient_sendRedo_Internal( cv, undo );
	UndoesFree( undo );
    }
    cc->preserveUndo = 0;

#endif
}

int collabclient_inSession( CharViewBase *cv )
{
#ifdef BUILD_COLLAB

    if( !cv )
	return 0;
    if( !cv->fv )
	return 0;
    cloneclient_t* cc = cv->fv->collabClient;
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
    return cc!=0;

#endif
    return 0;
}

int collabclient_generatingUndoForWire( CharViewBase *cv )
{
#ifdef BUILD_COLLAB
    return collabclient_inSession( cv );
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

char*
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
    }
    return _("Unknown Collab State!");
}


int
collabclient_setHaveLocalServer( int v )
{
#ifdef BUILD_COLLAB

    collabclient_sniffForLocalServer_t* cc = &collabclient_sniffForLocalServer_singleton;
    int oldv = cc->haveServer;
    cc->haveServer = v;
    return oldv;

#else

    return 0;

#endif
}

int
collabclient_haveLocalServer( void )
{
#ifdef BUILD_COLLAB

    collabclient_sniffForLocalServer_t* cc = &collabclient_sniffForLocalServer_singleton;
    return cc->haveServer;
    
#else
    return 0;
#endif
}

void
collabclient_sniffForLocalServer( void )
{
#ifdef BUILD_COLLAB

    memset( &collabclient_sniffForLocalServer_singleton, 0,
	    sizeof(collabclient_sniffForLocalServer_t));
    collabclient_sniffForLocalServer_t* cc = &collabclient_sniffForLocalServer_singleton;

    zctx_t* ctx = obtainMainZMQContext();
    int port_default = 5556;

    cc->socket = zsocket_new ( ctx, ZMQ_REQ );
    zsocket_connect (cc->socket,
		     makeAddressString("localhost",
				       port_default + socket_offset_ping));
    cc->timer = BackgroundTimer_new( 1000, collabclient_sniffForLocalServer_timer, cc );
    zstr_send  (cc->socket, "ping");

#endif
}

void
collabclient_closeLocalServer( FontViewBase* fv )
{
#ifdef BUILD_COLLAB

    collabclient_sniffForLocalServer_t* cc = &collabclient_sniffForLocalServer_singleton;
    zctx_t* ctx = obtainMainZMQContext();
    int port_default = 5556;

    void* socket = zsocket_new ( ctx, ZMQ_REQ );
    zsocket_connect ( socket,
		      makeAddressString("localhost",
					port_default + socket_offset_ping));
    zstr_send( socket, "quit" );
    cc->haveServer = 0;

#endif
}



