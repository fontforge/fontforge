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
#ifndef _DLIST_H
#define _DLIST_H

/**
 * Doubly linked list abstraction. Putting a full member of this
 * struct first in another struct means you can treat it as a
 * dlinkedlist. You can have a struct in many lists simply by
 * embedding another dlistnode member and handing a pointer to that
 * member to the dlist() helper functions. Double linking has big
 * advantages in removal of single elements where you do not need to
 * rescan to find removeme->prev;
 */
struct dlistnode {
    struct dlistnode* next;
    struct dlistnode* prev;
};

struct dlistnodeExternal {
    struct dlistnode* next;
    struct dlistnode* prev;
    void* ptr;
};


extern void dlist_pushfront( struct dlistnode** list, struct dlistnode* node );
extern int  dlist_size( struct dlistnode** list );
extern int  dlist_isempty( struct dlistnode** list );
extern void dlist_erase( struct dlistnode** list, struct dlistnode* node );
typedef void (*dlist_foreach_func_type)( struct dlistnode* );
extern void dlist_foreach( struct dlistnode** list, dlist_foreach_func_type func );
typedef void (*dlist_foreach_udata_func_type)( struct dlistnode*, void* udata );
extern void dlist_foreach_udata( struct dlistnode** list, dlist_foreach_udata_func_type func, void* udata );

extern void dlist_pushfront_external( struct dlistnode** list, void* ptr );
extern void dlist_free_external( struct dlistnode** list );

#endif // _DLIST_H
