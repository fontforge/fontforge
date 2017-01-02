/* Copyright (C) 2002-2012 by George Williams */
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
#ifndef _CONFIG_FONTFORGE_H_
#define _CONFIG_FONTFORGE_H_




/* I used to use an approximation method when converting cubic to quadratic   */
/*  splines which was non-symmetric. In some cases it produced better results */
/*  than the current approach. This flag restores the old algorithm.	      */
/*									      */
/* #define FONTFORGE_CONFIG_NON_SYMMETRIC_QUADRATIC_CONVERSION		      */
/*									      */


/* Prior to late Sept of 2003 FontForge converted certain mac feature/settings*/
/*  into opentype-like tags. Some features could be converted directly but for*/
/*  a few I made up tags.  Now FontForge is capable of using the mac feature  */
/*  settings directly. If you set this flag then when FontForge loads in an sfd*/
/*  file with these non-standard opentype tags, it will convert them into the */
/*  appropriate mac feature/setting combinations.                             */
/*									      */
/* #define FONTFORGE_CONFIG_CVT_OLD_MAC_FEATURES			      */
/*									      */


/* In addition to placing snippets of charstrings into subrs, I tried adding  */
/*  whole glyphs (when that was possible). To my surprise, it made things     */
/*  worse in one of my test cases, and barely registered an improvement in    */
/*  another.   So I think we're better off without it. But I don't understand */
/*  why things are worse so I'm leaving the code in to play with              */
/*									      */
/* #define FONTFORGE_CONFIG_PS_REFS_GET_SUBRS				      */
/*									      */


/* ************************************************************************** */
/* **************************** Numeric Settings **************************** */
/* ************************************************************************** */

#endif
