/* Copyright (C) 2012 by Ben Martin */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fontforge-config.h>

#include "dlist.h"

#include <stdio.h>              /* for NULL */
#include <stdlib.h>

void dlist_pushfront( struct dlistnode** list, struct dlistnode* node ) {
    if( *list ) {
	node->next = *list;
	node->next->prev = node;
    }
    *list = node;
}

int dlist_size( struct dlistnode** list ) {
    struct dlistnode* node = *list;
    int ret = 0;
    for( ; node; node=node->next ) {
	ret++;
    }
    return ret;
}

int dlist_isempty( struct dlistnode** list ) {
    return *list == NULL;
}

void dlist_erase( struct dlistnode** list, struct dlistnode* node ) {
    if( !node )
	return;
    if( *list == node ) {
	*list = node->next;
	if( node->next ) {
	    node->next->prev = 0;
	}
	return;
    }
    if( node->prev ) {
	node->prev->next = node->next;
    }
    if( node->next ) {
	node->next->prev = node->prev;
    }
}

void dlist_foreach( struct dlistnode** list, dlist_foreach_func_type func )
{
    struct dlistnode* node = *list;
    while( node ) {
	struct dlistnode* t = node;
	node = node->next;
	func( t );
    }
}

void dlist_foreach_udata( struct dlistnode** list, dlist_foreach_udata_func_type func, void* udata )
{
    struct dlistnode* node = *list;
    while( node ) {
	struct dlistnode* t = node;
	node = node->next;
	func( t, udata );
    }
}

static struct dlistnode* dlist_last( struct dlistnode* node )
{
    if( !node )
	return node;
    
    while( node->next ) {
	node = node->next;
    }
    return node;
}

struct dlistnode* dlist_popback( struct dlistnode** list ) {
    struct dlistnode* node = dlist_last(*list);
    if( node )
	dlist_erase( list, node );
    return node;
}

void dlist_foreach_reverse_udata( struct dlistnode** list, dlist_foreach_udata_func_type func, void* udata )
{
    struct dlistnode* node = dlist_last(*list);
    while( node ) {
	struct dlistnode* t = node;
	node = node->prev;
	func( t, udata );
    }
}

void dlist_pushfront_external( struct dlistnode** list, void* ptr )
{
    struct dlistnodeExternal* n = calloc(1,sizeof(struct dlistnodeExternal));
    n->ptr = ptr;
    dlist_pushfront( list, (struct dlistnode*)n );
}

static void freenode(struct dlistnode* node )
{
    free(node);
}

void dlist_free_external( struct dlistnode** list )
{
    if( !list || !(*list) )
	return;
    dlist_foreach( list, freenode );
}

void dlist_trim_to_limit( struct dlistnode** list, int limit, dlist_visitor_func_type f )
{
    int sz = dlist_size( list );
    while( sz >= limit ) {
	struct dlistnode* node = dlist_popback( list );
	f(node);
	freenode(node);
	sz = dlist_size( list );
    }
}

