/******************************************************************************
*******************************************************************************
*******************************************************************************

    Copyright (C) 2013 Ben Martin
    
    This file is part of FontForge.

    FontForge is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation,either version 3 of the License, or
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

#include "collabclientpriv.h"
#include "collabclientui.h"
#include "uiinterface.h"
#include "sfundo.h"
#include "../fontforge/ffglib.h"
#include "fontforgeexe.h"

int  collabclient_setHaveLocalServer( int v );

char lastChangedName[1024];
int lastChangedPos;
char lastChangedEncoding[1024];
int lastChangedCodePoint;

char* Collab_getLastChangedName( void )
{
    return lastChangedName;
}

int Collab_getLastChangedPos( void )
{
    return lastChangedPos;
}

int Collab_getLastChangedCodePoint( void )
{
    return lastChangedCodePoint;
}


#ifdef BUILD_COLLAB
  // client beacon for discovery
  static zbeacon_t* client_beacon = 0;
  static GHashTable* peers = 0;
  static BackgroundTimer_t* client_beacon_timerID = 0;
#endif



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
    byte* msgtype = kvmsg_get_prop (kvmsg, "type" );

    printf("cc process_update() uuid:%s\n", uuid );
    FontView* fv = FontViewFindUI( FontViewFind_byXUIDConnected, uuid );
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

	if( !strcmp( msgtype, MSG_TYPE_UNDO_FONTLEVEL ))
	{
	    SFUndoes* u = SFUndoFromString( (char*)data );
	    SFUndoPerform( u, sf );
	    printf ("I: processed update=%d\n", (int) cc->sequence);
	    return;
	    
	}
	
	
	char* pos  = kvmsg_get_prop (kvmsg, "pos" );
	char* name = kvmsg_get_prop (kvmsg, "name" );
	printf("pos:%s\n", pos );
	SplineChar* sc = SFGetOrMakeChar( sf, -1, name );
	
	printf("sc:%p\n", sc );
	if( !sc )
	{
	    printf("WARNING: font view does not have a glyph for pos:%s\n",  pos );
	    printf("WARNING: font view does not have a glyph for name:%s\n", name );
	    return;
	}
	
	printf("sc.name:%s\n", sc->name );
	printf("data.size:%ld\n", data_size );
	if( DEBUG_SHOW_SFD_CHUNKS )
	    printf("data:%s\n", data );
		    
	int current_layer = 0;

	if( !sc->views && create )
	{
	    int show = 0;
	    CharView* cv = CharViewCreateExtended( sc, fv, -1, show );
	    printf("created charview:%p\n", cv );
	}
	
	for( CharViewBase* cv = sc->views; cv; cv = cv->next )
	{
	    printf("have charview:%p\n", cv );
	    
	    char filename[PATH_MAX];
	    snprintf(filename, PATH_MAX, "%s/fontforge-collab-inx-%d.sfd", getTempDir(), getpid() );
	    GFileWriteAll( filename, (char*)data);
	    FILE* file = fopen( filename, "rb" );
	    Undoes* undo = SFDGetUndo( file, sc,
				       "UndoOperation",
				       current_layer );
	    fclose(file);
	    if( !undo )
	    {
		printf("***** ERROR ****** reading back undo instance!\n");
		printf("data: %s\n\n", data );
	    }
	    if( undo )
	    {
		// NOT HANDLED!
		if( undo->undotype == ut_statehint )
		{
		    printf("*** warning ut_statehint not handled\n");
		    break;
		}

		printf("________________________ READ undo.layer: %d  dm:%d layer_sz:%d\n",
		       undo->layer, cv->drawmode, cv->sc->layer_cnt );
		int selectedlayer = cv->drawmode;
		if( undo->layer != UNDO_LAYER_UNKNOWN )
		    selectedlayer = undo->layer;    

		// use oldlayer to store current setting and switch to the
		// selected layer for this block.
		int oldlayer = cv->drawmode;
		cv->drawmode = selectedlayer;
		undo->next = 0;
		undo->next = cv->layerheads[selectedlayer]->redoes;
		cv->layerheads[selectedlayer]->redoes = undo;
		printf("________________________ READ ... 2\n");
		printf("________________________ READ ... 2... ly_fore:%d cv.layer:%d\n", ly_fore, CVLayer(cv));
		// cv->layerheads[cv->drawmode]-cv->sc->layers;
		printf("________________________ READ ... 2...         dm:%d\n", cv->drawmode );
		printf("________________________ READ ... 2... sc->layers:%p\n", cv->sc->layers );
		printf("________________________ READ ... 2... lh.dm     :%p\n", cv->layerheads[cv->drawmode] );
		
		CVDoRedo( cv );
		printf("________________________ READ ... 3\n");
		char* isLocalUndo = kvmsg_get_prop (kvmsg, "isLocalUndo" );
		if( isLocalUndo )
		{
		    if( isLocalUndo[0] == '1' )
		    {
			printf("________________________ READ ... isLocal. \n");
			Undoes* undo = cv->layerheads[selectedlayer]->undoes;
			if( undo )
			{
			    cv->layerheads[selectedlayer]->undoes = undo->next;
			    undo->next = cv->layerheads[selectedlayer]->redoes;
			    cv->layerheads[selectedlayer]->redoes = undo;
			}
		    }
		}
		printf("________________________ READ ... 4\n");

		if( cv->drawmode != oldlayer )
		{
		    cv->drawmode = oldlayer;
		    CVCharChangedUpdate( cv );

		}
		printf("________________________ READ ... 5\n");
		
		
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
		    int create = 1;
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



static void zeromq_beacon_show_peers( cloneclient_t* cc )
{
    return;
    
    printf("--- zeromq_beacon_show_peers(top)\n" );
    time_t tt = time(0);
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, peers);
    while (g_hash_table_iter_next (&iter, &key, &value)) 
    {
	beacon_announce_t* ba = (beacon_announce_t*)value;
	printf("----------\n" );
	printf("uuid:%s\n", ba->uuid );
	printf("user:%s\n", ba->username );
	printf("mach:%s\n", ba->machinename );
	printf("ip  :%s\n", ba->ip );
	printf("port:%d\n", ba->port );
	printf("seconds since last msg:%d\n", (int)(tt - ba->last_msg_from_peer_time) );
	
    }
    printf("--- zeromq_beacon_show_peers(end)\n" );
}



static void zeromq_beacon_fd_callback(int zeromq_fd, void* datas )
{
//    cloneclient_t* cc = (cloneclient_t*)datas;

//    printf("zeromq_beacon_fd_callback(top)\n");
    
    int opt = 0;
    size_t optsz = sizeof(int);
    zmq_getsockopt( zbeacon_socket (client_beacon), ZMQ_EVENTS, &opt, &optsz );
//    printf("zeromq_beacon_fd_callback(2) opt:%d\n", opt );

    if( opt & ZMQ_POLLIN )
    {
//	printf("zeromq_beacon_fd_callback() have message!\n");

	while( 1 )
	{
	    char *ipaddress = zstr_recv_nowait (zbeacon_socket (client_beacon));
//	    printf("zeromq_beacon_fd_callback() have data? p:%p\n", ipaddress );
	    if( ipaddress )
	    {
//		printf("zeromq_beacon_fd_callback() have message! ip:%s\n", ipaddress );
		zframe_t *content = zframe_recv_nowait (zbeacon_socket (client_beacon));
		if( content )
		{
		    beacon_announce_t* ba = (beacon_announce_t*)zframe_data(content);
//		    printf("uuid:%s\n", ba->uuid );
//		    printf("user:%s\n", ba->username );
//		    printf("mach:%s\n", ba->machinename );

//		    if( ba->version >= 2 && ff_uuid_isValid(ba->uuid) ) {
//			printf("have a beacon back for xuid:%s\n", ba->uuid );
//		    }
		    
		    beacon_announce_t* copy = g_malloc( sizeof(beacon_announce_t));
		    memcpy( copy, ba, sizeof(beacon_announce_t));
		    copy->last_msg_from_peer_time = time(0);
		    copy->port = ntohs( copy->port );
		    strncpy( copy->ip, ipaddress, beacon_announce_ip_sz );
		    g_hash_table_replace( peers, copy->uuid, copy );
		    zframe_destroy (&content);
		}
		free (ipaddress);
	    }
	    else
		break;
	}
    }
    zeromq_beacon_show_peers(0);
}

static void zeromq_beacon_timer_callback( void* ccvp )
{
//    cloneclient_t *cc = (cloneclient_t*)ccvp;
    zeromq_beacon_fd_callback( 0, 0 );
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

//    printf("collabclient_roundTripTimer() cc: %p\n", cc );
//    printf("collabclient_roundTripTimer() waitingseq: %d\n", cc->roundTripTimerWaitingSeq );

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
	    FontView* fv = FontViewFindUI( FontViewFind_byCollabPtr, cc );
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


typedef struct _CollabSessionCallbacks CollabSessionCallbacks;
typedef struct _CollabSessionCallbacksClass CollabSessionCallbacksClass;
struct _CollabSessionCallbacks         { GObject parent_instance; };
struct _CollabSessionCallbacksClass    { GObjectClass parent_class; };
GType collab_sessioncallbacks_get_type (void);
static void collab_sessioncallbacks_init (CollabSessionCallbacks *self) {}

static void
collab_sessioncallbacks_class_init (CollabSessionCallbacksClass *klass)
{
    GType argtypes[] = {G_TYPE_POINTER};

    g_signal_newv ("joining",
		   (collab_sessioncallbacks_get_type ()),
		   G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		   NULL /* closure */,
		   NULL /* accumulator */,
		   NULL /* accumulator data */,
		   g_cclosure_marshal_VOID__POINTER,
		   G_TYPE_NONE /* return_type */,
		   1           /* n_params */,
		   argtypes    /* param_types */);
    g_signal_newv ("leaving",
		   (collab_sessioncallbacks_get_type ()),
		   G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		   NULL /* closure */,
		   NULL /* accumulator */,
		   NULL /* accumulator data */,
		   g_cclosure_marshal_VOID__POINTER,
			 G_TYPE_NONE /* return_type */,
		   1           /* n_params */,
		   argtypes    /* param_types */);
}

