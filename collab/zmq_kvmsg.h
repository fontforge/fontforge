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

#ifndef __KVMSG_H_INCLUDED__
#define __KVMSG_H_INCLUDED__

#include "czmq.h"

//  Opaque class structure
typedef struct _kvmsg kvmsg_t;

#ifdef __cplusplus
extern "C" {
#endif

    enum { socket_offset_snapshot = 0,
	   socket_offset_subscriber,
	   socket_offset_publisher,
	   socket_offset_ping 
    };

    enum { socket_srv_offset_snapshot  = socket_offset_snapshot,
	   socket_srv_offset_publisher = socket_offset_subscriber,
	   socket_srv_offset_collector = socket_offset_publisher,
	   socket_srv_offset_ping      = socket_offset_ping
    };

    /**
     * Its 5556 by default, but making that value contained in this
     * function allows us to set it via prefs or some other means
     */
    extern int collabclient_getDefaultBasePort( void );

    /**
     * Convert address + port to a string like
     * tcp://localhost:5556
     * 
     * you do not own the return value, do not free it. 
     */
    extern char* collabclient_makeAddressString( char* address, int port );
    
    
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

    /**
     * Visit all entries in the kvmap with a sequence number
     * greater than minsequence in the natural integer ordering
     * of sequence numbers, ie: 5,6,7,8...n
     *
     * Each msg is passed to your callback function along with a
     * given argument that you supply.
     */
    void kvmap_visit( zhash_t* kvmap, int64_t minsequence,
		      kvmsg_visit_fn* callback, void *argument );


#ifdef __cplusplus
}
#endif

#endif      //  Included
