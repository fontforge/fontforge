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
#ifndef _BASICS_H
#define _BASICS_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>		/* for NULL */
#ifdef HAVE_STDINT_H
# include <stdint.h>
#else
# include <inttypes.h>
#endif
#include <stdlib.h>		/* for free */
#include <limits.h>

#define true 1
#define false 0

#define forever for (;;)

typedef int32_t		int32;
typedef uint32_t	uint32;
typedef int16_t		int16;
typedef uint16_t	uint16;
typedef int8_t		int8;
typedef uint8_t		uint8;

	/* An integral type which can hold a pointer */
typedef intptr_t	intpt;

#ifdef UNICHAR_16
typedef uint16 unichar_t;
#else
typedef uint32 unichar_t;
#endif

extern void *galloc(long size);
extern void *gcalloc(int cnt, long size);
extern void *grealloc(void *,long size);
extern void gfree(void *);
extern void galloc_set_trap(void (*)(void));
#endif
