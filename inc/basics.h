/* Copyright (C) 2000-2012 by George Williams */
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

#ifndef FONTFORGE_BASICS_H
#define FONTFORGE_BASICS_H

#include <fontforge-config.h>

#include <inttypes.h>
#include <limits.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>		/* for NULL */
#include <stdlib.h>		/* for free */
#include <string.h>

typedef uint32_t unichar_t;

#define FF_PI 3.1415926535897932

/* A macro to mark unused function parameters with. We often
 * have such parameters, because of extensive use of callbacks.
 */
#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

/* A macro to print a string for debug purposes
 */
#ifndef NDEBUG
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#else
#define TRACE(...) while(0)
#endif

/* assert() with an otherwise unused variable
 * to get around "unused" compiler warnings
 */
#ifndef NDEBUG
#define VASSERT(v) assert(v)
#else
#define VASSERT(v) ((void)(v))
#endif

extern void NoMoreMemMessage(void);

static inline int imin(int a, int b)
{
    return (a < b) ? a : b;
}

static inline int imax(int a, int b)
{
    return (a < b) ? b : a;
}

#define IS_IN_ORDER3( a, b, c )   ( ((a)<=(b)) && ((b)<=(c)) )


/**
 * Many lists in FontForge are singly linked. At times you might want
 * to append to the list which, when you only have a pointer to the
 * start of the list can be more verbose than one would like. To use
 * this macro you must defined a null initialized variable 'last'
 * outside of any loop that traverses the source list. The last
 * variable is used used by this macro to quickly append to the list
 * as you go. This macro also assumes that the 'last' and 'newitem'
 * types have a member "->next" which contains the single linked list
 * pointer to the next element.
 *
 * Efficient list append should really be a one line call in the bulk
 * of the code :)
 *
 * example:
 * MyListObjectType* newfoolast = 0;
 * MyListObjectType* newfoolist = 0;
 * 
 * for( ... iterate a source collection of foos ... )
 * {
 *    MyListObjectType* foocopy = CopyIt( foo );
 *    FFLIST_SINGLE_LINKED_APPEND( newfoolist, newfoolast, foocopy );
 * }
 */
#define FFLIST_SINGLE_LINKED_APPEND( head, last, newitem ) \
		    if ( !last )		       \
		    {				       \
			newitem->next = 0;	       \
			head = last = newitem;	       \
		    }				       \
		    else			       \
		    {				       \
			last->next = newitem;	       \
			last = newitem;		       \
		    }

#ifdef __GNU__
// This is for GNU Hurd, not GCC.
// cf. <https://www.gnu.org/software/hurd/community/gsoc/project_ideas/maxpath.html>
# ifndef PATH_MAX
#  define PATH_MAX 4096
# endif
# ifndef MAXPATHLEN
#  define MAXPATHLEN 4096
# endif
#endif

#endif /* FONTFORGE_BASICS_H */
