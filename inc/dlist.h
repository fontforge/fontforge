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

#ifndef FONTFORGE_DLIST_H
#define FONTFORGE_DLIST_H

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

/**
 * DEVELOPERS: make sure the start of this struct is compatible with
 * dlistnode. While I could use the dlistnode as a first member, using
 * a copy of the members in the same order as dlistnode has them
 * allows callers using this struct a bit simpler access.
 * 
 * While one can embed a dlistnode member into a struct to create
 * linked lists, sometimes you want to return a splice of one of those
 * lists. For example, if you have a double linked list of all your
 * hotkeys, you might like to return only the ones that have a
 * modifier of the Control key. You want to leave the hotkey structs
 * in their original list, but create a new kust that references just
 * a desired selection of objects.
 *
 * In other words, if you have some data you want to return in a
 * double linked list, then use this node type. You can build one up
 * using dlist_pushfront_external() and the caller can free that list
 * using dlist_free_external(). Any of the foreach() functions will
 * work to iterate a list of dlistnodeExternal as this list is
 * identical to a dlistnode with an extra ptr payload.
 */
struct dlistnodeExternal {
    struct dlistnode* next;
    struct dlistnode* prev;
    void* ptr;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Push the node onto the head of the list
 */
extern void dlist_pushfront( struct dlistnode** list, struct dlistnode* node );

/**
 * Take the last node off the list and return it. If the list is empty, return 0.
 */
struct dlistnode* dlist_popback( struct dlistnode** list );

/**
 * the number of nodes in the list
 */
extern int  dlist_size( struct dlistnode** list );

/**
 * is the list empty
 */
extern int  dlist_isempty( struct dlistnode** list );

/**
 * Remove the node from the list. The node itself is not free()ed.
 * That is still up to the caller. All this function does is preserve
 * the list structure without the node being in it.
 */
extern void dlist_erase( struct dlistnode** list, struct dlistnode* node );
typedef void (*dlist_foreach_func_type)( struct dlistnode* );

/**
 * Call func for every node in the list. This is a defensive
 *  implementation, if you want to remove a node from the list inside
 *  func() that is perfectly fine.
 */
extern void dlist_foreach( struct dlistnode** list, dlist_foreach_func_type func );
typedef void (*dlist_foreach_udata_func_type)( struct dlistnode*, void* udata );

/**
 * Like dlist_foreach(), defensive coding still, but the udata pointer
 * is passed back to your visitor function.
 */
extern void dlist_foreach_udata( struct dlistnode** list, dlist_foreach_udata_func_type func, void* udata );

/**
 * Like dlist_foreach_udata() but nodes are visited in reverse order.
 */
extern void dlist_foreach_reverse_udata( struct dlistnode** list, dlist_foreach_udata_func_type func, void* udata );

/**
 * Assuming list is an externalNode list, push a newly allocated list node with
 * a dlistnodeExternal.ptr = ptr passed.
 */
extern void dlist_pushfront_external( struct dlistnode** list, void* ptr );

/**
 * Free a list of externalNode type. The externalNode memory is
 * free()ed, whatever externalNode.ptr is pointing to is not free()ed.
 */
extern void dlist_free_external( struct dlistnode** list );


typedef void (*dlist_visitor_func_type)( struct dlistnode* );

/**
 * To create a list of bounded length, use this function. Limit is the
 * maximum length the list can reach. If list nodes have to be removed
 * to be under this limit then "f" is used as a callback to free list
 * nodes. This allows application specific freeing of a list node, and
 * the ability to maintain a limit on the length of a list as a simple
 * one line call.
 *
 * The current implementation expects you to only be trimming one or
 * two entries at a time. It will still work for trimming 100 entries
 * at a single time, but might not be quite as optimized for that case
 * as it could be.
 */
extern void dlist_trim_to_limit( struct dlistnode** list, int limit, dlist_visitor_func_type f );

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_DLIST_H */

