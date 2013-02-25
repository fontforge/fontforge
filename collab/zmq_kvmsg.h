/*  =====================================================================
 *  kvmsg - key-value message class for example applications
 *  ===================================================================== */

#ifndef __KVMSG_H_INCLUDED__
#define __KVMSG_H_INCLUDED__

#include "czmq.h"

//  Opaque class structure
typedef struct _kvmsg kvmsg_t;

#ifdef __cplusplus
extern "C" {
#endif

//  Constructor, sets sequence as provided
kvmsg_t *     kvmsg_new (int64_t sequence);
    
//  Destructor
void kvmsg_destroy (kvmsg_t **self_p);
    
//  Create duplicate of kvmsg
kvmsg_t * kvmsg_dup (kvmsg_t *self);

//  Reads key-value message from socket, returns new kvmsg instance.
kvmsg_t * kvmsg_recv (void *socket);

kvmsg_t * kvmsg_recv_full (void *socket, int sockopts );
    
//  Send key-value message to socket; any empty frames are sent as such.
void kvmsg_send (kvmsg_t *self, void *socket);

//  Return key from last read message, if any, else NULL
char * kvmsg_key (kvmsg_t *self);
    
//  Return sequence nbr from last read message, if any
int64_t kvmsg_sequence (kvmsg_t *self);
    
//  Return UUID from last read message, if any, else NULL
byte * kvmsg_uuid (kvmsg_t *self);
    
//  Return body from last read message, if any, else NULL
byte * kvmsg_body (kvmsg_t *self);
    
//  Return body size from last read message, if any, else zero
size_t kvmsg_size (kvmsg_t *self);

//  Set message key as provided
void kvmsg_set_key (kvmsg_t *self, char *key);
    
//  Set message sequence number
void kvmsg_set_sequence (kvmsg_t *self, int64_t sequence);
    
//  Set message UUID to generated value
void kvmsg_set_uuid (kvmsg_t *self);
    
//  Set message body
void kvmsg_set_body (kvmsg_t *self, byte *body, size_t size);
    
//  Set message key using printf format
void kvmsg_fmt_key (kvmsg_t *self, char *format, ...);
    
//  Set message body using printf format
void kvmsg_fmt_body (kvmsg_t *self, char *format, ...);

//  Get message property, if set, else ""
char * kvmsg_get_prop (kvmsg_t *self, char *name);
    
//  Set message property
//  Names cannot contain '='. Max length of value is 255 chars.
void kvmsg_set_prop (kvmsg_t *self, char *name, char *format, ...);
    
//  Store entire kvmsg into hash map, if key/value are set
//  Nullifies kvmsg reference, and destroys automatically when no longer
//  needed.
void kvmsg_store (kvmsg_t **self_p, zhash_t *hash);
    
//  Dump message to stderr, for debugging and tracing
void kvmsg_dump (kvmsg_t *self);

//  Runs self test of class
int kvmsg_test (int verbose);


void kvmsg_free (void *ptr);

    typedef int (kvmsg_visit_fn) (kvmsg_t* msg, void *argument);

    void kvmap_visit( zhash_t* kvmap, int64_t minsequence,
		      kvmsg_visit_fn* callback, void *argument );


#ifdef __cplusplus
}
#endif

#endif      //  Included