G_DEFINE_TYPE (CollabSessionCallbacks, collab_sessioncallbacks, G_TYPE_OBJECT)


gpointer getSessionCallbacksObject()
{
    static gpointer ret = 0;
    if( !ret )
	ret = g_object_new( collab_sessioncallbacks_get_type(), 0 );
    return ret;
}

void collabclient_notifySessionJoining( cloneclient_t *cc, FontViewBase* fv )
{
#ifdef BUILD_COLLAB

    printf("AAA collabclient_notifySessionJoining() cc: %p, fv:%p\n", cc, fv);
    g_signal_emit_by_name( getSessionCallbacksObject(), "joining", fv );

#endif    
}
void collabclient_notifySessionLeaving( cloneclient_t *cc, FontViewBase* fv )
{
#ifdef BUILD_COLLAB

    if (cc) {
        cc->sessionIsClosing = 1;
        g_signal_emit_by_name( getSessionCallbacksObject(), "leaving", fv );
    } else {
        LogError(_("collabclient_notifySessionLeaving: cc was NULL"));
    }

#endif
}
void collabclient_addSessionJoiningCallback( collabclient_notification_cb func )
{
    void* data = 0;
    g_signal_connect( getSessionCallbacksObject(), "joining", G_CALLBACK(func), data );
}
void collabclient_addSessionLeavingCallback( collabclient_notification_cb func )
{
    void* data = 0;
    g_signal_connect( getSessionCallbacksObject(), "leaving", G_CALLBACK(func), data );
}


