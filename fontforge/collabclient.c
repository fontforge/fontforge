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

#include "inc/ustring.h"
#include "collabclient.h"
#include "czmq.h"
#include "collab/zmq_kvmsg.h"
#include "inc/gfile.h"
#include "views.h"

#define MAGIC_VALUE 0xbeef
#define SUBTREE "/client/"

#define DEBUG zclock_log

#define MSG_TYPE_SFD  "msg_type_sfd"
#define MSG_TYPE_UNDO "msg_type_undo"

typedef struct {
    int magic_number;    //  Magic number to test if the pointer is likely valid
    zctx_t *ctx;         //  Main zeromq Context
    zloop_t *loop;       //  main zloop reactor
    char* address;       //  address of the server
    int port;            //  Base port we're working on, we use
                         //  base+n for all other sockets.
    zhash_t *kvmap;      //  Key-value store that we manage
    int64_t sequence;    //  How many updates we're at

    Undoes* preserveUndo;
    
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
    
} cloneclient_t;


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

static void zeromq_subscriber_process_update( cloneclient_t* cc, kvmsg_t *kvmsg, int create )
{
    cc->sequence = kvmsg_sequence (kvmsg);

    char* uuid = kvmsg_get_prop (kvmsg, "uuid" );
    byte* data = kvmsg_body (kvmsg);
    size_t data_size = kvmsg_size (kvmsg);

    printf("uuid:%s\n", uuid );
    FontView* fv = FontViewFind( FontViewFind_byXUID, uuid );
    printf("fv:%p\n", fv );
    if( fv )
    {
	if( !data_size )
	{
	    printf("zero length message!\n" );
	    return;
	}
			
	SplineFont *sf = fv->b.sf;
	char* pos = kvmsg_get_prop (kvmsg, "pos" );
	printf("pos:%s\n", pos );
	SplineChar *sc = sf->glyphs[ atoi(pos) ];
	printf("sc:%p\n", sc );
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


static void zeromq_subscriber_fd_callback(int zeromq_fd, void* datas )
{
    cloneclient_t* cc = (cloneclient_t*)datas;
    
//    printf("zeromq_subscriber_fd_callback(1)\n");

    int opt = 0;
    size_t optsz = sizeof(int);
    zmq_getsockopt( cc->subscriber, ZMQ_EVENTS, &opt, &optsz );

    if( opt & ZMQ_POLLIN )
    {
	printf("zeromq_subscriber_fd_callback() have message!\n");

	while( 1 )
	{
	    kvmsg_t *kvmsg = kvmsg_recv_full (cc->subscriber, ZMQ_DONTWAIT);
	    if (kvmsg)
	    {
		//  Discard out-of-sequence kvmsgs, incl. heartbeats
		if (kvmsg_sequence (kvmsg) > cc->sequence)
		{
		    int create = 0;
		    zeromq_subscriber_process_update( cc, kvmsg, create );
    
		    /* cc->sequence = kvmsg_sequence (kvmsg); */

		    /* char* uuid = kvmsg_get_prop (kvmsg, "uuid" ); */
		    /* byte* data = kvmsg_body (kvmsg); */
		    /* size_t data_size = kvmsg_size (kvmsg); */

		    /* printf("uuid:%s\n", uuid ); */
		    /* FontView* fv = FontViewFind( FontViewFind_byXUID, uuid ); */
		    /* printf("fv:%p\n", fv ); */
		    /* if( fv ) */
		    /* { */
		    /* 	if( !data_size ) */
		    /* 	{ */
		    /* 	    printf("zero length message!\n" ); */
		    /* 	    continue; */
		    /* 	} */
			
		    /* 	SplineFont *sf = fv->b.sf; */
		    /* 	char* pos = kvmsg_get_prop (kvmsg, "pos" ); */
		    /* 	printf("pos:%s\n", pos ); */
		    /* 	SplineChar *sc = sf->glyphs[ atoi(pos) ]; */
		    /* 	printf("sc:%p\n", sc ); */
		    /* 	printf("sc.name:%s\n", sc->name ); */
		    /* 	printf("data.size:%d\n", data_size ); */
		    
		    /* 	int current_layer = 0; */
			
		    /* 	for( CharViewBase* cv = sc->views; cv; cv = cv->next ) */
		    /* 	{ */
		    /* 	    char filename[PATH_MAX]; */
		    /* 	    snprintf(filename, PATH_MAX, "/tmp/fontforge-collab-inx.sfd"); */
		    /* 	    GFileWriteAll( filename, (char*)data); */
		    /* 	    FILE* file = fopen( filename, "r" ); */
		    /* 	    Undoes* undo = SFDGetUndo( sf, file, sc, */
		    /* 				       "UndoOperation", */
		    /* 				       "EndUndoOperation", */
		    /* 				       current_layer ); */
		    /* 	    fclose(file); */
		    /* 	    if( !undo ) */
		    /* 	    { */
		    /* 		printf("ERROR reading back undo instance!\n"); */
		    /* 		printf("data: %s\n\n", data ); */
		    /* 	    } */
		    /* 	    if( undo ) */
		    /* 	    { */
		    /* 		undo->next = 0; */
		    /* 		cv->layerheads[cv->drawmode]->redoes = undo; */
		    /* 		CVDoRedo( cv ); */
		    /* 	    } */
			    
		    /* 	    break; */
		    /* 	} */
		    /* } */
		    
		    
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


void* collabclient_new( char* address, int port )
{
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
    
    cc->snapshot = zsocket_new (cc->ctx, ZMQ_DEALER);
    zsocket_connect (cc->snapshot, makeAddressString(cc->address,cc->port));
    
    cc->subscriber = zsocket_new (cc->ctx, ZMQ_SUB);
    zsockopt_set_subscribe (cc->subscriber, "");
    zsocket_connect (cc->subscriber, makeAddressString(cc->address,cc->port+1));
    zsockopt_set_subscribe (cc->subscriber, SUBTREE);

    cc->publisher = zsocket_new (cc->ctx, ZMQ_PUSH);
    zsocket_connect (cc->publisher, makeAddressString(cc->address,cc->port+2));

    int fd = 0;
    size_t fdsz = sizeof(fd);
    int rc = zmq_getsockopt( cc->subscriber, ZMQ_FD, &fd, &fdsz );
    printf("rc:%d fd:%d\n", rc, fd );
    setZeroMQReadFD( 0, fd, cc, zeromq_subscriber_fd_callback );
    
    return cc;
}

void collabclient_free( void** ccvp )
{
    cloneclient_t* cc = (cloneclient_t*)*ccvp;
    if( !cc )
	return;
    if( cc->magic_number != MAGIC_VALUE )
    {
	fprintf(stderr,"collabclient_free() called on an invalid client object!\n");
	return;
    }
    cc->magic_number = MAGIC_VALUE + 1;

    zhash_destroy (&cc->kvmap);
    free(cc->address);
    free(cc);
    *ccvp = 0;
}

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


void collabclient_sessionStart( void* ccvp, FontView *fv )
{
    cloneclient_t* cc = (cloneclient_t*)ccvp;

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
}

/* static int collabclient_sessionJoin_findSFD_foreach_fn( const char *key, void *item, void *argumentpp ) */
/* { */
/*     void** argument = (void**)argumentpp; */
/*     kvmsg_t* kvmsg = (kvmsg_t*)item; */

/*     printf("collabclient_sessionJoin_findSFD_foreach_fn() type:%s\n", kvmsg_get_prop (kvmsg, "type")); */
/*     if( !strcmp(kvmsg_get_prop (kvmsg, "type"), MSG_TYPE_SFD )) */
/*     { */
/* 	if( *argument ) */
/* 	{ */
/* 	    kvmsg_t* current = (kvmsg_t*)*argument; */
/* 	    if( kvmsg_sequence (kvmsg) > kvmsg_sequence (current) ) */
/* 	    { */
/* 		*argument = kvmsg; */
/* 	    } */
/* 	} */
/* 	else */
/* 	{ */
/* 	    *argument = kvmsg; */
/* 	} */
/*     } */
/*     return 0; */
/* } */


/* struct collabclient_sessionJoin_processUpdates_foreach_args */
/* { */
/*     cloneclient_t* cc; */
/*     kvmsg_t* lastSFD; */
/* }; */
/* static int collabclient_sessionJoin_processUpdates_foreach_fn( const char *key, void *item, void *argument ) */
/* { */
/*     struct collabclient_sessionJoin_processUpdates_foreach_args* args = */
/* 	(struct collabclient_sessionJoin_processUpdates_foreach_args*)argument; */
/*     cloneclient_t* cc = args->cc; */
/*     kvmsg_t* lastSFD = args->lastSFD; */
/*     kvmsg_t* kvmsg = (kvmsg_t*)item; */

/*     printf("collabclient_sessionJoin_processUpdates_foreach_fn() type:%s\n", kvmsg_get_prop (kvmsg, "type")); */
/*     if( !strcmp(kvmsg_get_prop (kvmsg, "type"), MSG_TYPE_UNDO )) */
/*     { */
/* 	if( kvmsg_sequence (kvmsg) > kvmsg_sequence (lastSFD) ) */
/* 	{ */
/* 	    printf("processUpdates_foreach_fn() seq:%d\n", kvmsg_sequence(kvmsg)); */
/* 	    int create = 1; */
/* 	    zeromq_subscriber_process_update( cc, kvmsg, create ); */
/* 	} */
/*     } */
/*     return 0; */
/* } */


static int collabclient_sessionJoin_processmsg_foreach_fn( kvmsg_t* msg, void *argument )
{
    cloneclient_t* cc = (cloneclient_t*)argument;
    printf("processmsg_foreach() seq:%ld\n", kvmsg_sequence (msg) );
    int create = 1;
    zeromq_subscriber_process_update( cc, msg, create );
    
    return 0;
}


void collabclient_sessionJoin( void* ccvp, FontView *fv )
{
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
	printf ("I: storing seq=%ld\n", kvmsg_sequence (kvmsg));
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

void collabclient_CVPreserveStateCalled( CharViewBase *cv )
{
    cloneclient_t* cc = cv->fv->collabClient;
    if( !cc )
	return;

    Undoes *undo = cv->layerheads[cv->drawmode]->undoes;
    cc->preserveUndo = undo;
}


void collabclient_sendRedo( CharViewBase *cv )
{
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
}

int collabclient_inSession( CharViewBase *cv )
{
    if( !cv )
	return 0;
    if( !cv->fv )
	return 0;
    cloneclient_t* cc = cv->fv->collabClient;
    return cc;
}



#if 0

int main (void)
{
    //  Get state snapshot
    int64_t sequence = 0;
    zstr_sendm (snapshot, "ICANHAZ?");
    zstr_send  (snapshot, SUBTREE);
    while (true) {
        kvmsg_t *kvmsg = kvmsg_recv (snapshot);
        if (!kvmsg)
            break;          //  Interrupted
        if (streq (kvmsg_key (kvmsg), "KTHXBAI")) {
            sequence = kvmsg_sequence (kvmsg);
            printf ("I: received snapshot=%d\n", (int) sequence);
            kvmsg_destroy (&kvmsg);
            break;          //  Done
        }
        kvmsg_store (&kvmsg, kvmap);
    }
    int64_t alarm = zclock_time () + 1000;
    while (!zctx_interrupted) {
        zmq_pollitem_t items [] = { { subscriber, 0, ZMQ_POLLIN, 0 } };
        int tickless = (int) ((alarm - zclock_time ()));
        if (tickless < 0)
            tickless = 0;
        int rc = zmq_poll (items, 1, tickless * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;              //  Context has been shut down

        if (items [0].revents & ZMQ_POLLIN) {
            kvmsg_t *kvmsg = kvmsg_recv (subscriber);
            if (!kvmsg)
                break;          //  Interrupted

            //  Discard out-of-sequence kvmsgs, incl. heartbeats
            if (kvmsg_sequence (kvmsg) > sequence) {
                sequence = kvmsg_sequence (kvmsg);
                kvmsg_store (&kvmsg, kvmap);
                printf ("I: received update=%d\n", (int) sequence);
            }
            else
                kvmsg_destroy (&kvmsg);
        }
        //  If we timed-out, generate a random kvmsg
        if (zclock_time () >= alarm) {
            kvmsg_t *kvmsg = kvmsg_new (0);
            kvmsg_fmt_key  (kvmsg, "%s%d", SUBTREE, randof (10000));
            kvmsg_fmt_body (kvmsg, "%d", randof (1000000));
            kvmsg_set_prop (kvmsg, "ttl", "%d", randof (30));
            kvmsg_send     (kvmsg, publisher);
            kvmsg_destroy (&kvmsg);
            alarm = zclock_time () + 1000;
        }
    }
    printf (" Interrupted\n%d messages in\n", (int) sequence);
    zhash_destroy (&kvmap);
    zctx_destroy (&ctx);
    return 0;
}

#endif
