/* Copyright (C) 2001 by George Williams */
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
 
#ifndef _TTFMOD_CONFIG_H
#define _TTFMOD_CONFIG_H

 /****************************************************************************/
 /*			   USER CONFIGURATION OPTIONS			     */
 /****************************************************************************/


/* If you change any of these, do a "$ make clean" before building */

#ifndef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
/* I have two levels of behavior for dealing with Apple's TTF patents */
/* Level 1 => You are not affected by these patents (if you're licensed by */
/*		apple, or live somewhere that they don't apply) In this mode */
/*	1) TtfMod will show you the splines before and after they are grid fit*/
/*	2) TtfMod will show you the generated raster for that grid */
/*  To use this mode you MUST also compile freetype with		*/
/*		TT_CONFIG_OPTION_BYTECODE_INTERPRETER			*/
/*  defined in {include/freetype/config,builds/<system>}/ftoption.h	*/
/*  (Yes, I decided to use the same name to make the connection clear)  */
/* Level 0 => You are affected by Apple's patents */
/*	TtfMod will not show you the grid fit splines or the resultant rasters*/

/* As I read the patents they apply to ttfmod as much as they do to the freetype */
/* library (which I used to do the gridfitting when allowed). See FreeType's */
/* patent page */
/*	http://freetype.sourceforge.ent/patents2.html */
/* or contact Apple's legal department: patents@apple.com */

/* The default behavior is to presume that you are affected by the patents */
/*  if you are not, then change the following #defined constant to 1 */
# define TT_CONFIG_OPTION_BYTECODE_INTERPRETER	0
#endif

#ifndef TT_CONFIG_OPTION_BYTECODE_DEBUG
/* I also have my own byte code interpreter. It has three modes and is */
/*  dependent on both TT_CONFIG_OPTION_BYTECODE_INTERPRETER and the above flag*/
/*  If the above flag is off then there will be no byte code debugger or */
/*  interpreter in the program. If it is on and TT_CONFIG_OPTION_BYTECODE_INTERPRETER */
/*  is off then the interpreter will run but will not actually move any points*/
/*  (it just figures out what points would be moved). If both are on then it */
/*  will be a full interpreter and will (probably) be in violation of the patent */
/* I believe that the intermediate mode is perfectly legal, so I leave it on */
/*  by default. */
# define TT_CONFIG_OPTION_BYTECODE_DEBUG	1
#endif

#if TT_CONFIG_OPTION_BYTECODE_DEBUG && TT_CONFIG_OPTION_BYTECODE_INTERPRETER
# ifndef TT_CONFIG_OPTION_FREETYPE_DEBUG
/* If you have freetype with the bytecode interpreter enabled, and you have */
/*  the freetype sources on your machine then, then you should define this */
/*  flag to be 1. You should also add					   */
/*		-I<freetype2-top-dir>/src/truetype			   */
/*  to your CFLAGS. If set then ttfmod will use freetype's built-in debugger */
/*  as the basis of it's own debugging (it needs access to internal freetype */
/*  include files, hence all the complexity).				   */
/* I leave this off by default because most people don't have the freetype */
/*  sources on their machines. */
#  define TT_CONFIG_OPTION_FREETYPE_DEBUG 0
# endif
#endif

#ifndef TT_RASTERIZE_FONTVIEW
/* If you do not want to link with FreeType at all then turn this off. When */
/*  on the fontview will use FreeType to rasterize bitmaps from the truetype */
/*  to display in the fontview. When off the fontview will just pick a bitmap */
/*  font already on your system and display glyphs from it. FreeType does not */
/*  need the bytecode interpreter for this */
# define TT_RASTERIZE_FONTVIEW 1
#endif


 /****************************************************************************/
 /*			END OF USER CONFIGURATION OPTIONS			     */
 /****************************************************************************/
#endif
