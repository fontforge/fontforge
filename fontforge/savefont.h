/* Copyright (C) 2007-2012 by George Williams */
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

#ifndef FONTFORGE_SAVEFONT_H
#define FONTFORGE_SAVEFONT_H

#include <fontforge-config.h>

#include "splinefont.h"

enum fm_flags { fm_flag_afm = 0x1,
                fm_flag_pfm = 0x2,
                fm_flag_shortps = 0x4,
                fm_flag_nopshints = 0x8,
                fm_flag_apple = 0x10,
                fm_flag_pfed_comments = 0x20,
                fm_flag_pfed_colors = 0x40,
                fm_flag_opentype = 0x80,
                fm_flag_glyphmap = 0x100,
                fm_flag_TeXtable = 0x200,
                fm_flag_ofm = 0x400,
                fm_flag_applemode = 0x800,
                // fm_flag_??? = 0x1000,
                fm_flag_symbol = 0x2000,
                fm_flag_dummyDSIG = 0x4000,
                // fm_flag_??? = 0x8000,
                fm_flag_tfm = 0x10000,
                fm_flag_nohintsubs = 0x20000,
                fm_flag_noflex = 0x40000,
                fm_flag_nottfhints = 0x80000,
                fm_flag_restrict256 = 0x100000,
                fm_flag_round = 0x200000,
                fm_flag_afmwithmarks = 0x400000,
                fm_flag_pfed_lookups = 0x800000,
                fm_flag_pfed_guides = 0x1000000,
                fm_flag_pfed_layers = 0x2000000,
                fm_flag_winkern = 0x4000000
              };

extern const char (*savefont_extensions[]), (*bitmapextensions[]);
extern int old_sfnt_flags;

void PrepareUnlinkRmOvrlp(SplineFont *sf,const char *filename,int layer);
void RestoreUnlinkRmOvrlp(SplineFont *sf,const char *filename,int layer);
int _DoSave(SplineFont *sf,char *newname,int32 *sizes,int res,
	EncMap *map, char *subfontdefinition,int layer);
int CheckIfTransparent(SplineFont *sf);

extern int GenerateScript(SplineFont *sf, char *filename, const char *bitmaptype, int fmflags, int res, char *subfontdirectory, struct sflist *sfs, EncMap *map, NameList *rename_to, int layer);

#ifdef FONTFORGE_CONFIG_WRITE_PFM
extern int WritePfmFile(char *filename, SplineFont *sf, EncMap *map, int layer);
#endif

#endif /* FONTFORGE_SAVEFONT_H */