void collabclient_sessionDisconnect( FontViewBase* fv )
{
#ifdef BUILD_COLLAB
    
    cloneclient_t *cc = fv->collabClient;

    collabclient_notifySessionLeaving( cc, fv );


    fv->collabState = cs_disconnected;
    fv->collabClient = 0;
    FVTitleUpdate( fv );
    
    collabclient_free( (void**)&cc );

#endif
}

#ifdef BUILD_COLLAB

void
collabclient_ensureClientBeacon(void)
{
    if( client_beacon )
	return;
    
    peers = g_hash_table_new_full( g_str_hash, g_str_equal, 0, g_free );
    client_beacon_timerID = 0;
    
    
    client_beacon = zbeacon_new( obtainMainZMQContext(), 5670 );
    DEBUG("client beacon address: %s\n", zbeacon_hostname(client_beacon));
    zbeacon_subscribe (client_beacon, NULL, 0);
    zsocket_set_rcvtimeo (zbeacon_socket (client_beacon), 100);
    int fd = 0;
    size_t fdsz = sizeof(fd);
    int rc = zmq_getsockopt( zbeacon_socket(client_beacon), ZMQ_FD, &fd, &fdsz );
//    printf("beacon rc:%d fd:%d\n", rc, fd );
//    GDrawAddReadFD( 0, fd, cc, zeromq_beacon_fd_callback );
    client_beacon_timerID = BackgroundTimer_new( 1000, zeromq_beacon_timer_callback, 0 );
}

#else
void collabclient_ensureClientBeacon()
{
}
#endif


#ifdef BUILD_COLLAB


static void
collabclient_remakeSockets( cloneclient_t *cc )
{
    cc->snapshot = zsocket_new (cc->ctx, ZMQ_DEALER);
    zsocket_connect (cc->snapshot,
        "tcp://%s:%d", cc->address, cc->port+socket_offset_snapshot);
    
    cc->subscriber = zsocket_new (cc->ctx, ZMQ_SUB);
    zsocket_set_subscribe (cc->subscriber, "");
    zsocket_connect (cc->subscriber,
        "tcp://%s:%d", cc->address, cc->port+socket_offset_subscriber);
    zsocket_set_subscribe (cc->subscriber, SUBTREE);

    cc->publisher = zsocket_new (cc->ctx, ZMQ_PUSH);
    zsocket_connect (cc->publisher,
        "tcp://%s:%d",
        cc->address, cc->port+socket_offset_publisher);

    int fd = 0;
    size_t fdsz = sizeof(fd);
    int rc = zmq_getsockopt( cc->subscriber, ZMQ_FD, &fd, &fdsz );
    printf("subscriber rc:%d fd:%d\n", rc, fd );
    GDrawAddReadFD( 0, fd, cc, zeromq_subscriber_fd_callback );
  
}

