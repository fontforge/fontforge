/* Copyright (C) 2002 by George Williams */
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
#ifndef _CONFIG_PFAEDIT_H_
#define _CONFIG_PFAEDIT_H_

/* I doubt anyone but me will be interested in the tile path command, so I    */
/*  don't include code for it by default, but if you do want it simply define */
/*  the following macro							      */
/*									      */
/* #define PFAEDIT_CONFIG_TILEPATH					      */
/*									      */
/* The tile path command uses the contents of the clipboard as a tile which is*/
/*  applied to any selected paths, replacing them. It could be used to make a */
/*  font whose stems were filled with celtic knotwork, for example */


/* There used to be a property _XFREE86_GLYPH_RANGES (in bdf/pcf) fonts which */
/*  gave a quick view about what glyphs were in a bdf font. From what I gather*/
/*  this property has been dropped because it was redundant.  If you would    */
/*  like pfaedit to generate it						      */
/*									      */
/* #define PFAEDIT_CONFIG_BDF_GLYPH_RANGES				      */
/*									      */


/* Harald Harders would like to be able to generate a PFM file without        */
/*  creating a font along with it. I don't see the need for this, but he pro- */
/*  vided a patch. Setting this flag will enable his patch		      */
/*									      */
/* #define PFAEDIT_CONFIG_WRITE_PFM					      */
/*									      */

#endif