#endif

void* collabclient_newFromPackedAddress( char* packed )
{
#ifdef BUILD_COLLAB

    if( !strncmp( packed, "tcp://", strlen("tcp://")))
	packed += strlen("tcp://");
    
    int port_default = 5556;
    int port = port_default;
    char address[IPADDRESS_STRING_LENGTH_T];
    strncpy( address, packed, IPADDRESS_STRING_LENGTH_T-1 );
    HostPortUnpack( address, &port, port_default );
    
    return( collabclient_new( address, port ) );
    
#else
    return 0;
#endif
}


void* collabclient_new( char* address, int port )
{
#ifdef BUILD_COLLAB

    printf("collabclient_new() address:%s port:%d\n", address, port );

    DEBUG_SHOW_SFD_CHUNKS = getenv("FONTFORGE_DEBUG_COLLAB_SHOW_SFD_CHUNKS") > 0;
    
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
    cc->sessionIsClosing = 0;
    
    int32 roundTripTimerMS = pref_collab_roundTripTimerMS;
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

    FontView* fv = FontViewFindUI( FontViewFind_byCollabPtr, cc );
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

static void collabclient_sendSFD( void* ccvp, char* sfd, char* collab_uuid, char* fontname )
{
    cloneclient_t* cc = (cloneclient_t*)ccvp;

    kvmsg_t *kvmsg = kvmsg_new(0);
    kvmsg_fmt_key  (kvmsg, "%s%d", SUBTREE, cc->publisher_sendseq++);
    kvmsg_set_body (kvmsg, sfd, strlen(sfd));
    kvmsg_set_prop (kvmsg, "type", MSG_TYPE_SFD );
    kvmsg_set_prop (kvmsg, "fontname", fontname );
    printf("****** collab_uuid: %s\n", collab_uuid );
    kvmsg_set_prop (kvmsg, "collab_uuid", collab_uuid );
//    kvmsg_set_prop (kvmsg, "ttl", "%d", randof (30));
    kvmsg_send     (kvmsg, cc->publisher);
    kvmsg_destroy (&kvmsg);
    DEBUG("Sent a SFD of %zu bytes to the server\n",strlen(sfd));
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
collabclient_sendRedo_sfdfragment( cloneclient_t* cc,
				   SplineFont* sf,
				   SplineChar *sc, /* optional */
				   char* sfdfrag,  /* SFD that makes sense for redo */
				   char* msgtype,  /* will be MSG_TYPE_UNDO if set to zero */
				   int isLocalUndo )
{
    printf("collabclient_sendRedo_sfdfragment(top)\n");
    if( !cc )
	return;
    if( !msgtype )
	msgtype = MSG_TYPE_UNDO;
    
    char pos[100];
    char* uuid = sf->xuid;
    printf("uuid:%s\n", uuid );

    int idx = 0;
    char* sfd = sfdfrag;
    printf("read undo sfd, data:%p\n", sfd );
    if( DEBUG_SHOW_SFD_CHUNKS )
	printf("SENDING: %s\n\n", sfd );

    printf("timers1...\n" );
    cc->roundTripTimerWaitingSeq = cc->publisher_sendseq;
    BackgroundTimer_touch( cc->roundTripTimer );
    printf("timers2...\n" );
    printf("sfd:%p...\n", sfd );

    kvmsg_t *kvmsg = kvmsg_new(0);
    kvmsg_fmt_key  (kvmsg, "%s%d", SUBTREE, cc->publisher_sendseq++);
    kvmsg_set_body (kvmsg, sfd, strlen(sfd) );
    kvmsg_set_prop (kvmsg, "type", msgtype );
    kvmsg_set_prop (kvmsg, "uuid", uuid );
    if( sc )
    {
	sprintf(pos, "%d", sc->orig_pos );
	printf("pos:%s\n", pos );
	printf("sc.name:%s\n", sc->name );
    }
    
    size_t data_size = kvmsg_size (kvmsg);
    printf("data.size:%ld\n", data_size );

    kvmsg_set_prop (kvmsg, "pos", pos );
    if( sc )
	kvmsg_set_prop (kvmsg, "name", sc->name );
    sprintf(pos, "%d", isLocalUndo );
    kvmsg_set_prop (kvmsg, "isLocalUndo", pos );
    kvmsg_send     (kvmsg, cc->publisher);
    kvmsg_destroy  (&kvmsg);
    DEBUG("Sent a undo chunk of %zu bytes to the server\n",strlen(sfd));
    free(sfd);
}


static void
collabclient_sendRedo_Internal( FontViewBase *fv, SplineChar *sc, Undoes *undo, int isLocalUndo )
{
    printf("collabclient_sendRedo_Internal()\n");
    cloneclient_t* cc = fv->collabClient;
    if( !cc )
	return;
    char* uuid = fv->sf->xuid;
    printf("uuid:%s\n", uuid );

    printf("________________________ WRITE undo.layer: %d type:%d layer_sz:%d\n",
	   undo->layer, undo->undotype, sc->layer_cnt );
    
    int idx = 0;
    char filename[PATH_MAX];
    snprintf(filename, PATH_MAX, "%s/fontforge-collab-x.sfd", getTempDir() );
    FILE* f = fopen( filename, "wb" );
    SFDDumpUndo( f, sc, undo, "Undo", idx );
    fclose(f);
    printf("wrote undo sfd... filename: %s\n", filename );
    char* sfd = GFileReadAll( filename );
    printf("read undo sfd, data:%p\n", sfd );
    if( DEBUG_SHOW_SFD_CHUNKS )
	printf("SENDING: %s\n\n", sfd );

    printf("timers1...\n" );
    cc->roundTripTimerWaitingSeq = cc->publisher_sendseq;
    BackgroundTimer_touch( cc->roundTripTimer );
    printf("timers2...\n" );
    printf("sfd:%p...\n", sfd );

    kvmsg_t *kvmsg = kvmsg_new(0);
    kvmsg_fmt_key  (kvmsg, "%s%d", SUBTREE, cc->publisher_sendseq++);
    kvmsg_set_body (kvmsg, sfd, strlen(sfd) );
    kvmsg_set_prop (kvmsg, "type", MSG_TYPE_UNDO );
    kvmsg_set_prop (kvmsg, "uuid", uuid );
    char pos[100];
    sprintf(pos, "%d", sc->orig_pos );
    printf("pos:%s\n", pos );
    printf("sc.name:%s\n", sc->name );
    
    size_t data_size = kvmsg_size (kvmsg);
    printf("data.size:%ld\n", data_size );

    kvmsg_set_prop (kvmsg, "pos", pos );
    kvmsg_set_prop (kvmsg, "name", sc->name );
    sprintf(pos, "%d", isLocalUndo );
    kvmsg_set_prop (kvmsg, "isLocalUndo", pos );
    kvmsg_send     (kvmsg, cc->publisher);
    kvmsg_destroy (&kvmsg);
    DEBUG("Sent a undo chunk of %zu bytes to the server\n",strlen(sfd));
    free(sfd);
}

static void
collabclient_sendRedo_Internal_CV( CharViewBase *cv, Undoes *undo, int isLocalUndo )
{
    printf("collabclient_sendRedo_Internal_CV() cv:%p\n", cv );
    collabclient_sendRedo_Internal( cv->fv, cv->sc, undo, isLocalUndo );
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
//	printf("collabclient_sniffForLocalServer_timer() p:%s\n", p);
	if( !strcmp(p,"pong"))
	{
//	    printf("******* have local server!\n");
	    cc->haveServer = 1;
	}
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef BUILD_COLLAB

BackgroundTimer_t* beacon_moon_bounce_timerID = 0;

static void beacon_moon_bounce_timer_callback( void* ccvp )
{
    cloneclient_t *cc = (cloneclient_t*)ccvp;
    char* sought_uuid = cc->unacknowledged_beacon_uuid;

    BackgroundTimer_remove( beacon_moon_bounce_timerID );
    beacon_moon_bounce_timerID = 0;
    
    time_t tt = time(0);
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, peers);
    while (g_hash_table_iter_next (&iter, &key, &value)) 
    {
	beacon_announce_t* ba = (beacon_announce_t*)value;
	if( !strcmp( ba->uuid, sought_uuid ))
	{
	    printf("it took %d seconds to get a beacon back from the server\n",
		   (int)(tt - cc->unacknowledged_beacon_sendTime) );
	    strcpy( cc->unacknowledged_beacon_uuid, "" );
	    cc->unacknowledged_beacon_sendTime = 0;
	    return;
	}
    }

    LogError( _("Collab: A beacon has not been received from the server"));
    LogError( _("Collab: Please ensure that UDP port 5670 is not firewalled."));
}
#endif


void collabclient_sessionStart( void* ccvp, FontView *fv )
{
#ifdef BUILD_COLLAB

    cloneclient_t* cc = (cloneclient_t*)ccvp;
    
    //
    // Fire up the fontforge-internal-collab-server process...
    //
    {
	char command_line[PATH_MAX+1];
	
#if defined(__MINGW32__)

	sprintf(command_line, "'%s/ffcollab.bat' %d", getGResourceProgramDir(), cc->port );
	
#else	
	sprintf(command_line,
		"%s/bin/FontForgeInternal/fontforge-internal-collab-server %d",
		getLibexecDir_NonWindows(), cc->port );
#endif	
	printf("command_line:%s\n", command_line );
	GError * error = 0;
	if(!getenv("FONTFORGE_USE_EXISTING_SERVER"))
	{
	    int rc = g_spawn_command_line_async( command_line, &error );
	    if( !rc )
	    {
		fprintf(stderr, "Error starting collab server\n");
		if( error )
		    fprintf(stderr, "code:%d message:%s\n", error->code, error->message );
	    }
	}
    }
    
    printf("Starting a session, sending it the current SFD as a baseline...\n");
    if( !ff_uuid_isValid( fv->b.sf->collab_uuid))
	ff_uuid_generate( fv->b.sf->collab_uuid );
    strcpy( cc->unacknowledged_beacon_uuid, fv->b.sf->collab_uuid );
    time( &cc->unacknowledged_beacon_sendTime );

    
    int s2d = 0;
    char filename[PATH_MAX];
    snprintf(filename, PATH_MAX, "%s/fontforge-collab-start-%d.sfd", getTempDir(), getpid());
    int ok = SFDWrite(filename,fv->b.sf,fv->b.map,fv->b.normal,s2d);
    printf("connecting to server...3 ok:%d\n",ok);
    if ( ok )
    {
	char* sfd = GFileReadAll( filename );
	printf("connecting to server...4 sfd:%p\n", sfd );
	collabclient_sendSFD( cc, sfd, fv->b.sf->collab_uuid, fv->b.sf->fontname );
    }
    GFileUnlink(filename);
    printf("connecting to server...sent the sfd for session start.\n");
    fv->b.collabState = cs_server;
    FVTitleUpdate( &fv->b );

    collabclient_setHaveLocalServer( 1 );

    beacon_moon_bounce_timerID = BackgroundTimer_new( 3000, beacon_moon_bounce_timer_callback, cc );

    collabclient_notifySessionJoining( cc, (FontViewBase*)fv );
#endif
}




FontViewBase* collabclient_sessionJoin( void* ccvp, FontView *fv )
{
    FontViewBase* ret = 0;
    
#ifdef BUILD_COLLAB
    
    cloneclient_t* cc = (cloneclient_t*)ccvp;
    printf("collabclient_sessionJoin(top)\n");

    //  Get state snapshot
    cc->sequence = 0;
    zstr_sendm (cc->snapshot, "ICANHAZ?");
    zstr_send  (cc->snapshot, SUBTREE);

    // if we wait for timeoutMS millisec then we assume failure
    // timeWaitedMS is used to keep track of how long we have waited
    kvmsg_t* lastSFD = 0;
    int timeWaitedMS = 0;
    int timeoutMS = pref_collab_sessionJoinTimeoutMS;
    while (true)
    {
	printf("timeoutMS:%d timeWaitedMS:%d\n", timeoutMS, timeWaitedMS );
	if( timeWaitedMS >= timeoutMS )
	{
	    char* addrport = collabclient_makeAddressString( cc->address, cc->port );
	    gwwv_post_error(_("FontForge Collaboration"),
			    _("Failed to connect to server session at %s"),
			    addrport );
	    return 0;
	}

        kvmsg_t *kvmsg = kvmsg_recv_full( cc->snapshot, ZMQ_DONTWAIT );
	if (!kvmsg)
	{
	    int msToWait = 50;
	    g_usleep( msToWait * 1000 );
	    timeWaitedMS += msToWait;
	    continue;
        }
	
    
        /* kvmsg_t *kvmsg = kvmsg_recv (cc->snapshot); */
        /* if (!kvmsg) */
        /*     break;          //  Interrupted */
	
        if (streq (kvmsg_key (kvmsg), "KTHXBAI")) {
            cc->sequence = kvmsg_sequence (kvmsg);
            printf ("I: received snapshot=%d\n", (int) cc->sequence);
            kvmsg_destroy (&kvmsg);
	    // Done
            break;
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
	return 0;
    }
    
    if( lastSFD )
    {
	int openflags = 0;
	char filename[PATH_MAX];
	snprintf(filename, PATH_MAX, "%s/fontforge-collab-import-%d.sfd",getTempDir(),getpid());
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
	collabclient_notifySessionJoining( cc, newfv );

	ret = newfv;
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
    return ret;
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

    FontView* fv = FontViewFindUI( FontViewFind_byCollabPtr, cc );
    if( fv )
    {
	collabclient_sessionJoin( cc, fv );
    }

#endif
}




static int reallyPerformUndo = 1;

int collabclient_reallyPerformUndo( CharViewBase *cv )
{
    return reallyPerformUndo;
}


void collabclient_performLocalUndo( CharViewBase *cv )
{
#ifdef BUILD_COLLAB

    Undoes *undo = cv->layerheads[cv->drawmode]->undoes;
    if( undo )
    {
	printf("undo:%p\n", undo );
	cv->layerheads[cv->drawmode]->undoes = undo->next;
	undo->next = 0;
	collabclient_sendRedo_Internal_CV( cv, undo, 1 );
	UndoesFree( undo );
    }
    cloneclient_t* cc = cv->fv->collabClient;
    cc->preserveUndo = 0;

#endif
}


void collabclient_performLocalRedo( CharViewBase *cv )
{
#ifdef BUILD_COLLAB

    cloneclient_t* cc = cv->fv->collabClient;
    if( !cc )
	return;

    printf("collabclient_performLocalRedo() cv:%p\n", cv );
    printf("collabclient_performLocalRedo() dm:%d\n", cv->drawmode );
    printf("collabclient_performLocalRedo() preserveUndo:%p\n", cc->preserveUndo );
    Undoes *undo = cv->layerheads[cv->drawmode]->redoes;
    printf("collabclient_performLocalRedo() undo:%p\n", undo );

    if( undo )
    {
	cv->layerheads[cv->drawmode]->redoes = undo->next;
	undo->next = 0;
	collabclient_sendRedo_Internal_CV( cv, undo, 0 );
	undo->next = 0;
	UndoesFree( undo );
    }
    cc->preserveUndo = 0;

#endif
}



void collabclient_sendRedo_SC( SplineChar *sc, int layer )
{
#ifdef BUILD_COLLAB

    if( layer == UNDO_LAYER_UNKNOWN )
    {
	layer = ly_fore;
    }
    

    FontViewBase* fv = sc->parent->fv;
    cloneclient_t* cc = fv->collabClient;
    if( !cc )
	return;

    printf("collabclient_sendRedo(SC) fv:%p layer:%d\n", fv, layer );
    printf("collabclient_sendRedo() preserveUndo:%p\n", cc->preserveUndo );
    if( !cc->preserveUndo )
	return;

    if( DEBUG_SHOW_SFD_CHUNKS )
	dumpUndoChain( "start of collabclient_sendRedo()", sc, sc->layers[layer].undoes );
    if( true )
    {
	Undoes* undo = sc->layers[layer].undoes;
	while( undo && undo->undotype == ut_statehint )
	{
	    undo = undo->next;
	    // FIXME: throwing away the statehints for now.
	}
	sc->layers[layer].undoes = undo;
    }

#if 0    
    // This is a special case for testing metricsview
    // for undo chains 3,1,7: ut_statehint, ut_state, ut_width
    {
	Undoes* undo = sc->layers[layer].undoes;
	if( undo && undo->next && undo->undotype == ut_state && undo->next->undotype == ut_width )
	    undo = undo->next;
	// FIXME: throwing away info here, should do better.
	sc->layers[layer].undoes = undo;
    }
#endif
    if( DEBUG_SHOW_SFD_CHUNKS )
	dumpUndoChain( "start of collabclient_sendRedo(2)", sc, sc->layers[layer].undoes );

    
    SCDoUndo( sc, layer );
//    CVDoUndo( cv );
    Undoes *undo = sc->layers[layer].redoes;
    printf("collabclient_sendRedo() undo:%p\n", undo );

    if( undo )
    {
	sc->layers[layer].redoes = undo->next;
	collabclient_sendRedo_Internal( fv, sc, undo, 0 );
	undo->next = 0;
	UndoesFree( undo );
    }
    cc->preserveUndo = 0;

#endif
}


void collabclient_sendRedo( CharViewBase *cv )
{
#ifdef BUILD_COLLAB

    collabclient_sendRedo_SC( cv->sc, CVLayer(cv) );
    
    /* cloneclient_t* cc = cv->fv->collabClient; */
    /* if( !cc ) */
    /* 	return; */

    /* printf("collabclient_sendRedo() cv:%p\n", cv ); */
    /* printf("collabclient_sendRedo() dm:%d\n", cv->drawmode ); */
    /* printf("collabclient_sendRedo() preserveUndo:%p\n", cc->preserveUndo ); */
    /* if( !cc->preserveUndo ) */
    /* 	return; */

    /* dumpUndoChain( "start of collabclient_sendRedo()", cv->sc, cv->layerheads[cv->drawmode]->undoes ); */
    /* { */
    /* 	Undoes* undo = cv->layerheads[cv->drawmode]->undoes; */
    /* 	if( undo && undo->undotype == ut_statehint ) */
    /* 	{ */
    /* 	    cv->layerheads[cv->drawmode]->undoes = undo->next; */
    /* 	    // FIXME */
    /* 	} */
    /* } */
    
    /* CVDoUndo( cv ); */
    /* Undoes *undo = cv->layerheads[cv->drawmode]->redoes; */
    /* printf("collabclient_sendRedo() undo:%p\n", undo ); */

    /* if( undo ) */
    /* { */
    /* 	cv->layerheads[cv->drawmode]->redoes = undo->next; */
    /* 	collabclient_sendRedo_Internal_CV( cv, undo, 0 ); */
    /* 	undo->next = 0; */
    /* 	UndoesFree( undo ); */
    /* } */
    /* cc->preserveUndo = 0; */

#endif
}

void collabclient_sendFontLevelRedo( SplineFont* sf )
{
#ifdef BUILD_COLLAB
    
    printf("collabclient_sendFontLevelRedo() sf:%p\n", sf );
    if( !sf->undoes )
	return;
    
    struct sfundoes *undo = sf->undoes;
    printf("font level undo msg:%s\n", undo->msg );

    //
    // create the "redo" based on what the last undo type was
    //
    undo = SFUndoCreateRedo( undo, sf );
    char* sfdfrag = SFUndoToString( undo );
    
    /* switch(undo->type) */
    /* { */
    /* case sfut_fontinfo: */
    /* 	printf("font info!\n"); */
    /* 	sfdfrag = DumpSplineFontMetadata( sf ); */
    /* 	break; */
    /* } */
    
    if( sfdfrag )
    {
	printf("have sfd frag len:%zu\n", strlen(sfdfrag));
	printf("have sfd frag\n%s\n\n", sfdfrag);

	FontViewBase* fv = sf->fv;
	cloneclient_t* cc = fv->collabClient;
	if( !cc )
	    return;

	printf("have cc too!\n");

	int isLocalUndo = 0;
	collabclient_sendRedo_sfdfragment( cc, sf, 0, sfdfrag, MSG_TYPE_UNDO_FONTLEVEL, isLocalUndo );
    }
    
    
    
#endif
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
    int port_default = collabclient_getDefaultBasePort();

    cc->socket = zsocket_new ( ctx, ZMQ_REQ );
    zsocket_connect (cc->socket,
        "tcp://localhost:%d", port_default+socket_offset_ping);
    cc->timer = BackgroundTimer_new( 1000, collabclient_sniffForLocalServer_timer, cc );
    zstr_send  (cc->socket, "ping");

#endif
}

void
collabclient_closeLocalServer( int port )
{
#ifdef BUILD_COLLAB

    collabclient_sniffForLocalServer_t* cc = &collabclient_sniffForLocalServer_singleton;
    zctx_t* ctx = obtainMainZMQContext();
    int beacon_port = port;
    if( !port )
	port = collabclient_getDefaultBasePort();

    printf("collabclient_closeLocalServer() port:%d\n", port);
    void* socket = zsocket_new ( ctx, ZMQ_REQ );
    zsocket_connect ( socket,
        "tcp://localhost:%d", port+socket_offset_ping);
    zstr_send( socket, "quit" );
    cc->haveServer = 0;

    if( beacon_port )
    {
	g_hash_table_remove_all( peers );
    }
    collabclient_sniffForLocalServer();
    
#endif
}

void
collabclient_closeAllLocalServersForce()
{
#ifdef BUILD_COLLAB

    char command_line[PATH_MAX+1];
    sprintf(command_line, "killall -9 fontforge-internal-collab-server" );
    printf("command_line:%s\n", command_line );
    GError * error = 0;
    g_spawn_command_line_async( command_line, &error );
    
#endif
}


int64_t collabclient_getCurrentSequenceNumber(void* ccvp)
{
#ifdef BUILD_COLLAB

    if( !ccvp )
	return 0;
    
    cloneclient_t *cc = (cloneclient_t *)ccvp;
    return cc->sequence;
    
#endif

    return 0;
}

void collabclient_trimOldBeaconInformation( int secondsCutOff )
{
#ifdef BUILD_COLLAB

    GHashTableIter iter;
    gpointer key, value;
    int i=0;
    time_t tt = time(0);
    if( !secondsCutOff )
	secondsCutOff = 2;

    g_hash_table_iter_init (&iter, peers);
    for( i=0; g_hash_table_iter_next (&iter, &key, &value); i++ )
    {
	beacon_announce_t* ba = (beacon_announce_t*)value;
	int seconds_since_last_msg = tt - ba->last_msg_from_peer_time;
	
	printf("seconds since last msg:%d\n", (int)(tt - ba->last_msg_from_peer_time) );
	if( seconds_since_last_msg > secondsCutOff )
	{
	    g_hash_table_remove( peers, ba->uuid );
	}
    }
    
    
#endif
}

GHashTable* collabclient_getServersFromBeaconInfomration( void )
{
#ifdef BUILD_COLLAB
    return peers;
#endif
    return 0;
}

int collabclient_getBasePort(void* ccvp)
{
#ifdef BUILD_COLLAB

    if( !ccvp )
	return 0;

    cloneclient_t *cc = (cloneclient_t *)ccvp;
    return cc->port;
#endif
    
    return 0;
}


int collabclient_isAddressLocal( char* address )
{
    char a[IPADDRESS_STRING_LENGTH_T];
    if(getNetworkAddress( a ))
    {
	if( !strcmp( address, a ))
	    return 1;
    }
    if( !strcmp( address, "127.0.0.1" ))
	return 1;
    if( !strcmp( address, "localhost" ))
	return 1;
    
    return 0;
}
